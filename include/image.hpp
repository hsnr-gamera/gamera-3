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

#include "gameramodule.hpp"
#include "pixel.hpp"
#include "image_data.hpp"
#include <stddef.h>

namespace Gamera {

  class Image : public Rect {
  public:
    Image()
      : Rect() {
      m_resolution = 0;
      m_scaling = 1.0;
      features = 0;
      features_len = 0;
    }
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
    Py_ssize_t features_len;
  protected:
    double m_resolution;
    double m_scaling;
  };

  /*
    ImageBase

    This is the base class for all images.
  */
  template<class T>
  class ImageBase : public Image {
  public:
    ImageBase() : Image() { }
    ImageBase(const Point& upper_left, const Point& lower_right)
      : Image(upper_left, lower_right) { }
    ImageBase(const Point& upper_left, const Size& size)
      : Image(upper_left, size) { }
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
};

#endif
