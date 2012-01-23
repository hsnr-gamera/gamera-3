/*
 *
 * Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
 *               2011      Christian Brandt
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "graph.hpp"
#include "graphobject.hpp"
#include "edgeobject.hpp"
#include "nodeobject.hpp"
#include "graphobject_algorithm.hpp"
#include "iteratorobject.hpp"

extern "C" {
   static PyObject* graph_new(PyTypeObject* pytype, PyObject* args,
           PyObject* kwds);
   static void graph_dealloc(PyObject* self);
   static PyObject* graph_copy(PyObject* self, PyObject* args);
   static PyObject* graph_add_node(PyObject* self, PyObject* pyobject);
   static PyObject* graph_add_nodes(PyObject* self, PyObject* pyobject);
   static PyObject* graph_remove_node_and_edges(PyObject* self, PyObject* pyobject);
   static PyObject* graph_remove_node(PyObject* self, PyObject* pyobject);
   static PyObject* graph_add_edge(PyObject* self, PyObject* args);
   static PyObject* graph_add_edges(PyObject* self, PyObject* args);
   static PyObject* graph_remove_edge(PyObject* self, PyObject* args);
   static PyObject* graph_remove_all_edges(PyObject* self, PyObject* args);
   static PyObject* graph_is_directed(PyObject* self, PyObject* _);
   static PyObject* graph_is_undirected(PyObject* self, PyObject* _);
   static PyObject* graph_make_directed(PyObject* self, PyObject* _);
   static PyObject* graph_make_undirected(PyObject* self, PyObject* _);
   static PyObject* graph_is_cyclic(PyObject* self, PyObject* _);
   static PyObject* graph_is_acyclic(PyObject* self, PyObject* _);
   static PyObject* graph_make_cyclic(PyObject* self, PyObject* _);
   static PyObject* graph_make_acyclic(PyObject* self, PyObject* _);
   static PyObject* graph_is_tree(PyObject* self, PyObject* _);
   static PyObject* graph_is_blob(PyObject* self, PyObject* _);
   static PyObject* graph_make_blob(PyObject* self, PyObject* _);
   static PyObject* graph_make_tree(PyObject* self, PyObject* _);
   static PyObject* graph_is_multi_connected(PyObject* self, PyObject* _);
   static PyObject* graph_is_singly_connected(PyObject* self, PyObject* _);
   static PyObject* graph_make_multi_connected(PyObject* self, PyObject* _);
   static PyObject* graph_make_singly_connected(PyObject* self, PyObject* _);
   static PyObject* graph_is_self_connected(PyObject* self, PyObject* _);
   static PyObject* graph_make_self_connected(PyObject* self, PyObject* _);
   static PyObject* graph_make_not_self_connected(PyObject* self, PyObject* _);
   static PyObject* graph_get_node(PyObject* self, PyObject* args);
   static PyObject* graph_get_nodes(PyObject* self, PyObject* _);
   static PyObject* graph_has_node(PyObject* self, PyObject* node);
   static PyObject* graph_get_nnodes(PyObject* self, PyObject* _);
   static PyObject* graph_get_edges(PyObject* self, PyObject* _);
   static PyObject* graph_has_edge(PyObject* self, PyObject* args);
   static PyObject* graph_get_nedges(PyObject* self, PyObject* _);
   static PyObject* graph_get_subgraph_roots(PyObject* self, PyObject* args);
   static PyObject* graph_get_nsubgraphs(PyObject* self, PyObject* _);
   static PyObject* graph_size_of_subgraph(PyObject* self, PyObject* pyobject);
   static PyObject* graph_is_fully_connected(PyObject* self, PyObject* _);
   static PyObject* graph_has_flag(PyObject* self, PyObject* pyobject);
   static PyObject* graph_has_path(PyObject* self, PyObject* args);
}



// -----------------------------------------------------------------------------
// Python Type Definition
// -----------------------------------------------------------------------------
static PyTypeObject GraphType = {
  PyObject_HEAD_INIT(NULL)
  0,
};



// -----------------------------------------------------------------------------
PyMethodDef graph_methods[] = {
  { CHAR_PTR_CAST "copy", graph_copy, METH_VARARGS,
    CHAR_PTR_CAST "**copy** (*flags* = ``FREE``)\n\n" \
    "Copies a graph (optionally specifying new flags for the new graph).\n\n" \
    "In some cases, copying the graph to a new graph type may be faster\n" \
    "than using one of the in-place conversion functions.\n\n" \
    "See `Graph constructor`_ for a definition of *flags*.\n\n"
  },
  { CHAR_PTR_CAST "add_node", graph_add_node, METH_O,
    CHAR_PTR_CAST "**add_node** (*value*)\n\n" \
    "Add a node identified by the given *value*. " \
    "The newly-created node has no edges.\n\n" \
    "Returns ``1`` if a new node was created.\n" \
    "Returns ``0`` if a node already exists with the associated *value*.\n\n" \
    "**Complexity**: Nodes are added in logarithmic time.\n\n"
  },
  { CHAR_PTR_CAST "add_nodes", graph_add_nodes, METH_O,
    CHAR_PTR_CAST "**add_nodes** (*list_of_values*)\n\n" \
    "Add nodes identified by each value in a list. " \
    "The newly-created nodes have no edges.\n\n" \
    "Returns the number of new nodes that were created.\n\n" \
    "**Complexity**: `add_nodes` is moderately faster than multiple calls to " \
    "add_node_. Nodes are added in logarithmic time\n\n"
  },
  { CHAR_PTR_CAST "remove_node_and_edges", graph_remove_node_and_edges, METH_O,
    CHAR_PTR_CAST "**remove_node_and_edges** (*value*)\n\n" \
    "Remove the node identifed by *value* from the graph, and remove all " \
    "edges pointing inward or outward from that node.\n\n" \
    "For instance, given the graph::\n\n" \
    "  a -> b -> c\n\n" \
    "``.remove_node_and_edges('b')`` will result in::\n\n" \
    "  a         c\n\n" \
    "**Complexity**: Removing a node takes *O* (*n* + *e*) where *n* is the " \
    "number of nodes in the graph and *e* is the number of edges attached to " \
    "the given node.\n\n"
  },
  { CHAR_PTR_CAST "remove_node", graph_remove_node, METH_O,
    CHAR_PTR_CAST "**remove_node** (*value*)\n\n" \
    "Remove a node identified by *value* from the graph, stitching together " \
    "the broken edges.\n\n" \
    "For instance, given the graph::\n\n" \
    "  a -> b -> c\n\n" \
    "``.remove_node('b')`` will result in::\n\n" \
    "  a -> c\n\n" \
    "**Complexity**: Removing a node takes *O* (*n* + *e*) where *n* is the " \
    "number of nodes in the graph and *e* is the number of edges attached to the given node.\n\n"
  },
  { CHAR_PTR_CAST "add_edge", graph_add_edge, METH_VARARGS,
    CHAR_PTR_CAST "**add_edge** (*from_value*, *to_value*, *cost* = 1.0, *label* = None)\n\n" \
    "Add an edge between the two nodes identified by *from_value* and *to_value*.\n\n" \
    "The return value is the number of edges created. If the graph has set " \
    "the flag ``CHECK_ON_INSERT`` and the edge violates any of the " \
    "restrictions specified the edge by the flags to the graph's constructor " \
    "will not be created. If the graph is ``DIRECTED``, the edge goes from " \
    "*from_value* to *to_value*. If a node representing one of the values is " \
    "not present, a node wille be implicitly created. \n\n" \
    "Optionally, a *cost* and *label* can be associated with the edge. These " \
    "values are used by some higher-level graph algorithms such as " \
    "create_minimum_spanning_tree_ or shortest_path_.\n\n" \
    "**Complexity**: Edges are added in *O* ( ln *n* ) because of getting " \
    "the nodes which are associated to *from_value* or *to_value*.\n\n"
  },
  { "add_edges", graph_add_edges, METH_O,
    "**add_edges** (*list_of_tuples*)\n\n" \
    "Add edges specified by a list of tuples of the form:\n\n" \
    "   (*from_value*, *to_value*, [*cost*[, *label*]]).\n\n" \
    "The members of this tuple correspond to the arguments to add_edge_.\n\n" \
    "The return value is the number of edges created. For more information " \
    "on adding edges see add_edge_  If an edge violates any of the " \
    "restrictions specified\n\n" \
    "**Complexity:** ``add_edges`` is moderately faster than multiple calls " \
    "to add_edge_.\n\n" \
  },
  { CHAR_PTR_CAST "remove_edge", graph_remove_edge, METH_VARARGS,
    CHAR_PTR_CAST "**remove_edge** (*from_value*, *to_value*)\n\n" \
    "Remove an edge between two nodes identified by *from_value* and *to_value*.\n\n" \
    "If the edge does not exist in the graph, a ``RuntimeError`` exception is raised.\n\n" \
    "When the graph is ``DIRECTED``, only the edge going from *from_value* to *to_value* is removed.\n\n" \
    "If the graph is ``MULTI_CONNECTED``, **all** edges from *from_value* to *to_value* are removed.\n\n" \
    "**Complexity**: Edges can be removed in *O*(*e*) time where *e* is the number of edges in the graph.\n\n"
  },
  { CHAR_PTR_CAST "remove_all_edges", graph_remove_all_edges, METH_NOARGS,
    CHAR_PTR_CAST "**remove_all_edges** ()\n\n" \
    "Remove all the edges in the graph, leaving all nodes as islands.\n\n" \
    "**Complexity**: ``remove_all_edges`` takes *O* ( *n* + *e*) time where *n* is the number of nodes in the graph and *e* is the number of edges in the graph."
  },
  { CHAR_PTR_CAST "is_directed", graph_is_directed, METH_NOARGS,
    CHAR_PTR_CAST "**is_directed** ()\n\n" \
    "Return ``True`` if the graph is defined as directed."
  },
  { CHAR_PTR_CAST "is_undirected", graph_is_undirected, METH_NOARGS,
    CHAR_PTR_CAST "**is_undirected** ()\n\n" \
    "Return ``True`` if the graph is defined as undirected."
  },
  { CHAR_PTR_CAST "make_directed", graph_make_directed, METH_NOARGS,
    CHAR_PTR_CAST "**make_directed** ()\n\n" \
    "If the graph is undirected, converts it into an directed graph by adding a complementary edge for\n" \
    "each existing edge.\n" \
    "**Complexity**: The graph can be converted in *O* ( *e* ) time.\n\n"
  },
  { CHAR_PTR_CAST "make_undirected", graph_make_undirected, METH_NOARGS,
    CHAR_PTR_CAST "**make_undirected** ()\n\n" \
    "If the graph is directed, converts it into an undirected graph. Each edge in the existing graph\n" \
    "will become a non-directional edge in the resulting graph."
  },
  { CHAR_PTR_CAST "is_cyclic", graph_is_cyclic, METH_NOARGS,
    CHAR_PTR_CAST "**is_cyclic** ()\n\n" \
    "Returns ``True`` if the graph has any cycles. Note that this is independent \n"\
    "from the flag ``CYCLIC``.\n\n" 
  },
  { CHAR_PTR_CAST "is_acyclic", graph_is_acyclic, METH_NOARGS,
    CHAR_PTR_CAST "**is_acyclic** ()\n\n" \
    "Returns ``True`` if the graph does not have any cycles."
  },
  { CHAR_PTR_CAST "make_cyclic", graph_make_cyclic, METH_NOARGS,
    CHAR_PTR_CAST "**make_cyclic** ()\n\n" \
    "Allow the graph to include cycles from this point on.  This does nothing except set the ``CYCLIC`` flag."
  },
  { CHAR_PTR_CAST "make_acyclic", graph_make_acyclic, METH_NOARGS,
    CHAR_PTR_CAST "**make_acyclic** ()\n\n" \
    "Remove any cycles (using a depth-first search technique) and disallow cycles from this point on.\n\n" \
    "This may not be the most appropriate cycle-removing technique for all applications.\n\n" \
    "See create_spanning_tree_ for other ways to do this.\n\n"
  },
  { CHAR_PTR_CAST "is_tree", graph_is_tree, METH_NOARGS,
    CHAR_PTR_CAST "**is_tree** ()\n\n" \
    "Returns ``True`` if the graph conforms to the restrictions of a tree."
  },
  { CHAR_PTR_CAST "is_blob", graph_is_blob, METH_NOARGS,
    CHAR_PTR_CAST "**is_blob** ()\n\n" \
    "Returns ``True`` if the graph does not conform to the restricitions of a tree."
  },
  { CHAR_PTR_CAST "make_tree", graph_make_tree, METH_NOARGS,
    CHAR_PTR_CAST "**make_tree** ()\n\n" \
    "Turns the graph into a tree by calling make_acyclic_ followed by make_undirected_.  Sets the ``BLOB`` flag to ``False``.\n\n" \
    "This approach may not be reasonable for all applications.  For other ways to convert blobs to trees, see `spanning trees`_.\n\n"
  },
  { CHAR_PTR_CAST "make_blob", graph_make_blob, METH_NOARGS,
    CHAR_PTR_CAST "**make_blob** ()\n\n" \
    "Make the graph into a blob (the opposite of a tree).  This does nothing \n"
    "except set the ``BLOB`` flag.\n" 
  },
  { CHAR_PTR_CAST "is_multi_connected", graph_is_multi_connected, METH_NOARGS,
    CHAR_PTR_CAST "**is_multi_connected** ()\n\n" \
    "Returns ``True`` if the graph is multi-connected (multiple edges between a single pair of nodes)."
  },
  { CHAR_PTR_CAST "is_singly_connected", graph_is_singly_connected, METH_NOARGS,
    CHAR_PTR_CAST "**is_singly_connected** ()\n\n" \
    "Returns ``True`` if the graph does not have multiple edges between a single pair of nodes."
  },
  { CHAR_PTR_CAST "make_multi_connected", graph_make_multi_connected, METH_NOARGS,
    CHAR_PTR_CAST "**make_multi_connected** ()\n\n" \
    "Allow the graph to be multi-connected from this point on.  This does \n"
    "nothing except set the ``MULTI_CONNECTED`` flag."
  },
  { CHAR_PTR_CAST "make_singly_connected", graph_make_singly_connected, METH_NOARGS,
    CHAR_PTR_CAST "**make_singly_connected** ()\n\n" \
    "For each pair of nodes, leave only one remaining edge in either direction.\n" \
    "Restrict the graph to being singly-connected from this point on."
  },
  { CHAR_PTR_CAST "is_self_connected", graph_is_self_connected, METH_NOARGS,
    CHAR_PTR_CAST "**is_self_connected** ()\n\n" \
    "Returns ``True`` if the graph is self-connected. When ``True`` the graph\n"
    "has edges pointfrom from one node to that same node.\n"
  },
  { CHAR_PTR_CAST "make_self_connected", graph_make_self_connected, METH_NOARGS,
    CHAR_PTR_CAST "**make_self_connected** ()\n\n" \
    "Allow the graph to be self-conncted from this point on.  This does nothing except set the ``SELF_CONNECTED`` flag.\n"
  },
  { CHAR_PTR_CAST "make_not_self_connected", graph_make_not_self_connected, METH_NOARGS,
    CHAR_PTR_CAST "**make_not_self_connected** ()\n\n" \
    "Remove all self-connections and restrict the graph to have no self-connections from this point on."
  },
  { CHAR_PTR_CAST "get_node", graph_get_node, METH_O,
    CHAR_PTR_CAST "**get_node** (*value*)\n\n" \
    "Returns the ``Node`` object identified by the given *value*.\n\n"
    "Raises a ``ValueError`` exception if there is no node associated with the given *value*.\n\n"
    "**Complexity**: Searching for a node takes *O* ( ln *n* ) time where \n"\
    "*n* is the number of nodes in the graph.\n\n"
  },
  { CHAR_PTR_CAST "get_nodes", graph_get_nodes, METH_NOARGS,
    CHAR_PTR_CAST "**get_nodes** ()\n\n" \
    "Returns a lazy iterator over all nodes in the graph.  The ordering of the nodes is undefined.\n" \
  },
  { CHAR_PTR_CAST "get_subgraph_roots", graph_get_subgraph_roots, METH_NOARGS,
    CHAR_PTR_CAST "**get_subgraph_roots** ()\n\n" \
    "Returns a lazy iterator over each of the subgraph roots.  Performing a breadth-first or depth-first search\n" \
    "from each of this notes will visit every node in the graph. Currently this algorithm is proofed for undirected graphs only but should also work for directed graphs.\n\n"
  },
  { CHAR_PTR_CAST "has_node", graph_has_node, METH_O,
    CHAR_PTR_CAST "**has_node** (*value*)\n\n" \
    "Returns ``True`` if graph has a node identified by *value*.\n\n"\
    "**Comlexity**: Searching for an node takes *O* ( ln *n* ) time where \n"\
    "*n* is the number of nodes in the graph.\n\n"
  },
  { CHAR_PTR_CAST "get_edges", graph_get_edges, METH_NOARGS,
    CHAR_PTR_CAST "**get_edges** ()\n\n" \
    "Returns an iterator over all edges in the graph.  The ordering of the edges is undefined.\n\n"
  },
  { CHAR_PTR_CAST "has_edge", graph_has_edge, METH_VARARGS,
    CHAR_PTR_CAST "**has_edge** (*from_value*, *to_value*)\n\n" \
    "  *or*\n\n**has_edge** (*from_node*, *to_node*)\n\n" \
    "  *or*\n\n**has_edge** (*edge*)\n\n" \
    "Returns ``True`` if graph contains the given edge.  The edge can be \n"\
    "specified as either a pair of values identifying nodes,\n" \
    "a pair of ``Node`` objects, or a single ``Edge`` object.\n\n" \
    "**Complexity**: Searching for an edge takes *O* ( *e* + ln *n* ) time \n"\
    "where *e* is the number of edges in the graph and *n* is the number of \n"\
    "nodes in the graph."
  },
  { CHAR_PTR_CAST "size_of_subgraph", graph_size_of_subgraph, METH_O,
    CHAR_PTR_CAST "**size_of_subgraph** (*value*)\n\n  *or*\n\n**size_of_subgraph** (*node*)\n\n" \
    "Returns the size of the subgraph rooted at the given node.  In other words, this returns the\n" \
    "number of nodes reachable from the given node."
  },
  { CHAR_PTR_CAST "is_fully_connected", graph_is_fully_connected, METH_NOARGS,
    CHAR_PTR_CAST "**is_fully_connected** ()\n\n" \
    "Returns ``True`` if there is only one subgraph in the graph. In other words it returns \n" \
    "``True`` if the number of nodes reachable from the first inserted node is equal to the \n" \
    "overall number of nodes. Currently this algorithm is only defined for undirected graphs."
  },
  { CHAR_PTR_CAST "has_flag", graph_has_flag, METH_O,
    CHAR_PTR_CAST "**has_flag** (*flag*)\n\n" \
    "Returns ``True`` when the given flag is set in the graph. *flag* can be a single flag or \n"
    "or-combination of flags which are documented in `Graph constructor`_ ."
  },
  { CHAR_PTR_CAST "has_path", graph_has_path, METH_VARARGS,
    CHAR_PTR_CAST "**has_path** (*from_node*, *to_node*)\n\n" \
    "Returns ``True`` when *to_node* is reachable starting at *start_node*.\n"
    "*start_node* and *to_node* can be a Node object or a node's value.\n\n"
  },
 
  ALGORITHM_METHODS
  { NULL }
};



// -----------------------------------------------------------------------------  
PyGetSetDef graph_getset[] = {
  { CHAR_PTR_CAST "nnodes", (getter)graph_get_nnodes, 0,
    CHAR_PTR_CAST "Number of nodes in the graph", 0 },
  { CHAR_PTR_CAST "nedges", (getter)graph_get_nedges, 0,
    CHAR_PTR_CAST "Number of edges in the graph", 0 },
  { CHAR_PTR_CAST "nsubgraphs", (getter)graph_get_nsubgraphs, 0,
    CHAR_PTR_CAST "Number of edges in the graph", 0 },
  { NULL }
};



// -----------------------------------------------------------------------------  
void init_GraphType(PyObject* d) {
  GraphType.ob_type = &PyType_Type;
  GraphType.tp_name = CHAR_PTR_CAST "gamera.graph.Graph";
  GraphType.tp_basicsize = sizeof(GraphObject);
  GraphType.tp_dealloc = graph_dealloc;
  GraphType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  GraphType.tp_new = graph_new;
  GraphType.tp_getattro = PyObject_GenericGetAttr;
  GraphType.tp_alloc = NULL; // PyType_GenericAlloc;
  GraphType.tp_free = NULL; // _PyObject_Del;
  GraphType.tp_methods = graph_methods;
  GraphType.tp_getset = graph_getset;
  GraphType.tp_weaklistoffset = 0;
  GraphType.tp_doc = CHAR_PTR_CAST
    "**Graph** (*flags* = ``FREE``)\n\n"        \
    "Construct a new graph.\n\n" \
    "The *flags* are used to set certain restrictions on the graph.  When adding an edge\n" \
    "violates one of these restrictions, the edge is not added and ``None`` is returned.  Note\n" \
    "that exceptions are not raised.  The graph type may be changed at any time after creation\n" \
    "using methods such as make_directed_ or make_undirected_, but conversion may take some time.\n\n" \
    "The *flags* may be any combination of the following values (use bitwise-or to combine flags). The values\n" \
    "of these flags are defined in the ``graph`` module.  By default, all flags except of ``CHECK_ON_INSERT`` are ``True``:\n\n" \
    "  - ``DIRECTED``:\n\n" \
    "       When ``True``, the graph will have directed edges.  Nodes will only\n" \
    "       traverse to other nodes in the direction of the edge.\n\n" \
    "  - ``CYCLIC``:\n\n" \
    "       When ``True``, the graph may contain cycles.  When ``False``, edges are\n" \
    "       added to the graph only when they do not create cycles.  (When ``False``, ``MULTI_CONNECTED``" \
    "       and ``SELF_CONNECTED`` are set to ``False``.)\n\n" \
    "  - ``BLOB``:\n\n" \
    "       A \"blob\" is defined as the opposite of a tree.  (When ``False``, ``DIRECTED``\n" \
    "       and ``CYCLIC`` will be set to ``False``).\n\n" \
    "  - ``MULTI_CONNECTED``:\n\n"
    "       When ``True``, the graph may contain multiple edges between a single\n" \
    "       pair of nodes.\n\n" \
    "  - ``SELF_CONNECTED``:\n\n"
    "       When ``True``, the graph may contain edges that start and end at the\n" \
    "       same node.\n\n" \
    "  - ``CHECK_ON_INSERT``:\n\n" \
    "       When ``True``, an edge is only inserted when it conforms all restrictions. \n" \
    "       This is disabled by default because the checks slow down adding edges \n" \
    "       significantly.\n\n" \
    "In addition to these raw flags, there are some convenience values for common combinations of these\n" \
    "flags.\n\n" \
    "  - ``FREE``: Equivalent to all flags being set.  There are no restrictions on the graph morphology.\n\n" \
    "  - ``TREE``: Tree structure (no flags set).\n\n" \
    "  - ``FLAG_DAG``: Directed, acyclic graph.\n\n" \
    "  - ``UNDIRECTED``: Undirected, cyclic graph.\n\n";

  PyType_Ready(&GraphType);
  PyDict_SetItemString(d, "Graph", (PyObject*)&GraphType);
}


GraphObject* graph_new(flag_t flags) {
   GraphObject* so = (GraphObject*)(GraphType.tp_alloc(&GraphType, 0));
   so->assigned_edgeobjects = new EdgeObjectMap;
   so->_graph = new Graph(flags);
   return so;
}

GraphObject* graph_new(Graph* g) {
   GraphObject* so = (GraphObject*)(GraphType.tp_alloc(&GraphType, 0));
   so->_graph = g;
   
   so->assigned_edgeobjects = new EdgeObjectMap();
   return so;
}



GraphObject* graph_copy(GraphObject* so, flag_t flags) {
   Graph* g = new Graph(so->_graph, flags);
   return graph_new(g);
}



// -----------------------------------------------------------------------------  
PyObject* graph_new(PyTypeObject* pytype, PyObject* args,
		    PyObject* kwds) {
   unsigned long flags = FLAG_FREE;
   if (PyArg_ParseTuple(args, CHAR_PTR_CAST "|k:Graph.__init__", &flags) <= 0)
      return NULL;

   return (PyObject*)graph_new(flags);
}



// -----------------------------------------------------------------------------  
bool is_GraphObject(PyObject* self) {
  return PyObject_TypeCheck(self, &GraphType);
}



// -----------------------------------------------------------------------------  
void graph_dealloc(PyObject* self) {
   if(is_GraphObject(self)) {
      INIT_SELF_GRAPH();
      if(so->_graph) {
      //invalidate nodeeobjects
         NodePtrIterator* it = so->_graph->get_nodes();
         Node *n;
         while((n = it->next()) != NULL) {
            if(n->_value == NULL)
               continue;

            GraphDataPyObject* d = dynamic_cast<GraphDataPyObject*>(n->_value);
            if(d == NULL)
               throw std::runtime_error("somehting went wrong in dealloc");

            if(d->_node != NULL) {
               ((NodeObject*)d->_node)->_graph = NULL;
               ((NodeObject*)d->_node)->_node = NULL;
               d->_node = NULL;
            }


            delete d;
         }
         delete it;
      
         delete so->_graph;
         so->_graph = NULL;
      }
#ifdef __DEBUG_GAPI__
      std::cerr << "assigned edgeobjects" << so->assigned_edgeobjects->size() << std::endl;
#endif
      delete so->assigned_edgeobjects;
      so->assigned_edgeobjects = NULL;
      self->ob_type->tp_free(self);
   }
}



// -----------------------------------------------------------------------------  
PyObject* graph_copy(PyObject* self, PyObject* args) {
   INIT_SELF_GRAPH();
   unsigned long flags = FLAG_FREE;
   if (PyArg_ParseTuple(args, CHAR_PTR_CAST "|k:Graph.copy", &flags) <= 0)
      return NULL;

   return (PyObject*)graph_copy(so, flags);
}



// -----------------------------------------------------------------------------  
/* Python wrapper methods                                                    */
// -----------------------------------------------------------------------------  
PyObject* graph_add_node(PyObject* self, PyObject* pyobject) {
   INIT_SELF_GRAPH();
   GraphDataPyObject* data = new GraphDataPyObject(pyobject);
   if(so->_graph->add_node(data)) {
#ifdef __DEBUG_GAPI__
      std::cerr << "Node added" << std::endl;
#endif
      RETURN_INT(1);
   }

#ifdef __DEBUG_GAPI__
   std::cerr << "Node not added" << std::endl;
#endif
   delete data;
   RETURN_INT(0);
 
}



