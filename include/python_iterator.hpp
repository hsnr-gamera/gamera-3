/*
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

#ifndef mgd010105_iterator_hpp
#define mgd010105_iterator_hpp

#include "Python.h"
#include "gameramodule.hpp"

struct IteratorObject {
  PyObject_HEAD
  PyObject*(*m_fp_next)(IteratorObject*);
  void(*m_fp_dealloc)(IteratorObject*);
  static void dealloc(IteratorObject* self) { };
};

void init_IteratorType();
// PyTypeObject* get_IteratorType();

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

#endif
