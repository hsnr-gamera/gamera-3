/*
 *
 * Copyright (C) 2001-2004 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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

#include <Python.h>
#include "gamera.hpp"

/*
  This file holds the C++ interface for the Python objects that wrap
  the core Gamera C++ objects. Each object has the struct for the Python
  object, a type-checking function, and a factory to create an instance
  of the Python type from the corresponding C++ type. Only the Python
  instance struct is exported here - the type struct is not exported, but
  is available via the object->tp_type field in the instance struct and
  through a function.

  Some of the functions here come in 2 versions - one is located in the
  CPP files for each of the types and is available within the gameracore
  module (i.e. in the CPP files that are linked into gameracore). The other
  is inlined so that it is available to all of the gamera plugins. The inline
  versions are less efficient because they have to lookup the types at
  runtime.
*/

/*
  Utilities
*/

/*
  Get the dictionary for a module by name. This does all of the error
  handling for you to get the dictionary for a module. Returns the module
  on success of NULL on failure with the error set.
*/
inline PyObject* get_module_dict(char* module_name) {
  PyObject* mod = PyImport_ImportModule(module_name);
  if (mod == 0)
    return PyErr_Format(PyExc_RuntimeError, "Unable to load %s.\n", module_name);
  PyObject* dict = PyModule_GetDict(mod);
  if (dict == 0)
    return PyErr_Format(PyExc_RuntimeError,
			"Unable to get dict for module %s.\n", module_name);
  Py_DECREF(mod);
  return dict;
}

/*
  Get the dictionary for gameracore. This uses get_module_dict above, but caches
  the result for faster lookups in subsequent calls.
*/
inline PyObject* get_gameracore_dict() {
  static PyObject* dict = 0;
  if (dict == 0) {
    dict = get_module_dict("gamera.gameracore");
    if (dict == 0)
      return 0;
  }
  return dict;
}

#ifndef GAMERACORE_INTERNAL
inline PyObject* get_ArrayInit() {
  static PyObject* t = 0;
  if (t == 0) {
    PyObject* array_module = PyImport_ImportModule("array");
    if (array_module == 0) {
      PyErr_SetString(PyExc_RuntimeError,
		      "Unable to get array module.\n");
      return 0;
    }
    PyObject* array_dict = PyModule_GetDict(array_module);
    if (array_dict == 0) {
      PyErr_SetString(PyExc_RuntimeError,
		      "Unable to get array module dictionary.\n");
      return 0;
    }
    t = PyDict_GetItemString(array_dict, "array");
    if (t == 0) {
      PyErr_SetString(PyExc_RuntimeError,
		      "Unable to get array object.\n");
      return 0;
    }
    Py_DECREF(array_module);
  }
  return t;
}

inline PyObject* get_ArrayAppend() {
  static PyObject* t = 0;
  if (t == 0) {
    PyObject* array_init = get_ArrayInit();
    if (array_init == 0)
      return 0;
    t = PyObject_GetAttrString(array_init, "append");
    if (t == 0) {
      PyErr_SetString(PyExc_RuntimeError,
		      "Unable to get array append method.\n");
      return 0;
    }
  }
  return t;
}
#endif

/*
  SIZE OBJECT
*/
struct SizeObject {
  PyObject_HEAD
  Size* m_x;
};

#ifndef GAMERACORE_INTERNAL
inline PyTypeObject* get_SizeType() {
  static PyTypeObject* t = 0;
  if (t == 0) {
    PyObject* dict = get_gameracore_dict();
    if (dict == 0)
      return 0;
    t = (PyTypeObject*)PyDict_GetItemString(dict, "Size");
    if (t == 0) {
      PyErr_SetString(PyExc_RuntimeError,
		      "Unable to get Size type for gamera.gameracore.\n");
      return 0;
    }
  }
  return t;
}
#else
extern PyTypeObject* get_SizeType();
#endif

