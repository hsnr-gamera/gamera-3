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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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

#undef major
#undef minor

namespace Gamera {
  typedef std::pair<size_t, int> RunPair;
  typedef std::vector<RunPair> RunVector;

  ///////////////////////////////////////////////////////////////////////////
  // These classes manage the two dimensions on which most of the
  // functions in this modules are parameterized: color and direction.
  // These parameters are templatized types in most functions.
  namespace runs {
    class White {
    public:
      template<class T>
      inline bool is_self(const T& v) const {
	return is_white(v);
      }
      template<class T>
      inline bool is_other(const T& v) const {
	return is_black(v);
      }
      template<class T>
      inline typename T::value_type self(const T& v) const {
	return white(v);
      }
      template<class T>
      inline typename T::value_type other(const T& v) const {
	return black(v);
      }
    };

    class Black {
    public:
      template<class T>
      inline bool is_self(const T& v) const {
	return is_black(v);
      }
      template<class T>
      inline bool is_other(const T& v) const {
	return is_white(v);
      }
      template<class T>
      inline typename T::value_type self(const T& v) const {
	return black(v);
      }
      template<class T>
      inline typename T::value_type other(const T& v) const {
	return white(v);
      }
    };

    inline Black get_other_color(const White& color) {
      return Black();
    }

    inline White get_other_color(const Black& color) {
      return White();
    }

    class Horizontal {
    public:
      template<class T>
      inline typename T::const_row_iterator begin(const T& image) const {
	return image.row_begin();
      }
      template<class T>
      inline typename T::const_row_iterator end(const T& image) const {
	return image.row_end();
      }
      template<class T>
      inline typename T::row_iterator begin(T& image) const {
	return image.row_begin();
      }
      template<class T>
      inline typename T::row_iterator end(T& image) const {
	return image.row_end();
      }
      template<class T>
      size_t major(const T& image) const {
	return image.nrows();
      }
      template<class T>
      size_t minor(const T& image) const {
	return image.ncols();
      }
    };

    class Vertical {
    public:
      template<class T>
      inline typename T::const_col_iterator begin(const T& image) const {
	return image.col_begin();
      }
      template<class T>
      inline typename T::const_col_iterator end(const T& image) const {
	return image.col_end();
      }
      template<class T>
      inline typename T::col_iterator begin(T& image) const {
	return image.col_begin();
      }
      template<class T>
      inline typename T::col_iterator end(T& image) const {
	return image.col_end();
      }
      template<class T>
      size_t major(const T& image) const {
	return image.ncols();
      }
      template<class T>
      size_t minor(const T& image) const {
	return image.nrows();
      }
    };

    template<class T, class Direction>
    class GetIterator {
    public:
      typedef typename T::vec_iterator iterator;
      typedef typename T::const_vec_iterator const_iterator;
    };

    template<class T>
    class GetIterator<T, Vertical> {
    public:
      typedef typename T::col_iterator iterator;
      typedef typename T::const_col_iterator const_iterator;
    };

