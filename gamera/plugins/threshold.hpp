/*
 *
 * Copyright (C) 2001 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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

#ifndef kwm12032001_threshold
#define kwm12032001_threshold

#include "gamera.hpp"

using namespace Gamera;

template<class T>
Image* threshold(const T &m, int threshold) {
  typename T::data_type* data = new typename T::data_type(m.size(), m.ul_y(), m.ul_x());
  typedef typename T::value_type PIXEL;
  typedef typename image_factory<T>::view_type O;
  O* out = new O(*data, m);

  typename T::const_vec_iterator i = m.vec_begin();
  typename T::const_vec_iterator end = m.vec_end();
  typename O::vec_iterator j = out->vec_begin();
  
  for (; i != end; i++, j++)
    if (*i > PIXEL(threshold))
      *j = white(*out);
    else
      *j = black(*out);
  return out;
}

#endif


