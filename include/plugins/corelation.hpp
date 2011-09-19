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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef mgd06292004_corelation
#define mgd06292004_corelation

#include "gamera.hpp"

namespace Gamera {

  template<class T, class U>
  double corelation_weighted(const T& a, const U& b, const Point& p, double bb, double bw, double wb, double ww) {
    size_t ul_y = std::max(a.ul_y(), p.y());
    size_t ul_x = std::max(a.ul_x(), p.x());
    size_t lr_y = std::min(a.lr_y(), p.y() + b.nrows());
    size_t lr_x = std::min(a.lr_x(), p.x() + b.ncols());
    double result = 0;
    double area = 0;

    for (size_t y = ul_y, ya = ul_y-a.ul_y(), yb = ul_y-p.y(); y < lr_y; ++y, ++ya, ++yb) {
      for (size_t x = ul_x, xa = ul_x-a.ul_x(), xb = ul_x-p.x(); x < lr_x; ++x, ++xa, ++xb) {
	if (is_black(b.get(Point(xb, yb)))) {
	  area++;
	  if (is_black(a.get(Point(xa, ya))))
	    result += bb;
	  else
	    result += bw;
	} else {
	  if (is_black(a.get(Point(xa, ya))))
	    result += wb;
	  else
	    result += ww;
	}
      }
    }
    return result / area;
  }


  inline double corelation_absolute_distance(OneBitPixel a, OneBitPixel b) {
    if (is_black(a) == is_black(b))
      return 0.0;
    return 1.0;
  }

  inline double corelation_absolute_distance(GreyScalePixel a, OneBitPixel b) {
    if (is_black(b))
      return (double)a;
    return (double)(NumericTraits<GreyScalePixel>::max() - a);
  }

  template<class T, class U>
  double corelation_sum(const T& a, const U& b, const Point& p, 
			ProgressBar progress_bar = ProgressBar()) {
    size_t ul_y = std::max(a.ul_y(), p.y());
    size_t ul_x = std::max(a.ul_x(), p.x());
    size_t lr_y = std::min(a.lr_y(), p.y() + b.nrows());
    size_t lr_x = std::min(a.lr_x(), p.x() + b.ncols());
    double result = 0;
    double area = 0;

    progress_bar.set_length(lr_y - ul_y);
    for (size_t y = ul_y, ya = ul_y-a.ul_y(), yb = ul_y-p.y(); y < lr_y; ++y, ++ya, ++yb) {
      for (size_t x = ul_x, xa = ul_x-a.ul_x(), xb = ul_x-p.x(); x < lr_x; ++x, ++xa, ++xb) {
	typename T::value_type px_a = a.get(Point(xb, yb));
	typename U::value_type px_b = b.get(Point(xb, yb));
	if (is_black(px_b))
	  area++;
	result += corelation_absolute_distance(px_a, px_b);
      }
      progress_bar.step();
    }
    return result / area;
  }

  inline double corelation_square_absolute_distance(OneBitPixel a, OneBitPixel b) {
    if (is_black(a) == is_black(b))
      return 0.0;
    return 1.0;
  }

  inline double corelation_square_absolute_distance(GreyScalePixel a, OneBitPixel b) {
    double result = 0;
    if (is_black(a))
      result = a;
    else
      result = (double)(NumericTraits<GreyScalePixel>::max() - a);
    return result * result;
  }

  template<class T, class U>
  double corelation_sum_squares(const T& a, const U& b, const Point& p, ProgressBar progress_bar = ProgressBar()) {
    size_t ul_y = std::max(a.ul_y(), p.y());
    size_t ul_x = std::max(a.ul_x(), p.x());
    size_t lr_y = std::min(a.lr_y(), p.y() + b.nrows());
    size_t lr_x = std::min(a.lr_x(), p.x() + b.ncols());
    double result = 0;
    double area = 0;

    progress_bar.set_length(lr_y - ul_y);
    for (size_t y = ul_y, ya = ul_y-a.ul_y(), yb = ul_y-p.y(); y < lr_y; ++y, ++ya, ++yb) {
      for (size_t x = ul_x, xa = ul_x-a.ul_x(), xb = ul_x-p.x(); x < lr_x; ++x, ++xa, ++xb) {
	typename T::value_type px_a = a.get(Point(xb, yb));
	typename U::value_type px_b = b.get(Point(xb, yb));
	if (is_black(px_b))
	  area++;
	result += corelation_square_absolute_distance(px_a, px_b);
      }
      progress_bar.step();
    }
    return result / area;
  }

}

#endif
