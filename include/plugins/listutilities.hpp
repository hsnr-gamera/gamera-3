/*
 *
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

#ifndef mgd04302003_listutilities
#define mgd04302003_listutilities

#include <Python.h>

using namespace Gamera;

int permute_list(PyObject* list) {
  if (!PyList_Check(list)) {
    PyErr_Format(PyExc_TypeError, "Must pass a Python list to permute list.");
    return 0;
  }
  size_t n = PyList_Size(list);
  size_t j = 1;
  while (j < n && PyObject_Compare(PyList_GET_ITEM(list, j - 1), PyList_GET_ITEM(list, j)) > -1)
    ++j;
  if (j >= n)
    return 0;
  size_t l = 0;
  PyObject* tmp = PyList_GET_ITEM(list, j);
  while (PyObject_Compare(PyList_GET_ITEM(list, l), tmp) > -1)
    ++l;
  
  PyList_SET_ITEM(list, j, PyList_GET_ITEM(list, l));
  PyList_SET_ITEM(list, l, tmp);

  size_t k = 0;
  l = j - 1;
  while (k < l) {
    tmp = PyList_GET_ITEM(list, k);
    PyList_SET_ITEM(list, k, PyList_GET_ITEM(list, l));
    PyList_SET_ITEM(list, l, tmp);
    ++k;
    --l;
  }
  return 1;
}

#endif
