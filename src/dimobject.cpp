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

#define GAMERACORE_INTERNAL
#include "gameramodule.hpp"

using namespace Gamera;

extern "C" {
  static PyObject* dim_new(PyTypeObject* pytype, PyObject* args,
			    PyObject* kwds);
  static void dim_dealloc(PyObject* self);
  static int dim_set_nrows(PyObject* self, PyObject* value);
  static PyObject* dim_get_nrows(PyObject* self);
  static int dim_set_ncols(PyObject* self, PyObject* value);
  static PyObject* dim_get_ncols(PyObject* self);
  static PyObject* dim_richcompare(PyObject* a, PyObject* b, int op);
  static PyObject* dim_repr(PyObject* self);
}

static PyTypeObject DimType = {
  PyObject_HEAD_INIT(NULL)
  0,
};

static PyGetSetDef dim_getset[] = {
  { (char *)"nrows", (getter)dim_get_nrows, (setter)dim_set_nrows,
    (char *)"(int property get/set)\n\nThe current number of rows", 0},
  { (char *)"ncols", (getter)dim_get_ncols, (setter)dim_set_ncols,
    (char *)"(int property get/set)\n\nthe current number of columns", 0},
  { NULL }
};

PyTypeObject* get_DimType() {
  return &DimType;
}

static PyObject* dim_new(PyTypeObject* pytype, PyObject* args,
			  PyObject* kwds) {
  int x, y;
  if (PyArg_ParseTuple(args, CHAR_PTR_CAST "ii:Dim.__init__", &x, &y) <= 0)
    return 0;
  DimObject* so;
  so = (DimObject*)pytype->tp_alloc(pytype, 0);
  so->m_x = new Dim((size_t)x, (size_t)y);
  return (PyObject*)so;
}

static void dim_dealloc(PyObject* self) {
  DimObject* x = (DimObject*)self;
  delete x->m_x;
  self->ob_type->tp_free(self);
}

#define CREATE_GET_FUNC(name) static PyObject* dim_get_##name(PyObject* self) {\
  Dim* x = ((DimObject*)self)->m_x; \
  return PyInt_FromLong((int)x->name()); \
}

#define CREATE_SET_FUNC(name) static int dim_set_##name(PyObject* self, PyObject* value) {\
  Dim* x = ((DimObject*)self)->m_x; \
  x->name((size_t)PyInt_AS_LONG(value)); \
  return 0; \
}

CREATE_GET_FUNC(nrows)
CREATE_SET_FUNC(nrows)
CREATE_GET_FUNC(ncols)
CREATE_SET_FUNC(ncols)

static PyObject* dim_richcompare(PyObject* a, PyObject* b, int op) {
  if (!is_DimObject(a) || !is_DimObject(b)) {
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
  }

  Dim& ap = *((DimObject*)a)->m_x;
  Dim& bp = *((DimObject*)b)->m_x;

  /*
    Only equality and inequality make sense.
  */
  bool cmp;
  switch (op) {
  case Py_LT:
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
  case Py_LE:
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
  case Py_EQ:
    cmp = ap == bp;
    break;
  case Py_NE:
    cmp = ap != bp;
    break;
  case Py_GT:
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
  case Py_GE:
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
  default:
    return 0; // cannot happen
  }
  if (cmp) {
    Py_INCREF(Py_True);
    return Py_True;
  } else {
    Py_INCREF(Py_False);
    return Py_False;
  }
}

static PyObject* dim_repr(PyObject* self) {
  Dim* x = ((DimObject*)self)->m_x;
  return PyString_FromFormat("<gameracore.Dim ncols: %i nrows: %i>",
			     (int)x->ncols(), (int)x->nrows());
}

void init_DimType(PyObject* module_dict) {
  DimType.ob_type = &PyType_Type;
  DimType.tp_name = CHAR_PTR_CAST "gameracore.Dim";
  DimType.tp_basicsize = sizeof(DimObject);
  DimType.tp_dealloc = dim_dealloc;
  DimType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  DimType.tp_new = dim_new;
  DimType.tp_getattro = PyObject_GenericGetAttr;
  DimType.tp_alloc = NULL; // PyType_GenericAlloc;
  DimType.tp_richcompare = dim_richcompare;
  DimType.tp_getset = dim_getset;
  DimType.tp_free = NULL; // _PyObject_Del;
  DimType.tp_repr = dim_repr;
  DimType.tp_doc = CHAR_PTR_CAST
"__init__(Int *ncols*, Int *nrows*)\n\n"
"Dim stores a dimension (*ncols*, *nrows*)\n\n";
  PyType_Ready(&DimType);
  PyDict_SetItemString(module_dict, "Dim", (PyObject*)&DimType);
}
