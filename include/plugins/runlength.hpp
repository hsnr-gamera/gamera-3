/*
 * Copyright (C) 2002 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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

#ifndef kwm11052002_runlength
#define kwm11052002_runlength

#include "gamera.hpp"
#ifndef GAMERA_NO_PYTHON
  #include "Python.h"
#endif
#include <vector>
#include <algorithm>
#include <sstream>

namespace Gamera {
  /*
    Information about runs.
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
    or column of a image.
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


  template<class T>
  size_t most_frequent_black_horizontal_run(const T& image) {
    typedef typename T::const_row_iterator iterator;
    std::vector<size_t> hist(image.ncols() + 1, 0);
  
    iterator end = image.row_end();
    for (iterator i = image.row_begin(); i != end; ++i) {
      black_run_histogram(i.begin(), i.end(), hist);

    }
    return std::max_element(hist.begin(), hist.end()) - hist.begin();
  }

  template<class T>
  size_t most_frequent_black_vertical_run(const T& image) {
    typedef typename T::const_col_iterator iterator;
    std::vector<size_t> hist(image.nrows() + 1, 0);

    iterator end = image.col_end();
    for (iterator i = image.col_begin(); i != end; ++i) {
      black_run_histogram(i.begin(), i.end(), hist);
    }
    return std::max_element(hist.begin(), hist.end()) - hist.begin();
  }

  template<class T>
  size_t most_frequent_white_horizontal_run(const T& image) {
    typedef typename T::const_row_iterator iterator;
    std::vector<size_t> hist(image.ncols() + 1, 0);

    iterator end = image.row_end();
    for (iterator i = image.row_begin(); i != end; ++i) {
      white_run_histogram(i.begin(), i.end(), hist);
    }
    return std::max_element(hist.begin(), hist.end()) - hist.begin();
  }

  template<class T>
  size_t most_frequent_white_vertical_run(const T& image) {
    typedef typename T::const_col_iterator iterator;
    std::vector<size_t> hist(image.nrows() + 1, 0);

    iterator end = image.col_end();
    for (iterator i = image.col_begin(); i != end; ++i) {
      white_run_histogram(i.begin(), i.end(), hist);

    }
    return std::max_element(hist.begin(), hist.end()) - hist.begin();
  }

  /*
    Rulength filtering.
  */

  template<class T>
  void filter_narrow_runs(T& image, size_t max_width) {
    image_filter_short_run(image.row_begin(), image.row_end(), max_width);
  }

  template<class T>
  void filter_short_runs(T& image, size_t max_height) {
    image_filter_short_run(image.col_begin(), image.col_end(), max_height);
  }


  template<class T>
  void filter_tall_runs(T& image, size_t min_height) {
    image_filter_long_run(image.col_begin(), image.col_end(), min_height);
  }

  template<class T>
  void filter_wide_runs(T& image, size_t min_width) {
    image_filter_long_run(image.row_begin(), image.row_end(), min_width);
  }

  /*
    To/From rle
  */
#ifndef GAMERA_NO_PYTHON

  template<class T>
  std::string to_rle(T& image) {
    // White first
    std::ostringstream oss;

    for (typename T::vec_iterator i = image.vec_begin();
	 i != image.vec_end(); /* deliberately blank */) {
      typename T::vec_iterator start;
      start = i;
      white_run_end(i, image.vec_end());
      oss << int(i - start) << " ";
      start = i;
      black_run_end(i, image.vec_end());
      oss << int(i - start) << " ";
    }

    return oss.str();
  }

  char * next_number(char *s, size_t &number) {
    number = 0;

    // Scan through whitespace
    while (*s < '0' || *s > '9')
      ++s;

    // Read in number
    for (; *s >= '0' && *s <= '9'; ++s) {
      number *= 10;
      number += *s - '0';
    }
    return s;
  }

  template<class T>
  void from_rle(T& image, const char *runs) {
    // I would love to use istream::scan for this, but it's a GNU
    // extension.  Pretty much everywhere we compile is GNU anyway,
    // but...
    
    // White first

    char *p = const_cast<char *>(runs);

    for (typename T::vec_iterator i = image.vec_begin();
	 i != image.vec_end(); /* deliberately blank */) {
      // white
      size_t run;
      p = next_number(p, run);
      typename T::vec_iterator end = i + run;
      if (end > image.vec_end())
	throw std::invalid_argument("Image is too small for run-length data");
      std::fill(i, end, white(image));
      i = end;
      p = next_number(p, run);
      end = i + run;
      if (end > image.vec_end())
	throw std::invalid_argument("Image is too small for run-length data");
      std::fill(i, end, black(image));
      i = end;
    }
  }

#endif

}

#endif
