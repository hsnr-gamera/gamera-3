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
#ifndef _GRAPHDATAPYOBJECT_HPP_367750A70EE140
#define _GRAPHDATAPYOBJECT_HPP_367750A70EE140

#include <Python.h>
#include "graphdata.hpp"

namespace Gamera { namespace GraphApi {

   
   
// -----------------------------------------------------------------------------
/** GraphDataPyObject holds data for a node and pointer to a possibly delivered 
 * NodeObject used in the Python wrapper
 * */
struct GraphDataPyObject: public GraphData {
   PyObject* data;
   PyObject* _node;
   
   GraphDataPyObject(PyObject* d = NULL) {
      data = d;
      _node = NULL;
      incref();
   }

   ~GraphDataPyObject() {
      decref();
   }


   GraphDataPyObject& operator=(const GraphDataPyObject& other) {
      data = other.data;
      _node = other._node;
      incref();
      return *this;
   }

   void incref() {
      if(data != NULL)
         Py_INCREF(data);
      if(_node != NULL)
         Py_INCREF(_node);
   }

   void decref() {
      if(data != NULL)
         Py_DECREF(data);
      if(_node != NULL)
         Py_DECREF(_node);
   }

   int compare(const GraphData& b) {
      return PyObject_Compare(data, 
            dynamic_cast<const GraphDataPyObject&>(b).data);
   }


   GraphData* copy() {
      GraphData *a = new GraphDataPyObject(data);
      return a;
   }
};



}} //end Gamera::GraphApi
#endif /* _GRAPHDATAPYOBJECT_HPP_367750A70EE140 */

