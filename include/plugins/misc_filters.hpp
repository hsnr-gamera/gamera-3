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

#ifndef kwm11062002_misc_filters
#define kwm11062002_misc_filters

#include "gamera.hpp"
#include "logical.hpp"
#include "morphology.hpp"
#include "image_utilities.hpp"

namespace Gamera {

  template<class T>
  typename ImageFactory<T>::view_type* outline(const T& in) {
    typedef typename ImageFactory<T>::data_type data_type;
    typedef typename ImageFactory<T>::view_type view_type;
    data_type* data = new data_type(in.size(), in.offset_y(), in.offset_x());
    view_type* out = new view_type(*data, in);
    image_copy_fill(in, *out);
    dilate(*out);
    xor_image(*out, in);
    return out;
  }

  template<class T>
  typename ImageFactory<T>::view_type* expand_edges(const T& in, size_t size, bool zeros=0, bool even=0) {
    typedef typename ImageFactory<T>::data_type data_type;
    typedef typename ImageFactory<T>::view_type view_type;
    data_type* out_data = new data_type(
        in.nrows() + size * 2, in.ncols() + size * 2, in.offset_y() - 1, in.offset_x() - 1);
    view_type* out_view = new view_type(*out_data);
    std::cerr << out_view->offset_x() << " " << out_view->ncols();

    for (size_t y = 0; y < in.nrows(); ++y)
      for (size_t x = 0; x < in.ncols(); ++x) 
	out_view->set(y + size, x + size, in.get(y, x));

    if (zeros) {
      for (size_t y = 0; y < size; ++y)
	for (size_t x = 0; x < out_view->ncols(); ++x)
	  out_view->set(y, x, white(*out_view));
      for (size_t y = out_view->nrows() - size; y < out_view->nrows(); ++y)
	for (size_t x = 0; x < out_view->ncols(); ++x)
	  out_view->set(y, x, white(*out_view));
      for (size_t y = size; y < out_view->nrows(); ++y) {
	for (size_t x = 0; x < size; ++x)
	  out_view->set(y, x, white(*out_view));
	for (size_t x = out_view->ncols() - size; x < out_view->ncols(); ++x)
	  out_view->set(y, x, white(*out_view));
      }
    } else {
      size_t low_offset = size * 2;
      size_t high_y_offset = out_view->nrows() - size * 2;
      size_t high_x_offset = out_view->ncols() - size * 2;
      if (!even) {
	low_offset += 1;
	high_y_offset -= 1;
	high_x_offset -= 1;
      }
      for (size_t y = 0; y < size; ++y)
	for (size_t x = size; x < out_view->ncols() - size; ++x)
	  out_view->set(y, x, out_view->get(low_offset - y, x));
      for (size_t y = 0; y < size; ++y)
	for (size_t x = size; x < out_view->ncols() - size; ++x)
	  out_view->set(out_view->nrows() - y, x, out_view->get(high_y_offset + y, x));
      for (size_t y = 0; y < out_view->nrows(); ++y) {
	for (size_t x = 0; x < size; ++x)
	  out_view->set(y, x, out_view->get(y, low_offset - x));
	for (size_t x = 0; x < size; ++x)
	  out_view->set(y, out_view->ncols() - x, out_view->get(y, high_x_offset + x));
      }
    }
    return out_view;
  }

  template<class T>
  void thin_zs_flag_bp1(T& thin, T& flag) {
    size_t x, y, xsize, ysize, N, S;
    bool p[10];

    for (size_t y = 1; x < thin.nrows(); ++x)
      for (size_t x = 1; y < thin.ncols(); ++y)
	thin_zs_get_ngb(x, y, thin, p);
  }

  template<class T>
  typename ImageFactory<T>::view_type* thin_zs(const T& in) {
    typedef typename ImageFactory<T>::data_type data_type;
    typedef typename ImageFactory<T>::view_type view_type;
    view_type* thin_view = expand_edges(in, 1, 0, 0);
    data_type* flag_data = new data_type(in.size(), in.offset_y(), in.offset_x());
    view_type* flag_view = new view_type(*data, in);
    return thin_view;
  }


}

#endif
