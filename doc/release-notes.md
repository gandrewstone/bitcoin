Release Notes for BCH Unlimited 2.1.0.0
======================================================

BCH Unlimited version 2.1.0.0 is now available from:

  <https://bitcoinunlimited.info/download>

Please report bugs using the issue tracker at github:

  <https://gitlab.com/bitcoinunlimited/BCHUnlimited/-/issues>

This is a major release of BCH Unlimited compatible with the May 15th, 2024, protocol upgrade of the Bitcoin Cash network:

- https://upgradespecs.bitcoincashnode.org/2024-05-15-upgrade/

The following is detailed list of all previous protocol upgrades specifications:

- https://gitlab.com/bitcoin-cash-node/bchn-sw/bitcoincash-upgrade-specifications/-/blob/master/spec/uahf-technical-spec.md (Aug 1st '17, ver 1.1.0.0)
- https://gitlab.com/bitcoin-cash-node/bchn-sw/bitcoincash-upgrade-specifications/-/blob/master/spec/nov-13-hardfork-spec.md (Nov 13th '17, ver 1.1.2.0)
- https://gitlab.com/bitcoin-cash-node/bchn-sw/bitcoincash-upgrade-specifications/-/blob/master/spec/may-2018-hardfork.md (May 15th '18, ver 1.3.0.0, 1.3.0.1, 1.4.0.0)
- https://gitlab.com/bitcoin-cash-node/bchn-sw/bitcoincash-upgrade-specifications/-/blob/master/spec/2018-nov-upgrade.md (Nov 15th '18, ver 1.5.0.0, 1.5.0.1, 1.5.0.2, 1.5.1.0)
- https://gitlab.com/bitcoin-cash-node/bchn-sw/bitcoincash-upgrade-specifications/-/blob/master/spec/2019-05-15-upgrade.md (May 15th '19, ver 1.6.0)
- https://gitlab.com/bitcoin-cash-node/bchn-sw/bitcoincash-upgrade-specifications/-/blob/master/spec/2019-11-15-upgrade.md (Nov 15th '19, ver 1.7.0)
- https://gitlab.com/bitcoin-cash-node/bchn-sw/bitcoincash-upgrade-specifications/-/blob/master/spec/2020-05-15-upgrade.md (May 15th '20, ver 1.8.0)
- https://gitlab.com/bitcoin-cash-node/bchn-sw/bitcoincash-upgrade-specifications/-/blob/master/spec/2020-11-15-upgrade.md (Nov 15th '20, ver 1.9.0, 1.9.0.1, 1.9.1)
- https://gitlab.com/bitcoin-cash-node/bchn-sw/bitcoincash-upgrade-specifications/-/blob/master/spec/2021-05-15-upgrade.md (May 15th '21, ver 1.9.2)
- https://gitlab.com/bitcoin-cash-node/bchn-sw/bitcoincash-upgrade-specifications/-/blob/master/spec/2022-05-15-upgrade.md (May 15th '22, ver 1.10.0)
- https://gitlab.com/bitcoin-cash-node/bchn-sw/bitcoincash-upgrade-specifications/-/blob/master/spec/2023-05-15-upgrade.md (May 15th '23, ver 2.0.0.0)


Upgrading
---------

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/Bitcoin-Qt (on Mac) or
bitcoind/bitcoin-qt (on Linux).

Main Changes in 2.1.0.0
-----------------------

This is list of the main changes that have been merged in this release:

