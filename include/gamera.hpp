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

#ifndef kwm01032002_gamera_hpp
#define kwm01032002_gamera_hpp

/*
  The bulk of gamera is in the following includes - this file is simply for
  convenience.
 */

#include <vector>
#include <list>

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

namespace Gamera {

  /*
   * This file contains a few typedefs to make standard classes
   * easier to use.
   */

  /***********************************************************
   * FEATURES
   ***********************************************************/

  /**
   * Image feature type.
   *
   * The feature_t is a typedef for a floating point type
   * used for all of the feature calculation functions
   * in Gamera.
   */
  typedef double feature_t;

  /**
   * Floating-point vector.
   *
   * The Gamera::FloatVector type is used to pass arrays of
   * floating-point data. Each element is of type feature_t
   * and the container is a std::vector.
   */
  typedef std::vector<feature_t> FloatVector;

  /***********************************************************
   * UTILITY
   ***********************************************************/

  /**
   * String Vector.
   *
   * The Gamera::StringVectors is a std::vector of std::strings.
   */
  typedef std::vector<std::string> StringVector;

  /**
   * Int Vector.
   *
   * The Gamera::IntVector type is used to pass arrays of
   * integers types. The container is a std::vector.
   */
  typedef std::vector<int> IntVector;

  /**
   * SignedIntVector
   *
   * The Gamera::SignedIntVector type is used to pass arrays
   * of signed integer types. In previous versions this was
   * different from Gamera::IntVectors, which held unsigned
   * types. Currently, however, IntVector and SignedIntVector
   * are the same.
   */
  typedef std::vector<int> SignedIntVector;

  /**
   * Region
   *
   * The Gamera::Region type is used to associate a rectangular
   * areas on an image with a set of values. These values are stored
   * as key/value pairs (string/double). See RegionTemplate for more
   * information.
   */
  typedef RegionTemplate<double> Region;

  /**
   * Region Map
   *
   * The Gamera::RegionMap type is a list of regions. Regions can be
   * added to the list and searched for by position.
   */
  typedef RegionMapTemplate<double> RegionMap;

  /**
   * Colors
   *
   * Gamera::color_set is a standard set of colors for use whenever
   * colors need to be applied to an image. By limiting the size to
   * eight it makes it easy to pick an arbitrary color by bitwise
   * operators on pixels.
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

  /**
   * Point Vector
   *
   * For managing lists of points
   */
  typedef std::vector<Point> PointVector;

  template<class T>
  static inline T sign(const T& x) {
    return ((x) > 0 ? 1 : (x) < 0 ? -1 : 0);
  }
  
}

#endif

