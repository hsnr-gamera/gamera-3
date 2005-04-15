import py.test
import sys

from gamera import graph

###############################################################################
# PART I: Tests for explicit values on small graphs

def _construct1(flag):
   g = graph.Graph(flag)
   print g.add_edge('a', 'b')
   print g.add_edge('a', 'c')
   print g.add_edge('a', 'd')
   print g.add_edge('b', 'd')
   print g.add_edge('b', 'd')
   print g.add_edge('c', 'e')
   print g.add_edge('f', 'g')
   print g.add_edge('e', 'e')
   return g

def _construct2(flag):
   g = graph.Graph(flag)
   g.add_node('a')
   g.add_node('b')
   g.add_node('c')
   g.add_node('d')
   g.add_node('e')
   g.add_node('f')
   g.add_node('g')
   print g.add_edge('a', 'b')
   print g.add_edge('a', 'c')
   print g.add_edge('a', 'd')
   print g.add_edge('b', 'd')
   print g.add_edge('b', 'd')
   print g.add_edge('c', 'e')
   print g.add_edge('f', 'g')
   print g.add_edge('e', 'e')
   return g

def _construct3(flag):
   g = graph.Graph(flag)
   g.add_nodes('abcdefg')
   g.add_edges([('a', 'b'), ('a', 'c'), ('a', 'd'), ('b', 'd'),
                ('b', 'd'), ('c', 'e'), ('f', 'g'), ('e', 'e')])
   return g

def _main_test(g, nnodes, nedges, bfs_a, dfs_a, bfs_e, bfs_f, subgraphs,
               *args):
   def _missing_node():
      node = g.get_node('h')
   print (g.nnodes, g.nedges, \
          ''.join([x() for x in g.BFS('a')]),
          ''.join([x() for x in g.DFS('a')]),
          ''.join([x() for x in g.BFS('e')]),
          ''.join([x() for x in g.BFS('f')]),
          [''.join([x() for x in g.BFS(y)]) for y in g.get_subgraph_roots()])
   assert g.nnodes == nnodes
   assert g.nedges == nedges
   assert g.get_node('a')() == 'a'
   py.test.raises(ValueError, _missing_node)
   nodes = list([x() for x in g.get_nodes()])
   nodes.sort()
   assert ''.join(nodes) == 'abcdefg'

   assert len(list(g.get_edges())) == nedges
   assert g.has_edge('a', 'c')

   assert ''.join([x() for x in g.BFS('a')]) == bfs_a
   assert ''.join([x() for x in g.DFS('a')]) == dfs_a
   assert ''.join([x() for x in g.BFS('e')]) == bfs_e
   assert ''.join([x() for x in g.BFS('f')]) == bfs_f
   assert [''.join([x() for x in g.BFS(y)])
           for y in g.get_subgraph_roots()] == subgraphs
   assert ([g.size_of_subgraph(x) for x in g.get_subgraph_roots()] ==
           [len(x) for x in subgraphs])
   assert not g.is_fully_connected()

def _test_remove1(g, nnodes, nedges, bfs_a, dfs_a, bfs_e, bfs_f, subgraphs,
                  after_remove_c, after_remove_c_edges, after_remove_edge,
                  *args):
   g.remove_node('c')
   print "test_remove1", g.nedges
   assert g.nedges == after_remove_c

def _test_remove2(g, nnodes, nedges, bfs_a, dfs_a, bfs_e, bfs_f, subgraphs,
                  after_remove_c, after_remove_c_edges, after_remove_edge,
                  *args):
   g.remove_node_and_edges('c')
   print "test_remove2", g.nedges
   assert g.nedges == after_remove_c_edges

def _test_remove3(g, nnodes, nedges, bfs_a, dfs_a, bfs_e, bfs_f, subgraphs,
                  after_remove_c, after_remove_c_edges, after_remove_edge,
                  *args):
   g.remove_edge('a', 'c')
   print "test_remove3", g.nedges
   assert g.nedges == after_remove_edge

def _test_remove4(g, *args):
   g.remove_all_edges()
   assert g.nedges == 0
   assert len(list(g.BFS('a'))) == 1

def _test_convert1(g, *args):
   if g.is_directed():
      g.make_undirected()
   elif g.is_undirected():
      g.make_directed()
   else:
      assert False

def _test_convert2(g, *args):
   if g.is_cyclic():
      edges = g.nedges
      g.make_acyclic()
      assert g.nedges < edges
   elif g.is_acyclic():
      g.make_cyclic()
      assert g.is_cyclic()
   else:
      assert False

def _test_convert3(g, *args):
   if g.is_tree():
      g.make_blob()
      assert g.is_blob()
   elif g.is_blob():
      edges = g.nedges
      g.make_tree()
      assert g.nedges <= edges
   else:
      assert False

def _test_convert4(g, *args):
   if g.is_multi_connected():
      edges = g.nedges
      g.make_singly_connected()
      assert g.nedges <= edges
   elif g.is_singly_connected():
      g.make_multi_connected()
   else:
      assert False

