/*
 *
 * Copyright (C) 2001-2002 Ichiro Fujinaga, Michael Droettboom,
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

#ifndef kwm05212002_segmentation
#define kwm05212002_segmentation

#include <vector>
#include <algorithm>
#include <functional>
#include <map>
#include <stdexcept>
#include "gamera.hpp"
// this will safely include <limit> on platforms with broken libraries
#include "gamera_limits.hpp"
#include "features.hpp"

/*
  Connected-component analysis (8-connected)

  This is the standard two-pass connected-component analysis algorithm that
  will work on any matrix regardless of the storage format but only for
  OneBit or floating-point pixels.  The labeling works by setting the value
  in the matrix to the correct label (that is why OneBit matrices use
  unsigned shorts instead of some bit-packed format).  This means that the
  number of components is limited by the size of the pixel type (65536 for
  unsigned shorts).

  Authors
  -------
  Karl MacMillan <karlmac@peabody.jhu.edu>
  adapted from OMR by Ichiro Fujinaga <ich@peabody.jhu.edu>

  History
  -------
  Started 6/8/01 KWM

  TODO
  ----
  - This algorithm can be significantly faster when working on run-length data
  so an appropriate specialization should be made when run-length matrices
  are completed.
*/

namespace {
  /*
    This is a two-step connected-component labeling algorithm - the first step
    labels the pixel and the second pass reduces the number of labels to one
    per connected-component. The data necessary to form this second pass is
    stored in the equivalence and equiv_table objects.
  */
  struct equivalence {
    equivalence(size_t l = 0, size_t e = 0) { label = l; equiv = e; }
    size_t label;
    size_t equiv;
    // for sorting by label
    bool operator<(const equivalence& other) const {
      return label < other.label; }
  };
  
  struct equiv_table : public std::vector<equivalence> {
    equiv_table() : std::vector<equivalence>() { }
    // add an equivalence
    void add(size_t a, size_t b) {
      if (size() == 0 || (back().label != a || back().equiv != b)) {
	if (a < b)
	  push_back(equivalence(a, b));
	else
	  push_back(equivalence(b, a));
      }
    }
  };
}

namespace Gamera {

  template<class T>
  std::list<Image*>* cc_analysis(T& mat) {
    equiv_table eq;
    /*
      If this image has been labeled once already, it is necessary to start
      with all the pixels labeled with 1.
    */
    for (typename T::row_iterator row = mat.row_begin(); row != mat.row_end(); ++row)
      for (typename T::col_iterator col = row.begin(); col != row.end(); ++col)
	if (*col > 0)
	  *col = 1;

    // get the max value that can be held in the matrix
    typename T::value_type max_value = 
      std::numeric_limits<typename T::value_type>::max();
    // The first label we use is 2 to distinguish it from an unlabled black pixel
    typename T::value_type curr_label = 2;

    typename T::value_type W, NW, N, NE;
    for (size_t i = 0; i < mat.nrows(); i++) {
      for (size_t j = 0; j < mat.ncols(); j++) {
	if (mat.get(i, j) > 0) {
	  W = NW = N = NE = 0;
	  if (i > 0)
	    N = mat.get(i - 1, j);
	  if (j > 0)
	    W = mat.get(i, j - 1);
	  if (i > 0 && j > 0)
	    NW = mat.get(i - 1, j - 1);
	  if (i > 0 && j < (mat.ncols() - 1))
	    NE = mat.get(i - 1, j + 1);
				
	  if (W  == 0) W  = max_value;
	  if (NW == 0) NW = max_value;
	  if (N  == 0) N  = max_value;
	  if (NE == 0) NE = max_value;
	  typename T::value_type smallest_label = max_value;
				
	  if (smallest_label > W ) smallest_label = W;
	  if (smallest_label > NW) smallest_label = NW;
	  if (smallest_label > N ) smallest_label = N;
	  if (smallest_label > NE) smallest_label = NE;
				
	  if (smallest_label == max_value) { // new object found!
	    mat.set(i, j, curr_label);
	    if (curr_label == max_value) {
	      throw std::range_error("Max label exceeded - change OneBitPixel type in pixel.hpp");
	    }
	    curr_label++;
	  } else {
	    mat.set(i, j, smallest_label);
					
	    // adjust equiv_table if necessary
	    if (W  == max_value) W  = 0;
	    if (NW == max_value) NW = 0;
	    if (N  == max_value) N  = 0;
	    if (NE == max_value) NE = 0;
					
	    if (W && W != smallest_label)
	      eq.add(smallest_label, W);
	    if (NW && NW != smallest_label)
	      eq.add(smallest_label, NW);
	    if (N && N != smallest_label)
	      eq.add(smallest_label, N);
	    if (NE && NE != smallest_label)
	      eq.add(smallest_label, NE);
	  }
	}
      }
    }
  
    /*
      labels size can be curr_label because curr_label is always the next
      label - i.e. it is currently unused
    */

    std::vector<size_t> labels(curr_label);
    for (size_t i = 0; i < labels.size(); i++)
      labels[i] = i;
	
    // sort by label
    std::sort(eq.begin(), eq.end());
	
    // resolve the equivalences
    for (size_t i = 1; i < eq.size(); i++) {
      size_t x = eq[i].label;
      size_t y = eq[i].equiv;
		
      if (labels[y] > labels[x]) {
	if (labels[y] != y)
	  labels[labels[y]] = labels[x];
	labels[y] = labels[x];
      } else if (labels[y] < labels[x]) {
	if (labels[labels[y]] < labels[x])
	  labels[x] = labels[labels[y]];
	else
	  labels[x] = labels[y];
      }
		
    }
    bool swapped = true;
    while (swapped) {
      swapped = false;
      for (size_t i = 0; i < eq.size(); i++) {
	size_t x = eq[i].label;
	size_t y = eq[i].equiv;
			
	if (labels[x] != labels[y]) {
	  swapped = true;
	  if (labels[x] < labels[y])
	    labels[y] = labels[x];
	  else
	    labels[x] = labels[y];
	}
      }
    }
	
    for (size_t i = 0; i < labels.size(); i++)
      if(labels[labels[i]] < labels[i])
	labels[i] = labels[labels[i]];
	
    // Second Pass - relabel with equivalences and get bounding boxes
    typedef std::map<size_t, Rect> map_type;
    map_type rects;
	
    for (size_t i = 0; i < mat.nrows(); i++) {
      for (size_t j = 0; j < mat.ncols(); j++) {
	// relabel
	mat.set(i, j, labels[mat.get(i, j)]);
	// put bounding box in map
	typename T::value_type label = mat.get(i, j);
	if (label) {
	  if (rects.find(label) == rects.end()) {
	    rects[label].ul_x(j);
	    rects[label].ul_y(i);
	    rects[label].lr_x(j);
	    rects[label].lr_y(i);
	  } else {
	    if (j < rects[label].ul_x())
	      rects[label].ul_x(j);
	    if (j > rects[label].lr_x())
	      rects[label].lr_x(j);
	    if (i < rects[label].ul_y())
	      rects[label].ul_y(i);
	    if (i > rects[label].lr_y())
	      rects[label].lr_y(i);
	  }
	}
      }
    }
	
    // create ConnectedComponents
    std::list<Image*>* ccs = new std::list<Image*>;
    for (map_type::iterator k = rects.begin(); k != rects.end(); k++) {
      ccs->push_back(new ConnectedComponent<typename T::data_type>(*((typename T::data_type*)mat.data()),
								   OneBitPixel(k->first), k->second));
    }
    return ccs;
  }

