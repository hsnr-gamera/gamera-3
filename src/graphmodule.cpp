/*
 *
 * Copyright (C) 2001 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "gameramodule.hpp"
#include <Python.h>
#include <set>
#include <map>
#include <queue>
#include <stack>
#include <list>
#include <vector>
#include <exception>
#include <bitset>

#define DEBUG 0

#define NUM_NODE_DATA_ELEMENTS 4

// Forward references
struct IteratorObject;
struct NodeObject;
struct EdgeObject;
struct GraphObject;

typedef double CostType;

typedef std::set<NodeObject*> NodeSet;
typedef std::set<EdgeObject*> EdgeSet;
typedef std::list<NodeObject*> NodeList;
typedef std::list<EdgeObject*> EdgeList;
typedef std::vector<NodeObject*> NodeVector;
typedef std::vector<EdgeObject*> EdgeVector;
typedef std::stack<NodeObject*> NodeStack;
typedef std::stack<EdgeObject*> EdgeStack;
typedef std::queue<NodeObject*> NodeQueue;
typedef std::queue<EdgeObject*> EdgeQueue;
typedef std::map<PyObject*, NodeObject*> DataToNodeMap;
typedef std::map<NodeObject*, CostType> NodeToCostMap;
typedef std::map<NodeObject*, EdgeObject*> NodeToEdgeMap;
typedef std::map<size_t, NodeObject*> IdToNodeMap;
typedef std::vector<long> LongVector;

// For loading in the float matrix
static PyTypeObject* imagebase_type;
static PyTypeObject* cc_type;

// Object structs (defined at top so things are easier below)

struct IteratorObject {
  PyObject_HEAD
  PyObject*(*fp_next)(IteratorObject*);
  void(*fp_dealloc)(IteratorObject*);
  static void dealloc(IteratorObject* self) { };
};

union Any {
  int Int;
  long Long;
  bool Bool;
  float Float;
  double Double;
  CostType Cost;
  NodeObject* NodeObjectPtr;
  void* VoidPtr;
};

struct NodeObject {
  PyObject_HEAD
  GraphObject* m_graph;
  PyObject* m_data;
  EdgeList* m_out_edges;
  EdgeList* m_in_edges;
  bool m_is_subgraph_root;
  unsigned int m_set_id;
  long m_disj_set;
  Any m_node_properties[NUM_NODE_DATA_ELEMENTS];
};

#define NP_VISITED(a) ((a)->m_node_properties[0].Bool)
#define NP_KNOWN(a) ((a)->m_node_properties[0].Bool)
#define NP_DISTANCE(a) ((a)->m_node_properties[1].Cost)
#define NP_PATH(a) ((a)->m_node_properties[2].NodeObjectPtr)

struct EdgeObject {
  PyObject_HEAD
  GraphObject* m_graph;
  NodeObject* m_from_node;
  NodeObject* m_to_node;
  CostType m_cost;
  PyObject* m_label;
  Any m_edge_properties[NUM_NODE_DATA_ELEMENTS];
};

#define EP_VISITED(a) ((a)->m_edge_properties[0].Bool)
#define EP_PARTITION_COUNTER(a) ((a)->m_edge_properties[1].Bool)
#define EP_KNOWN(a) ((a)->m_node_properties[0].Bool)
#define EP_DISTANCE(a) ((a)->m_node_properties[1].Cost)
#define EP_PATH(a) ((a)->m_node_properties[2].NodeObjectPtr)

struct GraphObject {
  PyObject_HEAD
  bool m_directed;
  bool m_cyclic;
  bool m_blob;
  bool m_multiconnected;
  bool m_self_connected;
  NodeVector* m_nodes;
  // EdgeSet* m_edges; // EC: remove me later
  size_t m_nedges;
  NodeSet* m_subgraph_roots;
  DataToNodeMap* m_data_to_node;
  // LongVector* m_disj_set;
};

extern "C" {
  DL_EXPORT(void) initgraph(void);

  static PyObject* iterator_get_iter(PyObject* self);
  static PyObject* iterator_next(PyObject* self);

  static void node_dealloc(PyObject* self);
  static bool is_NodeObject(PyObject* self);
  static PyObject* node___call__(PyObject* self, PyObject* args, PyObject* kwds);
  static PyObject* node_get_data(PyObject* self);
  static int node_set_data(PyObject* self, PyObject* data);
  static PyObject* node_get_out_edges(PyObject* self);
  static PyObject* node_get_in_edges(PyObject* self);

  static void edge_dealloc(PyObject* self);
  static bool is_EdgeObject(PyObject* self);
  static PyObject* edge___repr__(PyObject* self);
  static PyObject* edge___call__(PyObject* self, PyObject* args, PyObject* kwds);
  static PyObject* edge_get_from_node(PyObject* self);
  static PyObject* edge_get_to_node(PyObject* self);
  static PyObject* edge_get_cost(PyObject* self);
  static int edge_set_cost(PyObject* self, PyObject* obj);
  static PyObject* edge_get_label(PyObject* self);
  static int edge_set_label(PyObject* self, PyObject* obj);

  static PyObject* graph_new(PyTypeObject* pytype, PyObject* args,
			     PyObject* kwds);
  static void graph_dealloc(PyObject* self);
  static PyObject* graph_copy(PyObject* self, PyObject* other);
  static PyObject* graph_add_node(PyObject* self, PyObject* pyobject);
  static PyObject* graph_add_nodes(PyObject* self, PyObject* pyobject);
  static PyObject* graph_add_nodes_and_edges(PyObject* self, PyObject* args);
  static PyObject* graph_remove_node_and_edges(PyObject* self, PyObject* pyobject);
  static PyObject* graph_remove_node(PyObject* self, PyObject* pyobject);
  static PyObject* graph_add_edge(PyObject* self, PyObject* args);
  static PyObject* graph_add_edges(PyObject* self, PyObject* args);
  static PyObject* graph_remove_edge(PyObject* self, PyObject* args);
  static PyObject* graph_remove_all_edges(PyObject* self, PyObject* args);
  static PyObject* graph_is_directed(PyObject* self, PyObject* args);
  static PyObject* graph_is_undirected(PyObject* self, PyObject* args);
  static PyObject* graph_make_directed(PyObject* self, PyObject* args);
  static PyObject* graph_make_undirected(PyObject* self, PyObject* args);
  static PyObject* graph_is_cyclic(PyObject* self, PyObject* args);
  static PyObject* graph_is_acyclic(PyObject* self, PyObject* args);
  static PyObject* graph_make_cyclic(PyObject* self, PyObject* args);
  static PyObject* graph_make_acyclic(PyObject* self, PyObject* args);
  static PyObject* graph_is_tree(PyObject* self, PyObject* args);
  static PyObject* graph_is_blob(PyObject* self, PyObject* args);
  static PyObject* graph_make_blob(PyObject* self, PyObject* args);
  static PyObject* graph_make_tree(PyObject* self, PyObject* args);
  static PyObject* graph_is_multi_connected(PyObject* self, PyObject* args);
  static PyObject* graph_is_singly_connected(PyObject* self, PyObject* args);
  static PyObject* graph_make_multi_connected(PyObject* self, PyObject* args);
  static PyObject* graph_make_singly_connected(PyObject* self, PyObject* args);
  static PyObject* graph_is_self_connected(PyObject* self, PyObject* args);
  static PyObject* graph_make_self_connected(PyObject* self, PyObject* args);
  static PyObject* graph_make_not_self_connected(PyObject* self, PyObject* args);
  static PyObject* graph_get_node(PyObject* self, PyObject* args);
  static PyObject* graph_get_nodes(PyObject* self, PyObject* args);
  static PyObject* graph_has_node(PyObject* self, PyObject* node);
  static PyObject* graph_get_nnodes(PyObject* self);
  static PyObject* graph_get_edges(PyObject* self, PyObject* args);
  static PyObject* graph_has_edge(PyObject* self, PyObject* args);
  static PyObject* graph_get_nedges(PyObject* self);
  static PyObject* graph_BFS(PyObject* self, PyObject* args);
  static PyObject* graph_DFS(PyObject* self, PyObject* args);
  static PyObject* graph_get_subgraph_roots(PyObject* self, PyObject* args);
  static PyObject* graph_size_of_subgraph(PyObject* self, PyObject* args);
  static PyObject* graph_get_nsubgraphs(PyObject* self);
  static PyObject* graph_is_fully_connected(PyObject* self, PyObject* args);
  static PyObject* graph_create_spanning_tree(PyObject* self, PyObject* pyobject);
  static PyObject* graph_partitions(PyObject* self, PyObject* args);
  static PyObject* graph_djikstra_shortest_path(PyObject* self, PyObject* root);
  static PyObject* graph_djikstra_all_pairs_shortest_path(PyObject* self, PyObject* root);
  static PyObject* graph_all_pairs_shortest_path(PyObject* so, PyObject* args);
  static PyObject* graph_minimum_spanning_tree(PyObject* so, PyObject* args);

  static PyObject* new_Tree(PyObject* so, PyObject* args);
  static PyObject* new_FreeGraph(PyObject* so, PyObject* args);
  static PyObject* new_DAG(PyObject* so, PyObject* args);
}

inline static PyObject* node_new_simple(GraphObject* graph, PyObject *data);
inline static bool node_check_alive(NodeObject* node);

inline static PyObject* edge_new_simple(GraphObject* graph, NodeObject* from_node, 
					NodeObject* to_node, CostType cost = 1.0,
					PyObject* label = NULL);
inline static bool edge_check_alive(EdgeObject* edge);

inline static GraphObject* graph_new_simple(bool directed = true, bool cyclic = true,
					    bool blob = true, bool multiconnected = true,
					    bool self_connected = true);
inline static GraphObject* graph_copy(GraphObject* so, bool directed = true,
				      bool cyclic = true, bool blob = true,
				      bool multiconnected = true, bool self_connected = true);
inline static NodeObject* graph_add_node(GraphObject* self, NodeObject* node);
inline static NodeObject* graph_add_node(GraphObject* self, PyObject* pyobject);
inline static NodeObject* graph_find_node(GraphObject* so, PyObject* pyobject,
					  bool exception = true);
inline static bool graph_remove_node_and_edges(GraphObject* so, NodeObject* node);
inline static bool graph_remove_node(GraphObject* so, NodeObject* node);
inline static size_t graph_disj_set_find_and_compress(GraphObject* so, size_t x);
inline void graph_disj_set_union_by_height(GraphObject* so, size_t a, size_t b);
inline static bool graph_add_edge0(GraphObject* so, NodeObject* from_node,
				   NodeObject* to_node, CostType cost = 1.0,
				   PyObject* label = NULL, bool check = true);
inline static bool graph_add_edge(GraphObject* so, NodeObject* from_node,
				  NodeObject* to_node, CostType cost = 1.0, 
				  PyObject* label = NULL);
inline static bool graph_add_edge(GraphObject* so, PyObject* from_object,
				  PyObject* to_object, CostType cost = 1.0,
				  PyObject* label = NULL);
inline static bool graph_remove_edge(GraphObject* so, EdgeObject* edge);
inline static void graph_make_directed(GraphObject* so);
inline static void graph_make_undirected(GraphObject* so);
inline static void graph_make_cyclic(GraphObject* so);
inline static void graph_make_acyclic(GraphObject* so);
inline static void graph_make_blob(GraphObject* so);
inline static void graph_make_tree(GraphObject* so);
inline static void graph_make_multi_connected(GraphObject* so);
inline static void graph_make_singly_connected(GraphObject* so, bool maximum_cost = true);
inline static void graph_make_self_connected(GraphObject* so);
inline static void graph_make_not_self_connected(GraphObject* so);
inline static bool graph_has_node(GraphObject* so, NodeObject* node);
inline static size_t graph_has_edge(GraphObject* so, NodeObject* from_node,
				    NodeObject* to_node);
inline static size_t graph_size_of_subgraph(GraphObject* so, NodeObject* node);
inline static GraphObject* graph_create_spanning_tree(GraphObject* so, NodeObject* root);
inline static NodeList* graph_djikstra_shortest_path(GraphObject* self, NodeObject* root);
inline static void graph_minimum_spanning_tree(GraphObject* so);

// Iterators

static PyTypeObject IteratorType = {
  PyObject_HEAD_INIT(NULL)
  0,
};

template<class T>
static T* iterator_new_simple() {
  IteratorObject* so;
  IteratorType.tp_basicsize = sizeof(T);
  so = (IteratorObject*)(IteratorType.tp_alloc(&IteratorType, 0));
  so->fp_next = T::next;
  so->fp_dealloc = T::dealloc;
  return (T*)so;
}

static void iterator_dealloc(PyObject* self) {
  IteratorObject* so = (IteratorObject*)self;
#if DEBUG
  std::cerr << "iterator dealloc\n";
#endif
  (*(so->fp_dealloc))(so);
  self->ob_type->tp_free(self);
}

static PyObject* iterator_get_iter(PyObject* self) {
  Py_INCREF(self);
  return self;
}

static PyObject* iterator_next(PyObject* self) {
  IteratorObject* so = (IteratorObject*)self;
  PyObject* result = (*(so->fp_next))(so);
  if (result == NULL) {
    PyErr_SetString(PyExc_StopIteration, "");
    return 0;
  }
  Py_INCREF(result);
  return result;
}

// Node

static PyTypeObject NodeType = {
  PyObject_HEAD_INIT(NULL)
  0,
};

extern PyTypeObject* get_NodeType() {
  return &NodeType;
}

PyMethodDef node_methods[] = {
  { NULL }
};

PyGetSetDef node_getset[] = {
  { "data", (getter)node_get_data, (setter)node_set_data,
    "Data stored in the node (get/set)", 0 },
  { "edges", (getter)node_get_out_edges, 0,
    "Edges pointing out from node (get) (alias for out_edges)", 0 },
  { "out_edges", (getter)node_get_out_edges, 0,
    "Edges pointing out from node (get)", 0 },
  { "in_edges", (getter)node_get_in_edges, 0,
    "Edges pointing in to node (get)", 0 },
  { NULL }
};

static PyObject* node_new_simple(GraphObject* graph, PyObject* data) {
  NodeObject* so;
  so = (NodeObject*)(NodeType.tp_alloc(&NodeType, 0));
  so->m_graph = graph;
  Py_INCREF(data);
  so->m_data = data;
  so->m_out_edges = new EdgeList();
  so->m_in_edges = new EdgeList();
  return (PyObject*)so;
}

static void node_dealloc(PyObject* self) {
  NodeObject* so = (NodeObject*)self;
#if DEBUG
  std::cerr << "node dealloc " << PyString_AsString(PyObject_Repr(self)) << std::endl;
#endif
  delete so->m_out_edges;
  delete so->m_in_edges;
  Py_DECREF((PyObject*)so->m_data);
  self->ob_type->tp_free(self);
}

static bool is_NodeObject(PyObject* self) {
  return PyObject_TypeCheck(self, &NodeType);
}

static bool node_check_alive(NodeObject* so) {
  if (so->m_graph == NULL) {
    PyErr_SetString(PyExc_RuntimeError, 
		    "The underlying graph of this node has been destroyed.");
    return false;
  }
  return true;
}

PyObject* node___repr__(PyObject* self) {
  NodeObject* so = (NodeObject*)self;
  if (!node_check_alive(so))
    return 0;
  return PyString_FromFormat("<Node of %s>", 
			     PyString_AsString(PyObject_Repr(so->m_data)));
}

PyObject* node___call__(PyObject* self, PyObject* args, PyObject* kwds) {
  PyObject* data = NULL;
  if (PyArg_ParseTuple(args, "|O", &data) <= 0)
    return 0;
  if (data == NULL)
    return node_get_data(self);
  node_set_data(self, data);
  Py_INCREF(Py_None);
  return Py_None;
}  

PyObject* node_get_data(PyObject* self) {
  NodeObject* so = (NodeObject*)self;
  if (!node_check_alive(so))
    return 0;
  Py_INCREF(so->m_data);
  return so->m_data;
}

int node_set_data(PyObject* self, PyObject* data) {
  NodeObject* so = (NodeObject*)self;
  if (!node_check_alive(so))
    return -1;
  Py_DECREF(so->m_data);
  so->m_data = data;
  Py_INCREF(so->m_data);
  return 0;
}

// Edge

static PyTypeObject EdgeType = {
  PyObject_HEAD_INIT(NULL)
  0,
};

extern PyTypeObject* get_EdgeType() {
  return &EdgeType;
}

PyMethodDef edge_methods[] = {
  { NULL }
};

PyGetSetDef edge_getset[] = {
  { "from_node", (getter)edge_get_from_node, 0,
    "Node this edge starts from (get)", 0 },
  { "to_node", (getter)edge_get_to_node, 0,
    "Node this edge points to (get)", 0 },
  { "cost", (getter)edge_get_cost, (setter)edge_set_cost,
    "Cost of traversing this node (get/set)", 0 },
  { "label", (getter)edge_get_label, (setter)edge_set_label,
    "An arbitrary label attached to the edge (get/set)", 0 },
  { NULL }
};

static PyObject* edge_new_simple(GraphObject* graph, NodeObject* from_node,
				 NodeObject* to_node, CostType cost, PyObject* label) {
  EdgeObject* so;
  so = (EdgeObject*)EdgeType.tp_alloc(&EdgeType, 0);
  so->m_graph = graph;
  so->m_from_node = (NodeObject*)from_node;
  so->m_to_node = (NodeObject*)to_node;
  so->m_cost = cost;
  so->m_label = label;
  if (label != NULL)
    Py_INCREF(label);
  return (PyObject*)so;
}  

static void edge_dealloc(PyObject* self) {
#if DEBUG
  std::cerr << "edge dealloc " << PyString_AsString(PyObject_Repr(self)) << std::endl;
#endif
  EdgeObject* so = (EdgeObject*)self;
  if (so->m_label != NULL)
    Py_DECREF(so->m_label);
  self->ob_type->tp_free(self);
}

static bool is_EdgeObject(PyObject* self) {
  return PyObject_TypeCheck(self, &NodeType);
}

static bool edge_check_alive(EdgeObject* so) {
  if (so->m_graph == NULL) {
    PyErr_SetString(PyExc_RuntimeError, 
		    "The underlying graph of this edge has been destroyed.");
    return false;
  }
  return true;
}

PyObject* edge___repr__(PyObject* self) {
  EdgeObject* so = (EdgeObject*)self;
  if (!edge_check_alive(so))
    return 0;
  PyObject* cost = PyFloat_FromDouble(so->m_cost);
  return PyString_FromFormat("<Edge from %s to %s (%s)>", 
			     PyString_AsString(PyObject_Repr((PyObject*)so->m_from_node)),
			     PyString_AsString(PyObject_Repr((PyObject*)so->m_to_node)),
			     PyString_AsString(PyObject_Repr(cost)));
}

PyObject* edge___call__(PyObject* self, PyObject* args, PyObject* kwds) {
  PyObject* data = NULL;
  if (PyArg_ParseTuple(args, "|O", &data) <= 0)
    return 0;
  if (data == NULL)
    return edge_get_cost(self);
  edge_set_cost(self, data);
  Py_INCREF(Py_None);
  return Py_None;
}  

PyObject* edge_get_from_node(PyObject* self) {
  EdgeObject* so = (EdgeObject*)self;
  if (!edge_check_alive(so))
    return 0;
  Py_INCREF((PyObject*)so->m_from_node);
  return (PyObject*)so->m_from_node;
}

PyObject* edge_get_to_node(PyObject* self) {
  EdgeObject* so = (EdgeObject*)self;
  if (!edge_check_alive(so))
    return 0;
  Py_INCREF((PyObject*)so->m_to_node);
  return (PyObject*)so->m_to_node;
}

PyObject* edge_get_cost(PyObject* self) {
  EdgeObject* so = (EdgeObject*)self;
  if (!edge_check_alive(so))
    return 0;
  return PyFloat_FromDouble(so->m_cost);
}

int edge_set_cost(PyObject* self, PyObject* cost) {
  if (!PyFloat_Check(cost)) {
    PyErr_SetString(PyExc_TypeError, "edge: expected a float");
    return -1;
  }
  EdgeObject* so = (EdgeObject*)self;
  if (!edge_check_alive(so))
    return -1;
  so->m_cost = PyFloat_AsDouble(cost);
  return 0;
}

PyObject* edge_get_label(PyObject* self) {
  EdgeObject* so = (EdgeObject*)self;
  if (!edge_check_alive(so))
    return 0;
  Py_INCREF(so->m_label);
  return so->m_label;
}

int edge_set_label(PyObject* self, PyObject* data) {
  EdgeObject* so = (EdgeObject*)self;
  if (!edge_check_alive(so))
    return -1;
  if (so->m_label != NULL)
    Py_DECREF(so->m_label);
  so->m_label = data;
  Py_INCREF(so->m_label);
  return 0;
}

// Concrete iterators

template<class T>
struct BasicIterator : IteratorObject {
  int init(typename T::iterator begin, typename T::iterator end) {
    m_it = m_begin = begin;
    m_end = end;
    return 1;
  }
  static PyObject* next(IteratorObject* self) {
    BasicIterator<T>* so = (BasicIterator<T>*)self;
    if (so->m_it == so->m_end)
      return 0;
    return (PyObject*)*((so->m_it)++);
  }
  typename T::iterator m_it, m_begin, m_end;
};

template<class T>
struct MapValueIterator : IteratorObject {
  int init(typename T::const_iterator begin, typename T::const_iterator end) {
    m_it = m_begin = begin;
    m_end = end;
    return 1;
  }
  static PyObject* next(IteratorObject* self) {
    BasicIterator<T>* so = (BasicIterator<T>*)self;
    if (so->m_it == so->m_end)
      return 0;
    return (PyObject*)(*((so->m_it)++)).second;
  }
  typename T::const_iterator m_it, m_begin, m_end;
};

template<class T>
struct MapKeyIterator : IteratorObject {
  int init(typename T::iterator begin, typename T::iterator end) {
    m_it = m_begin = begin;
    m_end = end;
    return 1;
  }
  static PyObject* next(IteratorObject* self) {
    BasicIterator<T>* so = (BasicIterator<T>*)self;
    if (so->m_it == so->m_end)
      return 0;
    return (PyObject*)(*((so->m_it)++)).first;
  }
  typename T::iterator m_it, m_begin, m_end;
};

struct BFSIterator : IteratorObject {
  int init(GraphObject* graph, NodeObject* root) {
    m_node_queue = new NodeQueue();
    m_node_queue->push(root);
    NodeVector::iterator i = graph->m_nodes->begin();
    for (; i != graph->m_nodes->end(); ++i)
      NP_VISITED(*i) = false;
    NP_VISITED(root) = true;
    return 1;
  }
  static PyObject* next(IteratorObject* self) {
    BFSIterator* so = (BFSIterator*)self;
    if (so->m_node_queue->empty()) {
	return 0;
    }
    NodeObject* node = so->m_node_queue->front();
    so->m_node_queue->pop();
    for (EdgeList::iterator i = node->m_out_edges->begin();
	 i != node->m_out_edges->end(); ++i) {
      NodeObject* subnode = (*i)->m_to_node;
      if (!NP_VISITED(subnode)) {
	NP_VISITED(subnode) = true;
	so->m_node_queue->push(subnode);
      }
    }
    return (PyObject*)node;
  }
  NodeQueue* m_node_queue;
};

struct DFSIterator : IteratorObject {
  int init(GraphObject* graph, NodeObject* root) {
    m_node_stack = new NodeStack();
    m_node_stack->push(root);
    NodeVector::iterator i = graph->m_nodes->begin();
    for (; i != graph->m_nodes->end(); ++i)
      NP_VISITED(*i) = false;
    NP_VISITED(root) = true;
    return 1;
  }
  static PyObject* next(IteratorObject* self) {
    DFSIterator* so = (DFSIterator*)self;
    if (so->m_node_stack->empty()) {
      return 0;
    }
    NodeObject* node = so->m_node_stack->top();
    so->m_node_stack->pop();
    for (EdgeList::iterator i = node->m_out_edges->begin();
	 i != node->m_out_edges->end(); ++i) {
      NodeObject* subnode = (*i)->m_to_node;
      if (!NP_VISITED(subnode)) {
	NP_VISITED(subnode) = true;
	so->m_node_stack->push(subnode);
      }
    }
    return (PyObject*)node;
  }
  NodeStack* m_node_stack;
};

PyObject* node_get_out_edges(PyObject* self) {
  NodeObject* so = ((NodeObject*)self);
  if (!node_check_alive(so))
    return 0;
  typedef BasicIterator<EdgeList> EdgeListIterator;
  EdgeListIterator* iterator = iterator_new_simple<EdgeListIterator>();
  iterator->init(so->m_out_edges->begin(), so->m_out_edges->end());
  return (PyObject*)iterator;
}

PyObject* node_get_in_edges(PyObject* self) {
  NodeObject* so = ((NodeObject*)self);
  if (!node_check_alive(so))
    return 0;
  typedef BasicIterator<EdgeList> EdgeListIterator;
  EdgeListIterator* iterator = iterator_new_simple<EdgeListIterator>();
  iterator->init(so->m_in_edges->begin(), so->m_in_edges->end());
  return (PyObject*)iterator;
}

// GRAPH

static PyTypeObject GraphType = {
  PyObject_HEAD_INIT(NULL)
  0,
};

extern PyTypeObject* get_GraphType() {
  return &GraphType;
}

static bool is_GraphObject(PyObject* self) {
  return PyObject_TypeCheck(self, &GraphType);
}

PyMethodDef graph_methods[] = {
  { "copy", graph_copy, METH_VARARGS,
    "Copy a graph (optionally specifying new flags for the new graph)\n" \
    "In some cases, copying the graph to a new graph type may be faster\n" \
    "than using one of the in-place conversion functions." },
  { "add_node", graph_add_node, METH_O,
    "Add a node to the graph.\n" \
    "The node is by default not connected to anything." },
  { "add_nodes", graph_add_nodes, METH_O,
    "Add each node in a (Python) list of nodes to the graph." },
  { "add_nodes_and_edges", graph_add_nodes_and_edges, METH_VARARGS,
    "Add a list of nodes and connect them based on an adjacency matrix." },
  { "remove_node_and_edges", graph_remove_node_and_edges, METH_O,
    "Remove a node from the graph, removing all associated edges" },
  { "remove_node", graph_remove_node, METH_O,
    "Remove a node from the graph, stitching together the broken edges" },
  { "add_edge", graph_add_edge, METH_VARARGS,
    "Add an edge between two objects in the graph" },
  { "add_edges", graph_add_edges, METH_VARARGS,
    "Add edges from a list of tuples of the form:\n" \
    "   (from_node, to_node, [cost,] [label])." },
  { "remove_edge", graph_remove_edge, METH_VARARGS,
    "Remove an edge between two objects on the graph" },
  { "remove_all_edges", graph_remove_all_edges, METH_NOARGS,
    "Remove all the edges in the graph" },
  { "is_directed", graph_is_directed, METH_NOARGS,
    "Is the graph defined as being directed?" },
  { "is_undirected", graph_is_undirected, METH_NOARGS,
    "Is the graph defined as being undirected?" },
  { "make_directed", graph_make_directed, METH_VARARGS,
    "Make the graph a directed one" },
  { "make_undirected", graph_make_directed, METH_NOARGS,
    "Make the graph an undirected one" },
  { "is_cyclic", graph_is_cyclic, METH_NOARGS,
    "Is the graph defined as being cyclic?" },
  { "is_acyclic", graph_is_acyclic, METH_NOARGS,
    "Is the graph defined as being acyclic?" },
  { "make_cyclic", graph_make_cyclic, METH_VARARGS,
    "Allow the graph to include cycles" },
  { "make_acyclic", graph_make_acyclic, METH_NOARGS,
    "Remove cycles and further disallow cycles" },
  { "is_tree", graph_is_tree, METH_NOARGS,
    "Is the graph defined as being a tree?" },
  { "is_blob", graph_is_blob, METH_NOARGS,
    "Is the graph defined as being a blob (i.e. not a tree)?" },
  { "make_tree", graph_make_tree, METH_NOARGS,
    "Turn the graph into a tree" },
  { "make_blob", graph_make_blob, METH_NOARGS,
    "Make the graph into a blob (i.e. not a tree)" },
  { "is_multi_connected", graph_is_multi_connected, METH_NOARGS,
    "Is the graph defined as being multi-connected " \
    "(i.e. multiple edges between the same pair of nodes)?" },
  { "is_singly_connected", graph_is_singly_connected, METH_NOARGS,
    "Is the graph defined as being singly-connected " \
    "(i.e. at most one edge between each pair of nodes)?" },
  { "make_multi_connected", graph_make_multi_connected, METH_VARARGS,
    "Allow the graph to be multi-conncted" },
  { "make_singly_connected", graph_make_singly_connected, METH_VARARGS,
    "Remove multi-connections and make singly-connected" },
  { "is_self_connected", graph_is_self_connected, METH_NOARGS,
    "Is the graph defined as being self-connected (i.e. nodes can point to themselves)?" },
  { "make_self_connected", graph_make_self_connected, METH_VARARGS,
    "Allow the graph to be self-conncted" },
  { "make_not_self_connected", graph_make_not_self_connected, METH_NOARGS,
    "Remove self-connections and make the graph not self-connected" },
  { "get_node", graph_get_node, METH_O,
    "An iterator over all nodes in the graph" },
  { "get_nodes", graph_get_nodes, METH_NOARGS,
    "An iterator over all nodes in the graph" },
  { "get_subgraph_roots", graph_get_subgraph_roots, METH_NOARGS,
    "An iterator over the root node of each subgraph" },
  { "has_node", graph_has_node, METH_O,
    "Returns true if graph has the given node" },
  { "get_edges", graph_get_edges, METH_NOARGS,
    "An iterator over all edges in the graph" },
  { "has_edge", graph_has_edge, METH_VARARGS,
    "Returns true if graph has the given edge" },
  { "size_of_subgraph", graph_size_of_subgraph, METH_O,
    "Returns the size of the subgraph rooted at the given node" },
  { "is_fully_connected", graph_is_fully_connected, METH_NOARGS,
    "True if there is only one subgraph in the graph" },
  { "BFS", graph_BFS, METH_O,
    "An iterator over all the nodes in breadth-first order" },
  { "DFS", graph_DFS, METH_O,
    "An iterator over all the nodes in depth-first order" },
  { "create_spanning_tree", graph_create_spanning_tree, METH_O,
    "Returns a new graph with a spanning tree of all nodes reachable from the given node" },
  { "partitions", graph_partitions, METH_O,
    "An iterator over all the partitions of the subgraph at the given root" },
  { "djikstra_shortest_path", graph_djikstra_shortest_path, METH_O,
    "The shortest paths from the given root" },
  { "shortest_path", graph_djikstra_shortest_path, METH_O,
    "The shortest path from the given root" },
  { "djikstra_all_pairs_shortest_path", graph_djikstra_all_pairs_shortest_path, METH_NOARGS,
    "The shortest paths between all nodes (using Djikstra algorithm N times)" },
  { "all_pairs_shortest_path", graph_all_pairs_shortest_path, METH_NOARGS,
    "The shortest paths between all nodes (using tight inner loops)" },
  { "minimum_spanning_tree", graph_minimum_spanning_tree, METH_NOARGS,
    "Creates a minimum spanning tree (in place)" },
  { NULL }
};

PyGetSetDef graph_getset[] = {
  { "nnodes", (getter)graph_get_nnodes, 0,
    "Number of nodes in the graph", 0 },
  { "nedges", (getter)graph_get_nedges, 0,
    "Number of edges in the graph", 0 },
  { "nsubgraphs", (getter)graph_get_nsubgraphs, 0,
    "Number of subgraphs in the graph", 0 },
  { NULL }
};

static GraphObject* graph_new_simple(bool directed, bool cyclic, bool blob,
				     bool multiconnected, bool self_connected) {
  GraphObject* so;
  so = (GraphObject*)(GraphType.tp_alloc(&GraphType, 0));
  if (!blob) {
    cyclic = false;
    directed = true;
  }
  if (!cyclic) {
    self_connected = false;
    multiconnected = false;
  }
  so->m_directed = directed;
  so->m_cyclic = cyclic;
  so->m_blob = blob;
  so->m_multiconnected = multiconnected;
  so->m_self_connected = self_connected;
  so->m_nodes = new NodeVector();
  // so->m_edges = new EdgeSet(); EC
  so->m_nedges = 0;
  so->m_data_to_node = new DataToNodeMap();
  so->m_subgraph_roots = new NodeSet();
  // so->m_disj_set = new LongVector();
  // so->m_disj_set->push_back(0);
  return so;
}

static PyObject* graph_new(PyTypeObject* pytype, PyObject* args, 
			   PyObject* kwds) {
  int directed = 1, cyclic = 1, blob = 1, multiconnected = 1, self_connected = 1;
  if (PyArg_ParseTuple(args, "|iiiii", &directed, &cyclic, &blob, 
		       &multiconnected, &self_connected) <= 0)
    return 0;
  return (PyObject*)graph_new_simple(directed != 0, cyclic != 0, blob != 0,
				     multiconnected != 0,
				     self_connected != 0);
}

static void graph_dealloc(PyObject* self) {
#if DEBUG
  std::cerr << "graph_dealloc\n";
#endif
  GraphObject* so = (GraphObject*)self;

  // Decrease reference counts on node list
  for (NodeVector::iterator i = so->m_nodes->begin();
       i != so->m_nodes->end(); ++i) {
    for (EdgeList::iterator j = (*i)->m_out_edges->begin();
	 j != (*i)->m_out_edges->end(); ++j)
      Py_DECREF((PyObject*)*j);
      // (*j)->m_graph = NULL;
    Py_DECREF((PyObject*)*i);
  }
  delete so->m_nodes;
  // delete so->m_edges;
  delete so->m_data_to_node;
  delete so->m_subgraph_roots;
  // delete so->m_disj_set;
  self->ob_type->tp_free(self);
}

static GraphObject* graph_copy(GraphObject* so, bool directed, bool cyclic, bool blob,
			       bool multiconnected, bool self_connected) {
  GraphObject* result = graph_new_simple(directed, cyclic, blob, multiconnected,
					 self_connected);
  result->m_nodes->reserve(so->m_nodes->size());
  /* if (!result->m_cyclic)
     result->m_disj_set->reserve(so->m_nodes->size() + 1); */

  for (NodeVector::iterator i = so->m_nodes->begin();
       i != so->m_nodes->end(); ++i)
    graph_add_node(result, (*i)->m_data);

  for (NodeVector::iterator i = so->m_nodes->begin();
       i != so->m_nodes->end(); ++i)
    for (EdgeList::iterator j = (*i)->m_out_edges->begin();
	 j != (*i)->m_out_edges->end(); ++j)
      graph_add_edge(result, (*i)->m_data, (*j)->m_to_node->m_data,
		     (*j)->m_cost);
  return result;
}

