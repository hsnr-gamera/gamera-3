/*
 *
 * Copyright (C) 2011 Christian Brandt
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


#ifndef _ITERATOROBJECT_HPP_EBAA99C250B47F
#define _ITERATOROBJECT_HPP_EBAA99C250B47F

#include "wrapper.hpp"
#include "python_iterator.hpp"
#include "nodetraverseiterator.hpp"
#include "nodeobject.hpp"
#include "edgeobject.hpp"


/**
   This classes are wrappers from gamera's python_iterator
   to the Graph-API specific iterators.

   Node* are put in NodeObject*
   Edge* are put in EdgeObject*
   for being used in Python applications
   */



// -----------------------------------------------------------------------------
template <typename itertype>
struct NTIteratorObject: IteratorObject {
   GraphObject* _graph;
   void init(itertype* it, GraphObject* graph) {
      _iterator = it; 
      _graph = graph;
      if(_graph)
         Py_INCREF(_graph);
   };
   static PyObject* next(IteratorObject* self) {
      if(self == NULL || ((NTIteratorObject*)self)->_iterator == NULL) 
        return NULL; 
      Node *n = ((NTIteratorObject*)self)->_iterator->next();
      if (n == NULL)
         return NULL;
      return (PyObject*)node_deliver(n,((NTIteratorObject*)self)->_graph);
   }
   static void dealloc(IteratorObject* self) {
      if(((NTIteratorObject*)self)->_graph) {
         Py_DECREF(((NTIteratorObject*)self)->_graph);
      }
      delete (itertype*)((NTIteratorObject*)self)->_iterator;
   }

   itertype* _iterator;
};



// -----------------------------------------------------------------------------
template <typename itertype>
struct ETIteratorObject: IteratorObject {
   GraphObject* _graph;
   void init(itertype* it, GraphObject* graph) {
      _iterator = it;
      _graph = graph;
      if(_graph)
         Py_INCREF(_graph);
   }
   static PyObject* next(IteratorObject* self) {
      if(self == NULL || ((ETIteratorObject*)self)->_iterator == NULL || ((ETIteratorObject*)self)->_graph == NULL)
         return NULL;

      Edge *n = ((ETIteratorObject*)self)->_iterator->next();
      if (n == NULL)
         return NULL;
      return (PyObject*)edge_deliver(n,((ETIteratorObject*)self)->_graph);
   }
   static void dealloc(IteratorObject* self) {
      if(((ETIteratorObject*)self)->_graph) {
         Py_DECREF(((ETIteratorObject*)self)->_graph);
      }
      delete (itertype*)((ETIteratorObject*)self)->_iterator;
   }

   itertype* _iterator;
};

#endif /* _ITERATOROBJECT_HPP_EBAA99C250B47F */

