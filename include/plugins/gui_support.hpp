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
      typename Mat::const_row_iterator row = mat.row_begin();
      typename Mat::const_col_iterator col;
      T tmp;
      for (; row != mat.row_end(); ++row) {
	for (col = row.begin(); col != row.end(); ++col) {
	  tmp = acc.get(col);
	  if (tmp > 255)
	    tmp = 255;
	  *(i++) = (char)tmp;
	  *(i++) = (char)tmp;
	  *(i++) = (char)tmp;
	}
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
	*(i++) = (char)tmp;
	*(i++) = (char)tmp;
	*(i++) = (char)tmp;
      }
    }
  };

  template<>
  struct to_string_impl<ComplexPixel> {
    template<class Mat>
    void operator()(const Mat& mat, char* data) {
      char* i = data;

      if ((mat.parent().nrows() <= 1) || mat.parent().ncols() <= 1)
	throw std::range_error("Out of range!");
      double scale;
      ComplexPixel max = 0;
      max = find_max(mat.parent());
      if (max.real() > 0)
	scale = 255.0 / max.real();
      else 
	scale = 0.0;

      typename Mat::const_vec_iterator vi = mat.vec_begin();
      double tmp;
      for (; vi != mat.vec_end(); ++vi) {
	tmp = (*vi).real() * scale;
	if (tmp > 255.0)
	  tmp = 255.0;
	*i = (char)floor(tmp); i++;
	*i = (char)floor(tmp); i++;
	*i = (char)floor(tmp); i++;
      }
    }
  };

  template<>
  struct to_string_impl<Grey16Pixel> {
    template<class Mat>
    void operator()(const Mat& mat, char* data) {
      ImageAccessor<Grey16Pixel> acc;
      typename Mat::const_row_iterator row = mat.row_begin();
      typename Mat::const_col_iterator col;
      char tmp;
      char* i = data;
      for (; row != mat.row_end(); ++row) {
	for (col = row.begin(); col != row.end(); ++col) {
	  /*
	    This should correctly map the 16 bit grey values onto
	    the rgb color space. KWM
	  */
	  tmp = char(acc.get(col));
	  *(i++) = tmp;
	  *(i++) = tmp;
	  *(i++) = tmp;
	}
      }
    }
  };
  
  template<>
  struct to_string_impl<RGBPixel> {
    template<class Mat>
    void operator()(const Mat& mat, char* data) {
      ImageAccessor<RGBPixel> acc;
      typename Mat::const_row_iterator row = mat.row_begin();
      typename Mat::const_col_iterator col;
      char* i = data;
      for (; row != mat.row_end(); ++row) {
	for (col = row.begin(); col != row.end(); ++col) {
	  RGBPixel tmp = acc.get(col);
	  *(i++) = (unsigned char)tmp.red();
	  *(i++) = (unsigned char)tmp.green();
	  *(i++) = (unsigned char)tmp.blue();
	}
      }
    }
  };
  
  template<>
  struct to_string_impl<OneBitPixel> {
    template<class Mat>
    void operator()(const Mat& mat, char* data) {
      char* i = data;
      typename Mat::const_row_iterator row = mat.row_begin();
      typename Mat::const_col_iterator col;
      ImageAccessor<OneBitPixel> acc;
      unsigned char tmp;
      for (; row != mat.row_end(); ++row) {
	for (col = row.begin(); col != row.end(); ++col) {
	  if (is_white(acc(col)))
 	    tmp = 255;
 	  else
 	    tmp = 0;
	  *(i++) = tmp;
	  *(i++) = tmp;
	  *(i++) = tmp;
	}
      }
    }
  };
};

template<class T>
PyObject* to_string(T& m) {
  PyObject* str = PyString_FromString("");
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
      const unsigned char* out_color = color_set[color];
      dst->red(out_color[0]);
      dst->green(out_color[1]);
      dst->blue(out_color[2]);
    }
  }
  
  return image;
}


}

#endif
