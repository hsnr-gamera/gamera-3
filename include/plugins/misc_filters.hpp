/*
 *
 * Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom,
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

#ifndef kwm11062002_misc_filters
#define kwm11062002_misc_filters

#include "gamera.hpp"
#include "image_utilities.hpp"
#include "neighbor.hpp"
#include "vigra/gaborfilter.hxx"

using namespace std;

namespace Gamera {

  //---------------------------
  // mean filter
  //---------------------------
  template<class T>
  class Mean {
  public:
    inline T operator() (typename vector<T>::iterator begin,
			 typename vector<T>::iterator end);
  };

  template<class T>
  inline T Mean<T>::operator() (typename vector<T>::iterator begin,
				typename vector<T>::iterator end) {
    long sum = 0;
    size_t size = end - begin;
    for (; begin != end; ++begin)
      sum += size_t(*begin);
    return T(sum / size);
  }

  template<class T>
  typename ImageFactory<T>::view_type* mean(T &m) {
    typedef typename ImageFactory<T>::data_type data_type;
    typedef typename ImageFactory<T>::view_type view_type;
    if (m.nrows() < 3 || m.ncols() < 3)
      return simple_image_copy(m);

    data_type* new_data = new data_type(m.size(), m.origin());
    view_type* new_view = new view_type(*new_data);

    try {
      Mean<typename T::value_type> mean_op;
      neighbor9(m, mean_op, *new_view);
    } catch (std::exception e) {
      delete new_view;
      delete new_data;
      throw;
    }
    return new_view;
  }

  //---------------------------
  // rank filter
  //---------------------------
  template<class T>
  class Rank {
    unsigned int rank;
  public:
    Rank<T>(unsigned int rank_) { rank = rank_ - 1; }
    inline T operator() (typename vector<T>::iterator begin,
			 typename vector<T>::iterator end);
  };

  template<class T>
  inline T Rank<T>::operator() (typename vector<T>::iterator begin,
				typename vector<T>::iterator end) {
    nth_element(begin, begin + rank, end);
    return *(begin + rank);
  }

  template<>
  inline OneBitPixel Rank<OneBitPixel>::operator() (vector<OneBitPixel>::iterator begin,
						    vector<OneBitPixel>::iterator end) {
    nth_element(begin, end - rank, end);
    return *(end - rank);
  }

  template<class T>
  typename ImageFactory<T>::view_type* rank(const T &m, unsigned int r) {
    typedef typename ImageFactory<T>::data_type data_type;
    typedef typename ImageFactory<T>::view_type view_type;
    if (m.nrows() < 3 || m.ncols() < 3)
      return simple_image_copy(m);

    data_type* new_data = new data_type(m.size(), m.origin());
    view_type* new_view = new view_type(*new_data);

    try {
      Rank<typename T::value_type> rank(r);
      neighbor9(m, rank, *new_view);
    } catch (std::exception e) {
      delete new_view;
      delete new_data;
      throw;
    }
    return new_view;
  }

  //---------------------------
  // Gabor filter
  //---------------------------
  template<class T>
  Image* create_gabor_filter(const T& src, double orientation, double frequency, int direction) {

    FloatImageData* dest_data = new FloatImageData(src.size(), src.origin());
    FloatImageView* dest = new FloatImageView(*dest_data);

    image_copy_fill(src, *dest);

    try {
      vigra::createGaborFilter(dest_image_range(*dest), orientation, frequency,
			       vigra::angularGaborSigma(direction, frequency),
			       vigra::radialGaborSigma(frequency));
      
    } catch(std::exception e) {
      delete dest;
      delete dest_data;
      throw std::runtime_error("VIGRA function 'createGaborFilter' failed!");
    }

    return dest;

  }

}

#endif