def _test_convert5(g, *args):
   if g.is_self_connected():
      edges = g.nedges
      g.make_not_self_connected()
      assert g.nedges <= edges
   else:
      g.make_self_connected()

def _make_test_copy(flag):
   def _test(g, *args):
      g.copy(flag)
   return _test
_test_copy1 = _make_test_copy(graph.FREE)
_test_copy2 = _make_test_copy(graph.UNDIRECTED)
_test_copy3 = _make_test_copy(graph.TREE)
_test_copy4 = _make_test_copy(graph.FLAG_DAG)

def _test_djikstra_shortest_path(g, *args):
   shortest_path =  {'a': (0.0, []), 'c': (1.0, ['c']), 'b': (1.0, ['b']), 'e': (2.0, ['c', 'e']), 'd': (1.0, ['d'])}.values()
   shortest_path.sort()
   path = g.shortest_path('a').values()
   path.sort()
   assert path == shortest_path

all_pairs_shortest_path_results = [
      {'a': {'a': (0.0, []), 'c': (1.0, ['c']), 'b': (1.0, ['b']), 'e': (2.0, ['c', 'e']), 'd': (1.0, ['d'])}, 'c': {'a': (1.0, ['a']), 'c': (0.0, []), 'b': (2.0, ['a', 'b']), 'e': (1.0, ['e']), 'd': (2.0, ['a', 'd'])}, 'b': {'a': (1.0, ['a']), 'c': (2.0, ['a', 'c']), 'b': (0.0, []), 'e': (3.0, ['a', 'c', 'e']), 'd': (2.0, ['a', 'd'])}, 'e': {'a': (2.0, ['c', 'a']), 'c': (1.0, ['c']), 'b': (3.0, ['c', 'a', 'b']), 'e': (0.0, []), 'd': (3.0, ['c', 'a', 'd'])}, 'd': {'a': (1.0, ['a']), 'c': (2.0, ['a', 'c']), 'b': (2.0, ['a', 'b']), 'e': (3.0, ['a', 'c', 'e']), 'd': (0.0, [])}, 'g': {'g': (0.0, []), 'f': (1.0, ['f'])}, 'f': {'g': (1.0, ['g']), 'f': (0.0, [])}},
      {'a': {'a': (0.0, []), 'c': (1.0, ['c']), 'b': (1.0, ['b']), 'e': (2.0, ['c', 'e']), 'd': (1.0, ['d'])}, 'c': {'a': (1.0, ['a']), 'c': (0.0, []), 'b': (2.0, ['a', 'b']), 'e': (1.0, ['e']), 'd': (2.0, ['a', 'd'])}, 'b': {'a': (1.0, ['a']), 'c': (2.0, ['a', 'c']), 'b': (0.0, []), 'e': (3.0, ['a', 'c', 'e']), 'd': (1.0, ['d'])}, 'e': {'a': (2.0, ['c', 'a']), 'c': (1.0, ['c']), 'b': (3.0, ['c', 'a', 'b']), 'e': (0.0, []), 'd': (3.0, ['c', 'a', 'd'])}, 'd': {'a': (1.0, ['a']), 'c': (2.0, ['a', 'c']), 'b': (1.0, ['b']), 'e': (3.0, ['a', 'c', 'e']), 'd': (0.0, [])}, 'g': {'g': (0.0, []), 'f': (1.0, ['f'])}, 'f': {'g': (1.0, ['g']), 'f': (0.0, [])}},
      {'a': {'a': (0.0, []), 'c': (1.0, ['c']), 'b': (1.0, ['b']), 'e': (2.0, ['c', 'e']), 'd': (1.0, ['d'])}, 'c': {'c': (0.0, []), 'e': (1.0, ['e'])}, 'b': {'b': (0.0, []), 'd': (1.0, ['d'])}, 'e': {'e': (0.0, [])}, 'd': {'d': (0.0, [])}, 'g': {'g': (0.0, [])}, 'f': {'g': (1.0, ['g']), 'f': (0.0, [])}}]

def _test_all_pairs_shortest_path(g, *args):
   shortest_paths = g.djikstra_all_pairs_shortest_path()
   assert shortest_paths in all_pairs_shortest_path_results
   # TODO: Broken
##    shortest_paths2 = g.all_pairs_shortest_path()
##    assert shortest_paths2 in all_pairs_shortest_path_results
##    assert shortest_paths == shortest_paths2

def _test_spanning_tree(g, *args):
   nedges = g.nedges
   g2 = g.create_spanning_tree('a')
   nodes = [x() for x in g2.get_nodes()]
   nodes.sort()
   print g2.nedges
   assert ''.join(nodes) == 'abcde'
   assert g2.nedges <= g.nedges

def _test_minimum_spanning_tree(g, *args):
   nedges = g.nedges
   g2 = g.create_minimum_spanning_tree()
   nodes = [x() for x in g2.get_nodes()]
   nodes.sort()
   assert ''.join(nodes) == 'abcdefg'
   assert g2.nedges <= g.nedges