inline bool is_SizeObject(PyObject* x) {
  PyTypeObject* t = get_SizeType();
  if (t == 0)
    return 0;
  if (PyObject_TypeCheck(x, t))
    return true;
  else
    return false;
}

inline PyObject* create_SizeObject(const Size& d) {
  PyTypeObject* t = get_SizeType();
  if (t == 0)
    return 0;
  SizeObject* so;
  so = (SizeObject*)t->tp_alloc(t, 0);
  so->m_x = new Size(d);
  return (PyObject*)so;
}

/*
  DIMENSIONS OBJECT
*/
struct DimensionsObject {
  PyObject_HEAD
  Dimensions* m_x;
};

#ifndef GAMERACORE_INTERNAL
inline PyTypeObject* get_DimensionsType() {
  static PyTypeObject* t = 0;
  if (t == 0) {
    PyObject* dict = get_gameracore_dict();
    if (dict == 0)
      return 0;
    t = (PyTypeObject*)PyDict_GetItemString(dict, "Dimensions");
    if (t == 0) {
      PyErr_SetString(PyExc_RuntimeError,
		      "Unable to get Dimensions type for gamera.gameracore.\n");
      return 0;
    }
  }
  return t;
}
#else
extern PyTypeObject* get_DimensionsType();
#endif

inline bool is_DimensionsObject(PyObject* x) {
  PyTypeObject* t = get_DimensionsType();
  if (t == 0)
    return 0;
  if (PyObject_TypeCheck(x, t))
    return true;
  else
    return false;
}

inline PyObject* create_DimensionsObject(const Dimensions& d) {
  PyTypeObject* t = get_DimensionsType();
  if (t == 0)
    return 0;
  DimensionsObject* so;
  so = (DimensionsObject*)t->tp_alloc(t, 0);
  so->m_x = new Dimensions(d);
  return (PyObject*)so;
}

/*
  POINT OBJECT
*/
struct PointObject {
  PyObject_HEAD
  Point* m_x;
};

#ifndef GAMERACORE_INTERNAL
inline PyTypeObject* get_PointType() {
  static PyTypeObject* t = 0;
  if (t == 0) {
    PyObject* dict = get_gameracore_dict();
    if (dict == 0)
      return 0;
    t = (PyTypeObject*)PyDict_GetItemString(dict, "Point");
    if (t == 0) {
      PyErr_SetString(PyExc_RuntimeError,
		      "Unable to get Point type for gamera.gameracore.\n");
      return 0;
    }
  }
  return t;
}
#else
extern PyTypeObject* get_PointType();
#endif

inline bool is_PointObject(PyObject* x) {
  PyTypeObject* t = get_PointType();
  if (t == 0)
    return 0;
  if (PyObject_TypeCheck(x, t))
    return true;
  else
    return false;
}

inline PyObject* create_PointObject(const Point& d) {
  PyTypeObject* t = get_PointType();
  if (t == 0)
    return 0;
  PointObject* so;
  so = (PointObject*)t->tp_alloc(t, 0);
  so->m_x = new Point(d);
  return (PyObject*)so;
}

/*
  RECT OBJECT
*/
struct RectObject {
  PyObject_HEAD
  Rect* m_x;
};

#ifndef GAMERACORE_INTERNAL
inline PyTypeObject* get_RectType() {
  static PyTypeObject* t = 0;
  if (t == 0) {
    PyObject* dict = get_gameracore_dict();
    if (dict == 0)
      return 0;
    t = (PyTypeObject*)PyDict_GetItemString(dict, "Rect");
    if (t == 0) {
      PyErr_SetString(PyExc_RuntimeError,
		      "Unable to get Rect type for gamera.gameracore.\n");
      return 0;
    }
  }
  return t;
}
#else
extern PyTypeObject* get_RectType();
#endif

inline bool is_RectObject(PyObject* x) {
  PyTypeObject* t = get_RectType();
  if (t == 0)
    return 0;
  if (PyObject_TypeCheck(x, t))
    return true;
  else
    return false;
}

