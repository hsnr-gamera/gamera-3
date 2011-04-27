#!/usr/bin/env python
import sys, gc, os
from gamera.core import *
init_gamera()
import gamera.graph
import gamera.graph_util

#gc.set_debug(gc.DEBUG_LEAK)  
flags = [
      gamera.graph.DIRECTED, 
      gamera.graph.CYCLIC, #undirected
      gamera.graph.BLOB, #not tree
      gamera.graph.MULTI_CONNECTED,
      gamera.graph.SELF_CONNECTED,
      gamera.graph.FREE, #all flags set
      gamera.graph.TREE, #undirected, acyclic, single-connected, not-self-connected
      gamera.graph.FLAG_DAG, #directed, acyclic
      gamera.graph.UNDIRECTED #undirected, cyclic
   ]



# -----------------------------------------------------------------------------
# tests for small graphs only
# -----------------------------------------------------------------------------

# ------------------------------------------------------------------------------
def _test_flags(flag = gamera.graph.FREE):
   g = gamera.graph.Graph(flag)
   correct_flags = {
      gamera.graph.DIRECTED: [gamera.graph.DIRECTED], 
      gamera.graph.CYCLIC: [gamera.graph.CYCLIC], #undirected
      gamera.graph.BLOB: [gamera.graph.BLOB], #not tree
      gamera.graph.MULTI_CONNECTED: [],
      gamera.graph.SELF_CONNECTED: [],
      gamera.graph.FREE: [gamera.graph.DIRECTED, gamera.graph.CYCLIC, 
                          gamera.graph.BLOB, gamera.graph.MULTI_CONNECTED, 
                          gamera.graph.SELF_CONNECTED], #all flags set
      gamera.graph.TREE: [gamera.graph.TREE], 
         #undirected, acyclic, single-connected, not-self-connected
      gamera.graph.FLAG_DAG: [gamera.graph.DIRECTED, gamera.graph.BLOB, 
                              gamera.graph.FLAG_DAG], #directed, acyclic
      gamera.graph.UNDIRECTED: [gamera.graph.BLOB, gamera.graph.CYCLIC, 
                                gamera.graph.UNDIRECTED] #undirected, cyclic
   }

   if flag in [gamera.graph.DIRECTED, gamera.graph.FLAG_DAG, gamera.graph.FREE]:
      assert g.is_directed()
   else:
      assert g.is_undirected()

   if correct_flags.has_key(flag):# and f not in ignore_flags:
      for f in correct_flags[flag] :
         assert g.has_flag(f)



# ------------------------------------------------------------------------------
def _test_remove_difference(flag = gamera.graph.FREE):
   result = {
      gamera.graph.FREE:      [2,3,1,2,2,3,0,2],
      gamera.graph.DIRECTED:  [2,3,1,2,2,3,0,2],
      gamera.graph.CYCLIC:    [2,3,1,2,2,3,0,2], #undirected
      gamera.graph.BLOB:      [2,3,1,2,2,3,0,2],#not tree
      gamera.graph.MULTI_CONNECTED: [2,3,1,2,2,3,0,2],
      gamera.graph.SELF_CONNECTED:  [2,3,1,2,2,3,0,2],
      gamera.graph.FREE:      [2,3,1,2,2,3,0,2], #all flags set
      gamera.graph.TREE:      [2,3,1,2,2,3,0,2], 
         #undirected, acyclic, single-connected, not-self-conn
      gamera.graph.FLAG_DAG:  [2,3,1,2,2,3,0,2], #directed, acyclic
      gamera.graph.UNDIRECTED:      [2,3,1,2,2,3,0,2]#undirected, cyclic
   }

   g = gamera.graph.Graph()
   g.add_edge(1,2)
   g.add_edge(2,3)
   assert g.nedges == result[flag][0]
   assert g.nnodes == result[flag][1]
   g.remove_node(2)
   assert g.nedges == result[flag][2]
   assert g.nnodes == result[flag][3]
   assert "%s" % list(g.get_edges()) == "[<Edge from 1 to 3 (2.0)>]"
   del g
   
   g = gamera.graph.Graph()
   g.add_edge(1,2)
   g.add_edge(2,3)
   assert g.nedges == result[flag][4]
   assert g.nnodes == result[flag][5]
   g.remove_node_and_edges(2)
   assert g.nedges == result[flag][6]
   assert g.nnodes == result[flag][7]

   del g
 


