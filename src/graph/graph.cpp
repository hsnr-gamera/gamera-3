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

#include "graph.hpp"

extern "C" {
  static PyObject* graph_new(PyTypeObject* pytype, PyObject* args,
			     PyObject* kwds);
  static void graph_dealloc(PyObject* self);
  static PyObject* graph_copy(PyObject* self, PyObject* other);
  static PyObject* graph_add_node(PyObject* self, PyObject* pyobject);
  static PyObject* graph_add_nodes(PyObject* self, PyObject* pyobject);
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
  static PyObject* graph_get_subgraph_roots(PyObject* self, PyObject* args);
  static PyObject* graph_size_of_subgraph(PyObject* self, PyObject* args);
  static PyObject* graph_is_fully_connected(PyObject* self, PyObject* args);
}

static PyTypeObject GraphType = {
  PyObject_HEAD_INIT(NULL)
  0,
};

GraphObject* graph_new(size_t flags) {
  GraphObject* so;
  so = (GraphObject*)(GraphType.tp_alloc(&GraphType, 0));
  if (!(HAS_FLAG(flags, FLAG_BLOB))) {
    UNSET_FLAG(flags, FLAG_CYCLIC);
    UNSET_FLAG(flags, FLAG_DIRECTED);
  }
  if (!HAS_FLAG(flags, FLAG_CYCLIC)) {
    UNSET_FLAG(flags, FLAG_MULTI_CONNECTED);
    UNSET_FLAG(flags, FLAG_SELF_CONNECTED);
  }
  so->m_flags = flags;
  so->m_nodes = new NodeVector();
  so->m_edges = new EdgeVector();
  so->m_data_to_node = new DataToNodeMap();
  return so;
}

PyObject* graph_new(PyTypeObject* pytype, PyObject* args, 
		    PyObject* kwds) {
  long flags = FLAG_DEFAULT;
  if (PyArg_ParseTuple(args, "|i", &flags) <= 0)
    return 0;
  return (PyObject*)graph_new((size_t)flags);
}

bool is_GraphObject(PyObject* self) {
  return PyObject_TypeCheck(self, &GraphType);
}

void graph_dealloc(PyObject* self) {
#ifdef DEBUG_DEALLOC
  std::cerr << "graph_dealloc\n";
#endif
  GraphObject* so = (GraphObject*)self;
  for (NodeVector::iterator i = so->m_nodes->begin();
       i != so->m_nodes->end(); ++i) {
    delete (*i);
  }
  for (EdgeVector::iterator i = so->m_edges->begin();
       i != so->m_edges->end(); ++i) {
    delete (*i);
  }
  delete so->m_nodes;
  delete so->m_edges;
  delete so->m_data_to_node;
  self->ob_type->tp_free(self);
}

GraphObject* graph_copy(GraphObject* so, size_t flags) {
  GraphObject* result = graph_new(flags);
  result->m_nodes->reserve(so->m_nodes->size());

  for (NodeVector::iterator i = so->m_nodes->begin();
       i != so->m_nodes->end(); ++i)
    graph_add_node(result, (*i)->m_data);

  for (EdgeVector::iterator j = so->m_edges->begin();
       j != so->m_edges->end(); ++j)
    graph_add_edge(result, (*j)->m_from_node->m_data,
		   (*j)->m_to_node->m_data,
		   (*j)->m_cost);
  return result;
}

PyObject* graph_copy(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  long flags = FLAG_DEFAULT;
  if (PyArg_ParseTuple(args, "|i", &flags) <= 0)
    return 0;
  return (PyObject*)graph_copy(so, (size_t)flags);
}

PyObject* graph_add_node(PyObject* self, PyObject* pyobject) {
  GraphObject* so = ((GraphObject*)self);
  Node* node = graph_find_node(so, pyobject, false);
  if (node == 0) {
    node = new Node(so, pyobject);
    graph_add_node(so, node);
    return PyInt_FromLong((long)1);
  }
  return PyInt_FromLong((long)0);
}

PyObject* graph_add_nodes(PyObject* self, PyObject* pyobject) {
  GraphObject* so = ((GraphObject*)self);
  if (!PyList_Check(pyobject)) {
    PyErr_SetString(PyExc_TypeError, "Argument must be a list of nodes");
    return 0;
  }
  size_t list_size = PyList_Size(pyobject);
  size_t result = 0;
  so->m_nodes->reserve(so->m_nodes->size() + list_size);
  for (size_t i = 0; i < list_size; ++i)
    if (graph_add_node(so, PyList_GET_ITEM(pyobject, i)))
      result++;
  return PyInt_FromLong((long)result);
}

