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
#include "pixel.hpp"

using namespace Gamera;

extern "C" {
  static PyObject* image_new(PyTypeObject* pytype, PyObject* args,
			     PyObject* kwds);
  static PyObject* sub_image_new(PyTypeObject* pytype, PyObject* args,
				 PyObject* kwds);
  static PyObject* cc_new(PyTypeObject* pytype, PyObject* args,
				 PyObject* kwds);
  static void image_dealloc(PyObject* self);
  // methods
  static PyObject* image_get(PyObject* self, PyObject* args);
  static PyObject* image_set(PyObject* self, PyObject* args);
  static PyObject* image_getitem(PyObject* self, PyObject* args);
  static PyObject* image_setitem(PyObject* self, PyObject* args);
  static PyObject* image_len(PyObject* self, PyObject* args);
  static PyObject* image_sort(PyObject* self, PyObject* args);
  // Get/set
  static PyObject* image_get_data(PyObject* self);
  static PyObject* image_get_features(PyObject* self);
  static PyObject* image_get_id_name(PyObject* self);
  static PyObject* image_get_children_images(PyObject* self);
  static PyObject* image_get_classification_state(PyObject* self);
  static PyObject* image_get_scaling(PyObject* self);
  static PyObject* image_get_resolution(PyObject* self);
  static int image_set_features(PyObject* self, PyObject* v);
  static int image_set_id_name(PyObject* self, PyObject* v);
  static int image_set_children_images(PyObject* self, PyObject* v);
  static int image_set_classification_state(PyObject* self, PyObject* v);
  static int image_set_scaling(PyObject* self, PyObject* v);
  static int image_set_resolution(PyObject* self, PyObject* v);
  static PyObject* cc_get_label(PyObject* self);
  static int cc_set_label(PyObject* self, PyObject* v);
}

static PyTypeObject ImageType = {
  PyObject_HEAD_INIT(NULL)
  0,
};

PyTypeObject* get_ImageType() {
  return &ImageType;
}

static PyTypeObject SubImageType = {
  PyObject_HEAD_INIT(NULL)
  0,
};

PyTypeObject* get_SubImageType() {
  return &SubImageType;
}

static PyTypeObject CCType = {
  PyObject_HEAD_INIT(NULL)
  0,
};

PyTypeObject* get_CCType() {
  return &CCType;
}

static PyGetSetDef image_getset[] = {
  { "data", (getter)image_get_data, 0, "The underlying image data", 0 },
  { "features", (getter)image_get_features, (setter)image_set_features,
    "The features of the image", 0 },
  { "id_name", (getter)image_get_id_name, (setter)image_set_id_name,
    "A list of strings representing the classifications of the image.",
    0 },
  { "children_images", (getter)image_get_children_images, 
    (setter)image_set_children_images,
    "A list of images created from classifications that produce images.", 0 },
  { "classification_state", (getter)image_get_classification_state, 
    (setter)image_set_classification_state,
    "How (or whether) an image is classified", 0 },
  { "scaling", (getter)image_get_scaling, (setter)image_set_scaling,
    "The scaling applied to the features", 0 },
  { "resolution", (getter)image_get_resolution, (setter)image_set_resolution,
    "The resolution of the image", 0 },
  { NULL }
};

static PyGetSetDef cc_getset[] = {
  { "label", (getter)cc_get_label, (setter)cc_set_label, "The label for the Cc", 0},
  { NULL }
};

static PyMethodDef image_methods[] = {
  { "get", image_get, METH_VARARGS },
  { "set", image_set, METH_VARARGS },
  { "__getitem__", image_getitem, METH_VARARGS },
  { "__setitem__", image_setitem, METH_VARARGS },  
  { "__len__", image_len, METH_NOARGS },  
  { "sort", image_sort, METH_NOARGS },  
  { NULL }
};

