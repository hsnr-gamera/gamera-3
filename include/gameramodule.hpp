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
    return PyErr_Format(PyExc_ImportError, "Unable to load module '%s'.\n", module_name);
  PyObject* dict = PyModule_GetDict(mod);
  if (dict == 0)
    return PyErr_Format(PyExc_RuntimeError,
			"Unable to get dict for module '%s'.\n",
			module_name);
  Py_DECREF(mod);
  return dict;
}

/* 
  Sends a DeprecationWarning
*/
inline int send_deprecation_warning(char* message, char* filename, int lineno) {
  static PyObject* dict = 0;
  if (dict == 0)
    dict = get_module_dict("gamera.util");
  static PyObject* py_warning_func = 0;
  if (py_warning_func == 0)
    py_warning_func = PyDict_GetItemString(dict, "warn_deprecated");
  PyObject* result = PyObject_CallFunction(py_warning_func, "ssii", message, filename, lineno, 1);
  if (result == 0)
    return 0;
  Py_DECREF(result);
  return 1;
}

/*
  Get the dictionary for gameracore. This uses get_module_dict above, but caches
  the result for faster lookups in subsequent calls.
*/
inline PyObject* get_gameracore_dict() {
  static PyObject* dict = 0;
  if (dict == 0)
    dict = get_module_dict("gamera.gameracore");
  return dict;
}

#ifndef GAMERACORE_INTERNAL
inline PyObject* get_ArrayInit() {
  static PyObject* t = 0;
  if (t == 0) {
    PyObject* array_module = PyImport_ImportModule("array");
    if (array_module == 0) {
      PyErr_SetString(PyExc_ImportError,
		      "Unable to get 'array' module.\n");
      return 0;
    }
    PyObject* array_dict = PyModule_GetDict(array_module);
    if (array_dict == 0) {
      PyErr_SetString(PyExc_RuntimeError,
		      "Unable to get 'array' module dictionary.\n");
      return 0;
    }
    t = PyDict_GetItemString(array_dict, "array");
    if (t == 0) {
      PyErr_SetString(PyExc_RuntimeError,
		      "Unable to get 'array' object.\n");
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
		      "Unable to get 'array' append method.\n");
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
		      "Unable to get Size type from gamera.gameracore.\n");
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
  return PyObject_TypeCheck(x, t);
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
#if GAMERA_INCLUDE_DEPRECATED
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
		      "Unable to get Dimensions type from gamera.gameracore.\n");
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
  return PyObject_TypeCheck(x, t);
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
#endif // GAMERA_INCLUDE_DEPRECATED

/*
  DIM OBJECT
*/
struct DimObject {
  PyObject_HEAD
  Dim* m_x;
};

#ifndef GAMERACORE_INTERNAL
inline PyTypeObject* get_DimType() {
  static PyTypeObject* t = 0;
  if (t == 0) {
    PyObject* dict = get_gameracore_dict();
    if (dict == 0)
      return 0;
    t = (PyTypeObject*)PyDict_GetItemString(dict, "Dim");
    if (t == 0) {
      PyErr_SetString(PyExc_RuntimeError,
		      "Unable to get Dim type from gamera.gameracore.\n");
      return 0;
    }
  }
  return t;
}
#else
extern PyTypeObject* get_DimType();
#endif

inline bool is_DimObject(PyObject* x) {
  PyTypeObject* t = get_DimType();
  if (t == 0)
    return 0;
  return PyObject_TypeCheck(x, t);
}

inline PyObject* create_DimObject(const Dim& d) {
  PyTypeObject* t = get_DimType();
  if (t == 0)
    return 0;
  DimObject* so;
  so = (DimObject*)t->tp_alloc(t, 0);
  so->m_x = new Dim(d);
  return (PyObject*)so;
}

/*
  POINT OBJECT
*/
struct PointObject {
  PyObject_HEAD
  Point* m_x;
};

struct FloatPointObject {
  PyObject_HEAD
  FloatPoint* m_x;
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
		      "Unable to get Point type from gamera.gameracore.\n");
      return 0;
    }
  }
  return t;
}
#else
extern PyTypeObject* get_PointType();
#endif