PyObject* graph_remove_node_and_edges(PyObject* self, PyObject* a) {
  GraphObject* so = ((GraphObject*)self);
  Node* node;
  // Find the node
  node = graph_find_node(so, a);
  if (node == 0)
    return 0;
  return PyInt_FromLong((long)graph_remove_node_and_edges(so, node));
}

PyObject* graph_remove_node(PyObject* self, PyObject* a) {
  GraphObject* so = ((GraphObject*)self);
  Node* node;

  // Find the node
  node = graph_find_node(so, a);
  if (node == 0)
    return 0;
  return PyInt_FromLong((long)graph_remove_node(so, node));
}

PyObject* graph_add_edge(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  PyObject* from_pyobject, *to_pyobject;
  CostType cost = 1.0;
  PyObject* label = NULL;
  if (PyArg_ParseTuple(args, "OO|dO", &from_pyobject, &to_pyobject, &cost, &label) <= 0)
    return 0;
  return PyInt_FromLong((long)graph_add_edge(so, from_pyobject, to_pyobject, cost, label));
}

PyObject* graph_add_edges(PyObject* self, PyObject* args) {
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
    if (PyArg_ParseTuple(tuple, "OO|dO", &from_node, &to_node, &cost, &label) <= 0)
      return 0;
    graph_add_edge(so, from_node, to_node, cost, label);
  }
  Py_INCREF(Py_None);
  return Py_None;
}