static PyObject* image_new(PyTypeObject* pytype, PyObject* args,
			   PyObject* kwds) {
  int nrows, ncols, pixel, format, offset_y, offset_x;
  if (PyArg_ParseTuple(args, "iiiiii", &offset_y, &offset_x, &nrows, &ncols, &pixel, &format) <= 0) {
    return 0;
  }
  ImageObject* o;
  // we do not call rect_new here because we do all of the
  // required initializations
  o = (ImageObject*)pytype->tp_alloc(pytype, 0);
  // initialize the weakreflist
  o->m_weakreflist = NULL;
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
  try {
    if (format == DENSE) {
      if (pixel == ONEBIT) {
	o->m_data = create_ImageDataObject(nrows, ncols, offset_y, offset_x, pixel, format);
	ImageData<OneBitPixel>* data =
	  ((ImageData<OneBitPixel>*)((ImageDataObject*)o->m_data)->m_x);
	((RectObject*)o)->m_x =
	  new ImageView<ImageData<OneBitPixel> >(*data, offset_y, offset_x, nrows, ncols);
      } else if (pixel == GREYSCALE) {
	o->m_data = create_ImageDataObject(nrows, ncols, offset_y, offset_x, pixel, format);
	ImageData<GreyScalePixel>* data =
	  ((ImageData<GreyScalePixel>*)((ImageDataObject*)o->m_data)->m_x);
	((RectObject*)o)->m_x =
	  new ImageView<ImageData<GreyScalePixel> >(*data, offset_y, offset_x, nrows, ncols);
      } else if (pixel == GREY16) {
	o->m_data = create_ImageDataObject(nrows, ncols, offset_y, offset_x, pixel, format);
	ImageData<Grey16Pixel>* data =
	  ((ImageData<Grey16Pixel>*)((ImageDataObject*)o->m_data)->m_x);
	((RectObject*)o)->m_x =
	  new ImageView<ImageData<Grey16Pixel> >(*data, offset_y, offset_x, nrows, ncols);
      } else if (pixel == Gamera::FLOAT) {
	o->m_data = create_ImageDataObject(nrows, ncols, offset_y, offset_x, pixel, format);
	ImageData<FloatPixel>* data =
	  ((ImageData<FloatPixel>*)((ImageDataObject*)o->m_data)->m_x);
	((RectObject*)o)->m_x =
	  new ImageView<ImageData<FloatPixel> >(*data, offset_y, offset_x, nrows, ncols);
      } else if (pixel == RGB) {
	o->m_data = create_ImageDataObject(nrows, ncols, offset_y, offset_x, pixel, format);
	ImageData<RGBPixel>* data =
	  ((ImageData<RGBPixel>*)((ImageDataObject*)o->m_data)->m_x);
	((RectObject*)o)->m_x =
	  new ImageView<ImageData<RGBPixel> >(*data, offset_y, offset_x, nrows, ncols);
      } else {
	PyErr_SetString(PyExc_TypeError, "Unkown Pixel type!");
	return 0;
      }
    } else if (format == RLE) {
      if (pixel == ONEBIT) {
	o->m_data = create_ImageDataObject(nrows, ncols, offset_y, offset_x, pixel, format);
	RleImageData<OneBitPixel>* data =
	  ((RleImageData<OneBitPixel>*)((ImageDataObject*)o->m_data)->m_x);
	((RectObject*)o)->m_x =
	  new ImageView<RleImageData<OneBitPixel> >(*data, offset_y, offset_x, nrows, ncols);
      } else {
	PyErr_SetString(PyExc_TypeError,
			"Pixel type must be Onebit for Rle data!");
	return 0;
      }
    } else {
      PyErr_SetString(PyExc_TypeError, "Unkown Format!");
      return 0;
    }
  } catch (std::exception& e) {
    PyErr_SetString(PyExc_RuntimeError, e.what());
    return 0;
  }
  return init_image_members(o);
}

