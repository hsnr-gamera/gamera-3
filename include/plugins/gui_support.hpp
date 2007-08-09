/*
 *
 * Copyright (C) 2001-2005
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

      typename Mat::const_vec_iterator vi = mat.vec_begin();
      FloatPixel max = *vi;
      FloatPixel min = *vi;
      for (; vi != mat.vec_end(); ++vi) {
	if (*vi > max)
	  max = *vi;
	if (*vi < min)
	  min = *vi;
      }

      FloatPixel scale = 255.0 / (max - min);

      vi = mat.vec_begin();
      FloatPixel tmp;
      for (; vi != mat.vec_end(); vi++) {
	tmp = (*vi + min) * scale;
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

      typename Mat::const_vec_iterator vi = mat.vec_begin();
      double max = (*vi).real();
      double min = (*vi).real();
      for (; vi != mat.vec_end(); ++vi) {
	if ((*vi).real() > max)
	  max = (*vi).real();
	if ((*vi).real() < min)
	  min = (*vi).real();
      }

      double scale = 255.0 / (max - min);

      vi = mat.vec_begin();
      double tmp;
      for (; vi != mat.vec_end(); ++vi) {
	tmp = ((*vi).real() - min) * scale;
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
  PyObject* str = PyString_FromStringAndSize(NULL, m.nrows() * m.ncols() * 3);
  if (!str)
    throw std::exception();
  char* buffer;
#ifdef HAVE_SSIZE_T
  Py_ssize_t length;
#else
  int length;
#endif
  int error = PyString_AsStringAndSize(str, &buffer, &length);
  if (error) {
    Py_DECREF(str);
    throw std::exception();
  }
  to_string_impl<typename T::value_type> func;
  func(m, buffer);
  return str;
}

template<class T>
void to_buffer(T& m, PyObject *py_buffer) {
  char *buffer;
#ifdef HAVE_SSIZE_T  
  Py_ssize_t buffer_len;
#else
  int buffer_len;
#endif
  PyObject_AsWriteBuffer(py_buffer, (void **)&buffer, &buffer_len);
  if (buffer_len != m.nrows() * m.ncols() * 3 || buffer == NULL) {
    printf("The image passed to to_buffer is not of the correct size.\n");
    return;
  }
  to_string_impl<typename T::value_type> func;
  func(m, buffer);
}

template<class T>
Image *color_ccs(T& m) {
  typedef TypeIdImageFactory<RGB, DENSE> RGBViewFactory;
  RGBViewFactory::image_type* image =
    RGBViewFactory::create(m.origin(), m.dim());

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

template<class P>
struct to_buffer_colorize_impl {
  template<class T>
  void operator()(const T& m, char* buffer,
		  unsigned char red, unsigned char green, unsigned char blue) {
    size_t r = (size_t)red;
    size_t g = (size_t)green;
    size_t b = (size_t)blue;

    char* i = buffer;
    typename T::const_row_iterator row = m.row_begin();
    typename T::const_col_iterator col;
    ImageAccessor<GreyScalePixel> acc;
    for (; row != m.row_end(); ++row) {
      for (col = row.begin(); col != row.end(); ++col) {
	GreyScalePixel tmp = acc(col);
	*(i++) = (r * tmp) >> 8;
	*(i++) = (g * tmp) >> 8;
	*(i++) = (b * tmp) >> 8;
      }
    }
  }
};

template<class P>
struct to_buffer_colorize_invert_impl {
  template<class T>
  void operator()(const T& m, char* buffer,
		  unsigned char red, unsigned char green, unsigned char blue) {
    size_t r = (size_t)red;
    size_t g = (size_t)green;
    size_t b = (size_t)blue;

    char* i = buffer;
    typename T::const_row_iterator row = m.row_begin();
    typename T::const_col_iterator col;
    ImageAccessor<GreyScalePixel> acc;
    for (; row != m.row_end(); ++row) {
      for (col = row.begin(); col != row.end(); ++col) {
	GreyScalePixel tmp = 255 - acc(col);
	*(i++) = (r * tmp) >> 8;
	*(i++) = (g * tmp) >> 8;
	*(i++) = (b * tmp) >> 8;
      }
    }
  }
};

template<>
struct to_buffer_colorize_impl<OneBitPixel> {
  template<class T>
  void operator()(const T& m, char* buffer,
		  unsigned char red, unsigned char green, unsigned char blue) {
    char* i = buffer;
    typename T::const_row_iterator row = m.row_begin();
    typename T::const_col_iterator col;
    ImageAccessor<OneBitPixel> acc;
    for (; row != m.row_end(); ++row) {
      for (col = row.begin(); col != row.end(); ++col) {
	if (is_white(acc(col))) {
	  *(i++) = red;
	  *(i++) = green;
	  *(i++) = blue;
	} else {
	  *(i++) = 0;
	  *(i++) = 0;
	  *(i++) = 0;
	}
      }
    }
  }
};

template<>
struct to_buffer_colorize_invert_impl<OneBitPixel> {
  template<class T>
  void operator()(const T& m, char* buffer,
		  unsigned char red, unsigned char green, unsigned char blue) {
    char* i = buffer;
    typename T::const_row_iterator row = m.row_begin();
    typename T::const_col_iterator col;
    ImageAccessor<OneBitPixel> acc;
    for (; row != m.row_end(); ++row) {
      for (col = row.begin(); col != row.end(); ++col) {
	if (is_white(acc(col))) {
	  *(i++) = 0;
	  *(i++) = 0;
	  *(i++) = 0;
	} else {
	  *(i++) = red;
	  *(i++) = green;
	  *(i++) = blue;
	}
      }
    }
  }
};

template<class T>
void to_buffer_colorize(const T& m, PyObject* py_buffer, 
			int red, int green, int blue,
			bool invert) {
  char *buffer;
#ifdef HAVE_SSIZE_T
  Py_ssize_t buffer_len;
#else
  int buffer_len;
#endif

  PyObject_AsWriteBuffer(py_buffer, (void **)&buffer, &buffer_len);
  if (buffer_len != m.nrows() * m.ncols() * 3 || buffer == NULL) {
    printf("The image passed to to_buffer is not of the correct size.\n");
    return;
  }

  unsigned char rc = (unsigned char)red;
  unsigned char gc = (unsigned char)green;
  unsigned char bc = (unsigned char)blue;

  if (invert) {
    to_buffer_colorize_invert_impl<typename T::value_type> func;
    func(m, buffer, rc, gc, bc);
  } else {
    to_buffer_colorize_impl<typename T::value_type> func;
    func(m, buffer, rc, gc, bc);
  }
}

template<class T, class U>
void draw_cc(T& m, const U& cc,
	     int red, int green, int blue) {
  if (cc.intersects(m)) {
    RGBPixel color((unsigned char)red, (unsigned char)green, (unsigned char)blue);
    Rect intersection = cc.intersection(m);
    T sub_m(m, intersection);
    U sub_cc(cc, intersection);

    typename T::row_iterator dst_row = sub_m.row_begin();
    typename T::col_iterator dst_col;
    typename U::row_iterator src_row = sub_cc.row_begin();
    typename U::col_iterator src_col;
    ImageAccessor<OneBitPixel> acc;
    for (; dst_row != sub_m.row_end(); ++dst_row, ++src_row) {
      for (dst_col = dst_row.begin(), src_col = src_row.begin(); 
	   dst_col != dst_row.end(); 
	   ++dst_col, ++src_col) {
	if (is_black(acc(src_col))) {
	  *dst_col = color;
	}
      }
    }
  }
}

}

#endif
