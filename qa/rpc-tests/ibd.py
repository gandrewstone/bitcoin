#!/usr/bin/env python3
# Copyright (c) 2014-2015 The Bitcoin Core developers
# Copyright (c) 2015-2016 The Bitcoin Unlimited developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.


from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

class IBDTest (BitcoinTestFramework):
    def __init__(self):
      self.rep = False
      BitcoinTestFramework.__init__(self)

    def setup_chain(self):
        print ("Initializing test directory "+self.options.tmpdir)
        initialize_chain_clean(self.options.tmpdir, 3)

    def setup_network(self, split=False):
        self.nodes = []
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug="]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug="]))
        self.nodes.append(start_node(2, self.options.tmpdir, ["-debug=", "-prune=1000"]))
        interconnect_nodes(self.nodes)
        self.is_network_split=False
        self.sync_all()

                    
    def run_test (self):
        
        # Mine a 2001 blocks chain.  Mining more than 2000 blocks will test the request
        # of a second GETHEADERS.
        print ("Mining blocks...")
        self.nodes[0].generate(500)
        print ("Mining blocks...")
        self.nodes[0].generate(500)
        print ("Mining blocks...")
        self.nodes[0].generate(500)
        print ("Mining blocks...")
        self.nodes[0].generate(501)
        print ("Finished mining iniital blocks...")

        # Stop nodes
        stop_nodes(self.nodes)
        wait_bitcoinds()

        ######################################################################
        # Verify that pruned and non-pruned nodes can sync from a regular node
        ######################################################################

        # Start the first node and mine 10 blocks
        print ("Mining 10 more blocks...")
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug="]))
        self.nodes[0].generate(10)

        # Connect the first NETWORK_NODE - all nodes should sync
        print ("Connect NETWORK_NODE...")
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug="]))
        connect_nodes(self.nodes[1],0)
        self.sync_all()

        # Connect the second node as non pruned  node (not a network node)- all nodes should sync
        print ("Connect Pruned node...")
        self.nodes.append(start_node(2, self.options.tmpdir, ["-debug=", "-prune=1000"]))
        connect_nodes(self.nodes[1],2)
        self.sync_all()
        
        #stop nodes
        stop_nodes(self.nodes)
        wait_bitcoinds()


        ##############################################################################
        # Verify that NETWORK_NODE will sync from pruned nodes that are close to today
        ##############################################################################

        # Mine blocks on node 0 and sync to the pruned node 1
        print ("Mining 10 more blocks...")
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug="]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug=", "-prune=1000"]))
        connect_nodes(self.nodes[0],1)
        self.nodes[0].generate(10)

        # Connect node2  only to pruned node 1.  They should sync.
        self.nodes.append(start_node(2, self.options.tmpdir, ["-debug="]))
        connect_nodes(self.nodes[2],1)
        self.sync_all()
        counts = [ x.getblockcount() for x in self.nodes ]
        assert_equal(counts, [2021, 2021, 2021])  
         
        #stop nodes
        stop_nodes(self.nodes)
        wait_bitcoinds()


        ######################################################################################
        # Verify that NETWORK_NODE will NOT sync from pruned nodes that are not close to today
        ######################################################################################

        # Mine blocks on node 0 and sync to the pruned node 1
        print ("Mining 10 more blocks...")
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug="]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug=", "-prune=1000"]))
        connect_nodes(self.nodes[0],1)
        self.nodes[0].generate(10)
        self.sync_all()

        # Advance the clock by one day and one second and then connect node2  only to pruned node 1.
        # They should NOT sync because the best header on node 2 is too old.
        self.nodes.append(start_node(2, self.options.tmpdir, ["-debug="]))
        cur_time = int(time.time()) * 60 * 60 * 24  + 1
        self.nodes[2].setmocktime(cur_time)
        connect_nodes(self.nodes[2],1)
        time.sleep(5); #give sync a chance to happen
        counts = [ x.getblockcount() for x in self.nodes ]
        assert_equal(counts, [2031, 2031, 2021])  
        print ("Success - did not sync with pruned node...")

        #stop nodes
        stop_nodes(self.nodes)
        wait_bitcoinds()


        ######################################################################################
        # Verify that NETWORK_NODE will sync from a NETWORK_NODE that is not close to today
        ######################################################################################

        # Mine blocks on node 0 and sync to the pruned node 1
        print ("Mining 1 more block...")
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug="]))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug=", "-prune=1000"]))
        connect_nodes(self.nodes[0],1)
        self.nodes[0].generate(1)

        # Advance the clock by one day and one second and then connect node2  only to network node 0.
        # They should sync even though we are more than 24 hours behind.
        self.nodes.append(start_node(2, self.options.tmpdir, ["-debug="]))
        cur_time = int(time.time()) * 60 * 60 * 24  + 1
        self.nodes[2].setmocktime(cur_time)
        connect_nodes(self.nodes[2],0)
        self.sync_all()
        print ("Success - sync with network node...")

        #stop nodes
        stop_nodes(self.nodes)
        wait_bitcoinds()

if __name__ == '__main__':
    IBDTest ().main ()

    
