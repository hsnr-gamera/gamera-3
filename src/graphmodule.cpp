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
#include <exception>

// Forward references
struct NodeObject;
struct EdgeObject;
struct GraphObject;

typedef std::set<NodeObject*> NodeSet;
typedef std::set<EdgeObject*> EdgeSet;

extern "C" {
  DL_EXPORT(void) initgraph(void);

  static PyObject* iterator_get_iter(PyObject* self);
  static PyObject* iterator_next(PyObject* self);

  static PyObject* node_new(PyTypeObject* pytype, PyObject* args,
			     PyObject* kwds);
  static PyObject* node_new_simple(PyObject *data);
  static void node_dealloc(PyObject* self);

  static PyObject* edge_new(PyTypeObject* pytype, PyObject* args,
			     PyObject* kwds);
  static PyObject* edge_new_simple(NodeObject* from_node, NodeObject* to_node,
				   double cost);
  static void edge_dealloc(PyObject* self);
  static PyObject* edge___repr__(PyObject* self);
  static PyObject* edge_get_from_node(PyObject* self);
  static PyObject* edge_get_to_node(PyObject* self);
  static PyObject* edge_get_cost(PyObject* self);
  static int edge_set_cost(PyObject* self, PyObject* cost);

  static PyObject* graph_new(PyTypeObject* pytype, PyObject* args,
			     PyObject* kwds);
  static void graph_dealloc(PyObject* self);
  PyObject* graph_add_node(PyObject* self, PyObject* pyobject);
  PyObject* graph_remove_node(PyObject* self, PyObject* pyobject);
  PyObject* graph_add_edge(PyObject* self, PyObject* args);
  PyObject* graph_remove_edge(PyObject* self, PyObject* args);
  PyObject* graph_get_nodes(PyObject* self, PyObject* args);
  PyObject* graph_get_edges(PyObject* self, PyObject* args);
}

// Iterators

static PyTypeObject IteratorType = {
  PyObject_HEAD_INIT(NULL)
  0,
};

struct IteratorObject {
  PyObject_HEAD
  PyObject*(*m_next_iter_function)(IteratorObject*, GraphObject*);
  GraphObject* m_graph;
};

template<class T>
static PyObject* iterator_new_simple(GraphObject* graph) {
  IteratorObject* so;
  IteratorType.tp_basicsize = sizeof(T);
  so = (IteratorObject*)(IteratorType.tp_alloc(&IteratorType, 0));
  so->m_graph = graph;
  so->m_next_iter_function = T::next_iter;
  if (((T*)(so))->init_iter(graph) <= 0)
    PyErr_SetString(PyExc_TypeError, "iterator: could not initialize");
  return (PyObject*)so;
}

static PyObject* iterator_get_iter(PyObject* self) {
  Py_INCREF(self);
  return self;
}

