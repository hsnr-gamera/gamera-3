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
#include "morphology.hpp"
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

/*

________________________________________________________________

        bin_ab.c
        $Id$
        Copyright 1990, Blab, UiO
        Image processing lab, Department of Informatics
        University of Oslo
        E-mail: blab@ifi.uio.no
________________________________________________________________
  
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
 
References:
&Ahmed S. Abutaleb
"Automatic thresholding of gray-level pictures using
two-dimensional entropy",
Computer Vision, Graphics, and Image Processing,
vol 47, pp 22-32, 1989.

Original author:
Øivind Due Trier
*/

template<class T>
Image* abutaleb_threshold(const T &m, int storage_format) {
  typedef typename ImageFactory<T>::view_type view_type;
  view_type* average = simple_image_copy(m);
  mean(*average);
  
  typedef FloatImageData histogram_data_type;
  typedef FloatImageView histogram_type;
  histogram_data_type histogram_data(256, 256);
  histogram_type histogram(histogram_data);
  histogram_data_type P_histogram_data(256, 256);
  histogram_type P_histogram(P_histogram_data);
  histogram_data_type H_histogram_data(256, 256);
  histogram_type H_histogram(H_histogram_data);

  typename histogram_type::vec_iterator hist_it = histogram.vec_begin();
  for (; hist_it != histogram.vec_end(); ++hist_it)
    *hist_it = 0.0;

  for (size_t y = 0; y < m.nrows(); ++y)
    for (size_t x = 0; x < m.ncols(); ++x) {
      size_t a = m.get(y, x);
      size_t b = average->get(y, x);
      histogram.set(b, a, histogram.get(b, a) + 1.0);
    }
 
  double one_over_area = 1.0 / (m.nrows() * m.ncols());
  for (size_t b = 0; b < 256; ++b)
    for (size_t a = 0; a < 256; ++a)
      histogram.set(b, a, histogram.get(b, a) * one_over_area);

  double P_sum = 0.0;
  for (size_t s = 0; s < 256; ++s) {
    P_sum += histogram.get(0, s);
    P_histogram.set(0, s, P_sum);
  }
  for (size_t t = 1; t < 256; ++t) {
    P_sum = 0.0;
    for (size_t s = 0; s < 256; ++s) {
      P_sum += histogram.get(t, s);
      P_histogram.set(t, s, P_histogram.get(t - 1, s) + P_sum);
    }
  }
  
  double H_sum = 0.0;
  for (size_t s = 0; s < 256; ++s) {
    double p = histogram.get(0, s);
    if (p != 0)
      H_sum -= p * log(p);
    H_histogram.set(0, s, H_sum);
  }
  for (size_t t = 1; t < 256; ++t) {
    H_sum = 0.0;
    for (size_t s = 0; s < 256; ++s) {
      double p = histogram.get(t, s);
      if (p != 0)
	H_sum -= p * log(p);
      H_histogram.set(t, s, H_histogram.get(t - 1, s) + H_sum);
    }
  }

  double Phi_max = std::numeric_limits<double>::min();
  double tiny = 1e-6;
  double H_end = H_histogram.get(255, 255);
  size_t threshold = 0, avg_threshold = 0;
  for (size_t s = 0; s < 256; ++s)
    for (size_t t = 0; t < 256; ++t) {
      double P = P_histogram.get(t, s);
      double H = H_histogram.get(t, s);
      if ((P > tiny) && ((1.0 - P) > tiny)) {	
	double Phi = log(P * (1.0 - P)) + H / P + (H_end - H) / (1.0 - P);
	if (Phi > Phi_max) {
	  Phi_max = Phi;
	  threshold = s;
	  avg_threshold = t;
	}
      }
    }

  if (storage_format == DENSE) {
    typedef TypeIdImageFactory<ONEBIT, DENSE> result_type;
    typename result_type::image_type* view = result_type::create(m.offset_y(), m.offset_x(),
								 m.nrows(), m.ncols());
    for (size_t y = 0; y < m.nrows(); ++y)
      for (size_t x = 0; x < m.ncols(); ++x) {
	if (m.get(y, x) <= threshold && average->get(y, x) <= avg_threshold)
	  view->set(y, x, black(*view));
	else
	  view->set(y, x, white(*view));
      }
    delete average->data();
    delete average;
    return view;
  } else {
    typedef TypeIdImageFactory<ONEBIT, RLE> result_type;
    typename result_type::image_type* view = result_type::create(m.offset_y(), m.offset_x(),
								 m.nrows(), m.ncols());
    for (size_t y = 0; y < m.nrows(); ++y) 
      for (size_t x = 0; x < m.ncols(); ++x) {
	if (m.get(y, x) <= threshold && average->get(y, x) <= avg_threshold)
	  view->set(y, x, black(*view));
	else
	  view->set(y, x, white(*view));
      }

    delete average->data();
    delete average;
    return view;
  }
}  

/*
  References:
  &'John Bernsen'
  "Dynamic thresholding of grey-level images", 
  Proc. 8th International Conference on Pattern 
  Recognition (ICPR8), pp 1251-1255, Paris, France, 
  October 1986.

  Original author:
  Øivind Due Trier
*/

template<class T>
Image* bernsen_threshold(const T &m, int storage_format, size_t region_size, size_t contrast_limit, bool set_doubt_to_low) {
  if ((contrast_limit < 0) || (contrast_limit > 255))
    throw std::range_error("bernsen_threshold: contrast_limit out of range (0 - 255)");
  if ((region_size < 1) || (region_size > std::min(m.nrows(), m.ncols())))
    throw std::range_error("bernsen_threshold: region_size out of range");

  typedef typename T::value_type pixel_type;
  int half_region_size = region_size / 2;

  //  if (storage_format == DENSE) {
    typedef TypeIdImageFactory<ONEBIT, DENSE> result_type;
    typename result_type::image_type* view = result_type::create(m.offset_y(), m.offset_x(),
								 m.nrows(), m.ncols());
    //  } else {
    //typedef TypeIdImageFactory<ONEBIT, RLE> result_type;
    //typename result_type::image_type* view = result_type::create(m.offset_y(), m.offset_x(),
    //					 m.nrows(), m.ncols());
//  }

  OneBitPixel confused;
  if (set_doubt_to_low)
    confused = black(*view);
  else
    confused = white(*view);

  for (size_t y = 0; y < m.nrows(); ++y)
    for (size_t x = 0; x < m.ncols(); ++x) {
      pixel_type minimum = std::numeric_limits<pixel_type>::max();
      pixel_type maximum = 0;
      for (int dy = -half_region_size; dy < half_region_size; ++dy) {
	int use_dy = (y + dy < 0 || y + dy >= m.nrows()) ? -dy : dy;
	for (int dx = -half_region_size; dx < half_region_size; ++dx) {
	  int use_dx = (x + dx < 0 || x + dx >= m.ncols()) ? -dx : dx;
	  pixel_type pixel = m.get(y + use_dy, x + use_dx);
	  minimum = std::min(minimum, pixel);
	  maximum = std::max(maximum, pixel);
	}
      }
      pixel_type c = maximum - minimum;
      if (c < contrast_limit)
	view->set(y, x, confused);
      else {
	long t = (maximum + minimum) / 2;
	if (m.get(y, x) >= t)
	  view->set(y, x, white(*view));
	else
	  view->set(y, x, black(*view));
      }
    }
  return view;
}

#endif


