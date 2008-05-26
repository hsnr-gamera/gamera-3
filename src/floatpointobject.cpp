/*
 *
 * Copyright (C) 2005 Michael Droettboom
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

extern "C" {
  static PyObject* floatpoint_new(PyTypeObject* pytype, PyObject* args,
				  PyObject* kwds);
  static void floatpoint_dealloc(PyObject* self);
  // get/set
  static PyObject* floatpoint_get_x(PyObject* self);
  static PyObject* floatpoint_get_y(PyObject* self);
  static PyObject* floatpoint_richcompare(PyObject* a, PyObject* b, int op);
  // methods
  static PyObject* floatpoint_distance(PyObject* self, PyObject* args);
  static PyObject* floatpoint_repr(PyObject* self);
  // operators
  static PyObject* floatpoint_add(PyObject* self, PyObject* args);
  static PyObject* floatpoint_sub(PyObject* self, PyObject* args);
  static PyObject* floatpoint_mul(PyObject* self, PyObject* args);
  static PyObject* floatpoint_div(PyObject* self, PyObject* args);
  static PyObject* floatpoint_negative(PyObject* self);
  static PyObject* floatpoint_positive(PyObject* self);
  static PyObject* floatpoint_absolute(PyObject* self);
}

static PyTypeObject FloatPointType = {
  PyObject_HEAD_INIT(NULL)
  0,
};

static PyNumberMethods floatpoint_number_methods;

static PyGetSetDef floatpoint_getset[] = {
  { (char *)"x", (getter)floatpoint_get_x, NULL,
    (char *)"(float property)\n\nGet the current x value", 0},
  { (char *)"y", (getter)floatpoint_get_y, NULL,
    (char *)"(float property)\n\nGet the current y value", 0},
  { NULL }
};

static PyMethodDef floatpoint_methods[] = {
  { CHAR_PTR_CAST "distance", floatpoint_distance, METH_O,
    CHAR_PTR_CAST "**distance** (POINT *p*)\n\nCalculates the Euclidean distance from this point to another point."},
  { NULL }
};

PyTypeObject* get_FloatPointType() {
  return &FloatPointType;
}

static PyObject* _floatpoint_new(PyTypeObject* pytype, FloatPoint* fp) {
  FloatPointObject* so;
  so = (FloatPointObject*)pytype->tp_alloc(pytype, 0);
  so->m_x = fp;
  return (PyObject*)so;
}

static PyObject* floatpoint_new(PyTypeObject* pytype, PyObject* args,
			  PyObject* kwds) {
  int num_args = PyTuple_GET_SIZE(args);
  if (num_args == 2) {
    double x, y;
    if (PyArg_ParseTuple(args, CHAR_PTR_CAST "dd:FloatPoint.__init__", &x, &y))
      return _floatpoint_new(pytype, new FloatPoint(x, y));
  }

  PyErr_Clear();

  if (num_args == 1) {
    PyObject* p;
    if (PyArg_ParseTuple(args, CHAR_PTR_CAST "O", &p)) {
      try {
	return _floatpoint_new(pytype, new FloatPoint(coerce_FloatPoint(p)));
      } catch (std::exception e) {
	;
      }
    }
  }

  PyErr_Clear();
  PyErr_SetString(PyExc_TypeError, "Invalid arguments to FloatPoint constructor.  Valid forms are: (x, y), (Point p), and ((x, y)).");
  return 0;
}

static void floatpoint_dealloc(PyObject* self) {
  FloatPointObject* x = (FloatPointObject*)self;
  delete x->m_x;
  self->ob_type->tp_free(self);
}

#define CREATE_GET_FUNC(name) static PyObject* floatpoint_get_##name(PyObject* self) {\
  FloatPoint* x = ((FloatPointObject*)self)->m_x; \
  return PyFloat_FromDouble(x->name()); \
}

CREATE_GET_FUNC(x)
CREATE_GET_FUNC(y)

static PyObject* floatpoint_distance(PyObject* self, PyObject* point) {
  FloatPoint* x = ((FloatPointObject*)self)->m_x;

  try {
    FloatPoint fp = coerce_FloatPoint(point);
    double distance = x->distance(fp);
    return PyFloat_FromDouble(distance);
  } catch (std::exception e) {
    return 0;
  }
}

static PyObject* floatpoint_richcompare(PyObject* a, PyObject* b, int op) {
  FloatPoint ap, bp;
  try {
    ap = coerce_FloatPoint(a);
  } catch (std::exception e) {
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
  }
  try {
    bp = coerce_FloatPoint(b);
  } catch (std::exception e) {
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

static PyObject* floatpoint_repr(PyObject* self) {
  // Why do they make this so hard?  PyString_FromFormat does
  // not support doubles
  FloatPoint* x = ((FloatPointObject*)self)->m_x;
  std::ostringstream ostr;
  ostr << *x;
  PyObject* result = PyString_FromStringAndSize(ostr.str().data(), ostr.str().size());
  return result;
}

#define CREATE_BINARY_OPERATOR(name, op) \
  static PyObject* floatpoint_##name(PyObject* self, PyObject* args) { \
  FloatPoint* x = ((FloatPointObject*)self)->m_x; \
  try { \
    FloatPoint fp = coerce_FloatPoint(args); \
    FloatPoint result = *x op fp; \
    return create_FloatPointObject(result); \
  } catch (std::exception e) { \
    return 0; \
  } \
} \

#define CREATE_UNARY_OPERATOR(name, op) \
  static PyObject* floatpoint_##name(PyObject* self) { \
    FloatPoint* x = ((FloatPointObject*)self)->m_x; \
    FloatPoint result = op (*x); \
    return create_FloatPointObject(result); \
} \

CREATE_BINARY_OPERATOR(add, +);
CREATE_BINARY_OPERATOR(sub, -);
CREATE_BINARY_OPERATOR(mul, *);
CREATE_BINARY_OPERATOR(div, /);
CREATE_UNARY_OPERATOR(negative, -);
CREATE_UNARY_OPERATOR(positive, +);

static PyObject* floatpoint_absolute(PyObject* self) {
  FloatPoint* x = ((FloatPointObject*)self)->m_x;
  FloatPoint result = abs(*x);
  return create_FloatPointObject(result);
}

void init_FloatPointType(PyObject* module_dict) {
  floatpoint_number_methods.nb_add = floatpoint_add;
  floatpoint_number_methods.nb_subtract = floatpoint_sub;
  floatpoint_number_methods.nb_multiply = floatpoint_mul;
  floatpoint_number_methods.nb_divide = floatpoint_div;
  floatpoint_number_methods.nb_negative = floatpoint_negative;
  floatpoint_number_methods.nb_positive = floatpoint_positive;
  floatpoint_number_methods.nb_absolute = floatpoint_absolute;

  FloatPointType.ob_type = &PyType_Type;
  FloatPointType.tp_name = CHAR_PTR_CAST "gameracore.FloatPoint";
  FloatPointType.tp_basicsize = sizeof(FloatPointObject);
  FloatPointType.tp_dealloc = floatpoint_dealloc;
  FloatPointType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  FloatPointType.tp_new = floatpoint_new;
  FloatPointType.tp_getattro = PyObject_GenericGetAttr;
  FloatPointType.tp_alloc = NULL; // PyType_GenericAlloc;
  FloatPointType.tp_richcompare = floatpoint_richcompare;
  FloatPointType.tp_getset = floatpoint_getset;
  FloatPointType.tp_free = NULL; // _PyObject_Del;
  FloatPointType.tp_methods = floatpoint_methods;
  FloatPointType.tp_repr = floatpoint_repr;
  FloatPointType.tp_doc = CHAR_PTR_CAST
"**FloatPoint** (*x*, *y*)\n\n"
"   *or*\n\n"
"**FloatPoint** (Point *p*)\n\n"
"   *or*\n\n"
"**FloatPoint** ((*x*, *y*))\n\n"
"FloatPoint stores an (*x*, *y*) coordinate point using floating-point values.\n"
"It is an immutable object, i.e., once it has been created at a certain position,\n"
"it can not be moved.\n\n"
"The standard Gamera ``Point`` stores coordinates as unsigned (positive) integers, "
"and doesn't have any arithmetic operators.  For this reason, ``FloatPoint`` is "
"highly recommended for any analyses that require precision and flexibility.  ``Point`` "
"is kept around for backward compatibility, and because it is a more natural way to refer "
"to physical pixels, as opposed to logical coordinates.  There are, however, implicit "
"conversions between the two types, so a ``FloatPoint`` can be used in place of ``Point`` "
"where it makes sense.  Care should be taken to ensure that negative values are never used "
"to reference image pixels.  (Range checking is performed when using from Python, but not "
"when using from C++.)\n\n"
"The standard arithmetic operators are available on ``FloatPoint`` objects (+ - * / abs).";
  FloatPointType.tp_as_number = &floatpoint_number_methods;
  PyType_Ready(&FloatPointType);
  PyDict_SetItemString(module_dict, "FloatPoint", (PyObject*)&FloatPointType);
}
