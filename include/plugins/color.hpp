/*
 * Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, Karl MacMillan
 *               2010      Hasan Yildiz, Christoph Dalitz
 *               2014-2018 Christoph Dalitz
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
  extract_plane<RGBImageView, FloatImageView, CIE_Lab_a> cie_Lab_a;

  struct CIE_Lab_b {
    FloatPixel operator()(RGBPixel pixel) {
      return pixel.cie_Lab_b();
    }
  };
  extract_plane<RGBImageView, FloatImageView, CIE_Lab_b> cie_Lab_b;

  // TODO: Find a cool way to false color Complex images.

  // divergent colormap after Moreland, 2009
  // for assigning a color to a float value between 0 and 1
  class FloatColormap {
    //typedef typename std::vector<double> MSHValue;
    typedef std::vector<double> MSHValue;
    MSHValue msh_cold;
    MSHValue msh_warm;
    MSHValue xyz_white;
    double   m_mid;
    void rgb2xyz(const RGBPixel& rgb, MSHValue* xyz) {
      MSHValue rgbl(3,0);
      if (rgb.red() > 0.04045)
        rgbl[0] = pow((rgb.red()/255.0 + 0.055)/1.055, 2.4);
      else
        rgbl[0] = rgb.red()/(255.0*12.92);
      if (rgb.green() > 0.04045)
        rgbl[1] = pow((rgb.green()/255.0 + 0.055)/1.055, 2.4);
      else
        rgbl[1] = rgb.green()/(255.0*12.92);
      if (rgb.blue() > 0.04045)
        rgbl[2] = pow((rgb.blue()/255.0 + 0.055)/1.055, 2.4);
      else
        rgbl[2] = rgb.blue()/(255.0*12.92);
      /* this is from Moreland paper, but presumably wrong
      rgbl[0] = pow((rgb.red()/255.0 + 0.055)/1.055, 2.4);
      if (rgbl[0] <= 0.04045) rgbl[0] = rgb.red()/(255.0*12.92);
      rgbl[1] = pow((rgb.green()/255.0 + 0.055)/1.055, 2.4);
      if (rgbl[1] <= 0.04045) rgbl[1] = rgb.green()/(255.0*12.92);
      rgbl[2] = pow((rgb.blue()/255.0 + 0.055)/1.055, 2.4);
      if (rgbl[2] <= 0.04045) rgbl[2] = rgb.blue()/(255.0*12.92);
      */
      //printf("rgbl = (%f,%f,%f)\n", rgbl[0], rgbl[1], rgbl[2]);
      xyz->at(0) = 0.412453*rgbl[0] + 0.357580*rgbl[1] + 0.180423*rgbl[2];
      xyz->at(1) = 0.212671*rgbl[0] + 0.715160*rgbl[1] + 0.072169*rgbl[2];
      xyz->at(2) = 0.019334*rgbl[0] + 0.119193*rgbl[1] + 0.950227*rgbl[2];
    }
    double f(double x) {
      if (x>0.008856) return pow(x, 1/3.0);
      else return (0.787*x + 16.0/116.0);
    }
    double finv(double x) {
      if (x>0.20689) return (x*x*x);
      else return ((x - 16.0/116.0) / 0.787);
    }
    void rgb2msh(const RGBPixel& rgb, MSHValue* msh) {
      MSHValue lab(3,0), xyz(3,0);
      rgb2xyz(rgb, &xyz);
      //printf("xyz = (%f,%f,%f)\n", xyz[0], xyz[1], xyz[2]);
      lab[0] = 116.0*f(xyz[1]/xyz_white[1]) - 16.0;
      lab[1] = 500.0*(f(xyz[0]/xyz_white[0]) - f(xyz[1]/xyz_white[1]));
      lab[2] = 200.0*(f(xyz[1]/xyz_white[1]) - f(xyz[2]/xyz_white[2]));
      msh->at(0) = sqrt(lab[0]*lab[0] + lab[1]*lab[1] + lab[2]*lab[2]);
      if (msh->at(0) > 0.0001)
        msh->at(1) = acos(lab[0] / msh->at(0));
      else
        msh->at(1) = 0;
      if (msh->at(1) > 0.0001)
        msh->at(2) = atan2(lab[2], lab[1]);
      else
        msh->at(2) = 0;
    }
    void msh2rgb(const MSHValue& msh, RGBPixel* rgb) {
      MSHValue lab(3,0), xyz(3,0), rgbl(3,0), srgb(3,0);
      double fy, fx;
      // msh2lab
      lab[0] = msh[0] * cos(msh[1]);
      lab[1] = msh[0] * sin(msh[1]) * cos(msh[2]);
      lab[2] = msh[0] * sin(msh[1]) * sin(msh[2]);
      // lab2xyz
      fy = (lab[0] + 16.0) / 116.0;
      xyz[1] = finv(fy) * xyz_white[1];
      fx = lab[1]/500.0 + fy;
      xyz[0] = finv(fx) * xyz_white[0];
      xyz[2] = finv(fy - lab[2]/200.0) * xyz_white[2];
      //printf("xyz = (%f,%f,%f)\n", xyz[0], xyz[1], xyz[2]);
      // xyz2rgbl
      rgbl[0] =  3.240481*xyz[0] - 1.537152*xyz[1] - 0.498536*xyz[2];
      rgbl[1] = -0.969255*xyz[0] + 1.875990*xyz[1] + 0.041556*xyz[2];
      rgbl[2] =  0.055647*xyz[0] - 0.204041*xyz[1] + 1.057311*xyz[2];
      //printf("rgbl = (%f,%f,%f)\n", rgbl[0], rgbl[1], rgbl[2]);
      // rgbL2rgb
      for (size_t i=0; i<3; i++) {
        //if (rgbl[i] > 0.04045) // This is from moreland, but presumably wrong
        if (rgbl[i] > 0.001308)
          srgb[i] = pow(rgbl[i], 1/2.4)*1.055 - 0.055;
        else
          srgb[i] = rgbl[i]*12.92;
        srgb[i] = srgb[i]*255.0;
      }
      //printf("srgb = (%f,%f,%f)\n", srgb[0], srgb[1], srgb[2]);
      rgb->red(int(srgb[0]+0.25)); rgb->green(int(srgb[1]+0.25)); rgb->blue(int(srgb[2]+0.25));
    }
    double adjust_hue(const MSHValue& msh_sat, double m_unsat) {
      if (msh_sat[0] >= m_unsat-0.1)
        return msh_sat[2];
      double hspin = msh_sat[1]*sqrt(m_unsat*m_unsat - msh_sat[0]*msh_sat[0]) / (msh_sat[0] * sin(msh_sat[1]));
      if (msh_sat[2] > -M_PI/3)
        return msh_sat[2] + hspin;
      else
        return msh_sat[2] - hspin;
    }
  public:
    FloatColormap(const RGBPixel& rgb_cold, const RGBPixel& rgb_warm) {
      msh_cold.resize(3); msh_warm.resize(3); xyz_white.resize(3);
      // compute white point
      RGBPixel rgb_white(255,255,255);
      rgb2xyz(rgb_white, &xyz_white);
      // set cold and warm values
      rgb2msh(rgb_cold, &msh_cold); rgb2msh(rgb_warm, &msh_warm);
      RGBPixel shouldbe; msh2rgb(msh_cold, &shouldbe);
      m_mid = std::max(88.0, std::max(msh_cold[0], msh_warm[0]));
    }
    RGBPixel interpolate_color(double v) {
      MSHValue msh_mid(3,0), msh1(3,0), msh2(3,0);
      RGBPixel rgb;
      if (v < 0.0) {
        msh2rgb(msh_cold, &rgb);
        return rgb;
      }
      if (v > 1.0) {
        msh2rgb(msh_warm, &rgb);
        return rgb;
      }
      if (v<0.5) {
        msh2[0] = m_mid; msh2[1] = 0.0; msh2[2] = 0.0;
        for (size_t i=0; i<3; i++) msh1[i] = msh_cold[i];
        v = 2.0*v;
      } else {
        msh1[0] = m_mid; msh1[1] = 0.0; msh1[2] = 0.0;
        for (size_t i=0; i<3; i++) msh2[i] = msh_warm[i];
        v = 2.0*v - 1.0;
      }
      if ((msh1[1] < 0.05) && (msh2[1] > 0.05))
        msh1[2] = adjust_hue(msh2, msh1[0]);
      else if ((msh2[1] < 0.05) && (msh1[1] > 0.05))
        msh2[2] = adjust_hue(msh1, msh2[0]);
      for (size_t i=0; i<3; i++)
        msh_mid[i] = (1-v)*msh1[i] + v*msh2[i];
      msh2rgb(msh_mid, &rgb);
      return rgb;
    }
  };

  RGBImageView* false_color(const FloatImageView& image, int colormap) {
    RGBImageView* view = _image_conversion::creator<RGBPixel>::image(image);

    FloatImageView::const_vec_iterator vi = image.vec_begin();
    FloatPixel fmax = *vi;
    FloatPixel fmin = *vi;
    for (; vi != image.vec_end(); ++vi) {
      if (*vi > fmax)
        fmax = *vi;
      if (*vi < fmin)
        fmin = *vi;
    }
    double scale = fmax - fmin;
    FloatImageView::const_vec_iterator in = image.vec_begin();
    RGBImageView::vec_iterator out = view->vec_begin();

    if (colormap == 0) { // diverging colormap
      RGBPixel rgb1(59,76,192), rgb2(180,4,38), rgb3;
      FloatColormap fc(rgb1, rgb2);
      for (; in != image.vec_end(); ++in, ++out)
        out.set(fc.interpolate_color((in.get()-fmin)/scale));
    }
    else { // rainbow colormap
      double val;
      for (; in != image.vec_end(); ++in, ++out) {
        val = (in.get()-fmin)/scale;
        double a = (1.0 - val)*4.0;
        int x = int(a);
        int y = int(255*(a-x));
        if (x == 0)
          out.set(RGBPixel(255,y,0));
        else if (x == 1)
          out.set(RGBPixel(255-y,255,0));
        else if (x == 2)
          out.set(RGBPixel(0,255,y));
        else if (x == 3)
          out.set(RGBPixel(0,255-y,255));
        else
          out.set(RGBPixel(0,0,255));
      }
    }

    return view;	
  }

  RGBImageView* false_color(const GreyScaleImageView& image, int colormap) {
    RGBImageView* view = _image_conversion::creator<RGBPixel>::image(image);
    GreyScaleImageView::const_vec_iterator in = image.vec_begin();
    RGBImageView::vec_iterator out = view->vec_begin();
    ImageAccessor<GreyScalePixel> in_acc;
    ImageAccessor<RGBPixel> out_acc;

    // Build a table mapping greyscale values to false color RGBPixels
    RGBPixel table[256];
    if (colormap == 0) { // diverging colormap
      RGBPixel rgb1(59,76,192), rgb2(180,4,38), rgb3;
      FloatColormap fc(rgb1, rgb2);
      /*for (double v=0.0; v<=1; v+=1/256.0) {
        rgb3 = fc.interpolate_color(v);
        printf("%7.6f,%i,%i,%i\n", v, rgb3.red(), rgb3.green(), rgb3.blue());
      }*/
      for (size_t i=0.0; i<256; i++) {
        table[i] = fc.interpolate_color(double(i)/255.0);
      }
    }
    else { // rainbow colormap
      for (size_t i=0.0; i<256; i++) {
        double a = (255.0 - i)*4.0/255.0;
        int x = int(a);
        int y = int(255*(a-x));
        if (x == 0)
          table[i] = RGBPixel(255,y,0);
        else if (x == 1)
          table[i] = RGBPixel(255-y,255,0);
        else if (x == 2)
          table[i] = RGBPixel(0,255,y);
        else if (x == 3)
          table[i] = RGBPixel(0,255-y,255);
        else
          table[i] = RGBPixel(0,0,255);
      }
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