#ifndef GAMERACORE_INTERNAL
inline PyTypeObject* get_FloatPointType() {
  static PyTypeObject* t = 0;
  if (t == 0) {
    PyObject* dict = get_gameracore_dict();
    if (dict == 0)
      return 0;
    t = (PyTypeObject*)PyDict_GetItemString(dict, "FloatPoint");
    if (t == 0) {
      PyErr_SetString(PyExc_RuntimeError,
		      "Unable to get FloatPoint type from gamera.gameracore.\n");
      return 0;
    }
  }
  return t;
}
#else
extern PyTypeObject* get_FloatPointType();
#endif

inline bool is_PointObject(PyObject* x) {
  PyTypeObject* t = get_PointType();
  if (t == 0)
    return 0;
  return PyObject_TypeCheck(x, t);
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

inline Point coerce_Point(PyObject* obj) {
  // Fast method if the Point is a real Point type.
  PyTypeObject* t2 = get_PointType();
  if (t2 == 0) {
    PyErr_SetString(PyExc_RuntimeError, "Couldn't get Point type.");
    throw std::runtime_error("Couldn't get Point type.");
  }
  if (PyObject_TypeCheck(obj, t2))
    return Point(*(((PointObject*)obj)->m_x));

  PyTypeObject* t = get_FloatPointType();
  if (t == 0) {
    PyErr_SetString(PyExc_RuntimeError, "Couldn't get FloatPoint type.");
    throw std::runtime_error("Couldn't get FloatPoint type.");
  }
  if (PyObject_TypeCheck(obj, t)) {
    FloatPoint* fp = ((FloatPointObject*)obj)->m_x;
    return Point(size_t(fp->x()), size_t(fp->y()));
  }

  PyObject* py_x0 = NULL;
  PyObject* py_y0 = NULL;
  PyObject* py_x1 = NULL;
  PyObject* py_y1 = NULL;

  // Treat 2-element sequences as Points.
  if (PySequence_Check(obj)) {
    if (PySequence_Length(obj) == 2) {
      py_x0 = PySequence_GetItem(obj, 0);
      py_x1 = PyNumber_Int(py_x0);
      if (py_x1 != NULL) {
	long x = PyInt_AsLong(py_x1);
	Py_DECREF(py_x1);
	py_y0 = PySequence_GetItem(obj, 1);
	py_y1 = PyNumber_Int(py_y0);
	if (py_y1 != NULL) {
	  long y = PyInt_AsLong(py_y1);
	  Py_DECREF(py_y1);
	  return Point((size_t)x, (size_t)y);
	}
      }
    }
  }

  PyErr_Clear();
  PyErr_SetString(PyExc_TypeError, "Argument is not a Point (or convertible to one.)");
  throw std::invalid_argument("Argument is not a Point (or convertible to one.)");
}

/*
  FLOATPOINT OBJECT
*/

inline FloatPoint coerce_FloatPoint(PyObject* obj) {
  // Fast method if the Point is a real FloatPoint or Point type.
  PyTypeObject* t = get_FloatPointType();
  if (t == 0) {
    PyErr_SetString(PyExc_RuntimeError, "Couldn't get FloatPoint type.");
    throw std::runtime_error("Couldn't get FloatPoint type.");
  }
  if (PyObject_TypeCheck(obj, t)) {
    return FloatPoint(*(((FloatPointObject*)obj)->m_x));
  }

  PyTypeObject* t2 = get_PointType();
  if (t2 == 0) {
    PyErr_SetString(PyExc_RuntimeError, "Couldn't get Point type.");
    throw std::runtime_error("Couldn't get Point type.");
  }
  if (PyObject_TypeCheck(obj, t2))
    return FloatPoint(*(((PointObject*)obj)->m_x));

  PyObject* py_x0 = NULL;
  PyObject* py_y0 = NULL;
  PyObject* py_x1 = NULL;
  PyObject* py_y1 = NULL;

  // Treat 2-element sequences as Points.
  if (PySequence_Check(obj)) {
    if (PySequence_Length(obj) == 2) {
      py_x0 = PySequence_GetItem(obj, 0);
      py_x1 = PyNumber_Float(py_x0);
      if (py_x1 != NULL) {
	double x = PyFloat_AsDouble(py_x1);
	Py_DECREF(py_x1);
	py_y0 = PySequence_GetItem(obj, 1);
	py_y1 = PyNumber_Float(py_y0);
	if (py_y1 != NULL) {
	  double y = PyFloat_AsDouble(py_y1);
	  Py_DECREF(py_y1);
	  return FloatPoint(x, y);
	}
      }
    }
  }

  PyErr_Clear();
  PyErr_SetString(PyExc_TypeError, "Argument is not a FloatPoint (or convertible to one.)");
  throw std::invalid_argument("Argument is not a FloatPoint (or convertible to one.)");
}

inline PyObject* create_FloatPointObject(const FloatPoint& d) {
  PyTypeObject* t = get_FloatPointType();
  if (t == 0)
    return 0;
  FloatPointObject* so;
  so = (FloatPointObject*)t->tp_alloc(t, 0);
  so->m_x = new FloatPoint(d);
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
		      "Unable to get Rect type from gamera.gameracore.\n");
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
  return PyObject_TypeCheck(x, t);
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
		      "Unable to get RGBPixel type from gamera.gameracore.\n");
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
  return PyObject_TypeCheck(x, t);
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
		      "Unable to get Region type from gamera.gameracore.\n");
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
  return PyObject_TypeCheck(x, t);
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
		      "Unable to get RegionMap type from gamera.gameracore.\n");
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
  return PyObject_TypeCheck(x, t);
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
		      "Unable to get ImageData type from gamera.gameracore.\n");
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
  return PyObject_TypeCheck(x, t);
}

