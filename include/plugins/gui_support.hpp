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
#include <string>
#include <iostream>

using namespace Gamera;

namespace {

  template<class T>
  struct to_string_impl {
    template<class Mat>
    void operator()(const Mat& mat, std::string& data) {
      std::string::iterator i = data.begin();
      typename Mat::const_vec_iterator vi = mat.vec_begin();
      T tmp;
      for (; vi != mat.vec_end(); vi++) {
	tmp = *vi;
	if (tmp > 255)
	  tmp = 255;
	if (tmp > 0) {
	  *i = (char)tmp; i++;
	  *i = (char)tmp; i++;
	  *i = (char)tmp; i++;
	} else {
	  i += 3;
	}
      }
    }
  };
  
  template<>
  struct to_string_impl<FloatPixel> {
    template<class Mat>
    void operator()(const Mat& mat, std::string& data) {
      std::string::iterator i = data.begin();
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
	if (tmp > 0) {
	  *i = (char)tmp; i++;
	  *i = (char)tmp; i++;
	  *i = (char)tmp; i++;
	} else {
	  i += 3;
	}
      }
    }
  };

  template<>
  struct to_string_impl<Grey16Pixel> {
    template<class Mat>
    void operator()(const Mat& mat, std::string& data) {
      typename Mat::const_vec_iterator vi = mat.vec_begin();
      Grey16Pixel tmp;
      std::string::iterator i = data.begin();
      for (; vi != mat.vec_end(); vi++) {
	/*
	  This should correctly map the 16 bit grey values onto
	  the rgb color space. KWM
	*/
	tmp = *vi / 257;
	if (tmp > 0) {
	  *i = (char)tmp; i++;
	  *i = (char)tmp; i++;
	  *i = (char)tmp; i++;
	} else {
	  i += 3;
	}
      }
    }
  };
  
  template<>
  struct to_string_impl<RGBPixel> {
    template<class Mat>
    void operator()(const Mat& mat, std::string& data) {
      typename Mat::const_vec_iterator vi = mat.vec_begin();
      for (size_t i = 0; vi != mat.vec_end(); i += 3, vi++) {
	RGBPixel tmp = *vi;
	data[i] = (char)tmp.red();
	data[i + 1] = (char)tmp.green();
	data[i + 2] = (char)tmp.blue();
      }
    }
  };
  
  template<>
  struct to_string_impl<OneBitPixel> {
    template<class Mat>
    void operator()(const Mat& mat, std::string& data) {
      std::string::iterator i = data.begin();
      typename Mat::const_vec_iterator vi = mat.vec_begin();
      OneBitPixel tmp;
      for (; vi != mat.vec_end(); vi++) {
	tmp = *vi;
	if (is_white(tmp))
	  tmp = 255;
	else if (is_black(tmp))
	  tmp = 0;
	if (tmp > 0) {
	  *i = (char)tmp; i++;
	  *i = (char)tmp; i++;
	  *i = (char)tmp; i++;
	} else {
	  i += 3;
	}
      }
    }
  };
};

template<class T>
std::string to_string(T& m) {
  std::string buffer;
  buffer.resize(m.nrows() * m.ncols() * 3);
  to_string_impl<typename T::value_type> func;
  func(m, buffer);
  return buffer;
}

#endif
