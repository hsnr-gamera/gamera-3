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

#ifndef __image_algorithm_hh__
#define __image_algorithm_hh__

#include <algorithm>
#include <vector>

#include "gamera.hpp"

/*
  Image Algorithm

  This file contains a variety of utility algorithms for Gamera matrices.

  Author
  ------
  Karl MacMillan karlmac@peabody.jhu.edu

  History
  -------
  - Started 6/12/01

*/

namespace Gamera {

  // Print a image to the console
  template<class T>
  void print_image(const T& image) {
    typename T::const_row_iterator i = image.row_begin();
    typename T::const_row_iterator::iterator j;
    std::cout << "[" << std::endl;
    for (; i != image.row_end(); i++) {
      j = i.begin();
      for (; j != i.end(); j++) {
	std::cout << *j << " ";
      }
      std::cout << std::endl;
    }
    std::cout << "]" << std::endl;
  }

  // Shear a single column or row
  template<class T>
  inline void simple_shear(T begin, const T end, int distance) {
    // short-circuit
    if (distance == 0)
      return;
    typename T::value_type filler;
    // move down or right
    if (distance > 0) {
      filler = *begin;
      std::copy_backward(begin + distance, end - distance, end);
      std::fill(begin, begin + distance, filler);
      // move up or left
    } else {
      filler = *(end - 1);
      std::copy(begin - distance, end, begin);
      std::fill(end + distance, end, filler);
    }
  }

  // filter based on run-length
  template<class Iter>
  inline void filter_long_run(Iter i, const Iter end,
			      const int min_length) {
    while (i != end) {
      if (is_black(*i)) {
	Iter last = i;
	black_run_end(i, end);
	if (i - last > min_length)
	  std::fill(last, i, white(i));
      } else {
	white_run_end(i, end);
      }
    }
  }

  template<class Iter>
  inline void filter_short_run(Iter i, const Iter end,
			       const int max_length) {
    while (i != end) {
      if (is_black(*i)) {
	Iter last = i;
	black_run_end(i, end);
	if (i - last < max_length)
	  std::fill(last, i, white(i));
      } else {
	white_run_end(i, end);
      }
    }
  }

  template<class Iter>
  inline void image_filter_long_run(Iter i, const Iter end,
			      const int min_length) {
    for (; i != end; i++)
      filter_long_run(i.begin(), i.end(), min_length);
  }

  template<class Iter>
  inline void image_filter_short_run(Iter i, const Iter end,
			       const int max_length) {
    for (; i != end; i++)
      filter_short_run(i.begin(), i.end(), max_length);
  }

};

#endif
