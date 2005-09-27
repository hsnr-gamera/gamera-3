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
#ifdef GAMERA_DEPRECATED
  static PyObject* rect_get_dimensions(PyObject* self);
#endif
  static PyObject* rect_get_dim(PyObject* self);
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
#ifdef GAMERA_DEPRECATED
  static int rect_set_dimensions(PyObject* self, PyObject* value);
#endif
  static int rect_set_dim(PyObject* self, PyObject* value);
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
#ifdef GAMERA_DEPRECATED
  {"dimensions", (getter)rect_get_dimensions, (setter)rect_set_dimensions,
"(Dimensions property)\n\nThe dimensions of the rectangle.  Equivalent to ``Dimensions(image.nrows, image.ncols)``.\n\n"
".. warning::\n\n"
"  The Dimensions class is deprecated.\n\n"
"  Reason: (x, y) coordinate consistency.\n\n"
"  Use Dim(ncols, nrows) instead.\n\n"},
#endif
  {"dim", (getter)rect_get_dim, (setter)rect_set_dim,
   "(Dim property)\n\nThe dimensions of the rectangle.  Equivalent to ``Dim(image.ncols, image.nrows)``."},
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
   "(int property)\n\nThe y-location at the exact center of the rectangle in the logical coordinate space."},
  { NULL }
};

