/*
 *
 * Copyright (C) 2001-2004 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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

/**
 * This header contains the definition for all of the standard pixels in 
 * Gamera.  These include:
 *
 *  RGB - color pixels
 *  Float - floating point pixels that are convenient for many image processing
 *          algorithms
 *  GreyScale - grey scale pixels that hold values from 0 - 255 (8bit)
 *  OneBit - one bit pixels for black and white images.  These pixels actually
 *           can hold more than 2 values, which is used for labeling the pixels
 *           (using connected-components for example).  This seems like a lot
 *           of space to waste on one bit images, but if run-length encoding
 *           is used the space should be minimul.
 *
 * In addition to the pixels themselves, there is information about the pixels
 * (white/black values, etc).
 */

#include "gamera_limits.hpp"
#include "vigra/rgbvalue.hxx"

using namespace vigra;

namespace Gamera {

  /************************************************************************
   * PIXEL TYPES
   ************************************************************************/

  /**
   * Floating-point pixel.
   *
   * The Gamera::FloatPixel type represents a single pixel in a
   * floating-point image. For floating-point images 0 is considerd
   * black and max is considered white.
   */
  typedef double FloatPixel;

  /**
   * GreyScalePixel
   *
   * The Gamera::GreyScalePixel type is for 8bit greyscale images. For GreyScale
   * images 0 is considerd black and 255 is considered white.
   */
  typedef unsigned char GreyScalePixel;

  /**
   * Grey16Pixel
   *
   * The Gamera::Grey16Pixel type is for 16bit greyscale images.
   */
  typedef unsigned int Grey16Pixel;

  /**
   * OneBitPixel
   *
   * The Gamera::OneBitPixel type is for OneBitImages. For OneBit
   * images > 0 is considerd black and 0 is considered white. Also, see the note
   * at the beginning of this file about why OneBitPixels are so large.
   */
  typedef unsigned short OneBitPixel;

  /**
   * RGB Pixels
   *
   * The Gamera::RGB pixel type is derived from the Vigra class RGBValue. The
   * only reason that this is a derived class instead of directly using the
   * Vigra type is to provide conversion operators to and from the standard
   * Gamera types (instead of using Vigra style promotion traits) and to provide
   * overloaded red, green, and blue functions instead of the set* functions
   * in the Vigra class.
   */
  template<class T>
  class Rgb : public RGBValue<T> {
  protected:
    using RGBValue<T>::data_;
    
  public:
    using RGBValue<T>::luminance;

    /**
     * Construct a RGB pixel from a GreyScalePixel. RGB are all
     * set to the passed in GreyScalePixel.
     */
    explicit Rgb(GreyScalePixel grey) : RGBValue<T>(grey) { }
    
    /**
     * Construct a RGB pixel from a Grey16Pixel. RGB are all
     * set to the passed in Grey16Pixel.
     */
    explicit Rgb(Grey16Pixel grey) : RGBValue<T>(grey) { }

    /**
     * Construct a RGB pixel from a Float. RGB are all
     * set to the passed in Float (which is truncated first).
     */
    explicit Rgb(FloatPixel f) : RGBValue<T>((T)f) { }

    /**
     * Construct a RGB Pixel from a OneBitPixel. Appropriate conversion
     * is done.
     */
    explicit Rgb(OneBitPixel s) {
      // TODO: fix for new ONEBIT
      if (s > 0) {
	RGBValue<T>(1);
      } else {
	RGBValue<T>(0);
      }
    }

    /**
     * Default constructor - RGB are all set to 0.
     */
    Rgb() : RGBValue<T>() { }

    /**
     * Copy constructor.
     */
    template <class U>
    Rgb(RGBValue<U> const & r) : RGBValue<T>(r) { }

    Rgb(const Rgb& other) : RGBValue<T>(other) { }
    
    /**
     * Construct a RGB pixel from the passed in red, green, and blue
     * values.
     */
    Rgb(T red, T green, T blue) : RGBValue<T>(red, green, blue) { }

    /**
     * Construct a RGB pixel from the values contained in the iterator
     * range passed in.
     */
    template<class I>
    Rgb(I i, const I end) : RGBValue<T>(i, end) { }

    /// Set the red component to the passed in value.
    void red(T v) {
      setRed(v);
    }