// -----------------------------------------------------------------------------  
PyObject* graph_add_nodes(PyObject* self, PyObject* pyobject) {
   PyObject* seq = PySequence_Fast(pyobject, "Argument must be an iterable of nodes");
   if (seq == NULL)
      return 0;
   size_t list_size = PySequence_Fast_GET_SIZE(seq);
   size_t result = 0;
   for (size_t i = 0; i < list_size; ++i)
      if (graph_add_node(self, PySequence_Fast_GET_ITEM(seq, i)))
         result++;
   Py_DECREF(seq);

   RETURN_INT(result);
}



// -----------------------------------------------------------------------------  
PyObject* graph_remove_node_and_edges(PyObject* self, PyObject* a) {
   INIT_SELF_GRAPH();
   if(is_NodeObject(a)) {
      NodeObject* no = (NodeObject*)a;
      so->_graph->remove_node_and_edges(no->_node);
      no->_node = NULL;
      no->_graph = NULL;
   }
   else {
      GraphDataPyObject data(a);
      GraphDataPyObject* d = NULL;
      //invalidate delivered nodes
      Node* n = so->_graph->get_node(&data);
      if(n != NULL) {
         d = dynamic_cast<GraphDataPyObject*>(n->_value);
         NodeObject* no = (NodeObject*)d->_node;
         if(no != NULL) {
            no->_node = NULL;
            no->_graph = NULL;
         }
      }
      so->_graph->remove_node_and_edges(&data);
      delete d;
   }
   RETURN_VOID();
}



