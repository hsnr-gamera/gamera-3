/*
 *
 * Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, Karl MacMillan
 *               2009-2010 Christoph Dalitz
 *               2010      Robert Butz
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef kwm10242002_features
#define kwm10242002_features

#include "gamera.hpp"
#include "image_utilities.hpp"
#include "morphology.hpp"
#include "thinning.hpp"
#include "plugins/projections.hpp"
#include "plugins/transformation.hpp"
#include <cmath>
#include <vector>

namespace Gamera {
  //
  // Black Area
  //
  // Find the number of black pixels in an image. This is just a convenience
  // routine, but it demonstrates the correct method for determining whether
  // a pixel is black or white.
  //
  template<class T>
  void black_area(const T& mat, feature_t* buf) {
    *buf = 0;
    for (typename T::const_vec_iterator i = mat.vec_begin();
         i != mat.vec_end(); ++i) {
      if (is_black(*i))
        (*buf)++;
    }
  }
  // Old-style version, since it is called from C++ elsewhere
  template<class T>
  feature_t black_area(const T& mat) {
    int black_pixels = 0;
    for (typename T::const_vec_iterator i = mat.vec_begin();
         i != mat.vec_end(); ++i) {
      if (is_black(*i))
        black_pixels++;
    }
    return (feature_t)black_pixels;
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
  
  template<class T>
  void volume(const T &m, feature_t* buf) {
    *buf = volume(m);
  }
  

  //
  // Normalized Central Moments 
  //
  // Returns vector containing:
  // u10, u01, u20, u02, u11, u30, u12, u21, u03
  //

  template<class Iterator>
  void moments_1d(Iterator begin, Iterator end, feature_t& m0, feature_t& m1,
                  feature_t& m2, feature_t& m3) {
    // zeroeth, first, second, third order on one axis
    feature_t tmp = 0;
    size_t x = 0;
    Iterator itx = begin;
    for (; itx != end; ++itx, x++) {
      size_t y = 0, proj = 0;
      typename Iterator::iterator ity = itx.begin();
      for (; ity != itx.end(); ++ity, ++y)
        if (is_black(*ity))
          proj++;
      m0 += proj;
      m1 += (tmp = x * proj);
      m2 += (tmp *= x);
      m3 += (tmp * x);
    }
  }

  template<class Iterator>
  void moments_2d(Iterator begin, Iterator end, feature_t& m11, feature_t& m12,
                  feature_t& m21) {
    feature_t tmp = 0;
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
  void moments(T &m, feature_t* buf) {
    feature_t m10 = 0, m11 = 0, m20 = 0, m21 = 0, m12 = 0, 
      m01 = 0, m02 = 0, m30 = 0, m03 = 0, m00 = 0, dummy = 0;
    moments_1d(m.row_begin(), m.row_end(), m00, m01, m02, m03);
    moments_1d(m.col_begin(), m.col_end(), dummy, m10, m20, m30);
    moments_2d(m.col_begin(), m.col_end(), m11, m12, m21);

    feature_t x, y, x2, y2, div;
    x = (feature_t)m10 / m00;
    x2 = 2 * x * x;
    y = (feature_t)m01 / m00;
    y2 = 2 * y * y;

    // normalized center of gravity [0,1]
    if (m.ncols() > 1)
      *(buf++) = x / (m.ncols()-1);
    else
      *(buf++) = 0.5; // only one pixel wide
    if (m.nrows() > 1)
      *(buf++) = y / (m.nrows()-1);
    else
      *(buf++) = 0.5; // only one pixel high
  
    div = (feature_t)m00 * m00; // common normalization divisor 
    *(buf++) = (m20 - (x * m10)) / div; // u20 
    *(buf++) = (m02 - (y * m01)) / div; // u02 
    *(buf++) = (m11 - (y * m10)) / div; // u11 
  
    div *= (feature_t)sqrt((double)m00);  
    *(buf++) = (m30 - (3 * x * m20) + (x2 * m10)) / div;                // u30
    *(buf++) = (m12 - (2 * y * m11) - (x * m02) + (y2 * m10)) / div;    // u12
    *(buf++) = (m21 - (2 * x * m11) - (y * m20) + (x2 * m01)) / div;    // u21
    *buf = (m03 - (3 * y * m02) + (y2 * m01)) / div;                // u03
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
  inline int nholes_1d(Iterator begin, Iterator end) {

    int hole_count = 0;
    bool last;
    bool has_black;
    Iterator r = begin;
    for (; r != end; r++) {
      last = false;
      has_black = false;
      typename Iterator::iterator c = r.begin();
      for (; c != r.end(); c++) {
        if (is_black(*c)){
          last = true;
          has_black = true;
        }
        else if (last) {
          last = false;
          hole_count++;
        }
      }
      if (!last && hole_count && has_black){ 
        hole_count--;
      }
    }
    return hole_count;
  }
  
  template<class T>
  void nholes(T &m, feature_t* buf) {
    int vert, horiz;
    
    vert = nholes_1d(m.col_begin(), m.col_end());
    horiz = nholes_1d(m.row_begin(), m.row_end());
    
    *(buf++) = (feature_t)vert / m.ncols();
    *buf = (feature_t)horiz / m.nrows();
  }

  //
  // nholes_extended
  //
  // This divides the image into strips (both horizontally
  // and vertically) and computes the number of holes on each strip.
  //
  template<class T>
  void nholes_extended(const T& m, feature_t* buf) {
    double quarter_cols = m.ncols() / 4.0;
    double start = 0.0;
    for (size_t i = 0; i < 4; ++i) {
      *(buf++) = nholes_1d(m.col_begin() + size_t(start),
			    m.col_begin() + size_t(start) + size_t(quarter_cols))
                /quarter_cols;
      start += quarter_cols;
    }
    double quarter_rows = m.nrows() / 4.0;
    start = 0.0;
    for (size_t i = 0; i < 4; ++i) {
      *(buf++) = nholes_1d(m.row_begin() + size_t(start),
			   m.row_begin() + size_t(start) + size_t(quarter_rows))
               / quarter_rows;
      start += quarter_rows;
    }
  }

  template<class T>
  void area(const T& image, feature_t* buf) {
    *buf = feature_t(image.nrows() * image.ncols()) / image.scaling();
  }

  template<class T>
  void aspect_ratio(const T& image, feature_t* buf) {
    *buf = feature_t(image.ncols()) / feature_t(image.nrows());
  }

  template<class T>
  void nrows_feature(const T& image, feature_t* buf) {
    *buf = feature_t(image.nrows());
  }

  template<class T>
  void ncols_feature(const T& image, feature_t* buf) {
    *buf = feature_t(image.ncols());
  }


  // helper function for compactness which computes the surface
  // of pixels lying on the image bounding box that are ignored
  // by a dilation
  template<class T>
  feature_t compactness_border_outer_volume(const T &m) {
    int lastPx = 0;
    int i, rows, cols;
    int startPx = 0;
    feature_t num_dil_px = 0;
    rows = m.nrows();
    cols = m.ncols();

    startPx = m.get(Point(0,0)); // needed for the last pxs

    // 1st row left to right, including both corners
    for (i=0; i<cols; i++) {
      if (is_black(m.get(Point(i,0)))) {
        if (lastPx == 2)       num_dil_px += 1;
        else if (lastPx == 1)  num_dil_px += 2;
        else                   num_dil_px += 3;
        if (i == 0 || i== rows-1) // upper left or upper right corner
          num_dil_px += 2;
        lastPx = 2;
      } else { // px is white
        lastPx -=1;
        if (i== rows-1) // upper right corner
          lastPx = 0;
      }
    }
    // last column top-> down, including lower right corner
    for (i=1; i<rows; i++) {
      if (is_black( m.get(Point(cols-1,i)))) {
        if (lastPx == 2)       num_dil_px += 1;
        else if (lastPx == 1)  num_dil_px += 2;
        else                   num_dil_px += 3;
        if (i== rows-1)// lower right corner
          num_dil_px += 2;
        lastPx = 2;
      } else { // px is white
        lastPx -=1;
        if (i== rows-1)// lower right corner
          lastPx = 0;
      }
    }
    // last row right to left, including lower left corner
    for (i=cols-2; i>=0; i--) {
      if (is_black(m.get(Point(i,rows-1)))) {
        if (lastPx == 2)       num_dil_px += 1;
        else if (lastPx == 1)  num_dil_px += 2;
        else                   num_dil_px += 3;
        if (i== 0)// lower left corner
          num_dil_px += 2;
        lastPx = 2;
      }
      else{ // px is white
        lastPx -=1;
        if (i== 0) // lower left corner
          lastPx = 0;
      }
    }
    // 1st column down->top, no corners included
    for (i=rows-2; i>0; i--) {
      if (is_black(m.get(Point(0,i)))) {
        if (lastPx == 2)       num_dil_px += 1;
        else if (lastPx == 1)  num_dil_px += 2;
        else                   num_dil_px += 3;
        lastPx = 2;
      } else { // px is white
        lastPx -=1;
      }
    }

    // avoiding overlapping dilated_px: Start-End
    if (is_black(startPx)) {
      if (is_black(m.get(Point(0,1)))) {
        num_dil_px -= 2;
      }
      else if (is_black( m.get(Point(0,2)))) {
        num_dil_px -= 1;
      }
    }
    return num_dil_px / (rows*cols); // volume
  }

  //
  // compactness
  //
  // compactness is ratio of the volume of the outline of an image to
  // the volume of the image.
  //
  template<class T>
  void compactness(const T& image, feature_t* buf) {
    // I've converted this to a more efficient method.  Rather than
    // using (volume(outline) / volume(original)), I just use
    // volume(dilated) - volume(original) / volume(original).  This
    // prevents the unnecessary xor_image pixel-by-pixel operation from
    // happening.  We still need to create a copy to dilate, however,
    // since we don't want to change the original.
    // as dilate does not extend beyond the image borders,
    // we must compute the surface of the border pixels separately
    feature_t vol = volume(image);
    feature_t outer_vol = compactness_border_outer_volume(image);
    feature_t result;
    if (vol == 0)
      result = std::numeric_limits<feature_t>::max();
    else {
      typedef typename ImageFactory<T>::view_type* view_type;
      view_type copy = erode_dilate(image, 1, 0, 0);
      // dilate(*copy);
      result = (volume(*copy) + outer_vol - vol) / vol;
      delete copy->data();
      delete copy;
    }
    *buf = result;
  }

  //
  // volume16regions
  //
  // This function divides the image into 16 regions and takes the volume of
  // each of those regions.
  //
  template<class T>
  void volume16regions(const T& image, feature_t* buf) {
    double rows = image.nrows() / 4.0;
    double cols = image.ncols() / 4.0;
    size_t rows_int = size_t(rows);
    size_t cols_int = size_t(cols);
    Dim size(cols_int, rows_int);
    if (size.ncols() == 0)
        size.ncols(1);
    if (size.nrows() == 0)
        size.nrows(1);
    double start_col = double(image.offset_x());
    for (size_t i = 0; i < 4; ++i) {
      double start_row = double(image.offset_y());
      for (size_t j = 0; j < 4; ++j) {
        T tmp(image, Point((size_t)start_col, (size_t)start_row), size);
        *(buf++) = volume(tmp);
        start_row += rows;
        size.nrows( size_t(start_row + rows)-size_t(start_row));
        if (size.nrows() == 0)
            size.nrows(1);
      }
      start_col += cols;
      size.ncols( size_t(start_col + cols) - size_t(start_col));
      if (size.ncols() == 0)
        size.ncols(1);
    }
  }

  //
  // volume64regions
  //
  // This function divides the image into 64 regions and takes the volume of
  // each of those regions.
  //
  template<class T>
  void volume64regions(const T& image, feature_t* buf) {
    double rows = image.nrows() / 8.0;
    double cols = image.ncols() / 8.0;
    size_t rows_int = size_t(rows);
    size_t cols_int = size_t(cols);
    Dim size(cols_int, rows_int);
    if (size.ncols() == 0)
        size.ncols(1);
    if (size.nrows() == 0)
        size.nrows(1);
    double start_col = double(image.offset_x());
    for (size_t i = 0; i < 8; ++i) {
      double start_row = double(image.offset_y());
      for (size_t j = 0; j < 8; ++j) {
        T tmp(image, Point((size_t)start_col, (size_t)start_row), size);
        *(buf++) = volume(tmp);
        start_row += rows;
        size.nrows( size_t(start_row + rows)-size_t(start_row));
        if (size.nrows() == 0)
            size.nrows(1);
      }
      start_col += cols;
      size.ncols( size_t(start_col + cols) - size_t(start_col));
      if (size.ncols() == 0)
            size.ncols(1);
    }
  }


  //
  // Zernike Moments
  //

  double zer_pol_R(int n, int m, double x, double y) {
    // precomputed factorials => make sure that n < 11
    const int fak_a[] = {1,1,2,6,24,120,720,5040,40320,362880,3628800};
    int s,Na,Nb,Nc;
    int sign = 1;
    double result = 0;
    double distance = sqrt(x * x + y * y);
    double Zb = 1;

    for(s=0; s<=(n-m)/2; s++){
      Na = fak_a[n-s] / fak_a[s];
      if (n-2*s == 0)
        Zb = 1;
      else
        Zb *= distance;
      Nb = fak_a[(n+m)/2-s];
      Nc = fak_a[(n-m)/2-s];
      result += sign * (Na * Zb / Nb) / Nc;
      sign = -sign;
    }
    return result;
  }

  void zer_pol(int n, int m, double x, double y, double& real, double& imag, double norm_scale=1.0) {
    const complex<double> I(0.0, 1.0);
    // theoretically redundant due to scaling,
    // but with translation-normalizing after all needed
    if (sqrt(x*x + y*y) > 1.0) {
      //std::cout << "outer px=" << x << "," << y << std::endl;
      real = 0.0;
      imag = 0.0; 
    } else {
      double Rnm = zer_pol_R(n, m, x*norm_scale, y*norm_scale);
      double angle_Theta = atan2(y,x);
      complex<double> Inm = exp(m*angle_Theta*I);
      complex<double> result = conj(Rnm * Inm); // complex conj.
      real = result.real();
      imag = result.imag();
    }
  }

  // we use this wrapper so that it is easy to
  // change the maximum order in the future
  template<class T>
  void zernike_moments(const T& image, feature_t* buf) {
    // beware that, when changing the maximum order from six to
    // something different, the dimension of the feature vector
    // needs to be adjusted in the python interface features.py
    zernike_moments( image, buf, 6);
  }

  template<class T>
  void zernike_moments(const T& image, feature_t* buf, size_t order_n) {
    size_t const max_order_n=order_n; 
    size_t num_features=0; // evaluated by max_order_n
    double x_dist, y_dist, real_tmp, imag_tmp;

    // compute center of mass and normalization factor m00
    feature_t m00=0, m10=0, m01=0, dummy1=0, dummy2=0, dummy3=0;
    moments_1d(image.row_begin(), image.row_end(), m00, m01, dummy1, dummy2);
    moments_1d(image.col_begin(), image.col_end(), dummy1, m10, dummy2, dummy3);
    double centroid_x = m10/m00;
    double centroid_y = m01/m00;

    // we use a Zernike circle that includes the entire image
    // beware however that some pixels can fall outside the circle
    // by normalizing ZMs to be translation invariant, e.g. a large
    // bunch of pixels in the upper left corner which draws the
    // center to it, excludes pixels in the lower right corner.
    size_t max_dimension = std::max(image.ncols(), image.nrows());
    double unit_circle_scale = sqrt(2*pow(max_dimension/2.0,2));

    // number of features depends on maximum order
    for (size_t i=0 ; i<=max_order_n; i++)
      num_features += i/2 + 1;
    num_features -= 2; // A00 and A11 are constants

    feature_t* begin = buf;
    for (size_t i = 0; i < num_features; ++i)
      *(buf++) = 0.0;
    buf = begin;

    size_t m, n, idx;
    typename T::const_vec_iterator it = image.vec_begin();
    for (size_t y = 0; y < image.nrows(); ++y) {
      for (size_t x = 0; x < image.ncols(); ++x, ++it) {
        if (is_black(*it)) {
          x_dist = (x - centroid_x) / unit_circle_scale;
          y_dist = (y - centroid_y) / unit_circle_scale;
          for (n = 2, idx=0; n <= max_order_n; ++n) {
            for (m = n%2; m <= n; m+=2) {
              zer_pol(n, m, x_dist, y_dist, real_tmp, imag_tmp);
              buf[idx++] += sqrt(real_tmp*real_tmp + imag_tmp*imag_tmp);
            }
          }
        }
      }
    }

    // scale normalization by m00
    for (size_t n = 2, idx=0; n <= max_order_n; ++n) {
      double multiplier = (n + 1) / M_PI;
      multiplier /= m00;
      for (m= n%2; m<= n; m+=2){
        buf[idx++] *= multiplier;
      }
    }

  }

  //
  // Skeleton features
  //
  template<class T>
  void skeleton_features(const T& image, feature_t* buf) {
    if (image.nrows() == 1 || image.ncols() == 1) {
      *(buf++) = 0.0;
      *(buf++) = 0.0;
      *(buf++) = 0.0;
      *(buf++) = 3.0;
      *(buf++) = 3.0;
      *buf = 3.0;
      return;
    }

    typedef typename ImageFactory<T>::view_type* view_type;
    view_type skel = thin_lc(image);
    unsigned char p;
    size_t T_joints = 0, X_joints = 0, bend_points = 0;
    size_t end_points = 0, total_pixels = 0;
    size_t center_x = 0, center_y = 0;
    for (size_t y = 0; y < skel->nrows(); ++y) {
      size_t y_before = (y == 0) ? 1 : y - 1;
      size_t y_after = (y == skel->nrows() - 1) ? skel->nrows() - 2 : y + 1;
      for (size_t x = 0; x < skel->ncols(); ++x) {
	if (is_black(skel->get(Point(x, y)))) {
	  ++total_pixels;
	  center_x += x;
	  center_y += y;
	  size_t N, S;
	  thin_zs_get(y, y_before, y_after, x, *skel, p, N, S);
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
      for (size_t i = 0; i < 6; ++i)
	*(buf++) = 0.0;
      return;
    }

    center_x /= total_pixels;
    size_t x_axis_crossings = 0;
    bool last_pixel = false;
    for (size_t y = 0; y < skel->nrows(); ++y)
      if (is_black(skel->get(Point(center_x, y))) && !last_pixel) {
        last_pixel = true;
        ++x_axis_crossings;
      } else {
        last_pixel = false;
      }
  
    center_y /= total_pixels;
    size_t y_axis_crossings = 0;
    last_pixel = false;
    for (size_t x = 0; x < skel->ncols(); ++x) {
      if (is_black(skel->get(Point(x, center_y))) && !last_pixel) {
        last_pixel = true;
        ++y_axis_crossings;
      } else {
        last_pixel = false;
      }
    }
    
    delete skel->data();
    delete skel;

    *(buf++) = feature_t(X_joints);
    *(buf++) = feature_t(T_joints);
    *(buf++) = feature_t(bend_points) / feature_t(total_pixels);
    *(buf++) = feature_t(end_points);
    *(buf++) = feature_t(x_axis_crossings);
    *buf = feature_t(y_axis_crossings);
  }

  //
  // Top Bottom
  //
  template<class T>
  void top_bottom(const T& m, feature_t* buf) {
    int top = -1;
    typename T::const_row_iterator ri = m.row_begin();
    for (int i = 0; ri != m.row_end(); ri++, i++) {
      typename T::const_col_iterator ci = ri.begin();
      for (; ci != ri.end(); ci++)
	if (is_black(*ci)) {
	  top = i;
	  break;
	}
      if (top != -1)
	break;
    }
    if (top == -1) {
      *(buf++) = 1.0;
      *buf = 0.0;
      return;
    }
    int bottom = -1;
    ri = m.row_end();
    --ri;
    for (int i = m.nrows() - 1; ri != m.row_begin(); ri--, i--) {
      typename T::const_col_iterator ci = ri.begin();
      for (; ci != ri.end(); ci++)
	if (is_black(*ci)) {
	  bottom = i;
	  break;
	}
      if (bottom != -1)
	break;
    }
    *(buf++) = feature_t(top) / feature_t(m.nrows());
    *buf = feature_t(bottom) / feature_t(m.nrows());
  }




  template<class T>
  void diagonal_projection(const T& image, feature_t* buf) {
    typedef typename ImageFactory<T>::view_type* view_type;
    view_type rotated_image = rotate(image, 45, 0, 2);

    IntVector *proj_x = projection_cols(*rotated_image);
    IntVector *proj_y = projection_rows(*rotated_image);
    
    int mid_x = (*proj_x).size()/2;
    int mid_y = (*proj_y).size()/2;


    // for (int i = 0 ; i < (*proj_x).size(); i++) 
    //   cout << (*proj_x)[i] << endl; 
    // cout << endl;

    float relation_xy = (float) (*proj_x)[mid_x] / (float) (*proj_y)[mid_y];

    // cout << "mid_x: " << (*proj_x)[mid_x] << endl;
    // cout << "mid_y: " << (*proj_y)[mid_y] << endl;
    // cout << "relation_xy: " << relation_xy << endl;


    *buf = relation_xy;



    delete proj_x;
    delete proj_y;

    delete rotated_image;

  }





}
#endif
