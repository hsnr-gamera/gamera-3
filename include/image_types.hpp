/*
 *
 * Copyright (C) 2002 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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

#ifndef kwm03112002_image_types
#define kwm03112002_image_types

#include "pixel.hpp"
#include "image_data.hpp"
#include "image_view.hpp"
#include "rle_data.hpp"
#include "connected_components.hpp"

#include <list>

/*
  The standard image types.
*/

namespace Gamera {

  /*
    Image Data
   */
  typedef ImageData<GreyScalePixel> GreyScaleImageData;
  typedef ImageData<Grey16Pixel> Grey16ImageData;
  typedef ImageData<FloatPixel> FloatImageData;
  typedef ImageData<RGBPixel> RGBImageData;
  typedef ImageData<OneBitPixel> OneBitImageData;
  typedef RleImageData<OneBitPixel> OneBitRleImageData;

  /*
    ImageView
   */
  typedef ImageView<GreyScaleImageData> GreyScaleImageView;
  typedef ImageView<Grey16ImageData> Grey16ImageView;
  typedef ImageView<FloatImageData> FloatImageView;
  typedef ImageView<RGBImageData> RGBImageView;
  typedef ImageView<OneBitImageData> OneBitImageView;
  typedef ImageView<OneBitRleImageData> OneBitRleImageView;

  /*
    Connected-components
   */
  typedef ConnectedComponent<OneBitImageData> CC;
  typedef ConnectedComponent<OneBitRleImageData> RleCC;
  typedef std::list<CC> ConnectedComponents;
  typedef std::list<RleCC> RleConnectedComponents;

  /*
    Factory for types.
  */
  template<class T>
  struct image_factory {
    typedef ImageView<typename T::data_type> view_type;
    typedef ConnectedComponent<typename T::data_type> cc_type;
    typedef std::list<cc_type> ccs_type;
  };
}


#endif
