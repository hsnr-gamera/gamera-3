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

#ifndef kwm12032001_threshold
#define kwm12032001_threshold

#include "gamera.hpp"
#include "image_utilities.hpp"
#include <exception>
#include <vector>
#include <algorithm>

using namespace Gamera;

/*
  void threshold_fill(GreyScale|Grey16|Float image, OneBit image, threshold);

  This function will threshold an image, storing the result in the second
  image passed in. The dimensions of the image must match. This function is
  used by threshold internally and is useful when there are existing images.
  Presumably the second image is a OneBit image, but it is not required - any
  pixel type should work fine.
*/
template<class T, class U>
void threshold_fill(const T& in, U& out, typename T::value_type threshold) {
  if (in.nrows() != out.nrows() || in.ncols() != out.ncols())
    throw std::range_error("Dimensions must match!");

  typename T::const_row_iterator in_row = in.row_begin();
  typename T::const_col_iterator in_col;
  typename U::row_iterator out_row = out.row_begin();
  typename U::col_iterator out_col;

  ImageAccessor<typename T::value_type> in_acc;
  ImageAccessor<typename U::value_type> out_acc;
  typename T::value_type tmp;

  for (; in_row != in.row_end(); ++in_row, ++out_row) {
    for (in_col = in_row.begin(), out_col = out_row.begin(); in_col != in_row.end();
	 ++in_col, ++out_col) {
      tmp = in_acc.get(in_col);
      if (tmp > threshold)
	out_acc.set(white(out), out_col);
      else
	out_acc.set(black(out), out_col);
    }
  }
}

/*
  Image* threshold(GreyScale|Grey16|Float image, threshold, storage_format);

  Threshold an image return a OneBit thresholded type. The storage format is
  controlled by the third parameter and should be one the formats in image_types.hpp.
*/
template<class T>
Image* threshold(const T &m, int threshold, int storage_format) {
  if (storage_format == DENSE) {
    typedef TypeIdImageFactory<ONEBIT, DENSE> fact_type;
    typename fact_type::image_type* view = fact_type::create(m.offset_y(), m.offset_x(),
							     m.nrows(), m.ncols());
    threshold_fill(m, *view, threshold);
    return view;
  } else {
    typedef TypeIdImageFactory<ONEBIT, RLE> fact_type;
    typename fact_type::image_type* view = fact_type::create(m.offset_y(), m.offset_x(),
							     m.nrows(), m.ncols());
    threshold_fill(m, *view, threshold);
    return view;
  }
}

/*
  threshold otsu_find_threshold(GreyScale image);
  Image* otsu_threshold(GreyScale image);

  otsu_find_threshold finds a threshold point using the otsu algorithm.
  otsu_threshold returns a thresholded image using the otsu algorithm
  to find the threshold point.
*/
// Here is the original copyright notice from the software this was 
// adopted from

/*
  Permission to use, copy, modify and distribute this software and its
  documentation for any purpose and without fee is hereby granted, 
  provided that this copyright notice appear in all copies and that 
  both that copyright notice and this permission notice appear in supporting
  documentation and that the name of B-lab, Department of Informatics or
  University of Oslo not be used in advertising or publicity pertaining 
  to distribution of the software without specific, written prior permission.

  B-LAB DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL B-LAB
  BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
  OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
  CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 

*/
template<class T>
int otsu_find_threshold(const T& matrix) {
  int thresh;
  double criterion;
  double expr_1;
  int i, k;
  double omega_k;
  double sigma_b_k;
  double sigma_T;
  double mu_T;
  double mu_k;
  int k_low, k_high;

  FloatVector* p = histogram(matrix);

  mu_T = 0.0;
  for (i=0; i<256; i++)
    mu_T += i*(*p)[i];

  sigma_T = 0.0;
  for (i=0; i<256; i++)
    sigma_T += (i-mu_T)*(i-mu_T)*(*p)[i];

  
  for (k_low = 0; ((*p)[k_low] == 0) && (k_low < 255); k_low++);
  for (k_high = 255; ((*p)[k_high] == 0) && (k_high > 0); k_high--);

  criterion = 0.0;
  thresh = 127;

  omega_k = 0.0;
  mu_k = 0.0;
  for (k = k_low; k <= k_high ; k++)
    {
      omega_k += (*p)[k];
      mu_k += k*(*p)[k];

      expr_1 = (mu_T*omega_k - mu_k);
      sigma_b_k = expr_1 * expr_1 / (omega_k*(1-omega_k));
      if (criterion < sigma_b_k/sigma_T)
	{
	  criterion = sigma_b_k/sigma_T;
	  thresh = k;
	}
    }
  delete p;
  return thresh;
}

template<class T>
Image* otsu_threshold(const T &m, int storage_format) {
  int threshold = otsu_find_threshold(m);
  if (storage_format == DENSE) {
    typedef TypeIdImageFactory<ONEBIT, DENSE> fact_type;
    typename fact_type::image_type* view = fact_type::create(m.offset_y(), m.offset_x(),
							     m.nrows(), m.ncols());
    threshold_fill(m, *view, threshold);
    return view;
  } else {
    typedef TypeIdImageFactory<ONEBIT, RLE> fact_type;
    typename fact_type::image_type* view = fact_type::create(m.offset_y(), m.offset_x(),
							     m.nrows(), m.ncols());
    threshold_fill(m, *view, threshold);
    return view;
  }
}

#endif


