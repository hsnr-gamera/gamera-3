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
  static PyObject* rect_expand(PyObject* self, PyObject* args);
  static PyObject* rect_intersects_x(PyObject* self, PyObject* args);
  static PyObject* rect_intersects_y(PyObject* self, PyObject* args);
  static PyObject* rect_intersects(PyObject* self, PyObject* args);
  static PyObject* rect_intersection(PyObject* self, PyObject* args);
  static PyObject* rect_union_rects(PyObject* _, PyObject* rects);
  static PyObject* rect_union(PyObject* self, PyObject* args);
  static PyObject* rect_distance_euclid(PyObject* self, PyObject* args);
  static PyObject* rect_distance_bb(PyObject* self, PyObject* args);
  static PyObject* rect_distance_cx(PyObject* self, PyObject* args);
  static PyObject* rect_distance_cy(PyObject* self, PyObject* args);
  static PyObject* rect_richcompare(PyObject* a, PyObject* b, int op);
  static PyObject* rect_repr(PyObject* self);
}

static PyTypeObject RectType = {
  PyObject_HEAD_INIT(NULL)
  0,
};

static PyGetSetDef rect_getset[] = {
  {"ul", (getter)rect_get_ul, (setter)rect_set_ul,
  "(Point property)\n\nThe upper-left coordinate of the rectangle in logical coordinate space."},
  {"ul_x", (getter)rect_get_ul_x, (setter)rect_set_ul_x,
   "(int property)\n\nThe left edge of the rectangle in logical coordinate space."},
  {"ul_y", (getter)rect_get_ul_y, (setter)rect_set_ul_y,
   "(int property)\n\nThe upper edge of the rectangle in logical coordinate space."},
  {"ur", (getter)rect_get_ur, (setter)rect_set_ur,
   "(Point property)\n\nThe upper-right coordinate of the rectangle in logical coordinate space."},
  {"ur_x", (getter)rect_get_ur_x, (setter)rect_set_ur_x,
   "(int property)\n\nThe right edge of the rectangle in logical coordinate space."},
  {"ur_y", (getter)rect_get_ur_y, (setter)rect_set_ur_y,
   "(int property)\n\nThe upper edge of the rectangle in logical coordinate space."},
  {"lr", (getter)rect_get_lr, (setter)rect_set_lr,
   "(Point property)\n\nThe lower-right coordinate of the rectangle in logical coordinate space."},
  {"lr_x", (getter)rect_get_lr_x, (setter)rect_set_lr_x,
   "(int property)\n\nThe right edge of the rectangle in logical coordinate space."},
  {"lr_y", (getter)rect_get_lr_y, (setter)rect_set_lr_y,
   "(int property)\n\nThe lower edge of the rectangle in logical coordinate space."},
  {"ll", (getter)rect_get_ll, (setter)rect_set_ll,
   "(Point property)\n\nThe lower-left coordinate of the rectangle in logical coordinate space."},
  {"ll_x", (getter)rect_get_ll_x, (setter)rect_set_ll_x,
   "(int property)\n\nThe left edge of the rectangle in logical coordinate space."},
  {"ll_y", (getter)rect_get_ll_y, (setter)rect_set_ll_y,
   "(int property)\n\nThe lower edge of the rectangle in logical coordinate space."},
  {"dimensions", (getter)rect_get_dimensions, (setter)rect_set_dimensions,
   "(Dimensions property)\n\nThe dimensions of the rectangle.  Equivalent to ``Dimensions(image.nrows, image.ncols)``."},
  {"size", (getter)rect_get_size, (setter)rect_set_size,
   "(Size property)\n\nThe size of the rectangle.  Equivalent to ``Size(image.width, image.height)``."},
  {"nrows", (getter)rect_get_nrows, (setter)rect_set_nrows,
  "(int property)\n\nThe number of rows in the rectangle."},
  {"ncols", (getter)rect_get_ncols, (setter)rect_set_ncols,
  "(int property)\n\nThe number of columns in the rectangle."},
  {"width", (getter)rect_get_width, (setter)rect_set_width,
   "(int property)\n\nThe width of the rectangle."},
  {"height", (getter)rect_get_height, (setter)rect_set_height,
   "(int property)\n\nThe height of the rectangle."},
  {"offset_x", (getter)rect_get_offset_x, (setter)rect_set_offset_x,
   "(int property)\n\nThe left edge of the rectangle in the logical coordinate space."},
  {"offset_y", (getter)rect_get_offset_y, (setter)rect_set_offset_y,
   "(int property)\n\nThe upper edge of the rectable in the logical coordinate space."},
  {"center", (getter)rect_get_center, NULL,
   "(Point property)\n\nThe coordinate at the exact center of the rectangle in the logical coordinate space."},
  {"center_x", (getter)rect_get_center_x, NULL,
   "(int property)\n\nThe x-location at the exact center of the rectangle in the logical coordinate space."},
  {"center_y", (getter)rect_get_center_y, NULL,
   "(int property)\b\bThe y-location at the exact center of the rectangle in the logical coordinate space."},
  { NULL }
};

