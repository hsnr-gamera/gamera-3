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

#ifndef kwm11162001_pixel_hpp
#define kwm11162001_pixel_hpp

/*
  This header contains the definition for all of the standard 'pixels' in 
  Gamera.  These include:

    RGB - color pixels
    Float - floating point pixels that are convenient for many image processing
            algorithms
    GreyScale - grey scale pixels that hold values from 0 - 255 (8bit)
    OneBit - one bit pixels for black and white images.  These pixels actually
             can hold more than 2 values, which is used for labeling the pixels
             (using connected-components for example).  This seems like a lot
             of space to waste on one bit images, but if run-length encoding
             is used the space should be minimul.

  In addition to the pixels themselves, there is information about the pixels
  (white/black values, etc).
 */

#include "gamera_limits.hpp"
#include "vigra/rgbvalue.hxx"

using namespace vigra;

namespace Gamera {

  typedef float FloatPixel;
  typedef unsigned char GreyScalePixel;
  typedef unsigned short Grey16Pixel;
  typedef unsigned int OneBitPixel;

  /*
    The Gamera RGB pixel type is derived from the Vigra class RGBValue. The
    only reason that this is a derived class instead of directly using the
    Vigra type is to provide conversion operators to and from the standard
    Gamera types (instead of using Vigra style promotion traits) and to provide
    overloaded red, green, and blue functions instead of the set* functions
    in the Vigra class.
  */
  template<class T>
  class Rgb : public RGBValue<T> {
  public:
    Rgb(GreyScalePixel grey) : RGBValue<T>(grey) { }
    Rgb(Grey16Pixel grey) : RGBValue<T>(grey) { }
    Rgb(FloatPixel f) : RGBValue<T>((T)f) { }
    Rgb(OneBitPixel s) {
      if (s > 0) {
	RGBValue<T>(1);
      } else {
	RGBValue<T>(0);
      }
    }
    Rgb() : RGBValue<T>() { }
    Rgb(const Rgb& other) : RGBValue<T>(other) { }
    Rgb(T red, T green, T blue) : RGBValue<T>(red, green, blue) { }
    template<class I>
    Rgb(I i, const I end) : RGBValue<T>(i, end) { }
    void red(T v) {
      setRed(v);
    }
    void green(T v) {
      setGreen(v);
    }
    void blue(T v) {
      setBlue(v);
    }
    T const & red() const {
      return data_[0];
    }
    T const & green() const {
      return data_[1];
    }
    T const & blue() const {
      return data_[2];
    }
    operator FloatPixel() {
      return FloatPixel(luminance());
    }
    operator GreyScalePixel() {
      return GreyScalePixel(luminance());
    }
    operator Grey16Pixel() {
      return Grey16Pixel(luminance());
    }
    operator OneBitPixel() {
      if (luminance())
	return 1;
      else
	return 0;
    }
  };

  typedef Rgb<GreyScalePixel> RGBPixel;
  
  /*
    This is a test for black/white regardless of the pixel type.

    This default implementation is here mainly for CCProxies (see
    connected_components.hh).  Most of the real implementations are
    further down.
  */

  template<class T>
  inline bool is_black(T value) {
    T tmp = value;
    if (tmp) return true;
    else return false;
  }

  template<class T>
  inline bool is_white(T value) {
    T tmp = value;
    if (!tmp) return true;
    else return false;
  }

  // pixel traits
  template<class T>
  struct pixel_traits {
    static T white() {
      return std::numeric_limits<T>::max();
    }
    static T black() {
      return 0;
    }
  };

  /*
    Helper functions to get black/white from a given T that has a value_type
    member that is a pixel - i.e.

    DenseImage<OneBitPixel> ob;
    black(ob);

    The pixel_traits syntax is just too horrible to make users go through to
    get white/black.  From within a template function it looks like:

    Gamera::pixel_traits<typename T::value_type>::white();
    
    KWM 6/8/01
  */
  
  template<class T>
  typename T::value_type black(T& container) {
    return pixel_traits<typename T::value_type>::black();
  }

  template<class T>
  typename T::value_type white(T& container) {
    return pixel_traits<typename T::value_type>::white();
  }

  /*
    Everything beyond this point is implementation
   */

  // Specializations for black/white
  template<>
  inline bool is_black<FloatPixel>(FloatPixel value) {
    if (value > 0)
      return false;
    else
      return true;
  }

  template<>
  inline bool is_black<GreyScalePixel>(GreyScalePixel value) {
    if (value == 0)
      return true;
    else
      return false;
  }

  template<>
  inline bool is_black<Grey16Pixel>(Grey16Pixel value) {
    if (value == 0)
      return true;
    else
      return false;
  }

  template<>
  inline bool is_black<RGBPixel>(RGBPixel value) {
    if (value.green() == 0
        && value.red() == 0
        && value.blue() == 0)
      return true;
    else
      return false;
  }

  template<>
  inline bool is_black<OneBitPixel>(OneBitPixel value) {
    if (value > 0)
      return true;
    else
      return false;
  }

  template<>
  inline bool is_white<FloatPixel>(FloatPixel value) {
    if (value == 255)
      return true;
    else
      return false;
  }

  template<>
  inline bool is_white<GreyScalePixel>(GreyScalePixel value) {
    if (value == 255)
      return true;
    else
      return false;
  }

  template<>
  inline bool is_white<Grey16Pixel>(Grey16Pixel value) {
    if (value == std::numeric_limits<Grey16Pixel>::max())
      return true;
    else
      return false;
  }

  template<>
  inline bool is_white<RGBPixel>(RGBPixel value) {
    if (value.red() == 255
        && value.green() == 255
        && value.blue() == 255)
      return true;
    else
      return false;
  }

  template<>
  inline bool is_white<OneBitPixel>(OneBitPixel value) {
    if (value == 0)
      return true;
    else
      return false;
  }

  /*
    Specialization for pixel_traits
  */

  OneBitPixel pixel_traits<OneBitPixel>::black() {
    return 1;
  }

  OneBitPixel pixel_traits<OneBitPixel>::white() {
    return 0;
  }
  
  RGBPixel pixel_traits<RGBPixel>::black() {
    return RGBPixel(0, 0, 0);
  }
  
  RGBPixel pixel_traits<RGBPixel>::white() {
    return RGBPixel(255, 255, 255);
  }
};

#endif
