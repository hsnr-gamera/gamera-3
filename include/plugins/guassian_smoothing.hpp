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

#ifndef kwm03072002_guassian_smoothing
#define kwm03072002_guassian_smoothing

#include "gamera.hpp"
#include "vigra/convolution.hxx"
#include "image_utilities.hpp"

template<class T>
typename ImageFactory<T>::view_type* gaussian_smoothing(const T& src, double scale) {
  typename ImageFactory<T>::data_type* dest_data =
    new typename ImageFactory<T>::data_type(src.size(), src.offset_y(), src.offset_x());
  typename ImageFactory<T>::view_type* dest =
    new typename ImageFactory<T>::view_type(*dest_data, src);
  gaussianSmoothing(src_image_range(src), dest_image(*dest), scale);
  return dest; 
}

#endif