PyObject* sub_image_new(PyTypeObject* pytype, PyObject* args, PyObject* kwds) {
  PyObject* image;
  int nrows, ncols, off_x, off_y;
  if (PyArg_ParseTuple(args, "Oiiii", &image, &off_y, &off_x, &nrows,
		       &ncols) <= 0)
    return 0;
  if (!is_ImageObject(image)) {
    PyErr_SetString(PyExc_TypeError, "First argument must be an image!");
    return 0;
  }

  ImageObject* o;
  o = (ImageObject*)pytype->tp_alloc(pytype, 0);
  o->m_data = ((ImageObject*)image)->m_data;
  Py_INCREF(o->m_data);
  int pixel, format;
  pixel = ((ImageDataObject*)o->m_data)->m_pixel_type;
  format = ((ImageDataObject*)o->m_data)->m_storage_format;
  
  try {
    if (format == DENSE) {
      if (pixel == ONEBIT) {
	ImageData<OneBitPixel>* data =
	  ((ImageData<OneBitPixel>*)((ImageDataObject*)o->m_data)->m_x);
	((RectObject*)o)->m_x =
	  new ImageView<ImageData<OneBitPixel> >(*data, off_y, off_x, nrows, ncols);
      } else if (pixel == GREYSCALE) {
	ImageData<GreyScalePixel>* data =
	  ((ImageData<GreyScalePixel>*)((ImageDataObject*)o->m_data)->m_x);
	((RectObject*)o)->m_x =
	  new ImageView<ImageData<GreyScalePixel> >(*data, off_y, off_x, nrows,
						    ncols);
      } else if (pixel == GREY16) {
	ImageData<Grey16Pixel>* data =
	  ((ImageData<Grey16Pixel>*)((ImageDataObject*)o->m_data)->m_x);
	((RectObject*)o)->m_x =
	  new ImageView<ImageData<Grey16Pixel> >(*data, off_y, off_x, nrows, ncols);
      } else if (pixel == Gamera::FLOAT) {
	ImageData<FloatPixel>* data =
	  ((ImageData<FloatPixel>*)((ImageDataObject*)o->m_data)->m_x);
	((RectObject*)o)->m_x =
	  new ImageView<ImageData<FloatPixel> >(*data, off_y, off_x, nrows, ncols);
      } else if (pixel == RGB) {
	ImageData<RGBPixel>* data =
	  ((ImageData<RGBPixel>*)((ImageDataObject*)o->m_data)->m_x);
	((RectObject*)o)->m_x =
	  new ImageView<ImageData<RGBPixel> >(*data, off_y, off_x, nrows, ncols);
      } else {
	PyErr_SetString(PyExc_TypeError, "Unkown Pixel type!");
	return 0;
      }
    } else if (format == RLE) {
      if (pixel == ONEBIT) {
	RleImageData<OneBitPixel>* data =
	  ((RleImageData<OneBitPixel>*)((ImageDataObject*)o->m_data)->m_x);
	((RectObject*)o)->m_x =
	  new ImageView<RleImageData<OneBitPixel> >(*data, off_y, off_x, nrows,
						    ncols);
      } else {
	PyErr_SetString(PyExc_TypeError,
			"Pixel type must be Onebit for Rle data!");
	return 0;
      }
    } else {
      PyErr_SetString(PyExc_TypeError, "Unkown Format!");
      return 0;
    }
  } catch (std::exception& e) {
    PyErr_SetString(PyExc_RuntimeError, e.what());
    return 0;
  }
  // set the resolution
  ((Image*)((RectObject*)o)->m_x)->resolution(((Image*)((RectObject*)image)->m_x)->resolution());
  return init_image_members(o);
}

