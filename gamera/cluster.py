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
   visited_edges = { }
   remove = []
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

def go(glyphs):
   g = make_spanning_tree(glyphs)


