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

#define GAMERACORE_INTERNAL
#include "gameramodule.hpp"
#include <exception>

using namespace Gamera;

extern "C" {
  static PyObject* rect_new(PyTypeObject* pytype, PyObject* args,
			    PyObject* kwds);
  static void rect_dealloc(PyObject* self);
  // get
  static PyObject* rect_get_ul(PyObject* self);
  static PyObject* rect_get_ul_x(PyObject* self);
  static PyObject* rect_get_ul_y(PyObject* self);
  static PyObject* rect_get_ur(PyObject* self);
  static PyObject* rect_get_ur_x(PyObject* self);
  static PyObject* rect_get_ur_y(PyObject* self);
  static PyObject* rect_get_lr(PyObject* self);
  static PyObject* rect_get_lr_x(PyObject* self);
  static PyObject* rect_get_lr_y(PyObject* self);
  static PyObject* rect_get_ll(PyObject* self);
  static PyObject* rect_get_ll_x(PyObject* self);
  static PyObject* rect_get_ll_y(PyObject* self);
  static PyObject* rect_get_dimensions(PyObject* self);
  static PyObject* rect_get_size(PyObject* self);
  static PyObject* rect_get_ncols(PyObject* self);
  static PyObject* rect_get_nrows(PyObject* self);
  static PyObject* rect_get_width(PyObject* self);
  static PyObject* rect_get_height(PyObject* self);
  static PyObject* rect_get_offset_x(PyObject* self);
  static PyObject* rect_get_offset_y(PyObject* self);
  static PyObject* rect_get_center(PyObject* self);
  static PyObject* rect_get_center_x(PyObject* self);
  static PyObject* rect_get_center_y(PyObject* self);
  // set
  static int rect_set_ul(PyObject* self, PyObject* value);
  static int rect_set_ul_x(PyObject* self, PyObject* value);
  static int rect_set_ul_y(PyObject* self, PyObject* value);
  static int rect_set_ur(PyObject* self, PyObject* value);
  static int rect_set_ur_x(PyObject* self, PyObject* value);
  static int rect_set_ur_y(PyObject* self, PyObject* value);
  static int rect_set_lr(PyObject* self, PyObject* value);
  static int rect_set_lr_x(PyObject* self, PyObject* value);
  static int rect_set_lr_y(PyObject* self, PyObject* value);
  static int rect_set_ll(PyObject* self, PyObject* value);
  static int rect_set_ll_x(PyObject* self, PyObject* value);
  static int rect_set_ll_y(PyObject* self, PyObject* value);
  static int rect_set_dimensions(PyObject* self, PyObject* value);
  static int rect_set_size(PyObject* self, PyObject* value);
  static int rect_set_ncols(PyObject* self, PyObject* value);
  static int rect_set_nrows(PyObject* self, PyObject* value);
  static int rect_set_width(PyObject* self, PyObject* value);
  static int rect_set_height(PyObject* self, PyObject* value);
  static int rect_set_offset_x(PyObject* self, PyObject* value);
  static int rect_set_offset_y(PyObject* self, PyObject* value);
  // member functions
  static PyObject* rect_set(PyObject* self, PyObject* args);
  static PyObject* rect_move(PyObject* self, PyObject* args);
  static PyObject* rect_contains_x(PyObject* self, PyObject* args);
  static PyObject* rect_contains_y(PyObject* self, PyObject* args);
  static PyObject* rect_contains_point(PyObject* self, PyObject* args);
  static PyObject* rect_contains_rect(PyObject* self, PyObject* args);
  static PyObject* rect_intersects_x(PyObject* self, PyObject* args);
  static PyObject* rect_intersects_y(PyObject* self, PyObject* args);
  static PyObject* rect_intersects(PyObject* self, PyObject* args);
  static PyObject* rect_union(PyObject* _, PyObject* rects);
  static PyObject* rect_merge(PyObject* self, PyObject* args);
  static PyObject* rect_richcompare(PyObject* a, PyObject* b, int op);
  static PyObject* rect_repr(PyObject* self);
}

static PyTypeObject RectType = {
  PyObject_HEAD_INIT(NULL)
  0,
};

