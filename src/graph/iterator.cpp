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

#include "iterator.hpp"

extern "C" {
  static PyObject* iterator_get_iter(PyObject* self);
  static PyObject* iterator_next(PyObject* self);
}

static PyTypeObject IteratorType = {
  PyObject_HEAD_INIT(NULL)
  0,
};

PyTypeObject* get_IteratorType() {
  return &IteratorType;
}

void iterator_dealloc(PyObject* self) {
  IteratorObject* so = (IteratorObject*)self;
#ifdef DEBUG
  std::cerr << "iterator dealloc\n";
#endif
  (*(so->m_fp_dealloc))(so);
  self->ob_type->tp_free(self);
}

PyObject* iterator_get_iter(PyObject* self) {
  Py_INCREF(self);
  return self;
}

PyObject* iterator_next(PyObject* self) {
  IteratorObject* so = (IteratorObject*)self;
  PyObject* result = (*(so->m_fp_next))(so);
  if (result == NULL) {
    PyErr_SetString(PyExc_StopIteration, "");
    return 0;
  }
  return result;
}

void init_IteratorType() {
  IteratorType.ob_type = &PyType_Type;
  IteratorType.tp_name = "gamera.graph.Iterator";
  IteratorType.tp_basicsize = sizeof(IteratorObject);
  IteratorType.tp_dealloc = iterator_dealloc;
  IteratorType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  IteratorType.tp_getattro = PyObject_GenericGetAttr;
  IteratorType.tp_alloc = PyType_GenericAlloc;
  IteratorType.tp_free = _PyObject_Del;
  IteratorType.tp_iter = iterator_get_iter;
  IteratorType.tp_iternext = iterator_next;
  PyType_Ready(&IteratorType);
}

// CONCRETE ITERATORS

// struct BFSIterator : IteratorObject {
int BFSIterator::init(GraphObject* graph, Node* root) {
  m_node_queue = new NodeQueue();
  m_node_queue->push(root);
  NodeVector::iterator i = graph->m_nodes->begin();
  for (; i != graph->m_nodes->end(); ++i)
    NP_VISITED(*i) = false;
  NP_VISITED(root) = true;
  return 1;
}
inline Node* BFSIterator::next_node(IteratorObject* self) {
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
  m_node_stack->push(root);
  NodeVector::iterator i = graph->m_nodes->begin();
  for (; i != graph->m_nodes->end(); ++i)
    NP_VISITED(*i) = false;
  NP_VISITED(root) = true;
  return 1;
}
inline Node* DFSIterator::next_node(IteratorObject* self) {
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

// struct EdgeIterator : IteratorObject {
int AllEdgeIterator::init(NodeVector::iterator begin, NodeVector::iterator end) {
  m_it = begin;
  m_end = end;
  m_edge_it = (*begin)->m_edges.begin();
  m_edge_end = (*begin)->m_edges.end();
  return 1;
}
PyObject* AllEdgeIterator::next(IteratorObject* self) {
  AllEdgeIterator* so = (AllEdgeIterator*)self;
  while (so->m_edge_it == so->m_edge_end) {
    so->m_it++;
    if (so->m_it == so->m_end) {
      return 0;
    }
    so->m_edge_it = (*(so->m_it))->m_edges.begin();
    so->m_edge_end = (*(so->m_it))->m_edges.end();
  }
  return edgeobject_new(*((so->m_edge_it)++));
}