static PyObject* graph_copy(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  int directed = 1, cyclic = 1, blob = 1, multiconnected = 1, self_connected = 1;
  if (PyArg_ParseTuple(args, "|iiiii", &directed, &cyclic, &blob, 
		       &multiconnected, &self_connected) <= 0)
    return 0;
  return (PyObject*)graph_copy(so, directed != 0, cyclic != 0, blob != 0,
			       multiconnected != 0, self_connected != 0);
}

static NodeObject* graph_find_node(GraphObject* so, PyObject* pyobject, bool exception) {
  // Find the node
  NodeObject* node;
  if (is_NodeObject(pyobject))
    return (NodeObject*)pyobject;
  DataToNodeMap::iterator i = so->m_data_to_node->find(pyobject);
  if (i != so->m_data_to_node->end()) {
    node = i->second;
    return node;
  }
  if (exception)
    PyErr_SetString(PyExc_TypeError, 
		    PyString_AsString
		    (PyString_FromFormat
		     ("Node containing %s is not in the graph", 
		      PyString_AsString(PyObject_Repr(pyobject)))));
  return 0;
}

static NodeObject* graph_add_node(GraphObject* so, NodeObject* node) {
  so->m_nodes->push_back(node);
  node->m_set_id = so->m_nodes->size();
  node->m_disj_set = 0;
  (*(so->m_data_to_node))[node->m_data] = node;
  so->m_subgraph_roots->insert(node);
  node->m_is_subgraph_root = true; // EC
  /* if (!so->m_cyclic) {
    so->m_disj_set->push_back(0);
    } */
  return node;
}

