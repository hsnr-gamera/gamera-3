/*
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

#ifndef mgd120203_color
#define mgd120203_color

#include "gamera.hpp"
#include "image_conversion.hpp"

namespace Gamera {

  template<class T, class U, class F>
  struct extract_plane {
    U* operator()(const T& image) {
      typedef typename T::value_type from_pixel_type;
      typedef typename U::value_type to_pixel_type;
      U* view = _image_conversion::creator<to_pixel_type>::image(image);
      typename T::const_vec_iterator in = image.vec_begin();
      typename U::vec_iterator out = view->vec_begin();
      ImageAccessor<from_pixel_type> in_acc;
      ImageAccessor<to_pixel_type> out_acc;
      F f;
      for (; in != image.vec_end(); ++in, ++out)
	out_acc.set(f(from_pixel_type(in_acc.get(in))), out);
      return view;
    }
  };

  struct Hue {
    FloatPixel operator()(RGBPixel pixel) {
      return pixel.hue();
    }
  };
  extract_plane<RGBImageView, FloatImageView, Hue> hue;

  struct Saturation {
    FloatPixel operator()(RGBPixel pixel) {
      return pixel.saturation();
    }
  };
  extract_plane<RGBImageView, FloatImageView, Saturation> saturation;

  struct Value {
    FloatPixel operator()(RGBPixel pixel) {
      return pixel.value();
    }
  };
  extract_plane<RGBImageView, FloatImageView, Value> value;

  struct Cyan {
    FloatPixel operator()(RGBPixel pixel) {
      return pixel.cyan();
    }
  };
  extract_plane<RGBImageView, FloatImageView, Cyan> cyan;

  struct Magenta {
    FloatPixel operator()(RGBPixel pixel) {
      return pixel.magenta();
    }
  };
  extract_plane<RGBImageView, FloatImageView, Magenta> magenta;

  struct Yellow {
    FloatPixel operator()(RGBPixel pixel) {
      return pixel.yellow();
    }
  };
  extract_plane<RGBImageView, FloatImageView, Yellow> yellow;

  struct Red {
    FloatPixel operator()(RGBPixel pixel) {
      return pixel.red();
    }
  };
  extract_plane<RGBImageView, FloatImageView, Red> red;

  struct Green {
    FloatPixel operator()(RGBPixel pixel) {
      return pixel.green();
    }
  };
  extract_plane<RGBImageView, FloatImageView, Green> green;

  struct Blue {
    FloatPixel operator()(RGBPixel pixel) {
      return pixel.blue();
    }
  };
  extract_plane<RGBImageView, FloatImageView, Blue> blue;

  struct CIE_X {
    FloatPixel operator()(RGBPixel pixel) {
      return pixel.cie_x();
    }
  };
  extract_plane<RGBImageView, FloatImageView, CIE_X> cie_x;

  struct CIE_Y {
    FloatPixel operator()(RGBPixel pixel) {
      return pixel.cie_y();
    }
  };
  extract_plane<RGBImageView, FloatImageView, CIE_Y> cie_y;

  struct CIE_Z {
    FloatPixel operator()(RGBPixel pixel) {
      return pixel.cie_z();
    }
  };
  extract_plane<RGBImageView, FloatImageView, CIE_Z> cie_z;

  // TODO: Find a cool way to false color Complex images.

  RGBImageView* false_color(const FloatImageView& image) {
    RGBImageView* view = _image_conversion::creator<RGBPixel>::image(image);
    FloatPixel max = 0;
    max = find_max(image.parent());

    // We don't use a table (as with 8-bit greyscale) because we can get
    // much greater color "depth" this way (The table method only uses
    // 64 values per plane)

    FloatImageView::const_vec_iterator in = image.vec_begin();
    RGBImageView::vec_iterator out = view->vec_begin();
    ImageAccessor<FloatPixel> in_acc;
    ImageAccessor<RGBPixel> out_acc;
    for (; in != image.vec_end(); ++in, ++out) {
      double h = (in_acc.get(in) / max) * 4.0;
      size_t i = (size_t)h;
      GreyScalePixel f, q;
      // v == 255, p == 0
      switch (i) {
      case 0:
	f = (GreyScalePixel)((h - (double)i) * 255.0);
	out_acc.set(RGBPixel(255, f, 0), out);
	break;
      case 1:
	q = 255 - (GreyScalePixel)((h - (double)i) * 255.0);
	out_acc.set(RGBPixel(q, 255, 0), out);
	break;
      case 2:
	f = (GreyScalePixel)((h - (double)i) * 255.0);
	out_acc.set(RGBPixel(0, 255, f), out);
	break;
      case 3:
	q = 255 - (GreyScalePixel)((h - (double)i) * 255.0);
	out_acc.set(RGBPixel(0, q, 255), out);
	break;
      case 4: // The end (should only represent a single value)
	out_acc.set(RGBPixel(0, 0, 255), out); 
	break;
      }
    }
    return view;	
  }

  RGBImageView* false_color(const GreyScaleImageView& image) {
    RGBImageView* view = _image_conversion::creator<RGBPixel>::image(image);
    GreyScaleImageView::const_vec_iterator in = image.vec_begin();
    RGBImageView::vec_iterator out = view->vec_begin();
    ImageAccessor<GreyScalePixel> in_acc;
    ImageAccessor<RGBPixel> out_acc;

    // Build a table mapping greyscale values to false color RGBPixels
    RGBPixel table[256];
    size_t i = 0, j = 0;
    for (; i < 64; ++i, j += 4) {
      table[i].red(255); table[i].green(j); table[i].blue(0);
    }
    j -= 4;
    for (; i < 128; ++i, j -= 4) {
      table[i].red(j); table[i].green(255); table[i].blue(0);
    }
    j = 0;
    for (; i < 192; ++i, j += 4) {
      table[i].red(0); table[i].green(255); table[i].blue(j);
    }
    j -= 4;
    for (; i < 256; ++i, j -= 4) {
      table[i].red(0); table[i].green(j); table[i].blue(255);
    }

    // Create RGB based on table
    for (; in != image.vec_end(); ++in, ++out)
      out_acc.set(table[in_acc.get(in)], out);
      
    return view;	
  }

}

#endif