// -----------------------------------------------------------------------------  
PyObject* graph_remove_node(PyObject* self, PyObject* a) {
   INIT_SELF_GRAPH();
   try {
      if(is_NodeObject(a)) {
         NodeObject* no = (NodeObject*)a;
         so->_graph->remove_node(no->_node);
         no->_node = NULL;
         no->_graph = NULL;
      }
      else {
         GraphDataPyObject data(a);
         GraphDataPyObject* d = NULL;
         //invalidate delivered nodes
         Node* n = so->_graph->get_node(&data);
         if(n == NULL)
            throw std::runtime_error("node not found");

         d = dynamic_cast<GraphDataPyObject*>(n->_value);
         NodeObject* no = (NodeObject*)d->_node;
         if(no != NULL) {
            no->_node = NULL;
            no->_graph = NULL;
         }
         so->_graph->remove_node(n);
         delete d;
      }
   }
   catch (std::runtime_error e) {
      PyErr_SetString(PyExc_ValueError, e.what());
      return NULL; 
   }
   RETURN_VOID();
}



// -----------------------------------------------------------------------------  
PyObject* graph_add_edge(PyObject* self, PyObject* args) {
   INIT_SELF_GRAPH();
   int res = 0;
   PyObject* from_pyobject, *to_pyobject;
   cost_t cost = 1.0;
   PyObject* label = NULL;
   if(PyArg_ParseTuple(args, CHAR_PTR_CAST "OO|dO:add_edge", 
            &from_pyobject, &to_pyobject, &cost, &label) <= 0 )
      return NULL;

   if(is_NodeObject(from_pyobject) && is_NodeObject(to_pyobject)) {
      Node* from_node = ((NodeObject*)from_pyobject)->_node;
      Node* to_node = ((NodeObject*)to_pyobject)->_node;
      res = (so->_graph->add_edge(from_node, to_node, cost, so->_graph->is_directed(), label));
   }
   else {
      GraphDataPyObject *from = new GraphDataPyObject(from_pyobject);
      GraphDataPyObject *to = new GraphDataPyObject(to_pyobject);
      bool del_from = true, del_to = true;

      if(!so->_graph->has_node(from)) {
         del_from = false;
         so->_graph->add_node(from);
      }
      if(!so->_graph->has_node(to)) {
         so->_graph->add_node(to);
         del_to = false;
      }
#ifdef __DEBUG_GAPI__ 
      std::cerr << from << to << std::endl;
#endif
      if(label != NULL)
         Py_INCREF(label);

      res = so->_graph->add_edge(from, to, cost, so->_graph->is_directed(), label);

      if(del_from)
         delete from;
      if(del_to)
         delete to;
   }
   
   RETURN_INT(res);
}



