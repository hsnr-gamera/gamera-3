/*
 *
 * Copyright (C) 2001-2009
 * Ichiro Fujinaga, Michael Droettboom, Karl MacMillan, and Christoph Dalitz
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

#ifndef mgd12032001_draw_hpp
#define mgd12032001_draw_hpp

#include <stack>

namespace Gamera {

  template<class T>
  inline void _clip_points(T& image, size_t& x1, size_t& y1, size_t& x2, size_t& y2) {
    x1 -= image.ul_x();
    x2 -= image.ul_x();
    y1 -= image.ul_y();
    y2 -= image.ul_y();
    x1 = std::min(x1, image.ncols() - 1);
    x2 = std::min(x2, image.ncols() - 1);
    y1 = std::min(y1, image.nrows() - 1);
    y2 = std::min(y2, image.nrows() - 1);
  }

  // computation entry point of line in image
  inline void _cut_line(double &x1, double &y1, double &x2, double &y2, 
                        double x_len, double y_len, double lower, double upper) {
    if (y1 < lower) {
      x1 += (-y1 * x_len) / y_len;
      y1 = 0;
    }

    if (y2 > upper) {
      x2 += (-(y2 - upper) * x_len) / y_len;
      y2 = upper;
    }
  }

  // straightforward implementation of Bresenham's line drawing algorithm
  template<class T, class P>
  void _draw_line(T& image, const P& a, const P& b, 
                  const typename T::value_type value) {
    double x1 = double(a.x());
    double y1 = double(a.y());
    double x2 = double(b.x());
    double y2 = double(b.y());

    y1 -= (double)image.ul_y();
    y2 -= (double)image.ul_y();
    x1 -= (double)image.ul_x();
    x2 -= (double)image.ul_x();

    double y_len = y2 - y1;
    double x_len = x2 - x1;

    // Short circuit for a single pixel.  This speeds up
    // drawing Bezier curves a little bit
    if (((int)y_len) == 0 && ((int)x_len) == 0) {
      if (y1 >= 0 && y1 < image.nrows() &&
          x1 >= 0 && x1 < image.ncols())
        image.set(Point((size_t)x1, (size_t)y1), value);
      return;
    }

    // Cut the line so it doesn't go outside of the image bounding box.
    // It is more efficient to do a little math now than to test when 
    // writing each pixel.
    if (y_len > 0)
      _cut_line(x1, y1, x2, y2, x_len, y_len, 
                0.0, (double)image.nrows() - 1);
    else
      _cut_line(x2, y2, x1, y1, x_len, y_len, 
                0.0, (double)image.nrows() - 1);

    if (x_len > 0)
      _cut_line(y1, x1, y2, x2, y_len, x_len, 
                0.0, (double)image.ncols() - 1);
    else
      _cut_line(y2, x2, y1, x1, y_len, x_len, 
                0.0, (double)image.ncols() - 1);

    if (!(y1 >= 0 && y1 < image.nrows() &&
          x1 >= 0 && x1 < image.ncols() &&
          y2 >= 0 && y2 < image.nrows() &&
          x2 >= 0 && x2 < image.ncols())) {
      return;
    }

    int x_dist = int(x2) - int(x1);
    int y_dist = int(y2) - int(y1);
    int x_dist_abs = abs(x_dist);
    int y_dist_abs = abs(y_dist);

    if (x_dist_abs > y_dist_abs) { // x is controlling axis
      if (x1 > x2) {
        std::swap(x1, x2);
        std::swap(y1, y2);
      }
      int y_sign = Gamera::sign((int)y2 - (int)y1);
      int e = y_dist_abs - x_dist_abs;
      int y = y1;
      for (int x = x1; x <= (int)x2; ++x, e += y_dist_abs) {
        image.set(Point(x, y), value);
        if (e >= 0.0) {
          y += y_sign;
          e -= x_dist_abs;
        }
      }
    } else {
      if (y1 > y2) {
        std::swap(x1, x2);
        std::swap(y1, y2);
      }
      int x_sign = Gamera::sign(int(x2) - int(x1));
      int e = x_dist_abs - y_dist_abs;
      int x = x1;
      for (int y = y1; y <= (int)y2; ++y, e += x_dist_abs) {
        image.set(Point(x, y), value);
        if (e >= 0.0) {
          x += x_sign;
          e -= y_dist_abs;
        }
      }
    }

  }

  template<class T, class P>
  void draw_line(T& image, const P& a, const P& b, 
                 const typename T::value_type value, const double thickness=1.0) {
    const double half_thickness = (thickness - 1.0) / 2.0;
    for (double x = -half_thickness; x <= 0.0; x += 1.0) 
      for (double y = -half_thickness; y <= 0.0; y += 1.0) 
        _draw_line(image, P((double)a.x()+x, (double)a.y()+y), P((double)b.x()+x, (double)b.y()+y), value);
  
    for (double x = half_thickness; x >= 0.0; x -= 1.0) 
      for (double y = half_thickness; y >= 0.0; y -= 1.0) 
        _draw_line(image, P((double)a.x()+x, (double)a.y()+y), P((double)b.x()+x, (double)b.y()+y), value);

    _draw_line(image, a, b, value);
  }

  template<class T, class P>
  void draw_hollow_rect(T& image, const P& a, const P& b, 
                        const typename T::value_type value,
                        const double thickness = 1.0) {
    draw_line(image, a, P(a.x(), b.y()), value, thickness);
    draw_line(image, a, P(b.x(), a.y()), value, thickness);
    draw_line(image, b, P(b.x(), a.y()), value, thickness);
    draw_line(image, b, P(a.x(), b.y()), value, thickness);
  }

  template<class T>
  void draw_hollow_rect(T& image, const Rect& r, const typename T::value_type value) {
    draw_hollow_rect(image, r.ul(), r.lr(), value);
  }

  template<class T, class P>
  void draw_filled_rect(T& image, const P& a, const P& b, const typename T::value_type value) {
    size_t x1, y1, x2, y2;
    size_t x1_ = (size_t)a.x();
    size_t y1_ = (size_t)a.y();
    size_t x2_ = (size_t)b.x();
    size_t y2_ = (size_t)b.y();

    _clip_points(image, x1_, y1_, x2_, y2_);

    if (x1_ > x2_)
      x1 = x2_, x2 = x1_;
    else
      x1 = x1_, x2 = x2_;

    if (y1_ > y2_)
      y1 = y2_, y2 = y1_;
    else
      y1 = y1_, y2 = y2_;

    for (size_t y = y1; y <= y2; ++y) 
      for (size_t x = x1; x <= x2; ++x)
        image.set(Point(x, y), value);
  }

  template<class T>
  void draw_filled_rect(T& image, const Rect& r, const typename T::value_type value) {
    draw_filled_rect(image, r.ul(), r.lr(), value);
  }

  template<class T, class P>
  void draw_marker(T& image, const P& p, const size_t size, const size_t style, 
                   const typename T::value_type value) {
    int half_size = (int)ceil(double(size) / 2.0);
    switch (style) {
    case 0:
      draw_line(image, P(p.x(), p.y() - half_size), P(p.x(), p.y() + half_size), value);
      draw_line(image, P(p.x() - half_size, p.y()), P(p.x() + half_size, p.y()), value);
      break;
    case 1:
      draw_line(image, P(p.x() - half_size, p.y() - half_size), 
                P(p.x() + half_size, p.y() + half_size), value);
      draw_line(image, P(p.x() + half_size, p.y() - half_size), 
                P(p.x() - half_size, p.y() + half_size), value);
      break;
    case 2:
      draw_hollow_rect(image, P(p.x() - half_size, p.y() - half_size), 
                       P(p.x() + half_size, p.y() + half_size), value);
      break;
    case 3: {
      int leftx = std::max((int)p.x() - half_size, 0);
      int rightx = std::min((int)p.x() + half_size, (int)image.ncols()-1);
      int topy = std::max((int)p.y() - half_size, 0);
      int boty = std::min((int)p.y() + half_size, (int)image.nrows()-1);
      draw_filled_rect(image, P(leftx, topy), P(rightx, boty), value);
      break;
    }
    default:
      throw std::runtime_error("Invalid style.");
    }
  }

  inline double square(double a) {
    return a * a;
  }

  template<class T, class P>
  void draw_bezier(T& image, const P& start, const P& c1, const P& c2, 
                   const P& end, const typename T::value_type value,
                   const double thickness = 1.0, const double accuracy = 0.1) {
    double start_x = double(start.x());
    double start_y = double(start.y());
    double c1_x = double(c1.x());
    double c1_y = double(c1.y());
    double c2_x = double(c2.x());
    double c2_y = double(c2.y());
    double end_x = double(end.x());
    double end_y = double(end.y());

    // All of this is just to calculate epsilon given the accuracy and
    // the length and "waviness" of the curve
    double dd0 = square(start_x - 2*c1_x + c2_x) + square(start_y - 2*c1_y + c2_y);
    double dd1 = square(c1_x - 2*c2_x + end_x) + square(c1_y - 2*c2_y + end_y);
    double dd = 6.0 * sqrt(std::max(dd0, dd1));
    double e2 = 8.0 * accuracy <= dd ? 8 * accuracy / dd : 1.0;
    double epsilon = sqrt(e2);

    double y = start_y;
    double x = start_x;
    for (double a = 1.0, b = 0.0; a > 0.0; a -= epsilon, b += epsilon) {
      double a_3 = a * a * a;
      double a_2_b = a * a * b * 3.0;
      double b_3 = b * b * b;
      double b_2_a = b * b * a * 3.0;

      double new_x = start_x*a_3 + c1_x*a_2_b + c2_x*b_2_a + end_x*b_3;
      double new_y = start_y*a_3 + c1_y*a_2_b + c2_y*b_2_a + end_y*b_3;
      draw_line(image, P(x, y), P(new_x, new_y), value, thickness);
      y = new_y; x = new_x;
    }
    draw_line(image, P(x, y), end, value, thickness);
  }

  template<class T, class P>
  void draw_circle(T& image, const P& c, const double r, const typename T::value_type value,
                   const double thickness = 1.0, const double accuracy = 0.1) {
    static const double kappa = 4.0 * ((sqrt(2.0) - 1.0) / 3.0);
  
    // Bezier circle approximation from 
    // http://www.whizkidtech.redprince.net/bezier/circle/
    const double z = kappa * r;

    draw_bezier(image, P(c.x(), c.y() - r), P(c.x() + z, c.y() - r),
                P(c.x() + r, c.y() - z), P(c.x() + r, c.y()), 
                value, thickness, accuracy);
    draw_bezier(image, P(c.x() + r, c.y()), P(c.x() + r, c.y() + z),
                P(c.x() + z, c.y() + r), P(c.x(), c.y() + r),
                value, thickness, accuracy);
    draw_bezier(image, P(c.x(), c.y() + r), P(c.x() - z, c.y() + r),
                P(c.x() - r, c.y() + z), P(c.x() - r, c.y()),
                value, thickness, accuracy);
    draw_bezier(image, P(c.x() - r, c.y()), P(c.x() - r, c.y() - z),
                P(c.x() - z, c.y() - r), P(c.x(), c.y() - r),
                value, thickness, accuracy);
  }

  /* From John R. Shaw's QuickFill code which is based on
     "An Efficient Flood Visit Algorithm" by Anton Treuenfels,
     C/C++ Users Journal Vol 12, No. 8, Aug 1994 */

  template<class T>
  struct FloodFill {
    typedef std::stack<Point> Stack;

    inline static void travel(T& image, Stack& s,
                              const typename T::value_type& interior, 
                              const typename T::value_type& color,
                              const size_t left, const size_t right,
                              const size_t y) {
      if (left + 1 <= right) {
        typename T::value_type col1, col2;
        for (size_t x = left + 1; x <= right; ++x) {
          col1 = image.get(Point(x-1, y));
          col2 = image.get(Point(x, y));
          if (col1 == interior && col2 != interior) {
            s.push(Point(x-1, y));
          }
        }
        if (col2 == interior) {
          s.push(Point(right, y));
        }
      }
    }

    static void fill_seeds(T& image, Stack& s, 
                           const typename T::value_type& interior, 
                           const typename T::value_type& color) {
      typedef typename T::value_type pixel_t;
      size_t left, right;
      while (!s.empty()) {
        Point p = s.top();
        s.pop();
        if (image.get(p) == interior) {
          for (right = p.x();
               right < image.ncols();
               ++right) {
            if (image.get(Point(right, p.y())) != interior)
              break;
            image.set(Point(right, p.y()), color);
          }
          --right;

          long int l = p.x() - 1;
          for (; l >= 0; --l) {
            if (image.get(Point(l, p.y())) != interior)
              break;
            image.set(Point(l, p.y()), color);
          }
          left = (size_t)l + 1;

          if (left != right) {
            if (p.y() < image.nrows() - 1)
              travel(image, s, interior, color, left, right, p.y() + 1);
            if (p.y() > 0)
              travel(image, s, interior, color, left, right, p.y() - 1);
          } else {
            if (p.y() < image.nrows() - 1)
              if (image.get(Point(left, p.y() + 1)) != color)
                s.push(Point(left, p.y() + 1));
            if (p.y() > 1)
              if (image.get(Point(left, p.y() - 1)) != color)
                s.push(Point(left, p.y() - 1));
          }
        }
      }
    }
  };

  template<class T, class P>
  void flood_fill(T& image, const P& p, const typename T::value_type& color) {
    double x = double(p.x()) - double(image.ul_x());
    double y = double(p.y()) - double(image.ul_y());
    if (y >= image.nrows() || x >= image.ncols())
      throw std::runtime_error("Coordinate out of range.");
    typename T::value_type interior = image.get(Point((size_t)x, (size_t)y));
    if (color == interior)
      return;
    typename FloodFill<T>::Stack s;
    s.push(Point((size_t)x, (size_t)y));
    FloodFill<T>::fill_seeds(image, s, interior, color);
  }

  template<class T>
  void remove_border(T& image) {
    size_t bottom = image.nrows() - 1;
    size_t right = image.ncols() - 1;
    for (size_t x = 0; x < image.ncols(); ++x) {
      if (image.get(Point(x, 0)) != 0)
        flood_fill(image, Point(x, 0), white(image));
      if (image.get(Point(x, bottom)) != 0)
        flood_fill(image, Point(x, bottom), white(image));
    }
    for (size_t y = 0; y < image.nrows(); ++y) {
      if (image.get(Point(0, y)) != 0)
        flood_fill(image, Point(0, y), white(image));
      if (image.get(Point(right, y)) != 0)
        flood_fill(image, Point(right, y), white(image));
    }
  }

  template<class T, class U>
  void highlight(T& a, const U& b, const typename T::value_type& color) {
    size_t ul_y = std::max(a.ul_y(), b.ul_y());
    size_t ul_x = std::max(a.ul_x(), b.ul_x());
    size_t lr_y = std::min(a.lr_y(), b.lr_y());
    size_t lr_x = std::min(a.lr_x(), b.lr_x());
  
    if (ul_y > lr_y || ul_x > lr_x)
      return;
    for (size_t y = ul_y, ya = y-a.ul_y(), yb=y-b.ul_y(); 
         y <= lr_y; ++y, ++ya, ++yb)
      for (size_t x = ul_x, xa = x-a.ul_x(), xb=x-b.ul_x(); 
           x <= lr_x; ++x, ++xa, ++xb) {
        if (is_black(b.get(Point(xb, yb)))) 
          a.set(Point(xa, ya), color);
      }
  }

} // namespace Gamera

#endif


