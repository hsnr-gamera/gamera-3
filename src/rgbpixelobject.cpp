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
}

static PyTypeObject RGBPixelType = {
  PyObject_HEAD_INIT(NULL)
  0,
};

PyTypeObject* get_RGBPixelType() {
  return &RGBPixelType;
}

bool is_RGBPixelObject(PyObject* x) {
  if (PyObject_TypeCheck(x, &RGBPixelType))
    return true;
  else
    return false;
}

PyObject* create_RGBPixelObject(const RGBPixel& p) {
  RGBPixelObject* o = (RGBPixelObject*)RGBPixelType.tp_alloc(&RGBPixelType, 0);
  o->m_x = new RGBPixel(p);
  return (PyObject*)o;
}

static PyGetSetDef rgbpixel_getset[] = {
  { "red", (getter)rgbpixel_get_red, (setter)rgbpixel_set_red,
    "the current red value", 0 },
  { "green", (getter)rgbpixel_get_green, (setter)rgbpixel_set_green,
    "the current green value", 0 },
  { "blue", (getter)rgbpixel_get_blue, (setter)rgbpixel_set_blue,
    "the current blue value", 0 },
};

static PyObject* rgbpixel_new(PyTypeObject* pytype, PyObject* args,
			     PyObject* kwds) {
  int red, green, blue;
  if (PyArg_ParseTuple(args, "iii", &red, &green, &blue) <= 0)
    return 0;
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
  return Py_BuildValue("i", (int)x->name()); \
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
  return PyString_FromFormat("<gameracore.RGBPixel red: %i green: %i blue: %i>",
			     x->red(), x->green(), x->blue());
}

void init_RGBPixelType(PyObject* module_dict) {
  RGBPixelType.ob_type = &PyType_Type;
  RGBPixelType.tp_name = "gameracore.RGBPixel";
  RGBPixelType.tp_basicsize = sizeof(RGBPixelObject);
  RGBPixelType.tp_dealloc = rgbpixel_dealloc;
  RGBPixelType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  RGBPixelType.tp_new = rgbpixel_new;
  RGBPixelType.tp_getattro = PyObject_GenericGetAttr;
  RGBPixelType.tp_alloc = PyType_GenericAlloc;
  RGBPixelType.tp_richcompare = rgbpixel_richcompare;
  RGBPixelType.tp_getset = rgbpixel_getset;
  RGBPixelType.tp_free = _PyObject_Del;
  RGBPixelType.tp_repr = rgbpixel_repr;
  PyType_Ready(&RGBPixelType);
  PyDict_SetItemString(module_dict, "RGBPixel", (PyObject*)&RGBPixelType);
}