# ------------------------------------------------------------------------------
def _test_colorize(flag = gamera.graph.FREE):
   g = gamera.graph.Graph(flag)

   for i in range(7):
      for j in range(7):
         g.add_edge(i,j)

   try:
      g.colorize(6)
      assert False
   except:
      assert True

   del g


   g = gamera.graph.Graph(flag)
   for i in range(7):
      g.add_edge(i, i+1)

   g.colorize(6)

   colorcount = [0] * 6
   for i in range(7):
      colorcount[g.get_color(i)] += 1

   for i in range(6):
      assert colorcount[i] > 0 and colorcount[i] < 3

   del g



# ------------------------------------------------------------------------------
def _test_bfs(flag = gamera.graph.FREE):
   correct_bfs = {}
   a = [1,19,5,4,2,6,3,7]
   b = [1,5,2,6,3,7]
   correct_bfs = {
      gamera.graph.UNDIRECTED: a,
      gamera.graph.DIRECTED: b,
      gamera.graph.CYCLIC: a,
      gamera.graph.BLOB: a,
      gamera.graph.MULTI_CONNECTED: a,
      gamera.graph.SELF_CONNECTED: a,
      gamera.graph.FREE: b,
      gamera.graph.TREE: a,
      gamera.graph.FLAG_DAG: b
   }
   
   result_bfs = []
   g = gamera.graph.Graph(flag)
   g.add_edge(4,19)
   g.add_edge(19,1)
   g.add_edge(1,5)
   g.add_edge(5,2)
   g.add_edge(5,6)
   g.add_edge(2,6)
   g.add_edge(6,3)
   g.add_edge(2,3)
   g.add_edge(7,3)
   g.add_edge(3,7)

#   gamera.graph_util.graphviz_output(g, "graph_bfs_%d.viz" % flag)
   for n in g.BFS(1):
      result_bfs.append(n())

   assert correct_bfs[flag] == result_bfs

   del g



# ------------------------------------------------------------------------------
def _test_dfs(flag = gamera.graph.FREE):
   a = [1001,1005,1006,1003,1007,1002,10019,1004]
   b = [1001,1005,1006,1003,1007,1002]
   correct_dfs = { 
      gamera.graph.UNDIRECTED: a,
      gamera.graph.DIRECTED: b,
      gamera.graph.CYCLIC: a,
      gamera.graph.BLOB: a,
      gamera.graph.MULTI_CONNECTED: a,
      gamera.graph.SELF_CONNECTED: a,
      gamera.graph.FREE: b,
      gamera.graph.TREE: a,
      gamera.graph.FLAG_DAG: b,
   }
   result_dfs = []
   g = gamera.graph.Graph(flag)
   g.add_edge(1004,10019)
   g.add_edge(10019,1001)
   g.add_edge(1001,1005)
   g.add_edge(1005,1002)
   g.add_edge(1005,1006)
   g.add_edge(1002,1006)
   g.add_edge(1006,1003)
   g.add_edge(1002,1003)
   g.add_edge(1007,1003)
   g.add_edge(1003,1007)
   for n in g.DFS(1001):
      #print n
      result_dfs.append(n())
   if correct_dfs.has_key(flag):
      assert correct_dfs[flag] == result_dfs

   del g



# ------------------------------------------------------------------------------
def _test_dijkstra(flag = gamera.graph.FREE):
   #TODO more correct_paths
   g = gamera.graph.Graph(flag)
   correct_paths = {
      gamera.graph.FREE: {8: {8: (0.0, [8]), 1: (7.0, [1, 8]), 2: (15.0, [2, 7, 1, 8]), 6: (6.0, [6, 8]), 7: (12.0, [7, 1, 8])}, 1: {8: (7.0, [8, 7, 1]), 1: (0.0, [1]), 2: (8.0, [2, 7, 1]), 6: (9.0, [6, 2, 7, 1]), 7: (5.0, [7, 1])}, 2: {8: (4.0, [8, 7, 2]), 1: (11.0, [1, 8, 7, 2]), 2: (0.0, [2]), 6: (1.0, [6, 2]), 7: (2.0, [7, 2])}, 6: {8: (4.0, [8, 6]), 1: (11.0, [1, 8, 6]), 2: (19.0, [2, 7, 1, 8, 6]), 6: (0.0, [6]), 7: (16.0, [7, 1, 8, 6])}, 7: {8: (2.0, [8, 7]), 1: (9.0, [1, 8, 7]), 2: (3.0, [2, 7]), 6: (4.0, [6, 2, 7]), 7: (0.0, [7])}}

   }

   g.add_edges([
      (1,2,10,True), (1,7,5,True), (2,6,1,True), (2,7,2,True),
      (6,8,4,True), (7,8,2,True), (7,2,3,True), (7,6,9,True),
      (8,1,7,True), (8,6,6,True)
   ])

   assert g.nedges == 10
   p = g.dijkstra_shortest_path(2)
   print {flag: p}
   if correct_paths.has_key(flag):
      assert p == correct_paths[flag][2]
   del p
   del g
 