  namespace ccs {
    /*
      Connected-component filters for use with C++ - there are equivalent
      Python versions for use within the Python Gamera environment. Unlike
      the Python versions these are in-place.
    */

    template<class T>
    void filter_wide(T& ccs, size_t max_width) {
      typename T::iterator i;
      for (i = ccs.begin(); i != ccs.end();) {
	if (i->ncols() > max_width) {
	  std::fill(i->vec_begin(), i->vec_end(), 0);
	  ccs.erase(i++);
	} else {
	  ++i;
	}
      }
    }
	
    template<class T>
    void filter_narrow(T& ccs, size_t min_width) {
      typename T::iterator i;
      for (i = ccs.begin(); i != ccs.end();) {
	if (i->ncols() < min_width) {
	  std::fill(i->vec_begin(), i->vec_end(), 0);
	  ccs.erase(i++);
	} else {
	  ++i;
	}
      }	
    }

    template<class T>
    void filter_tall(T& ccs, size_t max_height) {
      typename T::iterator i;
      for (i = ccs.begin(); i != ccs.end();) {
	if (i->nrows() > max_height) {
	  std::fill(i->vec_begin(), i->vec_end(), 0);
	  ccs.erase(i++);
	} else {
	  ++i;
	}
      }	
    }
	
    template<class T>
    void filter_short(T& ccs, size_t min_height) {
      typename T::iterator i;
      for (i = ccs.begin(); i != ccs.end();) {
	if (i->nrows() < min_height) {
	  std::fill(i->vec_begin(), i->vec_end(), 0);
	  ccs.erase(i++);
	} else {
	  ++i;
	}
      }	
    }
	
    template<class T>
    void filter_large(T& ccs, size_t max_size) {
      typename T::iterator i;
      for (i = ccs.begin(); i != ccs.end();) {
	if (i->nrows() > max_size && i->ncols() > max_size) {
	  std::fill(i->vec_begin(), i->vec_end(), 0);
	  ccs.erase(i++);
	} else {
	  ++i;
	}
      }	
    }

    template<class T>
    void filter_small(T& ccs, size_t min_size) {
      typename T::iterator i;
      for (i = ccs.begin(); i != ccs.end();) {
	if (i->nrows() < min_size && i->ncols() < min_size) {
	  std::fill(i->vec_begin(), i->vec_end(), 0);
	  ccs.erase(i++);
	} else {
	  ++i;
	}
      }	
    }

    template<class T>
    void filter_black_area_large(T& ccs, size_t max_area) {
      typename T::iterator i;
      for (i = ccs.begin(); i != ccs.end();) {
	if (black_area(*i) > max_area) {
	  std::fill(i->vec_begin(), i->vec_end(), 0);
	  ccs.erase(i++);
	} else {
	  ++i;
	}
      }	
    }

    template<class T>
    void filter_black_area_small(T& ccs, size_t min_area) {
      typename T::iterator i;
      for (i = ccs.begin(); i != ccs.end();) {
	if (black_area(*i) < min_area) {
	  std::fill(i->vec_begin(), i->vec_end(), 0);
	  ccs.erase(i++);
	} else {
	  ++i;
	}
      }	
    }
  }

}

#endif
