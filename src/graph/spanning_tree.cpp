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
    for (EdgeList::iterator j = node->m_out_edges->begin();
	 j != node->m_out_edges->end(); ++j)
      if (!NP_VISITED((*j)->m_to_node)) {
	Node* new_node2 = graph_add_node(tree, (*j)->m_to_node->m_data);
	graph_add_edge(tree, new_node1, new_node2, (*j)->m_cost);
	node_stack.push((*j)->m_to_node);
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
       i != so->m_nodes->end(); ++i)
    for (EdgeList::iterator j = (*i)->m_out_edges->begin();
	 j != (*i)->m_out_edges->end(); ++j) {
      EP_VISITED(*j) = false;
  
  for (NodeVector::iterator i = so->m_nodes->begin();
       i != so->m_nodes->end(); ++i)
    for (EdgeList::iterator j = (*i)->m_out_edges->begin();
	 j != (*i)->m_out_edges->end(); ++j) {
      if (!EP_VISITED(*j)) {
	EP_VISITED(*j) = true;
	if ((*j)->m_other)
	  EP_VISITED((*j)->m_other) = true;
	edge_queue.push(*j);
      }
    }

  for (NodeVector::iterator i = so->m_nodes->begin();
       i != so->m_nodes->end(); ++i)
    for (EdgeList::iterator j = (*i)->m_out_edges->begin();
			j != (*i)->m_out_edges->end(); ++j) {
		if (!EP_VISITED(*j)) {
		  EP_VISITED(*j) = true;
		  EP_VISITED((*j)->m_other) = true;
		  edge_queue.push(*j);
		  // Increase reference count, since we're about to delete all edges
		  Py_INCREF((PyObject*)(*j));
		}
    }
  
  size_t flags = so->m_flags;
  UNSET_FLAG(flags, FLAG_CYCLIC);
  GraphObject* tree = graph_new(flags);
  tree->m_nodes->reserve(so->m_nodes->size());
  for (NodeVector::iterator i = so->m_nodes->begin();
       i != so->m_nodes->end(); ++i)
    graph_add_node(tree, (*i)->m_data);
  
  size_t divisor = 1;
  if (!HAS_FLAG(tree->m_flags, FLAG_DIRECTED))
    divisor = 2;

  while (!edge_queue.empty() && tree->m_nedges / divisor < tree->m_nodes->size() - 1) {
    Edge* edge = edge_queue.top();
    edge_queue.pop();
    graph_add_edge(tree, edge->m_from_node->m_data, edge->m_to_node->m_data, edge->m_cost, edge->m_label);
  }
  return tree;
}

PyObject* graph_create_minimum_spanning_tree(PyObject* self, PyObject* _) {
  GraphObject* so = ((GraphObject*)self);
  return (PyObject*)graph_create_minimum_spanning_tree(so);
}
