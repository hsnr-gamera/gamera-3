/*
 *
 * Copyright (C) 2002 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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

#ifndef kwm10092002_logical
#define kwm10092002_logical

#include "gamera.hpp"
#include "accessor.hpp"
#include <exception>

namespace Gamera {

template<class T, class U>
void and_image(T& a, const U& b) {
  size_t ul_y = std::max(a.ul_y(), b.ul_y());
  size_t ul_x = std::max(a.ul_x(), b.ul_x());
  size_t lr_y = std::min(a.lr_y(), b.lr_y());
  size_t lr_x = std::min(a.lr_x(), b.lr_x());

  if (ul_y >= lr_y || ul_x >= lr_x)
    return;
  for (size_t y = ul_y, ya = y-a.ul_y(), yb=y-b.ul_y(); y <= lr_y; ++y, ++ya, ++yb)
    for (size_t x = ul_x, xa = x-a.ul_x(), xb=x-b.ul_x(); x <= lr_x; ++x, ++xa, ++xb) {
      if (is_black(a.get(ya, xa)) && is_black(b.get(yb, xb))) 
	a.set(ya, xa, black(a));
      else
	a.set(ya, xa, white(a));
    }
}

template<class T, class U>
void or_image(T& a, const U& b) {
  size_t ul_y = std::max(a.ul_y(), b.ul_y());
  size_t ul_x = std::max(a.ul_x(), b.ul_x());
  size_t lr_y = std::min(a.lr_y(), b.lr_y());
  size_t lr_x = std::min(a.lr_x(), b.lr_x());

  if (ul_y >= lr_y || ul_x >= lr_x)
    return;
  for (size_t y = ul_y, ya = y-a.ul_y(), yb=y-b.ul_y(); y <= lr_y; ++y, ++ya, ++yb)
    for (size_t x = ul_x, xa = x-a.ul_x(), xb=x-b.ul_x(); x <= lr_x; ++x, ++xa, ++xb) {
      if (is_black(a.get(ya, xa)) || is_black(b.get(yb, xb))) {
	a.set(ya, xa, black(a));
      } else
	a.set(ya, xa, white(a));
    }
}

template<class T, class U>
void xor_image(T& a, const U& b) {
  size_t ul_y = std::max(a.ul_y(), b.ul_y());
  size_t ul_x = std::max(a.ul_x(), b.ul_x());
  size_t lr_y = std::min(a.lr_y(), b.lr_y());
  size_t lr_x = std::min(a.lr_x(), b.lr_x());
  
  if (ul_y >= lr_y || ul_x >= lr_x)
    return;
  for (size_t y = ul_y, ya = y-a.ul_y(), yb=y-b.ul_y(); y <= lr_y; ++y, ++ya, ++yb) {
    for (size_t x = ul_x, xa = x-a.ul_x(), xb=x-b.ul_x(); x <= lr_x; ++x, ++xa, ++xb) {
      if (is_black(a.get(ya, xa)) ^ is_black(b.get(yb, xb))) 
	a.set(ya, xa, black(a));
      else
	a.set(ya, xa, white(a));
    }

  }
}

}
#endif
