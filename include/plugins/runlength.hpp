/*
 * Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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
#include "python_iterator.hpp"
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
    if ((n < 0) || (n > (long)runs->size()))
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
    // MGD: Changed so data is accessed in row-major order.  This should make things
    //      much faster.
    typedef typename T::const_col_iterator iterator;
    IntVector* hist = new IntVector(image.nrows() + 1, 0);
    IntVector tmp(image.ncols(), 0);

    for (size_t r = 0; r != image.nrows(); ++r) {
      for (size_t c = 0; c != image.ncols(); ++c) {
	if (is_black(image.get(r, c))) {
	  tmp[c]++;
	} else {
	  if (tmp[c] > 0) {
	    (*hist)[tmp[c]]++;
	    tmp[c] = 0;
	  }
	}
      }
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
    // MGD: Changed so data is accessed in row-major order.  This should make things
    //      much faster.
    typedef typename T::const_col_iterator iterator;
    IntVector* hist = new IntVector(image.nrows() + 1, 0);
    IntVector tmp(image.ncols(), 0);

    for (size_t r = 0; r != image.nrows(); ++r) {
      for (size_t c = 0; c != image.ncols(); ++c) {
	if (is_white(image.get(r, c))) {
	  tmp[c]++;
	} else {
	  if (tmp[c] > 0) {
	    (*hist)[tmp[c]]++;
	    tmp[c] = 0;
	  }
	}
      }
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

  template<class T>
  void filter_narrow_black_runs(T& image, size_t max_width) {
    image_filter_short_black_run(image.row_begin(), image.row_end(), max_width);
  }

  template<class T>
  void filter_short_black_runs(T& image, size_t max_height) {
    image_filter_short_black_run(image.col_begin(), image.col_end(), max_height);
  }

  template<class T>
  void filter_tall_black_runs(T& image, size_t min_height) {
    image_filter_long_black_run(image.col_begin(), image.col_end(), min_height);
  }

  template<class T>
  void filter_wide_black_runs(T& image, size_t min_width) {
    image_filter_long_black_run(image.row_begin(), image.row_end(), min_width);
  }

  template<class T>
  void filter_narrow_white_runs(T& image, size_t max_width) {
    image_filter_short_white_run(image.row_begin(), image.row_end(), max_width);
  }

  template<class T>
  void filter_short_white_runs(T& image, size_t max_height) {
    image_filter_short_white_run(image.col_begin(), image.col_end(), max_height);
  }

  template<class T>
  void filter_tall_white_runs(T& image, size_t min_height) {
    image_filter_long_white_run(image.col_begin(), image.col_end(), min_height);
  }

  template<class T>
  void filter_wide_white_runs(T& image, size_t min_width) {
    image_filter_long_white_run(image.row_begin(), image.row_end(), min_width);
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

  inline long next_number(char* &s) {
    // I would love to use istream::scan for this, but it's a GNU
    // extension.  Plus, this is probably faster anyway,
    // since it's more naive.

    // Scan through whitespace (literally, non-numeric)
    while (true) {
      if ((*s >= 9 && *s <= 13) || *s == 32)
	++s;
      else { 
	if (*s >= '0' && *s <= '9')
	  break;
	if (*s == '\0')
	  return -1;
	throw std::invalid_argument("Invalid character in runlength string.");
      }
    }

    long number = 0;
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
      long run;
      run = next_number(p);
      if (run < 0)
	throw std::invalid_argument("Image is too large for run-length data");
      typename T::vec_iterator end = i + (size_t)run;
      if (end > image.vec_end())
	throw std::invalid_argument("Image is too small for run-length data");
      std::fill(i, end, white(image));
      i = end;
      // black
      run = next_number(p);
      if (run < 0)
	throw std::invalid_argument("Image is too large for run-length data");
      end = i + (size_t)run;
      if (end > image.vec_end())
	throw std::invalid_argument("Image is too small for run-length data");
      std::fill(i, end, black(image));
      i = end;
    }
  }

  template<class Image, class RunIterator>
  struct RowIterator : IteratorObject {
    int init(Image& image) {
      m_it = m_begin = image.row_begin();
      m_end = image.row_end();
      return 1;
    }
    static PyObject* next(IteratorObject* self) {
      RowIterator<Image, RunIterator>* so = (RowIterator<Image, RunIterator>*)self;
      if (so->m_it == so->m_end)
	return NULL;
      RunIterator* iterator = iterator_new<RunIterator>();
      iterator->init(so->m_it.begin(), so->m_it.end(), "row", so->m_it - so->m_begin);
      so->m_it++;
      return (PyObject*)iterator;
    }
    typename Image::row_iterator m_it, m_end, m_begin;
  };

  template<class Image, class RunIterator>
  struct ColIterator : IteratorObject {
    int init(Image& image) {
      m_it = m_begin = image.col_begin();
      m_end = image.col_end();
      return 1;
    }
    static PyObject* next(IteratorObject* self) {
      ColIterator<Image, RunIterator>* so = (ColIterator<Image, RunIterator>*)self;
      if (so->m_it == so->m_end)
	return NULL;
      RunIterator* iterator = iterator_new<RunIterator>();
      iterator->init(so->m_it.begin(), so->m_it.end(), "column", so->m_it - so->m_begin);
      so->m_it++;
      return (PyObject*)iterator;
    }
    typename Image::col_iterator m_it, m_end, m_begin;
  };

  PyObject* make_run_object(const int start, const int end, const char* sequence_name, 
			    const int sequence) {
    PyObject* dict = PyDict_New();

    PyObject* start_obj = PyInt_FromLong(start);
    PyDict_SetItemString(dict, "start", start_obj);
    Py_DECREF(start_obj);

    PyObject* end_obj = PyInt_FromLong(end);
    PyDict_SetItemString(dict, "end", end_obj);
    Py_DECREF(end_obj);

    PyObject* length_obj = PyInt_FromLong(end - start);
    PyDict_SetItemString(dict, "length", length_obj);
    Py_DECREF(length_obj);

    PyObject* sequence_obj = PyInt_FromLong(sequence);
    PyDict_SetItemString(dict, sequence_name, sequence_obj);
    Py_DECREF(sequence_obj);

    return dict;
  }

  template<class Iterator>
  struct BlackRunIterator : IteratorObject {
    int init(Iterator begin, Iterator end, const char* sequence_name, int sequence) {
      m_begin = m_it = begin;
      m_end = end;
      m_sequence_name = sequence_name;
      m_sequence = sequence;
      return 1;
    }
    static PyObject* next(IteratorObject* self) {
      BlackRunIterator<Iterator>* so = (BlackRunIterator<Iterator>*)self;
      PyObject* result = 0;
      while (so->m_it != so->m_end) {
	white_run_end(so->m_it, so->m_end);
	Iterator start = so->m_it;
	black_run_end(so->m_it, so->m_end);
	if (so->m_it - start > 0) {
	  result = make_run_object(start - so->m_begin, so->m_it - so->m_begin,
				   so->m_sequence_name, so->m_sequence);
	  break;
	}
      }
      return result;
    }
    
    Iterator m_begin, m_it, m_end;
    const char* m_sequence_name;
    int m_sequence;
  };

  template<class Iterator>
  struct WhiteRunIterator : IteratorObject {
    int init(Iterator begin, Iterator end, const char* sequence_name, int sequence) {
      m_begin = m_it = begin;
      m_end = end;
      m_sequence_name = sequence_name;
      m_sequence = sequence;
      return 1;
    }
    static PyObject* next(IteratorObject* self) {
      WhiteRunIterator<Iterator>* so = (WhiteRunIterator<Iterator>*)self;
      PyObject* result = 0;
      while (so->m_it != so->m_end) {
	black_run_end(so->m_it, so->m_end);
	Iterator start = so->m_it;
	white_run_end(so->m_it, so->m_end);
	if (so->m_it - start > 0) {
	  result = make_run_object(start - so->m_begin, so->m_it - so->m_begin,
				   so->m_sequence_name, so->m_sequence);
	  break;
	}
      }
      return result;
    }
    
    Iterator m_begin, m_it, m_end;
    const char* m_sequence_name;
    int m_sequence;
  };

  template<class T>
  PyObject* iterate_black_horizontal_runs(T& image) {
    typedef RowIterator<T, BlackRunIterator<typename T::col_iterator> > Iterator;
    Iterator* iterator = iterator_new<Iterator>();
    iterator->init(image);
    return (PyObject*)iterator;
  }

  template<class T>
  PyObject* iterate_black_vertical_runs(T& image) {
    typedef ColIterator<T, BlackRunIterator<typename T::row_iterator> > Iterator;
    Iterator* iterator = iterator_new<Iterator>();
    iterator->init(image);
    return (PyObject*)iterator;
  }

  template<class T>
  PyObject* iterate_white_horizontal_runs(T& image) {
    typedef RowIterator<T, WhiteRunIterator<typename T::col_iterator> > Iterator;
    Iterator* iterator = iterator_new<Iterator>();
    iterator->init(image);
    return (PyObject*)iterator;
  }

  template<class T>
  PyObject* iterate_white_vertical_runs(T& image) {
    typedef ColIterator<T, WhiteRunIterator<typename T::row_iterator> > Iterator;
    Iterator* iterator = iterator_new<Iterator>();
    iterator->init(image);
    return (PyObject*)iterator;
  }

#endif

}

#endif
