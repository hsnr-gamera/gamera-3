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
void and_image(T& a, U& b) {
  if (a.nrows() != b.nrows() || a.ncols() != b.ncols())
    throw std::range_error("Dimensions must match!");
  typename T::vec_iterator it_a, end;
  typename U::vec_iterator it_b;
  for (it_a = a.vec_begin(), end = a.vec_end(), it_b = b.vec_begin();
       it_a != end; ++it_a, ++it_b) {
    if (is_black(*it_a) && is_black(*it_b))
      *it_a = black(a);
    else
      *it_a = white(a);
  }
}

template<class T, class U>
void or_image(T& a, U& b) {
  if (a.nrows() != b.nrows() || a.ncols() != b.ncols())
    throw std::range_error("Dimensions must match!");
  typename T::vec_iterator it_a, end;
  typename U::vec_iterator it_b;
  for (size_t r = 0; r < a.nrows(); ++r) {
    for (size_t c = 0; c < a.ncols(); ++c) {
      if (is_black(a.get(r, c)) || is_black(b.get(r, c)))
	a.set(r, c, black(a));
      else
	a.set(r, c, white(a));
    }
  }
}

template<class T, class U>
void xor_image(T &a, const U &b) {
  if (a.nrows() != b.nrows() || a.ncols() != b.ncols())
    throw std::range_error("Dimensions must match!");
  typename T::vec_iterator it_a, end;
  typename U::const_vec_iterator it_b;
  for (it_a = a.vec_begin(), end = a.vec_end(), it_b = b.vec_begin();
       it_a != end; ++it_a, ++it_b) {
    if (is_black(*it_a) ^ is_black(*it_b))
      *it_a = black(a);
    else
      *it_a = white(a);
  }
}
}
#endif
