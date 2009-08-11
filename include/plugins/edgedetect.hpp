/*
 *
 * Copyright (C) 2002-2005 Michael Drottboom and Robert Ferguson
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

#ifndef lcninja_edgedetect
#define lcninja_edgedetect

#include "gamera.hpp"
#include "vigra/edgedetection.hxx"

using namespace Gamera;

template<class T>
typename ImageFactory<T>::view_type* difference_of_exponential_edge_image(const T& src, double scale, double gradient_threshold, unsigned int min_edge_length) {
  if ((scale < 0) || (gradient_threshold < 0))
    throw std::runtime_error("The scale and gradient_threshold must be greater than 0");

  typename ImageFactory<T>::data_type* dest_data =
    new typename ImageFactory<T>::data_type(src.size(), src.origin());

  typename ImageFactory<T>::view_type* dest =
    new typename ImageFactory<T>::view_type(*dest_data);

  try {
    vigra::differenceOfExponentialEdgeImage(src_image_range(src), dest_image(*dest), scale, gradient_threshold);
    
    if (min_edge_length > 0)
      vigra::removeShortEdges(dest_image_range(*dest), min_edge_length, NumericTraits<typename T::value_type>::one());
  } catch (std::exception e) {
    delete dest;
    delete dest_data;
    throw;
  }
  return dest;
}

template<class T>
typename ImageFactory<T>::view_type* difference_of_exponential_crack_edge_image(const T& src, double scale, double gradient_threshold, unsigned int min_edge_length, unsigned int close_gaps, unsigned int beautify) {
  if ((scale < 0) || (gradient_threshold < 0))
    throw std::runtime_error("The scale and gradient threshold must be greater than 0");

  typename ImageFactory<T>::data_type* dest_data =
    new typename ImageFactory<T>::data_type(Dim(src.ncols() * 2, src.nrows() * 2), src.origin());

  typename ImageFactory<T>::view_type* dest =
    new typename ImageFactory<T>::view_type(*dest_data);

  try {
    vigra::differenceOfExponentialCrackEdgeImage(src_image_range(src), dest_image(*dest), scale, gradient_threshold, NumericTraits<typename T::value_type>::one());
    
    if (min_edge_length > 0)
      vigra::removeShortEdges(dest_image_range(*dest), min_edge_length, NumericTraits<typename T::value_type>::one());
    
    if (close_gaps)
      vigra::closeGapsInCrackEdgeImage(dest_image_range(*dest), NumericTraits<typename T::value_type>::one());
    
    if (beautify)
      vigra::beautifyCrackEdgeImage(dest_image_range(*dest), NumericTraits<typename T::value_type>::one(), NumericTraits<typename T::value_type>::zero());
  } catch (std::exception e) {
    delete dest;
    delete dest_data;
    throw;
  }
  return dest;
}

template<class T>
typename ImageFactory<T>::view_type* canny_edge_image(const T& src, double scale, double gradient_threshold) {
  if ((scale < 0) || (gradient_threshold < 0))
    throw std::runtime_error("The scale and gradient threshold must be >= 0");

  typename ImageFactory<T>::data_type* dest_data =
    new typename ImageFactory<T>::data_type(src.size(), src.origin());

  typename ImageFactory<T>::view_type* dest =
    new typename ImageFactory<T>::view_type(*dest_data, src);

  try {
    vigra::cannyEdgeImage(src_image_range(src), dest_image(*dest), scale, gradient_threshold, NumericTraits<typename T::value_type>::one());
  } catch (std::exception e) {
    delete dest;
    delete dest_data;
    throw;
  }
  return dest;
}

#endif
