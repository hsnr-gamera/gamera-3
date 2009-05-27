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
  static PyObject* rgbpixel_new(PyTypeObject* pytype, PyObject* args,
			  PyObject* kwds);
  static void rgbpixel_dealloc(PyObject* self);
  // get/set
  static int rgbpixel_set_red(PyObject* self, PyObject* value);
  static int rgbpixel_set_green(PyObject* self, PyObject* value);
  static int rgbpixel_set_blue(PyObject* self, PyObject* value);
  static PyObject* rgbpixel_get_red(PyObject* self);
  static PyObject* rgbpixel_get_green(PyObject* self);
  static PyObject* rgbpixel_get_blue(PyObject* self);
  static PyObject* rgbpixel_get_hue(PyObject* self);
  static PyObject* rgbpixel_get_saturation(PyObject* self);
  static PyObject* rgbpixel_get_value(PyObject* self);
  static PyObject* rgbpixel_get_cie_x(PyObject* self);
  static PyObject* rgbpixel_get_cie_y(PyObject* self);
  static PyObject* rgbpixel_get_cie_z(PyObject* self);
  static PyObject* rgbpixel_get_cyan(PyObject* self);
  static PyObject* rgbpixel_get_magenta(PyObject* self);
  static PyObject* rgbpixel_get_yellow(PyObject* self);
}

static PyTypeObject RGBPixelType = {
  PyObject_HEAD_INIT(NULL)
  0,
};

PyTypeObject* get_RGBPixelType() {
  return &RGBPixelType;
}

static PyGetSetDef rgbpixel_getset[] = {
  { (char *)"red", (getter)rgbpixel_get_red, (setter)rgbpixel_set_red,
    (char *)"(int property)\n\nThe current red value [0,255]", 0 },
  { (char *)"green", (getter)rgbpixel_get_green, (setter)rgbpixel_set_green,
    (char *)"(int property)\n\nThe current green value [0, 255]", 0 },
  { (char *)"blue", (getter)rgbpixel_get_blue, (setter)rgbpixel_set_blue,
    (char *)"(int property)\n\nThe current blue value [0, 255]", 0 },
  { (char *)"hue", (getter)rgbpixel_get_hue, 0,
    (char *)"(float property)\n\nThe hue [0, 1.0]", 0 },
  { (char *)"saturation", (getter)rgbpixel_get_saturation, 0,
    (char *)"(float property)\n\nThe saturation [0, 1.0]", 0 },
  { (char *)"value", (getter)rgbpixel_get_value, 0,
    (char *)"(float property)\n\nThe value [0, 1.0]", 0 },
  { (char *)"cie_x", (getter)rgbpixel_get_cie_x, 0,
    (char *)"(float property)\n\nThe cie_x value [0, 1.0]", 0 },
  { (char *)"cie_y", (getter)rgbpixel_get_cie_y, 0,
    (char *)"(float property)\n\nThe cie_y value [0, 1.0]", 0 },
  { (char *)"cie_z", (getter)rgbpixel_get_cie_z, 0,
    (char *)"(float property)\n\nThe cie_z value [0, 1.0]", 0 },
  { (char *)"cyan", (getter)rgbpixel_get_cyan, 0,
    (char *)"(int property)\n\nThe cyan value [0, 255]", 0 },
  { (char *)"magenta", (getter)rgbpixel_get_magenta, 0,
    (char *)"(int property)\n\nThe magenta value [0, 255]", 0 },
  { (char *)"yellow", (getter)rgbpixel_get_yellow, 0,
    (char *)"(int property)\n\nThe yellow value [0, 255]", 0 },
  { NULL }
};

static PyObject* rgbpixel_new(PyTypeObject* pytype, PyObject* args,
			     PyObject* kwds) {
  int red, green, blue;
  if (PyArg_ParseTuple(args, CHAR_PTR_CAST "iii", &red, &green, &blue) <= 0)
    return 0;
  if (red < 0 || red > 255) {
    PyErr_Format(PyExc_ValueError, "'red' value '%d' is out of range (0, 255)", red);
    return 0;
  }
  if (green < 0 || green > 255) {
    PyErr_Format(PyExc_ValueError, "'green' value '%d' is out of range (0, 255)", green);
    return 0;
  }
  if (blue < 0 || blue > 255) {
    PyErr_Format(PyExc_ValueError, "'blue' value '%d' is out of range (0, 255)", blue);
    return 0;
  }
  RGBPixelObject* so = (RGBPixelObject*)pytype->tp_alloc(pytype, 0);
  so->m_x = new RGBPixel(red, green, blue);
  return (PyObject*)so;
}

