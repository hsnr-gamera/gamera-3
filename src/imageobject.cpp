/*
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
  static int image_traverse(PyObject* self, visitproc visit, void* arg);
  static int image_clear(PyObject* self);
  // methods
  static PyObject* image_get(PyObject* self, PyObject* args);
  static PyObject* image_set(PyObject* self, PyObject* args);
  static PyObject* image_getitem(PyObject* self, PyObject* args);
  static PyObject* image_setitem(PyObject* self, PyObject* args);
  static PyObject* image_len(PyObject* self, PyObject* args);
  // Removed 07/28/04 MGD.  Can't figure out why this is useful.
  // static PyObject* image_sort(PyObject* self, PyObject* args);
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
  { "data", (getter)image_get_data, 0, "(read-only property)\n\nReturns the underlying ImageData__ object.\n\n.. __: gamera.core.ImageData.html", 0 },
  { "features", (getter)image_get_features, (setter)image_set_features,
    "(read/write property)\n\nThe feature vector of the image (of type array)", 0 },
  { "id_name", (getter)image_get_id_name, (setter)image_set_id_name,
    "(read/write property)\n\nA list of strings representing the classifications of the image.",
    0 },
  { "children_images", (getter)image_get_children_images, 
    (setter)image_set_children_images,
    "(read/write property)\n\nA list of images created from classifications that produce images, such as splitting algorithms.", 0 },
  { "classification_state", (getter)image_get_classification_state, 
    (setter)image_set_classification_state,
    "(read/write property)\n\nHow (or whether) an image is classified", 0 },
  { "scaling", (getter)image_get_scaling, (setter)image_set_scaling,
    "(read/write property)\n\nThe scaling (if any) applied to the features as a floating-point value.", 0 },
  { "resolution", (getter)image_get_resolution, (setter)image_set_resolution,
    "(read/write property)\n\nThe resolution of the image", 0 },
  { NULL }
};

static PyGetSetDef cc_getset[] = {
  { "label", (getter)cc_get_label, (setter)cc_set_label, "(read/write property)\n\nThe pixel label value for the Cc", 0},
  { NULL }
};

static PyMethodDef image_methods[] = {
  { "get", image_get, METH_VARARGS, "**get** (Int *y*, Int *x*)\n\nGets a pixel value at the given (y, x) coordinate.\n\nThis coordinate is relative to the image view, not the logical coordinates." },
  { "set", image_set, METH_VARARGS, "**set** (Int *y*, Int *x*)\n\nSets a pixel value at the given (y, x) coordinate.\n\nThis coordinate is relative to the image view, not the logical coordinates." },
  { "__getitem__", image_getitem, METH_VARARGS },
  { "__setitem__", image_setitem, METH_VARARGS },  
  { "__len__", image_len, METH_NOARGS },  
  // Removed 07/28/04 MGD.  Can't figure out why this is useful.
  // { "sort", image_sort, METH_NOARGS },  
  { NULL }
};

static bool _image_new(int offset_y, int offset_x, int nrows, int ncols, int pixel, int format, 
		       ImageDataObject* &py_data, Rect* &image) {
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
	py_data = (ImageDataObject*)create_ImageDataObject(nrows, ncols, offset_y, offset_x, pixel, format);
	ImageData<OneBitPixel>* data = (ImageData<OneBitPixel>*)(py_data->m_x);
	image = (Rect*)new ImageView<ImageData<OneBitPixel> >(*data, offset_y, offset_x, nrows, ncols);
      } else if (pixel == GREYSCALE) {
	py_data = (ImageDataObject*)create_ImageDataObject(nrows, ncols, offset_y, offset_x, pixel, format);
	ImageData<GreyScalePixel>* data = (ImageData<GreyScalePixel>*)(py_data->m_x);
	image = (Rect *)new ImageView<ImageData<GreyScalePixel> >(*data, offset_y, offset_x, nrows, ncols);
      } else if (pixel == GREY16) {
	py_data = (ImageDataObject*)create_ImageDataObject(nrows, ncols, offset_y, offset_x, pixel, format);
	ImageData<Grey16Pixel>* data = (ImageData<Grey16Pixel>*)(py_data->m_x);
	image = (Rect*)new ImageView<ImageData<Grey16Pixel> >(*data, offset_y, offset_x, nrows, ncols);
      } else if (pixel == Gamera::FLOAT) {
	py_data = (ImageDataObject*)create_ImageDataObject(nrows, ncols, offset_y, offset_x, pixel, format);
	ImageData<FloatPixel>* data = (ImageData<FloatPixel>*)(py_data->m_x);
	image = (Rect*)new ImageView<ImageData<FloatPixel> >(*data, offset_y, offset_x, nrows, ncols);
      } else if (pixel == RGB) {
	py_data = (ImageDataObject*)create_ImageDataObject(nrows, ncols, offset_y, offset_x, pixel, format);
	ImageData<RGBPixel>* data = (ImageData<RGBPixel>*)(py_data->m_x);
	image = (Rect*)new ImageView<ImageData<RGBPixel> >(*data, offset_y, offset_x, nrows, ncols);
      } else if (pixel == Gamera::COMPLEX) {
	py_data = (ImageDataObject*)create_ImageDataObject(nrows, ncols, offset_y, offset_x, pixel, format);
	ImageData<ComplexPixel>* data = (ImageData<ComplexPixel>*)(py_data->m_x);
	image = (Rect*)new ImageView<ImageData<ComplexPixel> >(*data, offset_y, offset_x, nrows, ncols);
      } else {
	PyErr_SetString(PyExc_TypeError, "Unknown Pixel type!");
	return false;
      }
    } else if (format == RLE) {
      if (pixel == ONEBIT) {
	py_data = (ImageDataObject*)create_ImageDataObject(nrows, ncols, offset_y, offset_x, pixel, format);
	RleImageData<OneBitPixel>* data = (RleImageData<OneBitPixel>*)(py_data->m_x);
	image = (Rect*)new ImageView<RleImageData<OneBitPixel> >(*data, offset_y, offset_x, nrows, ncols);
      } else {
	PyErr_SetString(PyExc_TypeError,
			"Pixel type must be Onebit for Rle data!");
	return false;
      }
    } else {
      PyErr_SetString(PyExc_TypeError, "Unknown Format!");
      return false;
    }
  } catch (std::exception& e) {
    PyErr_SetString(PyExc_RuntimeError, e.what());
    return false;
  }
  return true;
}

static PyObject* image_new(PyTypeObject* pytype, PyObject* args,
			   PyObject* kwds) {
  int num_args = PyTuple_GET_SIZE(args);
  Rect* image = NULL;
  ImageDataObject* py_data = NULL;
  if (num_args >= 4 && num_args <= 6) {
    int offset_y, offset_x, nrows, ncols, pixel, format;
    pixel = format = 0;
    static char *kwlist[] = {"offset_y", "offset_x", "nrows", "ncols", "pixel_type", "storage_format", NULL};
    if (PyArg_ParseTupleAndKeywords(args, kwds, "iiii|ii", kwlist, 
				    &offset_y, &offset_x, &nrows, &ncols, &pixel, &format) > 0) {
      if (!_image_new(offset_y, offset_x, nrows, ncols, pixel, format, py_data, image))
	return 0;
    }
  }
  if (image == NULL && num_args >= 2 && num_args <= 4) {
    PyObject *a, *b;
    int pixel, format;
    pixel = format = 0;
    static char *kwlist[] = {"a", "b", "pixel_type", "storage_format", NULL};
    if (PyArg_ParseTupleAndKeywords(args, kwds, "OO|ii", kwlist, 
				    &a, &b, &pixel, &format)) {
      if (is_PointObject(a)) {
	if (is_PointObject(b)) {
	  // We could have just delegated to the overloaded constructors of Rect here,
	  // but since ImageData is not overloaded in the same way, we have to do this
	  // manually anyway, so we just convert to the four integer parameters each time.
	  Point* point_a = ((PointObject*)a)->m_x;
	  Point* point_b = ((PointObject*)b)->m_x;
	  int nrows = point_b->y() - point_a->y() + 1;
	  int ncols = point_b->x() - point_a->x() + 1;
	  int offset_y = point_a->y();
	  int offset_x = point_a->x();
	  if (!_image_new(offset_y, offset_x, nrows, ncols, pixel, format, py_data, image))
	    return 0;
	} else if (is_SizeObject(b)) {
	  Point* point_a = ((PointObject*)a)->m_x;
	  Size* size_b = ((SizeObject*)b)->m_x;
	  int nrows = size_b->height() + 1;
	  int ncols = size_b->width() + 1;
	  int offset_y = point_a->y();
	  int offset_x = point_a->x();
	  if (!_image_new(offset_y, offset_x, nrows, ncols, pixel, format, py_data, image))
	    return 0;
	} else if (is_DimensionsObject(b)) {
	  Point* point_a = ((PointObject*)a)->m_x;
	  Dimensions* size_b = ((DimensionsObject*)b)->m_x;
	  int nrows = size_b->nrows();
	  int ncols = size_b->ncols();
	  int offset_y = point_a->y();
	  int offset_x = point_a->x();
	  if (!_image_new(offset_y, offset_x, nrows, ncols, pixel, format, py_data, image))
	    return 0;
	}
      }
    }
  }
      
  if (image == NULL && num_args >= 1 && num_args <= 3) {
    PyObject *src;
    int pixel, format;
    pixel = format = -1;
    static char *kwlist[] = {"image", "pixel_type", "storage_format", NULL};
    if (PyArg_ParseTupleAndKeywords(args, kwds, "O|ii", kwlist, 
				    &src, &pixel, &format)) {
      if (is_ImageObject(src)) {
	ImageObject* py_src = (ImageObject*)src;
	Rect* rect = ((RectObject*)src)->m_x;
	if (pixel == -1) {
	  pixel = ((ImageDataObject*)py_src->m_data)->m_pixel_type;
	}
	if (format == -1) {
	  format = ((ImageDataObject*)py_src->m_data)->m_storage_format;
	}
	if (!_image_new(rect->ul_y(), rect->ul_x(), rect->nrows(), rect->ncols(), pixel, format, py_data, image))
	  return 0;
      }
    }
  }

  if (image == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid arguments to image constructor.");
    return 0;
  }
      
  ImageObject* o;
  // we do not call rect_new here because we do all of the
  // required initializations
  o = (ImageObject*)pytype->tp_alloc(pytype, 0);
  // initialize the weakreflist
  o->m_weakreflist = NULL;
  o->m_data = (PyObject*)py_data;
  ((RectObject*)o)->m_x = image;
  return init_image_members(o);
}

static bool _sub_image_new(PyObject* py_src, int offset_y, int offset_x, int nrows, int ncols, 
			   Rect* &image) {
  if (!is_ImageObject(py_src)) {
    PyErr_SetString(PyExc_TypeError, "First argument must be an image!");
    return false;
  }
  int pixel, format;
  ImageObject* src = (ImageObject*)py_src;
  pixel = ((ImageDataObject*)src->m_data)->m_pixel_type;
  format = ((ImageDataObject*)src->m_data)->m_storage_format;

  try {
    if (format == DENSE) {
      if (pixel == ONEBIT) {
	ImageData<OneBitPixel>* data =
	  ((ImageData<OneBitPixel>*)((ImageDataObject*)src->m_data)->m_x);
	image =	(Rect*)new ImageView<ImageData<OneBitPixel> >(*data, offset_y, offset_x, nrows, ncols);
      } else if (pixel == GREYSCALE) {
	ImageData<GreyScalePixel>* data =
	  ((ImageData<GreyScalePixel>*)((ImageDataObject*)src->m_data)->m_x);
	image = (Rect*)new ImageView<ImageData<GreyScalePixel> >(*data, offset_y, offset_x, nrows, ncols);
      } else if (pixel == GREY16) {
	ImageData<Grey16Pixel>* data =
	  ((ImageData<Grey16Pixel>*)((ImageDataObject*)src->m_data)->m_x);
	image = (Rect*)new ImageView<ImageData<Grey16Pixel> >(*data, offset_y, offset_x, nrows, ncols);
      } else if (pixel == Gamera::FLOAT) {
	ImageData<FloatPixel>* data =
	  ((ImageData<FloatPixel>*)((ImageDataObject*)src->m_data)->m_x);
	image = (Rect*)new ImageView<ImageData<FloatPixel> >(*data, offset_y, offset_x, nrows, ncols);
      } else if (pixel == RGB) {
	ImageData<RGBPixel>* data =
	  ((ImageData<RGBPixel>*)((ImageDataObject*)src->m_data)->m_x);
	image = (Rect*)new ImageView<ImageData<RGBPixel> >(*data, offset_y, offset_x, nrows, ncols);
      } else if (pixel == Gamera::COMPLEX) {
	ImageData<ComplexPixel>* data =
	  ((ImageData<ComplexPixel>*)((ImageDataObject*)src->m_data)->m_x);
	image = (Rect*)new ImageView<ImageData<ComplexPixel> >(*data, offset_y, offset_x, nrows, ncols);
      } else {
	PyErr_SetString(PyExc_TypeError, "Unknown Pixel type!");
	return false;
      }
    } else if (format == RLE) {
      if (pixel == ONEBIT) {
	RleImageData<OneBitPixel>* data =
	  ((RleImageData<OneBitPixel>*)((ImageDataObject*)src->m_data)->m_x);
	image = (Rect *)new ImageView<RleImageData<OneBitPixel> >(*data, offset_y, offset_x, nrows, ncols);
      } else {
	PyErr_SetString(PyExc_TypeError,
			"Pixel type must be Onebit for Rle data!");
	return false;
      }
    } else {
      PyErr_SetString(PyExc_TypeError, "Unknown Format!");
      return false;
    }
  } catch (std::exception& e) {
    PyErr_SetString(PyExc_RuntimeError, e.what());
    return false;
  }
  return true;
}
			   

PyObject* sub_image_new(PyTypeObject* pytype, PyObject* args, PyObject* kwds) {
  int num_args = PyTuple_GET_SIZE(args);
  PyObject* image = NULL;
  Rect* subimage = NULL;
  if (num_args == 5) {
    int offset_y, offset_x, nrows, ncols;
    if (PyArg_ParseTuple(args, "Oiiii", 
			 &image, &offset_y, &offset_x, &nrows, &ncols) > 0) {
      if (!_sub_image_new(image, offset_y, offset_x, nrows, ncols, subimage))
	  return 0;
    }
  }
  if (subimage == NULL && num_args == 3) {
    PyObject *a, *b;
    if (PyArg_ParseTuple(args, "OOO", &image, &a, &b)) {
      if (is_PointObject(a)) {
	if (is_PointObject(b)) {
	  // We could have just delegated to the overloaded constructors of Rect here,
	  // but since ImageData is not overloaded in the same way, we have to do this
	  // manually anyway, so we just convert to the four integer parameters each time.
	  Point* point_a = ((PointObject*)a)->m_x;
	  Point* point_b = ((PointObject*)b)->m_x;
	  int nrows = point_b->y() - point_a->y() + 1;
	  int ncols = point_b->x() - point_a->x() + 1;
	  int offset_y = point_a->y();
	  int offset_x = point_a->x();
	  if (!_sub_image_new(image, offset_y, offset_x, nrows, ncols, subimage))
	    return 0;
	} else if (is_SizeObject(b)) {
	  Point* point_a = ((PointObject*)a)->m_x;
	  Size* size_b = ((SizeObject*)b)->m_x;
	  int nrows = size_b->height() + 1;
	  int ncols = size_b->width() + 1;
	  int offset_y = point_a->y();
	  int offset_x = point_a->x();
	  if (!_sub_image_new(image, offset_y, offset_x, nrows, ncols, subimage))
	    return 0;
	} else if (is_DimensionsObject(b)) {
	  Point* point_a = ((PointObject*)a)->m_x;
	  Dimensions* size_b = ((DimensionsObject*)b)->m_x;
	  int nrows = size_b->nrows();
	  int ncols = size_b->ncols();
	  int offset_y = point_a->y();
	  int offset_x = point_a->x();
	  if (!_sub_image_new(image, offset_y, offset_x, nrows, ncols, subimage))
	    return 0;
	}
      }
    }
  }
      
  if (subimage == NULL && num_args == 2) {
    PyObject* pyrect;
    if (PyArg_ParseTuple(args, "OO", &image, &pyrect)) {
      if (is_RectObject(pyrect)) {
	Rect* rect = ((RectObject*)pyrect)->m_x;
	if (!_sub_image_new(image, rect->ul_y(), rect->ul_x(), rect->nrows(), rect->ncols(), subimage))
	  return 0;
      }
    }
  }

  if (subimage == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid arguments to SubImage constructor.");
    return 0;
  }

  ImageObject* o;
  o = (ImageObject*)pytype->tp_alloc(pytype, 0);
  ((RectObject*)o)->m_x = subimage;
  o->m_data = ((ImageObject*)image)->m_data;
  Py_INCREF(o->m_data);
  ((Image*)((RectObject*)o)->m_x)->resolution(((Image*)((RectObject*)image)->m_x)->resolution());
  return init_image_members(o);
}

static bool _cc_new(PyObject* py_src, int label, int offset_y, int offset_x, int nrows, int ncols, Rect* &cc) {
  if (!is_ImageObject(py_src)) {
    PyErr_SetString(PyExc_TypeError, "First argument must be an image!");
    return false;
  }
  int pixel, format;
  ImageObject* src = (ImageObject*)py_src;
  pixel = ((ImageDataObject*)src->m_data)->m_pixel_type;
  format = ((ImageDataObject*)src->m_data)->m_storage_format;

  try {
    if (pixel != ONEBIT) {
      PyErr_SetString(PyExc_TypeError, "Image must be OneBit!");
      return false;
    }
    
    if (format == DENSE) {
      ImageData<OneBitPixel>* data =
	((ImageData<OneBitPixel>*)((ImageDataObject*)src->m_data)->m_x);
      cc = (Rect*)new ConnectedComponent<ImageData<OneBitPixel> >(*data, label, offset_y, offset_x, nrows, ncols);
    } else if (format == RLE) {
      RleImageData<OneBitPixel>* data =
	((RleImageData<OneBitPixel>*)((ImageDataObject*)src->m_data)->m_x);
      cc = (Rect*)new ConnectedComponent<RleImageData<OneBitPixel> >(*data, label, offset_y, offset_x, nrows, ncols);
    } else {
      PyErr_SetString(PyExc_TypeError, "Unknown Format!");
      return false;
    }
  } catch (std::exception& e) {
    PyErr_SetString(PyExc_RuntimeError, e.what());
    return false;
  }
  return true;
}

PyObject* cc_new(PyTypeObject* pytype, PyObject* args, PyObject* kwds) {
  int num_args = PyTuple_GET_SIZE(args);
  PyObject* image = NULL;
  Rect* cc = NULL;
  if (num_args == 6) {
    int offset_y, offset_x, nrows, ncols, label;
    if (PyArg_ParseTuple(args, "Oiiiii", 
			 &image, &label, &offset_y, &offset_x, &nrows, &ncols) > 0) {
      if (!_cc_new(image, label, offset_y, offset_x, nrows, ncols, cc))
	  return 0;
    }
  }
  if (cc == NULL && num_args == 4) {
    PyObject *a, *b;
    int label;
    if (PyArg_ParseTuple(args, "OiOO", &image, &label, &a, &b)) {
      if (is_PointObject(a)) {
	if (is_PointObject(b)) {
	  // We could have just delegated to the overloaded constructors of Rect here,
	  // but since ImageData is not overloaded in the same way, we have to do this
	  // manually anyway, so we just convert to the four integer parameters each time.
	  Point* point_a = ((PointObject*)a)->m_x;
	  Point* point_b = ((PointObject*)b)->m_x;
	  int nrows = point_b->y() - point_a->y() + 1;
	  int ncols = point_b->x() - point_a->x() + 1;
	  int offset_y = point_a->y();
	  int offset_x = point_a->x();
	  if (!_cc_new(image, label, offset_y, offset_x, nrows, ncols, cc))
	    return 0;
	} else if (is_SizeObject(b)) {
	  Point* point_a = ((PointObject*)a)->m_x;
	  Size* size_b = ((SizeObject*)b)->m_x;
	  int nrows = size_b->height() + 1;
	  int ncols = size_b->width() + 1;
	  int offset_y = point_a->y();
	  int offset_x = point_a->x();
	  if (!_cc_new(image, label, offset_y, offset_x, nrows, ncols, cc))
	    return 0;
	} else if (is_DimensionsObject(b)) {
	  Point* point_a = ((PointObject*)a)->m_x;
	  Dimensions* size_b = ((DimensionsObject*)b)->m_x;
	  int nrows = size_b->nrows();
	  int ncols = size_b->ncols();
	  int offset_y = point_a->y();
	  int offset_x = point_a->x();
	  if (!_cc_new(image, label, offset_y, offset_x, nrows, ncols, cc))
	    return 0;
	}
      }
    }
  }
      
  if (cc == NULL && num_args == 3) {
    int label;
    PyObject* pyrect;
    if (PyArg_ParseTuple(args, "OiO", &image, &label, &pyrect)) {
      if (is_RectObject(pyrect)) {
	Rect* rect = ((RectObject*)pyrect)->m_x;
	if (!_cc_new(image, label, rect->ul_y(), rect->ul_x(), rect->nrows(), rect->ncols(), cc))
	  return 0;
      }
    }
  }

  if (cc == NULL) {
    PyErr_SetString(PyExc_TypeError, "Invalid arguments to image constructor.");
    return 0;
  }

  ImageObject* o;
  o = (ImageObject*)pytype->tp_alloc(pytype, 0);
  ((RectObject*)o)->m_x = cc;
  o->m_data = ((ImageObject*)image)->m_data;
  Py_INCREF(o->m_data);
  // set the resolution
  ((Image*)((RectObject*)o)->m_x)->resolution(((Image*)((RectObject*)image)->m_x)->resolution());
  return init_image_members(o);
}

static void image_dealloc(PyObject* self) {
  ImageObject* o = (ImageObject*)self;

  if (o->m_weakreflist != NULL) {
    PyObject_ClearWeakRefs(self);
  }

  image_clear(self);
  
  Py_DECREF(o->m_data);
  Py_DECREF(o->m_features);
  Py_DECREF(o->m_classification_state);

  delete ((RectObject*)self)->m_x;

  self->ob_type->tp_free(self);
  //free(self);
}

static int image_traverse(PyObject* self, visitproc visit, void *arg) {
  ImageObject* o = (ImageObject*)self;
  if (o->m_id_name)
    if (visit(o->m_id_name, arg) < 0)
      return -1;
  if (o->m_children_images)
    if (visit(o->m_children_images, arg) < 0)
      return -1;
  return 0;
}

static int image_clear(PyObject* self) {
  ImageObject* o = (ImageObject*)self;
  Py_XDECREF(o->m_id_name);
  o->m_id_name = NULL;
  Py_XDECREF(o->m_children_images);
  o->m_children_images = NULL;

  return 0;
}

static PyObject* image_get(PyObject* self, int row, int col) {
  RectObject* o = (RectObject*)self;
  ImageDataObject* od = (ImageDataObject*)((ImageObject*)self)->m_data;
  if (is_CCObject(self)) {
    return PyInt_FromLong(((Cc*)o->m_x)->get((size_t)row, (size_t)col));
  } else if (od->m_storage_format == RLE) {
    return PyInt_FromLong(((OneBitRleImageView*)o->m_x)->get((size_t)row, (size_t)col));
  } else {
    switch (od->m_pixel_type) {
    case Gamera::FLOAT:
      return PyFloat_FromDouble(((FloatImageView*)o->m_x)->get((size_t)row, (size_t)col));
      break;
    case Gamera::RGB:
      return create_RGBPixelObject(((RGBImageView*)o->m_x)->get((size_t)row, (size_t)col));
      break;
    case Gamera::GREYSCALE:
      return PyInt_FromLong(((GreyScaleImageView*)o->m_x)->get((size_t)row, (size_t)col));
      break;
    case Gamera::GREY16:
      return PyInt_FromLong(((Grey16ImageView*)o->m_x)->get((size_t)row, (size_t)col));
      break;
    case Gamera::ONEBIT:
      return PyInt_FromLong(((OneBitImageView*)o->m_x)->get((size_t)row, (size_t)col));
      break;
    case Gamera::COMPLEX: {
      ComplexPixel temp = ((ComplexImageView*)o->m_x)->get((size_t)row, (size_t)col);
      return PyComplex_FromDoubles(temp.real(), temp.imag());
      break;
    } default:
      return 0;
    }
  }
}

static PyObject* image_set(PyObject* self, int row, int col, PyObject* value) {
  RectObject* o = (RectObject*)self;
  ImageDataObject* od = (ImageDataObject*)((ImageObject*)self)->m_data;
  if (is_CCObject(self)) {
    if (!PyInt_Check(value)) {
      PyErr_SetString(PyExc_TypeError, "Pixel value for CC objects must be an int.");
      return 0;
    }
    ((Cc*)o->m_x)->set((size_t)row, (size_t)col, (OneBitPixel)PyInt_AS_LONG(value));
  } else if (od->m_pixel_type == Gamera::FLOAT) {
    if (!PyFloat_Check(value)) {
      PyErr_SetString(PyExc_TypeError, "Pixel value for Float objects must be a float.");
      return 0;
    }
    ((FloatImageView*)o->m_x)->set((size_t)row, (size_t)col, PyFloat_AS_DOUBLE(value));
  } else if (od->m_storage_format == RLE) {
    if (!PyInt_Check(value)) {
      PyErr_SetString(PyExc_TypeError, "Pixel value for OneBit objects must be an int.");
      return 0;
    }
    ((OneBitRleImageView*)o->m_x)->set((size_t)row, (size_t)col,
				       (OneBitPixel)PyInt_AS_LONG(value));
  } else if (od->m_pixel_type == RGB) {
    if (!is_RGBPixelObject((PyObject*)value)) {
      PyErr_SetString(PyExc_TypeError, "Pixel value for OneBit objects must be an RGBPixel");
      return 0;
    }
    RGBPixelObject* v = (RGBPixelObject*)value;
    ((RGBImageView*)o->m_x)->set((size_t)row, (size_t)col, *v->m_x);
  } else if (od->m_pixel_type == GREYSCALE) {
    if (!PyInt_Check(value)) {
      PyErr_SetString(PyExc_TypeError, "Pixel value for GreyScale objects must be an int.");
      return 0;
    }
    ((GreyScaleImageView*)o->m_x)->set((size_t)row, (size_t)col,
				       (GreyScalePixel)PyInt_AS_LONG(value));
  } else if (od->m_pixel_type == GREY16) {
    if (!PyInt_Check(value)) {
      PyErr_SetString(PyExc_TypeError, "Pixel value for Grey16 objects must be an int.");
      return 0;
    }
    ((Grey16ImageView*)o->m_x)->set((size_t)row, (size_t)col,
				    (Grey16Pixel)PyInt_AS_LONG(value));
  } else if (od->m_pixel_type == ONEBIT) {
    if (!PyInt_Check(value)) {
      PyErr_SetString(PyExc_TypeError, "Pixel value for OneBit objects must be an int.");
      return 0;
    }
    ((OneBitImageView*)o->m_x)->set((size_t)row, (size_t)col,
				    (OneBitPixel)PyInt_AS_LONG(value));
  } else if (od->m_pixel_type == Gamera::COMPLEX) {
    if (!PyComplex_Check(value)) {
      PyErr_SetString(PyExc_TypeError, "Pixel value for Complex objects must be a complex number.");
      return 0;
    }
    ComplexPixel temp(PyComplex_RealAsDouble(value), PyComplex_ImagAsDouble(value));
    ((ComplexImageView*)o->m_x)->set((size_t)row, (size_t)col, temp);
  }
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject* image_get(PyObject* self, PyObject* args) {
  int num_args = PyTuple_GET_SIZE(args);
  Image* image = (Image*)((RectObject*)self)->m_x;
  int row, col, i;
  if (num_args == 2) {
    if (PyArg_ParseTuple(args, "ii", &row, &col) <= 0)
      return 0;
  } else if (num_args == 1) {
    if (PyArg_ParseTuple(args, "i", &i) <= 0)
      return 0;
    row = i / image->ncols();
    col = i % image->ncols();
  } else {
    PyErr_SetString(PyExc_TypeError, "Invalid arguments");
    return 0;
  }
  if (size_t(row) >= image->nrows() || size_t(col) >= image->ncols()) {
    PyErr_SetString(PyExc_IndexError, "Out of bounds for image");
    return 0;
  }
  return image_get(self, row, col);
}

static PyObject* image_set(PyObject* self, PyObject* args) {
  int num_args = PyTuple_GET_SIZE(args);
  Image* image = (Image*)((RectObject*)self)->m_x;
  int row, col, i;
  PyObject* value;
  if (num_args == 3) {
    if (PyArg_ParseTuple(args, "iiO", &row, &col, &value) <= 0)
      return 0;
  } else if (num_args == 2) {
    if (PyArg_ParseTuple(args, "iO", &i, &value) <= 0)
      return 0;
    row = i / image->ncols();
    col = i % image->ncols();
  } else {
    PyErr_SetString(PyExc_TypeError, "Invalid arguments");
    return 0;
  }  
  if (size_t(row) >= image->nrows() || size_t(col) >= image->ncols()) {
    PyErr_SetString(PyExc_IndexError, "Out of bounds for image");
    return 0;
  }
  return image_set(self, row, col, value);
}

// convert Python indexing into row/col format for images
// Removed, since getitem/setitem now take a tuple of coordinates
/*
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
  } */

