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
