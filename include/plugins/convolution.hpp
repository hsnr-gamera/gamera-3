/*
 *
 * Copyright (C) 2001-2004 Ichiro Fujinaga, Michael Droettboom,
 * and Karl MacMillan
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

#ifndef mgd_convolution
#define mgd_convolution

#include "gamera.hpp"
#include "vigra/stdconvolution.hxx"

using namespace Gamera;

template<class T, class U>
typename ImageFactory<T>::view_type* convolve(T& src, const U& k, int border_mode) {
  if (k.nrows() > src.nrows() || k.ncols() > src.ncols())
    throw std::runtime_error("The image must be bigger than the kernel.");

  typename ImageFactory<T>::data_type* dest_data =
    new typename ImageFactory<T>::data_type(src.size(), src.offset_y(), 
					    src.offset_x());
  typename ImageFactory<T>::view_type* dest =
    new typename ImageFactory<T>::view_type(*dest_data, src);

  // I originally had the following two lines abstracted out in a function,
  // but that seemed to choke and crash gcc 3.3.2
  typename U::ConstIterator center = k.upperLeft() + Diff2D(k.center_x(), k.center_y());
  tuple5<
    typename U::ConstIterator,
    typename choose_accessor<U>::accessor,
    Diff2D, Diff2D, BorderTreatmentMode> kernel
    (center, choose_accessor<U>::make_accessor(k), 
     Diff2D(-k.center_x(), -k.center_y()),
     Diff2D(k.width() - k.center_x(), k.height() - k.center_y()),
     (BorderTreatmentMode)border_mode);

  vigra::convolveImage(src_image_range(src), dest_image(*dest), kernel); 
  return dest;
}

template<class T, class U>
typename ImageFactory<T>::view_type* convolve_x(T& src, const U& k, int border_mode) {
  if (k.nrows() > src.nrows() || k.ncols() > src.ncols())
    throw std::runtime_error("The image must be bigger than the kernel.");
  if (k.nrows() != 1)
    throw std::runtime_error("The 1D kernel must have only one row.");

  typename ImageFactory<T>::data_type* dest_data =
    new typename ImageFactory<T>::data_type(src.size(), src.offset_y(), 
					    src.offset_x());
  typename ImageFactory<T>::view_type* dest =
    new typename ImageFactory<T>::view_type(*dest_data, src);

  // I originally had the following two lines abstracted out in a function,
  // but that seemed to choke and crash gcc 3.3.2
  typename U::const_vec_iterator center = k.vec_begin() + k.center_x();
  tuple5<
    typename U::const_vec_iterator,
    typename choose_accessor<U>::accessor,
    int, int, BorderTreatmentMode> kernel
    (center, choose_accessor<U>::make_accessor(k), 
     -int(k.center_x()), int(k.width()) - int(k.center_x()),
     (BorderTreatmentMode)border_mode);

  vigra::separableConvolveX(src_image_range(src), dest_image(*dest), kernel); 
  return dest;
}

template<class T, class U>
typename ImageFactory<T>::view_type* convolve_y(T& src, const U& k, int border_mode) {
  if (k.nrows() > src.ncols() || k.ncols() > src.nrows())
    throw std::runtime_error("The image must be bigger than the kernel.");
  if (k.nrows() != 1)
    throw std::runtime_error("The 1D kernel must have only one row.");

  typename ImageFactory<T>::data_type* dest_data =
    new typename ImageFactory<T>::data_type(src.size(), src.offset_y(), 
					    src.offset_x());
  typename ImageFactory<T>::view_type* dest =
    new typename ImageFactory<T>::view_type(*dest_data, src);

  // I originally had the following two lines abstracted out in a function,
  // but that seemed to choke and crash gcc 3.3.2
  typename U::const_vec_iterator center = k.vec_begin() + k.center_x();
  tuple5<
    typename U::const_vec_iterator,
    typename choose_accessor<U>::accessor,
    int, int, BorderTreatmentMode> kernel
    (center, choose_accessor<U>::make_accessor(k), 
     -int(k.center_x()), int(k.width()) - int(k.center_x()),
     (BorderTreatmentMode)border_mode);

  vigra::separableConvolveY(src_image_range(src), dest_image(*dest), kernel); 
  return dest;
}

#endif
