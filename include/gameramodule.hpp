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

#ifndef KWM06292002_imagemodule
#define KWM06292002_imagemodule

#include "gamera.hpp"
#include <Python.h>

/*
  This file holds the C++ interface for the Python objects that wrap
  the core Gamera C++ objects. Each object has the struct for the Python
  object, a type-checking function, and a factory to create an instance
  of the Python type from the corresponding C++ type. Only the Python
  instance struct is exported here - the type struct is not exported, but
  is available via the object->tp_type field in the instance struct and
  through a function.
*/

namespace Gamera {
  namespace Python {
    /*
      Enumeration for all of the image types, pixel types, and storage
      types.
    */
    enum PixelTypes {
      ONEBIT,
      GREYSCALE,
      GREY16,
      RGB,
      FLOAT
    };
    
    enum StorageTypes {
      DENSE,
      RLE
    };

    /*
      To make the wrapping code a little easier these are all of the
      combinations of pixel and storage types. The order is so that
      the non-compressed views correspond to the PixelTypes.
    */
    enum ImageCombinations {
      ONEBITIMAGEVIEW,
      GREYSCALEIMAGEVIEW,
      GREY16IMAGEVIEW,
      RGBIMAGEVIEW,
      FLOATIMAGEVIEW,
      ONEBITRLEIMAGEVIEW,
      CC,
      RLECC
    };

    enum ClassificationStates {
      UNCLASSIFIED,
      AUTOMATIC,
      HEURISTIC,
      MANUAL
    };
  }
}

/*
  SIZE OBJECT
*/
struct SizeObject {
  PyObject_HEAD
  Size* m_x;
};

extern PyTypeObject* get_SizeType();
extern bool is_SizeObject(PyObject* x);
extern PyObject* create_SizeObject(const Size& d);

/*
  DIMENSIONS OBJECT
*/
struct DimensionsObject {
  PyObject_HEAD
  Dimensions* m_x;
};

extern PyTypeObject* get_DimensionsType();
extern bool is_DimensionsObject(PyObject* x);
extern PyObject* create_DimensionsObject(const Dimensions& d);

/*
  POINT OBJECT
*/
struct PointObject {
  PyObject_HEAD
  Point* m_x;
};

extern PyTypeObject* get_PointType();
extern bool is_PointObject(PyObject* x);
extern PyObject* create_PointObject(const Point& p);

/*
  RECT OBJECT
*/
struct RectObject {
  PyObject_HEAD
  Rect* m_x;
};

extern PyTypeObject* get_RectType();
bool is_RectObject(PyObject* x);
extern PyObject* create_RectObject(const Rect& r);

/*
  RGB Pixel OBJECT
*/

struct RGBPixelObject {
  PyObject_HEAD
  RGBPixel* m_x;
};

extern PyTypeObject* get_RGBPixelType();
bool is_RGBPixelObject(PyObject* x);
extern PyObject* create_RGBPixelObject(const RGBPixel& p);

/*
  REGION OBJECT
*/

struct RegionObject {
  PyObject_HEAD
  RectObject m_parent; // we inheric from Rect
};

extern PyTypeObject* get_RegionType();
bool is_RegionObject(PyObject* x);
extern PyObject* create_RegionObject(const Region& r);

/*
  REGION MAP OBJECT
*/

struct RegionMapObject {
  PyObject_HEAD
  RegionMap* m_x;
};

extern PyTypeObject* get_RegionMapType();
bool is_RegionMapObject(PyObject* x);
extern PyObject* create_RegionMapObject(const Region& r);

/*
  IMAGE DATA OBJECT
*/
struct ImageDataObject {
  PyObject_HEAD
  ImageDataBase* m_x;
  int m_pixel_type;
  int m_storage_format;
};

extern PyTypeObject* get_ImageDataType();
bool is_ImageDataObject(PyObject* x);
extern PyObject* create_ImageDataObject(int nrows, int ncols,
					int page_offset_y, int page_offset_x,
					int pixel_type, int storage_format);

/*
  IMAGE OBJECT
*/
struct ImageObject {
  RectObject m_parent; // we inherit from Rect
  PyObject* m_data; // an ImageDataObject for ref counting
  /*
    Classification related members
  */
  PyObject* m_features; // an array of doubles (Python array module)
  PyObject* m_id_name; // a list of strings for the classified ids
  PyObject* m_children_images; // list of images
  PyObject* m_classification_state; // how (or whether) an image is classified
  PyObject* m_scaling; // scaing value for the features
  PyObject* m_region_maps; // RegionMap object - see the object docs
  PyObject* m_region_map; // Current global region map
  PyObject* m_action_depth; // for limiting recursions for "actions"
};

extern PyTypeObject* get_ImageType();
bool is_ImageObject(PyObject* x);

/*
  Image type information and type checking utilities
*/

// get the storage format - no type checking is performed
inline int get_storage_format(PyObject* image) {
  return ((ImageDataObject*)((ImageObject*)image)->m_data)->m_storage_format;
}

// get the pixel type - no type checking is performed
inline int get_pixel_type(PyObject* image) {
  return ((ImageDataObject*)((ImageObject*)image)->m_data)->m_pixel_type;
}

// get the combination of pixel and image type
inline int get_image_combination(PyObject* image, PyTypeObject* cc_type) {
  int storage = get_storage_format(image);
  if (PyObject_TypeCheck(image, cc_type)) {
    if (storage == Gamera::Python::RLE)
      return Gamera::Python::RLECC;
    else if (storage == Gamera::Python::DENSE)
      return Gamera::Python::CC;
    else
      return -1;
  } else if (storage == Gamera::Python::RLE) {
    return Gamera::Python::ONEBITRLEIMAGEVIEW;
  } else if (storage == Gamera::Python::DENSE) {
    return get_pixel_type(image);
  } else {
    return -1;
  }
}

