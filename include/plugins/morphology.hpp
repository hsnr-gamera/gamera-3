/*
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

/* Derived from 
   Klette, R. and P. Zamperoni.  1996. Handbook of Image Processing Operators.
   New York: John Wiley & Sons.
*/

#ifndef kwm12032001_erode_dilate
#define kwm12032001_erode_dilate

#include <vector>
#include <algorithm>
#include "gamera.hpp"
#include "neighbor.hpp"

using namespace std;

namespace Gamera {

  template<class T>
  class Max {
  public:
    inline T operator() (typename vector<T>::iterator begin,
			 typename vector<T>::iterator end);
  };

  template<class T>
  inline T Max<T>::operator() (typename vector<T>::iterator begin,
			       typename vector<T>::iterator end) {
    return *(max_element(begin, end));
  }

  inline OneBitPixel Max<OneBitPixel>::operator()
    (vector<OneBitPixel>::iterator begin,
     vector<OneBitPixel>::iterator end) {
    return *(min_element(begin, end));
  }

  template<class T>
  class Min {
  public:
    inline T operator() (typename vector<T>::iterator begin,
			 typename vector<T>::iterator end);
  };

  template<class T>
  inline T Min<T>::operator() (typename vector<T>::iterator begin,
			       typename vector<T>::iterator end) {
    return *(min_element(begin, end));
  }

  inline OneBitPixel Min<OneBitPixel>::operator()
    (vector<OneBitPixel>::iterator begin,
     vector<OneBitPixel>::iterator end) {
    return *(max_element(begin, end));
  }

  template<class T>
  void erode_dilate(T &m, unsigned int times, int direction, int geo) {
    if (m.nrows() < 3 || m.ncols() < 3)
      return;
    typedef typename T::value_type value_type;
    typedef ImageData<value_type> data_type;
    ImageData<value_type> mat_data(m.nrows(), m.ncols());
    ImageView<data_type> *result = 
      new ImageView<data_type>(mat_data, 0, 0, m.nrows(), m.ncols());
    Max<value_type> max;
    Min<value_type> min;
    unsigned int r, ngeo;
    bool n8;
    ngeo = 1;
    for (r = 1; r <= times; r++) {
      if (geo && (ngeo % 2 == 0))
	n8 = true;
      else
	n8 = false;
      if (direction) {
	if (n8)
	  neighbor4x(m, max, *result);
	else
	  neighbor9(m, max, *result);
      }
      else {
	if (n8)
	  neighbor4x(m, min, *result);
	else
	  neighbor9(m, min, *result);
      }
      ngeo++;
    }
    delete result;
  }

  template<class T>
  void erode(T& image) {
    erode_dilate(image, 1, 1, 0);
  }

  template<class T>
  void dilate(T& image) {
    erode_dilate(image, 1, 0, 0);
  }

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

  inline OneBitPixel Rank<OneBitPixel>::operator() (vector<OneBitPixel>::iterator begin,
						    vector<OneBitPixel>::iterator end) {
    nth_element(begin, end - rank, end);
    return *(end - rank);
  }

  template<class T>
  void rank(T &m, unsigned int r) {
    if (m.nrows() < 3 || m.ncols() < 3)
      return;
    Rank<typename T::value_type> rank(r);
    neighbor9<T, Rank<typename T::value_type> >(m, rank);
  }

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
  void mean(T &m) {
    if (m.nrows() < 3 || m.ncols() < 3)
      return;
    Mean<typename T::value_type> mean_op;
    neighbor9<T, Mean<typename T::value_type> >(m, mean_op);
  }
  
}
#endif
