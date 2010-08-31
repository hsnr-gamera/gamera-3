/*
 *
 * Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, Karl MacMillan
 *               2010      Oliver Christen, Christoph Dalitz
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

  template<class T>
  PointVector * contour_samplepoints(const T& cc, int percentage) {
    PointVector *output = new PointVector();
    PointVector *contour_points = new PointVector();
    PointVector::iterator found;

    FloatVector *top = contour_top(cc);
    FloatVector *right = contour_right(cc);
    FloatVector *bottom = contour_bottom(cc);
    FloatVector *left = contour_left(cc);
    FloatVector::iterator it;

    int x, y, i;
    float d;

    unsigned int top_d = std::numeric_limits<unsigned int>::max() ;
    unsigned int top_max_x = 0;
    unsigned int top_max_y = 0;

    unsigned int right_d = std::numeric_limits<unsigned int>::max();
    unsigned int right_max_x = 0;
    unsigned int right_max_y = 0;

    unsigned int bottom_d = std::numeric_limits<unsigned int>::max();
    unsigned int bottom_max_x = 0;
    unsigned int bottom_max_y = 0;

    unsigned int left_d = std::numeric_limits<unsigned int>::max();
    unsigned int left_max_x = 0; 
    unsigned int left_max_y = 0;

    // top
    i = 0;for(it = top->begin() ; it != top->end() ; it++, i++) {
	    if( *it == std::numeric_limits<double>::infinity() ) {
		    continue;
	    }
	    d = *it;
	    x = cc.offset_x() + i;
	    y = cc.offset_y() + d;
	    if( d < top_d) {
		    top_d = d;
		    top_max_x = x;
		    top_max_y = y;	
	    }
	    found = find(contour_points->begin(), contour_points->end(), Point(x,y));
	    if(found == contour_points->end()) {
          contour_points->push_back( Point(x,y) );
	    }
    }
    // right
    i = 0;for(it = right->begin() ; it != right->end() ; it++, i++) {
      if( *it == std::numeric_limits<double>::infinity() ) {
        continue;
      }
      d = *it;
      x = cc.offset_x() + cc.ncols() - d;
      y = cc.offset_y() + i;
      if( d < right_d) {
        right_d = d;
        right_max_x = x;
        right_max_y = y;
      }
      found = find(contour_points->begin(), contour_points->end(), Point(x,y));
      if(found == contour_points->end()) {
        contour_points->push_back( Point(x,y) );
      }
    }
    // bottom
    i = 0;for(it = bottom->begin() ; it != bottom->end() ; it++, i++) {
      if( *it == std::numeric_limits<double>::infinity() ) {
        continue;
      }
      d = *it;
      x = cc.offset_x() + i;
      y = cc.offset_y() + cc.nrows() - d;
      if( d <= bottom_d) {
        bottom_d = d;
        bottom_max_x = x;
        bottom_max_y = y;
      }
      found = find(contour_points->begin(), contour_points->end(), Point(x,y));
      if(found == contour_points->end()) {
        contour_points->push_back( Point(x,y) );
      }
    }
    // left
    i = 0;for(it = left->begin() ; it != left->end() ; it++, i++) {
      if( *it == std::numeric_limits<double>::infinity() ) {
        continue;
      }
      d = *it;
      x = cc.offset_x() + d;
      y = cc.offset_y() + i;
      if( d <= left_d) {
        left_d = d;
        left_max_x = x;
        left_max_y = y;
      }
      found = find(contour_points->begin(), contour_points->end(), Point(x,y));
      if(found == contour_points->end()) {
        contour_points->push_back( Point(x,y) );
      }
    }

    // add only every 100/percentage-th point
    double delta = 100.0/percentage;
    double step = 0.0;
    unsigned int offset = 0; // to avoid overflow and rounding errors
    unsigned int ii = 0;
    while (ii < contour_points->size()) {
      output->push_back( (*contour_points)[ii] );
      step += delta;
      if (step > 100.0) {
        step -= 100.0;
        offset += 100;
      }
      ii = offset + (unsigned int)step;
    }

    // add the four outer extreme points ...
    // ... top
    if (top_d != std::numeric_limits<unsigned int>::max()) {
      found = find(output->begin(), output->end(), Point(top_max_x, top_max_y));
      if(found == output->end()) {
        output->push_back( Point(top_max_x, top_max_y) );
      }
    }
    // ... right
    if (right_d != std::numeric_limits<unsigned int>::max()) {
      found = find(output->begin(), output->end(), Point(right_max_x, right_max_y));
      if(found == output->end()) {
        output->push_back( Point(right_max_x, right_max_y) );
      }
    }
    // ... bottom
    if (bottom_d != std::numeric_limits<unsigned int>::max()) {
      found = find(output->begin(), output->end(), Point(bottom_max_x, bottom_max_y));
      if(found == output->end()) {
        output->push_back( Point(bottom_max_x, bottom_max_y) );
      }
    }
    // ... left
    if (left_d != std::numeric_limits<unsigned int>::max()) {
      found = find(output->begin(), output->end(), Point(left_max_x, left_max_y));
      if(found == output->end()) {
        output->push_back( Point(left_max_x, left_max_y) );
      }
    }

    delete top;
    delete right;
    delete bottom;
    delete left;
    delete contour_points;

    return output;
  }

}
#endif
