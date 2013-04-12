/*
 *
 * Copyright (C) 2002-2005 Michael Droettboom and Robert Ferguson
 *               2009      Christoph Dalitz
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

#ifndef lcninja_edgedetect
#define lcninja_edgedetect

#include "gamera.hpp"
#include "vigra/edgedetection.hxx"
#include "logical.hpp"
#include "morphology.hpp"

namespace Gamera {

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

  template<class T>
  Image* labeled_region_edges(const T& src, bool mark_both=false) {
    OneBitImageData* edges_data = new OneBitImageData(src.size(), src.origin());
    OneBitImageView* edges = new OneBitImageView(*edges_data);
    size_t x,y,max_x,max_y;

    max_x = src.ncols()-1;
    max_y = src.nrows()-1;

    // the following mask is sufficient:  xx
    //                                    xx
    // because we assume that no pixel is unlabeled

    // check bulk of image
    for (y=0; y<max_y; ++y) {
      for (x=0; x<max_x; ++x) {
        if (src.get(Point(x,y)) != src.get(Point(x+1,y))) {
          edges->set(Point(x,y),1);
          if (mark_both)
            edges->set(Point(x+1,y),1);
        }
        if (src.get(Point(x,y)) != src.get(Point(x,y+1))) {
          edges->set(Point(x,y),1);
          if (mark_both)
            edges->set(Point(x,y+1),1);
        }
        if (src.get(Point(x,y)) != src.get(Point(x+1,y+1))) {
          edges->set(Point(x,y),1);
          if (mark_both)
            edges->set(Point(x+1,y+1),1);
        }
      }
    }
    // check last row
    for (x=0; x<max_x; ++x) {
      if (src.get(Point(x,max_y)) != src.get(Point(x+1,max_y))) {
        edges->set(Point(x,max_y),1);
        if (mark_both)
          edges->set(Point(x+1,max_y),1);
      }      
    }
    // check last column
    for (y=0; y<max_y; ++y) {
      if (src.get(Point(max_x,y)) != src.get(Point(max_x,y+1))) {
        edges->set(Point(max_x,y),1);
        if (mark_both)
          edges->set(Point(max_x,y+1),1);
      }
    }

    return edges;
  }


  template<class T>
  typename ImageFactory<T>::view_type* outline(const T& in) {
    //typedef typename ImageFactory<T>::data_type data_type;
    typedef typename ImageFactory<T>::view_type view_type;
    view_type* out = erode_dilate(in, 1, 0, 0);
    xor_image(*out, in);
    return out;
  }
  
}

#endif
