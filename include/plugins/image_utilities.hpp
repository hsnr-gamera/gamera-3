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

#ifndef kwm12032001_image_utilities
#define kwm12032001_image_utilities

#include "gamera.hpp"
#include "gamera_limits.hpp"
#include "vigra/resizeimage.hxx"
#include <exception>
#include <math.h>


namespace Gamera {

template<class T>
Image* image_copy(T &a, int storage_format) {
  if (storage_format == DENSE) {
    typename ImageFactory<T>::dense_data_type* data =
      new typename ImageFactory<T>::dense_data_type(a.nrows(), a.ncols(), a.offset_y(), a.offset_x());
    typename ImageFactory<T>::dense_view_type* view =
      new typename ImageFactory<T>::dense_view_type(*data, a.offset_y(), a.offset_x(), a.nrows(), a.ncols());
    typename T::col_iterator a_col;
    typename T::row_iterator a_row;
    typename ImageFactory<T>::dense_view_type::col_iterator b_col;
    typename ImageFactory<T>::dense_view_type::row_iterator b_row;
    ImageAccessor<typename T::value_type> acc;
    for (a_row = a.row_begin(), b_row = view->row_begin(); a_row != a.row_end(); ++a_row, ++b_row)
      for (a_col = a_row.begin(), b_col = b_row.begin(); a_col != a_row.end(); ++a_col, ++b_col)
	acc.set(acc.get(a_col), b_col);
    return view;
  } else {
    typename ImageFactory<T>::rle_data_type* data =
      new typename ImageFactory<T>::rle_data_type(a.nrows(), a.ncols(), a.offset_y(), a.offset_x());
    typename ImageFactory<T>::rle_view_type* view =
      new typename ImageFactory<T>::rle_view_type(*data, a.offset_y(), a.offset_x(), a.nrows(), a.ncols());
    typename T::col_iterator a_col;
    typename T::row_iterator a_row;
    typename ImageFactory<T>::rle_view_type::col_iterator b_col;
    typename ImageFactory<T>::rle_view_type::row_iterator b_row;
    ImageAccessor<typename T::value_type> acc;
    for (a_row = a.row_begin(), b_row = view->row_begin(); a_row != a.row_end(); ++a_row, ++b_row)
      for (a_col = a_row.begin(), b_col = b_row.begin(); a_col != a_row.end(); ++a_col, ++b_col)
	acc.set(acc.get(a_col), b_col);
    return view;
  }
}

template<class T>
Image* rotate_copy(T &m, float hypot) {
  typedef ImageFactory<T> fact;
  typename fact::data_type* out_data;
  typename fact::view_type* out;
  size_t out_nrows, out_ncols;
  if (hypot == 90.0 || hypot == -270.0 || hypot == -90) {
    out_nrows = m.ncols();
    out_ncols = m.nrows();
  } else if (hypot == 180 || hypot == -180) {
    out_nrows = m.nrows();
    out_ncols = m.ncols();
  } else {
    out_nrows = out_ncols = m.nrows() + m.ncols();
  }
  out_data = new typename fact::data_type(out_nrows, out_ncols);
  out = new typename fact::view_type(*out_data, 0, 0, out_nrows, out_ncols);
  double angle = atan(hypot);
  double sin_angle = sin(angle);
  double cos_angle = cos(angle);
  int half_row = m.nrows() / 2;
  int half_col = m.ncols() / 2;
  int half_out_row = out->nrows() / 2;
  int half_out_col = out->ncols() / 2;
  for (int row = 0; row < int(m.nrows()); row++) {
    for (int col = 0; col < int(m.ncols()); col++) {
      double new_row = ((double(col - half_col) * sin_angle) + 
			(double(row - half_row) * cos_angle) +
			double(half_out_row));
      double new_col = ((double(col - half_col) * cos_angle) - 
			(double(row - half_row) * sin_angle) +
			double(half_out_col));
      out->set(int(new_row + 0.5), int(new_col + 0.5), typename fact::view_type::value_type(m.get(row, col)));
    }
  }
  return out;
}

template<class T>
Image* resize_copy(T& image, int nrows, int ncols, int resize_quality) {
  typename T::data_type* data = new typename T::data_type(nrows, ncols);
  ImageView<typename T::data_type>* view = 
    new ImageView<typename T::data_type>(*data, 0, 0, nrows ,ncols);
  if (resize_quality == 0) {
    resizeImageNoInterpolation(src_image_range(image), dest_image_range(*view));
  } else if (resize_quality == 1) {
    resizeImageLinearInterpolation(src_image_range(image), dest_image_range(*view));
  } else {
    resizeImageSplineInterpolation(src_image_range(image), dest_image_range(*view));
  }
  return view;
}

template<class T>
Image* scale_copy(T& image, double scaling, int resize_quality) {
  return resize_copy(image, size_t(image.nrows() * scaling),
		     size_t(image.ncols() * scaling), resize_quality);
}


/*
  FloatVector histogram(GreyScale|Grey16 image);

  Histogram returns a histogram of the values in an image. The
  values in the histogram are percentages.
*/
template<class T>
FloatVector* histogram(const T& image) {
  // The histogram is the size of all of the possible values of
  // the pixel type.
  size_t l = std::numeric_limits<typename T::value_type>::max() + 1;
  FloatVector* values = new FloatVector(l);

  // set the list to 0
  std::fill(values->begin(), values->end(), 0);

  typename T::const_row_iterator row = image.row_begin();
  typename T::const_col_iterator col;
  ImageAccessor<typename T::value_type> acc;

  // create the histogram
  for (; row != image.row_end(); ++row)
    for (col = row.begin(); col != row.end(); ++col)
      (*values)[acc.get(col)]++;

  // convert from absolute values to percentages
  double size = image.nrows() * image.ncols();
  for (size_t i = 0; i < l; i++) {
    (*values)[i] = (*values)[i] / size;
  }
  return values;
}

}
#endif
