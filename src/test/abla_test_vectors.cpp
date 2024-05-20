// Copyright (c) 2023 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "consensus/abla.h"
#include "data/abla_test_vectors.json.h"
#include "jsonutil.h"
#include "test_bitcoin.h"
#include <univalue.h>
#include "utilstrencodings.h"

#include <boost/test/unit_test.hpp>

#include <string_view>

BOOST_FIXTURE_TEST_SUITE(abla_test_vectors, BasicTestingSetup)

static uint64_t Parse64(const UniValue &o, std::string_view key)
{
    uint64_t ret;
    BOOST_REQUIRE(ParseUInt64(o[std::string(key)].getValStr(), &ret));
    return ret;
}

struct TestAblaState
{
    uint64_t n{};
    uint64_t epsilon{};
    uint64_t beta{};

    TestAblaState() = default;
    TestAblaState(const UniValue &o) { *this = o; }

    TestAblaState &operator=(const UniValue &o)
    {
        n = Parse64(o, "n");
        epsilon = Parse64(o, "epsilon");
        beta = Parse64(o, "beta");
        return *this;
    }
};

static void RunTest(size_t testNum, const UniValue &test)
{
    auto msg = strprintf("Running test #%i", testNum);
    if (test.exists("testName"))
    {
        UniValue value = test["testName"];
        if (value.isStr())
        {
            msg += strprintf("\n    Name: %s", value.get_str());
        }
    }
    if (test.exists("testDescription"))
    {
        UniValue value = test["testDescription"];
        if (value.isStr())
        {
            msg += strprintf("\n    Description: %s", value.get_str());
        }
    }
    BOOST_TEST_MESSAGE(msg);

    // Load config
    auto dump_msg = strprintf("    Top-level params:");
    const auto &conf_obj = test["ABLAConfig"].get_obj();
    dump_msg += strprintf("\n        ABLAConfig: %s", UniValue::stringify(conf_obj));
    dump_msg += strprintf("\n        ABLAStateInitial: %s", UniValue::stringify(test["ABLAStateInitial"]));
    dump_msg += strprintf("\n        blocksizeLimitInitial: %s", UniValue::stringify(test["blocksizeLimitInitial"]));
    BOOST_TEST_MESSAGE(dump_msg);
    abla::Config config;
    config.epsilon0 = Parse64(conf_obj, "epsilon0");
    config.beta0 = Parse64(conf_obj, "beta0");
    config.zeta_xB7 = Parse64(conf_obj, "zeta");
    config.gammaReciprocal = Parse64(conf_obj, "gammaReciprocal");
    config.delta = Parse64(conf_obj, "delta");
    config.thetaReciprocal = Parse64(conf_obj, "thetaReciprocal");
    config.SetMax();
    BOOST_REQUIRE(config.IsValid());

    // Parse n0
    const uint64_t n0 = Parse64(conf_obj, "n0");
    // Parse disable2GBLimit flag
    bool disable2GBLimit = false;
    if (conf_obj.exists("options"))
    {
        disable2GBLimit = conf_obj["options"].get_str().find("-disable2GBLimit") != std::string::npos;
    }

    // Set up initial state
    const TestAblaState initial_tstate = test["ABLAStateInitial"].get_obj();
    abla::State state = abla::State::FromTuple({0, initial_tstate.epsilon, initial_tstate.beta});
    BOOST_REQUIRE(state.IsValid(config));
    const uint64_t initial_bsLimit = Parse64(test, "blocksizeLimitInitial");
    uint64_t bsLimit = initial_bsLimit, blockSize{}, bsLimitNext{};
    uint64_t n = initial_tstate.n;
    TestAblaState tstate = initial_tstate;
    const UniValue &tv_array = test["testVector"].get_array();

    if (tv_array.size() == 1 && tv_array[0].isObject() && tv_array[0].exists("lookahead"))
    {
        // "lookahead" test; only 1 item and it describes where the algo activates and how far to look ahead

        const UniValue &o = tv_array[0].get_obj();
        const uint64_t lookahead = Parse64(o, "lookahead");
        BOOST_REQUIRE(lookahead > 0);
        BOOST_REQUIRE_EQUAL(bsLimit, tstate.epsilon + tstate.beta);
        BOOST_REQUIRE_EQUAL(state.GetBlockSizeLimit(disable2GBLimit), bsLimit);
        const uint64_t final_bsLimit = Parse64(o, "blocksizeLimitForLookaheadBlock");
        const TestAblaState final_tstate = o["ABLAStateForLookaheadBlock"].get_obj();
        BOOST_REQUIRE_EQUAL(final_bsLimit, final_tstate.epsilon + final_tstate.beta);
        BOOST_REQUIRE_EQUAL(tstate.n + lookahead, final_tstate.n);

        tstate.n = std::max(n0, tstate.n); // simulate "advancement" to the activation block
        if (tstate.n < final_tstate.n)
        {
            // Test vector only really does something if we activated.
            // If so call the lookahead function CalcLookaheadBlockSizeLimit, and compare result of that to test vector.
            state = abla::State::FromTuple({bsLimit, tstate.epsilon, tstate.beta}); // test calls for initial block to be "full"
            const uint64_t lookahead_result = state.CalcLookaheadBlockSizeLimit(config, final_tstate.n - tstate.n, disable2GBLimit);
            BOOST_CHECK_EQUAL(lookahead_result, final_bsLimit);
        }
    }
    else
    {
        // regular test vector, describing each step of the algo state as blocks are simulated.

        auto do_checks_and_advance = [&](bool pastTheEnd = false)
        {
            BOOST_REQUIRE_EQUAL(n, tstate.n);
            {
                // Update state with real blockSize
                const auto & [bs, epsilon, beta] = state.ToTuple();
                state = abla::State::FromTuple({blockSize, epsilon, beta});
            }
            BOOST_CHECK_EQUAL(bsLimit, state.GetBlockSizeLimit(disable2GBLimit));
            BOOST_CHECK_EQUAL(state.GetControlBlockSize(), tstate.epsilon);
            BOOST_CHECK_EQUAL(state.GetElasticBufferSize(), tstate.beta);
            if (n >= n0)
            {
                // post-activation, advance the state
                if (!pastTheEnd)
                {
                    BOOST_CHECK_EQUAL(state.GetNextBlockSizeLimit(config, disable2GBLimit), bsLimitNext);
                }
                state = state.NextBlockState(config, blockSize);
            }
            else
            {
                if (!pastTheEnd)
                {
                    BOOST_CHECK_EQUAL(initial_bsLimit, bsLimitNext);
                }
            }
            BOOST_CHECK(state.IsValid(config));
        };

        const std::vector<UniValue>& values = tv_array.getValues();
        for (const UniValue &uv : values)
        {
            auto tst_msg = strprintf("N: %i", n);
            const UniValue &o = uv.get_obj();
            tst_msg += strprintf(" Testing: %s", UniValue::stringify(o));
            BOOST_TEST_MESSAGE(tst_msg);
            blockSize = Parse64(o, "blocksize");
            bsLimitNext = Parse64(o, "blocksizeLimitForNextBlock");
            const TestAblaState nextState = o["ABLAStateForNextBlock"].get_obj();
            do_checks_and_advance();
            tstate = nextState;
            bsLimit = bsLimitNext;
            ++n;
        }
        do_checks_and_advance(true); // run the checks one last time after the last vector
    }
}

BOOST_AUTO_TEST_CASE(test_all)
{
    const char * const content = reinterpret_cast<const char *>(json_tests::abla_test_vectors);
    const UniValue tests = read_json({content, std::size(json_tests::abla_test_vectors)});
    size_t testNum = 1;
    const std::vector<UniValue>& vtests = tests.getValues();
    for (const auto &test : vtests)
    {
        BOOST_REQUIRE(test.isObject());
        RunTest(testNum++, test.get_obj());
    }
}

BOOST_AUTO_TEST_SUITE_END()
