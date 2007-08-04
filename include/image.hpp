/*
 *
 * Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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

#ifndef kwm11162001_image_hpp
#define kwm11162001_image_hpp

#include "pixel.hpp"
#include "image_data.hpp"
#include <stddef.h>

namespace Gamera {

  class Image : public Rect {
  public:
#ifdef GAMERA_DEPRECATED
    /*
Image(size_t origin_y = 0, size_t origin_x = 0, size_t nrows = 1,
size_t ncols = 1) is deprecated.

Reason: (x, y) coordinate consistency.

Use Image(Point(origin_x, origin_y), Dim(ncols, nrows)) instead.
    */
    Image(size_t origin_y = 0, size_t origin_x = 0, size_t nrows = 1,
	  size_t ncols = 1) GAMERA_CPP_DEPRECATED;
#else
    Image()
      : Rect() {
      m_resolution = 0;
      m_scaling = 1.0;
      features = 0;
      features_len = 0;
    }
#endif
    Image(const Point& upper_left, const Point& lower_right)
      : Rect(upper_left, lower_right) {
      m_resolution = 0;
      m_scaling = 1.0;
      features = 0;
      features_len = 0;
    }
    Image(const Point& upper_left, const Size& size)
      : Rect(upper_left, size) {
      m_resolution = 0;
      m_scaling = 1.0;
      features = 0;
      features_len = 0;
    }
#ifdef GAMERA_DEPRECATED
    /*
Image(const Point& upper_left, const Dimensions& dim) is deprecated.

Reason: (x, y) coordinate consistency. (Dimensions is now deprecated
in favor of Dim).

Use Image(Point(origin_x, origin_y), Dim(ncols, nrows)) instead.
    */
    Image(const Point& upper_left, const Dimensions& dim) GAMERA_CPP_DEPRECATED;
#endif
    Image(const Point& upper_left, const Dim& dim)
      : Rect(upper_left, dim) {
      m_resolution = 0;
      m_scaling = 1.0;
    }

    Image(const Rect& rect) : Rect(rect) {
      m_resolution = 0;
      m_scaling = 1.0;
      features = 0;
      features_len = 0;
    }
    virtual ~Image() {  }
    double resolution() const { return m_resolution; }
    void resolution(double r) { m_resolution = r; }
    double scaling() const { return m_scaling; }
    void scaling(double v) { m_scaling = v; }
    virtual ImageDataBase* data() const = 0;
  public:
    double* features;
    int features_len;
  protected:
    double m_resolution;
    double m_scaling;
  };

#ifdef GAMERA_DEPRECATED
  inline Image::Image(size_t origin_y, size_t origin_x, size_t nrows, size_t ncols)
    : Rect(Point(origin_x, origin_y), Dim(ncols, nrows)) {
    m_resolution = 0;
    m_scaling = 1.0;
    features = 0;
    features_len = 0;
  }
  
  inline Image::Image(const Point& upper_left, const Dimensions& dim) 
    : Rect(upper_left, Dim(dim.ncols(), dim.nrows())) {
    m_resolution = 0;
    m_scaling = 1.0;
  }
#endif

  /*
    ImageBase

    This is the base class for all images.
  */
  template<class T>
  class ImageBase : public Image {
  public:
#ifdef GAMERA_DEPRECATED
    /*
ImageBase(size_t origin_y = 0, size_t origin_x = 0, size_t nrows = 1,
size_t ncols = 1) is deprecated.

Reason: (x, y) coordinate consistency.

Use ImageBase(Point(origin_x, origin_y), Dim(ncols, nrows)) instead.
    */
    ImageBase(size_t origin_y = 0, size_t origin_x = 0, size_t nrows = 1,
	      size_t ncols = 1) GAMERA_CPP_DEPRECATED;
#else
    ImageBase() : Image() { }
#endif
    ImageBase(const Point& upper_left, const Point& lower_right)
      : Image(upper_left, lower_right) { }
    ImageBase(const Point& upper_left, const Size& size)
      : Image(upper_left, size) { }
#ifdef GAMERA_DEPRECATED
    /*
ImageBase(const Point& upper_left, const Dimensions& dim) is deprecated.

Reason: (x, y) coordinate consistency. (Dimensions is now deprecated
in favor of Dim).

Use ImageBase(Point(origin_x, origin_y), Dim(ncols, nrows)) instead.
    */
    ImageBase(const Point& upper_left, const Dimensions& dim) 
      GAMERA_CPP_DEPRECATED;
#endif
    ImageBase(const Point& upper_left, const Dim& dim)
      : Image(upper_left, dim) { }
    ImageBase(const Rect& rect) : Image(rect) { }
    virtual ~ImageBase() { }
    inline size_t depth() const { return sizeof(T) * 8; }
    inline size_t ncolors() const { return 1; }
  };

  template<>
  inline size_t ImageBase<OneBitPixel>::depth() const { return 1; }
  template<>
  inline size_t ImageBase<RGBPixel>::ncolors() const { return 3; }
  template<>
  inline size_t ImageBase<RGBPixel>::depth() const { return 8; }

#ifdef GAMERA_DEPRECATED
  template<class T>
  ImageBase<T>::ImageBase(size_t origin_y, size_t origin_x, size_t nrows,
			  size_t ncols)
    : Image(Point(origin_x, origin_y), Dim(ncols, nrows)) { } 
  
  template<class T>
  ImageBase<T>::ImageBase(const Point& upper_left, const Dimensions& dim)
    : Image(upper_left, Dim(dim.ncols(), dim.nrows())) { }
#endif

};

#endif
