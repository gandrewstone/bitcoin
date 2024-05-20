// Copyright (c) 2018-2020 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "forks.h"
#include "chain.h"
#include "chainparams.h"
#include "primitives/block.h"
#include "script/interpreter.h"
#include "txmempool.h"
#include "unlimited.h"

#include <inttypes.h>
#include <vector>

bool IsTxProbablyNewSigHash(const CTransaction &tx)
{
    bool oldsighash = false;
    for (auto txin : tx.vin)
    {
        std::vector<unsigned char> data;
        CScript::const_iterator pc(txin.scriptSig.begin());
        opcodetype op;
        if (txin.scriptSig.GetOp(pc, op, data))
        {
            if (!data.empty())
            {
                if (!(data.back() & SIGHASH_FORKID))
                {
                    oldsighash = true;
                }
            }
        }
    }
    return (oldsighash == false);
}

bool IsTxUAHFOnly(const CTxMemPoolEntry &txentry)
{
    if ((txentry.sighashType & SIGHASH_FORKID) || (txentry.sighashType == 0))
    {
        return true;
    }
    return false;
}

// return true for every block from fork block and forward [consensusParams.uahfHeight,+inf)
bool UAHFforkActivated(int height)
{
    const Consensus::Params &consensusParams = Params().GetConsensus();
    if (height >= consensusParams.uahfHeight)
    {
        return true;
    }
    return false;
}

// This will check if the Fork will be enabled at the next block
// i.e. we are at block x - 1, [consensusParams.uahfHeight-1, +inf]
// state fork: enabled or activated
bool IsUAHFforkActiveOnNextBlock(int height)
{
    const Consensus::Params &consensusParams = Params().GetConsensus();
    if (height >= (consensusParams.uahfHeight - 1))
        return true;
    return false;
}

// For pindexTip use the current chainActive.Tip().

bool IsDAAEnabled(const Consensus::Params &consensusparams, int nHeight)
{
    return nHeight >= consensusparams.daaHeight;
}

bool IsDAAEnabled(const Consensus::Params &consensusparams, const CBlockIndex *pindexTip)
{
    if (pindexTip == nullptr)
    {
        return false;
    }
    return IsDAAEnabled(consensusparams, pindexTip->nHeight);
}

bool IsNov2018Activated(const Consensus::Params &consensusparams, const int32_t nHeight)
{
    return nHeight >= consensusparams.nov2018Height;
}

bool IsNov2018Activated(const Consensus::Params &consensusparams, const CBlockIndex *pindexTip)
{
    if (pindexTip == nullptr)
    {
        return false;
    }
    return IsNov2018Activated(consensusparams, pindexTip->nHeight);
}

bool IsNov2019Activated(const Consensus::Params &consensusparams, const int32_t nHeight)
{
    return nHeight >= consensusparams.nov2019Height;
}

bool IsNov2019Activated(const Consensus::Params &consensusparams, const CBlockIndex *pindexTip)
{
    if (pindexTip == nullptr)
    {
        return false;
    }
    return IsNov2019Activated(consensusparams, pindexTip->nHeight);
}

bool IsMay2020Activated(const Consensus::Params &consensusparams, const int32_t nHeight)
{
    return nHeight >= consensusparams.may2020Height;
}

bool IsMay2020Activated(const Consensus::Params &consensusparams, const CBlockIndex *pindexTip)
{
    if (pindexTip == nullptr)
    {
        return false;
    }
    return IsMay2020Activated(consensusparams, pindexTip->nHeight);
}

bool IsNov2020Activated(const Consensus::Params &consensusparams, const CBlockIndex *pindexTip)
{
    if (pindexTip == nullptr)
    {
        return false;
    }
    if (consensusparams.nov2020Height)
    {
        return pindexTip->nHeight >= consensusparams.nov2020Height;
    }
    else
    {
        return pindexTip->IsforkActiveOnNextBlock(NOV2020_ACTIVATION_TIME);
    }
}

bool IsMay2022Activated(const Consensus::Params &consensusparams, const CBlockIndex *pindexTip)
{
    if (pindexTip == nullptr)
    {
        return false;
    }
    if (consensusparams.may2022Height)
    {
        return pindexTip->nHeight >= consensusparams.may2022Height;
    }
    // nolnet and regtest don't have height set.
    return pindexTip->IsforkActiveOnNextBlock(MAY2022_ACTIVATION_TIME);
}


// Check if May 15th 2023 fork (9 upgrade) has activated using block height
bool IsMay2023Activated(const Consensus::Params &consensusparams, const CBlockIndex *pindexTip)
{
    if (pindexTip == nullptr)
    {
        return false;
    }
    // if node's launched with custom activation height use that, cause we are presumably
    // running some functional/unit tests.
    int32_t height = GetArg("-upgrade9activationheight", consensusparams.may2023Height);
    return pindexTip->nHeight >= height;
}

// Check if May 15th 2024 fork has activated using MTP
bool IsMay2024Activated(const Consensus::Params &params, const CBlockIndex *pindexTip)
{
    if (pindexTip == nullptr)
    {
        return false;
    }
    uint64_t activationTime = nMiningForkTime;
    LOG(ACTIVATION, "%s: Param activation time is: %llu - GetMedianTimePast %llu - actived: %d - height: %d", __func__,
        activationTime, pindexTip->GetMedianTimePast(), pindexTip->IsforkActiveOnNextBlock(activationTime),
        pindexTip->nHeight);
    return pindexTip->IsforkActiveOnNextBlock(activationTime);
}

bool IsMay2024Active(const Consensus::Params &params, const CBlockIndex *pindex)
{
    if (pindex == nullptr)
    {
        return false;
    }
    uint64_t activationTime = nMiningForkTime;
    LOG(ACTIVATION, "%s: Height %d - Param activation time: %llu - GetMedianTimePast %llu - active: %d", __func__,
        pindex->nHeight, activationTime, pindex->GetMedianTimePast(), pindex->forkActivated(activationTime));
    return pindex->forkActivated(activationTime);
}
