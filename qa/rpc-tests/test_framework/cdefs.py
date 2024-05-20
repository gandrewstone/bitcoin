#!/usr/bin/env python3
# Copyright (c) 2017-2023 The Bitcoin developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Imports some application default values from source files outside the test
framework, and defines equivalents of consensus/policy parameters for the
test framework.
"""

import os
import re


def get_srcdir():
    """
    Try to find out the base folder containing the 'src' folder.
    If SRCDIR is set it does a sanity check and returns that.
    Otherwise it goes on a search and rescue mission.

    Returns None if it cannot find a suitable folder.
    """
    def contains_src(path_to_check):
        if not path_to_check:
            return False
        else:
            cand_path = os.path.join(path_to_check, 'src')
            return os.path.exists(cand_path) and os.path.isdir(cand_path)

    srcdir = os.environ.get('SRCDIR', '')
    if contains_src(srcdir):
        return srcdir

    # Try to work it based out on main module
    import sys
    mainmod = sys.modules['__main__']
    mainmod_path = getattr(mainmod, '__file__', '')
    if mainmod_path and mainmod_path.endswith('.py'):
        maybe_top = mainmod_path
        while maybe_top != '/':
            maybe_top = os.path.abspath(os.path.dirname(maybe_top))
            if contains_src(maybe_top):
                return maybe_top

    # No luck, give up.
    return None


# Slurp in consensus.h and policy.h contents
_consensus_h_fh = open(os.path.join(get_srcdir(), 'src', 'consensus', 'consensus.h'), 'rt', encoding='utf-8')
_consensus_h_contents = _consensus_h_fh.read()
_consensus_h_fh.close()

_policy_h_fh = open(os.path.join(get_srcdir(), 'src', 'policy', 'policy.h'), 'rt', encoding='utf-8')
_policy_h_contents = _policy_h_fh.read()
_policy_h_fh.close()

# This constant is currently needed to evaluate some that are formulas
ONE_MEGABYTE = 1000000

# Extract relevant default values parameters

# The maximum allowed block size before the fork
LEGACY_MAX_BLOCK_SIZE = ONE_MEGABYTE

# Default setting for maximum allowed size for a block, in bytes
_consensus_block_size_match = re.search(
    r'DEFAULT_CONSENSUS_BLOCK_SIZE = (.+);',
    _consensus_h_contents)
if _consensus_block_size_match is None:
    import sys
    print("DEFAULT_CONSENSUS_BLOCK_SIZE not found in consensus.h")
    sys.exit(1)
else:
    DEFAULT_CONSENSUS_BLOCK_SIZE = eval(_consensus_block_size_match.group(1))

# The following consensus parameters should not be automatically imported.
# They *should* cause test failures if application code is changed in ways
# that violate current consensus.

# The maximum allowed number of signature check operations per MB in a block
# (network rule)
MAX_BLOCK_SIGOPS_PER_MB = 20000

# The maximum allowed number of signature check operations per transaction
# (network rule)
MAX_TX_SIGOPS_COUNT = 20000


# The minimum number of max_block_size bytes required per executed signature
# check operation in a block. I.e. maximum_block_sigchecks = maximum_block_size
# / BLOCK_MAXBYTES_MAXSIGCHECKS_RATIO (network rule).
BLOCK_MAXBYTES_MAXSIGCHECKS_RATIO = 141

# The maximum number of sigops we're willing to relay/mine in a single tx
# (policy.h constant)
MAX_STANDARD_TX_SIGOPS = MAX_TX_SIGOPS_COUNT // 5

# Coinbase transaction outputs can only be spent after this number of new
# blocks (network rule)
COINBASE_MATURITY = 100

# Minimum size a transaction can have after Magnetic Anomaly (post Nov. 15 2018 HF)
MIN_TX_SIZE_MAGNETIC_ANOMALY = 100

# Minimum size a transaction can have after Upgrade9 (post May 15 2023 HF)
MIN_TX_SIZE_UPGRADE9 = 65

# Maximum bytes in a TxOut pubkey script
MAX_TXOUT_PUBKEY_SCRIPT = 10000

# From consensus/consensus.h
MAX_CONSENSUS_BLOCK_SIZE = 2000 * ONE_MEGABYTE

# The following are some node policy values, but also useful in tests.

# Default setting for maximum generated size of block (soft limit)
# This is a per-chain limit and is specified in src/chainparams.cpp
# Please ensure this value matches the setting for CRegTestParams.
DEFAULT_MAX_GENERATED_BLOCK_SIZE = 8 * ONE_MEGABYTE

# Limit for -txbroadcastrate
MAX_INV_BROADCAST_RATE = 1_000_000

# Limit for -txbroadcastingerval
MAX_INV_BROADCAST_INTERVAL = 1_000_000

# The number of blocks ahead of current tip that the node downloads from the p2p network
BLOCK_DOWNLOAD_WINDOW = 1024

# Ensure sanity of these constants
if DEFAULT_MAX_GENERATED_BLOCK_SIZE > DEFAULT_CONSENSUS_BLOCK_SIZE:
    import sys
    sys.exit(f"DEFAULT_MAX_GENERATED_BLOCK_SIZE ({DEFAULT_MAX_GENERATED_BLOCK_SIZE}) may not be larger than "
             f"DEFAULT_CONSENSUS_BLOCK_SIZE ({DEFAULT_CONSENSUS_BLOCK_SIZE})")


if __name__ == "__main__":
    # Output values if run standalone to verify
    print("DEFAULT_CONSENSUS_BLOCK_SIZE = {} (bytes)".format(DEFAULT_CONSENSUS_BLOCK_SIZE))
    print("MAX_BLOCK_SIGOPS_PER_MB = {} (sigops)".format(MAX_BLOCK_SIGOPS_PER_MB))
    print("MAX_TX_SIGOPS_COUNT = {} (sigops)".format(MAX_TX_SIGOPS_COUNT))
    print("COINBASE_MATURITY = {} (blocks)".format(COINBASE_MATURITY))
    print("DEFAULT_MAX_GENERATED_BLOCK_SIZE = {} (bytes)".format(DEFAULT_MAX_GENERATED_BLOCK_SIZE))
