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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */


#ifndef kwm05212002_segmentation
#define kwm05212002_segmentation

#include <vector>
#include <algorithm>
#include <functional>
#include <map>
#include <stdexcept>
#include "gamera.hpp"
#include "gamera_limits.hpp"
#include "features.hpp"
#include "image_utilities.hpp"
#include "projections.hpp"

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
    bool operator==(const equivalence& other) const {
      return label == other.label &&  equiv == other.equiv; }
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
  ImageList* cc_analysis(T& image) {
    equiv_table eq;
    // get the max value that can be held in the matrix
    typename T::value_type max_value = 
      std::numeric_limits<typename T::value_type>::max();
    // The first label we use is 2 to distinguish it from an unlabled black pixel
    typename T::value_type curr_label = 2;

    typename T::value_type W, NW, N, NE;
    ImageAccessor<typename T::value_type> acc;
    typename T::Iterator row, col, lr, ul, above;
    lr = image.lowerRight();
    ul = image.upperLeft();
    // progress_bar.set_length(image.nrows() / 40);
    size_t i0 = 0;
    for (row = image.upperLeft(); row.y != lr.y; ++row.y, ++i0) {
      for (col = row; col.x != lr.x; ++col.x) {
        /*
          If this image has been labeled once already, it is necessary to start
          with all the pixels labeled with 1.
        */
        if (acc(col) > 0)
          acc.set(1, col);
        if (acc(col) > 0) {
          W = NW = N = NE = 0;
          if (col.y != ul.y) {
            above = col;
            --above.y;
            N = acc(above);
            if (col.x != ul.x) {
              --above.x;
              NW = acc(above);
              ++above.x;
            }
            ++above.x;
            if (above.x != lr.x)
              NE = acc(above);
          }
          if (col.x != ul.x)
            W = acc(col - Diff2D(1, 0));
        
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
            acc.set(curr_label, col);
            if (curr_label == max_value) {
              throw std::range_error("Max label exceeded - change OneBitPixel type in pixel.hpp");
            }
            curr_label++;
          } else {
            acc.set(smallest_label, col);
          
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
      // if ((i0 % 20) == 0)
      // progress_bar.step();
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
  
    /*
      Second Pass - relabel with equivalences and get bounding boxes
      This used to use a map, but I think that it is worth the memory
      use to use a vector for mapping the labels to the rects. The 
      vector is a lot faster.
    */
    ImageList* ccs = NULL;

    typedef std::vector<Rect*> map_type;
    map_type rects(labels.size(), 0);
    try {
      row = image.upperLeft();
      for (size_t i = 0; i < image.nrows(); i++, ++row.y) {
        size_t j;
        for (j = 0, col = row; j < image.ncols(); j++, ++col.x) {
          // relabel
          acc.set(labels[acc(col)], col); 
          // put bounding box in map
          typename T::value_type label = acc(col);
          if (label) {
            if (rects[label] == 0) {
              rects[label] = new Rect(Point(j, i), Dim(1, 1));
            } else {
              if (j < rects[label]->ul_x())
                rects[label]->ul_x(j);
              if (j > rects[label]->lr_x())
                rects[label]->lr_x(j);
              if (i < rects[label]->ul_y())
                rects[label]->ul_y(i);
              if (i > rects[label]->lr_y())
                rects[label]->lr_y(i);
            }
          }
        }
        // if ((i % 20) == 0)
        // progress_bar.step();
      }

      // create ConnectedComponents
      ccs = new ImageList();
      try {
        for (size_t i = 0; i < rects.size(); ++i) {
          if (rects[i] != 0) {
            ccs->push_back(new ConnectedComponent<typename T::data_type>(*((typename T::data_type*)image.data()),
                                                                         OneBitPixel(i),
                                                                         Point(rects[i]->offset_x() + image.offset_x(),
                                                                               rects[i]->offset_y() + image.offset_y()),
                                                                         rects[i]->dim()));
            delete rects[i];
          }
        }
      } catch (std::exception e) {
        for (ImageList::iterator i = ccs->begin(); i != ccs->end(); ++i)
          delete *i;
        delete ccs;
        throw;
      }
    } catch (std::exception e) {
      for (size_t i = 0; i != rects.size(); ++i)
        delete rects[i];
    }
    return ccs;
  }

  template<class T>
  inline void delete_connected_components(T* ccs) {
    for (typename T::iterator i = ccs->begin(); i != ccs->end(); ++i)
      delete *i;
    delete ccs;
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
        if ((*i)->ncols() > max_width) {
          std::fill((*i)->vec_begin(), (*i)->vec_end(), 0);
          delete *i;
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
        if ((*i)->ncols() < min_width) {
          std::fill((*i)->vec_begin(), (*i)->vec_end(), 0);
          delete *i;
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
        if ((*i)->nrows() > max_height) {
          std::fill((*i)->vec_begin(), (*i)->vec_end(), 0);
          delete *i;
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
        if ((*i)->nrows() < min_height) {
          std::fill((*i)->vec_begin(), (*i)->vec_end(), 0);
          delete *i;
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
        if ((*i)->nrows() > max_size && (*i)->ncols() > max_size) {
          std::fill((*i)->vec_begin(), (*i)->vec_end(), 0);
          delete *i;
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
        if ((*i)->nrows() < min_size && (*i)->ncols() < min_size) {
          std::fill((*i)->vec_begin(), (*i)->vec_end(), 0);
          delete *i;
          ccs.erase(i++);
        } else {
          ++i;
        }
      } 
    }

    template<class T>
    void filter_black_area_large(T& ccs, int max_area) {
      typename T::iterator i;
      for (i = ccs.begin(); i != ccs.end();) {
        int bai = (int)black_area(**i);
        if (bai > max_area) {
          std::fill((*i)->vec_begin(), (*i)->vec_end(), 0);
          delete *i;
          ccs.erase(i++);
        } else {
          ++i;
        }
      } 
    }

    template<class T>
    void filter_black_area_small(T& ccs, int min_area) {
      typename T::iterator i;
      for (i = ccs.begin(); i != ccs.end();) {
        int bai = (int)black_area(**i);
        if (bai < min_area) {
          std::fill((*i)->vec_begin(), (*i)->vec_end(), 0);
          delete *i;
          ccs.erase(i++);
        } else {
          ++i;
        }
      } 
    }
  }

  size_t find_split_point(IntVector *projections, double& center) {
    double minimum = std::numeric_limits<size_t>::max();
    double middle = double(projections->size()) * center;
    size_t minimum_index = 0;
    size_t start = size_t(middle / 2);
    size_t end = size_t(((projections->size() - middle) / 2) + middle);
    for (size_t i=start; i != end; ++i) {
      double distance_from_middle = abs(middle - i);
      int value = (*projections)[i];
      double score = value*value*2 + distance_from_middle*distance_from_middle;
      if (score < minimum) {
        minimum = score;
        minimum_index = i;
      }
    }
    if (minimum_index == 0)
      minimum_index = 1;
    else if (minimum_index == projections->size() - 1)
      minimum_index = projections->size() - 2; 
    return minimum_index;
  }

  size_t find_split_point_max(IntVector *projections, double& center) {
    double minimum = std::numeric_limits<size_t>::max();
    double middle = double(projections->size()) * center;
    size_t minimum_index = 0;
    size_t start = size_t(middle / 2);
    size_t end = size_t(((projections->size() - middle) / 2) + middle);
    for (size_t i=start; i != end; ++i) {
      double distance_from_middle = abs(middle - i);
      int value = (*projections)[i];
      double score = -(value*value*2) + distance_from_middle*distance_from_middle*distance_from_middle;
      if (score < minimum) {
        minimum = score;
        minimum_index = i;
      }
    }
    if (minimum_index == 0)
      minimum_index = 1;
    else if (minimum_index == projections->size() - 1)
      minimum_index = projections->size() - 2; 
    return minimum_index;
  }

  template<class T>
  void split_error_cleanup(T* view,
                           ImageList* splits,
                           IntVector *projs,
                           ImageList* ccs) {
    delete view->data();
    delete view;
    for (ImageList::iterator i = splits->begin(); i != splits->end(); ++i) 
      delete (*i);
    delete splits;
    if (projs != NULL)
      delete projs;
    if (ccs != NULL) {
      for (ImageList::iterator i = ccs->begin(); i != ccs->end(); ++i) 
        delete (*i);
      delete ccs;
    }
  }

  template<class T>
  ImageList* splitx(T& image, FloatVector* center) {
    ImageList* splits = new ImageList();
    typename ImageFactory<T>::view_type* view = 0;
    ImageList* ccs = NULL;
    ImageList::iterator ccs_it;
    size_t last_split, new_split;

    if (image.ncols() <= 1) {
      view = simple_image_copy(T(image, image.origin(), image.dim()));
      splits->push_back(view);
      return splits;
    }
    sort(center->begin(), center->end());
    IntVector *projs = projection_cols(image);
    last_split = 0;
    for (size_t i = 0; i<center->size(); i++) {
      new_split = find_split_point(projs, (*center)[i]);
      if (new_split <= last_split)
        continue;
      view = simple_image_copy(T(image, 
                                 Point(image.ul_x() + last_split, image.ul_y()), 
                                 Dim(new_split - last_split, image.nrows())));
      last_split = new_split;
      try {
        ccs = cc_analysis(*view);
      } catch (std::range_error x) {
        split_error_cleanup(view, splits, projs, ccs);
        throw x;
      }
      for (ccs_it = ccs->begin(); ccs_it != ccs->end(); ++ccs_it)
        splits->push_back(*ccs_it);
      delete view;
      delete ccs;
    }
    delete projs;
    view = simple_image_copy(T(image, 
                               Point(image.ul_x() + last_split, image.ul_y()),
                               Dim(image.ncols() - last_split, image.nrows())));
    try {
      ccs = cc_analysis(*view);
    } catch (std::range_error x) {
      split_error_cleanup(view, splits, NULL, ccs);
      throw x;
    }
    for (ccs_it = ccs->begin(); ccs_it != ccs->end(); ++ccs_it)
      splits->push_back(*ccs_it);
    delete view;
    delete ccs;
    return splits;
  }

  template<class T>
  ImageList* splitx_max(T& image, FloatVector* center) {
    ImageList* splits = new ImageList();
    typename ImageFactory<T>::view_type* view = 0;
    ImageList* ccs = NULL;
    ImageList::iterator ccs_it;
    size_t last_split, new_split;

    if (image.ncols() <= 1) {
      view = simple_image_copy(T(image, image.origin(), image.dim()));
      splits->push_back(view);
      return splits;
    }
    sort(center->begin(), center->end());
    IntVector *projs = projection_cols(image);
    last_split = 0;
    for (size_t i = 0; i<center->size(); i++) {
      new_split = find_split_point_max(projs, (*center)[i]);
      if (new_split <= last_split)
        continue;
      view = simple_image_copy(T(image, 
                                 Point(image.ul_x()+last_split, image.ul_y()),
                                 Dim(new_split - last_split, image.nrows())));
      last_split = new_split;
      try {
        ccs = cc_analysis(*view);
      } catch (std::range_error x) {
        split_error_cleanup(view, splits, projs, ccs);
        throw x;
      }
      for (ccs_it = ccs->begin(); ccs_it != ccs->end(); ++ccs_it)
        splits->push_back(*ccs_it);
      delete view;
      delete ccs;
    }
    delete projs;
    view = simple_image_copy(T(image, 
                               Point(image.ul_x() + last_split, image.ul_y()),
                               Dim(image.ncols() - last_split, image.nrows())));
    try { 
      ccs = cc_analysis(*view);
    } catch (std::range_error x) {
      split_error_cleanup(view, splits, NULL, ccs);
      throw x;
    }
    for (ccs_it = ccs->begin(); ccs_it != ccs->end(); ++ccs_it)
      splits->push_back(*ccs_it);
    delete view;
    delete ccs;
    return splits;
  }

  template<class T>
  ImageList* splity(T& image, FloatVector* center) {
    ImageList* splits = new ImageList();
    typename ImageFactory<T>::view_type* view = 0;
    ImageList* ccs = NULL;
    ImageList::iterator ccs_it;
    size_t last_split, new_split;

    if (image.nrows() <= 1) {
      view = simple_image_copy(T(image, image.origin(), image.dim()));
      splits->push_back(view);
      return splits;
    }
    sort(center->begin(), center->end());
    IntVector *projs = projection_rows(image);
    last_split = 0;
    for (size_t i = 0; i<center->size(); i++) {
      new_split = find_split_point(projs, (*center)[i]);
      if (new_split <= last_split)
        continue;
      view = simple_image_copy(T(image, 
                                 Point(image.ul_x(), image.ul_y()+last_split), 
                                 Dim(image.ncols(), new_split - last_split)));
      last_split = new_split;
      try {
        ccs = cc_analysis(*view);
      } catch (std::range_error x) {
        split_error_cleanup(view, splits, projs, ccs);
        throw x;
      }
      for (ccs_it = ccs->begin(); ccs_it != ccs->end(); ++ccs_it)
        splits->push_back(*ccs_it);
      delete view;
      delete ccs;
    }
    delete projs;
    view = simple_image_copy(T(image, 
                               Point(image.ul_x(), image.ul_y() + last_split),
                               Dim(image.ncols(), image.nrows() - last_split)));
    try {
      ccs = cc_analysis(*view);
    } catch (std::range_error x) {
      split_error_cleanup(view, splits, NULL, ccs);
      throw x;
    }
    for (ccs_it = ccs->begin(); ccs_it != ccs->end(); ++ccs_it)
      splits->push_back(*ccs_it);
    delete view;
    delete ccs;
    return splits;
  }
}

#endif