    template<class T>
    class GetIterator<T, Horizontal> {
    public:
      typedef typename T::row_iterator iterator;
      typedef typename T::const_row_iterator const_iterator;
    };
  }

///////////////////////////////////////////////////////////////////////////
// Finding the end of runs
  template<class T, class Color>
  inline void run_end(T& i, const T end, const Color& color) {
    for (; i != end; ++i) {
      if (color.is_other(*i))
	break;
    }
  }

///////////////////////////////////////////////////////////////////////////
//  Find the length of the largest run in a a row
//  or column of a image.
  template<class T, class Color>
  inline size_t max_run(T i, const T end, const Color& color) {
    size_t max = 0;
    while (i != end) {
      if (color.is_self(*i)) {
	T last = i;
	run_end(i, end, color);
	size_t cur_length = i - last;
	if (cur_length > max)
	  max = cur_length;
      } else {
	run_end(i, end, color);
      }
    }
    return max;
  }

///////////////////////////////////////////////////////////////////////////
// Run-length histograms. These make a histogram of the lenght of the
// runs in an image. They take an iterator range and a random-access
// container for the result (that should be appropriately sized). The
// histogram vector is not filled with zeros so that successive calls
// can be made to this algorithm with the same vector to do the
// histogram of an entire image. KWM

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
    try {
      for (size_t i = 0; i < hist->size(); ++i) {
	(*runs)[i].first = i;
	(*runs)[i].second = (*hist)[i];
      }
      SortBySecondFunctor<RunPair> func;
      std::sort(runs->begin(), runs->end(), func);
    } catch (std::exception e) {
      delete runs;
      throw;
    }
    return runs;
  }

  PyObject* _run_results_to_python(RunVector* runs, long n) {
    if ((n < 0) || (n > (long)runs->size()))
      n = (long)runs->size();
    PyObject* result = PyList_New(n);
    for (long i = 0; i < n; ++i) {
      PyObject* tuple = Py_BuildValue(CHAR_PTR_CAST "ii", (*runs)[i].first, (*runs)[i].second);
      PyList_SET_ITEM(result, i, tuple);
    }
    delete runs;
    return result;
  }

  template<class T, class Vec, class Color>
  inline void run_histogram(T i, const T end, Vec& hist, const Color& color) {
    while (i != end) {
      if (color.is_self(*i)) {
	T last = i;
	run_end(i, end, color);
	size_t cur_length = i - last;
	hist[cur_length]++;
      } else {
	run_end(i, end, get_other_color(color));
      }
    }
  }

  /* Horizontal run histograms and vertical run histograms use an entirely
     different algorithm, for efficiency reasons.  These are handled by
     the overloading on the direction parameter.
  */
  template<class T, class Color>
  IntVector* run_histogram(const T& image, const Color& color, const runs::Horizontal& direction) {
    typedef typename runs::GetIterator<T, runs::Horizontal>::const_iterator iterator;
    IntVector* hist = new IntVector(image.ncols() + 1, 0);

    try {
      iterator end = direction.end(image);
      for (iterator i = direction.begin(image); i != end; ++i)
	run_histogram(i.begin(), i.end(), *hist, color);
    } catch (std::exception e) {
      delete hist;
      throw;
    }
    return hist;
  }

  template<class Color, class T>
  IntVector* run_histogram(const T& image, const Color& color, const runs::Vertical& direction) {
    // MGD: Changed so data is accessed in row-major order.  This should make things
    //      much faster.
    typedef typename runs::GetIterator<T, runs::Vertical>::const_iterator iterator;
    IntVector* hist = new IntVector(image.nrows() + 1, 0);
    IntVector tmp(image.ncols(), 0);

    try {
      for (size_t r = 0; r != image.nrows(); ++r) {
	for (size_t c = 0; c != image.ncols(); ++c) {
	  if (color.is_self(image.get(Point(c, r)))) {
	    tmp[c]++;
	  } else {
	    if (tmp[c] > 0) {
	      (*hist)[tmp[c]]++;
	      tmp[c] = 0;
	    }
	  }
	}
      }
    } catch (std::exception e) {
      delete hist;
      throw;
    }

    return hist;
  }

  template<class T>
  IntVector* run_histogram(const T& image, char* const& color_, char* const& direction_) {
    std::string color(color_);
    std::string direction(direction_);
    if (color == "black") {
      if (direction == "horizontal") {
	return run_histogram(image, runs::Black(), runs::Horizontal());
      } else if (direction == "vertical") {
	return run_histogram(image, runs::Black(), runs::Vertical());
      }
    } else if (color == "white") {
      if (direction == "horizontal") {
	return run_histogram(image, runs::White(), runs::Horizontal());
      } else if (direction == "vertical") {
	return run_histogram(image, runs::White(), runs::Vertical());
      }
    }
    throw std::runtime_error("color must be either \"black\" or \"white\" and direction must be either \"horizontal\" or \"vertical\".");
  }

