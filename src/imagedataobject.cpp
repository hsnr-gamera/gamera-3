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
  static PyObject* imagedata_get_stride(PyObject* self);
  static PyObject* imagedata_get_ncols(PyObject* self);
  static PyObject* imagedata_get_nrows(PyObject* self);
  static PyObject* imagedata_get_page_offset_x(PyObject* self);
  static PyObject* imagedata_get_page_offset_y(PyObject* self);
  static PyObject* imagedata_get_size(PyObject* self);
  static PyObject* imagedata_get_bytes(PyObject* self);
  static PyObject* imagedata_get_mbytes(PyObject* self);
  static PyObject* imagedata_get_pixel_type(PyObject* self);
  static PyObject* imagedata_get_storage_format(PyObject* self);
  static int imagedata_set_page_offset_x(PyObject* self, PyObject* v);
  static int imagedata_set_page_offset_y(PyObject* self, PyObject* v);
  static int imagedata_set_nrows(PyObject* self, PyObject* v);
  static int imagedata_set_ncols(PyObject* self, PyObject* v);
  // methods
  static PyObject* imagedata_dimensions(PyObject* self, PyObject* args);
}

static PyTypeObject ImageDataType = {
  PyObject_HEAD_INIT(NULL)
  0,
};

static PyGetSetDef imagedata_getset[] = {
  { "nrows", (getter)imagedata_get_nrows, (setter)imagedata_set_nrows,
    "The number of rows", 0 },
  { "ncols", (getter)imagedata_get_ncols, (setter)imagedata_set_ncols,
    "The number of columns", 0 },
  { "page_offset_x", (getter)imagedata_get_page_offset_x,
    (setter)imagedata_set_page_offset_x,
    "The x offset in the page for the data", 0 },
  { "page_offset_y", (getter)imagedata_get_page_offset_y,
    (setter)imagedata_set_page_offset_y,
    "The y offset in the page for the data", 0 },
  { "stride", (getter)imagedata_get_stride, 0,
    "The length of the data stride", 0 },
  { "bytes", (getter)imagedata_get_bytes, 0,
    "The size of the data in bytes", 0 },
  { "mbytes", (getter)imagedata_get_mbytes, 0,
    "The size of the data in mbytes", 0 },
  { "pixel_type", (getter)imagedata_get_pixel_type, 0,
    "The type of the pixel stored in the object", 0 },
  { "storage_format", (getter)imagedata_get_storage_format, 0,
    "The format of the storage", 0 },
  { NULL }
};

static PyMethodDef imagedata_methods[] = {
  { "dimensions", imagedata_dimensions, METH_VARARGS },
  { NULL }
};

bool is_ImageDataObject(PyObject* x) {
  if (PyObject_TypeCheck(x, &ImageDataType))
    return true;
  else
    return false;
}

PyObject* create_ImageDataObject(int nrows, int ncols,
				 int page_offset_y, int page_offset_x,
				 int pixel_type, int storage_format) {
  ImageDataObject* o;
  o = (ImageDataObject*)ImageDataType.tp_alloc(&ImageDataType, 0);
  o->m_pixel_type = pixel_type;
  o->m_storage_format = storage_format;
  if (storage_format == Python::DENSE) {
    if (pixel_type == Python::ONEBIT)
      o->m_x = new ImageData<OneBitPixel>(nrows, ncols, page_offset_y,
					  page_offset_x);
    else if (pixel_type == Python::GREYSCALE)
      o->m_x = new ImageData<GreyScalePixel>(nrows, ncols, page_offset_y,
					     page_offset_x);      
    else if (pixel_type == Python::GREY16)
      o->m_x = new ImageData<Grey16Pixel>(nrows, ncols, page_offset_y,
					  page_offset_x);      
    else if (pixel_type == Python::FLOAT)
      o->m_x = new ImageData<FloatPixel>(nrows, ncols, page_offset_y,
					 page_offset_x);      
    else if (pixel_type == Python::RGB)
      o->m_x = new ImageData<RGBPixel>(nrows, ncols, page_offset_y,
				       page_offset_x);      
    else {
      PyErr_SetString(PyExc_TypeError, "Unkown Pixel type!");
      return 0;
    }
  } else if (storage_format == Python::RLE) {
    if (pixel_type == Python::ONEBIT)
      o->m_x = new RleImageData<OneBitPixel>(nrows, ncols, page_offset_y,
					     page_offset_x);
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

static PyObject* imagedata_new(PyTypeObject* pytype, PyObject* args,
			       PyObject* kwds) {
  int nrows, ncols, page_offset_y, page_offset_x, format, pixel;
  if (PyArg_ParseTuple(args, "iiiiii", &nrows, &ncols, &page_offset_y,
		       &page_offset_x, &pixel, &format) <= 0)
    return 0;
  
  return create_ImageDataObject(nrows, ncols, page_offset_y, page_offset_x,
				pixel, format);
}
 
static void imagedata_dealloc(PyObject* self) {
  printf("freeing image data\n");
  ImageDataObject* x = (ImageDataObject*)self;
  delete x->m_x;
  self->ob_type->tp_free(self);
}

#define CREATE_GET_FUNC(name) static PyObject* imagedata_get_##name(PyObject* self) {\
  ImageDataBase* x = ((ImageDataObject*)self)->m_x; \
  return Py_BuildValue("i", (int)x->name()); \
}

#define CREATE_SET_FUNC(name) static int imagedata_set_##name(PyObject* self, PyObject* value) {\
  ImageDataBase* x = ((ImageDataObject*)self)->m_x; \
  x->name((size_t)PyInt_AS_LONG(value)); \
  return 0; \
}

