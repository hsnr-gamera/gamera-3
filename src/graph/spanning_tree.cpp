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

#include "spanning_tree.hpp"

GraphObject* graph_create_spanning_tree(GraphObject* so, Node* root) {
  GraphObject* tree = graph_new(FLAG_DAG);
  NodeVector::iterator i = so->m_nodes->begin();
  for (; i != so->m_nodes->end(); ++i)
    NP_VISITED(*i) = false;
  NodeStack node_stack;
  node_stack.push(root);
  while (!node_stack.empty()) {
    Node* node = node_stack.top();
    node_stack.pop();
    NP_VISITED(node) = true;
    Node* new_node1 = graph_add_node(tree, node->m_data);
    for (EdgeList::iterator j = node->m_edges.begin();
			j != node->m_edges.end(); ++j) {
      Node* inner_node = (*j)->traverse(node);
      if (!NP_VISITED(inner_node)) {
		  Node* new_node2 = graph_add_node(tree, inner_node->m_data);
		  graph_add_edge(tree, new_node1, new_node2, (*j)->m_cost);
		  node_stack.push(inner_node);
		  NP_VISITED(inner_node) = true;
      }
    }
  }
  return tree;
}

PyObject* graph_create_spanning_tree(PyObject* self, PyObject* pyobject) {
  GraphObject* so = ((GraphObject*)self);
  Node* root;
  root = graph_find_node(so, pyobject);
  if (root == NULL)
    return 0;
  return (PyObject*)graph_create_spanning_tree(so, root);
}


struct minimum_spanning_queue_comp_func
{
  bool operator()(Edge* const& a, Edge* const& b) const { 
    return a->m_cost > b->m_cost;
  }
};

GraphObject* graph_create_minimum_spanning_tree(GraphObject* so) {
  // Kruskal's algorithm
  typedef std::priority_queue<Edge*, std::vector<Edge*>,
    minimum_spanning_queue_comp_func> EdgeQueue;
  EdgeQueue edge_queue;

  for (NodeVector::iterator i = so->m_nodes->begin();
       i != so->m_nodes->end(); ++i) {
	 for (EdgeList::iterator j = (*i)->m_edges.begin();
	      j != (*i)->m_edges.end(); ++j) {
	   edge_queue.push(*j);
	 }
  }
  
  size_t flags = so->m_flags;
  UNSET_FLAG(flags, FLAG_CYCLIC);
  GraphObject* tree = graph_new(flags);
  tree->m_nodes->reserve(so->m_nodes->size());
  for (NodeVector::iterator i = so->m_nodes->begin();
       i != so->m_nodes->end(); ++i) {
    graph_add_node(tree, (*i)->m_data);
  }
  
  while (!edge_queue.empty() && tree->m_edges->size() < tree->m_nodes->size() - 1) {
    Edge* edge = edge_queue.top();
    edge_queue.pop();
    graph_add_edge(tree, edge->m_from_node->m_data, edge->m_to_node->m_data,
		   edge->m_cost, edge->m_label);
  }
  return tree;
}

PyObject* graph_minimum_spanning_tree_unique_distances(GraphObject* so, PyObject* images,
																		 PyObject* uniq_dists) {
  if (!PyList_Check(uniq_dists) || !PyList_Check(images)) {
	 PyErr_SetString(PyExc_TypeError, "uniq_dists and images must be a list.");
	 return 0;
  }
  
  // get the graph ready
  graph_remove_all_edges(so);
  graph_make_acyclic(so);
  
  // Add the nodes to the graph and build our map for later
  int images_len = PyList_Size(images);
  int i;
  
  // create the mst using kruskal
  i = 0;
  int uniq_dists_len = PyList_Size(uniq_dists);
  while (i < uniq_dists_len && (int(so->m_edges->size()) < (images_len - 1))) {
	 PyObject* cur_tuple = PyList_GET_ITEM(uniq_dists, i);
	 if (!PyTuple_Check(cur_tuple) || (PyTuple_GET_SIZE(cur_tuple) != 3)) {
		PyErr_SetString(PyExc_TypeError, "list didn't contain an appropriate tuple.");
		return 0;
	 }
	 PyObject* cur = PyTuple_GET_ITEM(cur_tuple, 0);
	 if (!PyFloat_Check(cur)) {
		PyErr_SetString(PyExc_TypeError, "First item in tuple must be a float.");
		return 0;
	 }
	 PyObject* a = PyTuple_GET_ITEM(cur_tuple, 1);
	 PyObject* b = PyTuple_GET_ITEM(cur_tuple, 2);
	 assert(a != b);
	 graph_add_edge(so, a, b, PyFloat_AS_DOUBLE(cur));
	 ++i;
  }
  Py_INCREF(Py_None);
  return Py_None;
}

PyObject* graph_create_minimum_spanning_tree(PyObject* self, PyObject* args) {
  PyObject* images = 0;
  PyObject* uniq_dists = 0;
  if (PyArg_ParseTuple(args, "|OO", &images, &uniq_dists) <= 0)
	 return 0;
  GraphObject* so = ((GraphObject*)self);
  if (images == 0 || uniq_dists == 0)
    return (PyObject*)graph_create_minimum_spanning_tree(so);
  else
    return graph_minimum_spanning_tree_unique_distances(so, images, uniq_dists);
}

