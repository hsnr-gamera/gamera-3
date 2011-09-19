/*
 *
 * Copyright (C) 2001-2005
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef kwm12032001_utility
#define kwm12032001_utility

#include "gamera.hpp"
using namespace Gamera;

template<class T>
Image* image_copy(const T &a, int storage_format) {
  if (storage_format == DENSE) {
  typename T::const_vec_iterator ait = a.vec_begin();
  typename V::vec_iterator bit = b.vec_begin();
  for (; ait != a.vec_end(); ++ait, ++bit)
    *bit = typename V::value_type(*ait);
}

#endif