static PyMethodDef rect_methods[] = {
  {"rect_set", rect_set, METH_VARARGS, 
   "**rect_set** (...)\n\nChanges the position and size of the rectangle.  Takes the same arguments as the Rect constructor."},
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

static PyObject* _rect_new(PyTypeObject* pytype, Rect* rect) {
  RectObject* so;
  so = (RectObject*)pytype->tp_alloc(pytype, 0);
  so->m_x = rect;
  return (PyObject*)so;
}

static PyObject* rect_new(PyTypeObject* pytype, PyObject* args,
			  PyObject* kwds) {
  int num_args = PyTuple_GET_SIZE(args);
  if (num_args == 2) {
    PyObject *a, *b;
    if (PyArg_ParseTuple(args, "OO", &a, &b)) {
      Point point_a;
      try {
	point_a = coerce_Point(a);
      } catch (std::invalid_argument e) {
	goto phase2;
      }
      
      try {
	Point point_b = coerce_Point(b);
	return _rect_new(pytype, new Rect(point_a, point_b));
      } catch (std::invalid_argument e) {
	PyErr_Clear();
	if (is_SizeObject(b)) {
	  return _rect_new(pytype, new Rect(point_a, *((SizeObject*)b)->m_x));
	} else if (is_DimObject(b)) {
	  return _rect_new(pytype, new Rect(point_a, *((DimObject*)b)->m_x));
	}
#ifdef GAMERA_DEPRECATED
	else if (is_DimensionsObject(b)) {
	  if (send_deprecation_warning(
"Rect(Point offset, Dimensions dimensions) is deprecated.\n\n"
"Reason: (x, y) coordinate consistency. (Dimensions is now deprecated \n"
"in favor of Dim).\n\n"
"Use Rect((offset_x, offset_y), Dim(ncols, nrows)) instead.", 
"imageobject.cpp", __LINE__) == 0)
	    return 0;
	  return _rect_new(pytype, new Rect(point_a, *((DimensionsObject*)b)->m_x)); // deprecated call
	}
#endif
      }
    }
  }
  
 phase2:
  PyErr_Clear();

  if (num_args == 1) {
    PyObject* other;
    if (PyArg_ParseTuple(args, "O", &other)) {
      if (is_RectObject(other)) {
	return _rect_new(pytype, new Rect(*((RectObject*)other)->m_x));
      }
    }
  }

  PyErr_Clear();
  
  if (num_args == 0) {
    return _rect_new(pytype, new Rect());
  }

#ifdef GAMERA_DEPRECATED
  PyErr_Clear();
  if (num_args == 4) {
    int offset_y, offset_x, nrows, ncols;
    if (PyArg_ParseTuple(args, "iiii", &offset_y, &offset_x, &nrows, &ncols)) {
      if (send_deprecation_warning(
"Rect(offset_y, offset_x, nrows, ncols) is deprecated.\n\n"
"Reason: (x, y) coordinate consistency.\n\n"
"Use Rect((offset_x, offset_y), Dim(ncols, nrows)) instead.", 
"imageobject.cpp", __LINE__) == 0)
	return 0;
      return _rect_new(pytype, new Rect(Point((size_t)offset_x, (size_t)offset_y), 
					Dim((size_t)ncols, (size_t)nrows)));
    }
  }
#endif

  PyErr_Clear();
  PyErr_SetString(PyExc_TypeError, "Incorrect arguments to Rect constructor.  See doc(Rect) for valid arguments.");
  return 0;
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
    PyErr_SetString(PyExc_TypeError, "Must be an integer value"); \
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
  try { \
    Point p = coerce_Point(value); \
    Rect* x = ((RectObject*)self)->m_x; \
    try { \
      x->name(p); \
    } catch(std::exception& e) { \
      PyErr_SetString(PyExc_TypeError, e.what()); \
      return -1; \
    } \
    return 0; \
  } catch (std::invalid_argument e) { \
    return -1; \
  } \
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

#ifdef GAMERA_DEPRECATED
static PyObject* rect_get_dimensions(PyObject* self) {
  Rect* x = ((RectObject*)self)->m_x;
  if (send_deprecation_warning(
"Rect.dimensions property is deprecated.\n\n"
"Reason: (x, y) coordinate consistency.\n\n"
"Use Rect.dim instead.", 
"imageobject.cpp", __LINE__) == 0)
    return 0;
  return create_DimensionsObject(x->dimensions()); // deprecated call
}
#endif

static PyObject* rect_get_dim(PyObject* self) {
  Rect* x = ((RectObject*)self)->m_x;
  return create_DimObject(x->dim());
}

static int rect_set_size(PyObject* self, PyObject* value) {
  Rect* x = ((RectObject*)self)->m_x;
  Size* size = ((SizeObject*)value)->m_x;
  x->size(*size);
  return 0;
}

#ifdef GAMERA_DEPRECATED
static int rect_set_dimensions(PyObject* self, PyObject* value) {
  if (send_deprecation_warning(
"Rect.dimensions property is deprecated.\n\n"
"Reason: (x, y) coordinate consistency.\n\n"
"Use Rect.dim instead.", 
"imageobject.cpp", __LINE__) == 0)
    return 0;
  if (!is_DimensionsObject(value)) {
    PyErr_SetString(PyExc_TypeError, "Must be a Dimensions object.");
    return -1;
  }
  Rect* x = ((RectObject*)self)->m_x;
  Dimensions* dim = ((DimensionsObject*)value)->m_x;
  x->dimensions(*dim); // deprecated call
  return 0;
}
#endif

static int rect_set_dim(PyObject* self, PyObject* value) {
  if (!is_DimObject(value)) {
    PyErr_SetString(PyExc_TypeError, "Must be a Dim object.");
    return -1;
  }
  Rect* x = ((RectObject*)self)->m_x;
  Dim* dim = ((DimObject*)value)->m_x;
  x->dim(*dim);
  return 0;
}

/*
  Standard methods
*/
static PyObject* rect_set(PyObject* self, PyObject* args) {
  Rect* x = ((RectObject*)self)->m_x;
  PyObject* py_other = rect_new(get_RectType(), args, NULL);
  if (py_other == NULL) {
    PyErr_Clear();
    PyErr_SetString(PyExc_TypeError, "Incorrect arguments to rect_set.  See doc(rect_set) for valid arguments.");
    return 0;
  }
  Rect* other = ((RectObject*)py_other)->m_x;
  x->rect_set(other->origin(), other->dim());
  Py_DECREF(py_other);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject* rect_move(PyObject* self, PyObject* args) {
  Rect* x = ((RectObject*)self)->m_x;
  int xv, y;
  if (PyArg_ParseTuple(args, "ii:move", &xv, &y) <= 0) {
    return 0;
  }
  x->move(xv, y);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject* rect_contains_x(PyObject* self, PyObject* args) {
  Rect* x = ((RectObject*)self)->m_x;
  int xv;
  if (PyArg_ParseTuple(args, "i:contains_x", &xv) <= 0)
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
  if (PyArg_ParseTuple(args, "i:contains_y", &y) <= 0)
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
  PyObject* py_point;
  if (PyArg_ParseTuple(args, "O:contains_point", &py_point)) {
    try {
      Point point = coerce_Point(py_point);
      if (x->contains_point(point)) {
	Py_INCREF(Py_True);
	return Py_True;
      } else {
	Py_INCREF(Py_False);
	return Py_False;
      }
    } catch (std::invalid_argument e) {
      ;
    }
  }
  return 0;
}

static PyObject* rect_contains_rect(PyObject* self, PyObject* args) {
  Rect* x = ((RectObject*)self)->m_x;
  PyObject* rect;
  if (PyArg_ParseTuple(args, "O:contains_rect", &rect) <= 0)
    return 0;
  if (!is_RectObject(rect)) {
    PyErr_SetString(PyExc_TypeError, "Argument must be a Rect object.");
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
  if (PyArg_ParseTuple(args, "i:expand", &size) <= 0)
    return 0;
  PyTypeObject* pytype = get_RectType();
  RectObject* so = (RectObject*)pytype->tp_alloc(pytype, 0);
  so->m_x = new Rect(x->expand(size));
  return (PyObject*)so;
}

static PyObject* rect_intersects_x(PyObject* self, PyObject* args) {
  Rect* x = ((RectObject*)self)->m_x;
  PyObject* rect;
  if (PyArg_ParseTuple(args, "O:intersects_x", &rect) <= 0)
    return 0;
  if (!is_RectObject(rect)) {
    PyErr_SetString(PyExc_TypeError, "Argument must be a Rect object.");
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
  if (PyArg_ParseTuple(args, "O:intersects_y", &rect) <= 0)
    return 0;
  if (!is_RectObject(rect)) {
    PyErr_SetString(PyExc_TypeError, "Argument must be a Rect object.");
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
  if (PyArg_ParseTuple(args, "O:intersects", &rect) <= 0)
    return 0;
  if (!is_RectObject(rect)) {
    PyErr_SetString(PyExc_TypeError, "Argument must be a Rect object.");
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
  if (PyArg_ParseTuple(args, "O:intersection", &rect) <= 0)
    return 0;
  if (!is_RectObject(rect)) {
    PyErr_SetString(PyExc_TypeError, "Argument must be a Rect object.");
    return 0;
  }
  PyTypeObject* pytype = get_RectType();
  RectObject* so = (RectObject*)pytype->tp_alloc(pytype, 0);
  so->m_x = new Rect(x->intersection(*((RectObject*)rect)->m_x));
  return (PyObject*)so;
}

static PyObject* rect_union_rects(PyObject* _ /* staticmethod */, PyObject* l) {
  PyObject* seq = PySequence_Fast(l, "First argument must be iterable of Rects");
  if (seq == NULL) 
    return 0;

  int num_rects = PySequence_Fast_GET_SIZE(seq);
  std::vector<Rect*> vec(num_rects);
  for (int i=0; i < num_rects; ++i) {
    PyObject* py_rect = PySequence_Fast_GET_ITEM(seq, i);
    if (!is_RectObject(py_rect)) {
      PyErr_SetString(PyExc_TypeError, "Argument must be a list of Rects");
      return 0;
    }
    vec[i] = ((RectObject *)py_rect)->m_x;
  }
  Py_DECREF(seq);
  PyTypeObject* pytype = get_RectType();
  RectObject* so = (RectObject*)pytype->tp_alloc(pytype, 0);
  so->m_x = Rect::union_rects(vec);
  return (PyObject*)so;
}

static PyObject* rect_union(PyObject* self, PyObject* args) {
  Rect* x = ((RectObject*)self)->m_x;
  PyObject* rect;
  if (PyArg_ParseTuple(args, "O:union", &rect) <= 0)
    return 0;
  if (!is_RectObject(rect)) {
    PyErr_SetString(PyExc_TypeError, "Argument must be a Rect object.");
    return 0;
  }
  x->union_rect(*((RectObject*)rect)->m_x);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject* rect_distance_euclid(PyObject* self, PyObject* args) {
  Rect* x = ((RectObject*)self)->m_x;
  PyObject* rect;
  if (PyArg_ParseTuple(args, "O:distance_euclid", &rect) <= 0)
    return 0;
  if (!is_RectObject(rect)) {
    PyErr_SetString(PyExc_TypeError, "Argument must be a Rect object.");
    return 0;
  }
  return PyFloat_FromDouble(x->distance_euclid(*((RectObject*)rect)->m_x));
}

static PyObject* rect_distance_bb(PyObject* self, PyObject* args) {
  Rect* x = ((RectObject*)self)->m_x;
  PyObject* rect;
  if (PyArg_ParseTuple(args, "O:distance_bb", &rect) <= 0)
    return 0;
  if (!is_RectObject(rect)) {
    PyErr_SetString(PyExc_TypeError, "Argument must be a Rect object.");
    return 0;
  }
  return PyFloat_FromDouble(x->distance_bb(*((RectObject*)rect)->m_x));
}

static PyObject* rect_distance_cx(PyObject* self, PyObject* args) {
  Rect* x = ((RectObject*)self)->m_x;
  PyObject* rect;
  if (PyArg_ParseTuple(args, "O:distance_cx", &rect) <= 0)
    return 0;
  if (!is_RectObject(rect)) {
    PyErr_SetString(PyExc_TypeError, "Argument must be a Rect object.");
    return 0;
  }
  return PyInt_FromLong((long)x->distance_cx(*((RectObject*)rect)->m_x));
}

static PyObject* rect_distance_cy(PyObject* self, PyObject* args) {
  Rect* x = ((RectObject*)self)->m_x;
  PyObject* rect;
  if (PyArg_ParseTuple(args, "O:distance_cy", &rect) <= 0)
    return 0;
  if (!is_RectObject(rect)) {
    PyErr_SetString(PyExc_TypeError, "Argument must be a Rect object.");
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
  case Py_EQ:
    cmp = ap == bp;
    break;
  case Py_NE:
    cmp = ap != bp;
    break;
  case Py_LT:
  case Py_LE:
  case Py_GT:
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
  return PyString_FromFormat("<gameracore.Rect((%i, %i), Dim(%i, %i))>",
			     (int)x->offset_x(), (int)x->offset_y(),
			     (int)x->ncols(), (int)x->nrows());
}

static long rect_hash(PyObject* self) {
  Rect* x = ((RectObject*)self)->m_x;
  return (((x->ul_x() & 0xff) << 24) & ((x->ul_y() & 0xff) << 16) & ((x->lr_x() & 0xff) << 8) & (x->lr_y() & 0xff));
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
  RectType.tp_alloc = NULL;
  RectType.tp_richcompare = rect_richcompare;
  RectType.tp_free = NULL;
  RectType.tp_repr = rect_repr;
  RectType.tp_hash = rect_hash;
  RectType.tp_doc = 
"There are a number of ways to initialize a ``Rect`` object:\n\n"
"  - **Rect** (Point *upper_left*, Point *lower_right*)\n\n"
"  - **Rect** (Point *upper_left*, Size *size*)\n\n"
"  - **Rect** (Point *upper_left*, Dim *dim*)\n\n"
"  - **Rect** (Rect *rectangle*)\n\n"
"**Deprecated forms:**\n\n"
"  - **Rect** (Point *upper_left*, Dimensions *dimensions*)\n\n"
"  - **Rect** (Int *offset_y*, Int *offset_x*, Int *nrows*, Int *ncols*)\n\n"
"The ``Rect`` class manages bounding boxes, and has a number of methods "
"to modify and analyse those bounding boxes.\n\n";
  PyType_Ready(&RectType);
  PyDict_SetItemString(module_dict, "Rect", (PyObject*)&RectType);
}