///////////////////////////////////////////////////////////////////////////
// Most frequent run(s) (basically returning a subset of a sorted histogram)

  template<class T, class Color, class Direction>
  size_t most_frequent_run(const T& image, const Color& color, const Direction& direction) {
    IntVector* hist = run_histogram(image, color, direction);
    size_t result;
    try {
      result = std::max_element(hist->begin(), hist->end()) - hist->begin();
    } catch (std::exception e) {
      delete hist;
      throw;
    }
    delete hist;
    return result;
  }

  template<class T>
  size_t most_frequent_run(const T& image, char* const& color_, char* const& direction_) {
    std::string color(color_);
    std::string direction(direction_);
    if (color == "black") {
      if (direction == "horizontal") {
	return most_frequent_run(image, runs::Black(), runs::Horizontal());
      } else if (direction == "vertical") {
	return most_frequent_run(image, runs::Black(), runs::Vertical());
      }
    } else if (color == "white") {
      if (direction == "horizontal") {
	return most_frequent_run(image, runs::White(), runs::Horizontal());
      } else if (direction == "vertical") {
	return most_frequent_run(image, runs::White(), runs::Vertical());
      }
    }
    throw std::runtime_error("color must be either \"black\" or \"white\" and direction must be either \"horizontal\" or \"vertical\".");
  }

  template<class T, class Color, class Direction>
  RunVector* most_frequent_runs(const T& image, const Color& color, const Direction& direction) {
    IntVector* hist = run_histogram(image, color, direction);
    RunVector* result = NULL;
    try {
      result = _sort_run_results(hist);
    } catch (std::exception e) {
      delete hist;
      throw;
    }
    delete hist;
    return result;
  }

  template<class T>
  RunVector* most_frequent_runs(const T& image, char* const& color_, char* const& direction_) {
    std::string color(color_);
    std::string direction(direction_);
    if (color == "black") {
      if (direction == "horizontal") {
	return most_frequent_runs(image, runs::Black(), runs::Horizontal());
      } else if (direction == "vertical") {
	return most_frequent_runs(image, runs::Black(), runs::Vertical());
      }
    } else if (color == "white") {
      if (direction == "horizontal") {
	return most_frequent_runs(image, runs::White(), runs::Horizontal());
      } else if (direction == "vertical") {
	return most_frequent_runs(image, runs::White(), runs::Vertical());
      }
    }
    throw std::runtime_error("color must be either \"black\" or \"white\" and direction must be either \"horizontal\" or \"vertical\".");
  }

  template<class T, class Color, class Direction>
  PyObject* most_frequent_runs(const T& image, long n, const Color& color, const Direction& direction) {
    RunVector* runs = most_frequent_runs(image, color, direction);
    PyObject* result;
    try {
      result = _run_results_to_python(runs, n);
    } catch (std::exception e) {
      delete runs;
      throw;
    }
    return result;
  }

  template<class T>
  PyObject* most_frequent_runs(const T& image, long n, char* const& color_, char* const& direction_) {
    std::string color(color_);
    std::string direction(direction_);
    if (color == "black") {
      if (direction == "horizontal") {
	return most_frequent_runs(image, n, runs::Black(), runs::Horizontal());
      } else if (direction == "vertical") {
	return most_frequent_runs(image, n, runs::Black(), runs::Vertical());
      }
    } else if (color == "white") {
      if (direction == "horizontal") {
	return most_frequent_runs(image, n, runs::White(), runs::Horizontal());
      } else if (direction == "vertical") {
	return most_frequent_runs(image, n, runs::White(), runs::Vertical());
      }
    }
    throw std::runtime_error("color must be either \"black\" or \"white\" and direction must be either \"horizontal\" or \"vertical\".");
  }

