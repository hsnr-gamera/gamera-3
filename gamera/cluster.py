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

def label(graph, node, label):
   for node in graph.DFS(node):
      node().classify_automatic(str(label))
			
def make_subtrees_stddev(graph, ratio):
   import stats
   cur_label = 0
   remove = []
   i = 0
   for edge in graph.get_edges():
      lengths = []
      path = { }
      #print node().get_main_id(), edge.cost
      get_lengths(edge.to_node, 3, lengths, 0, path)
      lengths.remove(edge.cost)
      #print lengths
      if not (len(lengths) > 1):
         continue
      mean = stats.mean(lengths)
      stdev2 = stats.samplestdev([mean, edge.cost])
      if stdev2 > ratio:
         remove.append(edge)
   for edge in remove:
      graph.remove_edge(edge)
   cur_label = 0
   for node in graph.get_nodes():
      node().classify_manual("")
   for node in graph.get_nodes():
      if node().get_main_id() == "":
         label(graph, node, cur_label)
         cur_label += 1
   nodes = []
   for node in graph.get_nodes():
      nodes.append(node())
   return nodes
	

def make_spanning_tree(glyphs):
   #glyphs = gamera_xml.glyphs_from_xml("C:\Documents and Settings\Karl MacMillan\Desktop\small.xml")
   k = knn.kNN()
   print "Getting distances"
   uniq_dists = k.unique_distances(glyphs)
   print "adding edges"
   g = graph.Undirected()
   #for x in uniq_dists:
   #   g.add_edge(x[1], x[2], x[0])
   #del uniq_dists
   print "creating spanning tree"
   g.create_minimum_spanning_tree(glyphs, uniq_dists)
   return g

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




   
