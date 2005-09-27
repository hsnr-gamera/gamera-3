/*
 *
 * Copyright (C) 2005 Michael Droettboom
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

#ifndef mgd_floatpoint
#define mgd_floatpoint

#include "gamera.hpp"
#include <math.h>

template<class T>
class PointBase {
private:
  T m_x, m_y;
public:
  typedef PointBase<T> self;
  
  // Default constructor
  inline PointBase() : m_x(), m_y() {}

  // Basic constructor
  inline PointBase(T x, T y) : m_x(x), m_y(y) {}

  // From standard Gamera point
  inline PointBase(const Gamera::Point& p) : m_x(T(p.x())), m_y(T(p.y())) {}

  // Copy constructor
  inline PointBase(const self& p) : m_x(p.x()), m_y(p.y()) {}

  // Return the x coordinate
  inline T x() const {
    return m_x;
  }

  // Return the y coordinate
  inline T y() const {
    return m_y;
  }

  // Equality
  template<class OtherPoint>
  inline bool operator==(const OtherPoint& p) const {
    T e = std::numeric_limits<T>::epsilon();
    return (std::abs(m_x - T(p.x())) < e &&
	    std::abs(m_y - T(p.y())) < e);
  }

  // Inequality
  template<class OtherPoint>
  inline bool operator!=(const OtherPoint& p) const {
    return (m_x != T(p.x()) || m_y != T(p.y()));
  }

  // Addition
  template<class OtherPoint>
  inline self operator+(const OtherPoint& p) const {
    return self(m_x + T(p.x()), m_y + T(p.y()));
  } 

  // Subtraction
  template<class OtherPoint>
  inline self operator-(const OtherPoint& p) const {
    return self(m_x - T(p.x()), m_y - T(p.y()));
  } 

  // Multiplication
  template<class OtherPoint>
  inline self operator*(const OtherPoint& p) const {
    return self(m_x * T(p.x()), m_y * T(p.y()));
  } 

  // Division
  template<class OtherPoint>
  inline self operator/(const OtherPoint& p) const {
    return self(m_x / T(p.x()), m_y / T(p.y()));
  } 

  // Unary minus
  inline self operator-() const {
    return self(-m_x, -m_y);
  }

  // Unary plus
  inline self operator+() const {
    return self(+m_x, +m_y);
  }
  
  // Euclidean distance
  template<class OtherPoint>
  inline T distance(const OtherPoint& p) const {
    T x_dist = m_x - T(p.x());
    T y_dist = m_y - T(p.y());
    return std::sqrt(x_dist*x_dist + y_dist*y_dist);
  }
};

template<class T>
inline static PointBase<T> abs(PointBase<T> p) {
  return PointBase<T>(std::abs(p.x()), std::abs(p.y()));
}

template<class T>
std::ostream& operator<<(std::ostream& s, const PointBase<T>& point) {
  return s << "FloatPoint(" << point.x() << ", " << point.y() << ")";
}

typedef PointBase<double> FloatPoint;

#endif
