/*
 *
 * Copyright (C) 2001-2004
 * Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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

#ifndef kwm01102002_to_string
#define kwm01102002_to_string

#include "gamera.hpp"
#include "vigra/resizeimage.hxx"
#include "image_utilities.hpp"
#include "connected_components.hpp"
#include "Python.h"

#include <algorithm>

namespace Gamera {

namespace {

  template<class T>
  struct to_string_impl {
    template<class Mat>
    void operator()(const Mat& mat, char* data) {
      char* i = data;
      ImageAccessor<T> acc;
      typename Mat::const_vec_iterator it = mat.vec_begin();
      T tmp;
      for (; it != mat.vec_end(); ++it) {
	tmp = acc.get(it);
	*i = (char)tmp; i++;
	*i = (char)tmp; i++;
	*i = (char)tmp; i++;
      }
    }
  };
  
  template<>
  struct to_string_impl<FloatPixel> {
    template<class Mat>
    void operator()(const Mat& mat, char* data) {
      char* i = data;
      FloatPixel max = 0;
      max = find_max(mat.parent());
      if (max > 0)
	max = 255.0 / max;
      else 
	max = 0;

      typename Mat::const_vec_iterator vi = mat.vec_begin();
      FloatPixel tmp;
      for (; vi != mat.vec_end(); vi++) {
	tmp = *vi * max;
	if (tmp > 255)
	  tmp = 255;
	*i = (char)tmp; i++;
	*i = (char)tmp; i++;
	*i = (char)tmp; i++;
      }
    }
  };

  template<>
  struct to_string_impl<Grey16Pixel> {
    template<class Mat>
    void operator()(const Mat& mat, char* data) {
      ImageAccessor<Grey16Pixel> acc;
      typename Mat::const_vec_iterator it = mat.vec_begin();
      char tmp;
      char* i = data;
      for (; it != mat.vec_end(); ++it) {
	  /*
	    This should correctly map the 16 bit grey values onto
	    the rgb color space. KWM
	  */
	  tmp = char(acc.get(it));
	  *i = tmp; i++;
	  *i = tmp; i++;
	  *i = tmp; i++;
      }
    }
  };
  
  template<>
  struct to_string_impl<RGBPixel> {
    template<class Mat>
    void operator()(const Mat& mat, char* data) {
      ImageAccessor<RGBPixel> acc;
      typename Mat::const_vec_iterator it = mat.vec_begin();
      register unsigned char* i = (unsigned char *)data;
      for (; it != mat.vec_end(); ++it) {
	RGBPixel tmp = acc.get(it);
	*i = (unsigned char)tmp.red(); i++;
	*i = (unsigned char)tmp.green(); i++;
	*i = (unsigned char)tmp.blue(); i++;
      }
    }
  };
  
  template<>
  struct to_string_impl<OneBitPixel> {
    template<class Mat>
    void operator()(const Mat& mat, char* data) {
      char* i = data;
      typename Mat::const_vec_iterator it = mat.vec_begin();
      ImageAccessor<OneBitPixel> acc;
      OneBitPixel tmp;
      unsigned char val;
      for (; it != mat.vec_end(); ++it) {
	tmp = acc(it);
	if (is_white(tmp))
	  tmp = 255;
	else
	  tmp = 0;
	val = (unsigned char)tmp;
	*i = val; i++;
	*i = val; i++;
	*i = val; i++;
      }
    }
  };
};

template<class T>
PyObject* to_string(T& m) {
  PyObject* str = PyString_FromString("this is stupid\n");
  if (_PyString_Resize(&str, m.nrows() * m.ncols() * 3) != 0)
    return 0;
  char* buffer = PyString_AS_STRING(str);
  to_string_impl<typename T::value_type> func;
  func(m, buffer);
  return str;
}

template<class T>
void to_buffer(T& m, PyObject *py_buffer) {
  char *buffer;
  int buffer_len;
  PyObject_AsWriteBuffer(py_buffer, (void **)&buffer, &buffer_len);
  to_string_impl<typename T::value_type> func;
  func(m, buffer);
}

template<class T>
Image *color_ccs(T& m) {
  typedef TypeIdImageFactory<RGB, DENSE> RGBViewFactory;
  RGBViewFactory::image_type* image =
    RGBViewFactory::create(0, 0, m.nrows(), m.ncols());

  typename T::vec_iterator src = m.vec_begin();
  typename RGBImageView::vec_iterator dst = image->vec_begin();

  for (; src != m.vec_end(); ++src, ++dst) {
    size_t color;
    if (is_white(*src)) {
      dst->red(255);
      dst->green(255);
      dst->blue(255);
    } else {
      color = *src & 0x7;
      dst->red(color_set[color][0]);
      dst->green(color_set[color][1]);
      dst->blue(color_set[color][2]);
    }
  }
  
  return image;
}


}

#endif