static NodeObject* graph_add_node(GraphObject* so, PyObject* pyobject) {
  NodeObject* node = graph_find_node(so, pyobject, false);
  if (node == 0) {
    node = (NodeObject*)node_new_simple(so, pyobject);
    return graph_add_node(so, node);
  }
  return node;
}

static PyObject* graph_add_node(PyObject* self, PyObject* pyobject) {
  GraphObject* so = ((GraphObject*)self);
  NodeObject* node = graph_find_node(so, pyobject, false);
  if (node == 0) {
    node = (NodeObject*)node_new_simple(so, pyobject);
    graph_add_node(so, node);
    return PyInt_FromLong((long)1);
  }
  return PyInt_FromLong((long)0);
}

static PyObject* graph_add_nodes(PyObject* self, PyObject* pyobject) {
  GraphObject* so = ((GraphObject*)self);
  if (!PyList_Check(pyobject)) {
    PyErr_SetString(PyExc_TypeError, "Argument must be a list of nodes");
    return 0;
  }
  size_t list_size = PyList_Size(pyobject);
  bool result = false;
  
  /* if (!so->m_cyclic)
     so->m_disj_set->reserve(so->m_disj_set->size() + list_size); */

  for (size_t i = 0; i < list_size; ++i)
    if (graph_add_node(so, PyList_GET_ITEM(pyobject, i)))
      result |= true;
  return PyInt_FromLong((long)result);
}

