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
#include "pixel.hpp"

using namespace Gamera;

extern "C" {
  static PyObject* image_new(PyTypeObject* pytype, PyObject* args,
			     PyObject* kwds);
  static void image_dealloc(PyObject* self);
  // Get/set
  static PyObject* image_get_data(PyObject* self);
  static PyObject* image_get_features(PyObject* self);
  static PyObject* image_get_id_name(PyObject* self);
  static PyObject* image_get_children_images(PyObject* self);
  static PyObject* image_get_classification_state(PyObject* self);
  static PyObject* image_get_scaling(PyObject* self);
  static int image_set_classification_state(PyObject* self);
  static int image_set_scaling(PyObject* self);
}

static PyTypeObject ImageType = {
  PyObject_HEAD_INIT(NULL)
  0,
};

static PyGetSetDef image_getset[] = {
  { "data", (getter)image_get_data, 0, "The underlying image data", 0 },
  { "features", (getter)image_get_features, 0, "The features of the image", 0 },
  { "id_name", (getter)image_get_id_name, 0, "A list of strings representing the possible classifications of the image.", 0 },
  { "children_images", (getter)image_get_children_images, 0, "A list of images created from classifications that produce images.", 0 },
  { "classification_state", (getter)image_get_classification_state, 0, "How (or whether) an image is classified", 0 },
  { "scaling", (getter)image_get_scaling, 0, "The scaling applied to the features", 0 },
  { NULL }
};

static PyObject* image_new(PyTypeObject* pytype, PyObject* args,
			   PyObject* kwds) {
  int nrows, ncols, page_offset_y, page_offset_x, pixel, format;
  if (PyArg_ParseTuple(args, "iiiiii", &nrows, &ncols, &page_offset_y,
		       &page_offset_x, &pixel, &format) <= 0) {
    return 0;
  }
  ImageObject* o;
  // we do not call rect_new here because we do all of the
  // required initializations
  o = (ImageObject*)pytype->tp_alloc(pytype, 0);
  /*
    This is looks really awful, but it is not. We are simply creating a
    matrix view and some matrix data based on the pixel type and storage
    format. The python interface only works with the base types, but here
    we have to create the appropriate C++ type. The type pixel/storage info
    is stored in the ImageDataObject structure, so we don't need another
    copy here. Additionally, all of the type information can be determined
    through RTTI, but it is simpler to use an enum and makes it easier to
    export to Python.
  */
  if (format == Python::DENSE) {
    if (pixel == Python::ONEBIT) {
      // create the appropriate data
      o->m_data = create_ImageDataObject(nrows, ncols, page_offset_y, page_offset_x,
				   pixel, format);
      // obtain a reference of the correct type to the data
      ImageData<OneBitPixel>* data =
	((ImageData<OneBitPixel>*)((ImageDataObject*)o->m_data)->m_x);
      // create the view for the data and put it in the m_x slot of
      // the RectObject struct
      ((RectObject*)o)->m_x =
	new ImageView<ImageData<OneBitPixel> >(*data, page_offset_y, page_offset_x,
					       nrows, ncols);
    } else if (pixel == Python::GREYSCALE) {
      // create the appropriate data
      o->m_data = create_ImageDataObject(nrows, ncols, page_offset_y, page_offset_x,
				   pixel, format);
      // obtain a reference of the correct type to the data
      ImageData<GreyScalePixel>* data =
	((ImageData<GreyScalePixel>*)((ImageDataObject*)o->m_data)->m_x);
      // create the view for the data and put it in the m_x slot of
      // the RectObject struct
      ((RectObject*)o)->m_x =
	new ImageView<ImageData<GreyScalePixel> >(*data, page_offset_y, page_offset_x,
						  nrows, ncols);
    } else if (pixel == Python::GREY16) {
      // create the appropriate data
      o->m_data = create_ImageDataObject(nrows, ncols, page_offset_y, page_offset_x,
				   pixel, format);
      // obtain a reference of the correct type to the data
      ImageData<Grey16Pixel>* data =
	((ImageData<Grey16Pixel>*)((ImageDataObject*)o->m_data)->m_x);
      // create the view for the data and put it in the m_x slot of
      // the RectObject struct
      ((RectObject*)o)->m_x =
	new ImageView<ImageData<Grey16Pixel> >(*data, page_offset_y, page_offset_x,
					       nrows, ncols);
    } else if (pixel == Python::FLOAT) {
      // create the appropriate data
      o->m_data = create_ImageDataObject(nrows, ncols, page_offset_y, page_offset_x,
				   pixel, format);
      // obtain a reference of the correct type to the data
      ImageData<FloatPixel>* data =
	((ImageData<FloatPixel>*)((ImageDataObject*)o->m_data)->m_x);
      // create the view for the data and put it in the m_x slot of
      // the RectObject struct
      ((RectObject*)o)->m_x =
	new ImageView<ImageData<FloatPixel> >(*data, page_offset_y, page_offset_x,
					      nrows, ncols);
    } else if (pixel == Python::RGB) {
      // create the appropriate data
      o->m_data = create_ImageDataObject(nrows, ncols, page_offset_y, page_offset_x,
				   pixel, format);
      // obtain a reference of the correct type to the data
      ImageData<RGBPixel>* data =
	((ImageData<RGBPixel>*)((ImageDataObject*)o->m_data)->m_x);
      // create the view for the data and put it in the m_x slot of
      // the RectObject struct
      ((RectObject*)o)->m_x =
	new ImageView<ImageData<RGBPixel> >(*data, page_offset_y, page_offset_x,
					    nrows, ncols);
    } else {
      PyErr_SetString(PyExc_TypeError, "Unkown Pixel type!");
      return 0;
    }
  } else if (format == Python::RLE) {
    if (pixel == Python::ONEBIT) {
      // create the appropriate data
      o->m_data = create_ImageDataObject(nrows, ncols, page_offset_y, page_offset_x,
				   pixel, format);
      // obtain a reference of the correct type to the data
      RleImageData<OneBitPixel>* data =
	((RleImageData<OneBitPixel>*)((ImageDataObject*)o->m_data)->m_x);
      // create the view for the data and put it in the m_x slot of
      // the RectObject struct
      ((RectObject*)o)->m_x =
	new ImageView<RleImageData<OneBitPixel> >(*data, page_offset_y, page_offset_x,
						  nrows, ncols);
    } else {
      PyErr_SetString(PyExc_TypeError,
		      "Pixel type must be Onebit for Rle data!");
      return 0;
    }
  } else {
    PyErr_SetString(PyExc_TypeError, "Unkown Format!");
    return 0;
  }
  /*
    Create the features array. This will load the array module
    and create an array object containing doubles.
  */
  PyObject* array_module = PyImport_ImportModule("array");
  if (array_module == 0)
    return 0;
  PyObject* array_dict = PyModule_GetDict(array_module);
  if (array_dict == 0)
    return 0;
  PyObject* array_func = PyDict_GetItemString(array_dict, "array");
  if (array_func == 0)
    return 0;
  PyObject* arglist = Py_BuildValue("(s)", "d");
  o->m_features = PyEval_CallObject(array_func, arglist);
  Py_DECREF(arglist);
  if (o->m_features == 0)
    return 0;
  /*
    id_name
  */
  o->m_id_name = PyList_New(0);
  if (o->m_id_name == 0)
    return 0;
  /*
    Children Images
  */
  o->m_children_images = PyList_New(0);
  if (o->m_children_images == 0)
    return 0;
  /*
    Classification state
  */
  o->m_classification_state = Py_BuildValue("i", Python::UNCLASSIFIED);
  /*
    Scaling
  */
  o->m_scaling = Py_BuildValue("i", 1);
  return (PyObject*)o;
}

