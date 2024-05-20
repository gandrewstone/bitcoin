// Copyright (c) 2023 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <consensus/consensus.h>
#include <serialize.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <tuple>

/// ABLA, the Adaptive Blocksize-Limit Algorithm
///
/// Algorithm and data types for dynamically adjusting the block size limit permitted on the Bitcoin Cash
/// network.
///
/// Originally written by bitcoincashautist but adapted to C++ and to fit into the BCHN sources by Calin Culianu.
namespace abla
{

/// Algorithm configuration -- this should be a part of a chain's consensus params.
struct Config
{
    /// Initial control block size value, also used as floor value
    uint64_t epsilon0{};
    /// Initial elastic buffer size value, also used as floor value
    uint64_t beta0{};
    /// Reciprocal of control function "forget factor" value
    uint64_t gammaReciprocal{};
    /// Control function "asymmetry factor" value
    uint64_t zeta_xB7{};
    /// Reciprocal of elastic buffer decay rate
    uint64_t thetaReciprocal{};
    /// Elastic buffer "gear factor"
    uint64_t delta{};
    /// Maximum control block size value
    uint64_t epsilonMax{};
    /// Maximum elastic buffer size value
    uint64_t betaMax{};

    /// Set epsilonMax and betaMax such that algo's internal arithmetic ops can't overflow UINT64_MAX
    void SetMax();

    /// Returns true if the configuration is valid and/or sane. On false return optional out `*err` is set to point to
    /// a constant string explaining the reason that it is invalid.
    [[nodiscard]] bool IsValid(const char **err = nullptr) const;

    /// Returns true if the configuration renders the algorithm as a 'no-op' that will always return a fixed size.
    /// This can only be true iff `epsilon0 == epsilonMax && beta0 == betaMax` (testnets 3 & 4 have this as true).
    [[nodiscard]] bool IsFixedSize() const;

    /// Used for debug purposes -- print all of this instance's variables to a string.
    std::string ToString() const;

    // gitlab.com/0353F40E/ebaa
    /// Returns a default configuration for mainnet, etc as suggested in the ABLA spec: https:
    /// @param fixedSize - if `true`, set `epsilonMax = epsilon0`, `betaMax = beta0`, thus making the ABLA algorithm
    ///        a no-op that always returns `defaultBlockSize` as the static max block size. This is normally set to
    ///        `true` for testnet3 and testnet4 (where we do not want the max block size to grow over time).
    static Config MakeDefault(uint64_t defaultBlockSize = DEFAULT_CONSENSUS_BLOCK_SIZE, bool fixedSize = false);
};

/// Algorithm's internal state
///
/// Intended to be used such that this State is associated with block N, and the block size limit for block N is to
/// be given by `state_N.GetBlockSizeLimit()`. Thus, when checking the limit for the next block N + 1, given the state
/// for N, one must do: `state_N.NextBlockState().GetBlockSizeLimit()`
class State
{
    /// Saved with the state -- the actual block size in bytes for this block
    uint64_t blockSize{};

    /// Control function state
    uint64_t controlBlockSize{};
    /// Elastic buffer function state
    uint64_t elasticBufferSize{};

public:
    State() = default;

    /// Construct a state using defaults from Config (suitable for all blocks before ABLA activation)
    State(const Config &config, uint64_t blkSize)
        : blockSize(blkSize), controlBlockSize(config.epsilon0), elasticBufferSize(config.beta0)
    {
    }

    /// Get the block size limit for this block -- note this is capped at MAX_CONSENSUS_BLOCK_SIZE == 2GB unless
    /// `disable2GBCap` is set to true.
    uint64_t GetBlockSizeLimit(bool disable2GBCap = false) const
    {
        if (disable2GBCap)
        {
            return controlBlockSize + elasticBufferSize;
        }
        else
        {
            return std::min<uint64_t>(controlBlockSize + elasticBufferSize, MAX_CONSENSUS_BLOCK_SIZE);
        }
    }

    /// Return the block size limit for the next block after this one. This is a utility method for consensus code
    /// to quickly know the limit to apply to the next block, given the current tip's defined State.
    uint64_t GetNextBlockSizeLimit(const Config &config, bool disable2GBCap = false) const
    {
        return NextBlockState(config, 0).GetBlockSizeLimit(disable2GBCap);
    }

    /// Advance the algorithm's state to the next block (N + 1), given the next block (N + 1) block size,
    /// algorithm state for this block (N), and global algorithm configuration.
    State NextBlockState(const Config &config, uint64_t nextBlockSize) const;

    /// Calculate algorithm's look-ahead block size limit, for a block N blocks ahead of current one. This is a
    /// "worst-case" calculation for the block size limit N blocks ahead. In other words, this function
    /// returns the limit for block with current+N height, assuming all blocks 100% full.
    uint64_t CalcLookaheadBlockSizeLimit(const Config &config, size_t count, bool disable2GBCap = false) const;

    /// Query this block's size
    uint64_t GetBlockSize() const { return blockSize; }
    /// Query this state's "control block size"
    uint64_t GetControlBlockSize() const { return controlBlockSize; }
    /// Query this state's "elastic buffer size"
    uint64_t GetElasticBufferSize() const { return elasticBufferSize; }

    /// Returns true if this state is valid relative to `config`. On false return, optional out `err` is set
    /// to point to a constant string explaining the reason that this state is invalid.
    [[nodiscard]] bool IsValid(const Config &config, const char **err = nullptr) const;

    /// Used for debug purposes -- print all of this instance's variables to a string.
    std::string ToString() const;

    ADD_SERIALIZE_METHODS

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action)
    {
        READWRITE(VARINT(blockSize));
        READWRITE(VARINT(controlBlockSize));
        READWRITE(VARINT(elasticBufferSize));
    }

    // -- To/From a tuple (mainly used by tests) --

    auto ToTuple() const { return std::tuple(blockSize, controlBlockSize, elasticBufferSize); }

    static State FromTuple(const std::tuple<uint64_t, uint64_t, uint64_t> &tup)
    {
        State ret;
        std::tie(ret.blockSize, ret.controlBlockSize, ret.elasticBufferSize) = tup;
        return ret;
    }

    // -- Minimal comparison ops --

    bool operator<(const State &o) const { return ToTuple() < o.ToTuple(); }
    bool operator==(const State &o) const { return ToTuple() == o.ToTuple(); }
    bool operator!=(const State &o) const { return ToTuple() != o.ToTuple(); }
};

// --- Some constants ---
// These are exposed here for unit tests, normally client code shouldn't need these.

/**
 * Constant 2^7, used as fixed precision for algorithm's "asymmetry factor" configuration value, e.g. we will
 * store the real number 1.5 as integer 192 so when we want to multiply or divide an integer with value of 1.5,
 * we will do muldiv(value, 192, B7) or muldiv(value, B7, 192).
 */
inline constexpr uint64_t B7 = 1u << 7u;

// Sanity ranges for configuration values
inline constexpr uint64_t MIN_ZETA_XB7 = 129u; ///< zeta real value of 1.0078125
inline constexpr uint64_t MAX_ZETA_XB7 = 256u; ///< zeta real value of 2.0000000
inline constexpr uint64_t MIN_GAMMA_RECIPROCAL = 9484u;
inline constexpr uint64_t MAX_GAMMA_RECIPROCAL = 151744u;
inline constexpr uint64_t MIN_DELTA = 0u;
inline constexpr uint64_t MAX_DELTA = 32u;
inline constexpr uint64_t MIN_THETA_RECIPROCAL = 9484u;
inline constexpr uint64_t MAX_THETA_RECIPROCAL = 151744u;

} // namespace abla
