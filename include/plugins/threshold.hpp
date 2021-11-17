/*
 *
 * Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, Karl MacMillan
 *               2014      Christoph Dalitz
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

#ifndef kwm12032001_threshold
#define kwm12032001_threshold

#include "gamera.hpp"
#include "image_utilities.hpp"
#include "misc_filters.hpp"
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
    typename fact_type::image_type* view = fact_type::create(m.origin(), m.dim());
    threshold_fill(m, *view, threshold);
    return view;
  } else {
    typedef TypeIdImageFactory<ONEBIT, RLE> fact_type;
    typename fact_type::image_type* view = fact_type::create(m.origin(), m.dim());
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
    typename fact_type::image_type* view = fact_type::create(m.origin(), m.dim());
    threshold_fill(m, *view, threshold);
    return view;
  } else {
    typedef TypeIdImageFactory<ONEBIT, RLE> fact_type;
    typename fact_type::image_type* view = fact_type::create(m.origin(), m.dim());
    threshold_fill(m, *view, threshold);
    return view;
  }
}


/*
  threshold tsai_moment_preserving_find_threshold(GreyScale image);
  Image* tsai_moment_preserving_threshold(GreyScale image);

  tsai_moment_preserving_find_threshold finds a threshold point using the tsai moment preserving algorithm.
  tsai_moment_preserving_threshold returns a thresholded image using the tsai_moment_preserving_find_threshold algorithm
  to find the threshold point.
*/

template<class T>
int tsai_moment_preserving_find_threshold(const T& matrix) {

  int thresh;
  int i;
  double criterion;
  double m1, m2, m3;
  double cd, c0, c1, z0, z1, pd, p0;

  FloatVector* p = histogram(matrix);

  /* calculate first 3 moments */
  m1 = m2 = m3 = 0.0;
  for (i = 0; i < 256; i++) {
    m1 += i *(*p)[i];
    m2 += i * i * (*p)[i];
    m3 += i * i * i * (*p)[i];    
  }

  /* moment preserving bilevel thresholding calculations*/

  cd = m2 - m1 * m1;
  c0 = (-m2 * m2 + m1 * m3) / cd;
  c1 = (-m3 + m2 * m1) / cd;
  
  z0 = 0.5 * (-c1 - sqrt (c1 * c1 - 4.0 * c0));
  z1 = 0.5 * (-c1 + sqrt (c1 * c1 - 4.0 * c0));

  pd = z1 - z0;
  p0 = (z1 - m1) / pd;

  /* find threshold */
  for (thresh = 0, criterion = 0.0; thresh < 256; thresh++) {
    criterion += (*p)[thresh];
    if (criterion > p0)
      break;
  }

  delete p;
  return thresh;
}