static PyObject* graph_add_nodes_and_edges(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  PyObject *nodes, *matrix;
  if (PyArg_ParseTuple(args, "OO", &nodes, &matrix) <= 0)
    return 0;
  if (!PyList_Check(nodes)) {
    PyErr_SetString(PyExc_TypeError, "Argument 1 must be a list of nodes");
    return 0;
  }
  if (!PyObject_TypeCheck(matrix, imagebase_type)) {
    PyErr_SetString(PyExc_TypeError, "Argument 2 must be a Float image");
    return 0;
  }
  if (get_image_combination(matrix, cc_type) != FLOATIMAGEVIEW) {
    PyErr_SetString(PyExc_TypeError, "Argument 2 must be a Float image");
    return 0;
  }

  FloatImageView *mat = ((FloatImageView*)((RectObject*)matrix)->m_x);
  size_t size = PyList_Size(nodes);
  if (mat->nrows() != size || mat->ncols() != size) {
    PyErr_SetString
      (PyExc_RuntimeError, 
       "The list of nodes and the adjacency matrix must have the same dimensions");
    return 0;
  }    

  /* if (!so->m_cyclic)
     so->m_disj_set->reserve(so->m_disj_set->size() + size); */

  for (size_t i = 0; i < size; ++i)
    graph_add_node(so, PyList_GetItem(nodes, i));

  for (size_t i = 0; i < size; ++i)
    for (size_t j = 0; j < size; ++j)
      graph_add_edge(so, PyList_GetItem(nodes, i), PyList_GetItem(nodes, j), mat->get(i, j));

  Py_INCREF(Py_None);
  return Py_None;
}

static bool graph_remove_node_and_edges(GraphObject* so, NodeObject* node) {
  // Remove all edges coming out of that node
  // Funny looping construct is so we can remove an edge and not
  // invalid the iterator
  for (EdgeList::iterator i, j = node->m_out_edges->begin(); 
       j != node->m_out_edges->end(); ) {
    i = j++;
    graph_remove_edge(so, *i);
  }  

  // Remove all edges pointing into that node
  // Funny looping construct is so we can remove an edge and not
  // invalid the iterator
  for (EdgeList::iterator i, j = node->m_in_edges->begin(); 
       j != node->m_in_edges->end(); ) {
    i = j++;
    graph_remove_edge(so, *i);
  }  

  // Adjust the disjoint set
  // Fix all the pointers in the nodes themselves
  if (!so->m_cyclic) {
    // LongVector* vec = (so->m_disj_set);
    size_t set_id = node->m_set_id;
    // Adjust all m_set_id's
    for (NodeVector::iterator i = so->m_nodes->begin(); i != so->m_nodes->end(); ++i) {
      if ((*i)->m_set_id > set_id)
	(*i)->m_set_id--;
      if ((*i)->m_disj_set > (long)set_id)
	(*i)->m_disj_set--;
    }
  }

  // Actually remove the node
  so->m_data_to_node->erase(node->m_data);
  if (node->m_is_subgraph_root) {
    so->m_subgraph_roots->erase(node);
    node->m_is_subgraph_root = false; // EC
  }
  for (NodeVector::iterator i = so->m_nodes->begin();
       i != so->m_nodes->end(); ++i)
    if ((*i) == node) {
      so->m_nodes->erase(i);
      break;
    }  // EC: actually slower than before

  Py_DECREF((PyObject*)node);
  return true;
}

static PyObject* graph_remove_node_and_edges(PyObject* self, PyObject* a) {
  GraphObject* so = ((GraphObject*)self);
  NodeObject* node;
  // Find the node
  node = graph_find_node(so, a);
  if (node == 0)
    return 0;
  return PyInt_FromLong((long)graph_remove_node_and_edges(so, node));
}

static bool graph_remove_node(GraphObject* so, NodeObject* node) {
  // Stitch together edges
  for (EdgeList::iterator i = node->m_in_edges->begin();
       i != node->m_in_edges->end(); ++i) {
    for (EdgeList::iterator j = node->m_out_edges->begin();
	 j != node->m_out_edges->end(); ++j) {
      graph_add_edge(so, (*i)->m_from_node, (*j)->m_to_node, 
		     (*i)->m_cost + (*j)->m_cost);
    }
  }

  // Remove node and original edges
  return graph_remove_node_and_edges(so, node);
}

static PyObject* graph_remove_node(PyObject* self, PyObject* a) {
  GraphObject* so = ((GraphObject*)self);
  NodeObject* node;

  // Find the node
  node = graph_find_node(so, a);
  if (node == 0)
    return 0;
  return PyInt_FromLong((long)graph_remove_node(so, node));
}

static size_t graph_disj_set_find_and_compress(GraphObject* so, size_t x) {
  //LongVector* vec = (so->m_disj_set);
  long disj_set = (*(so->m_nodes))[x-1]->m_disj_set;
  if (disj_set <= 0)
    return (*(so->m_nodes))[x-1]->m_set_id;
  return ((*(so->m_nodes))[x-1]->m_disj_set = graph_disj_set_find_and_compress(so, disj_set));
}

void graph_disj_set_union_by_height(GraphObject* so, size_t a, size_t b) {
  //LongVector* vec = (so->m_disj_set);
  long disj_set_a = (*(so->m_nodes))[a-1]->m_disj_set;
  long disj_set_b = (*(so->m_nodes))[b-1]->m_disj_set;
  if (disj_set_b < disj_set_a)
    (*(so->m_nodes))[a-1]->m_disj_set = b;
  else {
    if (disj_set_a == disj_set_b)
      (*(so->m_nodes))[a-1]->m_disj_set--;
    (*(so->m_nodes))[b-1]->m_disj_set = a;
  }
}

