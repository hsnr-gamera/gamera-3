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

//////////////////////////////////////////////////////////////////////////////
// Iterators

/* These objects make it (somewhat) easier to create Python iterator objects.
   To create a Python iterator, inherit from IteratorObject and define:
     An initializer (constructor):
       void init([any signature]);
     An iteration function:
       PyObject* next(IteratorObject* self);
     An optional destructor method:
       static void dealloc(IteratorObject* self);
     Plus any data members needed to keep the iteration going.

   Within your 'next' function, simply return 0 to stop iteration.  All the
   other details of handling Python iteration are taken care of by this base
   class.
     
   To create an iterator and return it from a function:
      MyIterator* iterator = iterator_new_simple<MyIterator>();
      iterator->init([initialization parameters]);
      return (PyObject*)iterator;

   See also below the BasicIterator template which makes it easy to iterate
   through C++ STL sequences.
*/

#ifndef mgd010103_iterator_hpp
#define mgd010103_iterator_hpp

#include "graphlib.hpp"
#include "node.hpp"
#include "edge.hpp"

struct IteratorObject {
  PyObject_HEAD
  PyObject*(*m_fp_next)(IteratorObject*);
  void(*m_fp_dealloc)(IteratorObject*);
  static void dealloc(IteratorObject* self) { };
};

void init_IteratorType();
PyTypeObject* get_IteratorType();
template<class T>
T* iterator_new();

template<class T>
T* iterator_new() {
  IteratorObject* so;
  PyTypeObject* type = get_IteratorType();
  type->tp_basicsize = sizeof(T);
  so = (IteratorObject*)(type->tp_alloc(type, 0));
  so->m_fp_next = T::next;
  so->m_fp_dealloc = T::dealloc;
  return (T*)so;
}

// CONCRETE ITERATORS

template<class T>
struct BasicIterator : IteratorObject {
  int init(typename T::iterator begin, typename T::iterator end) {
    m_it = begin;
    m_end = end;
    return 1;
  }
  static PyObject* next(IteratorObject* self) {
    BasicIterator<T>* so = (BasicIterator<T>*)self;
    if (so->m_it == so->m_end)
      return 0;
    return (PyObject*)*((so->m_it)++);
  }
  typename T::iterator m_it, m_end;
};

template<class T>
struct EdgeIterator : IteratorObject {
  int init(typename T::iterator begin, typename T::iterator end) {
    m_it = begin;
    m_end = end;
    return 1;
  }
  static PyObject* next(IteratorObject* self) {
    EdgeIterator<T>* so = (EdgeIterator<T>*)self;
    if (so->m_it == so->m_end)
      return 0;
    return edgeobject_new(*((so->m_it)++));
  }
  typename T::iterator m_it, m_end;
};

template<class T>
struct NodeEdgeIterator : IteratorObject {
  int init(Node* node, typename T::iterator begin, typename T::iterator end) {
    m_node = node;
    m_it = begin;
    m_end = end;
    return 1;
  }
  static PyObject* next(IteratorObject* self) {
    NodeEdgeIterator<T>* so = (NodeEdgeIterator<T>*)self;
    if (so->m_it == so->m_end)
      return 0;
    return nodeobject_new((*((so->m_it)++))->traverse(so->m_node));
  }
  typename T::iterator m_it, m_end;
  Node* m_node;
};

template<class T>
struct NodeEdgeTupleIterator : IteratorObject {
  int init(Node* node, typename T::iterator begin, typename T::iterator end) {
    m_node = node;
    m_it = begin;
    m_end = end;
    return 1;
  }
  static PyObject* next(IteratorObject* self) {
    NodeEdgeTupleIterator<T>* so = (NodeEdgeTupleIterator<T>*)self;
    if (so->m_it == so->m_end)
      return 0;
    Edge* edge = (*((so->m_it)++));
    Node* node = edge->traverse(so->m_node);
    EdgeObject* edge_object = edgeobject_new(edge);
    NodeObject* node_object = nodeobject_new(node);
    PyObject* tuple = PyTuple_New(2);
    PyTuple_SET_ITEM(tuple, 0, edge_object);
    PyTuple_SET_ITEM(tuple, 1, node_object);
    return tuple;
  }
  typename T::iterator m_it, m_end;
  Node* m_node;
};

template<class T>
struct NodeIterator : IteratorObject {
  int init(typename T::iterator begin, typename T::iterator end) {
    m_it = begin;
    m_end = end;
    return 1;
  }
  static PyObject* next(IteratorObject* self) {
    NodeIterator<T>* so = (NodeIterator<T>*)self;
    if (so->m_it == so->m_end)
      return 0;
    return nodeobject_new(*((so->m_it)++));
  }
  typename T::iterator m_it, m_end;
};


template<class T>
struct MapValueIterator : IteratorObject {
  int init(typename T::const_iterator begin, typename T::const_iterator end) {
    m_it = begin;
    m_end = end;
    return 1;
  }
  static PyObject* next(IteratorObject* self) {
    BasicIterator<T>* so = (BasicIterator<T>*)self;
    if (so->m_it == so->m_end)
      return 0;
    return (PyObject*)(*((so->m_it)++)).second;
  }
  typename T::const_iterator m_it, m_end;
};

template<class T>
struct MapKeyIterator : IteratorObject {
  int init(typename T::iterator begin, typename T::iterator end) {
    m_it = begin;
    m_end = end;
    return 1;
  }
  static PyObject* next(IteratorObject* self) {
    BasicIterator<T>* so = (BasicIterator<T>*)self;
    if (so->m_it == so->m_end)
      return 0;
    return (PyObject*)(*((so->m_it)++)).first;
  }
  typename T::iterator m_it, m_end;
};

struct BFSIterator : IteratorObject {
  int init(GraphObject* graph, Node* root);
  static Node* next_node(IteratorObject* self);
  static PyObject* next(IteratorObject* self);
  static void dealloc(IteratorObject* self) { 
    delete ((BFSIterator*)(self))->m_node_queue; 
  };
  NodeQueue* m_node_queue;
};

struct DFSIterator : IteratorObject {
  int init(GraphObject* graph, Node* root);
  static Node* next_node(IteratorObject* self);
  static PyObject* next(IteratorObject* self);
  static void dealloc(IteratorObject* self) { 
    delete ((DFSIterator*)(self))->m_node_stack; 
  };
  NodeStack* m_node_stack;
};

#endif
