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
#include "gameramodule.hpp"
#include "gamera_limits.hpp"
#include "vigra/resizeimage.hxx"
#include "plugins/logical.hpp"
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
    if ((src.nrows() != dest.nrows()) | (src.ncols() != dest.ncols()))
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
	dest_acc.set((typename U::value_type)src_acc.get(src_col), dest_col);
    image_copy_attributes(src, dest);
  }

  /*
    simple_image_copy

    This functions creates a new image of the same pixel type and storage format
    as the source image. If the image is a ConnectedComponent a OneBitImageView is
    returned rather that a ConnectedComponent (which is why the ImageFactory is used).
  */
  template<class T>
  typename ImageFactory<T>::view_type* simple_image_copy(const T& src) {
    typename ImageFactory<T>::data_type* dest_data =
      new typename ImageFactory<T>::data_type(src.size(), src.offset_y(), src.offset_x());
    typename ImageFactory<T>::view_type* dest =
      new typename ImageFactory<T>::view_type(*dest_data, src);
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


  /*
    union_images

    This function creates a new image that is the summation of all of the images
    in the passed-in list.
  */
  Image *union_images(std::vector<Image*> &list_of_images) {
    // TODO: get a proper maxint
    size_t min_x, min_y, max_x, max_y;
    min_x = min_y = std::numeric_limits<size_t>::max();
    max_x = max_y = 0;

    // Determine bounding box
    for (std::vector<Image*>::iterator i = list_of_images.begin();
	 i != list_of_images.end(); ++i) {
      Image* image = (*i);
      min_x = std::min(min_x, image->ul_x());
      min_y = std::min(min_y, image->ul_y());
      max_x = std::max(max_x, image->lr_x());
      max_y = std::max(max_y, image->lr_y()); 
    }

    size_t ncols = max_x - min_x + 1; 
    size_t nrows = max_y - min_y + 1;
    OneBitImageData *dest_data = new OneBitImageData(nrows, ncols, min_y, min_x);
    OneBitImageView *dest = new OneBitImageView(*dest_data, min_y, min_x, nrows, ncols);
    std::fill(dest->vec_begin(), dest->vec_end(), white(*dest));
    
    for (std::vector<Image*>::iterator i = list_of_images.begin();
	 i != list_of_images.end(); ++i) {
      Image* image = *i;
      OneBitImageView* tmp = new OneBitImageView(*dest_data,
						 image->ul_y(), image->ul_x(),
						 image->nrows(), image->ncols());
      Cc* cc_image = static_cast<Cc*>(image);
      if (cc_image) {
	or_image(*tmp, *cc_image);
      } else {
	RleCc* cc_rle_image = static_cast<RleCc*>(image);
	if (cc_rle_image) {
	  or_image(*tmp, *cc_rle_image);
	} else {
	  OneBitImageView* one_bit_image = static_cast<OneBitImageView*>(image);
	  if (one_bit_image) {
	    or_image(*tmp, *one_bit_image);
	  } else {
	    OneBitRleImageView* one_bit_rle_image = static_cast<OneBitRleImageView*>(image);
	    if (one_bit_rle_image) {
	      or_image(*tmp, *one_bit_rle_image);
	    }
	  }
	}
      }
    }
    return dest;
  }
  
  template<class T>
  Image* resize(T& image, int nrows, int ncols, int resize_quality) {
    typename T::data_type* data = new typename T::data_type(nrows, ncols);
    ImageView<typename T::data_type>* view = 
      new ImageView<typename T::data_type>(*data, 0, 0, nrows ,ncols);
    /*
      Images with nrows or ncols == 1 cannot be scaled. This is a hack that
      just returns an image with the same color as the upper-left pixel
    */
    if (image.nrows() <= 1 || image.ncols() <= 1 || 
	view->nrows() <= 1 || view->ncols() <= 1) {
      std::fill(view->vec_begin(), view->vec_end(), image.get(0, 0));
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
  Image* scale(T& image, double scaling, int resize_quality) {
    // nrows, ncols are cast to a double so that the multiplication happens
    // exactly as it does in Python
    return resize(image, 
		  size_t(ceil(double(image.nrows()) * scaling)),
		  size_t(ceil(double(image.ncols()) * scaling)), 
		  resize_quality);
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

  /*
    Fill an image with white.
  */
  template<class T>
  void fill_white(T& image) {
    std::fill(image.vec_begin(), image.vec_end(), white(image));
  }

  /* Invert an image */
  template<class Pixel>
  struct invert_specialized {
    template<class T>
    void operator()(T& image) {
      ImageAccessor<typename T::value_type> acc;
      typename T::vec_iterator in = image.vec_begin();
      for (; in != image.vec_end(); ++in)
	acc.set(invert(acc(in)), in);
    }
  };

  template<>
  struct invert_specialized<FloatPixel> {
    template<class T>
    void operator()(T& image) {
      FloatPixel max = 0;
      max = find_max(image.parent());
      ImageAccessor<FloatPixel> acc;
      typename T::vec_iterator in = image.vec_begin();
      for (; in != image.vec_end(); ++in)
	acc.set(max - acc(in), in);
    }
  };

  template<class T>
  void invert(T& image) {
    invert_specialized<typename T::value_type> invert_special;
    invert_special(image);
  }

  /*
    Shearing
  */

  template<class T>
  inline void simple_shear(T begin, const T end, int distance) {
    // short-circuit
    if (distance == 0)
      return;
    typename T::value_type filler;
    // move down or right
    if (distance > 0) {
      filler = *begin;
      std::copy_backward(begin + distance, end - distance, end);
      std::fill(begin, begin + distance, filler);
      // move up or left
    } else {
      filler = *(end - 1);
      std::copy(begin - distance, end, begin);
      std::fill(end + distance, end, filler);
    }
  }

  template<class T>
  void shear_column(T& mat, size_t column, int distance) {
    if (size_t(std::abs(distance)) >= mat.nrows())
      throw std::range_error("Tried to shear column too far");
    if (column >= mat.ncols())
      throw std::range_error("Column argument to shear_column out of range");
    simple_shear((mat.col_begin() + column).begin(),
		 (mat.col_begin() + column).end(),distance);
  }

  template<class T>
  void shear_row(T& mat, size_t row, int distance) {
    if (size_t(std::abs(distance)) >= mat.ncols())
      throw std::range_error("Tried to shear column too far");
    if (row >= mat.nrows())
      throw std::range_error("Column argument to shear_column out of range");
    simple_shear((mat.row_begin() + row).begin(),
		 (mat.row_begin() + row).end(), distance);
  }

  template<class T>
  Image *clip_image(T& m, const Rect& rect) {
    if (m.intersects(rect)) {
      size_t ul_y = std::max(m.ul_y(), rect.ul_y());
      size_t ul_x = std::max(m.ul_x(), rect.ul_x());
      size_t lr_y = std::min(m.lr_y(), rect.lr_y());
      size_t lr_x = std::min(m.lr_x(), rect.lr_x());
      return new T(m, ul_y, ul_x, lr_y - ul_y, lr_x - ul_x + 1);
    } else {
      return new T(m, m.ul_y(), m.ul_x(), 1, 1);
    };
  }

  template<class T, class U>
  typename ImageFactory<T>::view_type* mask(const T& a, U &b) {
    typename ImageFactory<T>::data_type* dest_data =
      new typename ImageFactory<T>::data_type(b.size(), b.offset_y(), b.offset_x());
    typename ImageFactory<T>::view_type* dest =
      new typename ImageFactory<T>::view_type(*dest_data, b);

    typename ImageFactory<T>::view_type a_view =
      typename ImageFactory<T>::view_type(a, b.ul(), b.size());

    ImageAccessor<typename T::value_type> a_accessor;
    ImageAccessor<typename U::value_type> b_accessor;

    typename T::vec_iterator it_a, end;
    typename U::vec_iterator it_b;
    typename T::vec_iterator it_dest;

    for (it_a = a_view.vec_begin(), end = a_view.vec_end(), it_b = b.vec_begin(), it_dest = dest->vec_begin();
	 it_a != end; ++it_a, ++it_b, ++it_dest) {
      if (is_black(b_accessor.get(it_b)))
	a_accessor.set(a_accessor.get(it_a), it_dest);
      else
	a_accessor.set(white(*dest), it_dest);
    }
    return dest;
  }
}
#endif
