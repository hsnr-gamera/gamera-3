/*
 *
 * Copyright (C) 2001-2009
 * Ichiro Fujinaga, Michael Droettboom, Karl MacMillan,
 * and Christoph Dalitz
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

    Rect r = a.intersection(b.expand(int_threshold));
    if (r.ul_x() > r.lr_x() || r.ul_y() > r.lr_y())
      return false;
    T a_roi(a, r);

    r = b.intersection(a.expand(int_threshold));
    if (r.ul_x() > r.lr_x() || r.ul_y() > r.lr_y())
      return false;
    U b_roi(b, r);

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

    // Yes, that's right: a goto statement.
    // According to Stroustrup "C++ Programming Language, 3rd ed.":
    //   "One of the few sensible uses of goto in ordinary code is to
    //   break out from a nested loop or switch-statement."

    for (long r = start_r; r != end_r; r += dir_r) {
      for (long c = start_c; c != end_c; c += dir_c) {
	if (is_black(a_roi.get(Point(c, r)))) {
	  bool is_edge = false;
	  if (r == 0l || (size_t)r == a_roi.nrows() - 1 ||
	      c == 0l || (size_t)c == a_roi.ncols() - 1) {
	    is_edge = true;
	    goto edge_found;
	  } else {
	    for (long ri = r - 1; ri < r + 2; ++ri) {
	      for (long ci = c - 1; ci < c + 2; ++ci) {
		if (is_white(a_roi.get(Point(ci, ri)))) {
		  is_edge = true;
		  goto edge_found;
		}
	      }
	    }
	  }
	  continue;
	edge_found:
	  double a_y = double(r + a_roi.ul_y());
	  double a_x = double(c + a_roi.ul_x());
	  for (size_t r2 = 0; r2 < b_roi.nrows(); ++r2) {
	    for (size_t c2 = 0; c2 < b_roi.ncols(); ++c2) {
	      if (is_black(b_roi.get(Point(c2, r2)))) {
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
    return false;
  }

  template<class T, class U>
  FloatVector *polar_distance(T &a, U &b) {
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
    FloatVector *result = new FloatVector(3);
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

#define ITMAX 100
#define EPS 3.0e-7
#define FPMIN 1.0e-30

  // From Numerical Recipes in C
  double gammln(const double xx) {
    static double cof[6] = {76.18009172947146,-86.50532032941677,
			    24.01409824083091,-1.231739572450155,
			    0.1208650973866179e-2,-0.5395239384953e-5};
    double x, y;
    x = y = xx;
    double tmp = x + 5.5;
    tmp -= (x + 0.5) * log(tmp);
    double ser = 1.000000000190015;
    for (size_t i = 0; i < 6; ++i) {
      y += 1.0;
      ser += cof[i] / y;
    }
    return -tmp / log(2.5066282746310005 * ser / x);
  }

  // From Numerical Recipes in C
  void gser(const double a, const double x, double& gamser, double& gln) {
    gln = gammln(a);
    if (x < 0.0)
      throw std::range_error("x less than 0.0 in argument to gser");
    else if (x == 0) {
      gamser = 0.0;
      return;
    }

    double ap = a;
    double delta, sum;
    delta = sum = 1.0 / a;
    for (size_t i = 0; i < ITMAX; ++i) {
      ap += 1;
      delta *= x / ap;
      sum += delta;
      if (fabs(delta) < fabs(sum) * EPS) {
	gamser = sum * exp(-x + a * log(x) - gln);
	return;
      }
    }
    throw std::range_error("a too large to compute in gser.");
  }

  // From Numerical Recipes in C
  void gcf(const double a, const double x, double& gammcf, double& gln) {
    gln = gammln(a);
    double b, c, d, h;
    b = x + 1.0 - a;
    c = 1.0 / FPMIN;
    h = d = 1.0 / b;
    double i = 1;
    for (; i <= ITMAX; ++i) {
      double an = -i * (i - a);
      b += 2.0;
      d = an * d + b;
      if (fabs(d) < FPMIN)
	d = FPMIN;
      c = b + an / c;
      if (fabs(c) < FPMIN)
	c = FPMIN;
      d = 1.0 / d;
      double delta = d * c;
      h *= delta;
      if (fabs(delta - 1.0) < EPS)
	break;
    }
    if (i > ITMAX)
      throw std::runtime_error("a too large in gcf.");
    try {
      gammcf = exp(-x + a * log(x) - gln) * h;
    } catch (std::overflow_error) {
      gammcf = std::numeric_limits<double>::max();
    }
  }

  // From Numerical Recipes in C
  double gammq(const double a, const double x) {
    double gamser, gln;
    if (x < 0.0 || a <= 0.0)
      throw std::range_error("Invalid arguments to gammq.");
    if (x < a + 1.0) {
      gser(a, x, gamser, gln);
      return 1.0 - gamser;
    } else {
      gcf(a, x, gamser, gln);
      return gamser;
    }
  }

  // From Numerical Recipes in C
  void least_squares_fit(const PointVector& points, double& a, double& b, double& q) {
    if (points.size() == 1) {
      a = 0.0;
      b = points[0].x();
      q = 1.0;
      return;
    }

    double sx, sy, st2, sxoss, chi2;
    sx = sy = st2 = a = b = chi2 = 0.0;

    for (PointVector::const_iterator i = points.begin(); i != points.end(); ++i) {
      sx += double((*i).x());
      sy += double((*i).y());
    }

    sxoss = sx / points.size();

    for (PointVector::const_iterator i = points.begin(); i != points.end(); ++i) {
      double t = double((*i).x()) - sxoss;
      st2 += t * t;
      b += t * double((*i).y());
    }

    b /= st2;
    a = (sy - sx * b) / points.size();

    for (PointVector::const_iterator i = points.begin(); i != points.end(); ++i) {
      double tmp = (double((*i).y()) - a - b * double((*i).x()));
      chi2 += tmp * tmp;
    }

    q = 1.0;

    if (points.size() >= 3) {
      try {
	q = gammq(0.5 * (points.size() - 2), 0.5 * chi2);
      } catch (std::exception) {
	;
      }
    }
  }

  PyObject* least_squares_fit(const PointVector* points) {
    double a, b, q;
    least_squares_fit(*points, a, b, q);
    return Py_BuildValue(CHAR_PTR_CAST "fff", b, a, q);
  }

  PyObject* least_squares_fit_xy(const PointVector* points) {
    double a, b, q;
    int x_of_y = 0;
    size_t xmin, xmax, ymin, ymax;
    PointVector::const_iterator p;
    p = points->begin();
    xmin = xmax = p->x(); ymin = ymax = p->y();
    for (p = points->begin() + 1; p != points->end(); ++p) {
      if (p->x() > xmax) xmax = p->x();
      if (p->x() < xmin) xmin = p->x();
      if (p->y() > ymax) ymax = p->y();
      if (p->y() < ymin) ymin = p->y();
    }
    if ((xmax-xmin) > (ymax-ymin)) {
      // line closer to horizontal
      least_squares_fit(*points, a, b, q);
    } else {
      // line closer to vertical
      PointVector transposed;
      for (p=points->begin(); p!=points->end(); ++p) {
        transposed.push_back(Point(p->y(),p->x()));
      }
      least_squares_fit(transposed, a, b, q);
      x_of_y = 1;
    }

    return Py_BuildValue(CHAR_PTR_CAST "fffi", b, a, q, x_of_y);
  }

  // straightforward implementation of Levenshtein's classic algorithm
  int edit_distance(std::string s1, std::string s2)
  {
    size_t s1len, s2len;       // length of the two strings
    IntVector *prev, *curr;    // previous and current matrix column
    IntVector *tmp;
    size_t result, add, del, sub;
    size_t i,j;
  
    s1len = s1.size(); s2len = s2.size();
    if (s1len == 0) return s2len;
    if (s2len == 0) return s1len;

    prev = new IntVector(s1len+1);
    curr = new IntVector(s1len+1);
    for (i=0; i<s1len+1; i++) (*prev)[i] = i;

    for (j=1; j<s2len+1; j++) {
      if (j>1) {
        // move one column further in evaluation matrix
        tmp = prev;
        prev = curr;
        curr = tmp;
      }
      (*curr)[0] = j;
      for (i=1; i<s1len+1; i++) {
        // cost of different transformation operations
        if (s1[i-1] == s2[j-1])
          sub = (*prev)[i-1];
        else
          sub = (*prev)[i-1] + 1;
        add = (*prev)[i] + 1;
        del = (*curr)[i-1] + 1;
        (*curr)[i] = std::min(sub, std::min(add,del));
      }
    }

    result = (*curr)[s1len];
    delete prev; delete curr;
    return result;
  }

}
#endif
