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

#ifndef mgd12032001_draw_hpp
#define mgd12032001_draw_hpp

template<class T>
void draw_line(T& image, size_t y1, size_t x1, size_t y2, size_t x2,
	       double pixel, double alpha = 1.0) {
  // Breshenham's Algorithm
  
  typename T::value_type value = (typename T::value_type)pixel;
  int x_dist = int(x2) - int(x1);
  int y_dist = int(y2) - int(y1);
  int x_dist_abs = abs(x_dist);
  int y_dist_abs = abs(y_dist);

  if (x_dist_abs > y_dist_abs) { // x is controlling axis
    int y_sign = 1 | (y_dist >> (sizeof(int) * 8 - 1));
    if (x1 > x2) 
      std::swap(x1, x2);
    int e = y_dist_abs - x_dist_abs;
    size_t y = y1;
    for (size_t x = x1; x <= x2; ++x, e += y_dist_abs) {
      image.set(y, x, value);
      if (e > 0.0) {
	y += y_sign;
	e -= x_dist_abs;
      }
    }
  } else {
    int x_sign = 1 | (x_dist >> (sizeof(int) * 8 - 1));
    if (y1 > y2) 
      std::swap(y1, y2);
    int e = x_dist_abs - y_dist_abs;
    size_t x = x1;
    for (size_t y = y1; y <= y2; ++y, e += x_dist_abs) {
      image.set(y, x, value);
      if (e > 0.0) {
	x += x_sign;
	e -= y_dist_abs;
      }
    }
  }
}

template<class T>
void draw_hollow_rect(T& image, size_t y1_, size_t x1_, size_t y2_, size_t x2_,
		      double pixel) {
  typename T::value_type value = (typename T::value_type)pixel;

  size_t x1, x2, y1, y2;
  if (x1_ > x2_)
    x1 = x2_, x2 = x1_;
  else
    x1 = x1_, x2 = x2_;

  if (y1_ > y2_)
    y1 = y2_, y2 = y1_;
  else
    y1 = y1_, y2 = y2_;

  for (size_t x = x1; x < x2; ++x) {
    image.set(y1, x, value);
    image.set(y2, x, value);
  }

  for (size_t y = y1; y < y2; ++y) {
    image.set(y, x1, value);
    image.set(y, x2, value);
  }
}

template<class T>
void draw_filled_rect(T& image, size_t y1_, size_t x1_, size_t y2_, size_t x2_,
		      double pixel) {
  typename T::value_type value = (typename T::value_type)pixel;

  size_t x1, x2, y1, y2;
  if (x1_ > x2_)
    x1 = x2_, x2 = x1_;
  else
    x1 = x1_, x2 = x2_;

  if (y1_ > y2_)
    y1 = y2_, y2 = y1_;
  else
    y1 = y1_, y2 = y2_;

  for (size_t y = y1; y < y2; ++y) 
    for (size_t x = x1; x < x2; ++x)
      image.set(y, x, value);
}

#endif
