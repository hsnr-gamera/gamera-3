/*
 *
 * Copyright (C) 2001 - 2002
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

#define TWO_PI M_PI * 2

template<class T, class U>
FloatVector *polar_distance(T &a, U &b) {
  FloatVector *result = new FloatVector(3);
  double x = (double)a.center_x() - (double)b.center_x();
  double y = (double)a.center_y() - (double)b.center_y();
  double r = sqrt(x*x + y*y);
  double q;
  if (x == 0)
    q = M_PI_2;
  else
    q = atan(y / x);
  if (y > 0)
    q += M_PI;
  double avg_diag = ((sqrt(a.nrows()*a.nrows() + a.ncols()*a.ncols()) +
		      sqrt(b.nrows()*b.nrows() + b.ncols()*b.ncols())) / 2.0);
  (*result)[0] = r / avg_diag;
  (*result)[1] = q;
  (*result)[2] = r;
  return result;
}

#define ANGULAR_THRESHOLD M_PI / 6.0
#define DISTANCE_THRESHOLD 1.6

int polar_match(double r1, double q1, double r2, double q2) {
  double distance1 = MAX(r1, r2);
  double distance2 = MIN(r1, r2);
  double q = abs(q1 - q2);
  if (q1 > M_PI)
    q = MIN(q, abs((M_PI - q1) - q2));
  if (q2 > M_PI)
    q = MIN(q, abs((M_PI - q2) - q1));
  return (q < ANGULAR_THRESHOLD && (distance1 / distance2) < DISTANCE_THRESHOLD);
}

#endif