static PyMethodDef rect_methods[] = {
  {"rect_set", rect_set, METH_VARARGS},
  {"contains_x", rect_contains_x, METH_VARARGS,
   "bool **contains_x** (Int *x*)\n\n``True`` if the rectangle contains the given x-value in logical coordinate space."},
  {"contains_y", rect_contains_y, METH_VARARGS,
   "bool **contains_y** (Int *y*)\n\n``True`` if the rectangle contains the given y-value in logical coordinate space."},
  {"contains_point", rect_contains_point, METH_VARARGS,
   "bool **contains_point** (Point *point*)\n\n``True`` if the rectangle contains the given ``Point`` in logical coordinate space"},
  {"contains_rect", rect_contains_rect, METH_VARARGS,
   "bool **contains_rect** (Rect *other*)\n\n``True`` if rectangle completely contains the given rectangle in logical coordinate space."},
  {"expand", rect_expand, METH_VARARGS,
   "Rect **expand** (int *size*)\n\nReturns a new Rect that is padded on all four sides by *size*."},
  {"intersects_x", rect_intersects_x, METH_VARARGS,
   "bool **intersects_x** (Rect *other*)\n\n``True`` if rectangle intersects the given rectangle in the *x* direction (completely ignoring the *y* direction).  (``True`` if the two rectangles are merely \"vertically aligned\".)"},
  {"intersects_y", rect_intersects_y, METH_VARARGS,
   "bool **intersects_y** (Rect *other*)\n\n``True`` if rectangle intersects the given rectangle in the *y* direction (completely ignoring the *x* direction).  (``True`` if the two rectangles are merely \"horizontally aligned\".)"},
  {"intersects", rect_intersects, METH_VARARGS,
   "bool **intersects** (Rect *other*)\n\n``True`` if rectangle intersects with the given rectangle."},
  {"intersection", rect_intersection, METH_VARARGS,
   "bool **intersection** (Rect *other*)\n\nReturns a new Rect that is the intersection of ``self`` and the given Rect object."},
  {"move", rect_move, METH_VARARGS},
  // TODO: If and when we move to Python 2.3, we should add the METH_STATIC flag
  // to rect_union, since this really should be a static method.  At this point, 
  // the calling convention will just have to look a bit funny from Python.
  // (i.e. rect_instance.union vs Rect.union)
  {"union_rects", rect_union_rects, METH_O,
   "Rect **union_rects** (RectList *rects*)\n\nReturns a new rectangle that encloses all of the given rectangles in a list."},
  {"union", rect_union, METH_VARARGS,
   "**union** (Rect *other*)\n\nExpands the rectangle to include the given rectangle and itself."},
  {"distance_euclid", rect_distance_euclid, METH_VARARGS,
   "float **distance_euclid** (Rect *other*)\n\nReturns the Euclidean distance between the center points of this rectangle and the given rectangle."},
  {"distance_bb", rect_distance_bb, METH_VARARGS,
   "float **distance_bb** (Rect *other*)\n\nReturns the closest (Euclidean) distance between the edges of this rectangle and the edges of the given rectangle."},
  {"distance_cx", rect_distance_cx, METH_VARARGS,
   "int **distance_cx** (Rect *other*)\n\nReturns the distance of the center points of this rectangle and the given rectangle in the horizontal direction."},
  {"distance_cy", rect_distance_cy, METH_VARARGS,
   "int **distance_cy** (Rect *other*)\n\nReturns the distance of the center points of this rectangle and the given rectangle in the vertical direction."},
  {NULL, NULL}
};

