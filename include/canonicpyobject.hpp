/*
 *
 * Copyright (C) 2009 Christoph Dalitz
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef cd22092009_canonicpyobject_hpp
#define cd22092009_canonicpyobject_hpp

/* CANONICPYOBJECT TYPE */

// A "canonic" wrapper class around PyObject*, to make PyObjects
// usable in STL containers, e.g. vector<canonicPyObject>
// The main point is that the proper comparison functions are called
// It is meant to be used on the C++ side only
class canonicPyObject {
public:
  PyObject* value;
  inline canonicPyObject(PyObject* c2) {value = c2;}
  inline canonicPyObject& operator=(const PyObject* c2) {
    value = (PyObject*)c2;
    return *this;
  }
  inline int operator<(const canonicPyObject& c2) const {
    return PyObject_RichCompareBool(value,c2.value,Py_LT);
  }
  inline int operator==(const canonicPyObject& c2) const {
    return PyObject_RichCompareBool(value,c2.value,Py_EQ);
  }
};



#endif
