// Copyright (c) 2023 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <config.h>
#include <consensus/consensus.h>
#include <consensus/tx_verify.h>
#include <util.h>

#include <test/test_bitcoin.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(getmintxsize_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(getmintxsize)
{
    const CChainParams config = Params(CBaseChainParams::REGTEST);
    CBlockIndex prev;

    std::array<CBlockIndex, 12> blocks;
    blocks[0].nHeight=0;
    for (size_t i = 1; i < blocks.size(); ++i)
    {
        blocks[i].pprev = &blocks[i - 1];
        blocks[i].nHeight = i;
    }

    SetArg("-upgrade9activationheight", strprintf("%d", 12));
    // Check if GetMinimumTxSize returns the correct value
    BOOST_CHECK_EQUAL(GetMinimumTxSize(config.GetConsensus(), &blocks.back()), MIN_TX_SIZE_MAGNETIC_ANOMALY);

    SetArg("-upgrade9activationheight", strprintf("%d", 10));
    BOOST_CHECK_EQUAL(GetMinimumTxSize(config.GetConsensus(), &blocks.back()), MIN_TX_SIZE_UPGRADE9);

    // Cleanup
    UnsetArg("-upgrade9activationheight");
}

BOOST_AUTO_TEST_SUITE_END()
