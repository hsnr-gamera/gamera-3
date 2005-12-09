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
  { "x", (getter)point_get_x, (setter)point_set_x, "(int property)\n\nThe current x value", 0},
  { "y", (getter)point_get_y, (setter)point_set_y, "(int property)\n\nThe current y value", 0},
  { NULL }
};

static PyMethodDef point_methods[] = {
  { "move", point_move, METH_VARARGS,
    "**move** (*x*, *y*)\n\nMoves the point to the given *x*, *y* coordinate."},
  { NULL }
};


PyTypeObject* get_PointType() {
  return &PointType;
}

static PyObject* _point_new(PyTypeObject* pytype, Point *p) {
  PointObject* so;
  so = (PointObject*)pytype->tp_alloc(pytype, 0);
  so->m_x = p;
  return (PyObject*)so;
}

static PyObject* point_new(PyTypeObject* pytype, PyObject* args,
			  PyObject* kwds) {
  int num_args = PyTuple_GET_SIZE(args);
  if (num_args == 2) {
    int x, y;
    if (PyArg_ParseTuple(args, "ii", &x, &y))
      return _point_new(pytype, new Point((size_t)x, (size_t)y));
  }

  PyErr_Clear();

  if (num_args == 1) {
    PyObject* py_point;
    if (PyArg_ParseTuple(args, "O", &py_point)) {
      try {
	return _point_new(pytype, new Point(coerce_Point(py_point)));
      } catch (std::invalid_argument e) {
	;
      }
    }
  }

  PyErr_Clear();
  PyErr_SetString(PyExc_TypeError, "Invalid arguments to Point constructor.  Must be Point(int x, int y)");
  return 0;
}

static void point_dealloc(PyObject* self) {
  PointObject* x = (PointObject*)self;
  delete x->m_x;
  self->ob_type->tp_free(self);
}

#define CREATE_GET_FUNC(name) static PyObject* point_get_##name(PyObject* self) {\
  Point* x = ((PointObject*)self)->m_x; \
  return PyInt_FromLong((int)x->name()); \
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
  if (PyArg_ParseTuple(args, "ii:move", &xv, &y) <= 0)
    return 0;
  x->move(xv, y);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject* point_richcompare(PyObject* a, PyObject* b, int op) {
  if (!is_PointObject(a)) {
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
  }

  Point ap = *((PointObject*)a)->m_x;
  Point bp;

  try {
    bp = coerce_Point(b);
  } catch (std::invalid_argument e) {
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
  }

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
			     (int)x->x(), (int)x->y());
}

static long point_hash(PyObject* self) {
  Point* x = ((PointObject*)self)->m_x;
  return ((x->x() << 16) + x->y());
}

void init_PointType(PyObject* module_dict) {
  PointType.ob_type = &PyType_Type;
  PointType.tp_name = "gameracore.Point";
  PointType.tp_basicsize = sizeof(PointObject);
  PointType.tp_dealloc = point_dealloc;
  PointType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  PointType.tp_new = point_new;
  PointType.tp_getattro = PyObject_GenericGetAttr;
  PointType.tp_alloc = NULL; // PyType_GenericAlloc;
  PointType.tp_richcompare = point_richcompare;
  PointType.tp_getset = point_getset;
  PointType.tp_free = NULL; // _PyObject_Del;
  PointType.tp_methods = point_methods;
  PointType.tp_repr = point_repr;
  PointType.tp_hash = point_hash;
  PointType.tp_doc = 
"__init__(Int *x*, Int *y*)\n\n"
"Point stores an (*x*, *y*) coordinate point.\n\n"
"Most functions that take a Point as an argument can also take a\n"
"2-element sequence.  For example, the following are all equivalent:\n\n"
".. code:: Python\n\n"
"    px = image.get(Point(5, 2))\n"
"    px = image.get((5, 2))\n"
"    px = image.get([5, 2])\n\n";
  PyType_Ready(&PointType);
  PyDict_SetItemString(module_dict, "Point", (PyObject*)&PointType);
}