# ------------------------------------------------------------------------------
def _test_dijkstra_all_pairs(flag = gamera.graph.FREE):
   #TODO more correct_paths
   g = gamera.graph.Graph(flag)
   correct_paths = {
      gamera.graph.FREE: {8: {8: (0.0, [8]), 1: (7.0, [1, 8]), 2: (15.0, [2, 7, 1, 8]), 6: (6.0, [6, 8]), 7: (12.0, [7, 1, 8])}, 1: {8: (7.0, [8, 7, 1]), 1: (0.0, [1]), 2: (8.0, [2, 7, 1]), 6: (9.0, [6, 2, 7, 1]), 7: (5.0, [7, 1])}, 2: {8: (4.0, [8, 7, 2]), 1: (11.0, [1, 8, 7, 2]), 2: (0.0, [2]), 6: (1.0, [6, 2]), 7: (2.0, [7, 2])}, 6: {8: (4.0, [8, 6]), 1: (11.0, [1, 8, 6]), 2: (19.0, [2, 7, 1, 8, 6]), 6: (0.0, [6]), 7: (16.0, [7, 1, 8, 6])}, 7: {8: (2.0, [8, 7]), 1: (9.0, [1, 8, 7]), 2: (3.0, [2, 7]), 6: (4.0, [6, 2, 7]), 7: (0.0, [7])}}
   }

   g.add_edges([
      (1,2,10,True), (1,7,5,True), (2,6,1,True), (2,7,2,True),
      (6,8,4,True), (7,8,2,True), (7,2,3,True), (7,6,9,True),
      (8,1,7,True), (8,6,6,True)
   ])

   assert g.nedges == 10
   p = g.all_pairs_shortest_path()
   if correct_paths.has_key(flag):
      assert p == correct_paths[flag]
   del p

   del g



#------------------------------------------------------------------------------
def _test_subgraph_roots(flag = gamera.graph.FREE):
   g = gamera.graph.Graph(flag);
   g.add_node(2)
   g.add_node(3)
      
   assert len(list(g.get_subgraph_roots())) == 2

   g.add_edges([(2,4), (5,7), (1,3), (8,9), (1,2), (5,6), (2,2)])
   g.add_node(10)
   tofind = [True, True, True, True]
   found = [False, False, False, False]

   i = 0
   if g.is_directed():
      for root in g.get_subgraph_roots():
         value = root()
         if(value == 1):
            found[0] = True
         elif(value == 5):
            found[1] = True
         elif(value == 8):
            found[2] = True
         elif(value == 10):
            found[3] = True
         i += 1
   else:
      for root in g.get_subgraph_roots():
         value = root()
         if(value <= 4):
            found[0] = True
         elif(value <= 7):
            found[1] = True
         elif(value <= 9):
            found[2] = True
         elif(value <=10):
            found[3] = True
         i += 1

   assert found == tofind
   assert i == 4


   del g



# ------------------------------------------------------------------------------
def _test_add_node(flag = gamera.graph.FREE):
   g = gamera.graph.Graph(flag)
   count_before = g.nnodes
   was_existing = g.has_node(43)
   assert g.add_node(43) == True
   assert (was_existing and g.nnodes == count_before) or \
         (not was_existing and g.nnodes == count_before + 1)

   assert g.add_node(43) == False

   del g



# ------------------------------------------------------------------------------
def _test_fully_connected(flag = gamera.graph.FREE):
   g = gamera.graph.Graph(flag)

   g.add_node(9)
   g.add_node(10)
   g.add_node(11)
   g.add_node(12)
   assert g.is_fully_connected() == False

   g.add_edge(9,10)
   g.add_edge(11,12)

   assert g.is_fully_connected() == False

   g.add_edge(10,11)
   print list(g.get_edges())
   print list(g.get_nodes())
   assert g.is_fully_connected() == True

   del g



# ------------------------------------------------------------------------------
def _test_is_cyclic(flag = gamera.graph.FREE): 
   g = gamera.graph.Graph(flag)
   assert g.is_cyclic() == False
   g.add_node(1)
   assert g.is_cyclic() == False

   g.add_edge(1,2)
   g.add_edge(2,3)

   assert g.is_cyclic() == False

   g.add_edge(3,1)
   assert g.is_cyclic() == True
   del g