static PyGetSetDef rect_getset[] = {
  {"ul", (getter)rect_get_ul, (setter)rect_set_ul},
  {"ul_x", (getter)rect_get_ul_x, (setter)rect_set_ul_x},
  {"ul_y", (getter)rect_get_ul_y, (setter)rect_set_ul_y},
  {"ur", (getter)rect_get_ur, (setter)rect_set_ur},
  {"ur_x", (getter)rect_get_ur_x, (setter)rect_set_ur_x},
  {"ur_y", (getter)rect_get_ur_y, (setter)rect_set_ur_y},
  {"lr", (getter)rect_get_lr, (setter)rect_set_lr},
  {"lr_x", (getter)rect_get_lr_x, (setter)rect_set_lr_x},
  {"lr_y", (getter)rect_get_lr_y, (setter)rect_set_lr_y},
  {"ll", (getter)rect_get_ll, (setter)rect_set_ll},
  {"ll_x", (getter)rect_get_ll_x, (setter)rect_set_ll_x},
  {"ll_y", (getter)rect_get_ll_y, (setter)rect_set_ll_y},
  {"dimensions", (getter)rect_get_dimensions, (setter)rect_set_dimensions},
  {"size", (getter)rect_get_size, (setter)rect_set_size},
  {"nrows", (getter)rect_get_nrows, (setter)rect_set_nrows},
  {"ncols", (getter)rect_get_ncols, (setter)rect_set_ncols},
  {"width", (getter)rect_get_width, (setter)rect_set_width},
  {"height", (getter)rect_get_height, (setter)rect_set_height},
  {"offset_x", (getter)rect_get_offset_x, (setter)rect_set_offset_x},
  {"offset_y", (getter)rect_get_offset_y, (setter)rect_set_offset_y},
  {"center", (getter)rect_get_center, NULL},
  {"center_x", (getter)rect_get_center_x, NULL},
  {"center_y", (getter)rect_get_center_y, NULL},
  { NULL }
};

static PyMethodDef rect_methods[] = {
  {"rect_set", rect_set, METH_VARARGS},
  {"contains_x", rect_contains_x, METH_VARARGS},
  {"contains_y", rect_contains_y, METH_VARARGS},
  {"contains_point", rect_contains_point, METH_VARARGS},
  {"contains_rect", rect_contains_rect, METH_VARARGS},
  {"intersects_x", rect_intersects_x, METH_VARARGS},
  {"intersects_y", rect_intersects_y, METH_VARARGS},
  {"intersects", rect_intersects, METH_VARARGS},
  {"move", rect_move, METH_VARARGS},
  // TODO: If and when we move to Python 2.3, we should add the METH_STATIC flag
  // to rect_union, since this really should be a static method.  At this point, 
  // the calling convention will just have to look a bit funny from Python.
  // (i.e. rect_instance.union vs Rect.union)
  {"union", rect_union, METH_O},
  {"merge", rect_merge, METH_VARARGS},
  {NULL, NULL}
};

extern PyTypeObject* get_RectType() {
  return &RectType;
}

static PyObject* rect_new(PyTypeObject* pytype, PyObject* args,
			  PyObject* kwds) {
  int num_args = PyTuple_GET_SIZE(args);
  if (num_args == 4) {
    int offset_x, offset_y, nrows, ncols;
    if (PyArg_ParseTuple(args, "iiii", &offset_x, &offset_y, &nrows, &ncols)
	<= 0)
      return 0;
    RectObject* so;
    so = (RectObject*)pytype->tp_alloc(pytype, 0);
    so->m_x = new Rect((size_t)offset_x, (size_t)offset_y, (size_t)nrows,
			  (size_t)ncols);
    return (PyObject*)so;
  } else if (num_args == 2) {
    PyObject *a, *b;
    if (PyArg_ParseTuple(args, "OO", &a, &b) <= 0)
      return 0;
    if (is_PointObject(a) && is_PointObject(b)) {
      RectObject* so;
      so = (RectObject*)pytype->tp_alloc(pytype, 0);
      so->m_x = new Rect(*((PointObject*)a)->m_x, *((PointObject*)b)->m_x);
      return (PyObject*)so;
    } else if (is_PointObject(a) && is_SizeObject(b)) {
      RectObject* so;
      so = (RectObject*)pytype->tp_alloc(pytype, 0);
      so->m_x = new Rect(*((PointObject*)a)->m_x, *((SizeObject*)b)->m_x);
      return (PyObject*)so;
    } else if (is_PointObject(a) && is_DimensionsObject(b)) {
      RectObject* so;
      so = (RectObject*)pytype->tp_alloc(pytype, 0);
      so->m_x = new Rect(*((PointObject*)a)->m_x,
      			    *((DimensionsObject*)b)->m_x);
      return (PyObject*)so;
    } else {
      PyErr_SetString(PyExc_TypeError, "No overloaded functions match!");
      return 0;
    }
  } else if (num_args == 1) {
    PyObject* other;
    if (PyArg_ParseTuple(args, "O", &other) <= 0)
      return 0;
    if (is_RectObject(other)) {
      RectObject* so;
      so = (RectObject*)pytype->tp_alloc(pytype, 0);
      so->m_x = new Rect(*((RectObject*)other)->m_x);
      return (PyObject*)so;
    } else {
      PyErr_SetString(PyExc_TypeError, "No overloaded functions match!");
      return 0;
    }
  } else if (num_args == 0) {
    RectObject* so;
    so = (RectObject*)pytype->tp_alloc(pytype, 0);
    so->m_x = new Rect();
    return (PyObject*)so;
  } else {
    PyErr_SetString(PyExc_TypeError, "No overloaded functions match!");
    return 0;
  }
}

