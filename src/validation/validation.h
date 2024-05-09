
// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2015-2020 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_VALIDATION_H
#define BITCOIN_VALIDATION_H

#include "chainparams.h"
#include "consensus/validation.h"
#include "forks.h"
#include "parallel.h"
#include "txdebugger.h"
#include "txmempool.h"
#include "versionbits.h"

extern std::atomic<uint64_t> nBlockSizeAtChainTip;

/** Default for -blockchain.maxReorgDepth */
static const int DEFAULT_MAX_REORG_DEPTH = 10;
/**
 * Default for -finalizationdelay
 * This is the minimum time between a block header reception and the block
 * finalization.
 * This value should be >> block propagation and validation time
 */
static const int64_t DEFAULT_MIN_FINALIZATION_DELAY = 2 * 60 * 60;

/** Is express validation turned on/off */
static const bool DEFAULT_XVAL_ENABLED = true;

enum DisconnectResult
{
    DISCONNECT_OK, // All good.
    DISCONNECT_UNCLEAN, // Rolled back, but UTXO set was inconsistent with block.
    DISCONNECT_FAILED // Something else went wrong.
};

/** Context-independent validity checks */
bool CheckBlockHeader(const CBlockHeader &block, CValidationState &state, bool fCheckPOW = true);

/** Context-dependent validity header checks */
bool ContextualCheckBlockHeader(const CBlockHeader &block, CValidationState &state, CBlockIndex *pindexPrev);

bool AcceptBlockHeader(const CBlockHeader &block,
    CValidationState &state,
    const CChainParams &chainparams,
    CBlockIndex **ppindex = nullptr);

CBlockIndex *AddToBlockIndex(const CBlockHeader &block);

/** Create a new block index entry for a given block hash */
CBlockIndex *InsertBlockIndex(const uint256 &hash);

/** Look up the block index entry for a given block hash. returns nullptr if it does not exist */
CBlockIndex *LookupBlockIndex(const uint256 &hash);


/** Unload database information */
void UnloadBlockIndex();

/** Load the block tree and coins database from disk */
bool LoadBlockIndex();

/** Initialize a new block tree database + block data on disk */
bool InitBlockIndex(const CChainParams &chainparams);

void CheckBlockIndex(const Consensus::Params &consensusParams);

/**
 * Check whether all inputs of this transaction are valid (no double spends, scripts & sigs, amounts)
 * This does not modify the UTXO set. If pvChecks is not nullptr, script checks are pushed onto it
 * instead of being performed inline.
 */
bool CheckInputs(const CTransactionRef &tx,
    CValidationState &state,
    const CCoinsViewCache &view,
    bool fScriptChecks,
    unsigned int flags,
    unsigned int maxOps,
    bool cacheStore,
    ValidationResourceTracker *resourceTracker,
    std::vector<CScriptCheck> *pvChecks = nullptr,
    unsigned char *sighashType = nullptr,
    CValidationDebugger *debugger = nullptr);

/** Remove invalidity status from a block and its descendants. */
bool ReconsiderBlock(CValidationState &state, CBlockIndex *pindex);

/** Check a block is completely valid from start to finish (only works on top of our current best block, with cs_main
 * held) */
bool TestBlockValidity(CValidationState &state,
    const CChainParams &chainparams,
    const ConstCBlockRef pblock,
    CBlockIndex *pindexPrev,
    bool fCheckPOW = true,
    bool fCheckMerkleRoot = true);

CAmount GetBlockSubsidy(int nHeight, const Consensus::Params &consensusParams);

/**
 * Determine what nVersion a new block should use.
 */
int32_t ComputeBlockVersion(const CBlockIndex *pindexPrev, const Consensus::Params &params);

CBlockIndex *FindMostWorkChain();

/** Mark a block as invalid. */
bool InvalidateBlock(CValidationState &state, const Consensus::Params &consensusParams, CBlockIndex *pindex);

void InvalidChainFound(CBlockIndex *pindexNew);

