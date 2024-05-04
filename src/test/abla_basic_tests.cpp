// Copyright (c) 2023 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/abla.h>

#include <random.h>
#include <serialize.h>
#include <streams.h>
#include <test/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <limits>
#include <string_view>

BOOST_FIXTURE_TEST_SUITE(abla_basic_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(config_ctor)
{
    const abla::Config config;

    // Test that default-constructed config is all 0's and is not valid
    BOOST_CHECK(!config.IsValid());
    BOOST_CHECK_EQUAL(config.epsilon0, 0);
    BOOST_CHECK_EQUAL(config.beta0, 0);
    BOOST_CHECK_EQUAL(config.gammaReciprocal, 0);
    BOOST_CHECK_EQUAL(config.zeta_xB7, 0);
    BOOST_CHECK_EQUAL(config.thetaReciprocal, 0);
    BOOST_CHECK_EQUAL(config.delta, 0);
    BOOST_CHECK_EQUAL(config.epsilonMax, 0);
    BOOST_CHECK_EQUAL(config.betaMax, 0);
}

BOOST_AUTO_TEST_CASE(config_isvalid)
{
    const abla::Config config = abla::Config::MakeDefault();
    const char *err = nullptr;

    BOOST_CHECK(config.IsValid(&err) && config.IsValid()); // test IsValid both with and without the err arg
    BOOST_CHECK_EQUAL(err, "");

    // Test default-constructed Config (all zeros) should be invalid
    BOOST_CHECK(!abla::Config{}.IsValid(&err) && !abla::Config{}.IsValid());
    BOOST_CHECK_NE(err, "");

    // Test IsValid on a bad Config: epsilon0 > epsilonMax
    auto bad = config;
    bad.epsilon0 = config.epsilonMax + 1u;
    BOOST_CHECK(!bad.IsValid(&err) && !bad.IsValid()); // test negative result from both forms of IsValid()
    BOOST_CHECK(std::string_view{err}.find("epsilonMax") != std::string_view::npos);
    bad = config;
    bad.epsilonMax = config.epsilon0 - 1u;
    BOOST_CHECK(!bad.IsValid(&err) && !bad.IsValid());
    BOOST_CHECK(std::string_view{err}.find("epsilonMax") != std::string_view::npos);

    // Test IsValid on a bad Config: beta0 > betaMax
    bad = config;
    bad.beta0 = config.betaMax + 1u;
    BOOST_CHECK(!bad.IsValid(&err) && !bad.IsValid());
    BOOST_CHECK(std::string_view{err}.find("betaMax") != std::string_view::npos);
    bad = config;
    bad.betaMax = config.beta0 - 1u;
    BOOST_CHECK(!bad.IsValid(&err) && !bad.IsValid());
    BOOST_CHECK(std::string_view{err}.find("betaMax") != std::string_view::npos);

    // Test IsValid on a bad Config: zeta_xB7 out of range
    bad = config;
    bad.zeta_xB7 = abla::MIN_ZETA_XB7 - 1u;
    BOOST_CHECK(!bad.IsValid(&err) && !bad.IsValid());
    BOOST_CHECK(std::string_view{err}.find("zeta") != std::string_view::npos);
    bad = config;
    bad.zeta_xB7 = abla::MAX_ZETA_XB7 + 1u;
    BOOST_CHECK(!bad.IsValid(&err) && !bad.IsValid());
    BOOST_CHECK(std::string_view{err}.find("zeta") != std::string_view::npos);

    // Test Isvalid on a bad Config: gammaReciprocal out of range
    bad = config;
    bad.gammaReciprocal = abla::MIN_GAMMA_RECIPROCAL - 1u;
    BOOST_CHECK(!bad.IsValid(&err) && !bad.IsValid());
    BOOST_CHECK(std::string_view{err}.find("gammaReciprocal") != std::string_view::npos);
    bad = config;
    bad.gammaReciprocal = abla::MAX_GAMMA_RECIPROCAL + 1u;
    BOOST_CHECK(!bad.IsValid(&err) && !bad.IsValid());
    BOOST_CHECK(std::string_view{err}.find("gammaReciprocal") != std::string_view::npos);

    // Test Isvalid on a bad Config: delta out of range
    bad = config;
    bad.delta = abla::MIN_DELTA - 1u;
    BOOST_CHECK(!bad.IsValid(&err) && !bad.IsValid());
    BOOST_CHECK(std::string_view{err}.find("delta") != std::string_view::npos);
    bad = config;
    bad.delta = abla::MAX_DELTA + 1u;
    BOOST_CHECK(!bad.IsValid(&err) && !bad.IsValid());
    BOOST_CHECK(std::string_view{err}.find("delta") != std::string_view::npos);

    // Test Isvalid on a bad Config: thetaReciprocal out of range
    bad = config;
    bad.thetaReciprocal = abla::MIN_THETA_RECIPROCAL - 1u;
    BOOST_CHECK(!bad.IsValid(&err) && !bad.IsValid());
    BOOST_CHECK(std::string_view{err}.find("thetaReciprocal") != std::string_view::npos);
    bad = config;
    bad.thetaReciprocal = abla::MAX_THETA_RECIPROCAL + 1u;
    BOOST_CHECK(!bad.IsValid(&err) && !bad.IsValid());
    BOOST_CHECK(std::string_view{err}.find("thetaReciprocal") != std::string_view::npos);

    // Test Isvalid on a bad Config: epsilon0 sanity check w.r.t. gammaReciprocal
    bad = config;
    bad.epsilon0 = config.gammaReciprocal * abla::B7 / (config.zeta_xB7 - abla::B7);
    // first ensure it's ok at the limit
    BOOST_CHECK(bad.IsValid(&err) && bad.IsValid());
    BOOST_CHECK_EQUAL(err, "");
    // next below the limit it should fail
    bad.epsilon0--;
    BOOST_CHECK(!bad.IsValid(&err) && !bad.IsValid());
    BOOST_CHECK(std::string_view{err}.find("epsilon0 sanity check") != std::string_view::npos);
}

BOOST_AUTO_TEST_CASE(config_setmax)
{
    auto config = abla::Config::MakeDefault();

    BOOST_CHECK_NE(config.epsilonMax, 0u);
    BOOST_CHECK_NE(config.betaMax, 0u);

    config.epsilonMax = config.betaMax = 0u;
    BOOST_CHECK(!config.IsValid());

    // Test that SetMax works and does something (sets to some huge value)
    config.SetMax();
    BOOST_CHECK(config.IsValid());
    BOOST_CHECK_GT(config.epsilonMax, std::numeric_limits<uint32_t>::max());
    BOOST_CHECK_GT(config.betaMax, std::numeric_limits<uint32_t>::max());
}

BOOST_AUTO_TEST_CASE(config_tostring)
{
    abla::Config config;
    config.epsilon0 = 1;
    config.beta0 = 2;
    config.gammaReciprocal = 3;
    config.zeta_xB7 = 4;
    config.thetaReciprocal = 5;
    config.delta = 6;
    config.epsilonMax = 7;
    config.betaMax = 8;

    BOOST_CHECK_EQUAL(config.ToString(),
                      "abla::Config(epsilon0=1, beta0=2, gammaReciprocal=3, zeta_xB7=4, thetaReciprocal=5, delta=6,"
                      " epsilonMax=7, betaMax=8)");
}

BOOST_AUTO_TEST_CASE(state_ctors)
{
    const abla::Config config = abla::Config::MakeDefault();

    // Test that default-constructed state is invalid and is all zeores (only really suitable for serialization)
    const abla::State state0;
    BOOST_CHECK(!state0.IsValid(config));
    BOOST_CHECK_EQUAL(state0.GetBlockSize(), 0u);
    BOOST_CHECK_EQUAL(state0.GetControlBlockSize(), 0u);
    BOOST_CHECK_EQUAL(state0.GetElasticBufferSize(), 0u);

    // Non-default constructed State however is valid and has some default values from Config
    const abla::State state(config, 12345u);
    BOOST_CHECK(state.IsValid(config));
    BOOST_CHECK_EQUAL(state.GetBlockSize(), 12345u);
    BOOST_CHECK_EQUAL(state.GetControlBlockSize(), config.epsilon0);
    BOOST_CHECK_EQUAL(state.GetElasticBufferSize(), config.beta0);
}

BOOST_AUTO_TEST_CASE(state_getblocksizelimit)
{
    const abla::Config config = abla::Config::MakeDefault();
    abla::State state(config, 12345u);

    // Basic test -- value should be what we expect
    BOOST_REQUIRE(state.IsValid(config));
    BOOST_CHECK_EQUAL(state.GetBlockSizeLimit(false), state.GetControlBlockSize() + state.GetElasticBufferSize());
    BOOST_CHECK_EQUAL(state.GetBlockSizeLimit(true), state.GetControlBlockSize() + state.GetElasticBufferSize());

    // Try the test with the control state advanced close to the 2GB limit and test the `disable2GBCap` flag
    const uint64_t hugeCBS = config.epsilonMax / 3u, hugeEBS = config.betaMax / 3u;
    BOOST_REQUIRE_GT(hugeCBS, MAX_CONSENSUS_BLOCK_SIZE);
    BOOST_REQUIRE_GT(hugeEBS, MAX_CONSENSUS_BLOCK_SIZE);
    state = abla::State::FromTuple({6789u, hugeCBS, hugeEBS});
    BOOST_REQUIRE(state.IsValid(config));
    BOOST_CHECK_EQUAL(state.GetControlBlockSize(), hugeCBS);
    BOOST_CHECK_EQUAL(state.GetElasticBufferSize(), hugeEBS);
    BOOST_CHECK_GT(state.GetControlBlockSize() + state.GetElasticBufferSize(), MAX_CONSENSUS_BLOCK_SIZE);
    // test effect of the `disable2GBCap` flag (defaults to false but we test that the default is false too)
    BOOST_CHECK_EQUAL(state.GetBlockSizeLimit(), MAX_CONSENSUS_BLOCK_SIZE);
    BOOST_CHECK_EQUAL(state.GetBlockSizeLimit(false), MAX_CONSENSUS_BLOCK_SIZE);
    BOOST_CHECK_EQUAL(state.GetBlockSizeLimit(true), state.GetControlBlockSize() + state.GetElasticBufferSize());
}

BOOST_AUTO_TEST_CASE(state_algo_blocksize_exceeds_max_block_size)
{
    // Try a corner case: the state's blockSize exceeds what the algo expects (it should clamp the size used internally)
    const abla::Config config = abla::Config::MakeDefault();
    const uint64_t hugeCBS = config.epsilonMax / 3u, hugeEBS = config.betaMax / 3u;
    const auto state = abla::State::FromTuple({6789u, hugeCBS, hugeEBS});
    BOOST_REQUIRE(state.IsValid(config));
    for (const bool disable2GBCap : {false, true})
    {
        // test at max
        const uint64_t delta = 1'000'000'000u;
        const uint64_t exceedsMax = state.GetBlockSizeLimit(disable2GBCap) + delta;
        const auto overSaturated = abla::State::FromTuple({exceedsMax, hugeCBS, hugeEBS});
        BOOST_REQUIRE(overSaturated.IsValid(config));
        if (disable2GBCap)
        {
            BOOST_CHECK_EQUAL(exceedsMax,
                              delta + overSaturated.GetControlBlockSize() + overSaturated.GetElasticBufferSize());
        }
        else
        {
            BOOST_CHECK_EQUAL(exceedsMax,
                              delta + MAX_CONSENSUS_BLOCK_SIZE);
        }
        const auto saturated = abla::State::FromTuple({exceedsMax - delta, hugeCBS, hugeEBS});
        BOOST_CHECK(overSaturated != saturated);
        const auto osNext = overSaturated.NextBlockState(config, 123u);
        const auto sNext = saturated.NextBlockState(config, 123u);
        if (disable2GBCap)
        {
            BOOST_CHECK(osNext == sNext);
        }
        else
        {
            BOOST_CHECK(osNext != sNext);
        }
        BOOST_CHECK_EQUAL(overSaturated.GetNextBlockSizeLimit(config, disable2GBCap),
                          saturated.GetNextBlockSizeLimit(config, disable2GBCap));
        BOOST_CHECK_EQUAL(sNext.GetBlockSizeLimit(disable2GBCap),
                          saturated.GetNextBlockSizeLimit(config, disable2GBCap));
        BOOST_CHECK_EQUAL(osNext.GetBlockSizeLimit(disable2GBCap),
                          overSaturated.GetNextBlockSizeLimit(config, disable2GBCap));

        // Test that abuse of algo with "overLimit" blockSize values such as u64max leads to "clamping" of the
        // blockSize used internally for advancing the state, and no overflows occur, and it's the same as if just the
        // maxBlockSize were used.
        const auto u64max = std::numeric_limits<uint64_t>::max();
        for (const uint64_t overLimit : {(hugeCBS + hugeEBS) * 2, (hugeCBS + hugeEBS) * 3, u64max})
        {
            auto mstate = abla::State::FromTuple({overLimit, hugeCBS, hugeEBS});
            BOOST_REQUIRE(mstate.IsValid(config));
            const auto N = 2048;
            const auto lookaheadLimit = mstate.CalcLookaheadBlockSizeLimit(config, N, disable2GBCap);
            for (size_t i = 0; i < N; ++i)
            {
                // this forces overLimit to the algo for all in-between states
                mstate = mstate.NextBlockState(config, overLimit);
            }
            // end result should still be the same as if the max block size per state was used, not overLimit
            BOOST_CHECK_EQUAL(lookaheadLimit, mstate.GetBlockSizeLimit(disable2GBCap));
        }
    }
}

BOOST_AUTO_TEST_CASE(state_isvalid)
{
    const abla::Config config = abla::Config::MakeDefault();
    const char *err = nullptr;

    abla::State state(config, 80);

    BOOST_CHECK(state.IsValid(config, &err));
    BOOST_CHECK_EQUAL(err, "");

    // Invalid due to controlBlockSize < config.epsilon0
    const auto badCBS = config.epsilon0 - 1;
    state = abla::State::FromTuple({80, badCBS, config.beta0});
    BOOST_CHECK_EQUAL(state.GetControlBlockSize(), badCBS);
    BOOST_CHECK(!state.IsValid(config, &err));
    BOOST_CHECK(std::string_view{err}.find("invalid controlBlockSize state") != std::string_view::npos);
    err = "";

    // Invalid due to controlBlockSize > config.epsilonMax
    const auto badCBS2 = config.epsilonMax + 1;
    state = abla::State::FromTuple({80, badCBS2, config.beta0});
    BOOST_CHECK_EQUAL(state.GetControlBlockSize(), badCBS2);
    BOOST_CHECK(!state.IsValid(config, &err));
    BOOST_CHECK(std::string_view{err}.find("invalid controlBlockSize state") != std::string_view::npos);
    err = "";

    // Invalid due to elasticBufferSize < config.beta0
    const auto badEBS = config.beta0 - 1;
    state = abla::State::FromTuple({80, config.epsilon0, badEBS});
    BOOST_CHECK_EQUAL(state.GetElasticBufferSize(), badEBS);
    BOOST_CHECK(!state.IsValid(config, &err));
    BOOST_CHECK(std::string_view{err}.find("invalid elasticBufferSize state") != std::string_view::npos);
    err = "";

    // Invalid due to elasticBufferSize > config.betaMax
    const auto badEBS2 = config.betaMax + 1;
    state = abla::State::FromTuple({80, config.epsilon0, badEBS2});
    BOOST_CHECK_EQUAL(state.GetElasticBufferSize(), badEBS2);
    BOOST_CHECK(!state.IsValid(config, &err));
    BOOST_CHECK(std::string_view{err}.find("invalid elasticBufferSize state") != std::string_view::npos);
    err = "";
}

BOOST_AUTO_TEST_CASE(state_tostring)
{
    const abla::Config config = abla::Config::MakeDefault();
    const abla::State state(config, 80u);

    BOOST_CHECK_EQUAL(state.ToString(),
                      "abla::State(blockSize=80, controlBlockSize=16000000, elasticBufferSize=16000000)");
}

BOOST_AUTO_TEST_CASE(state_tuple)
{
    const auto state = abla::State::FromTuple({1, 2, 3});
    BOOST_CHECK_EQUAL(state.GetBlockSize(), 1);
    BOOST_CHECK_EQUAL(state.GetControlBlockSize(), 2);
    BOOST_CHECK_EQUAL(state.GetElasticBufferSize(), 3);

    const auto tup = state.ToTuple();
    BOOST_CHECK(tup == std::tuple(1, 2, 3));
}

BOOST_AUTO_TEST_CASE(state_ser)
{
    FastRandomContext rng;
    const abla::Config config = abla::Config::MakeDefault();

    for (size_t i = 0; i < 10; ++i)
    {
        const uint64_t randomSize = rng.rand64(),
                       randomCBS = rng.randrange(config.epsilonMax),
                       randomEBS = rng.randrange(config.betaMax);
        const abla::State state = abla::State::FromTuple({randomSize, randomCBS, randomEBS});
        BOOST_REQUIRE(state.IsValid(config));
        BOOST_REQUIRE_EQUAL(state.GetBlockSize(), randomSize);
        BOOST_REQUIRE_EQUAL(state.GetControlBlockSize(), randomCBS);
        BOOST_REQUIRE_EQUAL(state.GetElasticBufferSize(), randomEBS);

        // test ser/deser cycle
        std::vector<uint8_t> v1;
        CVectorWriter(SER_DISK, INIT_PROTO_VERSION, v1, 0) << state;
        BOOST_CHECK(!v1.empty());
        abla::State state2; // default constructed should be all 0's
        BOOST_CHECK_EQUAL(state2.GetBlockSize(), 0);
        BOOST_CHECK_EQUAL(state2.GetControlBlockSize(), 0);
        BOOST_CHECK_EQUAL(state2.GetElasticBufferSize(), 0);
        // after unser, state2 should equal state
        VectorReader(SER_DISK, INIT_PROTO_VERSION, v1, 0) >> state2;
        BOOST_CHECK(state == state2);
        BOOST_CHECK_EQUAL(state2.GetBlockSize(), randomSize);
        BOOST_CHECK_EQUAL(state2.GetControlBlockSize(), randomCBS);
        BOOST_CHECK_EQUAL(state2.GetElasticBufferSize(), randomEBS);
    }
}

// Test that the "fixed size" configuration for the ABLA EBAA behaves as expected.
BOOST_AUTO_TEST_CASE(feature_fixedsize)
{
    const auto defBlkSz = DEFAULT_CONSENSUS_BLOCK_SIZE;
    const abla::Config confNormal = abla::Config::MakeDefault(defBlkSz, /* fixedSize = */ false);
    const abla::Config confFixed  = abla::Config::MakeDefault(defBlkSz, /* fixedSize = */ true);
    BOOST_REQUIRE(confNormal.IsValid());
    BOOST_REQUIRE(confFixed.IsValid());
    BOOST_CHECK( ! confNormal.IsFixedSize());
    BOOST_CHECK(confFixed.IsFixedSize());

    const abla::State stateNormal(confNormal, 0), stateFixed(confFixed, 0);
    BOOST_REQUIRE(stateNormal.IsValid(confNormal));
    BOOST_REQUIRE(stateFixed.IsValid(confFixed));

    // Both fixed and dynamic configs start off with defBlkSz as the limit
    BOOST_CHECK_EQUAL(stateNormal.GetBlockSizeLimit(), defBlkSz);
    BOOST_CHECK_EQUAL(stateFixed.GetBlockSizeLimit(), defBlkSz);

    // However, the normal (dynamic) config will grow if blocks are full
    BOOST_CHECK_GT(stateNormal.CalcLookaheadBlockSizeLimit(confNormal, 2048), defBlkSz);
    // But the fixed config never grows and stays at default
    BOOST_CHECK_EQUAL(stateFixed.CalcLookaheadBlockSizeLimit(confFixed, 2048), defBlkSz);

    BOOST_CHECK_GT(stateNormal.CalcLookaheadBlockSizeLimit(confNormal, 2048),
                   stateFixed.CalcLookaheadBlockSizeLimit(confFixed, 2048));
}

/**
 * Note: Rest of coverage for ABLA::State (such as state advancement, lookahead, etc) is in abla_test_vectors.cpp.
 **/

BOOST_AUTO_TEST_SUITE_END()
