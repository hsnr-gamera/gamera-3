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

#ifndef kwm01032002_gamera_hpp
#define kwm01032002_gamera_hpp

/*
  The bulk of gamera is in the following includes - this file is simply for
  convenience.
 */

#include "pixel.hpp"
#include "dimensions.hpp"
#include "image_info.hpp"
#include "image_algorithm.hpp"
#include "utility.hpp"
#include "image_view.hpp"
#include "connected_components.hpp"
#include "image_data.hpp"
#include "rle_data.hpp"
#include "image.hpp"
#include "region.hpp"
#include "static_image.hpp"
#include "vigra_support.hpp"
#include "image_types.hpp"
#include "accessor.hpp"

#include <vector>
#include <list>

namespace Gamera {

  /*
    Features
   */
  typedef double feature_t;
  typedef std::vector<feature_t> FloatVector;

  /*
    Utility
  */
  typedef std::vector<std::string> StringVector;
  typedef std::vector<int> IntVector;
  typedef std::vector<int> SignedIntVector;

  /*
    RegionMap
  */
  typedef RegionTemplate<double> Region;
  typedef RegionMapTemplate<double> RegionMap;

  /*
    Colors
  */
  static const int COLOR_SET_SIZE = 8;
  const unsigned char color_set[COLOR_SET_SIZE][3] = {
    {0xbc, 0x2d, 0x2d}, // Red
    {0xb4, 0x2d, 0xbc}, // Magenta
    {0x2d, 0x34, 0xbc}, // Blue
    {0x2d, 0xbc, 0xb7}, // Cyan
    {0x3a, 0xbc, 0x2d}, // Green
    {0xbc, 0xb7, 0x2d}, // Yellow
    {0xbc, 0x88, 0x2d}, // Orange
    {0x6e, 0x00, 0xc7}  // Purple
  };
}

#endif