/** Context-dependent validity block checks */
bool ContextualCheckBlock(ConstCBlockRef pblock, CValidationState &state, CBlockIndex *pindexPrev);

// BU: returns the blocksize if block is valid.  Otherwise 0
bool CheckBlock(ConstCBlockRef pblock, CValidationState &state, bool fCheckPOW = true, bool fCheckMerkleRoot = true);

/** Mark a block as having its data received and checked (up to BLOCK_VALID_TRANSACTIONS). */
bool ReceivedBlockTransactions(ConstCBlockRef pblock,
    CValidationState &state,
    CBlockIndex *pindexNew,
    const CDiskBlockPos &pos);

uint32_t GetBlockScriptFlags(const CBlockIndex *pindex, const Consensus::Params &consensusparams);
/// Returns the script flags which are basically GetBlockScriptFlags | STANDARD_SCRIPT_VERIFY_FLAGS
uint32_t GetMemPoolScriptFlags(const Consensus::Params &params,
    const CBlockIndex *pindex,
    uint32_t *nextBlockFlags = nullptr /* out param: block flags without standard */);

/** Undo the effects of this block (with given index) on the UTXO set represented by coins.
 *  In case pfClean is provided, operation will try to be tolerant about errors, and *pfClean
 *  will be true if no problems were found. Otherwise, the return value will be false in case
 *  of problems. Note that in any case, coins may be modified. */
DisconnectResult DisconnectBlock(const ConstCBlockRef pblock, const CBlockIndex *pindex, CCoinsViewCache &view);

/** Apply the effects of this block (with given index) on the UTXO set represented by coins */
bool ConnectBlock(ConstCBlockRef pblock,
    CValidationState &state,
    CBlockIndex *pindex,
    CCoinsViewCache &view,
    const CChainParams &chainparams,
    bool fJustCheck = false,
    bool fParallel = false);

/** Disconnect the current chainActive.Tip() */
bool DisconnectTip(CValidationState &state, const Consensus::Params &consensusParams, const bool fRollBack = false);

/** Find the best known block, and make it the tip of the block chain */
bool ActivateBestChain(CValidationState &state,
    const CChainParams &chainparams,
    ConstCBlockRef pblock = nullptr,
    bool fParallel = false,
    CNode *pfrom = nullptr);

/**
 * Process an incoming block. This only returns after the best known valid
 * block is made active. Note that it does not, however, guarantee that the
 * specific block passed to it has been checked for validity!
 *
 * @param[out]  state   This may be set to an Error state if any error occurred processing it, including during
 * validation/connection/etc of otherwise unrelated blocks during reorganisation; or it may be set to an Invalid state
 * if pblock is itself invalid (but this is not guaranteed even when the block is checked). If you want to *possibly*
 * get feedback on whether pblock is valid, you must also install a CValidationInterface (see validationinterface.h) -
 * this will have its BlockChecked method called whenever *any* block completes validation.
 * @param[in]   pfrom   The node which we are receiving the block from; it is added to mapBlockSource and may be
 * penalised if the block is invalid.
 * @param[in]   pblock  The block we want to process.
 * @param[in]   fForceProcessing Process this block even if unrequested; used for non-network block sources and
 * whitelisted peers.
 * @param[out]  dbp     If pblock is stored to disk (or already there), this will be set to its location.
 * @return True if state.IsValid()
 */
bool ProcessNewBlock(CValidationState &state,
    const CChainParams &chainparams,
    CNode *pfrom,
    ConstCBlockRef pblock,
    bool fForceProcessing,
    CDiskBlockPos *dbp,
    bool fParallel);

/**
 * Mark a block as finalized.
 * A finalized block can not be reorged in any way.
 */
bool FinalizeBlockAndInvalidate(CValidationState &state, CBlockIndex *pindex);

/** Get the the block index for the currently finalized block */
const CBlockIndex *GetFinalizedBlock();

/** Is this block finalized or within the chain that is already finalized */
bool IsBlockFinalized(const CBlockIndex *pindex);

