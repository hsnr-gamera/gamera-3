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

#include "shortest_path.hpp"

struct djikstra_queue_comp_func
{
  bool operator()(NodeObject* const& a, NodeObject* const& b) const { 
    return NP_DISTANCE(a) > NP_DISTANCE(b);
  }
};

NodeList* graph_djikstra_shortest_path(GraphObject* so, NodeObject* root) {
  typedef std::priority_queue<NodeObject*, std::vector<NodeObject*>, djikstra_queue_comp_func> NodeQueue;
  NodeList* main_node_list = new NodeList();

  if (HAS_FLAG(so->m_flags, FLAG_CYCLIC)) {
    DFSIterator* iterator = iterator_new_simple<DFSIterator>();
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

PyObject* graph_djikstra_shortest_path(PyObject* self, PyObject* pyobject) {
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

PyObject* graph_djikstra_all_pairs_shortest_path(PyObject* self, PyObject* pyobject) {
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

PyObject* graph_all_pairs_shortest_path(PyObject* self, PyObject* args) {
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