static PyObject* iterator_next(PyObject* self) {
  IteratorObject* so = (IteratorObject*)self;
  PyObject* result = (*(so->m_next_iter_function))(so, so->m_graph);
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

struct NodeObject {
  PyObject_HEAD
  PyObject* m_data;
  EdgeSet* m_edges;
};

extern PyTypeObject* get_NodeType() {
  return &NodeType;
}

PyMethodDef node_methods[] = {
  { NULL }
};

static PyObject* node_new(PyTypeObject* pytype, PyObject* args, 
			  PyObject* kwds) {
  PyObject* data;
  if (PyArg_ParseTuple(args, "O", &data) <= 0)
    return 0;
  return node_new_simple(data);
}

static PyObject* node_new_simple(PyObject* data) {
  NodeObject* so;
  so = (NodeObject*)(NodeType.tp_alloc(&NodeType, 0));
  Py_INCREF(data);
  so->m_data = data;
  so->m_edges = new EdgeSet();
  return (PyObject*)so;
}

static void node_dealloc(PyObject* self) {
  std::cerr << "node_dealloc\n";
  NodeObject* x = (NodeObject*)self;
  Py_DECREF((PyObject*)x->m_data);
  self->ob_type->tp_free(self);
}

PyObject* node___repr__(PyObject* self) {
  NodeObject* so = (NodeObject*)self;
  return PyString_FromFormat("<Node of %s>", 
			     PyString_AsString(PyObject_Repr(so->m_data)));
}

// Edge

static PyTypeObject EdgeType = {
  PyObject_HEAD_INIT(NULL)
  0,
};

extern PyTypeObject* get_EdgeType() {
  return &EdgeType;
}

struct EdgeObject {
  PyObject_HEAD
  NodeObject* m_from_node;
  NodeObject* m_to_node;
  double m_cost;
};

PyMethodDef edge_methods[] = {
  { NULL }
};

PyGetSetDef edge_getset[] = {
  { "from_node", (getter)edge_get_from_node, 0,
    "Node this edge starts from", 0 },
  { "to_node", (getter)edge_get_to_node, 0,
    "Node this edge points to", 0 },
  { "cost", (getter)edge_get_cost, (setter)edge_set_cost,
    "Cost of traversing this node", 0 },
  { NULL }
};

static PyObject* edge_new(PyTypeObject* pytype, PyObject* args, 
			  PyObject* kwds) {
  PyObject* from_node, *to_node;
  double cost = 1.0;
  if (PyArg_ParseTuple(args, "OO(d)", &from_node, &to_node, &cost) <= 0)
    return 0;
  return edge_new_simple((NodeObject*)from_node, (NodeObject*)to_node, cost);
}

static PyObject* edge_new_simple(NodeObject* from_node, NodeObject* to_node,
				 double cost) {
  EdgeObject* so;
  so = (EdgeObject*)EdgeType.tp_alloc(&EdgeType, 0);
  so->m_from_node = (NodeObject*)from_node;
  so->m_to_node = (NodeObject*)to_node;
  so->m_cost = cost;
  return (PyObject*)so;
}  

static void edge_dealloc(PyObject* self) {
  std::cerr << "edge_dealloc\n";
  // EdgeObject* x = (EdgeObject*)self;
  self->ob_type->tp_free(self);
}

PyObject* edge___repr__(PyObject* self) {
  EdgeObject* so = (EdgeObject*)self;
  return PyString_FromFormat("<Edge from %s to %s>", 
			     PyString_AsString(PyObject_Repr((PyObject*)so->m_from_node)),
			     PyString_AsString(PyObject_Repr((PyObject*)so->m_to_node)));
}

PyObject* edge_get_from_node(PyObject* self) {
  EdgeObject* so = (EdgeObject*)self;
  Py_INCREF((PyObject*)so->m_from_node);
  return (PyObject*)so->m_from_node;
}

PyObject* edge_get_to_node(PyObject* self) {
  EdgeObject* so = (EdgeObject*)self;
  Py_INCREF((PyObject*)so->m_to_node);
  return (PyObject*)so->m_to_node;
}

PyObject* edge_get_cost(PyObject* self) {
  EdgeObject* so = (EdgeObject*)self;
  return PyFloat_FromDouble(so->m_cost);
}

int edge_set_cost(PyObject* self, PyObject* cost) {
  if (!PyFloat_Check(cost)) {
    PyErr_SetString(PyExc_TypeError, "edge: expected a float");
    return -1;
  }
  EdgeObject* so = (EdgeObject*)self;
  so->m_cost = PyFloat_AsDouble(cost);
  return 0;
}

// GRAPH

static PyTypeObject GraphType = {
  PyObject_HEAD_INIT(NULL)
  0,
};

typedef std::map<PyObject*, NodeObject*> DataToNodeMap;

struct GraphObject {
  PyObject_HEAD
  bool m_directed;
  NodeSet* m_nodes;
  EdgeSet* m_edges;
  DataToNodeMap* m_data_to_node;
};

extern PyTypeObject* get_GraphType() {
  return &GraphType;
}

PyMethodDef graph_methods[] = {
  { "add_node", graph_add_node, METH_O,
    "Add a node to the graph." },
  { "remove_node", graph_remove_node, METH_O,
    "Remove a node from the graph." },
  { "add_edge", graph_add_edge, METH_VARARGS,
    "Add an edge between two objects in the graph." },
  { "remove_edge", graph_remove_edge, METH_VARARGS,
    "Remove an edge between two objects on the graph." },
  { "get_nodes", graph_get_nodes, METH_NOARGS,
    "Get an iterator over all nodes in the graph." },
  { "get_edges", graph_get_edges, METH_NOARGS,
    "Get an iterator over all edges in the graph." },
  { NULL }
};

static PyObject* graph_new(PyTypeObject* pytype, PyObject* args, 
			   PyObject* kwds) {
  int directed;
  if (PyArg_ParseTuple(args, "i", &directed) <= 0)
    return 0;
  GraphObject* so;
  so = (GraphObject*)pytype->tp_alloc(pytype, 0);
  so->m_directed = (directed == 1);
  so->m_nodes = new NodeSet();
  so->m_edges = new EdgeSet();
  so->m_data_to_node = new DataToNodeMap();
  return (PyObject*)so;
}

static void graph_dealloc(PyObject* self) {
  std::cerr << "graph_dealloc\n";
  GraphObject* x = (GraphObject*)self;
  // Decrease reference counts on node list
  NodeSet::iterator j = x->m_nodes->begin();
  for (; j != x->m_nodes->end(); ++j) {
    Py_DECREF((PyObject*)*j);
  }
  // Decrease reference counts on edge list
  EdgeSet::iterator i = x->m_edges->begin();
  for (; i != x->m_edges->end(); ++i) {
    Py_DECREF((PyObject*)*i);
  }
  self->ob_type->tp_free(self);
}

PyObject* graph_add_node(PyObject* self, PyObject* pyobject) {
  GraphObject* so = ((GraphObject*)self);
  NodeObject* node;
  DataToNodeMap::iterator i = so->m_data_to_node->find(pyobject);
  if (i != so->m_data_to_node->end()) {
    node = i->second;
  } else {
    node = (NodeObject*)node_new_simple(pyobject);
    so->m_nodes->insert(node);
    (*(so->m_data_to_node))[pyobject] = node;
  }
  Py_INCREF((PyObject*)node);
  return (PyObject*)node;
}

PyObject* graph_remove_node(PyObject* self, PyObject* pyobject) {
  GraphObject* so = ((GraphObject*)self);
  NodeObject* node;

  // Find the node
  DataToNodeMap::iterator i = so->m_data_to_node->find(pyobject);
  if (i != so->m_data_to_node->end())
    node = i->second;
  else {
    PyErr_SetString(PyExc_TypeError, "From node is not in the graph");
    return 0;
  }

  // Remove the node
  Py_DECREF((PyObject*)node);
  so->m_nodes->erase(node);
  so->m_data_to_node->erase(pyobject);

  // Erase all edges coming out of that node
  EdgeSet* edges = node->m_edges;
  EdgeSet::iterator j = edges->begin();
  for(; j != edges->end(); ++j) {
    so->m_edges->erase(*j);
    Py_DECREF((PyObject*)*j);
  }

  // Erase all edges pointing into that node
  NodeSet::iterator k = so->m_nodes->begin();
  for(; k != so->m_nodes->end(); ++k) {
    j = (*k)->m_edges->begin();
    for(; j != (*k)->m_edges->end(); ++j)
      if ((*j)->m_to_node == node) {
	(*k)->m_edges->erase(*j);
	so->m_edges->erase(*j);
	Py_DECREF((PyObject*)*j);
      }
  }
  Py_INCREF((PyObject*)node);
  return (PyObject*)node;
}

PyObject* graph_add_edge(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  PyObject* from_pyobject, *to_pyobject;
  double cost = 1.0;
  if (PyArg_ParseTuple(args, "OO|d", &from_pyobject, &to_pyobject, &cost) <= 0)
    return 0;
  NodeObject* from_node, *to_node;
  EdgeObject* edge;
  bool create_new_edge = false;
  DataToNodeMap::iterator i = so->m_data_to_node->find(from_pyobject);
  if (i != so->m_data_to_node->end()) {
    from_node = i->second;
  } else {
    from_node = (NodeObject*)node_new_simple(from_pyobject);
    so->m_nodes->insert(from_node);
    (*(so->m_data_to_node))[from_pyobject] = from_node;
    create_new_edge = true;
  }
  i = so->m_data_to_node->find(to_pyobject);
  if (i != so->m_data_to_node->end()) {
    to_node = i->second;
  } else {
    to_node = (NodeObject*)node_new_simple(to_pyobject);
    so->m_nodes->insert(to_node);
    (*(so->m_data_to_node))[to_pyobject] = to_node;
    create_new_edge = true;
  }
  
  if (!create_new_edge) {
    EdgeSet* from_edges = from_node->m_edges;
    EdgeSet::iterator j = from_edges->begin();
    create_new_edge = true;
    for(; j != from_edges->end(); ++j) {
      if ((*j)->m_to_node == to_node) {
	edge = *j;
	create_new_edge = false;
	break;
      }
    }
  }
  if (create_new_edge) {
    edge = (EdgeObject*)edge_new_simple(from_node, to_node, cost);
    so->m_edges->insert(edge);
    from_node->m_edges->insert(edge);
  }
  Py_INCREF((PyObject*)edge);
  return (PyObject*)edge;
}

PyObject* graph_remove_edge(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  PyObject* from_pyobject, *to_pyobject;
  if (PyArg_ParseTuple(args, "OO", &from_pyobject, &to_pyobject) <= 0)
    return 0;

  NodeObject* from_node, *to_node;
  EdgeObject* edge;
  DataToNodeMap::iterator i = so->m_data_to_node->find(from_pyobject);
  if (i != so->m_data_to_node->end())
    from_node = i->second;
  else {
    PyErr_SetString(PyExc_TypeError, "From node is not in the graph");
    return 0;
  }
  i = so->m_data_to_node->find(to_pyobject);
  if (i != so->m_data_to_node->end())
    to_node = i->second;
  else {
    PyErr_SetString(PyExc_TypeError, "To node is not in the graph");
    return 0;
  }
  
  EdgeSet* from_edges = from_node->m_edges;
  EdgeSet::iterator j = from_edges->begin();
  bool found_edge = false;
  for(; j != from_edges->end(); ++j) {
    if ((*j)->m_to_node == to_node) {
      edge = *j;
      found_edge = true;
      break;
    }
  }
  if (found_edge) {
    from_node->m_edges->erase(edge);
    so->m_edges->erase(edge);
  } else {
    PyErr_SetString(PyExc_TypeError, "There is no edge connecting these nodes");
    return 0;
  }    
  Py_INCREF((PyObject*)edge);
  return (PyObject*)edge;
}

struct NodeIterator {
  IteratorObject _;
  int init_iter(GraphObject* graph) {
    m_node_it = graph->m_nodes->begin();
    return 1;
  }
  static PyObject* next_iter(IteratorObject* self, GraphObject* graph) {
    NodeIterator* so = (NodeIterator*)self;
    if (so->m_node_it == graph->m_nodes->end())
      return 0;
    return (PyObject*)*((so->m_node_it)++);
  }
  NodeSet::iterator m_node_it;
};

PyObject* graph_get_nodes(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  return iterator_new_simple<NodeIterator>(so);
}

struct EdgeIterator {
  IteratorObject _;
  int init_iter(GraphObject* graph) {
    m_edge_it = graph->m_edges->begin();
    return 1;
  }
  static PyObject* next_iter(IteratorObject* self, GraphObject* graph) {
    EdgeIterator* so = (EdgeIterator*)self;
    if (so->m_edge_it == graph->m_edges->end())
      return 0;
    return (PyObject*)*((so->m_edge_it)++);
  }
  EdgeSet::iterator m_edge_it;
};

PyObject* graph_get_edges(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  return iterator_new_simple<EdgeIterator>(so);
}

PyMethodDef graph_module_methods[] = {
  {NULL}
};

DL_EXPORT(void) initgraph(void) {
  PyObject* m = Py_InitModule("gamera.graph", graph_module_methods);
  PyObject* d = PyModule_GetDict(m);

  IteratorType.ob_type = &PyType_Type;
  IteratorType.tp_name = "gamera.graph.Iterator";
  IteratorType.tp_basicsize = sizeof(IteratorObject);
  // IteratorType.tp_dealloc = iterator_dealloc;
  IteratorType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  // IteratorType.tp_new = iterator_new;
  IteratorType.tp_getattro = PyObject_GenericGetAttr;
  IteratorType.tp_alloc = PyType_GenericAlloc;
  IteratorType.tp_free = _PyObject_Del;
  IteratorType.tp_iter = iterator_get_iter;
  IteratorType.tp_iternext = iterator_next;
  PyType_Ready(&IteratorType);
  // PyDict_SetItemString(d, "Node", (PyObject*)&NodeType);

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
  PyType_Ready(&GraphType);
  PyDict_SetItemString(d, "Graph", (PyObject*)&GraphType);
}