static PyObject* image_getitem(PyObject* self, PyObject* args) {
  size_t row, col;
  PyObject* arg = PyTuple_GET_ITEM(args, 0);
  if (PyTuple_Check(arg)) {
    if (PyArg_ParseTuple(arg, "ii", &row, &col) <= 0)
      return 0;
  } else if (PyInt_Check(arg)) {
    size_t i;
    i = PyInt_AsLong(arg);
    Image* image = (Image*)((RectObject*)self)->m_x;
    row = i / image->ncols();
    col = i % image->ncols();
  } else if (is_PointObject(arg)) {
    Point* point = (Point*)((PointObject*)arg)->m_x;
    row = point->y();
    col = point->x();
  } else {
    PyErr_SetString(PyExc_TypeError, "Invalid arguments");
    return 0;
  }    
  return image_get(self, row, col);
}

static PyObject* image_setitem(PyObject* self, PyObject* args) {
  size_t row, col;
  PyObject* value;
  PyObject* arg;
  if (PyArg_ParseTuple(args, "OO", &arg, &value) <= 0) 
    return 0;
  if (PyTuple_Check(arg)) {
    if (PyArg_ParseTuple(arg, "ii", &row, &col) <= 0)
      return 0;
  } else if (PyInt_Check(arg)) {
    size_t i;
    i = PyInt_AsLong(arg);
    Image* image = (Image*)((RectObject*)self)->m_x;
    row = i / image->ncols();
    col = i % image->ncols();
  } else if (is_PointObject(arg)) {
    Point* point = (Point*)((PointObject*)arg)->m_x;
    row = point->y();
    col = point->x(); 
  } else {
    PyErr_SetString(PyExc_TypeError, "Invalid arguments");
    return 0;
  }    
  return image_set(self, row, col, value);
}