inline PyObject* create_RectObject(const Rect& d) {
  PyTypeObject* t = get_RectType();
  if (t == 0)
    return 0;
  RectObject* so;
  so = (RectObject*)t->tp_alloc(t, 0);
  so->m_x = new Rect(d);
  return (PyObject*)so;
}

/*
  RGB Pixel OBJECT
*/

struct RGBPixelObject {
  PyObject_HEAD
  RGBPixel* m_x;
};

#ifndef GAMERACORE_INTERNAL
inline PyTypeObject* get_RGBPixelType() {
  static PyTypeObject* t = 0;
  if (t == 0) {
    PyObject* dict = get_gameracore_dict();
    if (dict == 0)
      return 0;
    t = (PyTypeObject*)PyDict_GetItemString(dict, "RGBPixel");
    if (t == 0) {
      PyErr_SetString(PyExc_RuntimeError,
		      "Unable to get RGBPixel type for gamera.gameracore.\n");
      return 0;
    }
  }
  return t;
}
#else
extern PyTypeObject* get_RGBPixelType();
#endif

inline bool is_RGBPixelObject(PyObject* x) {
  PyTypeObject* t = get_RGBPixelType();
  if (t == 0)
    return 0;
  if (PyObject_TypeCheck(x, t))
    return true;
  else
    return false;
}

inline PyObject* create_RGBPixelObject(const RGBPixel& d) {
  PyTypeObject* t = get_RGBPixelType();
  if (t == 0)
    return 0;
  RGBPixelObject* so;
  so = (RGBPixelObject*)t->tp_alloc(t, 0);
  so->m_x = new RGBPixel(d);
  return (PyObject*)so;
}

/*
  REGION OBJECT
*/

struct RegionObject {
  PyObject_HEAD
  RectObject m_parent; // we inheric from Rect
};

#ifndef GAMERACORE_INTERNAL
inline PyTypeObject* get_RegionType() {
  static PyTypeObject* t = 0;
  if (t == 0) {
    PyObject* dict = get_gameracore_dict();
    if (dict == 0)
      return 0;
    t = (PyTypeObject*)PyDict_GetItemString(dict, "Region");
    if (t == 0) {
      PyErr_SetString(PyExc_RuntimeError,
		      "Unable to get Region type for gamera.gameracore.\n");
      return 0;
    }
  }
  return t;
}
#else
extern PyTypeObject* get_RegionType();
#endif

inline bool is_RegionObject(PyObject* x) {
  PyTypeObject* t = get_RegionType();
  if (t == 0)
    return 0;
  if (PyObject_TypeCheck(x, t))
    return true;
  else
    return false;
}

inline PyObject* create_RegionObject(const Region& d) {
  PyTypeObject* t = get_RegionType();
  if (t == 0)
    return 0;
  RegionObject* so;
  so = (RegionObject*)t->tp_alloc(t, 0);
  ((RectObject*)so)->m_x = new Region(d);
  return (PyObject*)so;
}

/*
  REGION MAP OBJECT
*/

struct RegionMapObject {
  PyObject_HEAD
  RegionMap* m_x;
};

#ifndef GAMERACORE_INTERNAL
inline PyTypeObject* get_RegionMapType() {
  static PyTypeObject* t = 0;
  if (t == 0) {
    PyObject* dict = get_gameracore_dict();
    if (dict == 0)
      return 0;
    t = (PyTypeObject*)PyDict_GetItemString(dict, "RegionMap");
    if (t == 0) {
      PyErr_SetString(PyExc_RuntimeError,
		      "Unable to get RegionMap type for gamera.gameracore.\n");
      return 0;
    }
  }
  return t;
}
#else
extern PyTypeObject* get_RegionMapType();
#endif