///////////////////////////////////////////////////////////////////////////
// Runlength filtering.

  // filter based on run-length
  template<class Iter, class Functor, class Color>
  inline void filter_run(Iter i, const Iter end, const int min_length, const Functor& functor, const Color& color) {
    while (i != end) {
      if (color.is_self(*i)) {
	Iter last = i;
	run_end(i, end, color);
	if (functor(i - last, min_length))
	  std::fill(last, i, color.other(i));
      } else {
	run_end(i, end, get_other_color(color));
      }
    }
  }

  template<class Iter, class Color>
  inline void image_filter_long_run(Iter i, const Iter end, const int min_length, const Color& color) {
    for (; i != end; i++)
      filter_run(i.begin(), i.end(), min_length, std::greater<size_t>(), color);
  }

  template<class Iter, class Color>
  inline void image_filter_short_run(Iter i, const Iter end, const int max_length, const Color& color) {
    for (; i != end; i++)
      filter_run(i.begin(), i.end(), max_length, std::less<size_t>(), color);
  }

  template<class T, class Color>
  void filter_narrow_runs(T& image, size_t max_width, const Color& color) {
    image_filter_short_run(image.row_begin(), image.row_end(), max_width, color);
  }

  template<class T>
  void filter_narrow_runs(T& image, size_t max_width, char* const& color_) {
    std::string color(color_);
    if (color == "black")
      return filter_narrow_runs(image, max_width, runs::Black());
    else if (color == "white")
      return filter_narrow_runs(image, max_width, runs::White());
    throw std::runtime_error("color must be either \"black\" or \"white\".");
  }

  template<class T, class Color>
  void filter_short_runs(T& image, size_t max_height, const Color& color) {
    image_filter_short_run(image.col_begin(), image.col_end(), max_height, color);
  }

  template<class T>
  void filter_short_runs(T& image, size_t max_width, char* const& color_) {
    std::string color(color_);
    if (color == "black")
      return filter_short_runs(image, max_width, runs::Black());
    else if (color == "white")
      return filter_short_runs(image, max_width, runs::White());
    throw std::runtime_error("color must be either \"black\" or \"white\".");
  }

  template<class T, class Color>
  void filter_tall_runs(T& image, size_t min_height, const Color& color) {
    image_filter_long_run(image.col_begin(), image.col_end(), min_height, color);
  }

  template<class T>
  void filter_tall_runs(T& image, size_t max_width, char* const& color_) {
    std::string color(color_);
    if (color == "black")
      return filter_tall_runs(image, max_width, runs::Black());
    else if (color == "white")
      return filter_tall_runs(image, max_width, runs::White());
    throw std::runtime_error("color must be either \"black\" or \"white\".");
  }

  template<class T, class Color>
  void filter_wide_runs(T& image, size_t min_width, const Color& color) {
    image_filter_long_run(image.row_begin(), image.row_end(), min_width, color);
  }

  template<class T>
  void filter_wide_runs(T& image, size_t max_width, char* const& color_) {
    std::string color(color_);
    if (color == "black")
      return filter_wide_runs(image, max_width, runs::Black());
    else if (color == "white")
      return filter_wide_runs(image, max_width, runs::White());
    throw std::runtime_error("color must be either \"black\" or \"white\".");
  }

