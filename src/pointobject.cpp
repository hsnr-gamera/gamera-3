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
  static PyObject* point_new(PyTypeObject* pytype, PyObject* args,
			    PyObject* kwds);
  static void point_dealloc(PyObject* self);
  // get/set
  static int point_set_x(PyObject* self, PyObject* value);
  static PyObject* point_get_x(PyObject* self);
  static int point_set_y(PyObject* self, PyObject* value);
  static PyObject* point_get_y(PyObject* self);
  static PyObject* point_richcompare(PyObject* a, PyObject* b, int op);
  // methods
  static PyObject* point_move(PyObject* self, PyObject* args);
  static PyObject* point_repr(PyObject* self);
}

static PyTypeObject PointType = {
  PyObject_HEAD_INIT(NULL)
  0,
};

static PyGetSetDef point_getset[] = {
  { "x", (getter)point_get_x, (setter)point_set_x, "the current x value", 0},
  { "y", (getter)point_get_y, (setter)point_set_y, "the current y value", 0},
  { NULL }
};

static PyMethodDef point_methods[] = {
  { "move", point_move, METH_VARARGS },
  { NULL }
};


PyTypeObject* get_PointType() {
  return &PointType;
}

bool is_PointObject(PyObject* x) {
  if (PyObject_TypeCheck(x, &PointType))
    return true;
  else
    return false;
}

PyObject* create_PointObject(const Point& p) {
  PointObject* so;
  so = (PointObject*)PointType.tp_alloc(&PointType, 0);
  so->m_x = new Point(p);
  return (PyObject*)so;
}

static PyObject* point_new(PyTypeObject* pytype, PyObject* args,
			  PyObject* kwds) {
  int x, y;
  if (PyArg_ParseTuple(args, "ii", &x, &y) <= 0)
    return 0;
  PointObject* so;
  so = (PointObject*)pytype->tp_alloc(pytype, 0);
  so->m_x = new Point((size_t)x, (size_t)y);
  return (PyObject*)so;
}

static void point_dealloc(PyObject* self) {
  PointObject* x = (PointObject*)self;
  delete x->m_x;
  self->ob_type->tp_free(self);
}

#define CREATE_GET_FUNC(name) static PyObject* point_get_##name(PyObject* self) {\
  Point* x = ((PointObject*)self)->m_x; \
  return Py_BuildValue("i", (int)x->name()); \
}

#define CREATE_SET_FUNC(name) static int point_set_##name(PyObject* self, PyObject* value) {\
  Point* x = ((PointObject*)self)->m_x; \
  x->name((size_t)PyInt_AS_LONG(value)); \
  return 0; \
}

CREATE_GET_FUNC(x)
CREATE_GET_FUNC(y)
CREATE_SET_FUNC(x)
CREATE_SET_FUNC(y)

static PyObject* point_move(PyObject* self, PyObject* args) {
  Point* x = ((PointObject*)self)->m_x;
  int xv, y;
  if (PyArg_ParseTuple(args, "ii", &xv, &y) <= 0)
    return 0;
  x->move(xv, y);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject* point_richcompare(PyObject* a, PyObject* b, int op) {
  if (!is_PointObject(a) || !is_PointObject(b)) {
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
  }

  Point& ap = *((PointObject*)a)->m_x;
  Point& bp = *((PointObject*)b)->m_x;

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

static PyObject* point_repr(PyObject* self) {
  Point* x = ((PointObject*)self)->m_x;
  return PyString_FromFormat("<gameracore.Point x: %i y: %i>",
			     x->x(), x->y());
}

void init_PointType(PyObject* module_dict) {
  PointType.ob_type = &PyType_Type;
  PointType.tp_name = "gameracore.Point";
  PointType.tp_basicsize = sizeof(PointObject);
  PointType.tp_dealloc = point_dealloc;
  PointType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  PointType.tp_new = point_new;
  PointType.tp_getattro = PyObject_GenericGetAttr;
  PointType.tp_alloc = PyType_GenericAlloc;
  PointType.tp_richcompare = point_richcompare;
  PointType.tp_getset = point_getset;
  PointType.tp_free = _PyObject_Del;
  PointType.tp_methods = point_methods;
  PointType.tp_repr = point_repr;
  PyDict_SetItemString(module_dict, "Point", (PyObject*)&PointType);
}