// -----------------------------------------------------------------------------  
PyObject* graph_add_edges(PyObject* self, PyObject* args) {
   PyObject* seq = PySequence_Fast(args, "Argument must be an iterable of edges");
   if (seq == NULL)
      return 0;
   size_t list_size = PySequence_Fast_GET_SIZE(seq);
   size_t result = 0;
   for (size_t i = 0; i < list_size; ++i)
      result += PyInt_AsUnsignedLongMask(graph_add_edge(self, PySequence_Fast_GET_ITEM(seq, i)));
   
   Py_DECREF(seq);

   RETURN_INT(result);

}



// -----------------------------------------------------------------------------  
PyObject* graph_remove_edge(PyObject* self, PyObject* args) {
   INIT_SELF_GRAPH();
   PyObject* from_pyobject, *to_pyobject = NULL;
   if(PyArg_ParseTuple(args, CHAR_PTR_CAST "O|O:remove_edge", &from_pyobject, &to_pyobject) <= 0)
      return NULL;
   try { 
      if(to_pyobject == NULL && is_EdgeObject(from_pyobject)) {
         so->_graph->remove_edge(((EdgeObject*)from_pyobject)->_edge);
      }
      else if(is_NodeObject(from_pyobject) && is_NodeObject(to_pyobject)) {
         Node* from_node = ((NodeObject*)from_pyobject)->_node;
         Node* to_node = ((NodeObject*)to_pyobject)->_node;
         so->_graph->remove_edge(from_node->_value, to_node->_value);
      }
      else if(from_pyobject != NULL && to_pyobject != NULL) {
         GraphDataPyObject a(from_pyobject), b(to_pyobject);
         so->_graph->remove_edge(&a, &b);
      }
   }
   catch(std::runtime_error e) {
      PyErr_SetString(PyExc_RuntimeError, e.what());
      return NULL;
   }
   RETURN_VOID();
}



