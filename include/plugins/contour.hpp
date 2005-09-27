/*
 *
 * Copyright (C) 2001-2005
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

#ifndef mgd10222004_contours
#define mgd10222004_contours

#include "gamera.hpp"

namespace Gamera {
  template<class T>
  FloatVector* contour_top(const T& m) {
    FloatVector* output = new FloatVector(m.ncols());
    try {
      for (size_t c = 0; c != m.ncols(); ++c) {
	size_t r = 0;
	for (; r != m.nrows(); ++r)
	  if (is_black(m.get(Point(c, r))))
	    break;
	double result;
	if (r >= m.nrows())
	  result = std::numeric_limits<double>::infinity();
	else
	  result = (double)r;
	(*output)[c] = result;
      }
    } catch (std::exception e) {
      delete output;
      throw;
    }
    return output;
  }

  template<class T>
  FloatVector* contour_bottom(const T& m) {
    FloatVector* output = new FloatVector(m.ncols());
    try {
      for (size_t c = 0; c != m.ncols(); ++c) {
	long r = m.nrows() - 1;
	for (; r >= 0; --r)
	  if (is_black(m.get(Point(c, r))))
	    break;
	double result;
	if (r < 0)
	  result = std::numeric_limits<double>::infinity();
	else
	  result = (double)(m.nrows() - r);
	(*output)[c] = result;
      }
    } catch (std::exception e) {
      delete output;
      throw;
    }
    return output;
  }

  template<class T>
  FloatVector* contour_left(const T& m) {
    FloatVector* output = new FloatVector(m.nrows());
    try {
      for (size_t r = 0; r != m.nrows(); ++r) {
	size_t c = 0;
	for (; c != m.ncols(); ++c)
	  if (is_black(m.get(Point(c, r))))
	    break;
	double result;
	if (c >= m.ncols())
	  result = std::numeric_limits<double>::infinity();
	else
	  result = (double)c;
	(*output)[r] = result;
      }
    } catch (std::exception e) {
      delete output;
      throw;
    }
    return output;
  }

  template<class T>
  FloatVector* contour_right(const T& m) {
    FloatVector* output = new FloatVector(m.nrows());
    try {
      for (size_t r = 0; r != m.nrows(); ++r) {
	long c = m.ncols() - 1;
	for (; c >= 0; --c)
	  if (is_black(m.get(Point(c, r))))
	    break;
	double result;
	if (c < 0)
	  result = std::numeric_limits<double>::infinity();
	else
	  result = (double)(m.ncols() - c);
	(*output)[r] = result;
      }
    } catch (std::exception e) {
      delete output;
      throw;
    }
    return output;
  }
}
#endif
