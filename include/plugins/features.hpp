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
#include "image_utilities.hpp"
#include "morphology.hpp"
#include "thinning.hpp"
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
  FloatVector* black_area(const T& mat) {
    int black_pixels = 0;
    for (typename T::const_vec_iterator i = mat.vec_begin();
	 i != mat.vec_end(); ++i) {
      if (is_black(*i))
	black_pixels++;
    }
    FloatVector* vec = new FloatVector(1);
    (*vec)[0] = (feature_t)black_pixels;
    return vec;
  }

  // Ratio of black to white pixels

  template<class T>
  feature_t volume0(const T &m) {
    unsigned int count = 0;
    typename T::const_vec_iterator i = m.vec_begin();
    for (; i != m.vec_end(); i++)
      if (is_black(*i))
	count++;
    return (feature_t(count) / (m.nrows() * m.ncols()));
  }
  
  template<class T>
  FloatVector* volume(const T &m) {
    FloatVector* vec = new FloatVector(1);
    (*vec)[0] = volume0(m);
    return vec;
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

    // This is just for convenience below
    FloatVector& output = *_output;
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
			       m.col_begin() + size_t(start) + size_t(quarter_cols));
      start += quarter_cols;
    }
    double quarter_rows = m.nrows() / 4.0;
    start = 0.0;
    for (size_t i = 0; i < 4; ++i) {
      (*output)[i + 4] = nholes_1d(m.row_begin() + size_t(start),
				   m.row_begin() + size_t(start)
				   + size_t(quarter_rows));
      start += quarter_rows;
    }
    return output;
  }

  template<class T>
  FloatVector* area(const T& image) {
    FloatVector* vec = new FloatVector(1);
    (*vec)[0] = feature_t(image.nrows() * image.ncols()) / image.scaling();
    return vec;
  }

  template<class T>
  FloatVector* aspect_ratio(const T& image) {
    FloatVector* vec = new FloatVector(1);
    (*vec)[0] = feature_t(image.ncols()) / feature_t(image.nrows());
    return vec;
  }

  /*
    compactness
    
    compactness is ratio of the volume of the outline of an image to
    the volume of the image.
  */
  template<class T>
  FloatVector* compactness(const T& image) {
    // I've converted this to a more efficient method.  Rather than
    // using (volume(outline) / volume(original)), I just use
    // volume(dilated) - volume(original) / volume(original).  This
    // prevents the unnecessary xor_image pixel-by-pixel operation from
    // happening.  We still need to create a copy to dilate, however,
    // since we don't want to change the original.
    feature_t vol = volume0(image);
    feature_t result;
    if (vol == 0)
      result = std::numeric_limits<feature_t>::max();
    else {
      typedef typename ImageFactory<T>::view_type* view_type;
      view_type copy = simple_image_copy(image);
      dilate(*copy);
      result = (volume0(*copy) - vol) / vol;
      delete copy->data();
      delete copy;
    }
    FloatVector* vec = new FloatVector(1);
    (*vec)[0] = result;
    return vec;
  }

  /*
    volume16regions

    This function divides the image into 16 regions and takes the volume of
    each of those regions.
  */
  template<class T>
  FloatVector* volume16regions(const T& image) {
    double rows = image.height() / 4.0;
    double cols = image.width() / 4.0;
    size_t rows_int = size_t(rows + 1);
    size_t cols_int = size_t(cols + 1);
    double start_col = double(image.offset_x());
    FloatVector* volumes = new FloatVector(16);
    for (size_t i = 0; i < 4; ++i) {
      double start_row = double(image.offset_y());
      for (size_t j = 0; j < 4; ++j) {
	T tmp(image, size_t(start_row), size_t(start_col),
	      rows_int, cols_int);
	(*volumes)[i * 4 + j] = volume0(tmp);
	start_row += rows;
      }
      start_col += cols;
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
    double rows = image.height() / 8.0;
    double cols = image.width() / 8.0;
    size_t rows_int = size_t(rows + 1);
    size_t cols_int = size_t(cols + 1);
    double start_col = double(image.offset_x());
    FloatVector* volumes = new FloatVector(64);
    for (size_t i = 0; i < 8; ++i) {
      double start_row = double(image.offset_y());
      for (size_t j = 0; j < 8; ++j) {
	T tmp(image, size_t(start_row), size_t(start_col),
	      rows_int, cols_int);
	(*volumes)[i * 8 + j] = volume0(tmp);
	start_row += rows;
      }
      start_col += cols;
    }
    return volumes;
  }

  double zer_pol_R(int n, int m_in, double x, double y) {
    int m = abs(m_in);
    int sign;
    double result = 0;
    double distance = (x * x + y * y);
      sign = 1;
    int a = 1;
    for (int i = 2; i <= n; ++i)
      a *= i;
    int b = 1;
    int c = 1;
    for (int i = 2; i <= (n + m) / 2; ++i)
      c *= i;
    int d = c;
    int s = 0; // Outside the loop, since we need to access it at the end too.
    for (; s < (n - m) / 2; ++s) {
      result += sign * (a * 1.0 / (b * c * d)) * pow(distance, (n / 2.0) - s);
      sign = -sign;
      a /= (n - s);
      b *= (s + 1);
      c /= ((n + m) / 2 - s);
      d /= ((n - m) / 2 - s);
    }
    result += sign * (a * 1.0 / (b * c * d)) * pow(distance, (n / 2.0) - s);
    return result;
  }

  void zer_pol(int n, int m, double x, double y, double& real, double& imag) {
    if ((x*x + y*y) > 1.0) {
      real = 0.0;
      imag = 0.0; 
    } else {
      double R = zer_pol_R(n, m, x, y);
      double arg = m * atan2(y, x);
      real = R * cos(arg);
      imag = R * sin(arg);
    }
  }

  template<class T>
  FloatVector* zernike_moments(const T& image) {
    // This is my own modification so that all pixels in the image become 
    // involved in the calculation.  I "imagine" a zernike circle that encompasses the
    // entire bounding box rectangle.  (Rather than an ellipse that fits
    // inside the bounding box.)  This not only helps to better distinguish
    // glyphs, it is slightly more efficient since it removes an 'if' in the
    // inner loop.
    size_t max_dimension = std::max(image.ncols(), image.nrows());
    double x_0 = (image.ncols() + 1) / 2.0;
    double y_0 = (image.nrows() + 1) / 2.0;
    double scale = max_dimension / 2.0;
    double x_dist, y_dist, real_tmp, imag_tmp;

    int m = 1;

    FloatVector* moments = new FloatVector(26);
    for (size_t i = 0; i < 26; ++i)
      (*moments)[i] = 0.0;

    typename T::const_vec_iterator it = image.vec_begin();
    for (size_t y = 0; y < image.nrows(); ++y)
      for (size_t x = 0; x < image.ncols(); ++x, ++it) {
	y_dist = (y - y_0) / scale;
	x_dist = (x - x_0) / scale;
	if (is_black(*it)) {
	  for (size_t n = 1; n < 14; ++n) { 
	    size_t idx = (n - 1) * 2;
	    zer_pol(n, m, x_dist, y_dist, real_tmp, imag_tmp);
	    (*moments)[idx] += real_tmp;
	    (*moments)[idx + 1] += (-imag_tmp);
	  }
	}
      }
    
    for (size_t n = 1; n < 14; ++n) {
      size_t idx = (n-1) * 2;
      double multiplier = (n + 1) / M_PI;
      (*moments)[idx] *= multiplier;
      (*moments)[idx + 1] *= multiplier;
    }

    return moments;
  }

  template<class T>
  FloatVector* skeleton_features(const T& image) {
    if (image.nrows() == 1 || image.ncols() == 1) {
      FloatVector* features = new FloatVector(6, 0.0);
      std::fill(features->begin() + 3, features->end(), 1.0);
      return features;
    }

    typedef typename ImageFactory<T>::view_type* view_type;
    view_type skel = thin_lc(image);
    unsigned char p;
    size_t T_joints = 0, X_joints = 0, bend_points = 0;
    size_t end_points = 0, total_pixels = 0;
    size_t center_x = 0, center_y = 0;
    for (size_t y = 0; y < skel->nrows(); ++y) {
      for (size_t x = 0; x < skel->ncols(); ++x) {
	if (is_black(skel->get(y, x))) {
	  ++total_pixels;
	  center_x += x;
	  center_y += y;
	  size_t N, S;
	  thin_zs_get(y, x, *skel, p, N, S);
	  switch (N) {
	  case 4:
	    ++X_joints;
	    break;
	  case 3:
	    ++T_joints;
	    break;
	  case 2:
	    if (!(((p & 17) == 17) || // Crosswise pairs
		  ((p & 34) == 34) ||
		  ((p & 68) == 68) ||
		  ((p & 136) == 136))) 
	      ++bend_points;
	    break;
	  case 1:
	    ++end_points;
	    break;
	  }
	}
      }
    }
    if (total_pixels == 0) {
      FloatVector* features = new FloatVector(6, 0.0);
      return features;
    }

    center_x /= total_pixels;
    size_t x_axis_crossings = 0;
    bool last_pixel = false;
    for (size_t y = 0; y < skel->nrows(); ++y)
      if (is_black(skel->get(y, center_x)) && !last_pixel) {
	last_pixel = true;
	++x_axis_crossings;
      } else 
	last_pixel = false;
  
    center_y /= total_pixels;
    size_t y_axis_crossings = 0;
    last_pixel = false;
    for (size_t x = 0; x < skel->ncols(); ++x)
      if (is_black(skel->get(center_y, x)) && !last_pixel) {
	last_pixel = true;
	++y_axis_crossings;
      } else 
	last_pixel = false;
    
    FloatVector* features = new FloatVector(6);
    (*features)[0] = float(X_joints);
    (*features)[1] = float(T_joints);
    (*features)[2] = float(bend_points) / float(total_pixels);
    (*features)[3] = float(end_points);
    (*features)[4] = float(x_axis_crossings);
    (*features)[5] = float(y_axis_crossings);
    return features;
  }
}
#endif