/*
  This initializes all of the non-image members of an Image class.
*/
inline PyObject* init_image_members(ImageObject* o) {
  /*
    Create the features array. This will load the array module
    (if required) and create an array object containing doubles.
  */
  static PyObject* array_func = 0;
  if (array_func == 0) {
    PyObject* array_module = PyImport_ImportModule("array");
    if (array_module == 0)
      return 0;
    PyObject* array_dict = PyModule_GetDict(array_module);
    if (array_dict == 0)
      return 0;
    array_func = PyDict_GetItemString(array_dict, "array");
    if (array_func == 0)
      return 0;
  }
  PyObject* arglist = Py_BuildValue("(s)", "d");
  o->m_features = PyEval_CallObject(array_func, arglist);
  Py_DECREF(arglist);
  if (o->m_features == 0)
    return 0;
  // id_name
  o->m_id_name = PyList_New(0);
  if (o->m_id_name == 0)
    return 0;
  // Children Images
  o->m_children_images = PyList_New(0);
  if (o->m_children_images == 0)
    return 0;
  // Classification state
  o->m_classification_state = Py_BuildValue("i", Python::UNCLASSIFIED);
  // Scaling
  o->m_scaling = Py_BuildValue("i", 1);
  return (PyObject*)o;
}

/*
  Create an ImageObject from a given ImageBase object. This
  requires using RTTI to determine the type of the object so
  that the pixel_type and storage format parameters can be filled
  in the ImageDataObject correctly. Additionally, the only way
  to determine whether this should be an ImageObject or a
  SubImage object is to see if the image completely covers the
  image data. Finally, because we want to create the subclasses in
  gamera.py of the objects defined here, we have to pass in those
  types to this function (the types are determined at module loading
  time).
*/

inline PyObject* create_ImageObject(Image* image, PyTypeObject* image_type,
				    PyTypeObject* subimage_type,
				    PyTypeObject* cc_type,
				    PyTypeObject* image_data) {
  int pixel_type;
  int storage_type;
  bool cc = false;
  if (dynamic_cast<GreyScaleImageView*>(image) != 0) {
    pixel_type = Gamera::Python::GREYSCALE;
    storage_type = Gamera::Python::DENSE;
  } else if (dynamic_cast<Grey16ImageView*>(image) != 0) {
    pixel_type = Gamera::Python::GREY16;
    storage_type = Gamera::Python::DENSE;
  } else if (dynamic_cast<FloatImageView*>(image) != 0) {
    pixel_type = Gamera::Python::FLOAT;
    storage_type = Gamera::Python::DENSE;
  } else if (dynamic_cast<RGBImageView*>(image) != 0) {
    pixel_type = Gamera::Python::RGB;
    storage_type = Gamera::Python::DENSE;
  } else if (dynamic_cast<OneBitImageView*>(image) != 0) {
    pixel_type = Gamera::Python::ONEBIT;
    storage_type = Gamera::Python::DENSE;
  } else if (dynamic_cast<OneBitRleImageView*>(image) != 0) {
    pixel_type = Gamera::Python::ONEBIT;
    storage_type = Gamera::Python::RLE;
  } else if (dynamic_cast<Cc*>(image) != 0) {
    pixel_type = Gamera::Python::ONEBIT;
    storage_type = Gamera::Python::DENSE;
    cc = true;
  } else if (dynamic_cast<RleCc*>(image) != 0) {
    pixel_type = Gamera::Python::ONEBIT;
    storage_type = Gamera::Python::RLE;
    cc = true;
  } else {
    PyErr_SetString(PyExc_TypeError, "Unknown type returned from plugin.");
    return 0;
  }
  ImageDataObject* d = (ImageDataObject*)image_data->tp_alloc(image_data, 0);
  d->m_pixel_type = pixel_type;
  d->m_storage_format = storage_type;
  d->m_x = image->data();
  ImageObject* i;
  if (cc)
    i = (ImageObject*)cc_type->tp_alloc(cc_type, 0);
  else if (image->nrows() < image->data()->nrows()
	   || image->ncols() < image->data()->ncols())
    i = (ImageObject*)subimage_type->tp_alloc(subimage_type, 0);
  else
    i = (ImageObject*)image_type->tp_alloc(image_type, 0);
  i->m_data = (PyObject*)d;
  ((RectObject*)i)->m_x = image;
  return init_image_members(i);
}

/*
  SUB IMAGE OBJECT

  The SubImage object is here simply to allow type checking and to provide
  a natural form of constructor overloading - otherwise it is identical
  to the ImageObject.
*/
struct SubImageObject {
  ImageObject m_parent;
};

extern PyTypeObject* get_SubImageType();
bool is_SubImageObject(PyObject* x);

/*
  CC TYPE

  ConnectedComponents are a special case of image - a separate class is
  used for clarity and type checking. Like the SubImageObject it is almost
  identical to an ImageObject.
*/
struct CCObject {
  ImageObject m_parent;
};

extern PyTypeObject* get_CCType();
bool is_CCObject(PyObject* x);


/*
  IMAGEINFO TYPE

  Holds information about an image - primarily used for opening images.
*/

struct ImageInfoObject {
  PyObject_HEAD
  ImageInfo* m_x;
};

extern PyTypeObject* get_ImageInfoType();
bool is_ImageInfoObject(PyObject* x);

#endif
