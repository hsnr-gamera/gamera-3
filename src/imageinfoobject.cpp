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
  static PyObject* imageinfo_new(PyTypeObject* pytpe, PyObject* args, PyObject* kwds);
  static void imageinfo_dealloc(PyObject* self);
  // get/set
  static int imageinfo_set_x_resolution(PyObject* self, PyObject* value);
  static PyObject* imageinfo_get_x_resolution(PyObject* self);
  static int imageinfo_set_y_resolution(PyObject* self, PyObject* value);
  static PyObject* imageinfo_get_y_resolution(PyObject* self);
  static int imageinfo_set_ncols(PyObject* self, PyObject* value);
  static PyObject* imageinfo_get_ncols(PyObject* self);
  static int imageinfo_set_nrows(PyObject* self, PyObject* value);
  static PyObject* imageinfo_get_nrows(PyObject* self);
  static int imageinfo_set_depth(PyObject* self, PyObject* value);
  static PyObject* imageinfo_get_depth(PyObject* self);
  static int imageinfo_set_ncolors(PyObject* self, PyObject* value);
  static PyObject* imageinfo_get_ncolors(PyObject* self);
}

static PyTypeObject ImageInfoType = {
  PyObject_HEAD_INIT(NULL)
  0,
};

static PyGetSetDef imageinfo_getset[] = {
  { "x_resolution", (getter)imageinfo_get_x_resolution,
    (setter)imageinfo_set_x_resolution, "The x resolution of the image." },
  { "y_resolution", (getter)imageinfo_get_y_resolution,
    (setter)imageinfo_set_y_resolution, "The y resolution of the image." },
  { "ncols", (getter)imageinfo_get_ncols,
    (setter)imageinfo_set_ncols, "The number of columns of the image." },
  { "nrows", (getter)imageinfo_get_nrows,
    (setter)imageinfo_set_nrows, "The number of rows of the image." },
  { "depth", (getter)imageinfo_get_depth,
    (setter)imageinfo_set_depth, "The bit depth of the image (in bits)." },
  { "ncolors", (getter)imageinfo_get_ncolors,
    (setter)imageinfo_set_ncolors, "The number of colors in the image." },
  { NULL }
};

PyTypeObject* get_ImageInfoType() {
  return &ImageInfoType;
}

static PyObject* imageinfo_new(PyTypeObject* pytype, PyObject* args, PyObject* kwds) {
  ImageInfoObject* o;
  o = (ImageInfoObject*)pytype->tp_alloc(pytype, 0);
  o->m_x = new ImageInfo();
  return (PyObject*)o;
}

static void imageinfo_dealloc(PyObject* self) {
  ImageInfoObject* x = (ImageInfoObject*)self;
  delete x->m_x;
  self->ob_type->tp_free(self);
}

#define CREATE_GET_FUNC(name) static PyObject* imageinfo_get_##name(PyObject* self) {\
  ImageInfo* x = ((ImageInfoObject*)self)->m_x; \
  return PyInt_FromLong((int)x->name()); \
}

#define CREATE_SET_FUNC(name) static int imageinfo_set_##name(PyObject* self, PyObject* value) {\
  ImageInfo* x = ((ImageInfoObject*)self)->m_x; \
  x->name((size_t)PyInt_AS_LONG(value)); \
  return 0; \
}

#define CREATE_GET_FLOAT_FUNC(name) static PyObject* imageinfo_get_##name(PyObject* self) {\
  ImageInfo* x = ((ImageInfoObject*)self)->m_x; \
  return Py_BuildValue("f", x->name()); \
}

#define CREATE_SET_FLOAT_FUNC(name) static int imageinfo_set_##name(PyObject* self, PyObject* value) {\
  ImageInfo* x = ((ImageInfoObject*)self)->m_x; \
  x->name(PyFloat_AS_DOUBLE(value)); \
  return 0; \
}

CREATE_GET_FLOAT_FUNC(x_resolution)
CREATE_SET_FLOAT_FUNC(x_resolution)
CREATE_GET_FLOAT_FUNC(y_resolution)
CREATE_SET_FLOAT_FUNC(y_resolution)
CREATE_GET_FUNC(nrows)
CREATE_SET_FUNC(nrows)
CREATE_GET_FUNC(ncols)
CREATE_SET_FUNC(ncols)
CREATE_GET_FUNC(depth)
CREATE_SET_FUNC(depth)
CREATE_GET_FUNC(ncolors)
CREATE_SET_FUNC(ncolors)

void init_ImageInfoType(PyObject* module_dict) {
  ImageInfoType.ob_type = &PyType_Type;
  ImageInfoType.tp_name = "gameracore.ImageInfo";
  ImageInfoType.tp_basicsize = sizeof(ImageInfoObject);
  ImageInfoType.tp_dealloc = imageinfo_dealloc;
  ImageInfoType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  ImageInfoType.tp_new = imageinfo_new;
  ImageInfoType.tp_getattro = PyObject_GenericGetAttr;
  ImageInfoType.tp_alloc = NULL; // PyType_GenericAlloc;
  ImageInfoType.tp_getset = imageinfo_getset;
  ImageInfoType.tp_free = NULL; // _PyObject_Del;
  ImageInfoType.tp_doc = "The ImageInfo class allows the properties of a disk-based image file to be examined without loading it.\n\nTo get image info, call the image_info(*filename*) function in ``gamera.core``.";
  PyType_Ready(&ImageInfoType);
  PyDict_SetItemString(module_dict, "ImageInfo", (PyObject*)&ImageInfoType);
}