    /// Set the green component to the passed in value.
    void green(T v) {
      setGreen(v);
    }

    /// Set the blue component to the passed in value.
    void blue(T v) {
      setBlue(v);
    }

    /// Retrieve the red component - the returned value is an lvalue.
    T const & red() const {
      return data_[0];
    }

    /// Retrieve the green component - the returned value is an lvalue.
    T const & green() const {
      return data_[1];
    }

    /// Retrieve the blue component - the returned value is an lvalue.
    T const & blue() const {
      return data_[2];
    }

    /// Return the hue of this pixel.
    FloatPixel const hue() {
      FloatPixel maxc = (FloatPixel)std::max(data_[0], std::max(data_[1], data_[2]));
      FloatPixel minc = (FloatPixel)std::min(data_[0], std::min(data_[1], data_[2]));
      if (minc == maxc)
	return 0;
      FloatPixel den = (maxc - minc);
      FloatPixel rc = (maxc - data_[0]) / den;
      FloatPixel gc = (maxc - data_[1]) / den;
      FloatPixel bc = (maxc - data_[2]) / den;
      FloatPixel h;
      if (data_[0] == maxc)
	h = bc - gc;
      else if (data_[1] == maxc)
	h = 2.0 + rc - bc;
      else
	h = 4.0 + gc - rc;
      h /= 6.0;
      h -= floor(h);
      return h;
    }

    /// Return the saturation of this pixel
    FloatPixel const saturation() {
      FloatPixel maxc = (FloatPixel)std::max(data_[0], std::max(data_[1], data_[2]));
      FloatPixel minc = (FloatPixel)std::min(data_[0], std::min(data_[1], data_[2]));
      if (minc == maxc)
	return 0;
      return (maxc - minc) / maxc;
    }

    /// Return the value of this pixel (max of RGB)
    FloatPixel const value() {
      return (FloatPixel)std::max(data_[0], std::max(data_[1], data_[2]));
    }

    FloatPixel const cie_x() {
      return (data_[0] * 0.607 + data_[1] * 0.174 + data_[2] * 0.200) / 256.0;
    }
    FloatPixel const cie_y() {
      return (data_[0] * 0.299 + data_[1] * 0.587 + data_[2] * 0.114) / 256.0;
    }
    FloatPixel const cie_z() {
      return (data_[1] * 0.066 + data_[2] * 1.111) / 256.0;
    }
    GreyScalePixel const cyan() {
      return std::numeric_limits<T>::max() - data_[0];
    }
    GreyScalePixel const magenta() {
      return std::numeric_limits<T>::max() - data_[1];
    }
    GreyScalePixel const yellow() {
      return std::numeric_limits<T>::max() - data_[2];
    }

    /// Conversion operator to a FloatPixel
    operator FloatPixel() {
      return FloatPixel(luminance());
    }

    /// Conversion operator to a GreyScalePixel
    operator GreyScalePixel() {
      return GreyScalePixel(luminance());
    }

    /// Conversion operator to a Grey16Pixel
    operator Grey16Pixel() {
      return Grey16Pixel(luminance());
    }

    /// Conversion operator to a OneBitPixel
    operator OneBitPixel() {
      if (luminance())
	return 1;
      else
	return 0;
    }
  };

  /// This is the standard form of the RGB pixels
  typedef Rgb<GreyScalePixel> RGBPixel;
  
  /*
   * This is a test for black/white regardless of the pixel type. For some
   * pixel types this test is complicated and this also allows us to use 0
   * for white in OneBit images and max for white in others without sacrificing
   * generality in the algorithms.
   *
   * This default implementation is here mainly for CCProxies (see
   * connected_components.hpp).  Most of the real implementations are
   * further down.
   */
  template<class T>
  inline bool is_black(T value) {
    T tmp = value;
    if (tmp) return true;
    else return false;
  }

  /*
   * This is here for the same reason as is_black above.
   */
  template<class T>
  inline bool is_white(T value) {
    T tmp = value;
    if (!tmp) return true;
    else return false;
  }