PyObject* graph_remove_edge(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  PyObject* a = NULL;
  PyObject* b = NULL;
  bool result = false;
  if (PyArg_ParseTuple(args, "O|O", &a, &b) <= 0)
    return 0;
  if (b == NULL) {
    if (is_EdgeObject(a)) {
      if (graph_has_edge(so,
			 ((EdgeObject*)a)->m_x->m_from_node,
			 ((EdgeObject*)a)->m_x->m_to_node))
	return PyInt_FromLong((long)graph_remove_edge
			      (so, ((EdgeObject*)a)->m_x));
      else {
	PyErr_SetString(PyExc_RuntimeError, "Given edge is not in the graph");
	return 0;
      }
    }
  } else {
    if (is_NodeObject(a)) {
      if (graph_has_node(so, ((NodeObject*)a)->m_x)) {
	if (is_NodeObject(b)) {
	  if (graph_has_node(so, ((NodeObject*)b)->m_x))
	    result = graph_remove_edge(so, ((NodeObject*)a)->m_x, ((NodeObject*)b)->m_x);
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
      Node *from_node, *to_node;
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

PyObject* graph_remove_all_edges(PyObject* self, PyObject* _) {
  GraphObject* so = ((GraphObject*)self);
  graph_remove_all_edges(so);
  Py_INCREF(Py_None);
  return Py_None;
}

PyObject* graph_is_directed(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  return PyInt_FromLong((long)(bool)HAS_FLAG(so->m_flags, FLAG_DIRECTED));
}

PyObject* graph_is_undirected(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  return PyInt_FromLong((long)!(bool)HAS_FLAG(so->m_flags, FLAG_DIRECTED));
}

void graph_make_directed(GraphObject* so) {
  if (!HAS_FLAG(so->m_flags, FLAG_DIRECTED)) {
    SET_FLAG(so->m_flags, FLAG_DIRECTED);
    for (NodeVector::iterator i = so->m_nodes->begin();
	 i != so->m_nodes->end(); ++i) {
      for (EdgeList::iterator j, k = (*i)->m_edges.begin();
	   k != (*i)->m_edges.end();) {
	j = k++;
	if ((*j)->m_from_node != *i)
	  (*i)->m_edges.remove(*j);
      }
    }
  }
}

void graph_make_undirected(GraphObject* so) {
  if (HAS_FLAG(so->m_flags, FLAG_DIRECTED)) {
    UNSET_FLAG(so->m_flags, FLAG_DIRECTED);
    
    EdgeList edges;
    for (NodeVector::iterator i = so->m_nodes->begin();
	 i != so->m_nodes->end(); ++i) {
      for (EdgeList::iterator j = (*i)->m_edges.begin();
	   j != (*i)->m_edges.end(); ++j) 
	edges.push_back(*j);
      (*i)->m_disj_set = 0;
    }

    for (EdgeList::iterator i = edges.begin();
	 i != edges.end(); ++i) {
      (*i)->m_to_node->m_edges.push_back(*i);
      size_t to_set_id = graph_disj_set_find_and_compress
	(so, (*i)->m_to_node->m_set_id);
      size_t from_set_id = graph_disj_set_find_and_compress
	(so, (*i)->m_from_node->m_set_id);
      if (from_set_id != to_set_id)
	graph_disj_set_union_by_height(so, to_set_id, from_set_id);
    }
  }
}

PyObject* graph_make_directed(PyObject* self, PyObject* args) {
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

PyObject* graph_make_undirected(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  graph_make_undirected(so);
  Py_INCREF(Py_None);
  return Py_None;
}  

PyObject* graph_is_cyclic(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  return PyInt_FromLong((long)(bool)HAS_FLAG(so->m_flags, FLAG_CYCLIC));
}

PyObject* graph_is_acyclic(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  return PyInt_FromLong((long)!(bool)HAS_FLAG(so->m_flags, FLAG_CYCLIC));
}

void graph_make_cyclic(GraphObject* so) {
  SET_FLAG(so->m_flags, FLAG_CYCLIC);
}

void graph_make_acyclic(GraphObject* so) {
  if (HAS_FLAG(so->m_flags, FLAG_CYCLIC)) {
    graph_make_not_self_connected(so);
    graph_make_singly_connected(so);
    if (so->m_edges->size()) {
      for (NodeVector::iterator i = so->m_nodes->begin();
	   i != so->m_nodes->end(); ++i)
	NP_VISITED(*i) = false;
      for (NodeVector::iterator i = so->m_nodes->begin();
	   i != so->m_nodes->end(); ++i) {
	if (!NP_VISITED(*i)) {
	  NodeStack node_stack;
	  node_stack.push(*i);
	  while (!node_stack.empty()) {
	    Node* node = node_stack.top();
	    node_stack.pop();
	    NP_VISITED(node) = true;
	    for (EdgeList::iterator k, j = node->m_edges.begin();
		 j != node->m_edges.end();) {
	      k = j++;
	      Node* inner_node = (*k)->traverse(node);
	      if (NP_VISITED(inner_node))
		graph_remove_edge(so, *k);
	      else {
		node_stack.push(inner_node);
		NP_VISITED(inner_node) = true;
	      }
	    }
	  }
	}
      }
    }
    UNSET_FLAG(so->m_flags, FLAG_CYCLIC);
  }
}

PyObject* graph_make_cyclic(PyObject* self, PyObject* args) {
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

PyObject* graph_make_acyclic(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  graph_make_acyclic(so);
  Py_INCREF(Py_None);
  return Py_None;
}  

PyObject* graph_is_tree(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  return PyInt_FromLong((long)!(bool)HAS_FLAG(so->m_flags, FLAG_BLOB));
}

PyObject* graph_is_blob(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  return PyInt_FromLong((long)(bool)HAS_FLAG(so->m_flags, FLAG_BLOB));
}

void graph_make_tree(GraphObject* so) {
  if (HAS_FLAG(so->m_flags, FLAG_BLOB)) {
    UNSET_FLAG(so->m_flags, FLAG_BLOB);
    graph_make_acyclic(so);
    graph_make_undirected(so);
  }
}

void graph_make_blob(GraphObject* so) {
  SET_FLAG(so->m_flags, FLAG_BLOB);
}

PyObject* graph_make_tree(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  graph_make_tree(so);
  Py_INCREF(Py_None);
  return Py_None;
}

PyObject* graph_make_blob(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  graph_make_blob(so);
  Py_INCREF(Py_None);
  return Py_None;
}  

PyObject* graph_is_multi_connected(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  return PyInt_FromLong((long)(bool)HAS_FLAG(so->m_flags, FLAG_MULTI_CONNECTED));
}

PyObject* graph_is_singly_connected(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  return PyInt_FromLong((long)!(bool)HAS_FLAG(so->m_flags, FLAG_MULTI_CONNECTED));
}

void graph_make_multi_connected(GraphObject* so) {
  SET_FLAG(so->m_flags, FLAG_MULTI_CONNECTED);
}

void graph_make_singly_connected(GraphObject* so, bool maximum_cost) {
  if (HAS_FLAG(so->m_flags, FLAG_MULTI_CONNECTED)) {
    if (so->m_edges->size()) {
      for (NodeVector::iterator i = so->m_nodes->begin();
	   i != so->m_nodes->end(); ++i) {
	NodeToEdgeMap node_map;
	for (EdgeList::iterator j, j0 = (*i)->m_edges.begin();
	     j0 != (*i)->m_edges.end(); ) {
	  j = j0++;
	  NodeToEdgeMap::iterator l = node_map.find((*j)->m_to_node);
	  if (l == node_map.end())
	    node_map[(*j)->m_to_node] = *j;
	  for (EdgeList::iterator k, k0 = j;
	       k0 != (*i)->m_edges.end();) {
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
    UNSET_FLAG(so->m_flags, FLAG_MULTI_CONNECTED);
  }
}

PyObject* graph_make_multi_connected(PyObject* self, PyObject* args) {
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

PyObject* graph_make_singly_connected(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  int maximum = 1;
  if (PyArg_ParseTuple(args, "|i", &maximum) <= 0)
    return 0;
  graph_make_singly_connected(so, maximum != 0);
  Py_INCREF(Py_None);
  return Py_None;
}  

PyObject* graph_is_self_connected(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  return PyInt_FromLong((long)(bool)HAS_FLAG(so->m_flags, FLAG_SELF_CONNECTED));
}

void graph_make_self_connected(GraphObject* so) {
  SET_FLAG(so->m_flags, FLAG_SELF_CONNECTED);
}

void graph_make_not_self_connected(GraphObject* so) {
  if (HAS_FLAG(so->m_flags, FLAG_SELF_CONNECTED)) {
    if (so->m_edges->size()) {
      EdgeList removals;
      for (NodeVector::iterator i = so->m_nodes->begin();
	   i != so->m_nodes->end(); ++i)
	for (EdgeList::iterator j, j0 = (*i)->m_edges.begin();
	     j0 != (*i)->m_edges.end();) {
	  j = j0++;
	  if ((*j)->traverse(*i) == (*i))
	    graph_remove_edge(so, *j);
	}
    }
    UNSET_FLAG(so->m_flags, FLAG_SELF_CONNECTED);
  }
}

PyObject* graph_make_self_connected(PyObject* self, PyObject* args) {
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

PyObject* graph_make_not_self_connected(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  graph_make_not_self_connected(so);
  Py_INCREF(Py_None);
  return Py_None;
}  

PyObject* graph_get_nodes(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  typedef NodeIterator<NodeVector> NodeVectorIterator;
  NodeVectorIterator* iterator = iterator_new<NodeVectorIterator>();
  iterator->init(so->m_nodes->begin(), so->m_nodes->end());
  return (PyObject*)iterator;
}

PyObject* graph_get_node(PyObject* self, PyObject* pyobject) {
  GraphObject* so = ((GraphObject*)self);
  Node* node = graph_find_node(so, pyobject);
  return nodeobject_new(node);
}

PyObject* graph_has_node(PyObject* self, PyObject* a) {
  GraphObject* so = (GraphObject*)self;
  Node* node = graph_find_node(so, a, false);
  if (node == 0)
    return PyInt_FromLong((long)0);
  return PyInt_FromLong((long)1);
}

PyObject* graph_get_nnodes(PyObject* self) {
  GraphObject* so = (GraphObject*)self;
  return PyInt_FromLong((long)so->m_nodes->size());
}

PyObject* graph_get_edges(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  AllEdgeIterator* iterator = iterator_new<AllEdgeIterator>();
  iterator->init(so->m_nodes->begin(), so->m_nodes->end());
  return (PyObject*)iterator;
}

PyObject* graph_has_edge(PyObject* self, PyObject* args) {
  GraphObject* so = (GraphObject*)self;
  PyObject *a = NULL, *b = NULL;
  if (PyArg_ParseTuple(args, "O|O", &a, &b) <= 0)
    return 0;
  if (is_EdgeObject(a) && b == NULL) {
    Edge *edge = ((EdgeObject*)a)->m_x;
    return PyInt_FromLong
      ((long)graph_has_edge(so, edge->m_from_node, edge->m_to_node));
  }
  if (is_NodeObject(a) && is_NodeObject(b)) {
    Node *from_node = ((NodeObject*)a)->m_x;
    Node *to_node = ((NodeObject*)b)->m_x;
    return PyInt_FromLong
      ((long)graph_has_edge(so, from_node, to_node));
  }
  if (!is_NodeObject(b) && !is_NodeObject(b) && !is_EdgeObject(a) && !is_EdgeObject(b)) {
    Node *from_node = graph_find_node(so, a, false);
    if (from_node == 0)
      return PyInt_FromLong(0);
    Node *to_node = graph_find_node(so, b, false);
    if (to_node == 0)
      return PyInt_FromLong(0);
    return PyInt_FromLong
      ((long)graph_has_edge(so, from_node, to_node));
  }    
  PyErr_SetString(PyExc_TypeError, "Invalid argument types");
  return 0;
}

PyObject* graph_get_nedges(PyObject* self) {
  GraphObject* so = (GraphObject*)self;
  return PyInt_FromLong((long)(so->m_edges->size()));
}

struct SubGraphRootIterator : IteratorObject {
  int init(NodeVector::iterator begin, NodeVector::iterator end) {
    m_it = begin;
    m_end = end;
    return 1;
  }
  static PyObject* next(IteratorObject* self) {
    SubGraphRootIterator* so = (SubGraphRootIterator*)self;
    if (so->m_it == so->m_end)
      return 0;
    while (!((*(so->m_it))->m_is_subgraph_root)) {
      (so->m_it)++;
      if (so->m_it == so->m_end)
	return 0;
    }
    return nodeobject_new(*((so->m_it)++));
  }
  NodeVector::iterator m_it, m_end;
};

struct SubTreeRootIterator : IteratorObject {
  int init(NodeVector::iterator begin, NodeVector::iterator end) {
    m_it = begin;
    m_end = end;
    return 1;
  }
  static PyObject* next(IteratorObject* self) {
    SubTreeRootIterator* so = (SubTreeRootIterator*)self;
    if (so->m_it == so->m_end)
      return 0;
    while ((*(so->m_it))->m_disj_set > 0) {
      (so->m_it)++;
      if (so->m_it == so->m_end)
	return 0;
    }
    return nodeobject_new(*((so->m_it)++));
  }
  NodeVector::iterator m_it, m_end;
};

PyObject* graph_get_subgraph_roots(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  
  if (HAS_FLAG(so->m_flags, FLAG_DIRECTED) || HAS_FLAG(so->m_flags, FLAG_CYCLIC)) {
    SubGraphRootIterator* iterator = iterator_new<SubGraphRootIterator>();
    iterator->init(so->m_nodes->begin(), so->m_nodes->end());
    return (PyObject*)iterator;
  } else {
    SubTreeRootIterator* iterator = iterator_new<SubTreeRootIterator>();
    iterator->init(so->m_nodes->begin(), so->m_nodes->end());
    return (PyObject*)iterator;
  }
}

size_t graph_size_of_subgraph(GraphObject* so, Node* root) {
  size_t count = 0;
  DFSIterator* iterator = iterator_new<DFSIterator>();
  iterator->init(so, root);
  while (DFSIterator::next_node(iterator))
    ++count;
  return count;
}

PyObject* graph_size_of_subgraph(PyObject* self, PyObject* pyobject) {
  GraphObject* so = ((GraphObject*)self);
  Node* root;
  root = graph_find_node(so, pyobject);
  if (root == 0)
    return 0;
  return PyInt_FromLong((long)graph_size_of_subgraph(so, root));
}

PyObject* graph_get_nsubgraphs(PyObject* self, PyObject* _) {
  GraphObject* so = ((GraphObject*)self);
  size_t count = 0; 
  if (HAS_FLAG(so->m_flags, FLAG_DIRECTED)) {
    for (NodeVector::iterator i = so->m_nodes->begin();
	 i != so->m_nodes->end(); ++i)
      if ((*i)->m_is_subgraph_root)
	++count;
  } else {
    for (NodeVector::iterator i = so->m_nodes->begin();
	 i != so->m_nodes->end(); ++i)
      if ((*i)->m_disj_set <= 0)
	++count;
  }    
  return PyInt_FromLong((long)count);
}

PyObject* graph_is_fully_connected(PyObject* self, PyObject* _) {
  GraphObject* so = (GraphObject*)self;
  bool result = false;
  return PyInt_FromLong((long)1);
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
  { "remove_node_and_edges", graph_remove_node_and_edges, METH_O,
    "Remove a node from the graph, removing all associated edges" },
  { "remove_node", graph_remove_node, METH_O,
    "Remove a node from the graph, stitching together the broken edges." },
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
  SEARCH_METHODS
  SHORTEST_PATH_METHODS
  SPANNING_TREE_METHODS
  PARTITIONS_METHODS
  { NULL }
};

PyGetSetDef graph_getset[] = {
  { "nnodes", (getter)graph_get_nnodes, 0,
    "Number of nodes in the graph", 0 },
  { "nedges", (getter)graph_get_nedges, 0,
    "Number of edges in the graph", 0 },
  { "nsubgraphs", (getter)graph_get_nsubgraphs, 0,
    "Number of edges in the graph", 0 },
  { NULL }
};

void init_GraphType(PyObject* d) {
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
}
