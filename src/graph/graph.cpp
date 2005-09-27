/*
 *
 * Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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
  if (PyArg_ParseTuple(args, "|i:Graph.__init__", &flags) <= 0)
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
  NodeVector* nodes = so->m_nodes;
  EdgeVector* edges = so->m_edges;
  DataToNodeMap* data_to_node = so->m_data_to_node;

  for (NodeVector::iterator i = nodes->begin();
       i != nodes->end(); ++i) {
    delete (*i);
  }
  for (EdgeVector::iterator i = edges->begin();
       i != edges->end(); ++i) {
    delete (*i);
  }
  delete nodes;
  delete edges;
  delete data_to_node;
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
  long flags = so->m_flags;
  if (PyArg_ParseTuple(args, "|i:copy", &flags) <= 0)
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
  PyObject* seq = PySequence_Fast(pyobject, "Argument must be an iterable of nodes");
  if (seq == NULL)
    return 0;
  size_t list_size = PySequence_Fast_GET_SIZE(seq);
  size_t result = 0;
  so->m_nodes->reserve(so->m_nodes->size() + list_size);
  for (size_t i = 0; i < list_size; ++i)
    if (graph_add_node(so, PySequence_Fast_GET_ITEM(seq, i)))
      result++;
  Py_DECREF(seq);
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
  if (PyArg_ParseTuple
      (args, "OO|dO:add_edge", &from_pyobject, &to_pyobject, &cost, &label) <= 0)
    return 0;
  return PyInt_FromLong((long)graph_add_edge(so, from_pyobject, to_pyobject, cost, label) != 0);
}

PyObject* graph_add_edges(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  PyObject* a;
  if (PyArg_ParseTuple(args, "O:add_edges", &a) <= 0)
    return 0;
  PyObject* seq = PySequence_Fast(a, "Input must be an iterable of edge tuples");
  if (seq == NULL)
    return 0;
  size_t list_size = PySequence_Fast_GET_SIZE(seq);
  for (size_t i = 0; i < list_size; ++i) {
    PyObject* tuple = PySequence_Fast_GET_ITEM(seq, i);
    PyObject* from_node, *to_node;
    CostType cost = 1;
    PyObject* label = NULL;
    if (PyArg_ParseTuple(tuple, "OO|dO:add_edges sequence element", &from_node, &to_node, &cost, &label) <= 0) {
      Py_DECREF(seq);
      return 0;
    }
    graph_add_edge(so, from_node, to_node, cost, label);
  }
  Py_DECREF(seq);
  Py_INCREF(Py_None);
  return Py_None;
}

PyObject* graph_remove_edge(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  PyObject* a = NULL;
  PyObject* b = NULL;
  bool result = false;
  if (PyArg_ParseTuple(args, "O|O:remove_edge", &a, &b) <= 0)
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
      NP_VISITED(*i) = false;
      (*i)->m_is_subgraph_root = false;
      for (EdgeList::iterator j, k = (*i)->m_edges.begin();
	   k != (*i)->m_edges.end();) {
	j = k++;
	if ((*j)->m_from_node != *i)
	  (*i)->m_edges.erase(j);
      }
    }

    // This is to take care of the subgraph roots
    for (NodeVector::iterator i = so->m_nodes->begin();
	 i != so->m_nodes->end(); ++i) {
      if (!NP_VISITED(*i)) {
	(*i)->m_is_subgraph_root = true;
	NodeStack node_stack;
	node_stack.push(*i);
	NP_VISITED(*i) = true;
	while (!node_stack.empty()) {
	  Node* node = node_stack.top();
	  node_stack.pop();
	  for (EdgeList::iterator j = node->m_edges.begin();
	       j != node->m_edges.end(); ++j) {
	    // Traverse not needed, since we know this is directed
	    // at this point
	    Node* inner_node = (*j)->m_to_node;
	    if (!NP_VISITED(inner_node)) {
	      node_stack.push(inner_node);
	      NP_VISITED(inner_node) = true;
	    }
	  }
	}
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
  if (PyArg_ParseTuple(args, "|i:make_directed", &directed) <= 0)
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
  if (HAS_FLAG(so->m_flags, FLAG_CYCLIC)) {
    return PyInt_FromLong(1);
  } else {
    return PyInt_FromLong(0);
  }
}

PyObject* graph_is_acyclic(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  if (HAS_FLAG(so->m_flags, FLAG_CYCLIC)) {
    return PyInt_FromLong(0);
  } else {
    return PyInt_FromLong(1);
  }
}

void graph_make_cyclic(GraphObject* so) {
  SET_FLAG(so->m_flags, FLAG_CYCLIC);
}

void graph_make_acyclic(GraphObject* so) {
  if (HAS_FLAG(so->m_flags, FLAG_CYCLIC)) {
    graph_make_not_self_connected(so);
    graph_make_singly_connected(so);
    NodeStack node_stack;
    if (so->m_edges->size()) {
      for (NodeVector::iterator i = so->m_nodes->begin();
	   i != so->m_nodes->end(); ++i) 
	NP_VISITED(*i) = false;
      for (NodeVector::iterator i = so->m_nodes->begin();
	   i != so->m_nodes->end(); ++i) {
	if (!NP_VISITED(*i)) {
	  if (node_stack.size())
	    throw std::runtime_error("Error in graph_make_acyclic.  This error should never be raised.  Please report it to the author.");
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
  if (PyArg_ParseTuple(args, "|i:make_cyclic", &cyclic) <= 0)
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
  if (HAS_FLAG(so->m_flags, FLAG_BLOB)) {
    return PyInt_FromLong(0);
  } else {
    return PyInt_FromLong(1);
  }
}

PyObject* graph_is_blob(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  if (HAS_FLAG(so->m_flags, FLAG_BLOB)) {
    return PyInt_FromLong(1);
  } else {
    return PyInt_FromLong(0);
  }
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
  SET_FLAG(so->m_flags, FLAG_CYCLIC);
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
  if (HAS_FLAG(so->m_flags, FLAG_MULTI_CONNECTED)) {
    return PyInt_FromLong(1);
  } else {
    return PyInt_FromLong(0);
  }
}

PyObject* graph_is_singly_connected(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  if (HAS_FLAG(so->m_flags, FLAG_MULTI_CONNECTED)) {
    return PyInt_FromLong(0);
  } else {
    return PyInt_FromLong(1);
  }
}

void graph_make_multi_connected(GraphObject* so) {
  SET_FLAG(so->m_flags, FLAG_MULTI_CONNECTED);
}

void graph_make_singly_connected(GraphObject* so, bool maximum_cost) {
  if (HAS_FLAG(so->m_flags, FLAG_MULTI_CONNECTED)) {
    if (so->m_edges->size()) {
      NodeToEdgeMap node_map;
      for (NodeVector::iterator i = so->m_nodes->begin();
	   i != so->m_nodes->end(); ++i) {
	node_map.clear();
	for (EdgeList::iterator j = (*i)->m_edges.begin();
	     j != (*i)->m_edges.end(); j++) {
	  NodeToEdgeMap::iterator l = node_map.find((*j)->m_to_node);
	  if (l == node_map.end())
	    node_map[(*j)->m_to_node] = *j;
	  else {
	    if (maximum_cost) {
	      if ((*l).second->m_cost < (*j)->m_cost)
		node_map[(*j)->m_to_node] = *j;
	    } else {
	      if ((*l).second->m_cost > (*j)->m_cost)
		node_map[(*j)->m_to_node] = *j;
	    }
	  }
	}

	for (EdgeList::iterator j, j0 = (*i)->m_edges.begin();
	     j0 != (*i)->m_edges.end(); ) {
	  j = j0++;
	  NodeToEdgeMap::iterator l = node_map.find((*j)->m_to_node);
	  if (l == node_map.end()) {
	    throw std::runtime_error("Error in graph_make_singly_connected.  This error should never be raised, please report it to the author.");
	  } else if ((*l).second != *j) {
	    graph_remove_edge(so, *j);
	  }
	}
      }
    }
    UNSET_FLAG(so->m_flags, FLAG_MULTI_CONNECTED);
  }
}

PyObject* graph_make_multi_connected(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  graph_make_multi_connected(so);
  Py_INCREF(Py_None);
  return Py_None;
}

PyObject* graph_make_singly_connected(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  int maximum = 1;
  if (PyArg_ParseTuple(args, "|i:make_singly_connected", &maximum) <= 0)
    return 0;
  graph_make_singly_connected(so, maximum != 0);
  Py_INCREF(Py_None);
  return Py_None;
}  

PyObject* graph_is_self_connected(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  if (HAS_FLAG(so->m_flags, FLAG_SELF_CONNECTED)) {
    return PyInt_FromLong(1);
  } else {
    return PyInt_FromLong(0);
  }
}

void graph_make_self_connected(GraphObject* so) {
  SET_FLAG(so->m_flags, FLAG_SELF_CONNECTED);
}

void graph_make_not_self_connected(GraphObject* so) {
  if (HAS_FLAG(so->m_flags, FLAG_SELF_CONNECTED)) {
    if (so->m_edges->size()) {
      EdgeList removals;
      for (NodeVector::iterator i = so->m_nodes->begin();
	   i != so->m_nodes->end(); ++i) {
	for (EdgeList::iterator j, j0 = (*i)->m_edges.begin();
	     j0 != (*i)->m_edges.end();) {
	  j = j0++;
	  if ((*j)->traverse(*i) == (*i))
	    graph_remove_edge(so, *j);
	}
      }
    }
    UNSET_FLAG(so->m_flags, FLAG_SELF_CONNECTED);
  }
}

PyObject* graph_make_self_connected(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  int self_connected = 1;
  if (PyArg_ParseTuple(args, "|i:make_self_connected", &self_connected) <= 0)
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
  if (node == 0) {
    PyErr_SetString(PyExc_ValueError, "There is no node associated with the given value.");
    return 0;
  }
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
  EdgeIterator<EdgeVector>* iterator = iterator_new<EdgeIterator<EdgeVector> >();
  iterator->init(so->m_edges->begin(), so->m_edges->end());
  return (PyObject*)iterator;
}

PyObject* graph_has_edge(PyObject* self, PyObject* args) {
  GraphObject* so = (GraphObject*)self;
  PyObject *a = NULL, *b = NULL;
  if (PyArg_ParseTuple(args, "O|O:has_edge", &a, &b) <= 0)
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
  if (HAS_FLAG(so->m_flags, FLAG_DIRECTED)) {
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
  Py_DECREF(iterator);
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
  return PyInt_FromLong((long)count <= 1);
}

PyMethodDef graph_methods[] = {
  { "copy", graph_copy, METH_VARARGS,
    "**copy** (*flags* = ``FREE``)\n\n" \
    "Copies a graph (optionally specifying new flags for the new graph).\n\n" \
    "In some cases, copying the graph to a new graph type may be faster\n" \
    "than using one of the in-place conversion functions.\n\n" \
    "See `Graph constructor`_ for a definition of *flags*.\n\n"
  },
  { "add_node", graph_add_node, METH_O,
    "**add_node** (*value*)\n\n" \
    "Add a node identified by the given *value*. " \
    "The newly-created node has no edges.\n\n" \
    "Returns 1 if a new node was created.\n" \
    "Returns 0 if a node already exists with the associated *value*.\n\n" \
    "**Complexity**: Nodes are added in constant time, except when requiring a reallocation of the node vector.\n\n"
  },
  { "add_nodes", graph_add_nodes, METH_O,
    "**add_nodes** (*list_of_values*)\n\n" \
    "Add nodes identified by each value in a list. " \
    "The newly-created nodes have no edges.\n\n" \
    "Returns the number of new nodes that were created.\n\n" \
    "**Complexity**: `add_nodes` is moderately faster than multiple calls to add_node_.\n" \
    "Nodes are added in constant time, except when requiring a reallocation of the node vector.\n\n"
  },
  { "remove_node_and_edges", graph_remove_node_and_edges, METH_O,
    "**remove_node_and_edges** (*value*)\n\n" \
    "Remove the node identifed by *value* from the graph, and remove all edges pointing inward or outward from that node.\n\n" \
    "For instance, given the graph::\n\n" \
    "  a -> b -> c\n\n" \
    "``.remove_node_and_edges('b')`` will result in::\n\n" \
    "  a         c\n\n" \
    "**Complexity**: Removing a node takes *O* (*n* + *e*) where *n* is the number of nodes in the graph and *e* is the number of edges attached to the given node.\n\n"
  },
  { "remove_node", graph_remove_node, METH_O,
    "**remove_node** (*value*)\n\n" \
    "Remove a node identified by *value* from the graph, stitching together the broken edges.\n\n" \
    "For instance, given the graph::\n\n" \
    "  a -> b -> c\n\n" \
    "``.remove_node('b')`` will result in::\n\n" \
    "  a -> c\n\n" \
    "**Complexity**: Removing a node takes *O* (*n* + *e*) where *n* is the number of nodes in the graph and *e* is the number of edges attached to the given node.\n\n"
  },
  { "add_edge", graph_add_edge, METH_VARARGS,
    "**add_edge** (*from_value*, *to_value*, *cost* = 1.0, *label* = None)\n\n" \
    "Add an edge between the two nodes identified by *from_value* and *to_value*.\n\n" \
    "The return value is the number of edges created.  If the edge violates any of the restrictions specified\n" \
    "by the flags to the graph's constructor, the edge will not be created.\n\n"
    "If the graph is ``DIRECTED``, the edge goes from *from_value* to *to_value*.\n\n" \
    "If a node representing one of the values is not present, a nodes will be implicitly created.\n\n" \
    "Optionally, a *cost* and *label* can be associated with the edge.  These values are used by some\n" \
    "higher-level graph algorithms such as create_minimum_spanning_tree_.\n\n" \
    "**Complexity**: Edges are added in constant time, except when requiring a reallocation of the edge vector, or when ``CYCLIC`` is ``False``.\n\n"
  },
  { "add_edges", graph_add_edges, METH_VARARGS,
    "**add_edges** (*list_of_tuples*)\n\n" \
    "Add edges specified by a list of tuples of the form:\n\n" \
    "   (*from_value*, *to_value*, [*cost*[, *label*]]).\n\n" \
    "The members of this tuple correspond to the arguments to add_edge_.\n\n" \
    "The return value is the number of edges created.  If an edge violates any of the restrictions specified\n" \
    "by the flags to the graph's constructor, that edge will not be created.\n\n" \
    "If a node representing any of the values are not present, a node will be implicitly created.\n\n" \
    "**Complexity:** ``add_edges`` is moderately faster than multiple calls to add_edge_.\n" \
    "Edges are added in constant time, except when requiring a reallocation of the edge vector, or when ``CYCLIC`` is ``False``.\n\n"
  },
  { "remove_edge", graph_remove_edge, METH_VARARGS,
    "**remove_edge** (*from_value*, *to_value*)\n\n" \
    "Remove an edge between two nodes identified by *from_value* and *to_value*.\n\n" \
    "If the edge does not exist in the graph, a ``RuntimeError`` exception is raised.\n\n" \
    "When the graph is ``DIRECTED``, only the edge going from *from_value* to *to_value* is removed.\n\n" \
    "If the graph is ``MULTI_CONNECTED``, **all** edges from *from_value* to *to_value* are removed.\n\n" \
    "**Complexity**: Edges can be removed in *O*(*e*) time where *e* is the number of edges in the graph.\n\n"
  },
  { "remove_all_edges", graph_remove_all_edges, METH_NOARGS,
    "**remove_all_edges** ()\n\n" \
    "Remove all the edges in the graph, leaving all nodes as islands.\n\n" \
    "**Complexity**: ``remove_all_edges`` takes *O* ( *n* + *e*) time where *n* is the number of nodes in the graph and *e* is the number of edges in the graph." 
  },
  { "is_directed", graph_is_directed, METH_NOARGS,
    "**is_directed** ()\n\n" \
    "Return ``True`` if the graph is defined as directed." 
  },
  { "is_undirected", graph_is_undirected, METH_NOARGS,
    "**is_undirected** ()\n\n" \
    "Return ``True`` if the graph is defined as undirected." 
  },
  { "make_directed", graph_make_directed, METH_VARARGS,
    "**make_directed** ()\n\n" \
    "If the graph is undirected, converts it into an undirected graph by adding a complementary edge for\n" \
    "each existing edge."
  },
  { "make_undirected", graph_make_undirected, METH_NOARGS,
    "**make_undirected** ()\n\n" \
    "If the graph is directed, converts it into an undirected graph.  Each edge in the existing graph\n" \
    "will become a non-directional edge in the resulting graph." 
  },
  { "is_cyclic", graph_is_cyclic, METH_NOARGS,
    "**is_cyclic** ()\n\n" \
    "Returns ``True`` if the graph is defined as cyclic.  Note that this is ``True`` even if the graph does\n" \
    "not currently have any cycles."
  },
  { "is_acyclic", graph_is_acyclic, METH_NOARGS,
    "**is_acyclic** ()\n\n" \
    "Returns ``True`` is the graph is defined as acyclic."
  },
  { "make_cyclic", graph_make_cyclic, METH_VARARGS,
    "**make_cyclic** ()\n\n" \
    "Allow the graph to include cycles from this point on.  This does nothing except set the ``CYCLIC`` flag." 
  },
  { "make_acyclic", graph_make_acyclic, METH_NOARGS,
    "**make_acyclic** ()\n\n" \
    "Remove any cycles (using a depth-first search technique) and disallow cycles from this point on.\n\n" \
    "This may not be the most appropriate cycle-removing technique for all applications.\n\n" \
    "See create_spanning_tree_ for other ways to do this.\n\n"
  },
  { "is_tree", graph_is_tree, METH_NOARGS,
    "**is_tree** ()\n\n" \
    "Returns ``True`` if the graph is defined as being a tree." },
  { "is_blob", graph_is_blob, METH_NOARGS,
    "**is_blob** ()\n\n" \
    "Returns ``True`` if the graph is defined as being a blob (the opposite of a tree).  Note that this will return ``True``\n" \
    "even if the graph currently conforms to the restrictions of a tree."},
  { "make_tree", graph_make_tree, METH_NOARGS,
    "**make_tree** ()\n\n" \
    "Turns the graph into a tree by calling make_acyclic_ followed by make_undirected_.  Sets the ``BLOB`` flag to ``False``.\n\n" \
    "This approach may not be reasonable for all applications.  For other ways to convert blobs to trees, see `spanning trees`_.\n\n"
  },
  { "make_blob", graph_make_blob, METH_NOARGS,
    "**make_blob** ()\n\n" \
    "Make the graph into a blob (the opposite of a tree).  This does nothing except set the ``BLOB`` flag.\n" },
  { "is_multi_connected", graph_is_multi_connected, METH_NOARGS,
    "**is_multi_connected** ()\n\n" \
    "Returns ``True`` if the graph is defined as being multi-connected (i.e. multiple edges between a single pair of nodes).\n" \
    "Note that this returns ``True`` even if there are no multi-connections in the graph."
  },
  { "is_singly_connected", graph_is_singly_connected, METH_NOARGS,
    "**is_singly_connected** ()\n\n" \
    "Returns ``True`` if the graph is defined as being singly-connected (i.e. at most one edge between a single pair of nodes).\n" \
    "Note that this will return ``False`` if the graph is defined as multi-connected, even if it contains no multi-connections.\n\n" 
  },
  { "make_multi_connected", graph_make_multi_connected, METH_NOARGS,
    "**make_multi_connected** ()\n\n" \
    "Allow the graph to be multi-connected from this point on.  This does nothing except set the ``MULTI_CONNECTED`` flag." 
  },
  { "make_singly_connected", graph_make_singly_connected, METH_VARARGS,
    "**make_singly_connected** ()\n\n" \
    "For each pair of nodes, leave only one remaining edge in either direction.\n" \
    "Restrict the graph to being singly-connected from this point on." 
  },
  { "is_self_connected", graph_is_self_connected, METH_NOARGS,
    "**is_self_connected** ()\n\n" \
    "Returns ``True`` if the graph is defined as self-connected (having edges that point from one node to that same node.)\n" \
    "Note that this returns ``True`` even if the graph does not have any self-connections.\n"
  },
  { "make_self_connected", graph_make_self_connected, METH_VARARGS,
    "**make_self_connected** ()\n\n" \
    "Allow the graph to be self-conncted from this point on.  This does nothing except set the ``SELF_CONNECTED`` flag.\n"
  },
  { "make_not_self_connected", graph_make_not_self_connected, METH_NOARGS,
    "**make_not_self_connected** ()\n\n" \
    "Remove all self-connections and restrict the graph to have no self-connections from this point on." 
  },
  { "get_node", graph_get_node, METH_O,
    "**get_node** (*value*)\n\n" \
    "Returns the ``Node`` object identified by the given *value*.\n\n"
    "Raises a ``ValueError`` exception if there is no node associated with the given *value*.\n\n"
  },
  { "get_nodes", graph_get_nodes, METH_NOARGS,
    "**get_nodes** ()\n\n" \
    "Returns a lazy iterator over all nodes in the graph.  The ordering of the nodes is undefined.\n" \
  },
  { "get_subgraph_roots", graph_get_subgraph_roots, METH_NOARGS,
    "**get_subgraph_roots** ()\n\n" \
    "Returns a lazy iterator over each of the subgraph roots.  Performing a breadth-first or depth-first search\n" \
    "from each of this notes will visit every node in the graph.\n\n"
  },
  { "has_node", graph_has_node, METH_O,
    "**has_node** (*value*)\n\n" \
    "Returns ``True`` if graph has a node identified by *value*.\n\n" 
  },
  { "get_edges", graph_get_edges, METH_NOARGS,
    "**get_edges** ()\n\n" \
    "Returns an iterator over all edges in the graph.  The ordering of the edges is undefined.\n\n" 
  },
  { "has_edge", graph_has_edge, METH_VARARGS,
    "**has_edge** (*from_value*, *to_value*)\n\n" \
    "  *or*\n\n**has_edge** (*from_node*, *to_node*)\n\n" \
    "  *or*\n\n**has_edge** (*edge*)\n\n" \
    "Returns ``True`` if graph contains the given edge.  The edge can be specified as either a pair of values identifying nodes,\n" \
    "a pair of ``Node`` objects, or a single ``Edge`` object."
  },
  { "size_of_subgraph", graph_size_of_subgraph, METH_O,
    "**size_of_subgraph** (*value*)\n\n  *or*\n\n**size_of_subgraph** (*node*)\n\n" \
    "Returns the size of the subgraph rooted at the given node.  In other words, this returns the\n" \
    "number of nodes reachable from the given node."
  },
  { "is_fully_connected", graph_is_fully_connected, METH_NOARGS,
    "**is_fully_connected** ()\n\n" \
    "Returns ``True`` if there is only one subgraph in the graph." 
  },
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
  GraphType.tp_alloc = NULL; // PyType_GenericAlloc;
  GraphType.tp_free = NULL; // _PyObject_Del;
  GraphType.tp_methods = graph_methods;
  GraphType.tp_getset = graph_getset;
  GraphType.tp_weaklistoffset = 0;
  GraphType.tp_doc = "**Graph** (*flags* = ``FREE``)\n\n" \
    "Construct a new graph.\n\n" \
    "The *flags* are used to set certain restrictions on the graph.  When adding an edge\n" \
    "violates one of these restrictions, the edge is not added and ``None`` is returned.  Note\n" \
    "that exceptions are not raised.  The graph type may be changed at any time after creation\n" \
    "using methods such as make_directed_ or make_undirected_, but conversion may take some time.\n\n" \
    "The *flags* may be any combination of the following values (use bitwise-or to combine flags). The values\n" \
    "of these flags are defined in the ``graph`` module.  By default, all flags are ``True``:\n\n" \
    "  - ``DIRECTED``:\n\n" \
    "       When ``True``, the graph will have directed edges.  Nodes will only\n" \
    "       traverse to other nodes in the direction of the edge.  (Implementation detail: When\n" \
    "       ``False``, each edge will be represented by two edges, one pointing in each direction.\n\n" \
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
    "In addition to these raw flags, there are some convenience values for common combinations of these\n" \
    "flags.\n\n" \
    "  - ``FREE``: Equivalent to all flags being set.  There are no restrictions on the graph morphology.\n\n" \
    "  - ``TREE``: Tree structure (no flags set).\n\n" \
    "  - ``FLAG_DAG``: Directed, acyclic graph.\n\n" \
    "  - ``UNDIRECTED``: Undirected, cyclic graph.\n\n";

  PyType_Ready(&GraphType);
  PyDict_SetItemString(d, "Graph", (PyObject*)&GraphType);
}
