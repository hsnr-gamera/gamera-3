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
  typedef std::pair<size_t, int> RunPair;
  typedef std::vector<RunPair> RunVector;

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
  
  template<class T>
  struct SortBySecondFunctor {
    bool operator()(const T& a, const T& b) {
      if (a.second == b.second)
	return a.first < b.first;
      return a.second >= b.second;
    }
  };

  RunVector* _sort_run_results(IntVector* hist) {
    RunVector* runs = new RunVector(hist->size());
    for (size_t i = 0; i < hist->size(); ++i) {
      (*runs)[i].first = i;
      (*runs)[i].second = (*hist)[i];
    }
    delete hist;
    SortBySecondFunctor<RunPair> func;
    std::sort(runs->begin(), runs->end(), func);
    return runs;
  }

  PyObject* _run_results_to_python(RunVector* runs, long n) {
    if (n < 0 or n > (long)runs->size())
      n = (long)runs->size();
    PyObject* result = PyList_New(n);
    for (long i = 0; i < n; ++i) {
      PyObject* tuple = Py_BuildValue("ii", (*runs)[i].first, (*runs)[i].second);
      PyList_SET_ITEM(result, i, tuple);
    }
    delete runs;
    return result;
  }

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
  IntVector* black_horizontal_run_histogram(const T& image) {
    typedef typename T::const_row_iterator iterator;
    IntVector* hist = new IntVector(image.ncols() + 1, 0);
  
    iterator end = image.row_end();
    for (iterator i = image.row_begin(); i != end; ++i)
      black_run_histogram(i.begin(), i.end(), *hist);

    return hist;
  }    

  template<class T>
  size_t most_frequent_black_horizontal_run(const T& image) {
    IntVector* hist = black_horizontal_run_histogram(image);
    size_t result = std::max_element(hist->begin(), hist->end()) - hist->begin();
    delete hist;
    return result;
  }

  template<class T>
  RunVector* most_frequent_black_horizontal_runs(const T& image) {
    IntVector* hist = black_horizontal_run_histogram(image);
    return _sort_run_results(hist);
  }

  template<class T>
  PyObject* most_frequent_black_horizontal_runs(const T& image, long n) {
    RunVector* runs = most_frequent_black_horizontal_runs(image);
    return _run_results_to_python(runs, n);
  }

  template<class T>
  IntVector* black_vertical_run_histogram(const T& image) {
    typedef typename T::const_col_iterator iterator;
    IntVector* hist = new IntVector(image.nrows() + 1, 0);

    iterator end = image.col_end();
    for (iterator i = image.col_begin(); i != end; ++i) {
      black_run_histogram(i.begin(), i.end(), *hist);
    }
    return hist;
  }

  template<class T>
  size_t most_frequent_black_vertical_run(const T& image) {
    IntVector* hist = black_vertical_run_histogram(image);
    size_t result = std::max_element(hist->begin(), hist->end()) - hist->begin();
    delete hist;
    return result;
  }

  template<class T>
  RunVector* most_frequent_black_vertical_runs(const T& image) {
    IntVector* hist = black_vertical_run_histogram(image);
    return _sort_run_results(hist);
  }

  template<class T>
  PyObject* most_frequent_black_vertical_runs(const T& image, long n) {
    RunVector* runs = most_frequent_black_vertical_runs(image);
    return _run_results_to_python(runs, n);
  }

  template<class T>
  IntVector* white_horizontal_run_histogram(const T& image) {
    typedef typename T::const_row_iterator iterator;
    IntVector* hist = new IntVector(image.ncols() + 1, 0);

    iterator end = image.row_end();
    for (iterator i = image.row_begin(); i != end; ++i) {
      white_run_histogram(i.begin(), i.end(), *hist);
    }
    return hist;
  }    

  template<class T>
  size_t most_frequent_white_horizontal_run(const T& image) {
    IntVector* hist = white_horizontal_run_histogram(image);
    size_t result = std::max_element(hist->begin(), hist->end()) - hist->begin();
    delete hist;
    return result;
  }

  template<class T>
  RunVector* most_frequent_white_horizontal_runs(const T& image) {
    IntVector* hist = white_horizontal_run_histogram(image);
    return _sort_run_results(hist);
  }

  template<class T>
  PyObject* most_frequent_white_horizontal_runs(const T& image, long n) {
    RunVector* runs = most_frequent_white_horizontal_runs(image);
    return _run_results_to_python(runs, n);
  }

  template<class T>
  IntVector* white_vertical_run_histogram(const T& image) {
    typedef typename T::const_col_iterator iterator;
    IntVector* hist = new IntVector(image.nrows() + 1, 0);

    iterator end = image.col_end();
    for (iterator i = image.col_begin(); i != end; ++i) {
      white_run_histogram(i.begin(), i.end(), *hist);

    }
    return hist;
  }

  template<class T>
  size_t most_frequent_white_vertical_run(const T& image) {
    IntVector* hist = white_vertical_run_histogram(image);
    size_t result = std::max_element(hist->begin(), hist->end()) - hist->begin();
    delete hist;
    return result;
  }

  template<class T>
  RunVector* most_frequent_white_vertical_runs(const T& image) {
    IntVector* hist = white_vertical_run_histogram(image);
    return _sort_run_results(hist);
  }

  template<class T>
  PyObject* most_frequent_white_vertical_runs(const T& image, long n) {
    RunVector* runs = most_frequent_white_vertical_runs(image);
    return _run_results_to_python(runs, n);
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

  inline size_t next_number(char* &s) {
    // I would love to use istream::scan for this, but it's a GNU
    // extension.  Plus, this is probably faster anyway,
    // since it's more naive.

    // Scan through whitespace (literally, non-numeric)
    while (*s < '0' || *s > '9') {
      if (*s == '\0')
	throw std::invalid_argument("Image is too large for run-length data");
      ++s;
    }

    size_t number = 0;
    // Read in number
    for (; *s >= '0' && *s <= '9'; ++s) {
      number *= 10;
      number += *s - '0';
    }
    return number;
  }

  template<class T>
  void from_rle(T& image, const char *runs) {
    // White first

    char *p = const_cast<char *>(runs);
    // Outside the loop since we need to do a check at the end
    for (typename T::vec_iterator i = image.vec_begin();
	 i != image.vec_end(); /* deliberately blank */) {
      // white
      size_t run;
      run = next_number(p);
      typename T::vec_iterator end = i + run;
      if (end > image.vec_end())
	throw std::invalid_argument("Image is too small for run-length data");
      std::fill(i, end, white(image));
      i = end;
      // black
      run = next_number(p);
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
