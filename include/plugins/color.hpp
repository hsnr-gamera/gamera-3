/*
 * Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, Karl MacMillan
 *               2010      Hasan Yildiz, Christoph Dalitz
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
#include "gameramodule.hpp"

#include <string>
#include <map>
#include <vector>

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

  struct CIE_Lab_L {
    FloatPixel operator()(RGBPixel pixel) {
      return pixel.cie_Lab_L();
    }
  };
  extract_plane<RGBImageView, FloatImageView, CIE_Lab_L> cie_Lab_L;

  struct CIE_Lab_a {
    FloatPixel operator()(RGBPixel pixel) {
      return pixel.cie_Lab_a();
    }
  };
  extract_plane<RGBImageView, FloatImageView, CIE_Lab_L> cie_Lab_a;

  struct CIE_Lab_b {
    FloatPixel operator()(RGBPixel pixel) {
      return pixel.cie_Lab_L();
    }
  };
  extract_plane<RGBImageView, FloatImageView, CIE_Lab_L> cie_Lab_b;

  // TODO: Find a cool way to false color Complex images.

  RGBImageView* false_color(const FloatImageView& image) {
    RGBImageView* view = _image_conversion::creator<RGBPixel>::image(image);

    FloatImageView::const_vec_iterator vi = image.vec_begin();
    FloatPixel max = *vi;
    FloatPixel min = *vi;
    for (; vi != image.vec_end(); ++vi) {
      if (*vi > max)
	max = *vi;
      if (*vi < min)
	min = *vi;
    }
    
    FloatPixel scale = (max - min);

    // We don't use a table (as with 8-bit greyscale) because we can get
    // much greater color "depth" this way (The table method only uses
    // 64 values per plane)

    FloatImageView::const_vec_iterator in = image.vec_begin();
    RGBImageView::vec_iterator out = view->vec_begin();
    ImageAccessor<FloatPixel> in_acc;
    ImageAccessor<RGBPixel> out_acc;
    for (; in != image.vec_end(); ++in, ++out) {
      double h = ((in_acc.get(in) - min) / scale) * 4.0;
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

  // replace colors with labels
  // Christoph Dalitz and Hasan Yildiz
  template<class T>
  Image* colors_to_labels(const T &src, PyObject* obj) {
    OneBitImageData* dest_data = new OneBitImageData(src.size(), src.origin());
    OneBitImageView* dest = new OneBitImageView(*dest_data, src.origin(), src.size());

    typedef typename T::value_type value_type;
    value_type value;
    typedef typename OneBitImageView::value_type onebit_value_type;

    std::string buf;
    onebit_value_type label;
    // highest label that can be stored in Onebit pixel type
    onebit_value_type max_value = std::numeric_limits<onebit_value_type>::max();

    std::map<unsigned int, unsigned int> pixel;
    std::map<unsigned int, unsigned int>::iterator iter;

    PyObject *itemKey, *itemValue;
    unsigned int testKey;
    Py_ssize_t pos = 0;

    // mapping given how colors are to be mapped to labels
    if (PyDict_Check(obj)) {

      // copy color->label map to C++ map
      long given_label;
      label = 1;
      while (PyDict_Next(obj, &pos, &itemKey, &itemValue)) {
        if (label == max_value) {
          char msg[128];
          sprintf(msg, "More RGB colors than available labels (%i).", max_value);
          throw std::range_error(msg);
        }
        label++;
        if( !PyObject_TypeCheck(itemKey, get_RGBPixelType()) ) {
            throw std::runtime_error("Dictionary rgb_to_label must have RGBPixel's as keys");
        }
        
        RGBPixel *rgbpixel = ((RGBPixelObject *) itemKey)->m_x;
        testKey = (rgbpixel->red() << 16) | (rgbpixel->green() << 8) | rgbpixel->blue();
        
        given_label = PyInt_AsLong(itemValue);
        if (given_label < 0)
          throw std::invalid_argument("Labels must be positive integers.");
        if (pixel.find(testKey) == pixel.end()) 
          pixel[testKey] = given_label;
      }

      for (size_t y=0; y<src.nrows(); ++y) {
        for (size_t x=0; x<src.ncols(); ++x) {
          value = src.get(Point(x,y));
          testKey = (value.red() << 16) | (value.green() << 8) | value.blue();
          if (pixel.find(testKey) != pixel.end()) 
            dest->set(Point(x,y), pixel.find(testKey)->second);
        }
      }
    }

    // no mapping given: determine labels automatically by counting
    else if (obj == Py_None) {
      label = 2;
      // special colors black and white
      pixel[0] = 1;
      pixel[(255<<16) | (255<<8) | 255] = 0;
      for (size_t y=0; y<src.nrows(); ++y) {
        for (size_t x=0; x<src.ncols(); ++x) {

          value = src.get(Point(x,y));  
          testKey = (value.red() << 16) | (value.green() << 8) | value.blue();
          
          if ( !(value.red()==0 && value.green()==0 && value.blue()==0) &&
               !(value.red()==255 && value.green()==255 && value.blue()==255) &&
               pixel.find(testKey) == pixel.end() ) {
            if (label == max_value) {
              char msg[128];
              sprintf(msg, "More RGB colors than available labels (%i).", max_value);
              throw std::range_error(msg);
            }
            pixel[testKey] = label++;
          }

          // replace color with label
          dest->set(Point(x,y), pixel.find(testKey)->second);
        }
      }
    }

    // some argument given that is not a mapping color -> label
    else {
      throw std::invalid_argument("Mapping rgb_to_label must be dict or None");
    }

    return dest;
  }

}

#endif