inline PyObject* create_ImageDataObject(const Dim& dim, const Point& offset,
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
      o->m_x = new ImageData<OneBitPixel>(dim, offset);
    else if (pixel_type == GREYSCALE)
      o->m_x = new ImageData<GreyScalePixel>(dim, offset);      
    else if (pixel_type == GREY16)
      o->m_x = new ImageData<Grey16Pixel>(dim, offset);      
    // We have to explicity declare which FLOAT we want here, since there
    // is a name clash on Mingw32 with a typedef in windef.h
    else if (pixel_type == Gamera::FLOAT)
      o->m_x = new ImageData<FloatPixel>(dim, offset);      
    else if (pixel_type == RGB)
      o->m_x = new ImageData<RGBPixel>(dim, offset);      
    else if (pixel_type == Gamera::COMPLEX)
      o->m_x = new ImageData<ComplexPixel>(dim, offset);
    else {
      PyErr_Format(PyExc_TypeError, "Unknown pixel type '%d'.", pixel_type);
      return 0;
    }
  } else if (storage_format == RLE) {
    if (pixel_type == ONEBIT)
      o->m_x = new RleImageData<OneBitPixel>(dim, offset);
    else {
      PyErr_SetString(PyExc_TypeError,
		      "Pixel type must be ONEBIT when storage format is RLE.");
      return 0;
    }
  } else {
    PyErr_SetString(PyExc_TypeError, "Unknown pixel type/storage format combination.");
    return 0;
  }
  o->m_x->m_user_data = (void*)o;
  return (PyObject*)o;
}

#ifdef GAMERA_DEPRECATED
/*
create_ImageDataObject(int nrows, int ncols, int page_offset_y, int
page_offset_x, int pixel_type, int storage_format) is deprecated.

Reason: (x, y) coordinate consistency.

Use create_ImageDataObject(Dim(nrows, ncols), Point(page_offset_x,
page_offset_y), pixel_type, storage_format) instead.
*/
GAMERA_CPP_DEPRECATED
inline PyObject* create_ImageDataObject(int nrows, int ncols,
					int page_offset_y, int page_offset_x,
					int pixel_type, int storage_format) {
  return create_ImageDataObject(Dim(ncols, nrows), Point(page_offset_x, page_offset_y), pixel_type, storage_format);
}
#endif

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
		      "Unable to get Image type from gamera.gameracore.\n");
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
  return PyObject_TypeCheck(x, t);
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
		      "Unable to get SubImage type from gamera.gameracore.\n");
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
  return PyObject_TypeCheck(x, t);
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
		      "Unable to get CC type from gamera.gameracore.\n");
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
  return PyObject_TypeCheck(x, t);
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