// -----------------------------------------------------------------------------  
PyObject* graph_remove_all_edges(PyObject* self, PyObject* _) {
   INIT_SELF_GRAPH();
   //TODO: invalidate delivered EdgeObjects
   so->_graph->remove_all_edges();
   RETURN_VOID();
}



// -----------------------------------------------------------------------------  
PyObject* graph_is_directed(PyObject* self, PyObject* _) {
   INIT_SELF_GRAPH();
   RETURN_BOOL(so->_graph->is_directed());
}



// -----------------------------------------------------------------------------  
PyObject* graph_is_undirected(PyObject* self, PyObject* _) {
   INIT_SELF_GRAPH();
   RETURN_BOOL(so->_graph->is_undirected());
}



// -----------------------------------------------------------------------------  
PyObject* graph_make_directed(PyObject* self, PyObject* _) {
   INIT_SELF_GRAPH();
   //TODO: invalidate delivered EdgeObjects
   so->_graph->make_directed();
   RETURN_VOID();
}



// -----------------------------------------------------------------------------  
PyObject* graph_make_undirected(PyObject* self, PyObject* _) {
   INIT_SELF_GRAPH();
   so->_graph->make_undirected();
   RETURN_VOID();
}



// -----------------------------------------------------------------------------  
PyObject* graph_is_cyclic(PyObject* self, PyObject* _) {
   INIT_SELF_GRAPH();
   RETURN_BOOL(so->_graph->is_cyclic());
}



