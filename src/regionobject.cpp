/*
 *
 * Copyright (C) 2002 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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

#include "gameramodule.hpp"

using namespace Gamera;

extern "C" {
  static PyObject* region_new(PyTypeObject* pytype, PyObject* args,
			     PyObject* kwds);
  static void region_dealloc(PyObject* self);
  static PyObject* region_get(PyObject* self, PyObject* args);
  static PyObject* region_add(PyObject* self, PyObject* args);
}

static PyTypeObject RegionType = {
  PyObject_HEAD_INIT(NULL)
  0,
};

static PyMethodDef region_methods[] = {
  { "get", region_get, METH_VARARGS },
  { "add", region_add, METH_VARARGS },
  { NULL }
};

PyTypeObject* get_RegionType() {
  return &RegionType;
}

bool is_RegionObject(PyObject* x) {
  if (PyObject_TypeCheck(x, &RegionType))
    return true;
  else
    return false;
}

PyObject* create_RegionObject(const Region& r) {
  RectObject* so;
  so = (RectObject*)RegionType.tp_alloc(&RegionType, 0);
  so->m_x = new Region(r);
  return (PyObject*)so;
}

static PyObject* region_new(PyTypeObject* pytype, PyObject* args,
			    PyObject* kwds) {
  int num_args = PyTuple_GET_SIZE(args);
  if (num_args == 4) {
    int offset_x, offset_y, nrows, ncols;
    if (PyArg_ParseTuple(args, "iiii", &offset_x, &offset_y, &nrows, &ncols)
	<= 0)
      return 0;
    RectObject* so;
    so = (RectObject*)pytype->tp_alloc(pytype, 0);
    so->m_x = new Region((size_t)offset_x, (size_t)offset_y, (size_t)nrows,
			 (size_t)ncols);
    return (PyObject*)so;
  } else if (num_args == 2) {
    PyObject *a, *b;
    if (PyArg_ParseTuple(args, "OO", &a, &b) <= 0)
      return 0;
    if (is_PointObject(a) && is_PointObject(b)) {
      RectObject* so;
      so = (RectObject*)pytype->tp_alloc(pytype, 0);
      so->m_x = new Region(*((PointObject*)a)->m_x, *((PointObject*)b)->m_x);
      return (PyObject*)so;
    } else if (is_PointObject(a) && is_SizeObject(b)) {
      RectObject* so;
      so = (RectObject*)pytype->tp_alloc(pytype, 0);
      so->m_x = new Region(*((PointObject*)a)->m_x, *((SizeObject*)b)->m_x);
      return (PyObject*)so;
    } else if (is_PointObject(a) && is_DimensionsObject(b)) {
      RectObject* so;
      so = (RectObject*)pytype->tp_alloc(pytype, 0);
      so->m_x = new Region(*((PointObject*)a)->m_x,
			   *((DimensionsObject*)b)->m_x);
      return (PyObject*)so;
    } else {
      PyErr_SetString(PyExc_TypeError, "No overloaded functions match!");
      return 0;
    }
  } else {
    PyErr_SetString(PyExc_TypeError, "No overloaded functions match!");
    return 0;
  }
}

static void region_dealloc(PyObject* self) {
  RectObject* r = (RectObject*)self;
  delete r->m_x;
  self->ob_type->tp_free(self);
}

static PyObject* region_get(PyObject* self, PyObject* args) {
  char* key;
  if (PyArg_ParseTuple(args, "s", &key) <= 0)
    return 0;
  RectObject* r = (RectObject*)self;
  Region* region = (Region*)r->m_x;
  return Py_BuildValue("d", region->get(key));
}

static PyObject* region_add(PyObject* self, PyObject* args) {
  char* key;
  double value;
  if (PyArg_ParseTuple(args, "sd", &key, &value) <= 0)
    return 0;
  RectObject* r = (RectObject*)self;
  Region* region = (Region*)r->m_x;
  region->add(key, value);
  Py_INCREF(Py_None);
  return Py_None;
}

void init_RegionType(PyObject* module_dict) {
  RegionType.ob_type = &PyType_Type;
  RegionType.tp_name = "gameracore.Region";
  RegionType.tp_basicsize = sizeof(RegionObject);
  RegionType.tp_dealloc = region_dealloc;
  RegionType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  RegionType.tp_base = get_RectType();
  RegionType.tp_methods = region_methods;
  RegionType.tp_new = region_new;
  RegionType.tp_getattro = PyObject_GenericGetAttr;
  RegionType.tp_alloc = PyType_GenericAlloc;
  RegionType.tp_free = _PyObject_Del;
  PyType_Ready(&RegionType);
  PyDict_SetItemString(module_dict, "Region", (PyObject*)&RegionType);
}
