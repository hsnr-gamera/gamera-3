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

#ifndef __matrix_algorithm_hh__
#define __matrix_algorithm_hh__

#include <algorithm>
#include <vector>

#include "gamera.hpp"

/*
  Matrix Algorithm

  This file contains a variety of utility algorithms for Gamera matrices.

  Author
  ------
  Karl MacMillan karlmac@peabody.jhu.edu

  History
  -------
  - Started 6/12/01

*/

namespace Gamera {

  // Print a matrix to the console
  template<class T>
  void print_matrix(const T& mat) {
    typename T::const_row_iterator i = mat.row_begin();
    typename T::const_row_iterator::iterator j;
    std::cout << "[" << std::endl;
    for (; i != mat.row_end(); i++) {
      j = i.begin();
      for (; j != i.end(); j++) {
	std::cout << *j << " ";
      }
      std::cout << std::endl;
    }
    std::cout << "]" << std::endl;
  }

  /*
    Various statistics about runs
  */

  template<class T>
  inline void black_run_end(T& i, const T end) {
    for (; i != end; ++i) {
      if (is_white(*i))
	break;
    }
  }
	
  template<class T>
  inline void white_run_end(T& i, const T end) {
    for (; i != end; ++i) {
      if (is_black(*i))
	break;
    }
  }
  
  /*
    These functions find the length of the largest run in a a row
    or column of a matrix.
  */
  template<class T>
  inline size_t max_black_run(T i, const T end) {
    size_t max = 0;
    while (i != end) {
      if (is_black(*i)) {
	T last = i;
	black_run_end(i, end);
	size_t cur_length = i - last;
	if (cur_length > max)
	  max = cur_length;
      } else {
	white_run_end(i, end);
      }
    }
    return max;
  }
	
  template<class T>
  inline size_t max_white_run(T i, const T end) {
    size_t max = 0;
    while (i != end) {
      if (is_white(*i)) {
	T last = i;
	white_run_end(i, end);
	size_t cur_length = i - last;
	if (cur_length > max)
	  max = cur_length;
      } else {
	black_run_end(i, end);
      }
    }
    return max;
  }

  /*
    Run-length histograms. These make a histogram of the lenght of the runs
    in an image. They take an iterator range and a random-access container
    for the result (that should be appropriately sized). The histogram vector
    is not filled with zeros so that successive calls can be made to this
    algorithm with the same vector to do the histogram of an entire image. KWM
  */
  template<class T, class Vec>
  inline void black_run_histogram(T i, const T end, Vec& hist) {
    while (i != end) {
      if (is_black(*i)) {
	T last = i;
	black_run_end(i, end);
	size_t cur_length = i - last;
	hist[cur_length]++;
      } else {
	white_run_end(i, end);
      }
    }
  }

  template<class T, class Vec>
  inline void white_run_histogram(T i, const T end, Vec& hist) {
    while (i != end) {
      if (is_white(*i)) {
	T last = i;
	white_run_end(i, end);
	size_t cur_length = i - last;
	hist[cur_length]++;
      } else {
	black_run_end(i, end);
      }
    }
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
  inline void matrix_filter_long_run(Iter i, const Iter end,
			      const int min_length) {
    for (; i != end; i++)
      filter_long_run(i.begin(), i.end(), min_length);
  }

  template<class Iter>
  inline void matrix_filter_short_run(Iter i, const Iter end,
			       const int max_length) {
    for (; i != end; i++)
      filter_short_run(i.begin(), i.end(), max_length);
  }

};

#endif
