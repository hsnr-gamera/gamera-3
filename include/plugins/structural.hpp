/*
 *
 * Copyright (C) 2001-2004
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

#ifndef mgd11272002_relational
#define mgd11272002_relational

#include "gamera.hpp"
#include <math.h>
#include <algorithm>

namespace Gamera {
  template<class T, class U>
  bool bounding_box_grouping_function(T& a, U& b, double threshold) {
    if (threshold < 0)
      throw std::runtime_error("Threshold must be a positive number.");
    size_t int_threshold = size_t(threshold + 0.5); // rounding
    return b->intersects(a->expand(int_threshold));
  }

  template<class T, class U>
  bool shaped_grouping_function(T& a, U& b, double threshold) {
    if (threshold < 0)
      throw std::runtime_error("Threshold must be a positive number.");

    size_t int_threshold = size_t(threshold + 0.5);

    T a_roi(a, a.intersection(b.expand(int_threshold)));
    if (a_roi.ul_x() > a_roi.lr_x() || a_roi.ul_y() > a_roi.lr_y())
      return false;

    U b_roi(b, b.intersection(a.expand(int_threshold)));
    if (b_roi.ul_x() > b_roi.lr_x() || b_roi.ul_y() > b_roi.lr_y())
      return false;

    double threshold_2 = threshold * threshold;
    long start_c, end_c, dir_c;
    long start_r, end_r, dir_r;

    if (b_roi.center_y() > a_roi.center_y()) {
      start_r = a_roi.nrows() - 1;
      end_r = -1;
      dir_r = -1;
    } else {
      start_r = 0;
      end_r = a_roi.nrows();
      dir_r = 1;
    }

    if (b_roi.center_x() > a_roi.center_x()) {
      start_c = a_roi.ncols() - 1;
      end_c = -1;
      dir_c = -1;
    } else {
      start_c = 0;
      end_c = a_roi.ncols();
      dir_c = 1;
    }
      
    for (long r = start_r; r != end_r; r += dir_r) {
      for (long c = start_c; c != end_c; c += dir_c) {
	if (is_black(a_roi.get(r, c))) {
	  bool is_edge = false;
	  if (r == 0l || (size_t)r == a_roi.nrows() - 1 || 
	      c == 0l || (size_t)c == a_roi.ncols() - 1) {
	    is_edge = true;
	  } else {
	    for (long ri = r - 1; ri < r + 2; ++ri) {
	      for (long ci = c - 1; ci < c + 2; ++ci) {
		if (is_white(a_roi.get(ri, ci))) {
		  is_edge = true;
		  break;
		}
	      }
	      if (is_edge)
		break;
	    }
	  }
	  if (is_edge) {
	    double a_y = double(r + a_roi.ul_y());
	    double a_x = double(c + a_roi.ul_x());
	    for (size_t r2 = 0; r2 < b_roi.nrows(); ++r2) {
	      for (size_t c2 = 0; c2 < b_roi.ncols(); ++c2) {
		if (is_black(b_roi.get(r2, c2))) {
		  double distance_y = double(r2 + b_roi.ul_y()) - a_y;
		  double distance_x = double(c2 + b_roi.ul_x()) - a_x;
		  double distance = distance_x*distance_x + distance_y*distance_y;
		  if (distance <= threshold_2)
		    return true;
		}
	      }
	    }
	  }
	}
      }
    }
    return false;
  }
  
  template<class T, class U>
  FloatVector *polar_distance(T &a, U &b) {
    FloatVector *result = new FloatVector(3);
    double x = (double)a.center_x() - (double)b.center_x();
    double y = (double)a.center_y() - (double)b.center_y();
    double r = sqrt(pow(x, 2.0) + pow(y, 2.0));
    double q;
    if (x == 0)
      q = M_PI / 2;
    else
      q = atan(y / x);
    if (y > 0)
      q += M_PI;
    double avg_diag = ((sqrt(pow(a.nrows(), 2.0) + pow(a.ncols(), 2.0)) +
			sqrt(pow(b.nrows(), 2.0) + pow(b.ncols(), 2.0))) / 2.0);
    (*result)[0] = r / avg_diag;
    (*result)[1] = q;
    (*result)[2] = r;
    return result;
  }
  
  int polar_match(double r1, double q1, double r2, double q2) {
    static const double ANGULAR_THRESHOLD = M_PI / 6.0;
    static const double DISTANCE_THRESHOLD = 1.6;
    double distance1, distance2;
    if (r1 > r2) {
      distance1 = r1;
      distance2 = r2;
    } else {
      distance1 = r2;
      distance2 = r1;
    }
    double q = fabs(q1 - q2);
    if (q1 > M_PI)
      q = std::min(q, fabs((M_PI - q1) - q2));
    if (q2 > M_PI)
      q = std::min(q, fabs((M_PI - q2) - q1));
    return (q < ANGULAR_THRESHOLD && (distance1 / distance2) < DISTANCE_THRESHOLD);
  }
}
#endif