# ------------------------------------------------------------------------------
def _test_is_multi_connected(flag = gamera.graph.FREE):
   g = gamera.graph.Graph(flag)
   g.add_nodes([1,2,3])
   g.add_edge(1,2)
   g.add_edge(1,2)
   assert g.is_multi_connected()
   del g

   g = gamera.graph.Graph(flag)
   if g.is_undirected():
      g.add_edge(1,2)
      g.add_edge(2,1)
      assert g.is_multi_connected()
      del g
      g = gamera.graph.Graph(flag)

   g.add_nodes([1,2,3])
   g.add_edge(1,2)
   assert not g.is_multi_connected()
   del g



# ------------------------------------------------------------------------------
def _test_is_self_connected(flag = gamera.graph.FREE):
   g = gamera.graph.Graph(flag)
   g.add_node([1,2,3])
   g.add_edge(1,2)
   assert not g.is_self_connected()
   g.add_edge(1,1)
   assert g.is_self_connected()
   del g



# ------------------------------------------------------------------------------
def _test_make_singly_connected(flag = gamera.graph.FREE):
   g = gamera.graph.Graph(flag)
   g.add_nodes([1,2,3,4,5,6,7,8])
   g.add_edges([(1,2),(1,2),(3,4),(4,5),(4,5),(5,6)])
   assert g.is_multi_connected()
   g.make_singly_connected()
   assert not g.is_multi_connected()
   assert g.nedges == 4
   del g



# ------------------------------------------------------------------------------
def _test_make_not_self_connected(flag = gamera.graph.FREE):
   g = gamera.graph.Graph(flag)
   g.add_nodes([1,2,3,4,5,6,7,8])
   g.add_edges([(1,1),(1,2),(2,3),(2,2),(3,4),(4,5),(5,5)])
   assert g.is_self_connected()
   g.make_not_self_connected()
   assert not g.is_self_connected()
   assert g.nedges == 4



# ------------------------------------------------------------------------------
def _test_make_acyclic(flag = gamera.graph.FREE):
   g = gamera.graph.Graph(flag)
   g.make_cyclic()
   g.add_edge(1,2,1,True)
   g.add_edge(2,3,1,True)
   g.add_edge(3,4,1,True)
   g.add_edge(4,5,1,True)
   g.add_edge(4,2,1,True)
   assert g.is_cyclic()
   assert g.nedges == 5
   g.make_acyclic()
   assert g.nedges == 4
   assert not g.is_cyclic()

   del g



# ------------------------------------------------------------------------------
def _test_make_tree(flag = gamera.graph.FREE):
   a = "[<Edge from 1 to 2 (10.0)>, <Edge from 1 to 7 (5.0)>, <Edge from 8 to 6 (6.0)>, <Edge from 1 to 8 (7.0)>]"
   b = "[<Edge from 1 to 2 (10.0)>, <Edge from 1 to 7 (5.0)>, <Edge from 7 to 6 (9.0)>, <Edge from 1 to 8 (7.0)>]"

   correct = { 
      gamera.graph.UNDIRECTED: a,
      gamera.graph.DIRECTED: b,
      gamera.graph.CYCLIC: a,
      gamera.graph.BLOB: a,
      gamera.graph.MULTI_CONNECTED: a,
      gamera.graph.SELF_CONNECTED: a,
      gamera.graph.FREE: b,
      gamera.graph.TREE: a,
      gamera.graph.FLAG_DAG: b,
   }

   g = gamera.graph.Graph(flag)
   g.add_edges([
      (1,2,10,True), (1,7,5,True), (2,6,1,True), (2,7,2,True),
      (6,8,4,True), (7,8,2,True), (7,2,3,True), (7,6,9,True),
      (8,1,7,True), (8,6,6,True)
   ])

   g.make_tree()

   if correct.has_key(flag):
      assert correct[flag]  == "%s" % list(g.get_edges())

   del g



# ------------------------------------------------------------------------------
def _test_check_insert_restrictions(flag = None):
   results = {
      gamera.graph.UNDIRECTED: [3, [(1,2),(2,3),(3,1)], []],
      gamera.graph.DIRECTED: [2, [(1,2),(2,3)], []],
      gamera.graph.CYCLIC: [3, [(1,2),(2,3),(3,1)], []],
      gamera.graph.BLOB: [3, [(1,2),(2,3)], []],
      gamera.graph.MULTI_CONNECTED: [None, None, []],
      gamera.graph.SELF_CONNECTED: [None, None, None],
      gamera.graph.FREE: [3, [(1,2),(2,3),(3,1)], [(3,3)]],
      gamera.graph.TREE: [2, [(1,2),(2,3)], []],
      gamera.graph.FLAG_DAG: [2, [(1,2),(2,3)], []]
   }

   
   g = gamera.graph.Graph(flag | gamera.graph.CHECK_ON_INSERT)
   g.add_nodes([1,2,3,4,5,6,7,8,9])
   
   #tests acyclic restriction
   g.add_edge(1,2)
   print g.nedges
   g.add_edge(2,3)
   print g.nedges
   g.add_edge(3,1)
   print g.nedges
   if results.has_key(flag) and results[flag][1] != None:
#      assert g.nedges == results[flag][0]
      assert sorted([(e.from_node(), e.to_node()) for e in list(g.get_edges())]) \
            == sorted(results[flag][1])

   del g


   #tests self-connected restriction
   g = gamera.graph.Graph(flag | gamera.graph.CHECK_ON_INSERT)
   g.add_nodes([1,2,3,4,5,6,7,8,9])
   g.add_edge(3,3)
   if results.has_key(flag) and results[flag][2] != None:
      assert sorted([(e.from_node(), e.to_node()) for e in g.get_edges()]) \
            == sorted(results[flag][2])


   #tests multi-connected restriction
   #TODO



# ------------------------------------------------------------------------------
def _test_minimum_spanning_tree(flag = gamera.graph.FREE):
   g = gamera.graph.Graph(flag)
   print g.add_edges([
      (9,10,4), (9,16,8), (10,16,11), (10,11,8), 
      (11,17,2), (11,14,4), (11,12,7), (12,13,9), 
      (12,14,14), (13,14,10), (14,15,2), (15,17,6), 
      (15,16,1), (16,17,7)]
   )
   assert g.nedges == 14

   try:
#      gamera.graph_util.graphviz_output(g, "mst_before_%d.viz" % flag)
      t = g.create_minimum_spanning_tree()

#      gamera.graph_util.graphviz_output(t, "mst_after_%d.viz" % flag)
      assert t.nnodes == 9
      assert t.nedges == 8
      assert "[<Edge from 15 to 16 (1.0)>, <Edge from 11 to 17 (2.0)>, <Edge from 14 to 15 (2.0)>, <Edge from 11 to 14 (4.0)>, <Edge from 9 to 10 (4.0)>, <Edge from 11 to 12 (7.0)>, <Edge from 9 to 16 (8.0)>, <Edge from 12 to 13 (9.0)>]" == str(list(t.get_edges()))

      del t

   except TypeError:
      assert g.is_directed()

   del g



# ------------------------------------------------------------------------------
def _test_spanning_tree(flag = gamera.graph.FREE):
   g = gamera.graph.Graph(flag)
   print g.add_edges([
      (9,10,4), (9,16,8), (10,16,11), (10,11,8), 
      (11,17,2), (11,14,4), (11,12,7), (12,13,9), 
      (12,14,14), (13,14,10), (14,15,2), (15,17,6), 
      (15,16,1), (16,17,7)]
   )
   assert g.nedges == 14

   bfsnodes = [n() for n in g.BFS(11)].sort()

   t = g.create_spanning_tree(11)
   treenodes = [n() for n in t.get_nodes()].sort()

   assert treenodes == bfsnodes

   del t
   del g





# -----------------------------------------------------------------------------
# tests for larger graphs
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
def _test_add_nodes(flag = gamera.graph.FREE, count = 2000):
   g = gamera.graph.Graph(flag)
   
   for i in range(1000,1000+count):
      assert g.has_node(i) == False

   for i in range(1000,1000+count):
      g.add_node(i)
      assert g.nnodes == i+1-1000
      assert g.has_node(i) == True
      assert g.get_node(i)() == i

   assert g.has_node(1000+count/2) == True
   assert g.get_node(1000+count/2)() == 1000+count/2

   for i in range(1000,1000+count):
      print i
      assert g.has_node(i) == True
      assert g.get_node(i)() == i

   try:
      g.get_node(1000 + 2*count)
      assert False
   except ValueError:
      pass

   del g



# ----------------------------------------------------------------------------
def _test_graph_copy(flag = gamera.graph.FREE, count = 1000):
   if count > 100:
      g = gamera.graph.Graph(flag)
      for i in range(0,count):
         g.add_edge(i, i+1)

      g.add_edge(count / 2, count / 2 + 20)
      assert g.nedges == count+1
      assert g.is_cyclic()
      h = g.copy(gamera.graph.CHECK_ON_INSERT)
      del g
      assert h.is_acyclic()
      assert h.nedges == count
      del h