// -----------------------------------------------------------------------------  
PyObject* graph_is_acyclic(PyObject* self, PyObject* _) {
   INIT_SELF_GRAPH();
   RETURN_BOOL(so->_graph->is_acyclic());
}



// -----------------------------------------------------------------------------  
PyObject* graph_make_cyclic(PyObject* self, PyObject* _) {
   INIT_SELF_GRAPH();
   so->_graph->make_cyclic();
   RETURN_VOID();
}



// -----------------------------------------------------------------------------  
PyObject* graph_make_acyclic(PyObject* self, PyObject* _) {
   INIT_SELF_GRAPH();
   //TODO: invalidate EdgeObjects
   so->_graph->make_acyclic();
   RETURN_VOID();
}



// -----------------------------------------------------------------------------  
PyObject* graph_is_tree(PyObject* self, PyObject* _) {
   INIT_SELF_GRAPH();
   RETURN_BOOL(so->_graph->is_tree());
}



// -----------------------------------------------------------------------------  
PyObject* graph_is_blob(PyObject* self, PyObject* _) {
   INIT_SELF_GRAPH();
   RETURN_BOOL(so->_graph->is_blob());
}



// -----------------------------------------------------------------------------  
PyObject* graph_make_tree(PyObject* self, PyObject* _) {
   INIT_SELF_GRAPH();
   //TODO: invalidate EdgeObjects
   so->_graph->make_tree();
   RETURN_VOID();
}