PyObject* cc_new(PyTypeObject* pytype, PyObject* args, PyObject* kwds) {
  PyObject* image;
  int nrows, ncols, off_x, off_y;
  int label;
  if (PyArg_ParseTuple(args, "Oiiiii", &image, &label, &off_y, &off_x, &nrows,
		       &ncols) <= 0)
    return 0;
  if (!is_ImageObject(image)) {
    PyErr_SetString(PyExc_TypeError, "First argument must be an image!");
    return 0;
  }

  ImageObject* o;
  o = (ImageObject*)pytype->tp_alloc(pytype, 0);
  o->m_data = ((ImageObject*)image)->m_data;
  Py_INCREF(o->m_data);
  int pixel, format;
  pixel = ((ImageDataObject*)o->m_data)->m_pixel_type;
  format = ((ImageDataObject*)o->m_data)->m_storage_format;
  try {
    if (pixel != ONEBIT) {
      PyErr_SetString(PyExc_TypeError, "Image must be OneBit!");
      return 0;
    }
    
    if (format == DENSE) {
      ImageData<OneBitPixel>* data =
	((ImageData<OneBitPixel>*)((ImageDataObject*)o->m_data)->m_x);
      ((RectObject*)o)->m_x =
	new ConnectedComponent<ImageData<OneBitPixel> >(*data, label, off_y,
							off_x, nrows, ncols);
    } else if (format == RLE) {
      RleImageData<OneBitPixel>* data =
	((RleImageData<OneBitPixel>*)((ImageDataObject*)o->m_data)->m_x);
      ((RectObject*)o)->m_x =
	new ConnectedComponent<RleImageData<OneBitPixel> >(*data, label,
							   off_y, off_x, nrows,
							   ncols);
    } else {
      PyErr_SetString(PyExc_TypeError, "Unkown Format!");
      return 0;
    }
  } catch (std::exception& e) {
    PyErr_SetString(PyExc_RuntimeError, e.what());
    return 0;
  }
  // set the resolution
  ((Image*)((RectObject*)o)->m_x)->resolution(((Image*)((RectObject*)image)->m_x)->resolution());
  return init_image_members(o);
}

static void image_dealloc(PyObject* self) {
  ImageObject* o = (ImageObject*)self;
  Py_DECREF(o->m_data);
  if (o->m_weakreflist != NULL) {
    //printf("dealing with weak refs\n");
    PyObject_ClearWeakRefs(self);
  }
  delete ((RectObject*)self)->m_x;

  // Added in an attempt to fix a leak.
  Py_DECREF(o->m_features);
  Py_DECREF(o->m_id_name);
  Py_DECREF(o->m_children_images);
  Py_DECREF(o->m_classification_state);

  // PyObject_Del(self);
  self->ob_type->tp_free(self);
  //free(self);
}

static PyObject* image_get(PyObject* self, int row, int col) {
  RectObject* o = (RectObject*)self;
  ImageDataObject* od = (ImageDataObject*)((ImageObject*)self)->m_data;
  if (is_CCObject(self)) {
    return Py_BuildValue("i", ((Cc*)o->m_x)->get((size_t)row, (size_t)col));
  } else if (od->m_pixel_type == Gamera::FLOAT) {
    return Py_BuildValue("d", ((FloatImageView*)o->m_x)->get((size_t)row, (size_t)col));
  } else if (od->m_storage_format == RLE) {
    return Py_BuildValue("i", ((OneBitRleImageView*)o->m_x)->get((size_t)row, (size_t)col));
  } else if (od->m_pixel_type == RGB) {
    return create_RGBPixelObject(((RGBImageView*)o->m_x)->get((size_t)row, (size_t)col));
  } else if (od->m_pixel_type == GREYSCALE) {
    return Py_BuildValue("i", ((GreyScaleImageView*)o->m_x)->get((size_t)row, (size_t)col));
  } else if (od->m_pixel_type == GREY16) {
    return Py_BuildValue("i", ((Grey16ImageView*)o->m_x)->get((size_t)row, (size_t)col));
  } else { // ONEBIT
    return Py_BuildValue("i", ((OneBitImageView*)o->m_x)->get((size_t)row, (size_t)col));
  }
}