static void rect_dealloc(PyObject* self) {
  RectObject* x = (RectObject*)self;
  delete x->m_x;
  self->ob_type->tp_free(self);
}

/*
  General case methods

  The majority of the get/set methods can be exported in a standard way
  using macros. Below are all of the get/set methods that accept/return
  ints or points. See below for the special cases.
*/
#define CREATE_GET_FUNC(name) static PyObject* rect_get_##name(PyObject* self) {\
  Rect* x = ((RectObject*)self)->m_x; \
  return Py_BuildValue("i", (int)x->name()); \
}

#define CREATE_GET_POINT_FUNC(name) static PyObject* rect_get_##name(PyObject* self) {\
  Rect* x = ((RectObject*)self)->m_x; \
  return create_PointObject(x->name()); \
}


#define CREATE_SET_FUNC(name) static int rect_set_##name(PyObject* self, PyObject* value) {\
  if (!PyInt_Check(value)) { \
    PyErr_SetString(PyExc_TypeError, "Type Error!"); \
    return -1; \
  } \
  Rect* x = ((RectObject*)self)->m_x; \
  try { \
    x->name((size_t)PyInt_AS_LONG(value)); \
  } catch(std::exception& e) { \
    PyErr_SetString(PyExc_TypeError, e.what()); \
    return -1; \
  } \
  return 0; \
}

#define CREATE_SET_POINT_FUNC(name) static int rect_set_##name(PyObject* self, PyObject* value) {\
  if (!is_PointObject(value)) { \
    PyErr_SetString(PyExc_TypeError, "Type Error!"); \
    return -1; \
  } \
  Rect* x = ((RectObject*)self)->m_x; \
  try { \
    x->name(*((PointObject*)value)->m_x); \
  } catch(std::exception& e) { \
    PyErr_SetString(PyExc_TypeError, e.what()); \
    return -1; \
  } \
  return 0; \
}

CREATE_GET_POINT_FUNC(ul)
CREATE_GET_POINT_FUNC(ur)
CREATE_GET_POINT_FUNC(lr)
CREATE_GET_POINT_FUNC(ll)
CREATE_GET_POINT_FUNC(center)

CREATE_GET_FUNC(ul_x)
CREATE_GET_FUNC(ul_y)
CREATE_GET_FUNC(ur_x)
CREATE_GET_FUNC(ur_y)
CREATE_GET_FUNC(lr_y)
CREATE_GET_FUNC(lr_x)
CREATE_GET_FUNC(ll_x)
CREATE_GET_FUNC(ll_y)
CREATE_GET_FUNC(nrows)
CREATE_GET_FUNC(ncols)
CREATE_GET_FUNC(width)
CREATE_GET_FUNC(height)
CREATE_GET_FUNC(offset_x)
CREATE_GET_FUNC(offset_y)
CREATE_GET_FUNC(center_x)
CREATE_GET_FUNC(center_y)

CREATE_SET_POINT_FUNC(ul)
CREATE_SET_POINT_FUNC(ur)
CREATE_SET_POINT_FUNC(lr)
CREATE_SET_POINT_FUNC(ll)