inline bool is_RegionMapObject(PyObject* x) {
  PyTypeObject* t = get_RegionMapType();
  if (t == 0)
    return 0;
  if (PyObject_TypeCheck(x, t))
    return true;
  else
    return false;
}

inline PyObject* create_RegionMapObject(const RegionMap& d) {
  PyTypeObject* t = get_RegionMapType();
  if (t == 0)
    return 0;
  RegionMapObject* so;
  so = (RegionMapObject*)t->tp_alloc(t, 0);
  so->m_x = new RegionMap(d);
  return (PyObject*)so;
}

/*
  IMAGE DATA OBJECT
*/
struct ImageDataObject {
  PyObject_HEAD
  ImageDataBase* m_x;
  int m_pixel_type;
  int m_storage_format;
};

#ifndef GAMERACORE_INTERNAL
inline PyTypeObject* get_ImageDataType() {
  static PyTypeObject* t = 0;
  if (t == 0) {
    PyObject* dict = get_gameracore_dict();
    if (dict == 0)
      return 0;
    t = (PyTypeObject*)PyDict_GetItemString(dict, "ImageData");
    if (t == 0) {
      PyErr_SetString(PyExc_RuntimeError,
		      "Unable to get ImageData type for gamera.gameracore.\n");
      return 0;
    }
  }
  return t;
}
#else
extern PyTypeObject* get_ImageDataType();
#endif

inline bool is_ImageDataObject(PyObject* x) {
  PyTypeObject* t = get_ImageDataType();
  if (t == 0)
    return 0;
  if (PyObject_TypeCheck(x, t))
    return true;
  else
    return false;
}

inline PyObject* create_ImageDataObject(int nrows, int ncols,
					int page_offset_y, int page_offset_x,
					int pixel_type, int storage_format) {

  ImageDataObject* o;
  PyTypeObject* id_type = get_ImageDataType();
  if (id_type == 0)
    return 0;
  o = (ImageDataObject*)id_type->tp_alloc(id_type, 0);
  o->m_pixel_type = pixel_type;
  o->m_storage_format = storage_format;
  if (storage_format == DENSE) {
    if (pixel_type == ONEBIT)
      o->m_x = new ImageData<OneBitPixel>(nrows, ncols, page_offset_y,
					  page_offset_x);
    else if (pixel_type == GREYSCALE)
      o->m_x = new ImageData<GreyScalePixel>(nrows, ncols, page_offset_y,
					     page_offset_x);      
    else if (pixel_type == GREY16)
      o->m_x = new ImageData<Grey16Pixel>(nrows, ncols, page_offset_y,
					  page_offset_x);      
    // We have to explicity declare which FLOAT we want here, since there
    // is a name clash on Mingw32 with a typedef in windef.h
    else if (pixel_type == Gamera::FLOAT)
      o->m_x = new ImageData<FloatPixel>(nrows, ncols, page_offset_y,
					 page_offset_x);      
    else if (pixel_type == RGB)
      o->m_x = new ImageData<RGBPixel>(nrows, ncols, page_offset_y,
				       page_offset_x);      
    else {
      PyErr_SetString(PyExc_TypeError, "Unkown Pixel type!");
      return 0;
    }
  } else if (storage_format == RLE) {
    if (pixel_type == ONEBIT)
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
  o->m_x->m_user_data = (void*)o;
  return (PyObject*)o;
}

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
//   PyObject* m_region_maps; // RegionMap object - see the object docs
//   PyObject* m_region_map; // Current global region map
//   PyObject* m_action_depth; // for limiting recursions for "actions"
  PyObject* m_weakreflist; // for Python weak references
};

#ifndef GAMERACORE_INTERNAL
inline PyTypeObject* get_ImageType() {
  static PyTypeObject* t = 0;
  if (t == 0) {
    PyObject* dict = get_gameracore_dict();
    if (dict == 0)
      return 0;
    t = (PyTypeObject*)PyDict_GetItemString(dict, "Image");
    if (t == 0) {
      PyErr_SetString(PyExc_RuntimeError,
		      "Unable to get Image type for gamera.gameracore.\n");
      return 0;
    }
  }
  return t;
}
#else
extern PyTypeObject* get_ImageType();
#endif

