/*
 *
 * Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, Karl MacMillan
 *               2010-2012 Christoph Dalitz
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

#ifndef cd03022010_transformation
#define cd03022010_transformation

#include "gamera.hpp"
#include "gameramodule.hpp"
#include "gamera_limits.hpp"
#include "vigra/resizeimage.hxx"
#include "vigra/basicgeometry.hxx"
#include "vigra/affinegeometry.hxx"
#include "plugins/logical.hpp"
#include "image_utilities.hpp"
#include <exception>
#include <math.h>
#include <algorithm>


namespace Gamera {
  

  /*
   * Rotate at an arbitrary angle.
   *
   * This algorithm works by first rotating for 90 degrees, depending whether
   * height and width are exchanged by rotation or not.
   * Afterwards VIGRA's rotation algorithm is called, which allows
   * for different types of interpolation.
   *
   * src - A view of of the source image
   * angle - Degree of rotation
   * bgcolor - Background color
   *
   */
  template<class T>
  typename ImageFactory<T>::view_type* rotate(const T &src, double angle, typename T::value_type bgcolor, int order)
  {
    if (order < 1 || order > 3) {
      throw std::range_error("Order must be between 1 and 3");
    }
    if (src.nrows()<2 && src.ncols()<2)
      return simple_image_copy(src);

    // Adjust angle to a positive double between 0-360
    while(angle<0.0) angle+=360;
    while(angle>=360.0) angle-=360;

    // some angle ranges flip width and height
    // as VIGRA requires source and destination to be of the same
    // size, it cannot handle a reduce in one image dimension.
    // Hence we must rotate by 90 degrees, if necessary
    bool rot90done = false;
    typename ImageFactory<T>::view_type* prep4vigra = (typename ImageFactory<T>::view_type*) &src;
    if ((45 < angle && angle < 135) ||
        (225 < angle && angle < 315)) {
      typename ImageFactory<T>::data_type* prep4vigra_data =
        new typename ImageFactory<T>::data_type(Size(src.height(),src.width()));
      prep4vigra = new typename ImageFactory<T>::view_type(*prep4vigra_data);
      size_t ymax = src.nrows() - 1;
      for (size_t y = 0; y < src.nrows(); ++y) {
        for (size_t x = 0; x < src.ncols(); ++x) {
          prep4vigra->set(Point(ymax-y,x), src.get(Point(x,y)));
        }
      }
      rot90done = true;
      // recompute rotation angle, because partial rotation already done
      angle -= 90.0;
      if (angle < 0.0) angle +=360;
    }

    double rad = (angle / 180.0) * M_PI;

    // new width/height depending on angle
    size_t new_width, new_height;
    if ((0 <= angle && angle <= 90) ||
        (180 <= angle && angle <= 270)) {
      new_width = size_t(0.5+abs(cos(rad) * (double)prep4vigra->width() + 
                                 sin(rad) * (double)prep4vigra->height()));
      new_height = size_t(0.5+abs(sin(rad) * (double)prep4vigra->width() + 
                                  cos(rad) * (double)prep4vigra->height()));
    } else {
      new_width = size_t(0.5+abs(cos(rad) * (double)prep4vigra->width() - 
                                 sin(rad) * (double)prep4vigra->height()));
      new_height = size_t(0.5+abs(sin(rad) * (double)prep4vigra->width() - 
                                  cos(rad) * (double)prep4vigra->height()));
    }
    size_t pad_width = 0;
    if (new_width > prep4vigra->width())
      pad_width = (new_width - prep4vigra->width()) / 2 + 2;
    size_t pad_height = 0;
    if (new_height > prep4vigra->height())
      pad_height = (new_height - prep4vigra->height()) / 2 + 2;

    typename ImageFactory<T>::view_type* tmp =
      pad_image(*prep4vigra, pad_height, pad_width, pad_height, pad_width, bgcolor);

    typename ImageFactory<T>::data_type* dest_data =
      new typename ImageFactory<T>::data_type(tmp->size());
    typename ImageFactory<T>::view_type* dest =
      new typename ImageFactory<T>::view_type(*dest_data);

    try {
      fill(*dest, bgcolor);

      if (order == 1) {
        vigra::SplineImageView<1, typename T::value_type> 
          spline(src_image_range(*tmp));
        vigra::rotateImage(spline, dest_image(*dest), -angle);
      } else if (order == 2) {
        vigra::SplineImageView<2, typename T::value_type> 
          spline(src_image_range(*tmp));
        vigra::rotateImage(spline, dest_image(*dest), -angle);
      } else if (order == 3) {
        vigra::SplineImageView<3, typename T::value_type> 
          spline(src_image_range(*tmp));
        vigra::rotateImage(spline, dest_image(*dest), -angle);
      }
    } catch (std::exception e) {
      delete tmp->data();
      delete tmp;
      delete dest;
      delete dest_data;
      if (rot90done) {
        delete prep4vigra->data();
        delete prep4vigra;
      }
      throw;
    }

    if (rot90done) {
      delete prep4vigra->data();
      delete prep4vigra;
    }
    delete tmp->data();
    delete tmp;

    return dest;
  }

  template<class T>
  Image* resize(T& image, const Dim& dim, int resize_quality) {
    typename T::data_type* data = new typename T::data_type
      (dim, image.origin());
    ImageView<typename T::data_type>* view = 
      new ImageView<typename T::data_type>(*data);
    /*
      Images with nrows or ncols == 1 cannot be scaled by VIGRA.
      This is a hack that just returns an image with the same 
      color as the upper-left pixel
    */
    if (image.nrows() <= 1 || image.ncols() <= 1 || 
	view->nrows() <= 1 || view->ncols() <= 1) {
      std::fill(view->vec_begin(), view->vec_end(), image.get(Point(0, 0)));
      return view;
    }
    if (resize_quality == 0) {
      // for straight scaling, resampleImage must be used in VIGRA
      double xfactor = (double)view->ncols()/image.ncols();
      double yfactor = (double)view->nrows()/image.nrows();
      // this is implemented incorrectly in VIGRA:
      //resizeImageNoInterpolation(src_image_range(image), dest_image_range(*view));
      // the following works however:
      // requires extension of VIGRA (see basicgeometry.hxx)
      // that are not yet merged into VIGRA 1.6.0
      resampleImage(src_image_range(image), dest_image(*view), xfactor, yfactor);
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
		  Dim(size_t(double(image.ncols()) * scaling),
		      size_t(double(image.nrows()) * scaling)),
		  resize_quality);
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
      std::copy_backward(begin, end - distance, end);
      std::fill(begin, begin + distance, filler);
      // move up or left
    } else if (distance < 0) {
      filler = *(end - 1);
      std::copy(begin - distance, end, begin);
      std::fill(end + distance, end, filler);
    } // if distance == 0, do nothing
  }

  template<class T>
  void shear_column(T& mat, size_t column, int distance) {
    if (size_t(std::abs(distance)) >= mat.nrows())
      throw std::range_error("Tried to shear column too far");
    if (column >= mat.ncols())
      throw std::range_error("Column argument to shear_column out of range");
    simple_shear((mat.col_begin() + column).begin(),
		 (mat.col_begin() + column).end(), distance);
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


  // mirror operations

  template<class T>
  void mirror_horizontal(T& m) {
    for (size_t r = 0; r < size_t(m.nrows()) / 2; ++r) {
      for (size_t c = 0; c < m.ncols(); ++c) {
	typename T::value_type tmp = m.get(Point(c, r));
	m.set(Point(c, r), m.get(Point(c, m.nrows() - r - 1)));
	m.set(Point(c, m.nrows() - r - 1), tmp);
      }
    }
  }

  template<class T>
  void mirror_vertical(T& m) {
    for (size_t r = 0; r < m.nrows(); ++r) {
      for (size_t c = 0; c < size_t(m.ncols() / 2); ++c) {
	typename T::value_type tmp = m.get(Point(c, r));
	m.set(Point(c, r), m.get(Point(m.ncols() - c - 1, r)));
	m.set(Point(m.ncols() - c - 1, r), tmp);
      }
    }
  }


}
#endif