CREATE_SET_FUNC(ul_x)
CREATE_SET_FUNC(ul_y)
CREATE_SET_FUNC(ur_x)
CREATE_SET_FUNC(ur_y)
CREATE_SET_FUNC(lr_y)
CREATE_SET_FUNC(lr_x)
CREATE_SET_FUNC(ll_x)
CREATE_SET_FUNC(ll_y)
CREATE_SET_FUNC(nrows)
CREATE_SET_FUNC(ncols)
CREATE_SET_FUNC(width)
CREATE_SET_FUNC(height)
CREATE_SET_FUNC(offset_x)
CREATE_SET_FUNC(offset_y)

/*
  Special case get/set methods
*/
static PyObject* rect_get_size(PyObject* self) {
  Rect* x = ((RectObject*)self)->m_x;
  return create_SizeObject(x->size());
}

static PyObject* rect_get_dimensions(PyObject* self) {
  Rect* x = ((RectObject*)self)->m_x;
  return create_DimensionsObject(x->dimensions());
}

static int rect_set_size(PyObject* self, PyObject* value) {
  Rect* x = ((RectObject*)self)->m_x;
  Size* size = ((SizeObject*)value)->m_x;
  x->size(*size);
  return 0;
}

static int rect_set_dimensions(PyObject* self, PyObject* value) {
  if (!is_DimensionsObject(value)) {
    PyErr_SetString(PyExc_TypeError, "Type Error!");
    return -1;
  }
  Rect* x = ((RectObject*)self)->m_x;
  Dimensions* dim = ((DimensionsObject*)value)->m_x;
  x->dimensions(*dim);
  return 0;
}