//! Check whether the block associated with this index entry is pruned or not.
bool IsBlockPruned(const CBlockIndex *pblockindex);

/// This class manages tracking exactly at what block a particular upgrade activated, relative to a block index it is
/// given.  Works correcly even if there is a reorg and/or if the active chain is not being considered.  It was written
/// originally for Upgrade9 activation height tracking, but it is generic enough in that it can be re-used for any
/// future upgrade, if needed.
struct ActivationBlockTracker
{
    /// Typedef for a function pointer to one of the Is*Enabled() functions in consensus/activation.h
    /// e.g.: IsUpgrade9Enabled
    using Predicate = bool (*)(const Consensus::Params &, const CBlockIndex *);

    ActivationBlockTracker(Predicate isUpgradeXEnabledFunc) : predicate(isUpgradeXEnabledFunc) {}

    /**
     * @brief GetActivationBlock - Given a block index for which the upgrade in question is already activated, returns
     *                             the activation block for the upgrade. (The activation block is the first block which
     *                             is an ancestor of `pindex` for which `predicate()` returns `true`.
     * @pre pindex **must** have the upgrade activated for itself (e.g. it must be a block index that returns `true` for
     *             `predicate(params, pindex)`. For efficiency, this precondition is not checked!
     * @param params - Consensus params for the global chain, e.g. config.GetChainParams().GetConsensus()
     * @param pindex - Usually the current tip, but not necessarily. pindex need not live on the active chain.
     * @return The block that the upgrade activated. The activation block is the last block mined under the OLD rules,
     *         and the first block for which `predicate()` returns `true`.  The block after this one would be really
     *         the first block where e.g. tokens are enabled if we are considering upgrade9, for example.  May return
     *         pindex itself.  If this function's precondition is met (`pindex` has the upgrade activated), will never
     *         return nullptr.  Otherwise if the precondition is not satisfied, this function's behavior is undefined.
     */
    const CBlockIndex *GetActivationBlock(const CBlockIndex *pindex, const Consensus::Params &params)
        EXCLUSIVE_LOCKS_REQUIRED(cs_main);

    /**
     * For testing purposes.  We cache the activation block index for efficiency. If block indices are freed then this
     * needs to be called to ensure no dangling pointer when a new block tree is created.
     */
    void ResetActivationBlockCache() noexcept EXCLUSIVE_LOCKS_REQUIRED(cs_main) { cachedActivationBlock = nullptr; }

    /**
     * For testing purposes.  Get the current cached activation block.
     */
    const CBlockIndex *GetActivationBlockCache() const noexcept EXCLUSIVE_LOCKS_REQUIRED(cs_main)
    {
        return cachedActivationBlock;
    }

    Predicate GetPredicate() const { return predicate; }

private:
    const CBlockIndex *cachedActivationBlock GUARDED_BY(cs_main) = nullptr;
    const Predicate predicate;
};

/// Returns the adaptive blocksize limit for the next block, given `pindexPrev`, if upgrade10 is activated.
/// If upgrade 10 is not activated, returns the legacy blocksize limit for the chain (e.g. 32MB for mainnet,
/// 2MB for testnet4, -excessiveblocksize=XX, etc).
/// @pre Either upgrade10 must *not* be activated, *or* if it is, `pindexPrev` *must* have a valid `ablaStateOpt`.
///      (This precondition is guaranteed if `pindexPrev` is on the active chain.)
uint64_t GetNextBlockSizeLimit(const CBlockIndex *pindexPrev);

/**
 * Checks that the block's size doesn't exceed nMaxBlockSize.
 * @param pBlockSize optional out param to report the calculated size. This is only set on true return.
 * @return true if the check passes, false otherwise
 */
bool CheckBlockSize(ConstCBlockRef pblock,
    CValidationState &state,
    uint64_t nMaxBlockSize,
    uint64_t *pBlockSize = nullptr);

/// Global object to track the exact height when Upgrade9 activated (needed by Token consensus rules).
extern ActivationBlockTracker g_upgrade9_block_tracker;


#endif
