/*
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

#include "iterator.hpp"

// CONCRETE ITERATORS

// struct BFSIterator : IteratorObject {
int BFSIterator::init(GraphObject* graph, Node* root) {
  m_node_queue = new NodeQueue();
  NodeVector::iterator i = graph->m_nodes->begin();
  for (; i != graph->m_nodes->end(); ++i)
    NP_VISITED(*i) = false;
  m_node_queue->push(root);
  NP_VISITED(root) = true;
  return 1;
}
Node* BFSIterator::next_node(IteratorObject* self) {
  BFSIterator* so = (BFSIterator*)self;
  if (so->m_node_queue->empty()) {
    return 0;
  }
  Node* node = so->m_node_queue->front();
  so->m_node_queue->pop();
  for (EdgeList::iterator i = node->m_edges.begin();
       i != node->m_edges.end(); ++i) {
    Node* subnode = (*i)->traverse(node);
    if (!NP_VISITED(subnode)) {
      NP_VISITED(subnode) = true;
      so->m_node_queue->push(subnode);
    }
  }
  return node;
}
PyObject* BFSIterator::next(IteratorObject* self) {
  Node* node = BFSIterator::next_node(self);
  if (node)
    return nodeobject_new(node);
  return 0;
}


// struct DFSIterator : IteratorObject {
int DFSIterator::init(GraphObject* graph, Node* root) {
  m_node_stack = new NodeStack();
  NodeVector::iterator i = graph->m_nodes->begin();
  for (; i != graph->m_nodes->end(); ++i)
    NP_VISITED(*i) = false;
  m_node_stack->push(root);
  NP_VISITED(root) = true;
  return 1;
}
Node* DFSIterator::next_node(IteratorObject* self) {
  DFSIterator* so = (DFSIterator*)self;
  if (so->m_node_stack->empty()) {
    return 0;
  }
  Node* node = so->m_node_stack->top();
  so->m_node_stack->pop();
  for (EdgeList::iterator i = node->m_edges.begin();
       i != node->m_edges.end(); ++i) {
    Node* subnode = (*i)->traverse(node);
    if (!NP_VISITED(subnode)) {
      NP_VISITED(subnode) = true;
      so->m_node_stack->push(subnode);
    }
  }
  return node;
}
PyObject* DFSIterator::next(IteratorObject* self) {
  Node* node = DFSIterator::next_node(self);
  if (node)
    return nodeobject_new(node);
  return 0;
}
