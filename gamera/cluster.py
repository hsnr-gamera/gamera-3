from gamera import knn
from gamera.plugins import distance
from gamera import util

class Edge:
    def __init__(self, node1, node2, cost):
        self.cost = cost
        self.node1 = node1
        self.node2 = node2
        self.visited = node1.visited

    def get_node(self, node):
        if node == self.node1:
            return self.node2
        else:
            return self.node1

    def __repr__(self):
        return "{ %s, %s }" % \
               (self.node1.cluster_name, self.node2.cluster_name)


class Graph(dict):
    def __init__(self):
        dict.__init__(self)
        self.edges = { }
        self.disj_set = []
        
    def add_node(self, node):
        if self.has_key(node):
            return
        node.edges = []
        self[node] = None
        node.set_id = len(self.disj_set)
        self.disj_set.append(0)
        
    def find(self, set_id):
        if (self.disj_set[set_id] <= 0):
            return set_id
        else:
            self.disj_set[set_id] = self.find(self.disj_set[set_id])
            return self.disj_set[set_id]
    
    def union(self, a, b):
        if self.disj_set[a] < self.disj_set[b]:
            self.disj_set[a] = b
        else:
            if self.disj_set[a] == self.disj_set[b]:
                self.disj_set[a] -= 1
            self.disj_set[b] = a

    def add_edge(self, node1, node2, cost):
        a = self.find(node1.set_id)
        b = self.find(node2.set_id)
        if a == b:
            return 1
        self.union(a, b)
        edge = Edge(node1, node2, cost)
        node1.edges.append(edge)
        node2.edges.append(edge)
        self.edges[edge] = None
        return 0

    def remove_edge(self, node1, node2):
        for edge in node1.edges:
            if (edge.node1 == node1 and edge.node2 == node2) \
               or (edge.node2 == node1 and edge.node1 == node2):
                node1.edges.remove(edge)
                node2.edges.remove(edge)
                del self.edges[edge]
                return

def do_detect_cycle(node):
    if node.visited:
        return 1
    node.visited = 1
    for edge in node.edges:
        if not edge.visited:
            edge.visited = 1
            if do_detect_cycle(edge.get_node(node)):
                return 1
    return 0

def detect_cycle(graph, node):
    for x in graph:
        x.visited = 0
    for x in graph.edges:
        x.visited = 0
    if do_detect_cycle(node):
        return 1
    return 0

def get_remove(graph, node, max_cost, path, remove):
    for edge in node.edges:
        if not path.has_key(edge):
            path[edge] = None
            if edge.cost > max_cost:
                remove.append(edge)
            get_remove(graph, edge.get_node(node), max_cost, path, remove)

def label(node, l, path):
    node.cluster_label = l
    for edge in node.edges:
        if not path.has_key(edge):
            path[edge] = None
            label(edge.get_node(node), l, path)

def make_subtrees(graph, max_cost):
    cur_label = 0
    remove = []
    for x in graph:
        x.cluster_label = 0
    node = graph.keys()[0]
    get_remove(graph, node, max_cost, { }, remove)
    for x in remove:
        graph.remove_edge(x.node1, x.node2)
        label(x.node1, cur_label, {})
        cur_label += 1
        label(x.node2, cur_label, {})
        cur_label += 1
    subs = []
    for i in range(cur_label):
        subs.append([])
        for x in graph:
            if x.cluster_label == i:
                subs[i].append(x)
    subs = [x for x in subs
            if x != []]
    return subs

def do_visit_connected(node, path):
    node.visited = 1
    for edge in node.edges:
        if not path.has_key(edge):
            path[edge] = None
            do_visit_connected(edge.get_node(node), path)
    
def check_connected(graph):
    for x in graph:
        x.visited = 0
    do_visit_connected(graph.keys()[0], {})
    for x in graph:
        if x.visited != 1:
            print "error: not connected!"
            #print graph.disj_set
            #for x in graph:
                #print x.cluster_name, x.edges
            return

def create_forest(glyphs):
    forest = []
    num_nodes = len(glyphs)
    dists = distance.distance_matrix(glyphs)
    for i in range(len(glyphs)):
        for j in range(i + 1, len(glyphs)):
            forest.append([dists.get(i, j), glyphs[i], glyphs[j]])
    del dists
    forest.sort()
    return forest

def cluster(forest, glyphs):
    num_nodes = len(glyphs)
    next = 1
    g = Graph()
    for i in range(len(glyphs)):
        glyphs[i].visited = 0
        g.add_node(glyphs[i])
        glyphs[i].cluster_name = str(i)
    g.add_edge(forest[0][1], forest[0][2], forest[0][0])
    #print forest[0][1].cluster_name, forest[0][2].cluster_name 
    progress = util.ProgressFactory("Creating graph . . . ", num_nodes - 1)
    try:
        for i in range(num_nodes - 2):
            while(1):
                if g.add_edge(forest[next][1], forest[next][2], forest[next][0]):
                    next += 1
                else:
                    next += 1
                    break
            progress.step()
        #print detect_cycle(g, g.keys()[0])
        #check_connected(g)
        total_cost = 0.0
        counted_edges = { }
    finally:
        progress.kill()
    progress = util.ProgressFactory("Getting avg dist . . . ", len(g))
    for node in g:
        for edge in node.edges:
            if not counted_edges.has_key(edge):
                counted_edges[edge] = None
                total_cost += edge.cost
        progress.step()

    avg_cost = total_cost / len(counted_edges)
    print avg_cost
    s = make_subtrees(g, avg_cost * 2)
    return g, s
    