///////////////////////////////////////////////////////////////////////////
// TO/FROM RLE
#ifndef GAMERA_NO_PYTHON

  template<class T>
  std::string to_rle(const T& image) {
    // White first
    std::ostringstream oss;

    for (typename T::const_vec_iterator i = image.vec_begin();
	 i != image.vec_end(); /* deliberately blank */) {
      typename T::const_vec_iterator start;
      start = i;
      run_end(i, image.vec_end(), runs::White());
      oss << int(i - start) << " ";
      start = i;
      run_end(i, image.vec_end(), runs::Black());
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

///////////////////////////////////////////////////////////////////////////
// Run iterators
  struct make_vertical_run {
    Rect operator() (const int start, const int end, const int column) {
      return Rect(Point(column, start), Point(column, end - 1));
    }
  };

  struct make_horizontal_run {
    Rect operator() (const int start, const int end, const int row) {
      return Rect(Point(start, row), Point(end - 1, row));
    }
  };

  template<class Image, class RunIterator>
  struct RowIterator : IteratorObject {
    typedef RowIterator<Image, RunIterator> SelfType;
    int init(Image& image) {
      m_offset_x = image.ul_x();
      m_offset_y = image.ul_y();
      m_it = m_begin = image.row_begin();
      m_end = image.row_end();
      return 1;
    }
    static PyObject* next(IteratorObject* self) {
      SelfType* so = (SelfType*)self;
      if (so->m_it == so->m_end)
	return NULL;
      RunIterator* iterator = iterator_new<RunIterator>();
      iterator->init(so->m_it.begin(), so->m_it.end(), (so->m_it - so->m_begin) + so->m_offset_y, so->m_offset_x);
      so->m_it++;
      return (PyObject*)iterator;
    }
    typename Image::row_iterator m_it, m_end, m_begin;
    size_t m_offset_x, m_offset_y;
  };

  template<class Image, typename RunIterator>
  struct ColIterator : IteratorObject {
    typedef ColIterator<Image, RunIterator> SelfType;
    int init(Image& image) {
      m_offset_x = image.ul_x();
      m_offset_y = image.ul_y();
      m_it = m_begin = image.col_begin();
      m_end = image.col_end();
      return 1;
    }
    static PyObject* next(IteratorObject* self) {
      SelfType* so = (SelfType*)self;
      if (so->m_it == so->m_end)
	return NULL;
      RunIterator* iterator = iterator_new<RunIterator>();
      iterator->init(so->m_it.begin(), so->m_it.end(), (so->m_it - so->m_begin) + so->m_offset_x, so->m_offset_y);
      so->m_it++;
      return (PyObject*)iterator;
    }
    typename Image::col_iterator m_it, m_end, m_begin;
    size_t m_offset_x, m_offset_y;
  };

  template<class Iterator, class RunMaker, class Color>
  struct RunIterator : IteratorObject {
    typedef RunIterator<Iterator, RunMaker, Color> SelfType;
    int init(Iterator begin, Iterator end, int sequence, size_t offset) {
      m_begin = m_it = begin;
      m_end = end;
      m_sequence = sequence;
      m_offset = offset;
      return 1;
    }
    static PyObject* next(IteratorObject* self) {
      SelfType* so = (SelfType*)self;
      PyObject* result = 0;
      while (so->m_it != so->m_end) {
	run_end(so->m_it, so->m_end, get_other_color(Color()));
	Iterator start = so->m_it;
	run_end(so->m_it, so->m_end, Color());
	if (so->m_it - start > 0) {
	  result = create_RectObject
	    (RunMaker()
	     ((start - so->m_begin) + so->m_offset, (so->m_it - so->m_begin) + so->m_offset, so->m_sequence));
	  break;
	}
      }
      return result;
    }

    Iterator m_begin, m_it, m_end;
    int m_sequence;
    size_t m_offset;
  };

  template<class T, class Color>
  PyObject* iterate_runs(T& image, const Color& color, const runs::Horizontal& direction) {
    typedef RowIterator<T, RunIterator<typename T::col_iterator, make_horizontal_run, Color> > Iterator;
    Iterator* iterator = iterator_new<Iterator>();
    iterator->init(image);
    return (PyObject*)iterator;
  }

  template<class T, class Color>
  PyObject* iterate_runs(T& image, const Color& color, const runs::Vertical& direction) {
    typedef ColIterator<T, RunIterator<typename T::row_iterator, make_vertical_run, Color> > Iterator;
    Iterator* iterator = iterator_new<Iterator>();
    iterator->init(image);
    return (PyObject*)iterator;
  }

  template<class T>
  PyObject* iterate_runs(T& image, char* const& color_, char* const& direction_) {
    std::string color(color_);
    std::string direction(direction_);
    if (color == "black") {
      if (direction == "horizontal") {
	return iterate_runs(image, runs::Black(), runs::Horizontal());
      } else if (direction == "vertical") {
	return iterate_runs(image, runs::Black(), runs::Vertical());
      }
    } else if (color == "white") {
      if (direction == "horizontal") {
	return iterate_runs(image, runs::White(), runs::Horizontal());
      } else if (direction == "vertical") {
	return iterate_runs(image, runs::White(), runs::Vertical());
      }
    }
    throw std::runtime_error("color must be either \"black\" or \"white\" and direction must be either \"horizontal\" or \"vertical\".");
  }

#endif // GAMERA_NOPYTHON

}

#endif
