from gamera import core, knn, graph, gamera_xml
core.init_gamera()

def get_lengths(node, depth, lengths, cur_depth=0, path = {}):
   if cur_depth >= depth:
	   return
   for edge in node.edges:
      if path.has_key(edge):
         continue
      path[edge] = None
      lengths.append(edge.cost)
      get_lengths(edge.traverse(node), depth, lengths, cur_depth + 1, path)

def label(graph, node, label_start, label):
   for node in graph.DFS(node):
      node().classify_automatic(label_start + str(label))
			
def make_subtrees_stddev(graph, ratio, distance, relabel=1, lab="cluster."):
   import stats
   cur_label = 0
   remove = []
   i = 0
   for edge in graph.get_edges():
      lengths = []
      path = { }
      #print node().get_main_id(), edge.cost
      get_lengths(edge.from_node, distance, lengths, 0, path)
      lengths.remove(edge.cost)
      #print lengths
      if not (len(lengths) > 1):
         continue
      mean = stats.mean(lengths)
      stdev2 = stats.samplestdev([mean, edge.cost])
      if stdev2 > ratio:
         #graph.remove_edge(edge)
         remove.append(edge)
   for edge in remove:
      graph.remove_edge(edge)
   if relabel:
      cur_label = 0
      for node in graph.get_nodes():
         node().classify_manual("")
      for node in graph.get_nodes():
         if node().get_main_id() == "":
            label(graph, node, lab, cur_label)
            cur_label += 1
   nodes = []
   for node in graph.get_nodes():
      nodes.append(node())
   return nodes
	

def make_spanning_tree(glyphs, k=None):
   if k is None:
      k = knn.kNN()
   uniq_dists = k.distance_matrix(glyphs)
   g = graph.Undirected()
   g.create_minimum_spanning_tree(glyphs, uniq_dists)
   return g

def cluster(glyphs, ratio=1.0, distance=2, label="cluster.", k=None):
   g = make_spanning_tree(glyphs, k)
   return make_subtrees_stddev(g, ratio, distance, lab=label)

def cluster2(glyphs):
   ko = knn.kNN()
   gc = knn.glyphs_by_category(cluster(glyphs, k=ko))
   small = []
   large = []
   for x in gc.itervalues():
      if len(x) < 10:
         small.extend(x)
      else:
         large.append(x)
   print len(small)
   output = cluster(small, 1, 1, label="cluster_small.", k=ko)
   cur_label = 0
   for x in large:
      l = x[0].get_main_id() + ".cluster_large" + str(cur_label) + "."
      cur_label += 1
      c = cluster(x, .6, 4, label=l, k=ko)
      output.extend(c)
   return output

def do_tests(filename):
   from gamera import gamera_xml
   from gamera import core
   import os.path

   image = core.load_image(filename)
   glyphs = image.cc_analysis()
   g = make_spanning_tree(glyphs)
   g_orig = g.copy()
   c = make_subtrees_stddev(g, 1.5)
   gamera_xml.glyphs_to_xml(c, os.path.abspath(os.path.dirname(filename) + "cluster_1_5_" + os.path.basename(filename)))
   g = g_orig.copy()
   c = make_subtrees_stddev(g, 2.0)
   gamera_xml.glyphs_to_xml(c, os.path.abspath(os.path.dirname(filename) + "cluster_2_0_" + os.path.basename(filename)))
   g = g_orig.copy()
   c = make_subtrees_stddev(g, 2.5)
   gamera_xml.glyphs_to_xml(c, os.path.abspath(os.path.dirname(filename) + "cluster_2_5_" + os.path.basename(filename)))
   g = g_orig.copy()
   c = make_subtrees_stddev(g, 3.0)
   gamera_xml.glyphs_to_xml(c, os.path.abspath(os.path.dirname(filename) + "cluster_3_0_" + os.path.basename(filename)))

def make_unique_names(glyphs):
   for i in range(len(glyphs)):
      glyphs[i].classify_automatic(glyphs[i].get_main_id() + str(i))

def graphvis_output(G, filename):
   fd = open(filename, 'w')
   if G.is_directed():
      fd.write("digraph G {\n")
      for node in G.get_nodes():
         for edge in node.out_edges:
            fd.write('   %s -> %s [ label = %f ];\n' % (node(), edge.to_node(), edge()))
      fd.write("}\n")
   else:
      fd.write("graph G {\n")
      for edge in G.get_edges():
         fd.write('   "%s" -- "%s" [label="%.2f",len="%f"];\n'
                  % (edge.from_node().get_main_id(), edge.to_node().get_main_id(),
                     edge.cost, edge.cost / 4))
      fd.write("}\n")
   fd.close()


def analysis(glyphs):
   by_id = {}
   for x in glyphs:
      id = x.get_main_id()
      if not by_id.has_key(id):
         by_id[id] = []
      by_id[id].append(x)
   num_features = len(x.features)

   for x in by_id:
      for i in range(len(x)):
         sum_vec
   
