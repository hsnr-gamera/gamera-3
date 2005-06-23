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
#include "image_utilities.hpp"

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

  template<>
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

  template<>
  inline OneBitPixel Min<OneBitPixel>::operator()
    (vector<OneBitPixel>::iterator begin,
     vector<OneBitPixel>::iterator end) {
    return *(max_element(begin, end));
  }

  template<class T>
  typename ImageFactory<T>::view_type* erode_dilate(T &m, const size_t times, int direction, int geo) {
    typedef typename ImageFactory<T>::data_type data_type;
    typedef typename ImageFactory<T>::view_type view_type;
    typedef typename T::value_type value_type;
    Max<value_type> max;
    Min<value_type> min;

    if (m.nrows() < 3 || m.ncols() < 3)
      return simple_image_copy(m);

    data_type* new_data = new data_type(m.size(), m.offset_y(), m.offset_x());
    view_type* new_view = new view_type(*new_data);

    if (times > 1) {
      view_type* flip_view = simple_image_copy(m);

      unsigned int r, ngeo = 0;
      bool n8;
      ngeo = 1;
      for (r = 1; r <= times; r++) {
	if (r > 1) {
	  typename view_type::vec_iterator g = flip_view->vec_begin();
	  typename view_type::vec_iterator h = new_view->vec_begin();
	  for (; g != flip_view->vec_end(); g++, h++)
	    *g = *h;
	}
	if (geo && (ngeo % 2 == 0))
	  n8 = true;
	else
	  n8 = false;
	if (direction) {
	  if (n8)
	    neighbor4x(*flip_view, max, *new_view);
	  else
	    neighbor9(*flip_view, max, *new_view);
	}
	else {
	  if (n8)
	    neighbor4x(*flip_view, min, *new_view);
	  else
	    neighbor9(*flip_view, min, *new_view);
	}
	ngeo++;
      }
      delete flip_view->data();
      delete flip_view;
      return new_view;
    } else {
      if (direction) {
	if (geo)
	  neighbor4x(m, max, *new_view);
	else
	  neighbor9(m, max, *new_view);
      }
      else {
	if (geo)
	  neighbor4x(m, min, *new_view);
	else
	  neighbor9(m, min, *new_view);
      }
      return new_view;
    }
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

    data_type* new_data = new data_type(m.size(), m.offset_y(), m.offset_x());
    view_type* new_view = new view_type(*new_data);

    Rank<typename T::value_type> rank(r);
    neighbor9(m, rank, *new_view);
    return new_view;
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
  class Mode {
  public:
    inline T operator() (typename vector<T>::iterator begin,
			 typename vector<T>::iterator end);
  };

  template<class T>
  inline T Mode<T>::operator() (typename vector<T>::iterator begin,
				typename vector<T>::iterator end) {
    std::map<T, size_t> votes;
    T max_value;
    size_t max_count = 0;
    for (; begin != end; ++begin) {
      votes[*begin]++;
      if (votes[*begin] > max_count) {
	max_count = votes[*begin];
	max_value = *begin;
      }
    }
    return max_value;
  }

  template<class T>
  typename ImageFactory<T>::view_type* mean(T &m) {
    typedef typename ImageFactory<T>::data_type data_type;
    typedef typename ImageFactory<T>::view_type view_type;
    if (m.nrows() < 3 || m.ncols() < 3)
      return simple_image_copy(m);

    data_type* new_data = new data_type(m.size(), m.offset_y(), m.offset_x());
    view_type* new_view = new view_type(*new_data);

    Mean<typename T::value_type> mean_op;
    neighbor9(m, mean_op, *new_view);
    return new_view;
  }

  template<class T>
  class All {
  public:
    inline T operator() (typename vector<T>::iterator begin,
			 typename vector<T>::iterator end);
  };

  template<class T>
  inline T All<T>::operator() (typename vector<T>::iterator begin,
			       typename vector<T>::iterator end) {
    typename vector<T>::iterator middle = begin + 4;
    for (; begin != end; ++begin)
      if (begin != middle)
	if (is_black(*begin))
	  return *middle;
    return pixel_traits<T>::white();
  }

  template<class T>
  void despeckle_single_pixel(T &m) {
    typedef typename ImageFactory<T>::data_type data_type;
    typedef typename ImageFactory<T>::view_type view_type;
    data_type* new_data = new data_type(m.size(), m.offset_y(), m.offset_x());
    view_type* new_view = new view_type(*new_data);

    All<typename T::value_type> all_op;
    neighbor9(m, all_op, *new_view);

    typename T::vec_iterator g = m.vec_begin();
    typename view_type::vec_iterator h = new_view->vec_begin();
    for (; g != m.vec_end(); g++, h++)
      *g = *h;
    return;
  }

  template<class T>
  void despeckle(T &m, size_t size) {
    if (m.nrows() < 3 || m.ncols() < 3)
      return;
    if (size == 1) {
      despeckle_single_pixel(m);
      return;
    }
    typedef typename T::value_type value_type;
    ImageData<value_type> mat_data(m.nrows(), m.ncols());
    ImageView<ImageData<value_type> > tmp(mat_data, 0, 0, m.nrows(), m.ncols());

    typedef std::vector<Point> PixelQueue;
    PixelQueue pixel_queue;
    pixel_queue.reserve(size * 2);
    for (size_t r = 0; r < m.nrows(); ++r) {
      for (size_t c = 0; c < m.ncols(); ++c) {
	if (is_white(tmp.get(r, c)) && is_black(m.get(r, c))) {
	  pixel_queue.clear();
	  pixel_queue.push_back(Point(c, r));
	  bool bail = false;
	  tmp.set(r, c, 1);
	  for (size_t i = 0;
	       (i < pixel_queue.size()) && (pixel_queue.size() < size);
	       ++i) {
	    Point center = pixel_queue[i];
	    for (size_t r2 = (center.y()>0) ? center.y() - 1 : 0; 
		 r2 < std::min(center.y() + 2, m.nrows()); ++r2) {
	      for (size_t c2 = (center.x()>0) ? center.x() - 1 : 0; 
		   c2 < std::min(center.x() + 2, m.ncols()); ++c2) {
		if (is_black(m.get(r2, c2)) && is_white(tmp.get(r2, c2))) {
		  tmp.set(r2, c2, 1);
		  pixel_queue.push_back(Point(c2, r2));
		} else if (tmp.get(r2, c2) == 2) {
		  bail = true;
		  break;
		}
	      }
	      if (bail)
		break;
	    }
	    if (bail)
	      break;
	  }
	  if (!bail && pixel_queue.size() < size) {
	    for (typename PixelQueue::iterator i = pixel_queue.begin();
		 i != pixel_queue.end(); ++i) {
	      m.set(i->y(), i->x(), white(m));
	    }
	  } else {
	    for (typename PixelQueue::iterator i = pixel_queue.begin();
		 i != pixel_queue.end(); ++i) {
	      tmp.set(i->y(), i->x(), 2);
	    }
	  }
	}
      }
    }
  }
}
#endif