static PyObject* image_len(PyObject* self, PyObject* args) {
  Image* image = (Image*)((RectObject*)self)->m_x;
  return Py_BuildValue("i", (long)(image->nrows() * image->ncols()));
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

static PyObject* image_richcompare(PyObject* a, PyObject* b, int op) {
  if (!is_ImageObject(a) || !is_ImageObject(b)) {
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
  }

  Image& ap = *(Image*)((RectObject*)a)->m_x;
  Image& bp = *(Image*)((RectObject*)b)->m_x;

  /*
    Only equality and inequality make sense.
  */
  bool cmp;
  switch (op) {
  case Py_EQ:
    cmp = (ap == bp) && (ap.data() == bp.data());
    break;
  case Py_NE:
    cmp = (ap != bp) || (ap.data() != bp.data());
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

static PyObject* cc_richcompare(PyObject* a, PyObject* b, int op) {
  if (!is_ImageObject(a) || !is_ImageObject(b)) {
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
  }

  Image& ap = *(Image*)((RectObject*)a)->m_x;
  Image& bp = *(Image*)((RectObject*)b)->m_x;

  /*
    Only equality and inequality make sense.
  */
  bool cmp;
  switch (op) {
  case Py_EQ:
    if (!is_CCObject(a) || !is_CCObject(b))
      cmp = false;
    else {
      Cc& ac = *(Cc*)((RectObject*)a)->m_x;
      Cc& bc = *(Cc*)((RectObject*)b)->m_x;
      cmp = (ap == bp) && (ap.data() == bp.data()) && ac.label() == bc.label();
    }
    break;
  case Py_NE:
    if (!is_CCObject(a) || !is_CCObject(b))
      cmp = true;
    else {
      Cc& ac = *(Cc*)((RectObject*)a)->m_x;
      Cc& bc = *(Cc*)((RectObject*)b)->m_x;
      cmp = (ap != bp) || (ap.data() != bp.data()) || ac.label() != bc.label();
    }
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

void init_ImageType(PyObject* module_dict) {
  ImageType.ob_type = &PyType_Type;
  ImageType.tp_name = "gameracore.Image";
  ImageType.tp_basicsize = sizeof(ImageObject);
  ImageType.tp_dealloc = image_dealloc;
  ImageType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | 
    Py_TPFLAGS_HAVE_WEAKREFS | Py_TPFLAGS_HAVE_GC;
  ImageType.tp_base = get_RectType();
  ImageType.tp_getset = image_getset;
  ImageType.tp_methods = image_methods;
  ImageType.tp_new = image_new;
  ImageType.tp_getattro = PyObject_GenericGetAttr;
  ImageType.tp_alloc = NULL; // PyType_GenericAlloc;
  ImageType.tp_free = NULL; //_PyObject_Del;
  ImageType.tp_richcompare = image_richcompare;
  ImageType.tp_weaklistoffset = offsetof(ImageObject, m_weakreflist);
  ImageType.tp_traverse = image_traverse;
  ImageType.tp_clear = image_clear;
  ImageType.tp_doc = "The Image constructor creates a new image with newly allocated underlying data.\n\nThere are multiple ways to create an Image:\n\n  - **Image** (Int *offset_y*, Int *offset_x*, Int *nrows*, Int *ncols*, Choice *pixel_type* = ONEBIT, Choice *format* = DENSE)\n\n  - **Image** (Point *upper_left*, Point *lower_right*, Choice *pixel_type* = ONEBIT, Choice *format* = DENSE)\n\n  - **Image** (Point *upper_left*, Size *size*, Choice *pixel_type* = ONEBIT, Choice *format* = DENSE)\n\n  - **Image** (Point *upper_left*, Dimensions *dimensions*, Choice *pixel_type* = ONEBIT, Choice *format* = DENSE)\n\n  - **Image** (Rect *rectangle*, Choice *pixel_type* = ONEBIT, Choice *format* = DENSE)\n\n  - **Image** (Image *image*, Choice *pixel_type* = ONEBIT, Choice *format* = DENSE)\n\nNote that the last constructor creates a new image with the same position\nand dimensions as the passed in image, but does not copy the data.\n(For that use image_copy).\n\n*pixel_type*\n  An integer value specifying the type of the pixels in the image.\n  See `pixel types`__ for more information.\n\n.. __: image_types.html#pixel-types\n\n*storage_format*\n  An integer value specifying the method used to store the image data.\n  See `storage formats`__ for more information.\n\n.. __: image_types.html#storage-formats\n";
  PyType_Ready(&ImageType);
  PyDict_SetItemString(module_dict, "Image", (PyObject*)&ImageType);

  SubImageType.ob_type = &PyType_Type;
  SubImageType.tp_name = "gameracore.SubImage";
  SubImageType.tp_basicsize = sizeof(SubImageObject);
  SubImageType.tp_dealloc = image_dealloc;
  SubImageType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | 
    Py_TPFLAGS_HAVE_WEAKREFS | Py_TPFLAGS_HAVE_GC;
  SubImageType.tp_base = &ImageType;
  SubImageType.tp_new = sub_image_new;
  SubImageType.tp_getattro = PyObject_GenericGetAttr;
  SubImageType.tp_alloc = NULL; // PyType_GenericAlloc;
  SubImageType.tp_free = NULL; // _PyObject_Del;
  SubImageType.tp_doc = "Creates a new view on existing data.\n\nThere are a number of ways to create a subimage:\n\n  - **SubImage** (Image *image*, Int *offset_y*, Int *offset_x*, Int *nrows*, Int *ncols*)\n\n  - **SubImage** (Image *image*, Point *upper_left*, Point *lower_right*)\n\n  - **SubImage** (Image *image*, Point *upper_left*, Size *size*)\n\n  - **SubImage** (Image *image*, Point *upper_left*, Dimensions *dimensions*)\n\n  - **SubImage** (Image *image*, Rect *rectangle*)\n\nChanges to subimages will affect all other subimages viewing the same data.";
  PyType_Ready(&SubImageType);
  PyDict_SetItemString(module_dict, "SubImage", (PyObject*)&SubImageType);

  CCType.ob_type = &PyType_Type;
  CCType.tp_name = "gameracore.Cc";
  CCType.tp_basicsize = sizeof(CCObject);
  CCType.tp_dealloc = image_dealloc;
  CCType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | 
    Py_TPFLAGS_HAVE_WEAKREFS | Py_TPFLAGS_HAVE_GC;
  CCType.tp_base = &ImageType;
  CCType.tp_new = cc_new;
  CCType.tp_getset = cc_getset;
  CCType.tp_getattro = PyObject_GenericGetAttr;
  CCType.tp_alloc = PyType_GenericAlloc;
  CCType.tp_richcompare = cc_richcompare;
  CCType.tp_free = NULL; //_PyObject_Del;
  CCType.tp_doc = "Creates a connected component representing part of a OneBit image. It is rare to create one of these objects directly: most often you will just use cc_analysis to create connected components.\n\nThere are a number of ways to create a Cc:\n\n  - **Cc** (Image *image*, Int *label*, Int *offset_y*, Int *offset_x*, Int *nrows*, Int *ncols*)\n\n  - **Cc** (Image *image*, Int *label*, Point *upper_left*, Point *lower_right*)\n\n  - **Cc** (Image *image*, Int *label*, Point *upper_left*, Size *size*)\n\n  - **Cc** (Image *image*, Int *label*, Point *upper_left*, Dimensions *dimensions*)\n\n  - **Cc** (Image *image*, Int *label*, Rect *rectangle*)\n\n*label*\n  The pixel value used to represent this Cc.";
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

