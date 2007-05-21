/*
 *
 * Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom,
 * and Karl MacMillan
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

#ifndef kwm11062002_misc_filters
#define kwm11062002_misc_filters

#include "gamera.hpp"
#include "logical.hpp"
#include "morphology.hpp"
#include "image_utilities.hpp"
#include "vigra/gaborfilter.hxx"


namespace Gamera {
  template<class T>
  typename ImageFactory<T>::view_type* outline(const T& in) {
    typedef typename ImageFactory<T>::data_type data_type;
    typedef typename ImageFactory<T>::view_type view_type;
    view_type* out = erode_dilate(in, 1, 0, 0);
    xor_image(*out, in);
    return out;
  }
  

  template<class T>
  Image* create_gabor_filter(const T& src, double orientation, double frequency, int direction) {

    FloatImageData* dest_data = new FloatImageData(src.size(), src.origin());
    FloatImageView* dest = new FloatImageView(*dest_data);

    image_copy_fill(src, *dest);

    try {
      vigra::createGaborFilter(dest_image_range(*dest), orientation, frequency,
			       vigra::angularGaborSigma(direction, frequency),
			       vigra::radialGaborSigma(frequency));
      
    } catch(std::exception e) {
      delete dest;
      delete dest_data;
      throw std::runtime_error("VIGRA function 'createGaborFilter' failed!");
    }

    return dest;

  }

}

#endif