- Implementation and activation code for CHIP-2023-04 ([2720](https://gitlab.com/bitcoinunlimited/BCHUnlimited/-/merge_requests/2720))

The new consensus rules in the aforementioned CHIPs will take effect once the median time past (MTP) of the most recent 11 blocks is greater than or equal to UNIX timestamp 1715774400.

Commit details
--------------
- `9f4e72ab25` Fix ubuntu 24.04 (noble) compilation issues (Griffith)
- `a052993122` ABLA state need to retrieved from activation block not the block before (Andrea Suisani)
- `5c7b740393` ABLA state need to be stored into CBlockIndex since activation. (Andrea Suisani)
- `bbe0d3d3d6` Fix std::optional objects serialization (Griffith)
- `1a5331e323` Add missing code to write abla block size to disk (Calin Culianu)
- `c752f7d0a2` Add a needed symbol to cashlib (token.cpp) and fix the HF effects in mempool_accept (Andrew Stone)
- `bbef0563b8` Add release notes for BCHU 2.1.0.0 (Andrea Suisani)
- `24257db684` Pass P2P port to rostrum (Dagur Valberg Johannsson)
- `389287eede` Fix chipnet configuration parameters (Andrea Suisani)
- `6aaf88c20c` Properly setup default activation time per chain during bootstrap (Andrea Suisani)
- `7021508e5b` [qa] Fix the last issue for feature_abla (Andrea Suisani)
- `ecfd6279c7` Use nBlockMaxSize as input for GetMaxBlockSigChecksCount() (Andrea Suisani)
- `b90fdc866e` Remove debug log from percentblockmaxsize validator (Andrea Suisani)
- `c968012ed7` [qa] fix getlogcategories.py functional test (Andrea Suisani)
- `d2d0cc2d8b` Fix compactblocks_1.py functional test (Andrea Suisani)
- `7ec0b113a8` Fix prioritise transactions functional test (Andrea Suisani)
- `6034a60c06` Fix formatting (Andrea Suisani)
- `93699bebb4` [qa] fix all remaining problem with feature_abla.py (Andrea Suisani)
- `5e35659909` Add two consensus tweaks (Andrea Suisani)
- `ae82a092c0` Fix code searching for activation block (Andrea Suisani)
- `65955cc20a` Slightly change activation time semantic. (Andrea Suisani)
- `70b3f07039` Add log statements to May 2024 upgrade helper funcs and to VerifyAblaStateForChain (Andrea Suisani)
- `5af4059887` Add a new debug category for protocol upgrade activation (Andrea Suisani)
- `c1cfadb26f` Fix the code that search for activation block in VerifyAblaStateForChain (Andrea Suisani)
- `544569d183` [qa] temp fix for feature and wallet p2sh32 (Andrea Suisani)
- `0175022371` Remove object to track the exact height when May 2023 upgrade activated (Andrea Suisani)
- `65413154aa` partially fix feature_abla.py test (Griffith)
- `94a4521291` add sigcheckslimit to mining candidate info (Griffith)
- `55fc70e969` add miningblocksizelimit to getmininginfo (Griffith)
- `996234522a` disable blockstorage.py test because the feature is disabled (Griffith)
- `2131faeebf` fix bug in fillmempool (Griffith)
- `94e5bdfe44` change boost::shared_ptr to std::shared_ptr (Griffith)
- `0ed541c7e7` [qa] fix another bunch of funcrtional tests (Andrea Suisani)
- `3655951995` [qa] add qa/rpc-tests/test_framework/cdefs.py to the test framework infra (Andrea Suisani)
- `e3bc0bc8ac` [qa] fix validateblocktemplate.py (Andrea Suisani)
- `68b71e6f2a` [qa] fix mempoolsync.py (Andrea Suisani)
- `ad704680ab` [qa] fix syntax error in feature_min_tx_size.py (Andrea Suisani)
- `e2df40b146` [qa] fix syntax error in feature_tx_version.py (Andrea Suisani)
- `34d5fd2125` [qa] Fix tweak.py (Andrea Suisani)
- `35e40111f7` [qa] fix parallel.py (Andrea Suisani)
- `4be4c8119b` [qa] fix miningtest.py (Andrea Suisani)
- `62db21b129` [qa] fix syntax error in feature_p2sh32.py (Andrea Suisani)
- `1524660170` Fix fomratting and remove dunused var in rpc/blockchain.cpp (Andrea Suisani)
- `23f481fdb9` Fix compilation errors in mempoofill rpc command code (Andrea Suisani)
- `e13d11a37d` add mempoolfill rpc (Griffith)
- `0e66d01a47` set consensusBlockSize to new max size when chainActive tip is updated (Griffith)
- `e7af2da3d3` remove no longer used max mining block size and max block size constants (Griffith)
- `80e79669df` rename excessiveBlockSize to consensusBlockSize. dont change rpc or arg (Griffith)
- `7dc21a769c` Fix formatting according to project standard (Andrea Suisani)
- `5f269f0d42` [consensus] Max signature checks is a function of ABLA maximu block size (Andrea Suisani)
- `9a15603df2` add partially converted ABLA qa feature test (Griffith)
- `aae26363de` add abla python logic to test_framework (Griffith)
- `1da27694c9` disable excessive QA test (Griffith)
- `17371a47e5` connect ABLA in validation code (Griffith)
- `af55da1185` [ci] fix last 2 failures in token_tests.cpp (Andrea Suisani)
- `86ae06a2e3` [ci] convert token_tests.cpp to be height activated (Andrea Suisani)
- `fabc8dec92` add abla tests to makefile (Griffith)
- `82bccd54c1` AD is no longer a part of subversion, remove it from tests (Griffith)
- `8e3bda40e3` add missing call to new in pow_tests.cpp (Griffith)
- `03f9362c4c` [ci] set back priority txns block space back to 0 as default (Andrea Suisani)
- `eedbe3faef` [ci] comment mining block size configuration tests (Andrea Suisani)
- `f93a816e19` Use boolean rather than bitwise operators (Andrea Suisani)
- `c6f1ff7633` removed unused const DEFAULT_BLOCK_MAX_SIZE_REGTEST (Andrea Suisani)
- `5fd7386074` [ci] make txn min size test to use block height activation (may 2023) (Andrea Suisani)
- `e5c84a3cd4` [consensus] may2023Height is defined otherwhise code won't compile (Andrea Suisani)
- `0adbd8eed3` fix compilation issue in miner_tests.cpp (Griffith)
- `912cf172a2` remove setexcessiveblock and miningmaxblock rpcs (Griffith)
- `e4edbd246f` remove ebTweak (Griffith)
- `c2e82657d0` make the maximum generated block size always 95% of the alba max size (Griffith)
- `f51a589265` add abla unit tests (Griffith)
- `ed878bcb12` update block and blockheader rpcs (Griffith)
- `aa93c836e2` [ci] remove test for excessive block size from checkblock_tests.cpp (Andrea Suisani)
- `1c8d8a5ae1` Temporary fix for nol net max block size. (Andrea Suisani)
- `332ebc7149` [ci] fix a typo in may 2024 unit tests (Andrea Suisani)
- `0f295c03fa` [ci] make token txns test to use block height activation (may 2023) (Andrea Suisani)
- `58a8d9bf65` update makefile (Griffith)
- `2b084d7e62` start to add abla validation code (Griffith)
- `3fd24b0e06` remove min max block size from chainparams (Griffith)
- `d02ad6f40f` rename more excessive block size to consensus block size (Griffith)
- `4fba7c7eab` add ABLA config to the chainparams (Griffith)
- `4177fe7bf2` re-add token_transaction_tests.cpp which went missing somehow (Griffith)
- `62a0c71e94` fix pow_tests.cpp (Griffith)
- `cd27ca7756` consensus: Add MTP based activation helpers for May 2024 protocol upgrade (Andrea Suisani)
- `ae6a45dbc9` Use BU logger facility in abla.cpp (Andrea Suisani)
- `95024ddc86` consensus: Use height for May 2023 hf activation (Andrea Suisani)
- `1c8b569cf8` Miscellaneous changes to the formatting tools (Andrea Suisani)
- `ee425efec9` Remove last remaining Acceptance Depth references (Andrea Suisani)
- `2647aa4a6e` comment out on disk storage sync, CDiskBlockIndex is no longer copyable (Griffith)
- `acc1e24bce` remove AcceptDepth and excessive chain concepts (Griffith)
- `97fa4ceec0` add AblaStateMixin as super of CBlockIndex (Griffith)
- `a67140f333` rename DEFAULT_EXCESSIVE_BLOCK* to DEFAULT_CONSENSUS_BLOCK* (Griffith)
- `1612ddf954` add ABLA code to consensus folder (Griffith)
- `59ea92082a` Bump BCHU version to 2.1.0.0 (Andrea Suisani)
- `e1291edf03` Fix building on FreeBSD and update the documentation (Andrew Kallmeyer)
- `dff00b0d3c` [depends] Avoid to build Rust if not building linux binaries (Andrea Suisani)
- `de3426a8d7` Allow fetching blocks outside active chain via RPC (Dagur Valberg Johannsson)
- TBD

Credits
=======

Thanks to everyone who directly contributed to this release:

- Andrea Suisani
- Andrew Kallmeyer
- Dagur Valberg Johannsson
- Greg Griffith

We have backported a set of changes from Bitcoin Cash Node, namely all the implementation of [CHIP-2023-04](https://gitlab.com/0353F40E/ebaa/-/blob/main/README.md) with some minor modifications to adapt the change to BCHU code base. In particular if you are interested to the MRs we ported over, the following is the main MR:

- https://gitlab.com/bitcoin-cash-node/bitcoin-cash-node/-/merge_requests/1782

Whereas the following are some code dependencies:

- https://gitlab.com/bitcoin-cash-node/bitcoin-cash-node/-/merge_requests/1786
- https://gitlab.com/bitcoin-cash-node/bitcoin-cash-node/-/merge_requests/1788
- https://gitlab.com/bitcoin-cash-node/bitcoin-cash-node/-/merge_requests/1789

Following all the indirect contributors whose work has been imported via the above backports:

- Andrew #128
- Calin Culianu
- freetrader
