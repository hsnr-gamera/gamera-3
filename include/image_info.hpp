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

#ifndef kwm01082002_image_info
#define kwm01082002_image_info

#include <cstdlib>

namespace Gamera {

  /// A simple class to hold information about images
  /**
     This is a simple class to hold information about images - it is to
     make the writing of image loading and saving functions easier.  Only the
     functions for reading the information are exported to python - in C++ the
     member variables are set directly for ease in the python export code.
   */
  class ImageInfo {
  public:
    ImageInfo() {
      m_x_resolution = 0; m_y_resolution = 0;
      m_nrows = 0; m_ncols = 0;
      m_depth = 0;
      m_ncolors = 0;
      m_inverted = false;
    }
    double x_resolution() { return m_x_resolution; }
    double y_resolution() { return m_y_resolution; }
    size_t nrows() { return m_nrows; }
    size_t ncols() { return m_ncols; }
    size_t depth() { return m_depth; }
    size_t ncolors() { return m_ncolors; }
    bool inverted() { return m_inverted; }
    void x_resolution(double res) { m_x_resolution = res; }
    void y_resolution(double res) { m_y_resolution = res; }
    void nrows(size_t x) { m_nrows = x; }
    void ncols(size_t x) { m_ncols = x; }
    void depth(size_t x) { m_depth = x; }
    void ncolors(size_t x) { m_ncolors = x; }
    void inverted(bool x) { m_inverted = x; }
  public:
    double m_x_resolution, m_y_resolution;
    size_t m_nrows, m_ncols;
    size_t m_depth;
    size_t m_ncolors;
    bool m_inverted;
  };

};

#endif