inline bool is_ImageObject(PyObject* x) {
  PyTypeObject* t = get_ImageType();
  if (t == 0)
    return 0;
  if (PyObject_TypeCheck(x, t))
    return true;
  else
    return false;
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

#ifndef GAMERACORE_INTERNAL
inline PyTypeObject* get_SubImageType() {
  static PyTypeObject* t = 0;
  if (t == 0) {
    PyObject* dict = get_gameracore_dict();
    if (dict == 0)
      return 0;
    t = (PyTypeObject*)PyDict_GetItemString(dict, "SubImage");
    if (t == 0) {
      PyErr_SetString(PyExc_RuntimeError,
		      "Unable to get SubImage type for gamera.gameracore.\n");
      return 0;
    }
  }
  return t;
}
#else
extern PyTypeObject* get_SubImageType();
#endif

inline bool is_SubImageObject(PyObject* x) {
  PyTypeObject* t = get_SubImageType();
  if (t == 0)
    return 0;
  if (PyObject_TypeCheck(x, t))
    return true;
  else
    return false;
}

/*
  CC TYPE

  ConnectedComponents are a special case of image - a separate class is
  used for clarity and type checking. Like the SubImageObject it is almost
  identical to an ImageObject.
*/
struct CCObject {
  ImageObject m_parent;
};

#ifndef GAMERACORE_INTERNAL
inline PyTypeObject* get_CCType() {
  static PyTypeObject* t = 0;
  if (t == 0) {
    PyObject* dict = get_gameracore_dict();
    if (dict == 0)
      return 0;
    t = (PyTypeObject*)PyDict_GetItemString(dict, "Cc");
    if (t == 0) {
      PyErr_SetString(PyExc_RuntimeError,
		      "Unable to get CC type for gamera.gameracore.\n");
      return 0;
    }
  }
  return t;
}
#else
extern PyTypeObject* get_CCType();
#endif

inline bool is_CCObject(PyObject* x) {
  PyTypeObject* t = get_CCType();
  if (t == 0)
    return 0;
  if (PyObject_TypeCheck(x, t))
    return true;
  else
    return false;
}

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
inline int get_image_combination(PyObject* image) {
  int storage = get_storage_format(image);
  if (is_CCObject(image)) {
    if (storage == Gamera::RLE)
      return Gamera::RLECC;
    else if (storage == Gamera::DENSE)
      return Gamera::CC;
    else
      return -1;
  } else if (storage == Gamera::RLE) {
    return Gamera::ONEBITRLEIMAGEVIEW;
  } else if (storage == Gamera::DENSE) {
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
    Py_DECREF(array_module);
  }
  PyObject* arglist = Py_BuildValue("(s)", "d");
  o->m_features = PyObject_CallObject(array_func, arglist);
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
  // o->m_classification_state = Py_BuildValue("i", UNCLASSIFIED);
  o->m_classification_state = PyInt_FromLong(UNCLASSIFIED);
  if (o->m_classification_state == 0)
    return 0;
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

inline PyObject* create_ImageObject(Image* image) {
  static bool initialized = false;
  static PyTypeObject *image_type, *subimage_type, *cc_type,
    *image_data;
  static PyObject* pybase_init;
  if (!initialized) {
    PyObject* dict = get_module_dict("gamera.core");
    if (dict == 0)
      return 0;
    pybase_init = PyObject_GetAttrString(PyDict_GetItemString(dict, "ImageBase"),
					 "__init__");
    image_type = (PyTypeObject*)PyDict_GetItemString(dict, "Image");
    subimage_type = (PyTypeObject*)PyDict_GetItemString(dict, "SubImage");
    cc_type = (PyTypeObject*)PyDict_GetItemString(dict, "Cc");
    image_data = (PyTypeObject*)PyDict_GetItemString(dict, "ImageData");
    initialized = true;
  }
  
  int pixel_type;
  int storage_type;
  bool cc = false;
  if (dynamic_cast<GreyScaleImageView*>(image) != 0) {
    pixel_type = Gamera::GREYSCALE;
    storage_type = Gamera::DENSE;
  } else if (dynamic_cast<Grey16ImageView*>(image) != 0) {
    pixel_type = Gamera::GREY16;
    storage_type = Gamera::DENSE;
  } else if (dynamic_cast<FloatImageView*>(image) != 0) {
    pixel_type = Gamera::FLOAT;
    storage_type = Gamera::DENSE;
  } else if (dynamic_cast<RGBImageView*>(image) != 0) {
    pixel_type = Gamera::RGB;
    storage_type = Gamera::DENSE;
  } else if (dynamic_cast<OneBitImageView*>(image) != 0) {
    pixel_type = Gamera::ONEBIT;
    storage_type = Gamera::DENSE;
  } else if (dynamic_cast<OneBitRleImageView*>(image) != 0) {
    pixel_type = Gamera::ONEBIT;
    storage_type = Gamera::RLE;
  } else if (dynamic_cast<Cc*>(image) != 0) {
    pixel_type = Gamera::ONEBIT;
    storage_type = Gamera::DENSE;
    cc = true;
  } else if (dynamic_cast<RleCc*>(image) != 0) {
    pixel_type = Gamera::ONEBIT;
    storage_type = Gamera::RLE;
    cc = true;
  } else {
    PyErr_SetString(PyExc_TypeError, "Unknown type returned from plugin.");
    return 0;
  }
  ImageDataObject* d;
  if (image->data()->m_user_data == 0) {
    d = (ImageDataObject*)image_data->tp_alloc(image_data, 0);
    d->m_pixel_type = pixel_type;
    d->m_storage_format = storage_type;
    d->m_x = image->data();
    image->data()->m_user_data = (void*)d;
  } else {
    d = (ImageDataObject*)image->data()->m_user_data;
    Py_INCREF(d);
  }
  ImageObject* i;
  if (cc) {
    i = (ImageObject*)cc_type->tp_alloc(cc_type, 0);
  } else if (image->nrows() < image->data()->nrows()
	     || image->ncols() < image->data()->ncols()) {
    i = (ImageObject*)subimage_type->tp_alloc(subimage_type, 0);
  } else {
    i = (ImageObject*)image_type->tp_alloc(image_type, 0);
  }
  i->m_data = (PyObject*)d;
  ((RectObject*)i)->m_x = image;
  PyObject* args = Py_BuildValue("(O)", (PyObject*)i);
  PyObject* result = PyObject_CallObject(pybase_init, args);
  Py_DECREF(args);
  if (result == 0)
    return 0;
  Py_DECREF(result);
  return init_image_members(i);
}

/*
  IMAGEINFO TYPE

  Holds information about an image - primarily used for opening images.
*/

struct ImageInfoObject {
  PyObject_HEAD
  ImageInfo* m_x;
};

#ifndef GAMERACORE_INTERNAL
inline PyTypeObject* get_ImageInfoType() {
static PyTypeObject* t = 0;
  if (t == 0) {
    PyObject* dict = get_gameracore_dict();
    if (dict == 0)
      return 0;
    t = (PyTypeObject*)PyDict_GetItemString(dict, "ImageInfo");
    if (t == 0) {
      PyErr_SetString(PyExc_RuntimeError,
		      "Unable to get ImageInfo type for gamera.gameracore.\n");
      return 0;
    }
  }
  return t;
}
#else
extern PyTypeObject* get_ImageInfoType();
#endif

inline bool is_ImageInfoObject(PyObject* x) {
  PyTypeObject* t = get_ImageInfoType();
  if (t == 0)
    return 0;
  if (PyObject_TypeCheck(x, t))
    return true;
  else
  return false;
}

inline PyObject* create_ImageInfoObject(ImageInfo* x) {
  PyTypeObject* info_type = get_ImageInfoType();
  if (info_type == 0)
    return 0;
  ImageInfoObject* o;
  o = (ImageInfoObject*)info_type->tp_alloc(info_type, 0);
  o->m_x = x;
  return (PyObject*)o;
}

#ifndef GAMERACORE_INTERNAL
inline PyObject* FloatVector_to_python(FloatVector* cpp) {
  PyObject* array_init = get_ArrayInit();
  if (array_init == 0)
    return 0;
  PyObject* str = PyString_FromStringAndSize((char*)(&((*cpp)[0])),
        cpp->size() * sizeof(double));
  PyObject* py = PyObject_CallFunction(array_init, "sO", "d", str);
  Py_DECREF(str);
  return py;
}

inline PyObject* IntVector_to_python(IntVector* cpp) {
  PyObject* array_init = get_ArrayInit();
  if (array_init == 0)
    return 0;
  PyObject* str = PyString_FromStringAndSize((char*)(&((*cpp)[0])),
        cpp->size() * sizeof(int));
  PyObject* py = PyObject_CallFunction(array_init, "sO", "i", str);
  Py_DECREF(str);
  return py;
}

inline FloatVector* FloatVector_from_python(PyObject* py) {
  int size = PyObject_Size(py);
  if (size < 0) {
      PyErr_SetString(PyExc_TypeError,
		      "Argument is not a sequence.\n");
      return 0;
  }
  FloatVector* cpp = new FloatVector(size);
  for (int i = 0; i < size; ++i)
    (*cpp)[i] = (double)PyFloat_AsDouble(PyObject_GetItem(py, PyInt_FromLong(i)));
  return cpp;
}

inline IntVector* IntVector_from_python(PyObject* py) {
  int size = PyObject_Size(py);
  if (size < 0) {
      PyErr_SetString(PyExc_TypeError,
		      "Argument is not a sequence.\n");
      return 0;
  }
  IntVector* cpp = new IntVector(size);
  for (int i = 0; i < size; ++i)
    (*cpp)[i] = (int)PyInt_AsLong(PyObject_GetItem(py, PyInt_FromLong(i)));
  return cpp;
}

// Converting pixel types to/from Python

template<class T>
struct pixel_to_python {
  inline PyObject* operator()(T obj);
};

inline PyObject* pixel_to_python<OneBitPixel>::operator()(OneBitPixel px) {
  return PyInt_FromLong(px);
}

inline PyObject* pixel_to_python<GreyScalePixel>::operator()(GreyScalePixel px) {
  return PyInt_FromLong(px);
}

inline PyObject* pixel_to_python<Grey16Pixel>::operator()(Grey16Pixel px) {
  return PyInt_FromLong(px);
}

inline PyObject* pixel_to_python<RGBPixel>::operator()(RGBPixel px) {
  return create_RGBPixelObject(px);
}

inline PyObject* pixel_to_python<FloatPixel>::operator()(FloatPixel px) {
  return PyFloat_FromDouble(px);
}

template<class T>
struct pixel_from_python {
  inline T operator()(PyObject* obj);
};

template<class T>
inline T pixel_from_python<T>::operator()(PyObject* obj) {
  if (!PyFloat_Check(obj)) {
    if (!PyInt_Check(obj))
      throw std::runtime_error("Pixel value is not valid");
    return (T)PyInt_AsLong(obj);
  }
  return (T)PyFloat_AsDouble(obj);
}

inline RGBPixel pixel_from_python<RGBPixel>::operator()(PyObject* obj) {
  if (!is_RGBPixelObject(obj))
    throw std::runtime_error("Pixel value is not an RGBPixel");
  return RGBPixel(*(((RGBPixelObject*)obj)->m_x));
}

#endif

#endif