inline const char* get_pixel_type_name(PyObject* image) {
  int pixel_type = get_pixel_type(image);
  const char* pixel_type_names[6] = {"OneBit", "GreyScale", "Grey16", "RGB", "Float", "Complex"};
  if (pixel_type >= 0 && pixel_type < 6)
    return pixel_type_names[pixel_type];
  else
    return "Unknown pixel type";
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

  if (dynamic_cast<Cc*>(image) != 0) {
    pixel_type = Gamera::ONEBIT;
    storage_type = Gamera::DENSE;
    cc = true;
  } else if (dynamic_cast<OneBitImageView*>(image) != 0) {
    pixel_type = Gamera::ONEBIT;
    storage_type = Gamera::DENSE;
  } else if (dynamic_cast<GreyScaleImageView*>(image) != 0) {
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
  } else if (dynamic_cast<ComplexImageView*>(image) != 0) {
    pixel_type = Gamera::COMPLEX;
    storage_type = Gamera::DENSE;
  } else if (dynamic_cast<OneBitRleImageView*>(image) != 0) {
    pixel_type = Gamera::ONEBIT;
    storage_type = Gamera::RLE;
  } else if (dynamic_cast<RleCc*>(image) != 0) {
    pixel_type = Gamera::ONEBIT;
    storage_type = Gamera::RLE;
    cc = true;
  } else {
    PyErr_SetString(PyExc_TypeError, "Unknown Image type returned from plugin.  Receiving this error indicates an internal inconsistency or memory corruption.  Please report it on the Gamera mailing list.");
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
		      "Unable to get ImageInfo type from gamera.gameracore.\n");
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
  return PyObject_TypeCheck(x, t);
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
inline PyObject* ImageList_to_python(std::list<Image*>* image_list) {
  PyObject* pylist = PyList_New(image_list->size());
  std::list<Image*>::iterator it = image_list->begin();
  for (size_t i = 0; i < image_list->size(); ++i, ++it) {
    PyObject *item = create_ImageObject(*it);
    PyList_SetItem(pylist, i, item);
  }
  return pylist;
}

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

inline PyObject* ComplexVector_to_python(ComplexVector* cpp) {
  PyObject* py = PyList_New(cpp->size());
  for (size_t i = 0; i < cpp->size(); ++i) {
    ComplexPixel& px = (*cpp)[i];
    PyObject* complex = PyComplex_FromDoubles(px.real(), px.imag());
    PyList_SET_ITEM(py, i, complex);
  }
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

inline PyObject* PointVector_to_python(PointVector* cpp) {
  PyObject* py = PyList_New(cpp->size());
  for (size_t i = 0; i < cpp->size(); ++i) {
    PyObject* point = create_PointObject(Point((*cpp)[i]));
    Py_INCREF(point); // leak?
    PyList_SetItem(py, i, point);
  }
  return py;
}

inline FloatVector* FloatVector_from_python(PyObject* py) {
  PyObject* seq = PySequence_Fast(py, "Argument must be a sequence of floats.");
  if (seq == NULL)
    return 0;
  int size = PySequence_Fast_GET_SIZE(seq);
  FloatVector* cpp = new FloatVector(size);
  try {
    for (int i = 0; i < size; ++i) {
      PyObject* number = PySequence_Fast_GET_ITEM(seq, i);
      if (!PyFloat_Check(number)) {
	delete cpp;
	PyErr_SetString(PyExc_TypeError,
			"Argument must be a sequence of floats.");
	Py_DECREF(seq);
	return 0;
      }      
      (*cpp)[i] = (double)PyFloat_AsDouble(number);
    }
  } catch (std::exception e) {
    delete cpp;
    Py_DECREF(seq);
    PyErr_SetString(PyExc_RuntimeError, e.what());
    return 0;
  }
  Py_DECREF(seq);
  return cpp;
}

inline ComplexVector* ComplexVector_from_python(PyObject* py) {
  PyObject* seq = PySequence_Fast(py, "Argument must be a sequence of complex numbers.");
  if (seq == NULL)
    return 0;
  int size = PySequence_Fast_GET_SIZE(seq);
  ComplexVector* cpp = new ComplexVector(size);
  try {
    for (int i = 0; i < size; ++i) {
      PyObject* value = PySequence_Fast_GET_ITEM(seq, i);
      if (!PyComplex_Check(value)) {
	delete cpp;
	Py_DECREF(seq);
	PyErr_SetString(PyExc_TypeError, "Argument must be a sequence of complex numbers.");
	return 0;
      }
      Py_complex temp = PyComplex_AsCComplex(value);
      (*cpp)[i] = ComplexPixel(temp.real, temp.imag);
    }
  } catch (std::exception e) {
    delete cpp;
    Py_DECREF(seq);
    PyErr_SetString(PyExc_RuntimeError, e.what());
    return 0;
  }
  Py_DECREF(seq);
  return cpp;
}

inline IntVector* IntVector_from_python(PyObject* py) {
  PyObject* seq = PySequence_Fast(py, "Argument must be a sequence of ints.");
  if (seq == NULL)
    return 0;
  int size = PySequence_Fast_GET_SIZE(seq);
  IntVector* cpp = new IntVector(size);
  try {
    for (int i = 0; i < size; ++i) {
      PyObject* number = PySequence_Fast_GET_ITEM(seq, i);
      if (!PyInt_Check(number)) {
	PyErr_SetString(PyExc_TypeError,
			"Argument must be a sequence of ints.");
	delete cpp;
	Py_DECREF(seq);
	return 0;
      }      
      (*cpp)[i] = (int)PyInt_AsLong(number);
    }
  } catch (std::exception e) {
    delete cpp;
    Py_DECREF(seq);
    PyErr_SetString(PyExc_RuntimeError, e.what());
    return 0;
  }
}

inline PointVector* PointVector_from_python(PyObject* py) {
  PyObject* seq = PySequence_Fast(py, "Argument must be an iterable of Points");
  if (seq == NULL)
    return 0;
  int size = PySequence_Fast_GET_SIZE(seq);
  PointVector* cpp = new PointVector();
  try {
    cpp->reserve(size);
    for (int i = 0; i < size; ++i) {
      PyObject* point = PySequence_Fast_GET_ITEM(seq, i);
      Point p = coerce_Point(point);
      cpp->push_back(p);
    }
  } catch (std::exception e) {
    delete cpp;
    Py_DECREF(seq);
    PyErr_SetString(PyExc_RuntimeError, e.what());
    return 0;
  }
  Py_DECREF(seq);
  return cpp;
}

/* ITERATOR TYPE
 */
inline PyTypeObject* get_IteratorType() {
  static PyTypeObject* t = 0;
  if (t == 0) {
    PyObject* dict = get_gameracore_dict();
    if (dict == 0)
      return 0;
    t = (PyTypeObject*)PyDict_GetItemString(dict, "Iterator");
    if (t == 0) {
      PyErr_SetString(PyExc_RuntimeError,
		      "Unable to get Iterator type from gamera.gameracore.\n");
      return 0;
    }
  }
  return t;
}

/* PROGRESS BAR TYPE */

class ProgressBar {
public:
  inline ProgressBar(char* message) {
    PyObject* dict = get_module_dict("gamera.util"); 
    if (!dict)
      throw std::runtime_error("Couldn't get gamera.util module");
    PyObject* progress_factory = PyDict_GetItemString(dict, "ProgressFactory");
    if (!progress_factory)
      throw std::runtime_error("Couldn't get ProgressFactory function");
    m_progress_bar = PyObject_CallFunction(progress_factory, "s", message);
    if (!m_progress_bar)
      throw std::runtime_error("Error getting progress bar");
  }
  inline ProgressBar() : m_progress_bar(NULL) {}
  inline ProgressBar(int x) {
    m_progress_bar = NULL;
  }
  inline ProgressBar(const ProgressBar& other) {
    m_progress_bar = other.m_progress_bar;
    if (m_progress_bar)
      Py_INCREF(m_progress_bar);
  }
  inline ~ProgressBar() {
    if (m_progress_bar)
      Py_DECREF(m_progress_bar);
  }
  inline void add_length(int l) {
    if (m_progress_bar) {
      PyObject* result = PyObject_CallMethod(m_progress_bar, "add_length", "i", l);
      if (!result)
	throw std::runtime_error("Error calling add_length on ProgressBar instance");
    }
  }
  inline void set_length(int l) {
    if (m_progress_bar) {
      PyObject* result = PyObject_CallMethod(m_progress_bar, "set_length", "i", l);
      if (!result)
	throw std::runtime_error("Error calling set_length on ProgressBar instance");
    }
  }
  inline void step() {
    if (m_progress_bar) {
      PyObject* result = PyObject_CallMethod(m_progress_bar, "step", NULL);
      if (!result)
	throw std::runtime_error("Error calling step on ProgressBar instance");
    }
  }
  inline void update(int num, int den) {
    if (m_progress_bar) {
      PyObject* result = PyObject_CallMethod(m_progress_bar, "update", "ii", num, den);
      if (!result)
	throw std::runtime_error("Error calling update on ProgressBar instance");
    }
  }
  inline void kill() {
    if (m_progress_bar) {
      PyObject* result = PyObject_CallMethod(m_progress_bar, "kill", NULL);
      if (!result)
	throw std::runtime_error("Error calling kill on ProgressBar instance");
    }
  }
protected:
  PyObject* m_progress_bar;
};

// Converting pixel types to/from Python

inline PyObject* pixel_to_python(OneBitPixel px) {
  return PyInt_FromLong(px);
}

inline PyObject* pixel_to_python(GreyScalePixel px) {
  return PyInt_FromLong(px);
}

inline PyObject* pixel_to_python(Grey16Pixel px) {
  return PyInt_FromLong(px);
}

inline PyObject* pixel_to_python(RGBPixel px) {
  return create_RGBPixelObject(px);
}

inline PyObject* pixel_to_python(FloatPixel px) {
  return PyFloat_FromDouble(px);
}

inline PyObject* pixel_to_python(ComplexPixel px) {
  return PyComplex_FromDoubles(px.real(), px.imag());
}

template<class T>
struct pixel_from_python {
  inline static T convert(PyObject* obj);
};

template<class T>
inline T pixel_from_python<T>::convert(PyObject* obj) {
  if (!PyFloat_Check(obj)) {
    if (!PyInt_Check(obj)) {
      if (!is_RGBPixelObject(obj)) {
	if (!PyComplex_Check(obj)) {
	  throw std::runtime_error("Pixel value is not valid");
	}
	Py_complex temp = PyComplex_AsCComplex(obj);
	return (T)temp.real;
      }
      return T((*(((RGBPixelObject*)obj)->m_x)).luminance());
    }
    return (T)PyInt_AsLong(obj);
  }
  return (T)PyFloat_AsDouble(obj);
}

template<>
inline RGBPixel pixel_from_python<RGBPixel>::convert(PyObject* obj) {
  if (!is_RGBPixelObject(obj)) {
    if (!PyFloat_Check(obj)) {
      if (!PyInt_Check(obj)) {
	if (!PyComplex_Check(obj)) {
	  throw std::runtime_error("Pixel value is not convertible to an RGBPixel");
	}
	Py_complex temp = PyComplex_AsCComplex(obj);
	return RGBPixel(ComplexPixel(temp.real, temp.imag));
      }
      return RGBPixel((GreyScalePixel)PyInt_AsLong(obj));
    }
    return RGBPixel(PyFloat_AsDouble(obj));
  }
  return RGBPixel(*(((RGBPixelObject*)obj)->m_x));
}

template<>
inline ComplexPixel pixel_from_python<ComplexPixel>::convert(PyObject* obj) {
  if (!PyComplex_Check(obj)) {
    if (!is_RGBPixelObject(obj)) {
      if (!PyFloat_Check(obj)) {
	if (!PyInt_Check(obj)) {
	  throw std::runtime_error("Pixel value is not convertible to a ComplexPixel");
	}
	return ComplexPixel((double)PyInt_AsLong(obj), 0.0);
      }
      return ComplexPixel(PyFloat_AsDouble(obj), 0.0);
    }
    return ComplexPixel(((RGBPixelObject*)obj)->m_x->luminance(), 0.0);
  }
  Py_complex temp = PyComplex_AsCComplex(obj);
  return ComplexPixel(temp.real, temp.imag);
}

#endif

#endif
 