extern PyTypeObject* get_RectType() {
  return &RectType;
}

static PyObject* rect_new(PyTypeObject* pytype, PyObject* args,
			  PyObject* kwds) {
  int num_args = PyTuple_GET_SIZE(args);
  if (num_args == 4) {
    int offset_y, offset_x, nrows, ncols;
    if (PyArg_ParseTuple(args, "iiii", &offset_y, &offset_x, &nrows, &ncols)
	<= 0)
      return 0;
    RectObject* so;
    so = (RectObject*)pytype->tp_alloc(pytype, 0);
    so->m_x = new Rect((size_t)offset_y, (size_t)offset_x, (size_t)nrows, (size_t)ncols);
    return (PyObject*)so;
  } else if (num_args == 2) {
    PyObject *a, *b;
    if (PyArg_ParseTuple(args, "OO", &a, &b) <= 0)
      return 0;
    if (is_PointObject(a)) {
      if (is_PointObject(b)) {
	RectObject* so;
	so = (RectObject*)pytype->tp_alloc(pytype, 0);
	so->m_x = new Rect(*((PointObject*)a)->m_x, *((PointObject*)b)->m_x);
	return (PyObject*)so;
      } else if (is_SizeObject(b)) {
	RectObject* so;
	so = (RectObject*)pytype->tp_alloc(pytype, 0);
	so->m_x = new Rect(*((PointObject*)a)->m_x, *((SizeObject*)b)->m_x);
	return (PyObject*)so;
      } else if (is_DimensionsObject(b)) {
	RectObject* so;
	so = (RectObject*)pytype->tp_alloc(pytype, 0);
	so->m_x = new Rect(*((PointObject*)a)->m_x,
			   *((DimensionsObject*)b)->m_x);
	return (PyObject*)so;
      } else {
	PyErr_SetString(PyExc_TypeError, "Incorrect arguments types.");
	return 0;
      }
    } else {
      PyErr_SetString(PyExc_TypeError, "Incorrect arguments.");
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
      PyErr_SetString(PyExc_TypeError, "Incorrect arguments.");
      return 0;
    }
  } else if (num_args == 0) {
    RectObject* so;
    so = (RectObject*)pytype->tp_alloc(pytype, 0);
    so->m_x = new Rect();
    return (PyObject*)so;
  } else {
    PyErr_SetString(PyExc_TypeError, "Incorrect arguments.");
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
  return PyInt_FromLong((int)x->name()); \
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

static PyObject* rect_expand(PyObject* self, PyObject* args) {
  Rect* x = ((RectObject*)self)->m_x;
  long size;
  if (PyArg_ParseTuple(args, "i", &size) <= 0)
    return 0;
  PyTypeObject* pytype = get_RectType();
  RectObject* so = (RectObject*)pytype->tp_alloc(pytype, 0);
  so->m_x = new Rect(x->expand(size));
  return (PyObject*)so;
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

static PyObject* rect_intersection (PyObject* self, PyObject* args) {
  Rect* x = ((RectObject*)self)->m_x;
  PyObject* rect;
  if (PyArg_ParseTuple(args, "O", &rect) <= 0)
    return 0;
  if (!is_RectObject(rect)) {
    PyErr_SetString(PyExc_TypeError, "Argument must be a Rect object!");
    return 0;
  }
  PyTypeObject* pytype = get_RectType();
  RectObject* so = (RectObject*)pytype->tp_alloc(pytype, 0);
  so->m_x = new Rect(x->intersection(*((RectObject*)rect)->m_x));
  return (PyObject*)so;
}

static PyObject* rect_union_rects(PyObject* _ /* staticmethod */, PyObject* list) {
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

static PyObject* rect_union(PyObject* self, PyObject* args) {
  Rect* x = ((RectObject*)self)->m_x;
  PyObject* rect;
  if (PyArg_ParseTuple(args, "O", &rect) <= 0)
    return 0;
  if (!is_RectObject(rect)) {
    PyErr_SetString(PyExc_TypeError, "Argument must be a Rect object!");
    return 0;
  }
  x->union_rect(*((RectObject*)rect)->m_x);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject* rect_distance_euclid(PyObject* self, PyObject* args) {
  Rect* x = ((RectObject*)self)->m_x;
  PyObject* rect;
  if (PyArg_ParseTuple(args, "O", &rect) <= 0)
    return 0;
  if (!is_RectObject(rect)) {
    PyErr_SetString(PyExc_TypeError, "Argument must be a Rect object!");
    return 0;
  }
  return PyFloat_FromDouble(x->distance_euclid(*((RectObject*)rect)->m_x));
}

static PyObject* rect_distance_bb(PyObject* self, PyObject* args) {
  Rect* x = ((RectObject*)self)->m_x;
  PyObject* rect;
  if (PyArg_ParseTuple(args, "O", &rect) <= 0)
    return 0;
  if (!is_RectObject(rect)) {
    PyErr_SetString(PyExc_TypeError, "Argument must be a Rect object!");
    return 0;
  }
  return PyFloat_FromDouble(x->distance_bb(*((RectObject*)rect)->m_x));
}

static PyObject* rect_distance_cx(PyObject* self, PyObject* args) {
  Rect* x = ((RectObject*)self)->m_x;
  PyObject* rect;
  if (PyArg_ParseTuple(args, "O", &rect) <= 0)
    return 0;
  if (!is_RectObject(rect)) {
    PyErr_SetString(PyExc_TypeError, "Argument must be a Rect object!");
    return 0;
  }
  return PyInt_FromLong((long)x->distance_cx(*((RectObject*)rect)->m_x));
}

static PyObject* rect_distance_cy(PyObject* self, PyObject* args) {
  Rect* x = ((RectObject*)self)->m_x;
  PyObject* rect;
  if (PyArg_ParseTuple(args, "O", &rect) <= 0)
    return 0;
  if (!is_RectObject(rect)) {
    PyErr_SetString(PyExc_TypeError, "Argument must be a Rect object!");
    return 0;
  }
  return PyInt_FromLong((long)x->distance_cy(*((RectObject*)rect)->m_x));
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
			     (int)x->offset_y(), (int)x->offset_x(),
			     (int)x->nrows(), (int)x->ncols());
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
  RectType.tp_alloc = NULL; // PyType_GenericAlloc;
  RectType.tp_richcompare = rect_richcompare;
  RectType.tp_free = NULL; // _PyObject_Del;
  RectType.tp_repr = rect_repr;
  RectType.tp_doc = "The ``Rect`` class manages bounding boxes, and has a number of operations on those bounding boxes.\n\nThere are multiple ways to create a Rect:\n\n  - **Rect** (Int *offset_y*, Int *offset_x*, Int *nrows*, Int *ncols*)\n\n  - **Rect** (Point *upper_left*, Point *lower_right*)\n\n  - **Rect** (Point *upper_left*, Size *size*)\n\n  - **Rect** (Point *upper_left*, Dimensions *dimensions*)\n";
  PyType_Ready(&RectType);
  PyDict_SetItemString(module_dict, "Rect", (PyObject*)&RectType);
}
