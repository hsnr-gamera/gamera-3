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
  static PyObject* imagedata_new(PyTypeObject* pytype, PyObject* args,
				 PyObject* kwds);
  static void imagedata_dealloc(PyObject* self);
  // get/set
  
}

static PyTypeObject ImageDataType = {
  PyObject_HEAD_INIT(NULL)
  0,
};

bool is_ImageDataObject(PyObject* x) {
  if (PyObject_TypeCheck(x, &ImageDataType))
    return true;
  else
    return false;
}

static PyObject* imagedata_new(PyTypeObject* pytype, PyObject* args,
			       PyObject* kwds) {
  int nrows, ncols, format, pixel;
  if (PyArg_ParseTuple(args, "iiii", &nrows, &ncols, &pixel, &format) <= 0)
    return 0;
  ImageDataObject* o;
  o = (ImageDataObject*)pytype->tp_alloc(pytype, 0);
  if (format == Python::DENSE) {
    if (pixel == Python::ONEBIT)
      o->m_x = new ImageData<OneBitPixel>(nrows, ncols);
    else if (pixel == Python::GREYSCALE)
      o->m_x = new ImageData<GreyScalePixel>(nrows, ncols);      
    else if (pixel == Python::GREY16)
      o->m_x = new ImageData<Grey16Pixel>(nrows, ncols);      
    else if (pixel == Python::FLOAT)
      o->m_x = new ImageData<FloatPixel>(nrows, ncols);      
    else if (pixel == Python::RGB)
      o->m_x = new ImageData<RGBPixel>(nrows, ncols);      
    else {
      PyErr_SetString(PyExc_TypeError, "Unkown Pixel type!");
      return 0;
    }
  } else if (format == Python::RLE) {
    if (pixel == Python::ONEBIT)
      o->m_x = new RleImageData<OneBitPixel>(nrows, ncols);
    else {
      PyErr_SetString(PyExc_TypeError,
		      "Pixel type must be Onebit for Rle data!");
      return 0;
    }
  } else {
    PyErr_SetString(PyExc_TypeError, "Unkown Format!");
    return 0;
  }
  return (PyObject*)o;
}
 
static void imagedata_dealloc(PyObject* self) {
  ImageDataObject* x = (ImageDataObject*)self;
  delete x->m_x;
  self->ob_type->tp_free(self);
}

void init_ImageDataType(PyObject* module_dict) {
  ImageDataType.ob_type = &PyType_Type;
  ImageDataType.tp_name = "gamera.ImageData";
  ImageDataType.tp_basicsize = sizeof(RectObject);
  ImageDataType.tp_dealloc = imagedata_dealloc;
  ImageDataType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  //ImageDataType.tp_methods = rect_methods;
  //ImageDataType.tp_getset = rect_getset;
  ImageDataType.tp_new = imagedata_new;
  ImageDataType.tp_getattro = PyObject_GenericGetAttr;
  ImageDataType.tp_alloc = PyType_GenericAlloc;
  //ImageDataType.tp_richcompare = rect_richcompare;
  ImageDataType.tp_free = _PyObject_Del;
  //ImageDataType.tp_repr = rect_repr;
  PyDict_SetItemString(module_dict, "ImageData", (PyObject*)&ImageDataType);
}