# ------------------------------------------------------------------------------
def _test_remove_nodes(flag = gamera.graph.FREE, count = 250):
   g = gamera.graph.Graph(flag)
   g.add_nodes(range(0,count))
   assert g.nnodes == count
   for i in range(0, count):
      g.remove_node(i)
      assert g.nnodes == count - i - 1

   for i in range(0,count):
      assert not g.has_node(i)

   assert g.nnodes == 0

   del g



# ------------------------------------------------------------------------------
def _test_remove_node_and_edges(flag = gamera.graph.FREE, count = 250):
   g = gamera.graph.Graph(flag)

   for i in range(0,count):
      g.add_edge(i,i+1)

   g.add_edge(count, 0)
   assert g.nedges == count+1
   assert g.nnodes == count+1

   for i in range(0,count):
      g.remove_node_and_edges(i)
      assert g.nedges == count-i-1
   
   assert g.nnodes == 1 #only count
   assert g.nedges == 0

   assert [x() for x in g.get_nodes()] == [count]

   del g

   g = gamera.graph.Graph(flag)

   for i in range(0,count):
      g.add_edge(i,i+1)

   assert g.nedges == count
   assert g.nnodes == count+1

   for i in range(1,count):
      g.remove_node(i)
      assert g.nedges == count-i

   assert g.nnodes == 2
   print list(g.get_edges()) 
   assert g.nedges == 1
   g.remove_node(0)
   assert g.nedges == 0
   assert g.nnodes == 1 #only count

   assert [x() for x in g.get_nodes()] == [count]

   del g



# ------------------------------------------------------------------------------
def _test_remove_all_edges(flag = gamera.graph.FREE, count = 250):
   g = gamera.graph.Graph(flag)

   for i in range(0,count):
      g.add_edge(i,i+1)

   g.add_edge(count, 0)
   assert g.nedges == count+1
   assert g.nnodes == count+1

   g.remove_all_edges()
   assert g.nedges == 0
   assert g.nnodes == count+1

   del g



# ------------------------------------------------------------------------------
def _test_remove_node2(flag = gamera.graph.FREE, count = 250):
   g = gamera.graph.Graph()

   for i in range(0,count):
      g.add_edge(i,i+1)

   g.add_edge(count,0)
   assert g.nedges == count+1
   assert g.nnodes == count+1

   for i in range(0,count-1):
      g.remove_node(i)
      assert g.nedges == count-i

   g.remove_node(count-1)
   assert g.nedges == 0
   
   assert g.nnodes == 1 #only count
   assert g.nedges == 0

   assert [x() for x in g.get_nodes()] == [count]

   del g



# ------------------------------------------------------------------------------
def _test_add_nodes_sequence(flag = gamera.graph.FREE, count = 2000):
   g = gamera.graph.Graph(flag)
   for i in range(1000,1000+count):
      assert g.has_node(i) == False

   g.add_nodes(list(range(1000,1000+count)))

   assert g.has_node(1000+count/2) == True
   assert g.get_node(1000+count/2)() == 1000+count/2

   for i in range(1000,1000+count):
      assert g.has_node(i) == True
      assert g.get_node(i)() == i

   assert g.nnodes == count

   assert g.has_node(500) == False

   del g



# ------------------------------------------------------------------------------
def _test_has_edge(flag = gamera.graph.FREE, count = 250):
   g = gamera.graph.Graph(flag)
   for i in range(0,count):
      g.add_edge(i,i+1)

   for i in range(0,count):
      assert g.has_edge(i,i+1)
   
   for e in g.get_edges():
      assert g.has_edge(e)
      assert g.has_edge(e.from_node, e.to_node)
      assert g.has_edge(e.from_node(), e.to_node())

   assert g.nedges == count
   assert g.nnodes == count+1

   for i in range(0,count):
      g.remove_node(i)
   
   assert g.nnodes == 1 #only count
   assert g.nedges == 0

   assert [x() for x in g.get_nodes()] == [count]

   del g



# ------------------------------------------------------------------------------
def _test_remove_edge(flag = gamera.graph.FREE, count = 250):
   g = gamera.graph.Graph(flag)
   for i in range(0,count):
      g.add_edge(i,i+1)

   for i in range(0,count):
      assert g.has_edge(i,i+1)
   edgecount = g.nedges
   for e in list(g.get_edges()):
      from_node, to_node = e.from_node, e.to_node

      g.remove_edge(from_node, to_node)
      assert not g.has_edge(from_node(), to_node())
      edgecount -= 1
      assert edgecount == g.nedges

   assert g.nedges == 0
   assert g.nnodes == count+1
   assert [x() for x in g.get_nodes()] == range(0,count+1)
   
   try:
      g.remove_edge(5*count, 6*count)
      assert False
   except RuntimeError:
      pass

   del g

   g = gamera.graph.Graph()
   for i in range(0,count):
      g.add_edge(i,i+1)

   for i in range(0,count):
      assert g.has_edge(i,i+1)
    
   g.remove_all_edges()

   for i in range(0,count):
      assert not g.has_edge(i,i+1)

   print g.nedges
   print list(g.get_edges())
   assert g.nedges == 0
   assert g.nnodes == count+1
   assert [x() for x in g.get_nodes()] == range(0,count+1)

   del g



