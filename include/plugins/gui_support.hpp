/*
 *
 * Copyright (C) 2001 - 2002
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
#include "Python.h"

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
	  *i = (char)tmp; i++;
	  *i = (char)tmp; i++;
	  *i = (char)tmp; i++;
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
      Mat full_mat = mat.parent();
      typename Mat::vec_iterator di = full_mat.vec_begin();
      for (; di != full_mat.vec_end(); di++) {
	if (max < *di)
	  max = *di;
      }
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
	  *i = tmp; i++;
	  *i = tmp; i++;
	  *i = tmp; i++;
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
      for (size_t i = 0; row != mat.row_end(); ++row) {
	for (col = row.begin(); col != row.end(); i += 3, ++col) {
	  RGBPixel tmp = acc.get(col);
	  data[i] = (unsigned char)tmp.red();
	  data[i + 1] = (unsigned char)tmp.green();
	  data[i + 2] = (unsigned char)tmp.blue();
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
      OneBitPixel tmp;
      for (; row != mat.row_end(); ++row) {
	for (col = row.begin(); col != row.end(); ++col) {
	  tmp = acc(col);
	  if (is_white(tmp))
	    tmp = 255;
	  else if (is_black(tmp))
	    tmp = 0;
	  *i = (char)tmp; i++;
	  *i = (char)tmp; i++;
	  *i = (char)tmp; i++;
	}
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
void highlight(T &m, PyObject *highlights) {
  typename T::vec_iterator dst = m.vec_begin();

  size_t list_size = PyList_GET_SIZE(highlights);
  for (size_t i=0; i < list_size; ++i) {
    std::cerr << "highlight\n";
    PyObject *list_item = PyList_GET_ITEM(highlights, i);
    Cc *cc = (Cc *)((RectObject*)PyTuple_GET_ITEM(list_item, 0))->m_x;
    RGBPixel *color = ((RGBPixelObject*)PyTuple_GET_ITEM(list_item, 1))->m_x;
    if (m.intersects(*cc)) {
      std::cerr << "intersects\n";
      std::cerr << m.ul_x() << " " << m.ul_y() << " " << m.lr_x() << " " << m.lr_y() << " " << cc->ul_x() << " " << cc->ul_y() << " " << cc->lr_x() << " " << cc->lr_y() << "\n";
      for (size_t r = m.ul_y(); r <= m.lr_y(); r++) {
	for (size_t c = m.ul_x(); c <= m.lr_x(); c++, dst++) {
	  // Why is point reversed?
	  if (cc->contains_point(Point(c, r))) {
	    std::cerr << ",";
	    // Why is get not page relative?
	    if (is_black(cc->get(r - cc->ul_y(), c - cc->ul_x()))) {
	      std::cerr << "." << int(color->red()) << " " << int(color->green()) << " " << int(color->blue());
	      *dst = *color;
	    }
	  }
	}
      }
    }
  }
}



template<class T>
Image *color_ccs(T& m) {
  typedef TypeIdImageFactory<RGB, DENSE> RGBViewFactory;
  RGBViewFactory fact;
  RGBViewFactory::image_type* image =
    fact.create(0, 0, m.nrows(), m.ncols());

  typename T::vec_iterator src = m.vec_begin();
  typename RGBViewFactory::image_type::vec_iterator dst = image->vec_begin();

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
