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

GraphObject* graph_create_spanning_tree(GraphObject* so, NodeObject* root) {
  GraphObject* tree = graph_new_simple(FLAG_DAG);
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

PyObject* graph_create_spanning_tree(PyObject* self, PyObject* pyobject) {
  GraphObject* so = ((GraphObject*)self);
  NodeObject* root;
  root = graph_find_node(so, pyobject);
  if (root == NULL)
    return 0;
  return (PyObject*)graph_create_spanning_tree(so, root);
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
      EP_VISITED(*j) = false;
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
  
  graph_remove_all_edges(so);
  graph_make_acyclic(so);
  
  size_t divisor = 1;
  if (!HAS_FLAG(so->m_flags, FLAG_DIRECTED))
    divisor = 2;

  while (!edge_queue.empty() && (so->m_nedges / divisor) < so->m_nodes->size() - 1) {
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