# ------------------------------------------------------------------------------
allocs = {"in_graph": 0, "add_edge_1": 0, "add_edge_2": 0, "check": 0}
deallocs = {"in_graph": 0, "add_edge_1": 0, "add_edge_2": 0, "check": 0}
objects = []
def _test_userdefined_class(flag = gamera.graph.FREE, count = 4096):
   class Test:
      def __init__(self, a, b="in_graph"):
         global objects
         global allocs
         objects.append(self)
         allocs[b] += 1
         self.a = a
         self.b = b

      def __cmp__(self, other):
         return cmp(self.a, other.a)

      def __eq__(self, other):
         return self.a == other.a

      def __lt__(self, other):
         return self.a < other.a
      
      def __repr__(self):
         return "Test(%s, %s)" % (self.a, self.b)

      def __del__(self):
#         global deallocs
#         deallocs[self.b] += 1
         pass

   g = gamera.graph.Graph(gamera.graph.UNDIRECTED)
   g.add_node(Test(0))
   for i in range(0, count):
      g.add_node(Test(i+1))
      g.add_edge(Test(i, "add_edge_1"),Test(i+1, "add_edge_2"))

   for node in g.get_nodes():
      o = node()
  
   for i in range(0, count):
      assert Test(i) == g.get_node(Test(i))()

   for e in g.get_edges():
      assert e.from_node().a == e.to_node().a-1
      print "Edge %s %s \n" % (e.from_node(), e.to_node())

   allocs = {"in_graph": 0, "add_edge_1": 0, "add_edge_2": 0, "check": 0}
   deallocs = {"in_graph": 0, "add_edge_1": 0, "add_edge_2": 0, "check": 0}
   objects = []

   del g
 


# ------------------------------------------------------------------------------
def _test_large_graph(flag = gamera.graph.FREE, count = 20000):
   gc.collect()
   print "memlarge: %s" % memory_usage()
   g = gamera.graph.Graph(flag)
   g.add_node(0)
   for i in range(0, count):
      g.add_node(i+1)
      g.add_edge(i,i+1)

   for i in range(0, count):
      assert g.has_node(i) == True
      assert g.has_edge(i,i+1) == True
      assert g.get_node(i)() == i

   assert g.nnodes == count + 1
   g.remove_edge(count/2, count/2+1)
   del g
   gc.collect()
   print "memlarge_after: %s" % memory_usage()







# ------------------------------------------------------------------------------
def memory_usage():
      cmd = "ps u -p %i | awk '{sum=sum+$6}; END {print sum}'" % os.getpid()
      p = os.popen(cmd, "r")
      memory = p.readline().strip()
      p.close()
      return memory



# ------------------------------------------------------------------------------
def refcount():
      count = 0
      print len(gc.get_objects())
      for obj in gc.get_objects():
         count += sys.getrefcount(obj)
   
      return count



# ------------------------------------------------------------------------------
def _test_memory(flag = gamera.graph.FREE, count = 100):


   new_usage = None
   old_usage = new_usage
   for i in range(0,5):
      old_usage=new_usage
      _test_large_graph(flag)
      gc.collect()
      new_usage = (memory_usage(), refcount())
      print old_usage, new_usage
      assert old_usage == new_usage or old_usage == None or i < 2

   print "memory: %s | refcount: %d" % (memory_usage(), refcount())
   new_usage = None
   for i in range(0,count):
      old_usage = new_usage
      g = gamera.graph.Graph(flag)
      g.add_node(200003+i)
      g.add_node(300003+i)
      g.add_edge(200003+i,300003+i)
      b = g.get_node(200003+i)
      assert g.has_node(200003+i) == True
      assert g.has_edge(200003+i, 300003+i)
      del g
      g = None
      gc.collect()
      new_usage = (memory_usage(), refcount())
      print old_usage,new_usage
      assert old_usage == new_usage or old_usage == None or i < 2