static PyObject* image_set(PyObject* self, int row, int col, PyObject* value) {
  RectObject* o = (RectObject*)self;
  ImageDataObject* od = (ImageDataObject*)((ImageObject*)self)->m_data;
  if (is_CCObject(self)) {
    if (!PyInt_Check(value)) {
      PyErr_SetString(PyExc_TypeError, "image_set for CC objects must be an int.");
      return 0;
    }
    ((Cc*)o->m_x)->set((size_t)row, (size_t)col, (OneBitPixel)PyInt_AS_LONG(value));
  } else if (od->m_pixel_type == Gamera::FLOAT) {
    if (!PyFloat_Check(value)) {
      PyErr_SetString(PyExc_TypeError, "image_set for Float objects must be an float.");
      return 0;
    }
    ((FloatImageView*)o->m_x)->set((size_t)row, (size_t)col, PyFloat_AS_DOUBLE(value));
  } else if (od->m_storage_format == RLE) {
    if (!PyInt_Check(value)) {
      PyErr_SetString(PyExc_TypeError, "image_set for OneBit objects must be an int.");
      return 0;
    }
    ((OneBitRleImageView*)o->m_x)->set((size_t)row, (size_t)col,
				       (OneBitPixel)PyInt_AS_LONG(value));
  } else if (od->m_pixel_type == RGB) {
    if (!is_RGBPixelObject((PyObject*)value)) {
      PyErr_SetString(PyExc_TypeError, "Value is not an RGBPixel!");
      return 0;
    }
    RGBPixelObject* v = (RGBPixelObject*)value;
    ((RGBImageView*)o->m_x)->set((size_t)row, (size_t)col, *v->m_x);
  } else if (od->m_pixel_type == GREYSCALE) {
    if (!PyInt_Check(value)) {
      PyErr_SetString(PyExc_TypeError, "image_set for GreyScale objects must be an int.");
      return 0;
    }
    ((GreyScaleImageView*)o->m_x)->set((size_t)row, (size_t)col,
				       (GreyScalePixel)PyInt_AS_LONG(value));
  } else if (od->m_pixel_type == GREY16) {
    if (!PyInt_Check(value)) {
      PyErr_SetString(PyExc_TypeError, "image_set for Grey16 objects must be an int.");
      return 0;
    }
    ((Grey16ImageView*)o->m_x)->set((size_t)row, (size_t)col,
				    (Grey16Pixel)PyInt_AS_LONG(value));
  } else if (od->m_pixel_type == ONEBIT) {
    if (!PyInt_Check(value)) {
      PyErr_SetString(PyExc_TypeError, "image_set for OneBit objects must be an int.");
      return 0;
    }
    ((OneBitImageView*)o->m_x)->set((size_t)row, (size_t)col,
				    (OneBitPixel)PyInt_AS_LONG(value));
  }
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject* image_get(PyObject* self, PyObject* args) {
  Image* image = (Image*)((RectObject*)self)->m_x;
  int row, col;
  PyArg_ParseTuple(args, "ii", &row, &col);
  if (size_t(row) >= image->nrows() || size_t(col) >= image->ncols()) {
    PyErr_SetString(PyExc_IndexError, "Out of bounds for image");
    return 0;
  }
  return image_get(self, row, col);
}

static PyObject* image_set(PyObject* self, PyObject* args) {
  Image* image = (Image*)((RectObject*)self)->m_x;
  int row, col;
  PyObject* value;
  if (PyArg_ParseTuple(args, "iiO", &row, &col, &value) <= 0)
    return 0;
  if (size_t(row) >= image->nrows() || size_t(col) >= image->ncols()) {
    PyErr_SetString(PyExc_IndexError, "Out of bounds for image");
    return 0;
  }
  return image_set(self, row, col, value);
}

// convert Python indexing into row/col format for images
static inline int get_rowcol(Image* image, long index, size_t* row, size_t* col) {
  if (index < 0) {
    size_t len = image->ncols() * image->nrows();
    size_t real_index = len + index;
    *row = real_index / image->ncols();
    *col = real_index - (*row * image->ncols());
  } else { 
    *row = (size_t)(index / image->ncols());
    *col = (size_t)(index - (*row * image->ncols()));
  }
  if (size_t(*row) >= image->nrows() || size_t(*col) >= image->ncols()) {
    PyErr_SetString(PyExc_IndexError, "Out of bounds for image");
    return -1;
  }
  return 0;
}

static PyObject* image_getitem(PyObject* self, PyObject* args) {
  Image* image = (Image*)((RectObject*)self)->m_x;
  int index;
  if (PyArg_ParseTuple(args, "i", &index) <= 0)
    return 0;
  size_t row, col;
  if (get_rowcol(image, index, &row, &col) < 0)
    return 0;
  return image_get(self, row, col);
}

static PyObject* image_setitem(PyObject* self, PyObject* args) {
  Image* image = (Image*)((RectObject*)self)->m_x;
  int index;
  PyObject* value;
  if (PyArg_ParseTuple(args, "iO", &index, &value) <= 0)
    return 0;
  size_t row, col;
  if (get_rowcol(image, index, &row, &col) < 0)
    return 0;
  return image_set(self, row, col, value);
}

static PyObject* image_len(PyObject* self, PyObject* args) {
  Image* image = (Image*)((RectObject*)self)->m_x;
  return Py_BuildValue("i", (long)(image->nrows() * image->ncols()));
}

static PyObject* image_sort(PyObject* self, PyObject* args) {
  Image* image = (Image*)((RectObject*)self)->m_x;
  ImageDataObject* od = (ImageDataObject*)((ImageObject*)self)->m_data;
  if (is_CCObject(self)) {
    Cc* im = (Cc*)image;
    std::sort(im->vec_begin(), im->vec_end());
  } else if (od->m_pixel_type == Gamera::FLOAT) {
    FloatImageView* im = (FloatImageView*)image;
    std::sort(im->vec_begin(), im->vec_end());
  } else if (od->m_storage_format == RLE) {
    OneBitRleImageView* im = (OneBitRleImageView*)image;
    std::sort(im->vec_begin(), im->vec_end());
  } else if (od->m_pixel_type == RGB) {
    PyErr_SetString(PyExc_TypeError, "RGB pixels cannot be sorted");
    return 0;
  } else if (od->m_pixel_type == GREYSCALE) {
    GreyScaleImageView* im = (GreyScaleImageView*)image;
    std::sort(im->vec_begin(), im->vec_end());
  } else if (od->m_pixel_type == GREY16) {
    Grey16ImageView* im = (Grey16ImageView*)image;
    std::sort(im->vec_begin(), im->vec_end());
  } else { // ONEBIT
    OneBitImageView* im = (OneBitImageView*)image;
    std::sort(im->vec_begin(), im->vec_end());
  }
  Py_INCREF(Py_None);
  return Py_None;
}

#define CREATE_GET_FUNC(name) static PyObject* image_get_##name(PyObject* self) {\
  ImageObject* o = (ImageObject*)self; \
  Py_INCREF(o->m_##name); \
  return o->m_##name; \
}

#define CREATE_SET_FUNC(name) static int image_set_##name(PyObject* self, PyObject* v) {\
  ImageObject* o = (ImageObject*)self; \
  Py_DECREF(o->m_##name); \
  o->m_##name = v; \
  Py_INCREF(o->m_##name); \
  return 0; \
}

CREATE_GET_FUNC(data)
CREATE_GET_FUNC(features)
CREATE_SET_FUNC(features)
CREATE_SET_FUNC(id_name)
CREATE_GET_FUNC(id_name)
CREATE_GET_FUNC(children_images)
CREATE_SET_FUNC(children_images)
CREATE_GET_FUNC(classification_state)
CREATE_SET_FUNC(classification_state)


static PyObject* image_get_scaling(PyObject* self) {
  RectObject* o = (RectObject*)self;
  return Py_BuildValue("f", ((Image*)o->m_x)->scaling());
}

static int image_set_scaling(PyObject* self, PyObject* v) {
  RectObject* o = (RectObject*)self;
  if (!PyFloat_Check(v)) {
    PyErr_SetString(PyExc_TypeError, "Type Error!");
    return -1;
  }
  ((Image*)o->m_x)->scaling(PyFloat_AS_DOUBLE(v));
  return 0;
}

static PyObject* image_get_resolution(PyObject* self) {
  RectObject* o = (RectObject*)self;
  return Py_BuildValue("f", ((Image*)o->m_x)->resolution());
}

static int image_set_resolution(PyObject* self, PyObject* v) {
  RectObject* o = (RectObject*)self;
  if (!PyFloat_Check(v)) {
    PyErr_SetString(PyExc_TypeError, "Type Error!");
    return -1;
  }
  ((Image*)o->m_x)->resolution(PyFloat_AS_DOUBLE(v));
  return 0;
}

static PyObject* cc_get_label(PyObject* self) {
  RectObject* o = (RectObject*)self;
  return Py_BuildValue("i", ((Cc*)o->m_x)->label());
}

static int cc_set_label(PyObject* self, PyObject* v) {
  RectObject* o = (RectObject*)self;
  if (!PyInt_Check(v)) {
    PyErr_SetString(PyExc_TypeError, "Type Error!");
    return -1;
  }
  ((Cc*)o->m_x)->label(PyInt_AS_LONG(v));
  return 0;
}


void init_ImageType(PyObject* module_dict) {
  ImageType.ob_type = &PyType_Type;
  ImageType.tp_name = "gameracore.Image";
  ImageType.tp_basicsize = sizeof(ImageObject);
  ImageType.tp_dealloc = image_dealloc;
  ImageType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_WEAKREFS;
  ImageType.tp_base = get_RectType();
  ImageType.tp_getset = image_getset;
  ImageType.tp_methods = image_methods;
  ImageType.tp_new = image_new;
  ImageType.tp_getattro = PyObject_GenericGetAttr;
  ImageType.tp_alloc = PyType_GenericAlloc;
  ImageType.tp_free = NULL; //_PyObject_Del;
  ImageType.tp_weaklistoffset = offsetof(ImageObject, m_weakreflist);
  PyType_Ready(&ImageType);
  PyDict_SetItemString(module_dict, "Image", (PyObject*)&ImageType);

  SubImageType.ob_type = &PyType_Type;
  SubImageType.tp_name = "gameracore.SubImage";
  SubImageType.tp_basicsize = sizeof(SubImageObject);
  SubImageType.tp_dealloc = image_dealloc;
  SubImageType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  SubImageType.tp_base = &ImageType;
  SubImageType.tp_new = sub_image_new;
  SubImageType.tp_getattro = PyObject_GenericGetAttr;
  SubImageType.tp_alloc = PyType_GenericAlloc;
  SubImageType.tp_free = NULL; // _PyObject_Del;
  PyType_Ready(&SubImageType);
  PyDict_SetItemString(module_dict, "SubImage", (PyObject*)&SubImageType);

  CCType.ob_type = &PyType_Type;
  CCType.tp_name = "gameracore.Cc";
  CCType.tp_basicsize = sizeof(CCObject);
  CCType.tp_dealloc = image_dealloc;
  CCType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  CCType.tp_base = &ImageType;
  CCType.tp_new = cc_new;
  CCType.tp_getset = cc_getset;
  CCType.tp_getattro = PyObject_GenericGetAttr;
  CCType.tp_alloc = PyType_GenericAlloc;
  CCType.tp_free = NULL; //_PyObject_Del;
  PyType_Ready(&CCType);
  PyDict_SetItemString(module_dict, "Cc", (PyObject*)&CCType);

  // some constants
  PyDict_SetItemString(module_dict, "UNCLASSIFIED",
		       Py_BuildValue("i", UNCLASSIFIED));
  PyDict_SetItemString(module_dict, "AUTOMATIC",
		       Py_BuildValue("i", AUTOMATIC));
  PyDict_SetItemString(module_dict, "HEURISTIC",
		       Py_BuildValue("i", HEURISTIC));
  PyDict_SetItemString(module_dict, "MANUAL",
		       Py_BuildValue("i", MANUAL));
}