CREATE_GET_FUNC(stride)
CREATE_GET_FUNC(ncols)
CREATE_GET_FUNC(nrows)
CREATE_GET_FUNC(page_offset_x)
CREATE_GET_FUNC(page_offset_y)
CREATE_GET_FUNC(size)
CREATE_GET_FUNC(bytes)

CREATE_SET_FUNC(page_offset_x)
CREATE_SET_FUNC(page_offset_y)
CREATE_SET_FUNC(nrows)
CREATE_SET_FUNC(ncols)

static PyObject* imagedata_get_mbytes(PyObject* self) {
  ImageDataBase* x = ((ImageDataObject*)self)->m_x;
  return Py_BuildValue("d", x->mbytes());
}

static PyObject* imagedata_get_pixel_type(PyObject* self) {
  return Py_BuildValue("i", ((ImageDataObject*)self)->m_pixel_type);
}

static PyObject* imagedata_get_storage_format(PyObject* self) {
  return Py_BuildValue("i", ((ImageDataObject*)self)->m_storage_format);
}

static PyObject* imagedata_dimensions(PyObject* self, PyObject* args) {
  ImageDataBase* x = ((ImageDataObject*)self)->m_x;
  int nrows, ncols;
  if (PyArg_ParseTuple(args, "ii", &nrows, &ncols) <= 0)
    return 0;
  x->dimensions((size_t)nrows, (size_t)ncols);
  Py_INCREF(Py_None);
  return Py_None;
}

void init_ImageDataType(PyObject* module_dict) {
  ImageDataType.ob_type = &PyType_Type;
  ImageDataType.tp_name = "gameracore.ImageData";
  ImageDataType.tp_basicsize = sizeof(ImageDataObject);
  ImageDataType.tp_dealloc = imagedata_dealloc;
  ImageDataType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  ImageDataType.tp_getset = imagedata_getset;
  ImageDataType.tp_methods = imagedata_methods;
  ImageDataType.tp_new = imagedata_new;
  ImageDataType.tp_getattro = PyObject_GenericGetAttr;
  ImageDataType.tp_alloc = PyType_GenericAlloc;
  ImageDataType.tp_free = _PyObject_Del;
  PyDict_SetItemString(module_dict, "ImageData", (PyObject*)&ImageDataType);
  // Some constants
  PyDict_SetItemString(module_dict, "FLOAT",
		       Py_BuildValue("i", Python::FLOAT));
  PyDict_SetItemString(module_dict, "ONEBIT",
		       Py_BuildValue("i", Python::ONEBIT));
  PyDict_SetItemString(module_dict, "GREYSCALE",
		       Py_BuildValue("i", Python::GREYSCALE));
  PyDict_SetItemString(module_dict, "GREY16",
		       Py_BuildValue("i", Python::GREY16));
  PyDict_SetItemString(module_dict, "RGB",
		       Py_BuildValue("i", Python::RGB));
  PyDict_SetItemString(module_dict, "DENSE",
		       Py_BuildValue("i", Python::DENSE));
  PyDict_SetItemString(module_dict, "RLE",
		       Py_BuildValue("i", Python::RLE));
}