  /*
   * pixel_traits allows us to find out certain properties of pixels in a generic
   * way. Again, this is primarily to allow the easy switching between min is white
   * and min is black representations for different pixel types.
   */
  template<class T>
  struct pixel_traits {
    static T white() {
      return std::numeric_limits<T>::max();
    }
    static T black() {
      return 0;
    }
    static T default_value() {
      return white();
    }
  };

  /*
   * Helper functions to get black/white from a given T that has a value_type
   * member that is a pixel - i.e.
   *
   * DenseImage<OneBitPixel> ob;
   * black(ob);
   *
   * The pixel_traits syntax is just too horrible to make users go through to
   * get white/black.  From within a template function it looks like:
   *
   * Gamera::pixel_traits<typename T::value_type>::white();
   *
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
    if (value == std::numeric_limits<GreyScalePixel>::max())
      return true;
    else
      return false;
  }

  template<>
  inline bool is_white<GreyScalePixel>(GreyScalePixel value) {
    if (value == std::numeric_limits<GreyScalePixel>::max())
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
    if (value.red() == std::numeric_limits<GreyScalePixel>::max()
        && value.green() == std::numeric_limits<GreyScalePixel>::max()
        && value.blue() == std::numeric_limits<GreyScalePixel>::max())
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

  inline OneBitPixel pixel_traits<OneBitPixel>::black() {
    return 1;
  }

  inline OneBitPixel pixel_traits<OneBitPixel>::white() {
    return 0;
  }

  inline RGBPixel pixel_traits<RGBPixel>::black() {
    return RGBPixel(0, 0, 0);
  }
  
  inline RGBPixel pixel_traits<RGBPixel>::white() {
    return RGBPixel(std::numeric_limits<GreyScalePixel>::max(),
		    std::numeric_limits<GreyScalePixel>::max(),
		    std::numeric_limits<GreyScalePixel>::max());
  }

  inline FloatPixel pixel_traits<FloatPixel>::default_value() {
    return 0.0;
  }

  /* 
  inline ComplexPixel pixel_traits<ComplexPixel>::default_value() {
    return ComplexPixel(0.0, 0.0);
  }
  */

  /*
   * Inversion of pixel values
   *
   * Generically invert pixel values.
   */

  inline FloatPixel invert(FloatPixel value) {
    // Hard to know what makes sense here... MGD
    return -value;
  }

  inline GreyScalePixel invert(GreyScalePixel value) {
    return std::numeric_limits<GreyScalePixel>::max() - value;
  }

  inline Grey16Pixel invert(Grey16Pixel value) {
    return std::numeric_limits<Grey16Pixel>::max() - value;
  }

  inline RGBPixel invert(RGBPixel value) {
    return RGBPixel(std::numeric_limits<RGBPixel::value_type>::max() -
		    value.red(),
		    std::numeric_limits<RGBPixel::value_type>::max() -
		    value.green(),
		    std::numeric_limits<RGBPixel::value_type>::max() -
		    value.blue());
  }

  inline OneBitPixel invert(OneBitPixel value) {
    if (is_white(value))
      return pixel_traits<OneBitPixel>::black();
    else
      return pixel_traits<OneBitPixel>::white();
  }

  /*
   * Blend pixels together.
   */
  inline FloatPixel blend(FloatPixel original, FloatPixel add, double alpha) {
    return alpha * original + (1.0 - alpha) * add; 
  }

  inline GreyScalePixel blend(GreyScalePixel original, GreyScalePixel add, double alpha) {
    return (GreyScalePixel)(alpha * original + (1.0 - alpha) * add);
  }

  inline Grey16Pixel blend(Grey16Pixel original, GreyScalePixel add, double alpha) {
    return (Grey16Pixel)(alpha * original + (1.0 - alpha) * add);
  }

  inline RGBPixel blend(RGBPixel original, RGBPixel add, double alpha) {
    double inv_alpha = 1.0 - alpha;
    return RGBPixel(GreyScalePixel(original.red() * alpha + add.red() * inv_alpha),
		    GreyScalePixel(original.green() * alpha + add.green() * inv_alpha),
		    GreyScalePixel(original.blue() * alpha + add.blue() * inv_alpha));
  }

  inline OneBitPixel blend(OneBitPixel original, RGBPixel add, double alpha) {
    if (alpha > 0.5)
      return original;
    return add.luminance();
  }

};

#endif
