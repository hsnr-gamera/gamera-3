/*
 *
 * Copyright (C) 2001 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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
    Image(size_t origin_y = 0, size_t origin_x = 0, size_t nrows = 1,
	      size_t ncols = 1)
      : Rect(origin_y, origin_x, nrows, ncols) {
      m_resolution = 0;
      m_scaling = 1.0;
    }
    Image(const Point& upper_left, const Point& lower_right)
      : Rect(upper_left, lower_right) {
      m_resolution = 0;
      m_scaling = 1;
    }
    Image(const Point& upper_left, const Size& size)
      : Rect(upper_left, size) {
      m_resolution = 0;
      m_scaling = 1;
    }
    Image(const Point& upper_left, const Dimensions& dim)
      : Rect(upper_left, dim) {
      m_resolution = 0;
      m_scaling = 1;
    }
    Image(const Rect& rect) : Rect(rect) { }
    virtual ~Image() { }
    double resolution() const { return m_resolution; }
    void resolution(double r) { m_resolution = r; }
    double scaling() const { return m_scaling; }
    void scaling(double v) { m_scaling = v; }
    virtual ImageDataBase* data() const = 0;
  protected:
    double m_resolution;
    double m_scaling;
  };

  /*
    ImageBase

    This is the base class for all matrices.
  */
  template<class T>
  class ImageBase : public Image {
  public:
    ImageBase(size_t origin_y = 0, size_t origin_x = 0, size_t nrows = 1,
	      size_t ncols = 1)
      : Image(origin_y, origin_x, nrows, ncols) { }
    ImageBase(const Point& upper_left, const Point& lower_right)
      : Image(upper_left, lower_right) { }
    ImageBase(const Point& upper_left, const Size& size)
      : Image(upper_left, size) { }
    ImageBase(const Point& upper_left, const Dimensions& dim)
      : Image(upper_left, dim) { }
    ImageBase(const Rect& rect) : Image(rect) { }
    virtual ~ImageBase() { }
    size_t depth() const { return sizeof(T) * 8; }
    size_t ncolors() const { return 1; }
  };

  size_t ImageBase<OneBitPixel>::depth() const { return 1; }
  size_t ImageBase<RGBPixel>::ncolors() const { return 3; }
  size_t ImageBase<RGBPixel>::depth() const { return 8; }

};

#endif
