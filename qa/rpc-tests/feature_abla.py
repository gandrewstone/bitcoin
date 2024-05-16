#!/usr/bin/env python3
# Copyright (c) 2023 The Bitcoin developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test support for ABLA EBAA which activates with Upgrade10."""
import time

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_greater_than_or_equal,
    assert_not_equal,
    connect_nodes,
    disconnect_nodes,
)
import test_framework.loginit
from test_framework.abla import *

from decimal import Decimal
import logging

# The minimum number of max_block_size bytes required per executed signature
# check operation in a block. I.e. maximum_block_sigchecks = maximum_block_size
# / BLOCK_MAXBYTES_MAXSIGCHECKS_RATIO (network rule).
BLOCK_MAXBYTES_MAXSIGCHECKS_RATIO = 141

DEFAULT_CONSENSUS_BLOCK_SIZE = 1000000 * 32 # 32MB

class AblaTest(BitcoinTestFramework):

    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True
        self.extra_args = [['-upgrade10activationtime=2000000000', '-debug=blk', '-use-grapheneblocks=0'], ['-use-grapheneblocks=0','-upgrade10activationtime=0']]
        # We need a long rpc timeout here so that the sanitizer-undefined CI job may pass
        self.rpc_timeout = 600

    def get_header(self, blockhash=None, verbose=True):
        if blockhash is None:
            blockhash = self.nodes[0].getblockchaininfo()['bestblockhash']
        return self.nodes[0].getblockheader(blockhash, verbose)

    def get_mtp(self, blockhash=None):
        return self.get_header(blockhash, True)['mediantime']

    def get_abla_activation_height(self):
        """Walk backwards from tip, finding ABLA activation height. Returns the height or None if ABLA not activated."""
        node = self.nodes[0]
        seen = False
        bh = node.getblockchaininfo()['bestblockhash']
        while bh:
            hdr = self.get_header(bh)
            if 'ablastate' in hdr:
                seen = True
            else:
                if seen:
                    return hdr['height'] + 1
                else:
                    return None
            bh = hdr.get('previousblockhash')
        return 0

    def run_test(self):
        node = self.nodes[0]
        addr = node.getnewaddress()
        node.set("mining.unsafeGetBlockTemplate=true")
        # We operate on node 0 disconnected from 1, and connect them at the end to verify that node1 was able to IBD
        # to node 0 correctly.
        disconnect_nodes(self.nodes[0], 1)
        # Start off 120 MB mempool (~= about 50MB serialized size)
        node.fillmempool(120)
        assert_greater_than(node.getmempoolinfo()['bytes'], DEFAULT_CONSENSUS_BLOCK_SIZE)
        assert_greater_than_or_equal(node.getblockchaininfo()['blocks'], 100)

        # Ensure no ABLA state in headers
        assert 'ablastate' not in self.get_header()

        node.set("mining.unsafeGetBlockTemplate=true")
        # Check that blocksize limit is at the consensus default
        assert_equal(node.getblocktemplate({})['sizelimit'], DEFAULT_CONSENSUS_BLOCK_SIZE)

        mtp = self.get_mtp()

        node.set("upgrade10activationtime="+str(mtp+1));
        node.set("percentblockmaxsize=100");
        node.set("mining.unsafeGetBlockTemplate=true")

        # No ABLA yet
        assert_equal(self.get_abla_activation_height(), None)

        saved_height = self.get_header()['height']

        # Mine 6 blocks (maybe sleep to advance the clock past activation NTP)
        t_diff = max(self.get_header()['time'], time.time()) - mtp
        if t_diff < 0:
            time.sleep(abs(t_diff) + 1)
        node.generatetoaddress(6, addr)

        activation_height = self.get_abla_activation_height()
        assert_not_equal(activation_height, None)
        assert_greater_than(activation_height, saved_height)

        node.fillmempool(290)  # Put 290 MB unserialized =~ 70 MB serialized tx data into mempool

        abla = AblaState.FromRpcDict(self.get_header())  # Get tip's abla state
        new_block_hash = node.generatetoaddress(1, addr)[0]  # Mine a full block
        new_block_size = node.getblock(new_block_hash, 1)['size']
        logging.info("Mined block size: " + str(new_block_size))
        assert_greater_than(new_block_size, abla.GetNextBlockSizeLimit() * 0.90)  # Ensure block was indeed full-ish

        new_abla_state = abla.NextBlockState(new_block_size)  # Advance abla state

        # Should match what we expect
        assert_equal(new_abla_state, AblaState.FromRpcDict(self.get_header()))

        # Since last block was full, limit should have grown
        next_block_size_limit = new_abla_state.GetNextBlockSizeLimit()
        assert_greater_than(next_block_size_limit, abla.GetNextBlockSizeLimit())

        # Ensure mining (using percentmaxblocksize=100.0) reports correct value
        mining_limit = next_block_size_limit
        gbtl = node.getblocktemplate({})
        mi = node.getmininginfo()
        assert_greater_than(gbtl['sizelimit'], DEFAULT_CONSENSUS_BLOCK_SIZE)
        assert_equal(str(mi['miningblocksizelimit']), str(mining_limit))
        assert_equal(gbtl['sizelimit'], next_block_size_limit)
        # Check sigop limit of the block
        expected_sigops = next_block_size_limit // BLOCK_MAXBYTES_MAXSIGCHECKS_RATIO
        assert_equal(gbtl['sigchecklimit'], expected_sigops)

        # Next, restart node, specifying -precentmaxblocksize=50. `getmininginfo` limits should reflect this
        node.set("upgrade10activationtime="+str(mtp+1));
        node.set("percentblockmaxsize=50");

        node.set("mining.unsafeGetBlockTemplate=true")
        mining_limit = next_block_size_limit // 2
        # Check that mempool > 1/2 the blocksize limit so below test works ...
        assert_greater_than_or_equal(node.getmempoolinfo()['bytes'], mining_limit)

        node.set("percentblockmaxsize=50")
        # FIXME: w/o this sleep it's like the gbtl call is not executed!?
        time.sleep(30)

        gbtl = node.getblocktemplate({})
        mi = node.getmininginfo()
        # GBT doesn't tell you the configured size limit, just the consensus limit :(
        assert_equal(gbtl['sizelimit'], next_block_size_limit)
        # Mining info is the one that tells you the blockmaxsize setting...
        assert_equal(str(mi['miningblocksizelimit']), str(mining_limit))
        assert_greater_than_or_equal(mining_limit, mi['currentblocksize'])
        reserved_space = 1000  # BlockAssembler reserves 1000 bytes for coinbase txn
        assert_greater_than_or_equal(mi['currentblocksize'] + reserved_space , mining_limit)
        # Finally, belt-and-suspenders check sigop limit of the block (should be unaffected by blockmaxsize)
        expected_sigops = next_block_size_limit // BLOCK_MAXBYTES_MAXSIGCHECKS_RATIO
        assert_equal(gbtl['sigchecklimit'], expected_sigops)

        # Mine beyond the initial 32 MB limit to test ABLA actually allowing for bigger blocks.
        node.set("upgrade10activationtime="+str(mtp+1));
        node.set("percentblockmaxsize=100");
        node.set("mining.unsafeGetBlockTemplate=true")

        #node.fillmempool(290)  # Put 290 MB unserialized =~ 70 MB serialized tx data into mempool
        assert_greater_than_or_equal(node.getmempoolinfo()['bytes'], DEFAULT_CONSENSUS_BLOCK_SIZE * 2)

        new_block_hash = node.generatetoaddress(2, addr)[1]  # Mine 2 full blocks to bump ABLA state >32MB
        assert_equal(new_block_hash, node.getbestblockhash())  # Ensure blocks actually accepted & connected
        new_block_size = node.getblock(new_block_hash, 1)['size']
        new_abla_state = new_abla_state.NextBlockState(new_block_size)  # Advance abla state
        logging.info("Mined block size: " + str(new_block_size))
        assert_greater_than(new_abla_state.GetBlockSizeLimit(), DEFAULT_CONSENSUS_BLOCK_SIZE)
        assert_greater_than(new_abla_state.GetNextBlockSizeLimit(), DEFAULT_CONSENSUS_BLOCK_SIZE)
        assert_greater_than(new_block_size, DEFAULT_CONSENSUS_BLOCK_SIZE)

        # "Abusing" the node by restarting it again with upgrade10 activation at the beginning of time should make the
        # node figure out how to recover and have the correct abla state for the activation block, which is now genesis.
        self.restart_node(0, extra_args=['-upgrade10activationtime=0', '-debug=blk', '-debug=activation'])
        node.set("mining.unsafeGetBlockTemplate=true")


        assert_equal(self.get_abla_activation_height(), 0)
        genesis_size = node.getblock(node.getblockhash(0), 1)['size']
        # Genesis abla state should be "default" AblaState (with the blockSize applied)
        abla = AblaState.FromRpcDict(self.get_header(0))
        assert_equal(abla, AblaState(blockSize=genesis_size))

        # Iterate through all blocks and ensure that the entire chain got the corrected abla state post-restart
        last_height = self.get_header()['height']
        for ht in range(1, last_height + 1):
            bh = node.getblockhash(ht)
            size = node.getblock(bh, 1)['size']
            new_abla = abla.NextBlockState(size)
            assert_equal(AblaState.FromRpcDict(self.get_header(bh)), new_abla)
            abla = new_abla

        # Lastly, connect the nodes and ensure node0 and node1 synch up (ABLA working ok to synch blocks)
        connect_nodes(self.nodes[0], 1)
        self.sync_blocks()


if __name__ == '__main__':
    AblaTest().main()