# Partitions testing is taken care of in the context
# of the grouping algorithm

class TestGraph:
   pass

def _make_test(constructor, tester, flag, *args):
   def _test(self):
      print >> sys.stderr, flag, constructor.__name__, tester.__name__
      sys.stderr.flush()
      return tester(constructor(flag), *args)
   return _test

for g in [(graph.FREE, 7, 8, 'abcde', 'adceb', 'e', 'fg',
           ['abcde', 'fg'], 7, 6, 7),
          (graph.UNDIRECTED, 7, 7, 'abcde', 'adceb', 'ecabd', 'fg',
           ['badce', 'gf'], 6, 5, 6),
          (graph.TREE, 7, 5, 'abcde', 'adceb', 'ecabd', 'fg',
           ['bacde', 'gf'], 4, 3, 4),
          (graph.FLAG_DAG, 7, 6, 'abcde', 'adceb', 'e', 'fg',
           ['abcde', 'fg'], 5, 4, 5)]:
   for constructor in [_construct1, _construct2, _construct3]:
      for _tester in [_main_test,
                      _test_remove1, _test_remove2, _test_remove3,
                      _test_remove4,
                      _test_convert1, _test_convert2, _test_convert3,
                      _test_convert4, _test_convert5,
                      _test_copy1, _test_copy2, _test_copy3, _test_copy4,
                      _test_djikstra_shortest_path,
                      _test_all_pairs_shortest_path,
                      _test_spanning_tree,
                      _test_minimum_spanning_tree]:
         setattr(TestGraph, 'test_%d_%s_%s' %
                 (g[0], constructor.__name__, _tester.__name__),
                 _make_test(constructor, _tester, *g))

##############################################################################
# PART II: Large graphs

SIZE = 100
objects = [object() for x in xrange(SIZE)]

def _generate_fully_connected_graph1(flag):
   g = graph.Graph(flag)
   cost = 0
   for obj1 in objects:
      for obj2 in objects:
         g.add_edge(obj1, obj2, cost)
         cost += 1
   return g

def _generate_fully_connected_graph2(flag):
   g = graph.Graph(flag)
   for obj in objects:
      g.add_node(obj)
   cost = 0
   for obj1 in objects:
      for obj2 in objects:
         g.add_edge(obj1, obj2, cost)
         cost += 1
   return g

def _generate_mostly_connected_graph1(flag):
   g = graph.Graph(flag)
   cost = 0
   for obj1 in objects:
      for obj2 in objects:
         if cost % 17 == 0:
            g.add_edge(obj1, obj2, cost)
         cost += 1
   return g

def _generate_mostly_connected_graph2(flag):
   g = graph.Graph(flag)
   for obj in objects:
      g.add_node(obj)
   cost = 0
   for obj1 in objects:
      for obj2 in objects:
         if cost % 17 == 0:
            g.add_edge(obj1, obj2, cost)
         cost += 1
   return g
   
def _test_basic(g):
   first_node = g.get_nodes().next()
   assert len(list(g.get_nodes())) == g.nnodes
   assert len(list(g.get_edges())) == g.nedges

# TODO: Broken
## def _test_remove_nodes(g):
##    for node in list(g.get_nodes()):
##       g.remove_node(node)
##    assert g.nnodes == 0
##    assert g.nedges == 0

def _test_remove_node_and_edges(g):
   for node in list(g.get_nodes()):
      g.remove_node_and_edges(node)
   assert g.nnodes == 0
   assert g.nedges == 0
   
def _test_remove_edges(g):
   nodes = g.nnodes
   for edge in list(g.get_edges()):
      g.remove_edge(edge)
   assert g.nedges == 0
   assert g.nnodes == nodes

def _test_others(g):
   # It's really hard to verify the results, so we
   # resort to just running the code
   first_node = g.get_nodes().next()
   g.djikstra_shortest_path(first_node)
   g.djikstra_all_pairs_shortest_path()
   ## TODO: Broken
##    g.all_pairs_shortest_path()
##    sys.stderr.write("D")
   g.create_spanning_tree(first_node)
   g.create_minimum_spanning_tree()

for flag in (graph.FREE, graph.UNDIRECTED, graph.TREE, graph.FLAG_DAG)[1:2]:
   for constructor in [_generate_fully_connected_graph1,
                       _generate_fully_connected_graph2,
                       _generate_mostly_connected_graph1,
                       _generate_mostly_connected_graph2][2:]:
      for _tester in [_test_basic, # _test_remove_nodes,
                      _test_remove_node_and_edges,
                      _test_remove_edges, _test_others,
                      _test_convert1, _test_convert2,
                      _test_convert3, _test_convert4,
                      _test_convert5]:
         setattr(TestGraph, 'test_%d_%s_%s' %
                 (flag, constructor.__name__, _tester.__name__),
                 _make_test(constructor, _tester, flag))