// WARNING: Internal function: DO NOT CALL DIRECTLY
static bool graph_add_edge0(GraphObject* so, NodeObject* from_node,
			    NodeObject* to_node, CostType cost, PyObject* label,
			    bool check) {
  bool found_cycle = false;
  if (check) {
    bool possible_cycle = true;
    if (!so->m_cyclic) {
      size_t to_set_id = graph_disj_set_find_and_compress(so, to_node->m_set_id);
      size_t from_set_id = graph_disj_set_find_and_compress(so, from_node->m_set_id);
      if (from_set_id != to_set_id) {
	possible_cycle = false;
	graph_disj_set_union_by_height(so, to_set_id, from_set_id);
      }
    }

    if (possible_cycle) {
      if (!so->m_blob)
	return false;
      else {
	if (!so->m_cyclic || to_node->m_is_subgraph_root) {
	  DFSIterator* iterator = iterator_new_simple<DFSIterator>();
	  iterator->init(so, to_node);
	  NodeObject* node = (NodeObject*)DFSIterator::next(iterator);
	  while ((node = (NodeObject*)DFSIterator::next(iterator)))
	    if (node == from_node) {
	      found_cycle = true;
	      break;
	    }
	}
	if (!so->m_cyclic && found_cycle)
	  return false;
      }
    }
  }

  EdgeObject* edge = (EdgeObject*)edge_new_simple(so, from_node, to_node, cost, label);
  // so->m_edges->insert(edge);
  so->m_nedges++;
  from_node->m_out_edges->push_back(edge);
  to_node->m_in_edges->push_back(edge);
  if (check && !found_cycle && to_node->m_is_subgraph_root) {
    so->m_subgraph_roots->erase(to_node);
    to_node->m_is_subgraph_root = false;
  }

  return true;
}

static bool graph_add_edge(GraphObject* so, NodeObject* from_node,
			    NodeObject* to_node, CostType cost, PyObject* label) {
  if (!so->m_self_connected && from_node == to_node)
    return false;
  if (!so->m_multiconnected) {
    for (EdgeList::iterator i = from_node->m_out_edges->begin();
	 i != from_node->m_out_edges->end(); ++i)
      if ((*i)->m_to_node == to_node)
	return false;
  }
  bool result = graph_add_edge0(so, from_node, to_node, cost, label);
  if (!so->m_directed && result)
    graph_add_edge0(so, to_node, from_node, cost, label, false);
  return result;
}

static bool graph_add_edge(GraphObject* so, PyObject* from_pyobject,
			   PyObject* to_pyobject, CostType cost, PyObject* label) {
  NodeObject* from_node, *to_node;
  from_node = graph_add_node(so, from_pyobject);
  to_node = graph_add_node(so, to_pyobject);
  return graph_add_edge(so, from_node, to_node, cost);
}

static PyObject* graph_add_edge(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  PyObject* from_pyobject, *to_pyobject;
  CostType cost = 1.0;
  PyObject* label = NULL;
  if (PyArg_ParseTuple(args, "OO|dO", &from_pyobject, &to_pyobject, &cost, &label) <= 0)
    return 0;
  return PyInt_FromLong((long)graph_add_edge(so, from_pyobject, to_pyobject, cost, label));
}

static PyObject* graph_add_edges(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  PyObject* a;
  if (PyArg_ParseTuple(args, "O", &a) <= 0)
    return 0;
  if (!PyList_Check(a))
    PyErr_SetString(PyExc_TypeError, "Input must be a list of edge tuples.");
  size_t list_size = PyList_Size(a);
  for (size_t i = 0; i < list_size; ++i) {
    PyObject* tuple = PyList_GetItem(a, i);
    PyObject* from_node, *to_node;
    CostType cost = 1;
    PyObject* label = NULL;
    if (PyArg_ParseTuple(args, "OO|dO", &from_node, &to_node, &cost, &label) <= 0)
      return 0;
    graph_add_edge(so, (NodeObject*)from_node, (NodeObject*)to_node, cost, label);
  }
  Py_INCREF(Py_None);
  return Py_None;
}

// WARNING: This is an internal function that assumes the edge already exists
static bool graph_remove_edge(GraphObject* so, EdgeObject* edge) {
  NodeObject* from_node = edge->m_from_node;
  NodeObject* to_node = edge->m_to_node;
  if (!so->m_cyclic) {
    // O(nm)  (m: avg # edges per node)
    // Deal with subgraph roots and disjoint sets
    // I'd love to find a faster way to do this, but...
    if (to_node != from_node) {
      for (NodeVector::iterator i = so->m_nodes->begin();
	   i != so->m_nodes->end(); ++i)
	NP_VISITED(*i) = false;
      to_node->m_disj_set = 0;
      from_node->m_disj_set = 0;
      NodeStack node_stack;
      node_stack.push(to_node);
      node_stack.push(from_node);
      while (!node_stack.empty()) {
	NodeObject* root = node_stack.top();
	node_stack.pop();
	if (!NP_VISITED(root)) {
	  NP_VISITED(root) = true;
	  size_t new_set_id = root->m_set_id;
	  for (EdgeList::iterator j = root->m_out_edges->begin();
	       j != root->m_out_edges->end(); ++j) {
	    NodeObject* node = (*j)->m_to_node;
	    if (!(NP_VISITED(node))) {
	      NP_VISITED(node) = true;
	      node->m_disj_set = new_set_id;
	      node_stack.push(node);
	    }
	  }
	}
      }
    }
  }

  if (from_node->m_is_subgraph_root) {
    DFSIterator* iterator = iterator_new_simple<DFSIterator>();
    iterator->init(so, to_node);
    NodeObject* node = (NodeObject*)DFSIterator::next(iterator);
    while ((node = (NodeObject*)DFSIterator::next(iterator))) {
      if (node == from_node) {
	so->m_subgraph_roots->erase(from_node);
	from_node->m_is_subgraph_root = false;
	break;
      }
    }
  }

  from_node->m_out_edges->remove(edge);
  to_node->m_in_edges->remove(edge);

  if (!to_node->m_is_subgraph_root) {
    so->m_subgraph_roots->insert(to_node);
    to_node->m_is_subgraph_root = true;
  }

  so->m_nedges--;

  Py_DECREF((PyObject*)edge);
  return true;
}

// WARNING: This is an internal function that assumes the nodes (but not necessarily
//          the edge) already exist
static bool graph_remove_edge(GraphObject* so, NodeObject* from_node, NodeObject* to_node) {
  bool found_any = false;
  // Funny looping construct is so we can remove an edge and not
  // invalid the iterator
  for(EdgeList::iterator i, j = from_node->m_out_edges->begin();
      j != from_node->m_out_edges->end(); ) {
    std::cerr << ".";
    i = j++;
    if ((*i)->m_to_node == to_node) {
      graph_remove_edge(so, *i);
      found_any = true;
    }
  }

  if (!found_any) {
    PyErr_SetString(PyExc_TypeError, "There is no edge connecting these nodes");
    return false;
  }
  return true;
}

static PyObject* graph_remove_edge(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  PyObject* a = NULL;
  PyObject* b = NULL;
  bool result = false;
  if (PyArg_ParseTuple(args, "O|O", &a, &b) <= 0)
    return 0;
  if (b == NULL) {
    if (is_EdgeObject(a)) {
      if (graph_has_edge(so, ((EdgeObject*)a)->m_from_node, ((EdgeObject*)a)->m_to_node))
	return PyInt_FromLong((long)graph_remove_edge(so, (EdgeObject*)a));
      else {
	PyErr_SetString(PyExc_RuntimeError, "Given edge is not in the graph");
	return 0;
      }
    }
  } else {
    if (is_NodeObject(a)) {
      if (graph_has_node(so, (NodeObject*)a)) {
	if (is_NodeObject(b)) {
	  if (graph_has_node(so, (NodeObject*)b))
	    result = graph_remove_edge(so, (NodeObject*)a, (NodeObject*)b);
	  else {
	    PyErr_SetString(PyExc_RuntimeError, "Given 'to' node is not in the graph");
	    return 0;
	  }
	} else {
	  PyErr_SetString(PyExc_TypeError, "Invalid argument types");
	  return 0;
	}
      } else {
	PyErr_SetString(PyExc_RuntimeError, "Given 'from' node is not is the graph");
	return 0;
      }
    } else { 
      NodeObject *from_node, *to_node;
      from_node = graph_find_node(so, a);
      if (from_node == 0)
	return 0;
      to_node = graph_find_node(so, b);
      if (to_node == 0)
	return 0;
      result = graph_remove_edge(so, from_node, to_node);
    }
  }
  if (!result)
    return 0;
  return PyInt_FromLong((long)result);
}

static void graph_remove_all_edges(GraphObject* so) {
  for (NodeVector::iterator i = so->m_nodes->begin();
       i != so->m_nodes->end(); ++i) {
    for (EdgeList::iterator j = (*i)->m_out_edges->begin();
	 j != (*i)->m_out_edges->end(); ++j)
      Py_DECREF((PyObject*)*j);
    (*i)->m_out_edges->clear();
    (*i)->m_in_edges->clear();
    so->m_subgraph_roots->insert(*i);
    (*i)->m_is_subgraph_root = true; // EC
    (*i)->m_disj_set = 0;
  }
  so->m_nedges = 0;
}