static void rgbpixel_dealloc(PyObject* self) {
  RGBPixelObject* x = (RGBPixelObject*)self;
  delete x->m_x;
  self->ob_type->tp_free(self);
}

#define CREATE_GET_FUNC(name) static PyObject* rgbpixel_get_##name(PyObject* self) {\
  RGBPixel* x = ((RGBPixelObject*)self)->m_x; \
  return PyInt_FromLong((int)x->name()); \
}

#define CREATE_FLOAT_GET_FUNC(name) static PyObject* rgbpixel_get_##name(PyObject* self) {\
  RGBPixel* x = ((RGBPixelObject*)self)->m_x; \
  return PyFloat_FromDouble((int)x->name()); \
}

#define CREATE_SET_FUNC(name) static int rgbpixel_set_##name(PyObject* self, PyObject* value) {\
  RGBPixel* x = ((RGBPixelObject*)self)->m_x; \
  x->name((size_t)PyInt_AS_LONG(value)); \
  return 0; \
}

CREATE_GET_FUNC(red)
CREATE_GET_FUNC(green)
CREATE_GET_FUNC(blue)
CREATE_SET_FUNC(red)
CREATE_SET_FUNC(green)
CREATE_SET_FUNC(blue)
CREATE_FLOAT_GET_FUNC(hue)
CREATE_FLOAT_GET_FUNC(saturation)
CREATE_FLOAT_GET_FUNC(value)
CREATE_FLOAT_GET_FUNC(cie_x)
CREATE_FLOAT_GET_FUNC(cie_y)
CREATE_FLOAT_GET_FUNC(cie_z)
CREATE_GET_FUNC(cyan)
CREATE_GET_FUNC(magenta)
CREATE_GET_FUNC(yellow)

static PyObject* rgbpixel_richcompare(PyObject* a, PyObject* b, int op) {
  if (!is_RGBPixelObject(a) || !is_RGBPixelObject(b)) {
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
  }

  RGBPixel& ap = *((RGBPixelObject*)a)->m_x;
  RGBPixel& bp = *((RGBPixelObject*)b)->m_x;

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

static PyObject* rgbpixel_repr(PyObject* self) {
  RGBPixel* x = ((RGBPixelObject*)self)->m_x;
  return PyString_FromFormat("RGBPixel(%i, %i, %i)",
			     x->red(), x->green(), x->blue());
}

static PyObject* rgbpixel_str(PyObject* self) {
  RGBPixel* x = ((RGBPixelObject*)self)->m_x;
  return PyString_FromFormat("(%i, %i, %i)",
			     x->red(), x->green(), x->blue());
}

static long rgbpixel_hash(PyObject* self) {
  RGBPixel* x = ((RGBPixelObject*)self)->m_x;

  return ((x->red() << 16) & (x->green() << 8) & x->blue());
}

void init_RGBPixelType(PyObject* module_dict) {
  RGBPixelType.ob_type = &PyType_Type;
  RGBPixelType.tp_name = CHAR_PTR_CAST "gameracore.RGBPixel";
  RGBPixelType.tp_basicsize = sizeof(RGBPixelObject);
  RGBPixelType.tp_dealloc = rgbpixel_dealloc;
  RGBPixelType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  RGBPixelType.tp_new = rgbpixel_new;
  RGBPixelType.tp_getattro = PyObject_GenericGetAttr;
  RGBPixelType.tp_alloc = NULL; // PyType_GenericAlloc;
  RGBPixelType.tp_richcompare = rgbpixel_richcompare;
  RGBPixelType.tp_getset = rgbpixel_getset;
  RGBPixelType.tp_free = NULL; // _PyObject_Del;
  RGBPixelType.tp_repr = rgbpixel_repr;
  RGBPixelType.tp_str = rgbpixel_str;
  RGBPixelType.tp_hash = rgbpixel_hash;
  RGBPixelType.tp_doc = CHAR_PTR_CAST
    "__init__(*red*, *green*, *blue*).\n\n"
    "Example: ``RGBPixel(255, 0, 0)``.\n\n"
    "Each color value is in the range 0-255 (8 bits).\n\n"
    "For more information about color operations, see the `Color plugin docs`__.\n\n"
    ".. __: color.html";
  PyType_Ready(&RGBPixelType);
  PyDict_SetItemString(module_dict, "RGBPixel", (PyObject*)&RGBPixelType);
}