static void image_dealloc(PyObject* self) {
  ImageObject* o = (ImageObject*)self;
  Py_DECREF(o->m_data);
  delete ((RectObject*)self)->m_x;
  self->ob_type->tp_free(self);
}

#define CREATE_GET_FUNC(name) static PyObject* image_get_##name(PyObject* self) {\
  ImageObject* o = (ImageObject*)self; \
  Py_INCREF(o->m_##name); \
  return o->m_##name; \
}

CREATE_GET_FUNC(data)
CREATE_GET_FUNC(features)
CREATE_GET_FUNC(id_name)
CREATE_GET_FUNC(children_images)
CREATE_GET_FUNC(classification_state)
CREATE_GET_FUNC(scaling)


void init_ImageType(PyObject* module_dict) {
  ImageType.ob_type = &PyType_Type;
  ImageType.tp_name = "gameracore.Image";
  ImageType.tp_basicsize = sizeof(ImageObject);
  ImageType.tp_dealloc = image_dealloc;
  ImageType.tp_flags = Py_TPFLAGS_DEFAULT;// | Py_TPFLAGS_BASETYPE;
  ImageType.tp_base = get_RectType();
  ImageType.tp_getset = image_getset;
  //ImageType.tp_methods = imagedata_methods;
  ImageType.tp_new = image_new;
  ImageType.tp_getattro = PyObject_GenericGetAttr;
  ImageType.tp_alloc = PyType_GenericAlloc;
  ImageType.tp_free = _PyObject_Del;
  PyType_Ready(&ImageType);
  PyDict_SetItemString(module_dict, "Image", (PyObject*)&ImageType);
  PyDict_SetItemString(module_dict, "UNCLASSIFIED", Py_BuildValue("i", Python::UNCLASSIFIED));
  PyDict_SetItemString(module_dict, "AUTOMATIC", Py_BuildValue("i", Python::AUTOMATIC));
  PyDict_SetItemString(module_dict, "HEURISTIC", Py_BuildValue("i", Python::HEURISTIC));
  PyDict_SetItemString(module_dict, "MANUAL", Py_BuildValue("i", Python::MANUAL));
}