// -----------------------------------------------------------------------------  
PyObject* graph_make_blob(PyObject* self, PyObject* _) {
   INIT_SELF_GRAPH();
   so->_graph->make_blob();
   RETURN_VOID();
}



// -----------------------------------------------------------------------------  
PyObject* graph_is_multi_connected(PyObject* self, PyObject* _) {
   INIT_SELF_GRAPH();
   RETURN_BOOL(so->_graph->is_multi_connected());
}



// -----------------------------------------------------------------------------  
PyObject* graph_is_singly_connected(PyObject* self, PyObject* _) {
   INIT_SELF_GRAPH();
   RETURN_BOOL(so->_graph->is_singly_connected());
}



// -----------------------------------------------------------------------------  
PyObject* graph_make_multi_connected(PyObject* self, PyObject* _) {
   INIT_SELF_GRAPH();
   so->_graph->make_multi_connected();
   RETURN_VOID();
}



// -----------------------------------------------------------------------------  
PyObject* graph_make_singly_connected(PyObject* self, PyObject* _) {
   INIT_SELF_GRAPH();
   //TODO: invalidate EdgeObjects
   so->_graph->make_singly_connected();
   RETURN_VOID();
}



// -----------------------------------------------------------------------------  
PyObject* graph_is_self_connected(PyObject* self, PyObject* _) {
   INIT_SELF_GRAPH();
   RETURN_BOOL(so->_graph->is_self_connected());
}



// -----------------------------------------------------------------------------  
PyObject* graph_make_self_connected(PyObject* self, PyObject* _) {
   INIT_SELF_GRAPH();
   so->_graph->make_self_connected();
   RETURN_VOID();
}



// -----------------------------------------------------------------------------  
PyObject* graph_make_not_self_connected(PyObject* self, PyObject* _) {
   INIT_SELF_GRAPH();
   //TODO: invalidate EdgeObjects
   so->_graph->make_not_self_connected();
   RETURN_VOID();
}



// -----------------------------------------------------------------------------  
PyObject* graph_get_nodes(PyObject* self, PyObject* _) {
   INIT_SELF_GRAPH();
   NodePtrIterator* it = so->_graph->get_nodes();

   NTIteratorObject<NodePtrIterator>* nti = iterator_new<NTIteratorObject<NodePtrIterator> >();
   nti->init(it, so);

   return (PyObject*)nti;
}



// -----------------------------------------------------------------------------  
PyObject* graph_get_node(PyObject* self, PyObject* pyobject) {
   INIT_SELF_GRAPH();
   GraphDataPyObject value(pyobject);
   Node *node = so->_graph->get_node(&value);
   if(node == NULL) {

      PyErr_SetString(PyExc_ValueError, "There is no node associated with the given value");
      return NULL; 
   }
   PyObject* ret = node_deliver(node, so);
//   Py_DECREF(pyobject);
   return ret;
}