tests_small = [
   _test_flags,
   _test_remove_difference,
   _test_bfs,
   _test_dfs,
   _test_dijkstra,
   _test_dijkstra_all_pairs,
   _test_subgraph_roots,
   _test_add_node,
   _test_fully_connected,
   _test_is_cyclic,
   _test_is_multi_connected,
   _test_is_self_connected,
   _test_make_singly_connected,
   _test_make_not_self_connected,
   _test_make_acyclic,
   _test_make_tree,
   _test_check_insert_restrictions,
   _test_minimum_spanning_tree,
   _test_spanning_tree,
   _test_colorize
]

tests_large = [
   _test_add_nodes,
   _test_graph_copy,
   _test_remove_nodes,
   _test_remove_node_and_edges,
   _test_remove_node2,
   _test_remove_all_edges,
   _test_add_nodes_sequence,
   _test_has_edge,
   _test_remove_edge,
   _test_userdefined_class,
   _test_large_graph
]

#   _test_memory


#tests = [_test_dijkstra]

# ------------------------------------------------------------------------------
def _test_memory_all():
   for test in tests:#lag in flags:
      for flag in flags:#test in tests:
#      try:
         print >>sys.stderr, flag, test
         new_usage = None
         for i in range(0,10):
            old_usage = new_usage
            test(flag)
            gc.collect()
            new_usage = (memory_usage(), refcount())
#      print >>sys.stderr, (old_usage, new_usage)
            if old_usage != None:
               if not ( (old_usage[1] == new_usage[1] \
                         and (int(new_usage[0])-int(old_usage[0]) < 2  )) \
                       or i < 4):
                  print >>sys.stderr, (old_usage, new_usage)

'''      except AssertionError:
         print >>sys.stderr, "%s failed !!!" %test
         info = sys.exc_info()
         exc_type = info[0]
         exc_value = info[1]
         exc_traceback = info[2]
                     
         trace = traceback.extract_tb(sys.exc_traceback)
         print >>sys.stderr, "Exception Type:  ", exc_type
         print >>sys.stderr, "Error Message:   ", exc_value
         print >>sys.stderr, "File name:       ", trace[0][0]
         print >>sys.stderr, "Error message:   ", trace[0][1]
         print >>sys.stderr, "Line:            ", trace[0][2]
         print >>sys.stderr, "Function:        ", trace[0][3]
'''



# ------------------------------------------------------------------------------
#prepare py.test
class TestGraph:
   pass

def _make_test(t, flag):
   def _t(self):
      return t(flag)
   return _t

def _make_test_size(t, flag, size):
   def _t(self):
      return t(flag, size)
   return _t

#small tests
for test in tests_small:
   for flag in flags:
      setattr(TestGraph, "test_small_%d_%s" % (flag, test.__name__ ), _make_test(test,flag))

#large default
#for test in tests_large:
#   for flag in flags:
#      setattr(TestGraph, "test_large_default_%d_%s" % (flag, test.__name__ ), _make_test(test,flag))

#large non-default
for size in [512]:
   for test in tests_large:
      for flag in flags:
         setattr(TestGraph, "test_large_%d_%d_%s" % (flag, size, test.__name__ ), 
                 _make_test_size(test, flag, size))
   




if __name__ == "__main__":
   t = TestGraph()
   for m in dir(t):
      if m.startswith("test_"):
         print m
         test = getattr(t, m)
         print test
         try:
            test()
         except AssertionError:
            pass
         #est(TestGraph)

   _test_memory_all()





# ------------------------------------------------------------------------------
def test_textline_reading_order():
   from gamera.plugins.pagesegmentation import textline_reading_order
   correct_orders = {"data/reading_order_2.png": [(42, 40, 1462, 114), (42, 158, 683, 232), (42, 276, 683, 350), (42, 418, 683, 492), (42, 560, 683, 633), (822, 158, 1462, 633), (42, 701, 1462, 775), (42, 843, 494, 917), (562, 843, 1132, 917), (42, 985, 683, 1059), (822, 985, 1132, 1059), (1200, 843, 1462, 1059)],
                     "data/reading_order.png": [(51, 56, 1471, 130), (51, 174, 691, 248), (51, 292, 691, 366), (51, 434, 691, 508), (51, 576, 691, 649), (830, 174, 1471, 508), (830, 576, 1471, 649), (51, 717, 1471, 791), (51, 859, 691, 933), (51, 1001, 691, 1075), (830, 859, 1471, 933), (830, 1001, 1471, 1075)]
                    }

   for file, correct in correct_orders.items():
      img = load_image(file)
      ccs = img.cc_analysis()
      ro = textline_reading_order(ccs)

      result = [(a.ul_x, a.ul_y, a.lr_x, a.lr_y) for a in ro]
      assert result == correct
      del ro
      del ccs
      del img


