/*
 *
 * Copyright (C) 2001-2002 Ichiro Fujinaga, Michael Droettboom,
 * and Karl MacMillan
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

#ifndef kwm10242002_features
#define kwm10242002_features

#include "gamera.hpp"
#include "misc_filters.hpp"
#include <cmath>
#include <vector>

namespace Gamera {
  /*
    Black Area

    Find the number of black pixels in an image. This is just a convenience
    routine, but it demonstrates the correct method for determining whether
    a pixel is black or white.
  */

  template<class T>
  int black_area(const T& mat) {
    int black_pixels = 0;
    for (typename T::const_vec_iterator i = mat.vec_begin();
	 i != mat.vec_end(); ++i) {
      if (is_black(*i))
	black_pixels++;
    }
    return black_pixels;
  }

  // Ratio of black to white pixels
  
  template<class T>
  feature_t volume(const T &m) {
    unsigned int count = 0;
    typename T::const_vec_iterator i = m.vec_begin();
    for (; i != m.vec_end(); i++)
      if (is_black(*i))
	count++;
    return (feature_t(count) / (m.nrows() * m.ncols()));
  }
  

  /*
    MOMENTS

    Returns vector containing:
    u10, u01, u20, u02, u11, u30, u12, u21, u03
  */

  template<class Iterator>
  void moments_1d(Iterator begin, Iterator end, size_t& m1, size_t& m2,
		  size_t& m3) {
    // first, second, third order on one axis
    size_t tmp = 0;
    size_t x = 0;
    Iterator itx = begin;
    for (; itx != end; ++itx, x++) {
      size_t y = 0, proj = 0;
      typename Iterator::iterator ity = itx.begin();
      for (; ity != itx.end(); ++ity, ++y)
	if (is_black(*ity))
	  proj++;
      m1 += (tmp = x * proj);
      m2 += (tmp *= x);
      m3 += (tmp * x);
    }
  }

  template<class Iterator>
  void moments_2d(Iterator begin, Iterator end, size_t& m11, size_t& m12,
		  size_t& m21) {
    size_t tmp = 0;
    size_t x = 0;
    Iterator itx = begin;
    for (; itx != end; itx++, x++) {
      size_t y = 0;
      typename Iterator::iterator ity = itx.begin();
      for (; ity != itx.end(); ity++, y++)
	if (is_black(*ity)) {
	  m11 += (tmp = x * y);
	  m21 += (tmp * x);
	  m12 += (tmp * y);
	}
    }
  }

  template<class T>
  FloatVector* moments(T &m) {
    FloatVector* _output = new FloatVector(9);
    FloatVector& output = *_output;

    size_t m10 = 0, m11 = 0, m20 = 0, m21 = 0, m12 = 0, 
      m01 = 0, m02 = 0, m30 = 0, m03 = 0;
    size_t m00 = (unsigned int)(m.nrows() * m.ncols());
    moments_1d(m.row_begin(), m.row_end(), m01, m02, m03);
    moments_1d(m.col_begin(), m.col_end(), m10, m20, m30);
    moments_2d(m.col_begin(), m.col_end(), m11, m12, m21);

    feature_t x, y, x2, y2, div;
    x = (feature_t)m10 / m00;
    x2 = 2 * x * x;
    y = (feature_t)m01 / m00;
    y2 = 2 * y * y;

    output[0] = x / m.ncols(); // normalized center of gravity [0,1] 
    output[1] = y / m.nrows(); // normalized center of gravity [0,1] 
  
    div = (feature_t)m00 * m00; // common normalization divisor 
    output[2] = (m20 - (x * m10)) / div; // u20 
    output[3] = (m02 - (y * m01)) / div; // u02 
    output[4] = (m11 - (y * m10)) / div; // u11 
  
    div *= (feature_t)sqrt((double)m00);  
    output[5] = (m30 - (3 * x * m20) + (x2 * m10)) / div;                // u30
    output[6] = (m12 - (2 * y * m11) - (x * m02) + (y2 * m10)) / div;    // u12
    output[7] = (m21 - (2 * x * m11) - (y * m20) + (x2 * m01)) / div;    // u21
    output[8] = (m03 - (3 * x * m02) + (y2 + m01)) / div;                // u03

    return _output;
  }
 
  // Number of holes in x and y direction
  
  // Counts the number of black runs (runs that "wrap-around" considered
  // two runs).
  // This is the Euler number
  
  // See:
  // Di Zenzo, S.; Cinque, L.; Levialdi, S. 
  // Run-based algorithms for binary image processing
  // Pattern Analysis and Machine Intelligence, IEEE Transactions on , 
  // Volume: 18 Issue: 1 , Jan. 1996 
  // Page(s): 83 -89
  
  template<class Iterator>
  inline size_t nholes_1d(Iterator begin, Iterator end) {
    size_t hole_count = 0;
    bool last;
    Iterator r = begin;
    for (; r != end; r++) {
      last = false;
      typename Iterator::iterator c = r.begin();
      for (; c != r.end(); c++) {
	if (is_black(*c))
	  last = true;
	else if (last) {
	  last = false;
	  hole_count++;
	}
      }
      if (!last)
	hole_count--;
    }
    return hole_count;
  }
  
  template<class T>
  FloatVector* nholes(T &m) {
    FloatVector* output = new FloatVector(2);
    size_t vert, horiz;
    
    vert = nholes_1d(m.col_begin(), m.col_end());
    horiz = nholes_1d(m.row_begin(), m.row_end());
    
    (*output)[0] = (feature_t)vert / m.ncols();
    (*output)[1] = (feature_t)horiz / m.nrows();
    return output;
  }

  /*
    nholes_extended

    This divides the image into quarters (both horizontally
    and vertically) and computes the number of holes on
    each of the sections.
  */
  template<class T>
  FloatVector* nholes_extended(const T& m) {
    FloatVector* output = new FloatVector(8);
    double quarter_cols = m.ncols() / 4.0;
    double start = 0.0;
    for (size_t i = 0; i < 4; ++i) {
      (*output)[i] = nholes_1d(m.col_begin() + size_t(start),
			       m.col_begin() + size_t(start) + size_t(start));
      start += quarter_cols;
    }
    double quarter_rows = m.nrows() / 4.0;
    start = 0.0;
    for (size_t i = 0; i < 4; ++i) {
      (*output)[i + 4] = nholes_1d(m.row_begin() + size_t(start),
				   m.row_begin() + size_t(start)
				   + size_t(start));
      start += quarter_rows;
    }
    return output;
  }

  template<class T>
  feature_t area(const T& image) {
    std::cout << image.nrows() << " " << image.ncols() << " " << image.scaling() << std::endl;
    return feature_t(image.nrows() * image.ncols()) / image.scaling();
  }

  template<class T>
  feature_t aspect_ratio(const T& image) {
    return feature_t(image.ncols()) / feature_t(image.nrows());
  }

  /*
    compactness
    
    compactness is ratio of the volume of the outline of an image to
    the volume of the image.
  */
  template<class T>
  feature_t compactness(const T& image) {
    feature_t vol = volume(image);
    if (vol == 0)
      return 32767;
    typename ImageFactory<T>::view_type* ol = outline(image);
    feature_t return_value = volume(*ol) / vol * image.scaling();
    delete ol->data();
    delete ol;
    return return_value;
  }

  /*
    volume16regions

    This function divides the image into 16 regions and takes the volume of
    each of those regions.
  */
  template<class T>
  FloatVector* volume16regions(const T& image) {
    float quarter_rows = image.height() / 4.0;
    float quarter_cols = image.width() / 4.0;
    size_t start_row = image.offset_y();
    size_t start_col = image.offset_x();
    FloatVector* volumes = new FloatVector(16);
    for (size_t i = 0; i < 4; ++i) {
      for (size_t j = 0; j < 4; ++j) {
	T tmp(image, start_row, start_col,
	      size_t(quarter_rows + 1),
	      size_t(quarter_cols + 1));
	(*volumes)[i * 4 + j] = volume(tmp);
	start_row += size_t(quarter_rows);
      }
      start_col += size_t(quarter_cols);
      start_row = image.offset_y();
    }
    return volumes;
  }

  /*
    volume64regions

    This function divides the image into 64 regions and takes the volume of
    each of those regions.
  */
  template<class T>
  FloatVector* volume64regions(const T& image) {
    float quarter_rows = image.height() / 8.0;
    float quarter_cols = image.width() / 8.0;
    size_t start_row = image.offset_y();
    size_t start_col = image.offset_x();
    FloatVector* volumes = new FloatVector(64);
    for (size_t i = 0; i < 8; ++i) {
      for (size_t j = 0; j < 8; ++j) {
	T tmp(image, start_row, start_col,
	      size_t(quarter_rows + 1),
	      size_t(quarter_cols + 1));
	(*volumes)[i * 8 + j] = volume(tmp);
	start_row += size_t(quarter_rows);
      }
      start_col += size_t(quarter_cols);
      start_row = image.offset_y();
    }
    return volumes;
  }

}
#endif