/*
  Standard methods
*/
static PyObject* rect_set(PyObject* self, PyObject* args) {
  Rect* x = ((RectObject*)self)->m_x;
  int offset_x, offset_y, nrows, ncols;
  if (PyArg_ParseTuple(args, "iiii", &offset_x, &offset_y, &nrows, &ncols) <= 0) {
    return 0;
  }
  x->rect_set(offset_x, offset_y, nrows, ncols);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject* rect_move(PyObject* self, PyObject* args) {
  Rect* x = ((RectObject*)self)->m_x;
  int xv, y;
  if (PyArg_ParseTuple(args, "ii", &xv, &y) <= 0) {
    return 0;
  }
  x->move(xv, y);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject* rect_contains_x(PyObject* self, PyObject* args) {
  Rect* x = ((RectObject*)self)->m_x;
  int xv;
  if (PyArg_ParseTuple(args, "i", &xv) <= 0)
    return 0;
  if (x->contains_x(xv)) {
    Py_INCREF(Py_True);
    return Py_True;
  } else {
    Py_INCREF(Py_False);
    return Py_False;
  }
}

static PyObject* rect_contains_y(PyObject* self, PyObject* args) {
  Rect* x = ((RectObject*)self)->m_x;
  int y;
  if (PyArg_ParseTuple(args, "i", &y) <= 0)
    return 0;
  if (x->contains_y(y)) {
    Py_INCREF(Py_True);
    return Py_True;
  } else {
    Py_INCREF(Py_False);
    return Py_False;
  }
}

static PyObject* rect_contains_point(PyObject* self, PyObject* args) {
  Rect* x = ((RectObject*)self)->m_x;
  PyObject* point;
  if (PyArg_ParseTuple(args, "O", &point) <= 0)
    return 0;
  if (!is_PointObject(point)) {
    PyErr_SetString(PyExc_TypeError, "Argument must be a Point object!");
    return 0;
  }
  if (x->contains_point(*((PointObject*)point)->m_x)) {
    Py_INCREF(Py_True);
    return Py_True;
  } else {
    Py_INCREF(Py_False);
    return Py_False;
  }
}

static PyObject* rect_contains_rect(PyObject* self, PyObject* args) {
  Rect* x = ((RectObject*)self)->m_x;
  PyObject* rect;
  if (PyArg_ParseTuple(args, "O", &rect) <= 0)
    return 0;
  if (!is_RectObject(rect)) {
    PyErr_SetString(PyExc_TypeError, "Argument must be a Rect object!");
    return 0;
  }
  if (x->contains_rect(*((RectObject*)rect)->m_x)) {
    Py_INCREF(Py_True);
    return Py_True;
  } else {
    Py_INCREF(Py_False);
    return Py_False;
  }
}

static PyObject* rect_intersects_x(PyObject* self, PyObject* args) {
  Rect* x = ((RectObject*)self)->m_x;
  PyObject* rect;
  if (PyArg_ParseTuple(args, "O", &rect) <= 0)
    return 0;
  if (!is_RectObject(rect)) {
    PyErr_SetString(PyExc_TypeError, "Argument must be a Rect object!");
    return 0;
  }
  if (x->intersects_x(*((RectObject*)rect)->m_x)) {
    Py_INCREF(Py_True);
    return Py_True;
  } else {
    Py_INCREF(Py_False);
    return Py_False;
  }
}

static PyObject* rect_intersects_y(PyObject* self, PyObject* args) {
  Rect* x = ((RectObject*)self)->m_x;
  PyObject* rect;
  if (PyArg_ParseTuple(args, "O", &rect) <= 0)
    return 0;
  if (!is_RectObject(rect)) {
    PyErr_SetString(PyExc_TypeError, "Argument must be a Rect object!");
    return 0;
  }
  if (x->intersects_y(*((RectObject*)rect)->m_x)) {
    Py_INCREF(Py_True);
    return Py_True;
  } else {
    Py_INCREF(Py_False);
    return Py_False;
  }
}

static PyObject* rect_intersects(PyObject* self, PyObject* args) {
  Rect* x = ((RectObject*)self)->m_x;
  PyObject* rect;
  if (PyArg_ParseTuple(args, "O", &rect) <= 0)
    return 0;
  if (!is_RectObject(rect)) {
    PyErr_SetString(PyExc_TypeError, "Argument must be a Rect object!");
    return 0;
  }
  if (x->intersects(*((RectObject*)rect)->m_x)) {
    Py_INCREF(Py_True);
    return Py_True;
  } else {
    Py_INCREF(Py_False);
    return Py_False;
  }
}

static PyObject* rect_union(PyObject* _ /* staticmethod */, PyObject* list) {
  if (!PyList_Check(list)) {
    PyErr_SetString(PyExc_TypeError, "Argument must be a list of Rects");
    return 0;
  }

  std::vector<Rect*> vec(PyList_GET_SIZE(list));
  for (int i=0; i < PyList_GET_SIZE(list); ++i) {
    PyObject* py_rect = PyList_GET_ITEM(list, i);
    if (!is_RectObject(py_rect)) {
      PyErr_SetString(PyExc_TypeError, "Argument must be a list of Rects");
      return 0;
    }
    vec[i] = ((RectObject *)py_rect)->m_x;
  }
  PyTypeObject* pytype = get_RectType();
  RectObject* so = (RectObject*)pytype->tp_alloc(pytype, 0);
  so->m_x = Rect::union_rects(vec);
  return (PyObject*)so;
}

static PyObject* rect_merge(PyObject* self, PyObject* args) {
  Rect* x = ((RectObject*)self)->m_x;
  PyObject* rect;
  if (PyArg_ParseTuple(args, "O", &rect) <= 0)
    return 0;
  if (!is_RectObject(rect)) {
    PyErr_SetString(PyExc_TypeError, "Argument must be a Rect object!");
    return 0;
  }
  x->merge(*((RectObject*)rect)->m_x);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject* rect_richcompare(PyObject* a, PyObject* b, int op) {
  if (!is_RectObject(a) || !is_RectObject(b)) {
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
  }

  Rect& ap = *((RectObject*)a)->m_x;
  Rect& bp = *((RectObject*)b)->m_x;

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

static PyObject* rect_repr(PyObject* self) {
  Rect* x = ((RectObject*)self)->m_x;
  return PyString_FromFormat("<gameracore.Rect offset_y: %i offset_x: %i nrows: %i ncols: %i>",
			     x->offset_y(), x->offset_x(), x->nrows(),
			     x->ncols());
}

void init_RectType(PyObject* module_dict) {
  RectType.ob_type = &PyType_Type;
  RectType.tp_name = "gameracore.Rect";
  RectType.tp_basicsize = sizeof(RectObject);
  RectType.tp_dealloc = rect_dealloc;
  RectType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  RectType.tp_methods = rect_methods;
  RectType.tp_getset = rect_getset;
  RectType.tp_new = rect_new;
  RectType.tp_getattro = PyObject_GenericGetAttr;
  RectType.tp_alloc = PyType_GenericAlloc;
  RectType.tp_richcompare = rect_richcompare;
  RectType.tp_free = _PyObject_Del;
  RectType.tp_repr = rect_repr;
  PyType_Ready(&RectType);
  PyDict_SetItemString(module_dict, "Rect", (PyObject*)&RectType);
}
