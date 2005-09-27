/*
 *
 * Copyright (C) 2005 Alex Cobb
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

#include "gamera.hpp"

#ifndef arc24_feb_2005_io
#define arc24_feb_2005_io

using namespace Gamera;

template<class T>
PyObject* _to_raw_string(const T &image) {
  typedef typename T::value_type value_type;
  typename T::const_vec_iterator j = image.vec_begin();
  size_t image_size = image.ncols() * image.nrows() * sizeof(value_type);
  PyObject* pystring = PyString_FromStringAndSize((char *)NULL, 
						  (int)image_size);
  if (pystring == NULL)
    return NULL;
  value_type* i = (value_type*)PyString_AS_STRING(pystring);
  for (; j != image.vec_end(); ++i, ++j) {
    *i = *j;
  }
  return Py_BuildValue("O", pystring);
};

template <class T>
bool fill_image_from_string(T &image, PyObject* data_string) {
  if (!PyString_CheckExact(data_string)) {
    PyErr_SetString(PyExc_TypeError, 
		    "data_string must be a Python string");
    return false;
  }
  char* s = PyString_AS_STRING(data_string);
  size_t length = PyString_GET_SIZE(data_string);
  typedef typename T::value_type value_type;
  size_t image_size = image.ncols() * image.nrows() * sizeof(value_type);
  if (length != image_size) {
    if (length > image_size) {
      PyErr_SetString(PyExc_ValueError,
		      "data_string too long for image");
    } else {
      PyErr_SetString(PyExc_ValueError,
		      "data_string too short for image");
    }
    return false;
  }
  typename T::vec_iterator i = image.vec_begin();
  value_type* j = (value_type*)s;
  for (; i != image.vec_end(); ++i, ++j) {
    *i = *j;
  }
  return true;
}

Image* _from_raw_string(Point offset, Dim size, 
			int pixel_type, int storage_format, 
			PyObject* data_string) {
  if (pixel_type == ONEBIT and storage_format == RLE) {
    typedef TypeIdImageFactory<ONEBIT, RLE> factory;
    typedef factory::image_type image_type;
    image_type* image = factory::create(offset, size);
    if (fill_image_from_string(*image, data_string))
      return image;
  } else if (pixel_type == ONEBIT and storage_format == DENSE) {
    typedef TypeIdImageFactory<ONEBIT, DENSE> factory;
    typedef factory::image_type image_type;
    image_type* image = factory::create(offset, size);
    if (fill_image_from_string(*image, data_string))
      return image;
  } else if (pixel_type == GREYSCALE) {
    typedef TypeIdImageFactory<GREYSCALE, DENSE> factory;
    typedef factory::image_type image_type;
    image_type* image = factory::create(offset, size);
    if (fill_image_from_string(*image, data_string))
      return image;
  } else if (pixel_type == GREY16) {
    typedef TypeIdImageFactory<GREY16, DENSE> factory;
    typedef factory::image_type image_type;
    image_type* image = factory::create(offset, size);
    if (fill_image_from_string(*image, data_string))
      return image;
  } else if (pixel_type == RGB) {
    typedef TypeIdImageFactory<RGB, DENSE> factory;
    typedef factory::image_type image_type;
    image_type* image = factory::create(offset, size);
    if (fill_image_from_string(*image, data_string))
      return image;
  } else if (pixel_type == FLOAT) {
    typedef TypeIdImageFactory<FLOAT, DENSE> factory;
    typedef factory::image_type image_type;
    image_type* image = factory::create(offset, size);
    if (fill_image_from_string(*image, data_string))
      return image;
  } else if (pixel_type == COMPLEX) {
    typedef TypeIdImageFactory<COMPLEX, DENSE> factory;
    typedef factory::image_type image_type;
    image_type* image = factory::create(offset, size);
    if (fill_image_from_string(*image, data_string))
      return image;
  } else {
    PyErr_SetString(PyExc_ValueError, "Invalid pixel_type or storage_format");
    return NULL;
  }
  return NULL;
}

#endif

