/*
 *
 * Copyright (C) 2001 - 2003
 * Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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

#ifndef kwm02212003_projections
#define kwm02212003_projections

#include "gamera.hpp"

namespace Gamera {

  /*
    Generic projection routine - x and y projections
    are acheived by passing in either row or col
    iterators.
  */
  template<class T>
  inline IntVector* projection(T i, const T end) {
    IntVector* proj = new IntVector(end - i, 0);
    typename T::iterator j;
    typename IntVector::iterator p = proj->begin();
    for (; i != end; ++i, ++p) {
      for (j = i.begin(); j != i.end(); ++j) {
	if (is_black(*j))
	    *p += 1;
      }
    }
    return proj;
  }

  /*
    Projection along the y axis (rows) of an image.
  */
  template<class T>
  IntVector* projection_rows(const T& image) {
    return projection(image.row_begin(), image.row_end());
  }

  /*
    Projection along the y axis (rows) of a portion
    on an image.    
  */
  template<class T>
  IntVector* projection_rows(const T& image, const Rect& rect) {
    T proj_image(image, rect);
    return projection_rows(proj_image);
  }

  /*
    Projection along the x axis (rows) of an image.
  */
  template<class T>
  IntVector* projection_cols(const T& image) {
    return projection(image.col_begin(), image.col_end());
  }

  /*
    Projection along the y axis (rows) of a portion
    on an image.    
  */
  template<class T>
  IntVector* projection_cols(const T& image, const Rect& rect) {
    T proj_image(image, rect);
    return projection_cols(proj_image);
  }

  /*
    Projections of strips of a image -
    the coordinates are relative to the view.
  */
  template<class T>
  IntVector* yproj_vertical_strip(T& image, size_t offset_x,
				  size_t width) {
    Rect r(image.offset_y(), image.offset_x() + offset_x,
	   image.nrows(), width);
    return projection_rows(image, r);
  }
  
  template<class T>
  IntVector* yproj_horizontal_strip(T& image, size_t offset_y,
				    size_t height) {
    Rect r(image.offset_y() + offset_y, image.offset_x(),
	   height, image.ncols());
    return projection_rows(image, r);
  }

  template<class T>
  IntVector* xproj_vertical_strip(T& image, size_t offset_x,
				  size_t width) {
    Rect r(image.offset_y(), image.offset_x() + offset_x,
	   image.nrows(), width);
    return projection_cols(image, r);
  }

  template<class T>
  IntVector* xproj_horizontal_strip(T& image, size_t offset_y,
				    size_t height) {
    Rect r(image.offset_y() + offset_y, image.offset_x(),
	   height, image.ncols());
    return projection_cols(image, r);
  }

  /*
    returns y-projections of a rotated image
  */
  template<class T>
  void projection_skewed_cols(const T& image, FloatVector* angles, std::vector<IntVector*>& proj) {
    int x;
    size_t i;
    size_t n = angles->size();

    FloatVector sina(n);
    FloatVector cosa(n);
    for (i = 0; i < n; i++) {
      sina[i] = sin((*angles)[i] * M_PI / 180.0);
      cosa[i] = cos((*angles)[i] * M_PI / 180.0);
    }

    for (i = 0; i < n; i++)
      proj[i] = new IntVector(image.ncols(), 0);

    // compute skewed projections simultanously
    for (size_t r = 0; r < image.nrows(); ++r) {
      for (size_t c = 0; c < image.ncols(); ++c) {
        if (is_black(image.get(r,c))) {
          for (i = 0; i < n; i++) {
            x = (int) round(c*cosa[i] - r*sina[i]);
            if ((x > 0) && (x < (int)image.ncols()))
              ++(*(proj[i]))[x];
          }
        }
      }
    }
  }

  // The Python part
  template<class T>
  PyObject* projection_skewed_cols(const T& image, FloatVector* angles) {
    size_t n = angles->size();
    std::vector<IntVector*> proj(n);
    projection_skewed_cols(image, angles, proj);

    PyObject* projlist = PyList_New(n);  
    // move projections to return list
    for (size_t i = 0; i < n; i++)
      PyList_SET_ITEM(projlist, i, IntVector_to_python(proj[i]));
    return projlist;
  }

  /*
    returns x-projections of a rotated image
  */
  template<class T>
  void projection_skewed_rows(const T& image, FloatVector* angles, 
			      std::vector<IntVector*>& proj) {
    int y;
    size_t i;
    size_t n = angles->size();

    FloatVector sina(n);
    FloatVector cosa(n);
    for (i = 0; i < n; i++) {
      sina[i] = sin((*angles)[i] * M_PI / 180.0);
      cosa[i] = cos((*angles)[i] * M_PI / 180.0);
    }

    for (i = 0; i < n; i++)
      proj[i] = new IntVector(image.nrows(), 0);

    // compute skewed projections simultanously
    for (size_t r = 0; r < image.nrows(); ++r) {
      for (size_t c = 0; c < image.ncols(); ++c) {
        if (is_black(image.get(r,c))) {
          for (i = 0; i < n; i++) {
            y = (int) round(c*sina[i] + r*cosa[i]);
            if ((y > 0) && (y < (int)image.nrows()))
              ++(*(proj[i]))[y];
          }
        }
      }
    }
  }

  // The Python part
  template<class T>
  PyObject* projection_skewed_rows(const T& image, FloatVector* angles) {
    size_t n = angles->size();
    std::vector<IntVector*> proj(n);
    projection_skewed_rows(image, angles, proj);

    PyObject* projlist = PyList_New(n);  
    // move projections to return list
    for (size_t i = 0; i < n; i++)
      PyList_SET_ITEM(projlist, i, IntVector_to_python(proj[i]));
    return projlist;
  }
}

#endif