static PyObject* graph_remove_all_edges(PyObject* self, PyObject* _) {
  GraphObject* so = ((GraphObject*)self);
  graph_remove_all_edges(so);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject* graph_is_directed(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  return PyInt_FromLong((long)so->m_directed);
}

static PyObject* graph_is_undirected(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  return PyInt_FromLong((long)!so->m_directed);
}

static void graph_make_directed(GraphObject* so) {
  so->m_directed = true;
}

static void graph_make_undirected(GraphObject* so) {
  if (so->m_directed) {
    so->m_directed = false;
    
    EdgeList edges;
    for (NodeVector::iterator i = so->m_nodes->begin();
	 i != so->m_nodes->end(); ++i)
      for (EdgeList::iterator j = (*i)->m_out_edges->begin();
	   j != (*i)->m_out_edges->end(); ++j)
	edges.push_back(*j);

    for (EdgeList::iterator i = edges.begin();
	 i != edges.end(); ++i)
      if (!graph_has_edge(so, (*i)->m_to_node, (*i)->m_from_node))
	graph_add_edge0(so, (*i)->m_to_node, (*i)->m_from_node,
			(*i)->m_cost, (*i)->m_label, false);
  }
}

static PyObject* graph_make_directed(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  int directed = 1;
  if (PyArg_ParseTuple(args, "|i", &directed) <= 0)
    return 0;
  if (directed)
    graph_make_directed(so);
  else
    graph_make_undirected(so);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject* graph_make_undirected(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  graph_make_undirected(so);
  Py_INCREF(Py_None);
  return Py_None;
}  

static PyObject* graph_is_cyclic(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  return PyInt_FromLong((long)so->m_cyclic);
}

static PyObject* graph_is_acyclic(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  return PyInt_FromLong((long)!so->m_cyclic);
}

static void graph_make_cyclic(GraphObject* so) {
  so->m_cyclic = true;
}

static void graph_make_acyclic(GraphObject* so) {
  if (so->m_cyclic) {
    graph_make_not_self_connected(so);
    graph_make_singly_connected(so);
    if (so->m_nedges) {
      for (NodeVector::iterator i = so->m_nodes->begin();
	   i != so->m_nodes->end(); ++i)
	NP_VISITED(*i) = false;
      for (NodeSet::iterator i = so->m_subgraph_roots->begin();
	   i != so->m_subgraph_roots->end(); ++i) {
	NodeStack node_stack;
	node_stack.push(*i);
	while (!node_stack.empty()) {
	  NodeObject* node = node_stack.top();
	  node_stack.pop();
	  NP_VISITED(node) = true;
	  for (EdgeList::iterator k, j = node->m_out_edges->begin();
	       j != node->m_out_edges->end();) {
	    k = j++;
	    if (NP_VISITED((*k)->m_to_node))
	      graph_remove_edge(so, *k);
	    else
	      node_stack.push((*k)->m_to_node);
	  }
	}
      }
    }
    so->m_cyclic = false;
  }
}

static PyObject* graph_make_cyclic(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  int cyclic = 1;
  if (PyArg_ParseTuple(args, "|i", &cyclic) <= 0)
    return 0;
  if (cyclic)
    graph_make_cyclic(so);
  else
    graph_make_acyclic(so);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject* graph_make_acyclic(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  graph_make_acyclic(so);
  Py_INCREF(Py_None);
  return Py_None;
}  

static PyObject* graph_is_tree(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  return PyInt_FromLong((long)!so->m_blob);
}

static PyObject* graph_is_blob(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  return PyInt_FromLong((long)so->m_blob);
}

static void graph_make_tree(GraphObject* so) {
  if (so->m_blob) {
    so->m_blob = false;
    graph_make_acyclic(so);
    graph_make_undirected(so);
  }
}

static void graph_make_blob(GraphObject* so) {
  so->m_blob = true;
}

static PyObject* graph_make_tree(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  graph_make_tree(so);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject* graph_make_blob(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  graph_make_blob(so);
  Py_INCREF(Py_None);
  return Py_None;
}  

static PyObject* graph_is_multi_connected(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  return PyInt_FromLong((long)so->m_multiconnected);
}

static PyObject* graph_is_singly_connected(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  return PyInt_FromLong((long)!so->m_multiconnected);
}

static void graph_make_multi_connected(GraphObject* so) {
  so->m_multiconnected = true;
}

static void graph_make_singly_connected(GraphObject* so, bool maximum_cost) {
  if (so->m_multiconnected) {
    if (so->m_nedges) {
      for (NodeVector::iterator i = so->m_nodes->begin();
	   i != so->m_nodes->end(); ++i) {
	NodeToEdgeMap node_map;
	for (EdgeList::iterator j, j0 = (*i)->m_out_edges->begin();
	     j0 != (*i)->m_out_edges->end(); ) {
	  j = j0++;
	  NodeToEdgeMap::iterator l = node_map.find((*j)->m_to_node);
	  if (l == node_map.end())
	    node_map[(*j)->m_to_node] = *j;
	  for (EdgeList::iterator k, k0 = j;
	       k0 != (*i)->m_out_edges->end();) {
	    k = k0++;
	    if (*j == *k)
	      continue;
	    if ((*j)->m_to_node == (*k)->m_to_node) {
	      if (maximum_cost) {
		if (node_map[(*j)->m_to_node]->m_cost > (*k)->m_cost)
		  graph_remove_edge(so, *k);
		else {
		  graph_remove_edge(so, node_map[(*j)->m_to_node]);
		  node_map[(*j)->m_to_node] = *k;
		}
	      } else {
		if (node_map[(*j)->m_to_node]->m_cost < (*k)->m_cost)
		  graph_remove_edge(so, *k);
		else {
		  graph_remove_edge(so, node_map[(*j)->m_to_node]);
		  node_map[(*j)->m_to_node] = *k;
		}
	      }
	    }
	  }
	}
      }
    }
    so->m_multiconnected = false;
  }
}

static PyObject* graph_make_multi_connected(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  int multi_connected = 1, maximum = 1;
  if (PyArg_ParseTuple(args, "|ii", &multi_connected, &maximum) <= 0)
    return 0;
  if (multi_connected)
    graph_make_multi_connected(so);
  else
    graph_make_singly_connected(so, maximum != 0);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject* graph_make_singly_connected(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  int maximum = 1;
  if (PyArg_ParseTuple(args, "|i", &maximum) <= 0)
    return 0;
  graph_make_singly_connected(so, maximum != 0);
  Py_INCREF(Py_None);
  return Py_None;
}  

static PyObject* graph_is_self_connected(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  return PyInt_FromLong((long)so->m_multiconnected);
}

static void graph_make_self_connected(GraphObject* so) {
  so->m_self_connected = true;
}

static void graph_make_not_self_connected(GraphObject* so) {
  if (so->m_self_connected) {
    if (so->m_nedges) {
      EdgeList removals;
      for (NodeVector::iterator i = so->m_nodes->begin();
	   i != so->m_nodes->end(); ++i)
	for (EdgeList::iterator j, j0 = (*i)->m_out_edges->begin();
	     j0 != (*i)->m_out_edges->end();) {
	  j = j0++;
	  if ((*j)->m_to_node == (*i))
	    graph_remove_edge(so, *j);
	}
    }
    so->m_self_connected = false;
  }
}

static PyObject* graph_make_self_connected(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  int self_connected = 1;
  if (PyArg_ParseTuple(args, "|i", &self_connected) <= 0)
    return 0;
  if (self_connected)
    graph_make_self_connected(so);
  else
    graph_make_not_self_connected(so);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject* graph_make_not_self_connected(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  graph_make_not_self_connected(so);
  Py_INCREF(Py_None);
  return Py_None;
}  

// Algorithms

static PyObject* graph_get_nodes(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  typedef BasicIterator<NodeVector> NodeVectorIterator;
  NodeVectorIterator* iterator = iterator_new_simple<NodeVectorIterator>();
  iterator->init(so->m_nodes->begin(), so->m_nodes->end());
  return (PyObject*)iterator;
}

static PyObject* graph_get_node(PyObject* self, PyObject* pyobject) {
  GraphObject* so = ((GraphObject*)self);
  NodeObject* node = graph_find_node(so, pyobject);
  Py_INCREF((PyObject*)node);
  return (PyObject*)node;
}

bool graph_has_node(GraphObject* so, NodeObject* node) {
  for (NodeVector::iterator i = so->m_nodes->begin();
       i != so->m_nodes->end(); ++i)
    if ((*i) == node)
      return true;
  return false;
}

static PyObject* graph_has_node(PyObject* self, PyObject* a) {
  GraphObject* so = (GraphObject*)self;
  NodeObject* node;
  node = graph_find_node(so, a, false);
  if (node == 0)
    return PyInt_FromLong((long)0);
  return PyInt_FromLong((long)1);
}

static PyObject* graph_get_nnodes(PyObject* self) {
  GraphObject* so = (GraphObject*)self;
  return PyInt_FromLong((long)so->m_nodes->size());
}

struct EdgeIterator : IteratorObject {
  int init(NodeVector::iterator begin, NodeVector::iterator end) {
    m_it = begin;
    m_end = end;
    m_edge_it = (*begin)->m_out_edges->begin();
    m_edge_end = (*begin)->m_out_edges->end();
    return 1;
  }
  static PyObject* next(IteratorObject* self) {
    EdgeIterator* so = (EdgeIterator*)self;
    while (so->m_edge_it == so->m_edge_end) {
      so->m_it++;
      if (so->m_it == so->m_end) {
	return 0;
      }
      so->m_edge_it = (*(so->m_it))->m_out_edges->begin();
      so->m_edge_end = (*(so->m_it))->m_out_edges->end();
    }
    return (PyObject*)*((so->m_edge_it)++);
  }
  NodeVector::iterator m_it, m_end;
  EdgeList::iterator m_edge_it, m_edge_end;
};


static PyObject* graph_get_edges(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  EdgeIterator* iterator = iterator_new_simple<EdgeIterator>();
  iterator->init(so->m_nodes->begin(), so->m_nodes->end());
  return (PyObject*)iterator;
}

static size_t graph_has_edge(GraphObject* so, NodeObject* from_node, NodeObject* to_node) {
  if (!graph_has_node(so, to_node)) {
    return 0;
  }
  if (!graph_has_node(so, from_node)) {
    return 0;
  }
  for (EdgeList::iterator i = from_node->m_out_edges->begin();
       i != from_node->m_out_edges->end(); ++i)
    if ((*i)->m_to_node == to_node)
      return true;
  return false;
}

static PyObject* graph_has_edge(PyObject* self, PyObject* args) {
  GraphObject* so = (GraphObject*)self;
  PyObject *a = NULL, *b = NULL;
  if (PyArg_ParseTuple(args, "O|O", &a, &b) <= 0)
    return 0;
  if (is_EdgeObject(a) && b == NULL) {
    EdgeObject *edge = (EdgeObject*)a;
    return PyInt_FromLong((long)graph_has_edge(so, edge->m_from_node, edge->m_to_node));
  }
  if (is_NodeObject(a) && is_NodeObject(b)) {
    NodeObject *from_node = (NodeObject*)a;
    NodeObject *to_node = (NodeObject*)b;
    return PyInt_FromLong((long)graph_has_edge(so, from_node, to_node));
  }
  if (!is_NodeObject(b) && !is_NodeObject(b) && !is_EdgeObject(a) && !is_EdgeObject(b)) {
    NodeObject *from_node = graph_find_node(so, a, false);
    if (from_node == 0)
      return PyInt_FromLong(0);
    NodeObject *to_node = graph_find_node(so, b, false);
    if (to_node == 0)
      return PyInt_FromLong(0);
    return PyInt_FromLong((long)graph_has_edge(so, from_node, to_node));
  }    
  PyErr_SetString(PyExc_TypeError, "Invalid argument types");
  return 0;
}

static PyObject* graph_get_nedges(PyObject* self) {
  GraphObject* so = (GraphObject*)self;
  return PyInt_FromLong((long)so->m_nedges);
}

static PyObject* graph_BFS(PyObject* self, PyObject* pyobject) {
  GraphObject* so = ((GraphObject*)self);
  NodeObject* root;
  root = graph_find_node(so, pyobject);
  if (root == 0)
    return 0;
  BFSIterator* iterator = iterator_new_simple<BFSIterator>();
  iterator->init(so, root);
  return (PyObject*)iterator;
}

static PyObject* graph_DFS(PyObject* self, PyObject* pyobject) {
  GraphObject* so = ((GraphObject*)self);
  NodeObject* root;
  root = graph_find_node(so, pyobject);
  if (root == 0)
    return 0;
  DFSIterator* iterator = iterator_new_simple<DFSIterator>();
  iterator->init(so, root);
  return (PyObject*)iterator;
}

static PyObject* graph_get_subgraph_roots(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  typedef BasicIterator<NodeSet> NodeSetIterator;
  NodeSetIterator* iterator = iterator_new_simple<NodeSetIterator>();
  iterator->init(so->m_subgraph_roots->begin(), so->m_subgraph_roots->end());
  return (PyObject*)iterator;
}

static size_t graph_size_of_subgraph(GraphObject* so, NodeObject* root) {
  size_t count = 0;
  DFSIterator* iterator = iterator_new_simple<DFSIterator>();
  iterator->init(so, root);
  while (DFSIterator::next(iterator))
    ++count;
  return count;
}

static PyObject* graph_size_of_subgraph(PyObject* self, PyObject* pyobject) {
  GraphObject* so = ((GraphObject*)self);
  NodeObject* root;
  root = graph_find_node(so, pyobject);
  if (root == 0)
    return 0;
  return PyInt_FromLong((long)graph_size_of_subgraph(so, root));
}

static PyObject* graph_get_nsubgraphs(PyObject* self) {
  GraphObject* so = (GraphObject*)self;
  return PyInt_FromLong((long)so->m_subgraph_roots->size());
}

static PyObject* graph_is_fully_connected(PyObject* self, PyObject* _) {
  GraphObject* so = (GraphObject*)self;
  bool result = false;
  result = so->m_subgraph_roots->size() == 1;
  return PyInt_FromLong((long)result);
}

static GraphObject* graph_create_spanning_tree(GraphObject* so, NodeObject* root) {
  GraphObject* tree = graph_new_simple(1, 0, 0);
  NodeVector::iterator i = so->m_nodes->begin();
  for (; i != so->m_nodes->end(); ++i)
    NP_VISITED(*i) = false;
  NodeStack node_stack;
  node_stack.push(root);
  while (!node_stack.empty()) {
    NodeObject* node = node_stack.top();
    node_stack.pop();
    NP_VISITED(node) = true;
    NodeObject* new_node1 = graph_add_node(tree, node->m_data);
    for (EdgeList::iterator j = node->m_out_edges->begin();
	 j != node->m_out_edges->end(); ++j)
      if (!NP_VISITED((*j)->m_to_node)) {
	NodeObject* new_node2 = graph_add_node(tree, (*j)->m_to_node->m_data);
	graph_add_edge(tree, new_node1, new_node2, (*j)->m_cost);
	node_stack.push((*j)->m_to_node);
      }
  }
  return tree;
}

static PyObject* graph_create_spanning_tree(PyObject* self, PyObject* pyobject) {
  GraphObject* so = ((GraphObject*)self);
  NodeObject* root;
  root = graph_find_node(so, pyobject);
  if (root == NULL)
    return 0;
  return (PyObject*)graph_create_spanning_tree(so, root);
}

struct PartitionIterator : IteratorObject {
  int init(GraphObject* graph, NodeObject* root) {
    m_count = 0;
    m_subgraph = new NodeList();

    for (NodeVector::iterator i = graph->m_nodes->begin();
	 i != graph->m_nodes->end(); ++i) {
      NP_VISITED(*i) = false;
      for (EdgeList::iterator j = (*i)->m_out_edges->begin();
	   j != (*i)->m_out_edges->end(); ++j)
	EP_VISITED(*j) = false;
    }

    NodeStack node_stack;
    node_stack.push(root);
    NP_VISITED(root) = true;
    while (!node_stack.empty()) {
      NodeObject* node;
      node = node_stack.top();
      node_stack.pop();
      m_count++;
      m_subgraph->push_back(node);
      
      for (EdgeList::iterator j = node->m_out_edges->begin();
	   j != node->m_out_edges->end(); ++j)
	if (!NP_VISITED((*j)->m_to_node)) {
	  node_stack.push((*j)->m_to_node);
	  NP_VISITED((*j)->m_to_node) = true;
	  EP_VISITED(*j) = true;
	  EP_PARTITION_COUNTER(*j) = false;
	}
    }
    m_i = 0;
    m_partitions = (size_t)pow(2, m_count - 1);
    m_root = root;
    return 1;
  }

  static PyObject* next(IteratorObject* self) {
    PartitionIterator* so = (PartitionIterator*)so;
    if (so->m_i >= so->m_partitions)
      return 0;

    bool carry = true;
    NodeStack outer_node_stack;
    NodeStack inner_node_stack;
    PyObject* result = PyList_New(0);
    outer_node_stack.push(so->m_root);
    while (!outer_node_stack.empty()) {
      PyObject* subresult = PyList_New(0);
      PyList_Append(result, subresult);
      NodeObject* root = outer_node_stack.top();
      outer_node_stack.pop();
      inner_node_stack.push(root);
      while (!inner_node_stack.empty()) {
	root = inner_node_stack.top();
	inner_node_stack.pop();
	for (EdgeList::iterator j = root->m_out_edges->begin();
	     j != root->m_out_edges->end(); ++j) {
	  if (EP_VISITED(*j)) {
	    if (EP_PARTITION_COUNTER(*j)) {
	      inner_node_stack.push((*j)->m_to_node);
	      if (carry)
		EP_PARTITION_COUNTER(*j) = false;
	    } else {
	      outer_node_stack.push((*j)->m_to_node);
	      if (carry) {
		EP_PARTITION_COUNTER(*j) = true;
		carry = false;
	      }
	    }
	  }
	}
	PyList_Append(subresult, root->m_data);
	// Py_DECREF((PyObject*)root);
      }
      Py_DECREF(subresult);
    }
    so->m_i++;
    return result;
  }
  size_t m_count;
  size_t m_partitions;
  size_t m_i;
  NodeObject* m_root;
  NodeList* m_subgraph;
};

static PyObject* graph_partitions(PyObject* self, PyObject* pyobject) {
  GraphObject* so = ((GraphObject*)self);
  NodeObject* root;
  root = graph_find_node(so, pyobject);
  if (root == NULL)
    return 0;
  PartitionIterator* iterator = iterator_new_simple<PartitionIterator>();
  iterator->init(so, root);
  return (PyObject*)iterator;
}

struct djikstra_queue_comp_func
{
  bool operator()(NodeObject* const& a, NodeObject* const& b) const { 
    return NP_DISTANCE(a) > NP_DISTANCE(b);
  }
};

static NodeList* graph_djikstra_shortest_path(GraphObject* so, NodeObject* root) {
  typedef std::priority_queue<NodeObject*, std::vector<NodeObject*>, djikstra_queue_comp_func> NodeQueue;
  NodeList* main_node_list = new NodeList();

  if (so->m_cyclic) {
    DFSIterator* iterator = iterator_new_simple<DFSIterator >();
    iterator->init(so, root);
    NodeObject* node;
    while ((node = (NodeObject*)DFSIterator::next(iterator))) {
      NP_KNOWN(node) = false;
      NP_DISTANCE(node) = std::numeric_limits<CostType>::max();
      NP_PATH(node) = NULL;
      main_node_list->push_back(node);
    }
    NP_DISTANCE(root) = 0;
    NodeQueue node_queue;
    node_queue.push(root);

    while(!node_queue.empty()) {
      NodeObject* node = node_queue.top();
      node_queue.pop();
      if (!NP_KNOWN(node)) {
	NP_KNOWN(node) = true;
	for (EdgeList::iterator i = node->m_out_edges->begin();
	     i != node->m_out_edges->end(); ++i) {
	  NodeObject* other_node = (*i)->m_to_node;
	  if (!NP_KNOWN(other_node)) {
	    if (NP_DISTANCE(node) + (*i)->m_cost < NP_DISTANCE(other_node)) {
	      NP_DISTANCE(other_node) = NP_DISTANCE(node) + (*i)->m_cost;
	      NP_PATH(other_node) = node;
	      node_queue.push(other_node);
	    }
	  }
	}
      }
    }
  } else { // acyclic version
    DFSIterator* iterator = iterator_new_simple<DFSIterator >();
    iterator->init(so, root);
    NodeObject* node;
    while ((node = (NodeObject*)DFSIterator::next(iterator))) {
      NP_DISTANCE(node) = std::numeric_limits<CostType>::max();
      NP_PATH(node) = NULL;
      main_node_list->push_back(node);
    }
    NP_DISTANCE(root) = 0;
    NP_PATH(root) = NULL;
    NodeStack node_stack;
    node_stack.push(root);
    while (!node_stack.empty()) {
      NodeObject* node = node_stack.top();
      node_stack.pop();
      main_node_list->push_back(node);
      for (EdgeList::iterator i = node->m_out_edges->begin();
	   i != node->m_out_edges->end(); ++i) {
	if (NP_DISTANCE(node) + (*i)->m_cost < NP_DISTANCE((*i)->m_to_node)) {
	  node_stack.push((*i)->m_to_node);
	  NP_DISTANCE((*i)->m_to_node) = NP_DISTANCE(node) + (*i)->m_cost;
	  NP_PATH((*i)->m_to_node) = node;
	}
      }
    }
  }

  return main_node_list;
}

static PyObject* graph_djikstra_shortest_path(PyObject* self, PyObject* pyobject) {
  GraphObject* so = ((GraphObject*)self);
  NodeObject* root;
  root = graph_find_node(so, pyobject);
  if (root == 0)
    return 0;
  NodeList* node_list = graph_djikstra_shortest_path(so, root);

  PyObject* result = PyDict_New();
  for (NodeList::iterator i = node_list->begin();
       i != node_list->end(); ++i) {
    NodeObject* node = *i;
    PyObject* tuple = PyTuple_New(2);
    PyTuple_SetItem(tuple, 0, PyFloat_FromDouble(NP_DISTANCE(node)));
    PyObject* path = PyList_New(0);
    NodeObject* subnode = node;
    while (NP_PATH(subnode) != NULL) {
      PyList_Insert(path, 0, subnode->m_data);
      subnode = NP_PATH(subnode);
    }
    PyTuple_SetItem(tuple, 1, path);
    PyDict_SetItem(result, node->m_data, tuple);
    Py_DECREF(tuple);
  }

  return result;
}

static PyObject* graph_djikstra_all_pairs_shortest_path(PyObject* self, PyObject* pyobject) {
  GraphObject* so = ((GraphObject*)self);
  
  PyObject* result = PyDict_New();
  for (NodeVector::iterator i = so->m_nodes->begin();
       i != so->m_nodes->end(); ++i) {
    NodeList* node_list = graph_djikstra_shortest_path(so, *i);
    PyObject* subresult = PyDict_New();
    for (NodeList::iterator j = node_list->begin();
	 j != node_list->end(); ++j) {
      NodeObject* node = *j;
      PyObject* tuple = PyTuple_New(2);
      PyTuple_SetItem(tuple, 0, PyFloat_FromDouble(NP_DISTANCE(node)));
      PyObject* path = PyList_New(0);
      NodeObject* subnode = node;
      while (NP_PATH(subnode) != NULL) {
	PyList_Insert(path, 0, subnode->m_data);
	subnode = NP_PATH(subnode);
      }
      PyTuple_SetItem(tuple, 1, path);
      PyDict_SetItem(subresult, node->m_data, tuple);
      Py_DECREF(tuple);
    }
    PyDict_SetItem(result, (*i)->m_data, subresult);
    Py_DECREF(subresult);
  }

  return result;
}

static PyObject* graph_all_pairs_shortest_path(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);

  size_t size = so->m_nodes->size() + 1;
  std::vector<CostType> distances(size * size);
  std::vector<size_t> paths(size * size);
  for (size_t i = 0; i < size; ++i)
    for (size_t j = 0; j < size; ++j) {
      distances[i * size + j] = std::numeric_limits<CostType>::max();
      paths[i * size + j] = 0;
    }

  // Initialize distances based on edges
  for (NodeVector::iterator i = so->m_nodes->begin();
       i != so->m_nodes->end(); ++i) {
    size_t row_index = (*i)->m_set_id * size;
    for (EdgeList::iterator j = (*i)->m_out_edges->begin();
	 j != (*i)->m_out_edges->end(); ++j) {
      size_t index = row_index + (*j)->m_to_node->m_set_id;
      distances[index] = (*j)->m_cost;
      paths[index] = (*i)->m_set_id;
    }
  }

  // Create a vector of set_id -> NodeObject*
  for (NodeVector::iterator i = so->m_nodes->begin();
       i != so->m_nodes->end(); ++i) {
    distances[(*i)->m_set_id * size + (*i)->m_set_id] = 0;
  }

  // The main loop
  for (size_t k = 1; k < size; ++k) {
    size_t k_row = k * size;
    for (size_t i = 1; i < size; ++i) {
      size_t i_row = i * size;
      size_t a = i_row + k;
      for (size_t j = 1; j < size; ++j) {
	size_t b = k_row + j;
	size_t c = i_row + j;
	if (distances[a] + distances[b] < distances[c]) {
	  distances[c] = distances[a] + distances[b];
	  paths[c] = k;
	}
      }
    }
  }

  PyObject* result = PyDict_New();
  for (NodeVector::iterator i = so->m_nodes->begin();
       i != so->m_nodes->end(); ++i) {
    size_t i_id = (*i)->m_set_id;
    PyObject* subresult = PyDict_New();
    for (NodeVector::iterator j = so->m_nodes->begin();
	 j != so->m_nodes->end(); ++j) {
      size_t j_id = (*j)->m_set_id;
      CostType distance = distances[i_id * size + j_id];
      if (distance < std::numeric_limits<CostType>::max()) {
	PyObject* tuple = PyTuple_New(2);
	PyTuple_SetItem(tuple, 0, PyFloat_FromDouble(distance));
	PyObject* path = PyList_New(0);
	while (paths[i_id * size + j_id] != 0) {
	  PyList_Insert(path, 0, (*(so->m_nodes))[j_id - 1]->m_data);
	  j_id = paths[i_id * size + j_id];
	}
	PyTuple_SetItem(tuple, 1, path);
	PyDict_SetItem(subresult, (*j)->m_data, tuple);
	Py_DECREF(tuple);
      }
    }
    PyDict_SetItem(result, (*i)->m_data, subresult);
    Py_DECREF(subresult);
  }

  return result;
}

struct minimum_spanning_queue_comp_func
{
  bool operator()(EdgeObject* const& a, EdgeObject* const& b) const { 
    return a->m_cost > b->m_cost;
  }
};

void graph_minimum_spanning_tree(GraphObject* so) {
  // Kruskal's algorithm
  typedef std::priority_queue<EdgeObject*, std::vector<EdgeObject*>,
    minimum_spanning_queue_comp_func> EdgeQueue;
  EdgeQueue edge_queue;
  
  for (NodeVector::iterator i = so->m_nodes->begin();
       i != so->m_nodes->end(); ++i)
    for (EdgeList::iterator j = (*i)->m_out_edges->begin();
	 j != (*i)->m_out_edges->end(); ++j) {
      edge_queue.push(*j);
      // Increase reference count, since we're about to delete all edges
      Py_INCREF((PyObject*)(*j));
    }
  
  graph_remove_all_edges(so);
  graph_make_tree(so);

  while (!edge_queue.empty() && so->m_nedges < so->m_nodes->size() - 1) {
    EdgeObject* edge = edge_queue.top();
    edge_queue.pop();
    graph_add_edge(so, edge->m_from_node, edge->m_to_node, edge->m_cost, edge->m_label);
    Py_DECREF((PyObject*)edge);
 }
}

PyObject* graph_minimum_spanning_tree(PyObject* self, PyObject* _) {
  GraphObject* so = ((GraphObject*)self);
  graph_minimum_spanning_tree(so);
  Py_INCREF(Py_None);
  return Py_None;
}

// Factory functions

static PyObject* new_Tree(PyObject* self, PyObject* args) {
  PyObject *a = NULL;
  if (PyArg_ParseTuple(args, "|O", &a) <= 0)
    return 0;
  if (a == NULL)
    return (PyObject*)graph_new_simple(false, false, false);
  if (is_GraphObject(a))
    return (PyObject*)graph_copy((GraphObject*)a, false, false, false);
  PyErr_SetString(PyExc_TypeError, "Invalid argument type (must be Graph)");
  return 0;
}

static PyObject* new_FreeGraph(PyObject* self, PyObject* args) {
  PyObject *a = NULL;
  if (PyArg_ParseTuple(args, "|O", &a) <= 0)
    return 0;
  if (a == NULL)
    return (PyObject*)graph_new_simple();
  if (is_GraphObject(a))
    return (PyObject*)graph_copy((GraphObject*)a);
  PyErr_SetString(PyExc_TypeError, "Invalid argument type (must be Graph)");
  return 0;
}

static PyObject* new_DAG(PyObject* self, PyObject* args) {
  PyObject *a = NULL;
  if (PyArg_ParseTuple(args, "|O", &a) <= 0)
    return 0;
  if (a == NULL)
    return (PyObject*)graph_new_simple(true, false);
  if (is_GraphObject(a))
    return (PyObject*)graph_copy((GraphObject*)a, true, false);
  PyErr_SetString(PyExc_TypeError, "Invalid argument type (must be Graph)");
  return 0;
}

PyMethodDef graph_module_methods[] = {
  { "Tree", new_Tree, METH_VARARGS,
    "Create a new Tree" },
  { "FreeGraph", new_FreeGraph, METH_VARARGS,
    "Create a new freeform Graph" },
  { "DAG", new_DAG, METH_VARARGS,
    "Create a new directed acyclic graph" },
  {NULL}
};

DL_EXPORT(void) initgraph(void) {
  PyObject* m = Py_InitModule("gamera.graph", graph_module_methods);
  PyObject* d = PyModule_GetDict(m);

  IteratorType.ob_type = &PyType_Type;
  IteratorType.tp_name = "gamera.graph.Iterator";
  IteratorType.tp_basicsize = sizeof(IteratorObject);
  IteratorType.tp_dealloc = iterator_dealloc;
  IteratorType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  // IteratorType.tp_new = iterator_new;
  IteratorType.tp_getattro = PyObject_GenericGetAttr;
  IteratorType.tp_alloc = PyType_GenericAlloc;
  IteratorType.tp_free = _PyObject_Del;
  IteratorType.tp_iter = iterator_get_iter;
  IteratorType.tp_iternext = iterator_next;
  PyType_Ready(&IteratorType);

  NodeType.ob_type = &PyType_Type;
  NodeType.tp_name = "gamera.graph.Node";
  NodeType.tp_basicsize = sizeof(NodeObject);
  NodeType.tp_dealloc = node_dealloc;
  NodeType.tp_repr = node___repr__;
  NodeType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  // NodeType.tp_new = node_new;
  NodeType.tp_getattro = PyObject_GenericGetAttr;
  NodeType.tp_alloc = PyType_GenericAlloc;
  NodeType.tp_free = _PyObject_Del;
  NodeType.tp_methods = node_methods;
  NodeType.tp_getset = node_getset;
  NodeType.tp_call = node___call__;
  PyType_Ready(&NodeType);
  // PyDict_SetItemString(d, "Node", (PyObject*)&NodeType);

  EdgeType.ob_type = &PyType_Type;
  EdgeType.tp_name = "gamera.graph.Edge";
  EdgeType.tp_basicsize = sizeof(EdgeObject);
  EdgeType.tp_dealloc = edge_dealloc;
  EdgeType.tp_repr = edge___repr__;
  EdgeType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  // EdgeType.tp_new = edge_new;
  EdgeType.tp_getattro = PyObject_GenericGetAttr;
  EdgeType.tp_alloc = PyType_GenericAlloc;
  EdgeType.tp_free = _PyObject_Del;
  EdgeType.tp_call = edge___call__;
  EdgeType.tp_methods = edge_methods;
  EdgeType.tp_getset = edge_getset;
  PyType_Ready(&EdgeType);
  // PyDict_SetItemString(d, "Edge", (PyObject*)&EdgeType);

  GraphType.ob_type = &PyType_Type;
  GraphType.tp_name = "gamera.graph.Graph";
  GraphType.tp_basicsize = sizeof(GraphObject);
  GraphType.tp_dealloc = graph_dealloc;
  GraphType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  GraphType.tp_new = graph_new;
  GraphType.tp_getattro = PyObject_GenericGetAttr;
  GraphType.tp_alloc = PyType_GenericAlloc;
  GraphType.tp_free = _PyObject_Del;
  GraphType.tp_methods = graph_methods;
  GraphType.tp_getset = graph_getset;
  PyType_Ready(&GraphType);
  PyDict_SetItemString(d, "Graph", (PyObject*)&GraphType);

  // Copied from the plugin code
  PyObject* mod = PyImport_ImportModule("gamera.core");
  if (mod == 0) {
    printf("Could not load gamera.py\n");
    return;
  }
  PyObject* dict = PyModule_GetDict(mod);
  if (dict == 0) {
    PyErr_SetString(PyExc_RuntimeError, "Unable to get module dictionary\n");
    return;
  }
  cc_type = (PyTypeObject*)PyDict_GetItemString(dict, "Cc");
  mod = PyImport_ImportModule("gamera.gameracore");
  if (mod == 0) {
    PyErr_SetString(PyExc_RuntimeError, "Unable to load gameracore.\n");
    return;
  }
  dict = PyModule_GetDict(mod);
  if (dict == 0) {
    PyErr_SetString(PyExc_RuntimeError, "Unable to get module dictionary\n");
    return;
  }
  imagebase_type = (PyTypeObject*)PyDict_GetItemString(dict, "Image");
}

