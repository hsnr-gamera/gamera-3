/*
 *
 * Copyright (C) 2001 - 2002
 * Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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

#ifndef kwm12032001_utility
#define kwm12032001_utility

#include "gamera.hpp"
using namespace Gamera;

template<class T>
Image* image_copy(T &a, int storage_format) {
  if (storage_format == DENSE) {
    typename ImageFactory<T>::dense_data_type* data =
      new typename ImageFactory<T>::dense_data_type(a.nrows(), a.ncols(), a.offset_y(), a.offset_x());
    typename ImageFactory<T>::dense_view_type* view =
      new typename ImageFactory<T>::dense_view_type(*data, a.offset_y(), a.offset_x(), a.nrows(), a.ncols());
    typename T::col_iterator a_col;
    typename T::row_iterator a_row;
    typename ImageFactory<T>::dense_view_type::col_iterator b_col;
    typename ImageFactory<T>::dense_view_type::row_iterator b_row;
    ImageAccessor<typename T::value_type> acc;
    for (a_row = a.row_begin(), b_row = view->row_begin(); a_row != a.row_end(); ++a_row, ++b_row)
      for (a_col = a_row.begin(), b_col = b_row.begin(); a_col != a_row.end(); ++a_col, ++b_col)
	acc.set(acc.get(a_col), b_col);
    return view;
  } else {
    typename ImageFactory<T>::rle_data_type* data =
      new typename ImageFactory<T>::rle_data_type(a.nrows(), a.ncols(), a.offset_y(), a.offset_x());
    typename ImageFactory<T>::rle_view_type* view =
      new typename ImageFactory<T>::rle_view_type(*data, a.offset_y(), a.offset_x(), a.nrows(), a.ncols());
    typename T::col_iterator a_col;
    typename T::row_iterator a_row;
    typename ImageFactory<T>::rle_view_type::col_iterator b_col;
    typename ImageFactory<T>::rle_view_type::row_iterator b_row;
    ImageAccessor<typename T::value_type> acc;
    for (a_row = a.row_begin(), b_row = view->row_begin(); a_row != a.row_end(); ++a_row, ++b_row)
      for (a_col = a_row.begin(), b_col = b_row.begin(); a_col != a_row.end(); ++a_col, ++b_col)
	acc.set(acc.get(a_col), b_col);
    return view;
  }
}

#endif