// -----------------------------------------------------------------------------  
PyObject* graph_has_node(PyObject* self, PyObject* a) {
   INIT_SELF_GRAPH();
   bool res;
   Py_INCREF(a);
#ifdef __DEBUG_GAPI__
   std::cout << "graph_has_node\n";
#endif
   if(is_NodeObject(a)) {
      res = so->_graph->has_node(((NodeObject*)a)->_node);
   }
   else {
      GraphDataPyObject obj(a);
      res = so->_graph->has_node(&obj);
   }
   Py_DECREF(a);
   RETURN_BOOL(res); 
}



// -----------------------------------------------------------------------------  
PyObject* graph_get_nnodes(PyObject* self, PyObject* _) {
   INIT_SELF_GRAPH();
   if(so->_graph)
      RETURN_INT(so->_graph->get_nnodes());
   
   PyErr_SetString(PyExc_RuntimeError, "internal error in graph");
   return NULL;
}



// -----------------------------------------------------------------------------  
PyObject* graph_get_edges(PyObject* self, PyObject* _) {
   INIT_SELF_GRAPH();
   EdgePtrIterator* it = so->_graph->get_edges();

   ETIteratorObject<EdgePtrIterator>* nti = iterator_new<ETIteratorObject<EdgePtrIterator> >();
   nti->init(it, so);

   return (PyObject*)nti;
}



// -----------------------------------------------------------------------------  
PyObject* graph_has_edge(PyObject* self, PyObject* args) {
   INIT_SELF_GRAPH();
   bool res = false;
   PyObject* from_pyobject, *to_pyobject = NULL;
   if(PyArg_ParseTuple(args, CHAR_PTR_CAST "O|O:has_edge", &from_pyobject, &to_pyobject) <= 0)
      return NULL;
   
   if(to_pyobject == NULL && is_EdgeObject(from_pyobject)) {
      res = so->_graph->has_edge(((EdgeObject*)from_pyobject)->_edge);
   }
   else if(is_NodeObject(from_pyobject) && is_NodeObject(to_pyobject)) {
      Node* from_node = ((NodeObject*)from_pyobject)->_node;
      Node* to_node = ((NodeObject*)to_pyobject)->_node;
      res = so->_graph->has_edge(from_node->_value, to_node->_value);
   }
   else if(from_pyobject != NULL && to_pyobject != NULL) {
      GraphDataPyObject from(from_pyobject), to(to_pyobject);
      res = so->_graph->has_edge(&from, &to);
   }

   RETURN_BOOL(res);

}



// -----------------------------------------------------------------------------  
PyObject* graph_get_nedges(PyObject* self, PyObject* _) {
   INIT_SELF_GRAPH();
   RETURN_INT(so->_graph->get_nedges());
}



// -----------------------------------------------------------------------------  
PyObject* graph_get_subgraph_roots(PyObject* self, PyObject* args) {
   try {
      INIT_SELF_GRAPH();
      NodeVector* roots = so->_graph->get_subgraph_roots();
      NodeVectorPtrIterator* it = new NodeVectorPtrIterator(so->_graph, roots);

      NTIteratorObject<NodeVectorPtrIterator>* nti = iterator_new<NTIteratorObject<NodeVectorPtrIterator> >();
      nti->init(it, so);

      return (PyObject*)nti;
   }
   catch(std::runtime_error e) {
      PyErr_SetString(PyExc_TypeError, e.what());
      return NULL;
   }
}



// -----------------------------------------------------------------------------  
PyObject* graph_is_fully_connected(PyObject* self, PyObject* _) {
   INIT_SELF_GRAPH();
   RETURN_BOOL(so->_graph->is_fully_connected());
}



// -----------------------------------------------------------------------------
PyObject* graph_get_nsubgraphs(PyObject* self, PyObject* _) {
   INIT_SELF_GRAPH();
   RETURN_INT(so->_graph->get_nsubgraphs());
}



// -----------------------------------------------------------------------------
PyObject* graph_size_of_subgraph(PyObject* self, PyObject* node) {
   INIT_SELF_GRAPH();
   size_t size = 0;
   if(is_NodeObject(node))
      size = so->_graph->size_of_subgraph(((NodeObject*)node)->_node);
   else {
      GraphDataPyObject a(node);
      size = so->_graph->size_of_subgraph(&a);
   }
   RETURN_INT(size);
}





static PyObject* graph_has_flag(PyObject* self, PyObject* pyobject) {
   INIT_SELF_GRAPH();
   flag_t flag = (flag_t)PyInt_AsUnsignedLongMask(pyobject);
   RETURN_BOOL(so->_graph->has_flag(flag)); 
}



// -----------------------------------------------------------------------------
static PyObject* graph_has_path(PyObject* self, PyObject* args) {
   INIT_SELF_GRAPH();
   bool res = false;
   PyObject* from_pyobject, *to_pyobject = NULL;
   if(PyArg_ParseTuple(args, CHAR_PTR_CAST "OO:has_path", &from_pyobject, &to_pyobject) <= 0)
      return NULL;
   
   if(is_NodeObject(from_pyobject) && is_NodeObject(to_pyobject)) {
      Node* from_node = ((NodeObject*)from_pyobject)->_node;
      Node* to_node = ((NodeObject*)to_pyobject)->_node;
      res = so->_graph->has_path(from_node, to_node);
   }
   else if(from_pyobject != NULL && to_pyobject != NULL) {
      GraphDataPyObject from(from_pyobject), to(to_pyobject);
      res = so->_graph->has_path(&from,&to);
   }

   RETURN_BOOL(res);


}