template<class T>
Image* tsai_moment_preserving_threshold(const T &m, int storage_format) {
  int threshold = tsai_moment_preserving_find_threshold(m);
  if(threshold == 255)
    threshold = 0;
  if (storage_format == DENSE) {
    typedef TypeIdImageFactory<ONEBIT, DENSE> fact_type;
    typename fact_type::image_type* view = fact_type::create(m.origin(), m.dim());
    threshold_fill(m, *view, threshold);
    return view;
  } else {
    typedef TypeIdImageFactory<ONEBIT, RLE> fact_type;
    typename fact_type::image_type* view = fact_type::create(m.origin(), m.dim());
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
  view_type* average = mean(m);
  
  typedef FloatImageData histogram_data_type;
  typedef FloatImageView histogram_type;
  histogram_data_type histogram_data(Dim(256, 256));
  histogram_type histogram(histogram_data);
  histogram_data_type P_histogram_data(Dim(256, 256));
  histogram_type P_histogram(P_histogram_data);
  histogram_data_type H_histogram_data(Dim(256, 256));
  histogram_type H_histogram(H_histogram_data);

  typename histogram_type::vec_iterator hist_it = histogram.vec_begin();
  for (; hist_it != histogram.vec_end(); ++hist_it)
    *hist_it = 0.0;

  for (size_t y = 0; y < m.nrows(); ++y)
    for (size_t x = 0; x < m.ncols(); ++x) {
      size_t a = m.get(Point(x, y));
      size_t b = average->get(Point(x, y));
      histogram.set(Point(a, b), histogram.get(Point(a, b)) + 1.0);
    }
 
  double one_over_area = 1.0 / (m.nrows() * m.ncols());
  for (size_t b = 0; b < 256; ++b)
    for (size_t a = 0; a < 256; ++a)
      histogram.set(Point(a, b), histogram.get(Point(a, b)) * one_over_area);

  double P_sum = 0.0;
  for (size_t s = 0; s < 256; ++s) {
    P_sum += histogram.get(Point(s, 0));
    P_histogram.set(Point(s, 0), P_sum);
  }
  for (size_t t = 1; t < 256; ++t) {
    P_sum = 0.0;
    for (size_t s = 0; s < 256; ++s) {
      P_sum += histogram.get(Point(s, t));
      P_histogram.set(Point(s, t), P_histogram.get(Point(s, t - 1)) + P_sum);
    }
  }
  
  double H_sum = 0.0;
  for (size_t s = 0; s < 256; ++s) {
    double p = histogram.get(Point(s, 0));
    if (p != 0)
      H_sum -= p * log(p);
    H_histogram.set(Point(s, 0), H_sum);
  }
  for (size_t t = 1; t < 256; ++t) {
    H_sum = 0.0;
    for (size_t s = 0; s < 256; ++s) {
      double p = histogram.get(Point(s, t));
      if (p != 0)
        H_sum -= p * log(p);
      H_histogram.set(Point(s, t), H_histogram.get(Point(s, t - 1)) + H_sum);
    }
  }

  double Phi_max = std::numeric_limits<double>::min();
  double tiny = 1e-6;
  double H_end = H_histogram.get(Point(255, 255));
  size_t threshold = 0, avg_threshold = 0;
  for (size_t s = 0; s < 256; ++s)
    for (size_t t = 0; t < 256; ++t) {
      double P = P_histogram.get(Point(s, t));
      double H = H_histogram.get(Point(s, t));
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
    typename result_type::image_type* view = result_type::create(m.origin(), m.dim());
    for (size_t y = 0; y < m.nrows(); ++y)
      for (size_t x = 0; x < m.ncols(); ++x) {
        if (m.get(Point(x, y)) <= threshold && average->get(Point(x, y)) <= avg_threshold)
          view->set(Point(x, y), black(*view));
        else
          view->set(Point(x, y), white(*view));
      }
    delete average->data();
    delete average;
    return view;
  } else {
    typedef TypeIdImageFactory<ONEBIT, RLE> result_type;
    typename result_type::image_type* view = result_type::create(m.origin(), m.dim());
    for (size_t y = 0; y < m.nrows(); ++y) 
      for (size_t x = 0; x < m.ncols(); ++x) {
        if (m.get(Point(x, y)) <= threshold && average->get(Point(x, y)) <= avg_threshold)
          view->set(Point(x, y), black(*view));
        else
          view->set(Point(x, y), white(*view));
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
Image* bernsen_threshold(const T &m, int storage_format, size_t region_size, size_t contrast_limit, bool doubt_to_black) {
  if ((contrast_limit < 0) || (contrast_limit > 255))
    throw std::range_error("bernsen_threshold: contrast_limit out of range (0 - 255)");
  if ((region_size < 1) || (region_size > std::min(m.nrows(), m.ncols())))
    throw std::range_error("bernsen_threshold: region_size out of range");

  typedef typename T::value_type pixel_type;
  int half_region_size = region_size / 2;

  typedef TypeIdImageFactory<ONEBIT, DENSE> result_type;
  typename result_type::image_type* view = result_type::create(m.origin(), m.dim());
  OneBitPixel confused;
  if (doubt_to_black)
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
          pixel_type pixel = m.get(Point(x + use_dx, y + use_dy));
          minimum = std::min(minimum, pixel);
          maximum = std::max(maximum, pixel);
        }
      }
      pixel_type c = maximum - minimum;
      if (c < contrast_limit)
        view->set(Point(x, y), confused);
      else {
        long t = (maximum + minimum) / 2;
        if (m.get(Point(x, y)) >= t)
          view->set(Point(x, y), white(*view));
        else
          view->set(Point(x, y), black(*view));
      }
    }
  return view;
}

/*
Color-based thresholding using the algorithm from DjVu image
compression.  See:

Bottou, L., P. Haffner, P. G. Howard, P. Simard, Y. Bengio and
Y. LeCun.  1998.  High Quality Document Image Compression with DjVu.  AT&T
Labs, Lincroft, NJ.
http://research.microsoft.com/~patrice/PDF/jei.pdf

Solomon, D.  Image Compression: The Complete Reference.  2nd Edition.
559-61.
*/

template<class T, class U>
inline double
djvu_distance(const T& x, const U& y) {
  // This approximates YUV distance, which is far more natural
  // than RGB distance.
  double r = (double)x.red() - (double)y.red();
  double g = (double)x.green() - (double)y.green();
  double b = (double)x.blue() - (double)y.blue();
  return (0.75*r*r + g*g + 0.5*b*b);
}

#define CONVERGE_THRESHOLD 2
template<class T>
inline bool djvu_converged(const T& fg, const T& bg) {
  return (djvu_distance(fg, bg) < CONVERGE_THRESHOLD);
}

template<class T, class U>
void djvu_threshold_recurse(const T image, 
                            const double smoothness,
                            const size_t min_block_size,
                            U& fg_image, U& bg_image,
                            Rgb<double> fg_init, 
                            Rgb<double> bg_init, 
                            const size_t block_size) {
  //typedef typename T::value_type value_type;
  typedef Rgb<double> promote_t;

  promote_t fg = fg_init;
  promote_t bg = bg_init;
  promote_t last_fg, last_bg;
  bool fg_converged = false, bg_converged = false;
  promote_t fg_init_scaled = promote_t(fg_init) * smoothness;
  promote_t bg_init_scaled = promote_t(bg_init) * smoothness;
  do {
    last_fg = fg;
    last_bg = bg;
    promote_t fg_avg, bg_avg;
    size_t fg_count = 0, bg_count = 0;
    for (typename T::const_vec_iterator i = image.vec_begin();
         i != image.vec_end(); ++i) {
      double fg_dist = djvu_distance(*i, fg);
      double bg_dist = djvu_distance(*i, bg);
      if (fg_dist <= bg_dist) {
        fg_avg += *i;
        ++fg_count;
      } else {
        bg_avg += *i;
        ++bg_count;
      }
    }

    if (fg_count) {
      fg = (((fg_avg / fg_count) * (1.0 - smoothness)) + fg_init_scaled);
      fg_converged = djvu_converged(fg, last_fg);
    } else {
      fg_converged = true;
    }
    if (bg_count) {
      bg = (((bg_avg / bg_count) * (1.0 - smoothness)) + bg_init_scaled);
      bg_converged = djvu_converged(bg, last_bg);
    } else {
      bg_converged = true;
    }
  } while (!(fg_converged && bg_converged));

  if (block_size < min_block_size) {
    fg_image.set(Point(image.ul_x() / min_block_size,
                       image.ul_y() / min_block_size), fg);
    bg_image.set(Point(image.ul_x() / min_block_size,
                       image.ul_y() / min_block_size), bg);
  } else {
    for (size_t r = 0; r <= (image.nrows() - 1) / block_size; ++r) {
      for (size_t c = 0; c <= (image.ncols() - 1) / block_size; ++c) {
        Point ul(c * block_size + image.ul_x(), r * block_size + image.ul_y());
        Point lr(std::min((c + 1) * block_size + image.ul_x(), image.lr_x()),
                 std::min((r + 1) * block_size + image.ul_y(), image.lr_y()));
        djvu_threshold_recurse(T(image, ul, lr), smoothness, min_block_size, 
                               fg_image, bg_image, fg, bg, block_size / 2);
      }
    }
  }
}

template<class T>
Image *djvu_threshold(const T& image, const double smoothness, 
                      const size_t max_block_size, const size_t min_block_size,
                      const size_t block_factor,
                      const typename T::value_type init_fg, 
                      const typename T::value_type init_bg) {
  // Create some temporary images to store the foreground and 
  // background colors for each block

  RGBImageData fg_data(Dim(image.ncols() / min_block_size + 1,
                           image.nrows() / min_block_size + 1),
                       Point(0, 0));
  RGBImageView fg_image(fg_data);

  RGBImageData bg_data(Dim(image.ncols() / min_block_size + 1,
                           image.nrows() / min_block_size + 1),
                       Point(0, 0));
  RGBImageView bg_image(bg_data);

  djvu_threshold_recurse(image, smoothness, min_block_size, 
                         fg_image, bg_image,
                         init_fg, init_bg, max_block_size);

  typedef TypeIdImageFactory<ONEBIT, DENSE> result_type;
  typename result_type::image_type* result = result_type::create
    (image.origin(), image.dim());
  
  typename choose_accessor<T>::interp_accessor fg_acc = 
    choose_accessor<T>::make_interp_accessor(fg_image);
  typename choose_accessor<T>::interp_accessor bg_acc = 
    choose_accessor<T>::make_interp_accessor(bg_image);

  for (size_t r = 0; r < image.nrows(); ++r) {
    for (size_t c = 0; c < image.ncols(); ++c) {
      double c_frac = std::max((double)0,std::min(((double)c - (0.5 * min_block_size)) / min_block_size,floor((double)(image.ncols() - 1)/min_block_size)));
      double r_frac = std::max((double)0,std::min(((double)r - (0.5 * min_block_size)) / min_block_size,floor((double)(image.nrows() - 1)/min_block_size)));
      RGBPixel fg = fg_acc(fg_image.upperLeft(), c_frac, r_frac); 
      RGBPixel bg = bg_acc(bg_image.upperLeft(), c_frac, r_frac);
      double fg_dist = djvu_distance(image.get(Point(c, r)), fg);
      double bg_dist = djvu_distance(image.get(Point(c, r)), bg);
      if (fg_dist <= bg_dist)
        result->set(Point(c, r), black(*result));
      else
        result->set(Point(c, r), white(*result));
    }
  }

  return result;
}

Image *djvu_threshold(const RGBImageView& image, double smoothness = 0.2, 
                      int max_block_size = 512, int min_block_size = 16, 
                      int block_factor = 2) {
  // We do an approximate histrogram here, using 6 bits per pixel
  // plane.  That greatly reduces the amount of memory required.
  RGBPixel max_color;
  {
    size_t max_count = 0;
    std::vector<size_t> histogram(64 * 64 * 64, 0);
    for (RGBImageView::const_vec_iterator i = image.vec_begin();
         i != image.vec_end(); ++i) {
      size_t approx_color = (((size_t)((*i).red() & 0xfc) << 10) |
                             ((size_t)((*i).green() & 0xfc) << 4) |
                             ((size_t)((*i).blue() & 0xfc) >> 2));
      size_t x = histogram[approx_color]++;
      if (x > max_count) {
        max_count = x;
        max_color = RGBPixel((*i).red() & 0xfc,
                             (*i).green() & 0xfc,
                             (*i).blue() & 0xfc);
      }
    }
  }

  if (max_color.red() < 128 || max_color.green() < 128 || max_color.blue() < 128)
    max_color = RGBPixel(255, 255, 255);

  return djvu_threshold(image, smoothness, max_block_size, min_block_size, 
                        block_factor, RGBPixel(0, 0, 0), max_color);
}


//
// soft thresholding after
// Dalitz: "Soft Thresholding for Visual Image Enhancement."
// Technischer Bericht Nr. 2014-01, Hochschule Niederrhein,
// Fachbereich Elektrotechnik und Informatik (2014)
//
template<class T>
double soft_threshold_find_sigma(const T& src, typename T::value_type t, int dist) {

  size_t i;
  double sigma = 0.0;
  const double sqrt3 = sqrt(3.0);

  FloatVector* h = histogram(src);
  double v_w = 0.0;
  double hsum = 0.0;
  for (i=t+1; i<h->size(); i++) {
    v_w += i*h->at(i);
    hsum += h->at(i);
  }
  if (hsum > 0.0) {
    v_w = v_w/hsum;
    if (dist==0) { // logistic distribution
      sigma = M_PI*(v_w-t)/(4.595120*sqrt3);
    }
    else if (dist==1) { // normal distribution
      sigma = (v_w-t)/2.236348;
    }
    else { // uniform distribution
      sigma = (v_w-t)/sqrt3;
    }
  }
  delete h;
  return sigma;
}

template<class T>
typename ImageFactory<T>::view_type*  soft_threshold(const T& src, typename T::value_type t, double sigma, int dist) {

  typedef typename ImageFactory<T>::data_type data_type;
  typedef typename ImageFactory<T>::view_type view_type;
  typedef typename T::value_type T_value_type;

  size_t i,x,y;
  size_t maxv = std::numeric_limits<T_value_type>::max() + 1;
  //maxv = 256;
  std::vector<T_value_type> transform(maxv);
  const double sqrt3 = sqrt(3.0);

  if (sigma == 0.0) {
    sigma = soft_threshold_find_sigma(src, t, dist);
  }
  //printf("sigma=%f\n", sigma);
  if (sigma == 0.0) { // may still occur when no values above t set
    for (i=0; i<=t; i++) transform[i] = black(src);
    for (i=t+1; i<maxv; i++) transform[i] = white(src);
  }
  else {
    if (dist==0) { // logistic distribution
      double theta = sigma*sqrt3/M_PI;
      for (i=0; i<maxv; i++)
        transform[i] = (T_value_type)((maxv-1)/(1+exp((t-float(i))/theta))+0.5);
    }
    else if (dist==1) { // normal distribution
      double sq2sigma = sqrt(2.0)*sigma;
      for (i=0; i<maxv; i++)
        transform[i] = (T_value_type)((maxv-1)*0.5*(1+vigra::erf((float(i)-t)/sq2sigma))+0.5);
    }
    else { // uniform distribution
      double h2 = sigma*sqrt3;
      size_t i1 = (size_t)(t-h2+0.5);
      size_t i2 = (size_t)(t+h2);
      for (i=0; i<=i1; i++)
        transform[i] = black(src);
      for (i=i1+1; i<i2; i++)
        transform[i] = (T_value_type)((maxv-1)*0.5*(1+(float(i)-t)/h2)+0.5);
      for (i=i2; i<maxv; i++)
        transform[i] = white(src);
    }
    //printf("%i -> %i\n", i, transform[i]);
  }

  data_type *res_data = new data_type(src.size(), src.origin());
  view_type *res= new view_type(*res_data);

  for (y=0; y<src.nrows(); y++) {
    for (x=0; x<src.ncols(); x++) {
      res->set(Point(x,y), transform[src.get(Point(x,y))]);
    }
  }

  return res;
}


#endif


