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
#include <algorithm>


namespace Gamera {
  
  /*
    This copies all of the misc attributes of an image (like
    label for Ccs or scaling).
  */
  template<class T, class U>
  void image_copy_attributes(const T& src, U& dest) {
    dest.scaling(src.scaling());
    dest.resolution(src.resolution());
  }

  /*
    These are full specializations for ConnectedComponents. This
    could be done with partial specialization, but that is broken
    on so many compilers it is easier just to do it manually :/
  */
  template<>
  void image_copy_attributes(const Cc& src, Cc& dest) {
    dest.scaling(src.scaling());
    dest.resolution(src.resolution());
    dest.label(src.label());
  }

  template<>
  void image_copy_attributes(const RleCc& src, Cc& dest) {
    dest.scaling(src.scaling());
    dest.resolution(src.resolution());
    dest.label(src.label());
  }

  template<>
  void image_copy_attributes(const Cc& src, RleCc& dest) {
    dest.scaling(src.scaling());
    dest.resolution(src.resolution());
    dest.label(src.label());
  }

  template<>
  void image_copy_attributes(const RleCc& src, RleCc& dest) {
    dest.scaling(src.scaling());
    dest.resolution(src.resolution());
    dest.label(src.label());
  }


  /*
    image_copy_fill

    This function copies the contents from one image to another of the same
    size. Presumably the pixel types of the two images are the same, but
    a cast is performed allowing any two pixels types with the approprate
    conversion functions defined (or built-in types) to be copied. The storage
    formats of the image do not need to match.
   */
  template<class T, class U>
  void image_copy_fill(const T& src, U& dest) {
    if (src.nrows() != dest.nrows() | src.ncols() != dest.ncols())
      throw std::range_error("image_copy_fill: src and dest image dimensions must match!");
    typename T::const_row_iterator src_row = src.row_begin();
    typename T::const_col_iterator src_col;
    typename U::row_iterator dest_row = dest.row_begin();
    typename U::col_iterator dest_col;
    ImageAccessor<typename T::value_type> src_acc;
    ImageAccessor<typename U::value_type> dest_acc;
    for (; src_row != src.row_end(); ++src_row, ++dest_row)
      for (src_col = src_row.begin(), dest_col = dest_row.begin(); src_col != src_row.end();
	   ++src_col, ++dest_col)
	dest_acc.set(typename U::value_type(src_acc.get(src_col)), dest_col);
    image_copy_attributes(src, dest);
  }

  /*
    simple_image_copy

    This functions creates a new image of the same pixel type and storage format
    as the source image. If the image is a ConnectedComponent a OneBitImageView is
    returned rather that a ConnectedComponent (which is why the ImageFactory is used).
  */
  template<class T>
  typename ImageFactory<T>::view_type simple_image_copy(const T& src) {
    typename ImageFactory<T>::data_type* dest_data =
      new typename ImageFactory<T>::data_type(src.size(), src.offset_y(), src.offset_x());
    typename ImageFactory<T>::view_type* dest =
      new typename ImageFactory<T>::view_type(*data, src);
    image_copy_fill(src, *dest);
    return dest;
  }

  /*
    image_copy

    This function creates a new image with the specified storage_format and
    copies the contents from the provided image. If the image is a ConnectedComponent a
    OneBit*ImageView is returned rather that a ConnectedComponent (which is why the
    ImageFactory is used).
  */
  template<class T>
  Image* image_copy(T &a, int storage_format) {
    if (storage_format == DENSE) {
      typename ImageFactory<T>::dense_data_type* data =
	new typename ImageFactory<T>::dense_data_type(a.size(), a.offset_y(), a.offset_x());
      typename ImageFactory<T>::dense_view_type* view =
	new typename ImageFactory<T>::dense_view_type(*data, a);
      image_copy_fill(a, *view);
      return view;
    } else {
      typename ImageFactory<T>::rle_data_type* data =
	new typename ImageFactory<T>::rle_data_type(a.size(), a.offset_y(), a.offset_x());
      typename ImageFactory<T>::rle_view_type* view =
	new typename ImageFactory<T>::rle_view_type(*data, a);
      image_copy_fill(a, *view);
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
    image_copy_attributes(m, *out);
    return out;
  }

  template<class T>
  Image* resize_copy(T& image, int nrows, int ncols, int resize_quality) {
    typename T::data_type* data = new typename T::data_type(nrows, ncols);
    ImageView<typename T::data_type>* view = 
      new ImageView<typename T::data_type>(*data, 0, 0, nrows ,ncols);
    /*
      Images with nrows or ncols == 1 cannot be scaled. This is a hack that
      just returns a black image.
    */
    if (view->nrows() == 1 || view->ncols() == 1) {
      std::fill(view->vec_begin(), view->vec_end(), black(*view));
      return view;
    }
    if (resize_quality == 0) {
      resizeImageNoInterpolation(src_image_range(image), dest_image_range(*view));
    } else if (resize_quality == 1) {
      resizeImageLinearInterpolation(src_image_range(image), dest_image_range(*view));
    } else {
      resizeImageSplineInterpolation(src_image_range(image), dest_image_range(*view));
    }
    image_copy_attributes(image, *view);
    return view;
  }

  template<class T>
  Image* scale_copy(T& image, double scaling, int resize_quality) {
    std::cout.flush();
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

  /*
    Find the maximum pixel value for an image
  */
  template<class T>
  typename T::value_type find_max(const T& image) {
    if (image.nrows() == 0 || image.ncols() == 0)
      throw std::range_error("Image must have nrows and ncols > 0.");
    typename T::const_row_iterator row = image.row_begin();
    typename T::const_col_iterator col;
    typename T::value_type tmp, max;
    ImageAccessor<typename T::value_type> acc;
    
    max = acc.get(row); 
    for ( ; row != image.row_end(); ++row) {
      for (col = row.begin(); col != row.end(); ++col) {
	tmp = acc.get(col);
	if (tmp > max)
	  max = tmp;
      }
    }
    return max;
  }

}
#endif
