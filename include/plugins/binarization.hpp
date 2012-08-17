/*
 *
 * Copyright (C) 2005 John Ashley Burgoyne and Ichiro Fujinaga
 *               2007 Uma Kompella and Christoph Dalitz
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef jab18112005_binarization
#define jab18112005_binarization

#include "gamera.hpp"
#include "threshold.hpp"
#include "math.h"
#include <numeric>
#include <vector>
#include <algorithm>

#include <iostream>

using namespace Gamera;

/* Adaptive function for summing to double. */
template<class T>
struct double_plus : public std::binary_function<double, double, T>
{
    double operator()(double x, T y) { return x + (double)y; }
};

/* Adaptive function for squaring to doubles. */
template<class T>
struct double_squared : public std::unary_function<double, T> 
{
    double operator()(T x) { return (double)x * (double)x; }
};

/* Adaptive function for adding pairs. */
template<class T>
struct pair_plus : public std::binary_function<T, T, T>
{
    T operator()(T p, T q) 
        {
            return std::make_pair(p.first + q.first, 
                                  p.second + q.second);
        }
};

/* Binary function for accumulating background pixels. */
template<class T, class U, class V>
struct gatos_accumulate 
    : public std::binary_function<T, U, V>
{
    typedef typename T::first_type type1;
    typedef typename T::second_type type2;
    T operator()(U mask, V pixel)
        {
            if (is_black(mask)) return std::make_pair((type1)0, (type2)0);
            else return std::make_pair((type1)1, (type2)pixel);
        }
};

/* Binary function for Gatos thresholding. */
template<class T, class U>
struct gatos_thresholder
    : public std::binary_function<U, T, T>
{
    const double q;
    const double delta;
    const double b;
    const double p1;
    const double p2;

    gatos_thresholder(double q, double delta, double b, double p1, double p2)
        : q(q), delta(delta), b(b), p1(p1), p2(p2) {} 
   
    U operator()(T src, T background)
        {
            return 
                ((double)(background - src) 
                 > (q 
                    * delta 
                    * (((1 - p2) 
                        / (1 
                           + std::exp(((-4 * background) / (b * (1 - p1))) 
                                      + ((2 * (1 + p1)) / (1 - p1))))) 
                       + p2)))
                ? pixel_traits<U>::black()
                : pixel_traits<U>::white();
        }
};

/* FloatPixel image_mean(Image src)
 *
 * Returns the mean value over all pixels of an image.
 */
template<class T>
FloatPixel image_mean(const T &src)
{
    FloatPixel sum 
        = std::accumulate(src.vec_begin(), 
                          src.vec_end(), 
                          (FloatPixel)0,
                          double_plus<typename T::value_type>());
    size_t area = src.nrows() * src.ncols();
    return sum / area;
}

/* FloatPixel image_variance(Image src)
 *
 * Returns the variance over all pixels of an image.
 */
template<class T>
FloatPixel image_variance(const T &src)
{
    FloatImageData* squaredData = new FloatImageData(src.size(), src.origin());
    FloatImageView* squares = new FloatImageView(*squaredData);

    transform(src.vec_begin(), 
              src.vec_end(), 
              squares->vec_begin(), 
              double_squared<typename T::value_type>());

    FloatPixel sum
        = std::accumulate(squares->vec_begin(), 
                          squares->vec_end(), 
                          (FloatPixel)0);
    size_t area = src.nrows() * src.ncols();
    FloatPixel mean = image_mean(src);
    
    delete squaredData;
    delete squares;
    return sum / area - mean * mean;
}

/* Float mean_filter(Image src, size_t region_size);
 *
 * The implementation of region size is not entirely correct because of
 * integer rounding but matches the implementation of the thresholding
 * algorithms.
 */
template<class T>
FloatImageView* mean_filter(const T &src, size_t region_size)
{
    if ((region_size < 1) || (region_size > std::min(src.nrows(), src.ncols())))
        throw std::out_of_range("mean_filter: region_size out of range");

    size_t half_region_size = region_size / 2;

    typename ImageFactory<T>::view_type* copy = ImageFactory<T>::new_view(src);
    FloatImageData* data = new FloatImageData(src.size(), src.origin());
    FloatImageView* view = new FloatImageView(*data);
  
    for (coord_t y = 0; y < src.nrows(); ++y) {
        for (coord_t x = 0; x < src.ncols(); ++x) {
            // Define the region.
            Point ul((coord_t)std::max(0, (int)x - (int)half_region_size),
                     (coord_t)std::max(0, (int)y - (int)half_region_size));
            Point lr((coord_t)std::min(x + half_region_size, src.ncols() - 1),
                     (coord_t)std::min(y + half_region_size, src.nrows() - 1));
            copy->rect_set(ul, lr);
            view->set(Point(x, y), image_mean(*copy));
        }
    }

    delete copy;
    return view;
}

/* Image variance_filter(Image src, Float means, size_t region_size);
 *
 * The implementation of region size is not entirely correct because of
 * integer rounding but matches the implementation of the thresholding
 * algorithms.
 */
template<class T>
FloatImageView* variance_filter(const T &src,
                                const FloatImageView &means,
                                size_t region_size) 
{
    if ((region_size < 1) || (region_size > std::min(src.nrows(), src.ncols())))
        throw std::out_of_range("variance_filter: region_size out of range");
     if (src.size() != means.size())
        throw std::invalid_argument("variance_filter: sizes must match");
 
    size_t half_region_size = region_size / 2;

    // Compute squares of each element. This step avoid repeating the squaring
    // operation for overlapping regions.
    FloatImageData* squaredData = new FloatImageData(src.size(), src.origin());
    FloatImageView* squares = new FloatImageView(*squaredData);

    transform(src.vec_begin(), 
              src.vec_end(), 
              squares->vec_begin(), 
              double_squared<typename T::value_type>());
  
    FloatImageData* data = new FloatImageData(src.size(), src.origin());
    FloatImageView* view = new FloatImageView(*data);  

    for (coord_t y = 0; y < src.nrows(); ++y) {
        for (coord_t x = 0; x < src.ncols(); ++x) {
            // Define the region.
            Point ul((coord_t)std::max(0, (int)x - (int)half_region_size),
                     (coord_t)std::max(0, (int)y - (int)half_region_size));
            Point lr((coord_t)std::min(x + half_region_size, src.ncols() - 1),
                     (coord_t)std::min(y + half_region_size, src.nrows() - 1));
            squares->rect_set(ul, lr);
            // Compute the variance.
            FloatPixel sum
                = std::accumulate(squares->vec_begin(), 
                                  squares->vec_end(), 
                                  (FloatPixel)0);
            size_t area = squares->nrows() * squares->ncols();
            FloatPixel mean = means.get(Point(x,y));
            view->set(Point(x, y), sum / area - mean * mean);
        }
    }
    
    delete squaredData;
    delete squares;
    return view;
}

/*
 * Image wiener_filter(Image src, size_t region_size, double noise_variance);
 * 
 */
template<class T>
T* wiener_filter(const T &src, size_t region_size, double noise_variance)
{
    if ((region_size < 1) || (region_size > std::min(src.nrows(), src.ncols())))
        throw std::out_of_range("niblack_threshold: region_size out of range");
    
    // Compute regional statistics.
    const FloatImageView* means = mean_filter(src, region_size);
    const FloatImageView* variances = variance_filter(src, *means, region_size);

    // Compute noise variance if needed.
    if (noise_variance < 0) {
        FloatImageData* orderedVariancesData 
            = new FloatImageData(variances->size(), variances->origin());
        FloatImageView* orderedVariances 
            = new FloatImageView(*orderedVariancesData);        
        std::copy(variances->vec_begin(),
                  variances->vec_end(),
                  orderedVariances->vec_begin());
        size_t area = orderedVariances->nrows() * orderedVariances->ncols();
        std::nth_element(orderedVariances->vec_begin(),
                         orderedVariances->vec_begin() + (area - 1) / 2,
                         orderedVariances->vec_end());
        noise_variance 
            = (double)*(orderedVariances->vec_begin() + (area - 1) / 2);
        delete orderedVariancesData;
        delete orderedVariances;
    }

    typedef typename T::value_type value_type;
    typedef typename ImageFactory<T>::data_type data_type;
    typedef typename ImageFactory<T>::view_type view_type;
    data_type* data = new data_type(src.size(), src.origin());
    view_type* view = new view_type(*data);

    for (coord_t y = 0; y < src.nrows(); ++y) {
        for (coord_t x = 0; x < src.ncols(); ++x) {
            double mean = (double)means->get(Point(x, y));
            double variance = (double)variances->get(Point(x, y));
            // The estimate of noise variance will never be perfect, but in
            // theory, it would be impossible for any region to have a local
            // variance less than it. The following check eliminates that
            // theoretical impossibility and has a side benefit of preventing
            // division by zero.
            if (variance < noise_variance) {
                view->set(Point(x, y), (value_type)mean);
            } else {
                double multiplier = (variance - noise_variance) / variance;
                double value = (double)src.get(Point(x, y));
                view->set(Point(x, y),
                          (value_type)(mean + multiplier * (value - mean)));
            }
        }
    }

    delete means->data(); delete means;
    delete variances->data(); delete variances;
    return view;
}

/*
 * OneBit niblack_threshold(GreyScale src, 
 *                          size_t region_size, 
 *                          double sensitivity,
 *                          int lower_bound,
 *                          int upper_bound);
 */
template<class T>
OneBitImageView* niblack_threshold(const T &src, 
                                   size_t region_size, 
                                   double sensitivity,
                                   int lower_bound,
                                   int upper_bound)
{
    if ((region_size < 1) || (region_size > std::min(src.nrows(), src.ncols())))
        throw std::out_of_range("niblack_threshold: region_size out of range");

    // Compute regional statistics.
    const FloatImageView* means = mean_filter(src, region_size);
    const FloatImageView* variances = variance_filter(src, *means, region_size);

    typedef ImageFactory<OneBitImageView>::data_type data_type;
    typedef ImageFactory<OneBitImageView>::view_type view_type;
    data_type* data = new data_type(src.size(), src.origin());
    view_type* view = new view_type(*data);

    for (coord_t y = 0; y < src.nrows(); ++y) {
        for (coord_t x = 0; x < src.ncols(); ++x) {
            // Check global thresholds and then threshold adaptively.
            FloatPixel pixel_value = (FloatPixel)src.get(Point(x, y));
            if (pixel_value < (FloatPixel)lower_bound) {
                view->set(Point(x, y), black(*view));
            } else if (pixel_value >= (FloatPixel)upper_bound) {
                view->set(Point(x, y), white(*view));
            } else {
                FloatPixel mean = means->get(Point(x, y));
                FloatPixel deviation = std::sqrt(variances->get(Point(x, y)));
                FloatPixel threshold = mean + sensitivity * deviation;
                view->set(Point(x, y), 
                          pixel_value > threshold ? white(*view) : black(*view));
            }
        }
    }

    delete means->data(); delete means;
    delete variances->data(); delete variances;
    return view;
}

/*
 * OneBit sauvola_threshold(GreyScale src, 
 *                          size_t region_size, 
 *                          double sensitivity,
 *                          int dynamic_range,
 *                          int lower_bound,
 *                          int upper_bound);
 */
template<class T>
OneBitImageView* sauvola_threshold(const T &src, 
                                   size_t region_size, 
                                   double sensitivity,
                                   int dynamic_range,
                                   int lower_bound,
                                   int upper_bound)
{
    if ((region_size < 1) || (region_size > std::min(src.nrows(), src.ncols())))
        throw std::out_of_range("niblack_threshold: region_size out of range");

    // Compute regional statistics.
    const FloatImageView* means = mean_filter(src, region_size);
    const FloatImageView* variances = variance_filter(src, *means, region_size);

    typedef ImageFactory<OneBitImageView>::data_type data_type;
    typedef ImageFactory<OneBitImageView>::view_type view_type;
    data_type* data = new data_type(src.size(), src.origin());
    view_type* view = new view_type(*data);

    for (coord_t y = 0; y < src.nrows(); ++y) {
        for (coord_t x = 0; x < src.ncols(); ++x) {
            // Check global thresholds and then threshold adaptively.
            FloatPixel pixel_value = (FloatPixel)src.get(Point(x, y));
            if (pixel_value < (FloatPixel)lower_bound) {
                view->set(Point(x, y), black(*view));
            } else if (pixel_value >= (FloatPixel)upper_bound) {
                view->set(Point(x, y), white(*view));
            } else {
                FloatPixel mean = means->get(Point(x, y));
                FloatPixel deviation = std::sqrt(variances->get(Point(x, y)));
                FloatPixel adjusted_deviation 
                    = 1.0 - deviation / (FloatPixel)dynamic_range;
                FloatPixel threshold 
                    = mean + (1.0 - sensitivity * adjusted_deviation);
                view->set(Point(x, y), 
                          pixel_value > threshold ? white(*view) : black(*view));
            }
        }
    }

    delete means->data(); delete means;
    delete variances->data(); delete variances;
    return view;
}

/* 
 * Image* gatos_background(Image src, size_t region_size);
 */
template<class T, class U>
T* gatos_background(const T &src, 
                    const U &binarization, 
                    size_t region_size)
{
    if ((region_size < 1) || (region_size > std::min(src.nrows(), src.ncols())))
        throw std::out_of_range("gatos_background: region_size out of range");
    if (src.size() != binarization.size())
        throw std::invalid_argument("gatos_background: sizes must match");
 
    size_t half_region_size = region_size / 2;

    typename ImageFactory<T>::view_type* scopy 
        = ImageFactory<T>::new_view(src);
    typename ImageFactory<U>::view_type* bcopy
        = ImageFactory<U>::new_view(binarization);

    typedef std::pair<unsigned int, FloatPixel> gatos_pair;
    typedef typename T::value_type src_value_type;
    typedef typename U::value_type binarization_value_type;

    typedef typename ImageFactory<T>::data_type data_type;
    typedef typename ImageFactory<T>::view_type view_type;
    data_type* data = new data_type(src.size(), src.origin());
    view_type* view = new view_type(*data);

    for (coord_t y = 0; y < src.nrows(); ++y) {
        for (coord_t x = 0; x < src.ncols(); ++x) {
            if (is_white(binarization.get(Point(x, y)))) {
                view->set(Point(x, y), src.get(Point(x, y)));
            } else {
                // Define the region.
                Point ul((coord_t)std::max(0, (int)x - (int)half_region_size),
                         (coord_t)std::max(0, (int)y - (int)half_region_size));
                Point lr((coord_t)std::min(x + half_region_size, src.ncols() - 1),
                         (coord_t)std::min(y + half_region_size, src.nrows() - 1));
                scopy->rect_set(ul, lr);
                bcopy->rect_set(ul, lr);
                // Count and accumulate background pixels.
                gatos_pair sums =
                    std::inner_product(bcopy->vec_begin(),
                                       bcopy->vec_end(),
                                       scopy->vec_begin(),
                                       gatos_pair(0, 0.0),
                                       pair_plus<gatos_pair>(),
                                       gatos_accumulate
                                       <
                                       gatos_pair,
                                       binarization_value_type,
                                       src_value_type
                                       >());
                view->set(Point(x, y), 
                          sums.first > 0
                          ? (src_value_type)(sums.second / sums.first)
                          : white(src));
            }
        }
    }

    delete scopy;
    delete bcopy;
    return view;
}


/*
 * Image gatos_threshold(Image src, 
 *                       Image background, 
 *                       Image binarization,
 *                       double q,
 *                       double p1,
 *                       double p2);
 */
template<class T, class U>
OneBitImageView* gatos_threshold(const T &src, 
                                 const T &background, 
                                 const U &binarization,
                                 double q,
                                 double p1,
                                 double p2)
{
    if (src.size() != background.size())
        throw std::invalid_argument("gatos_threshold: sizes must match");
    if (background.size() != binarization.size())
        throw std::invalid_argument("gatos_threshold: sizes must match");

    typedef std::pair<unsigned int, FloatPixel> gatos_pair;
    typedef typename T::value_type base_value_type;
    typedef typename U::value_type binarization_value_type;

    double delta_numerator 
        = std::inner_product(src.vec_begin(),
                             src.vec_end(),
                             background.vec_begin(),
                             (double)0,
                             double_plus<base_value_type>(),
                             std::minus<base_value_type>());
    unsigned int delta_denominator
        = std::count_if(binarization.vec_begin(),
                        binarization.vec_end(),
                        is_black<binarization_value_type>);
    double delta = delta_numerator / (double)delta_denominator;
                             
    gatos_pair b_sums
        = std::inner_product(binarization.vec_begin(),
                             binarization.vec_end(),
                             background.vec_begin(),
                             gatos_pair(0, 0.0),
                             pair_plus<gatos_pair>(),
                             gatos_accumulate
                             <
                             gatos_pair,
                             binarization_value_type,
                             base_value_type
                             >());
    double b = (double)b_sums.second / (double)b_sums.first;

    typedef ImageFactory<OneBitImageView>::data_type data_type;
    typedef ImageFactory<OneBitImageView>::view_type view_type;
    data_type* data = new data_type(src.size(), src.origin());
    view_type* view = new view_type(*data);

    std::transform(src.vec_begin(), 
                   src.vec_end(),
                   background.vec_begin(),
                   view->vec_begin(),
                   gatos_thresholder
                   <
                   typename T::value_type, 
                   typename U::value_type
                   >(q, delta, b, p1, p2));

    return view;
}


/*
 White Rohrer thresholding. This implementation uses code from
 the XITE library. According to its license, it may be freely included
 into Gamera (a GPL licensed software), provided the following
 notice is included into the code:

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

 Important notice: this implementation only works with 8-bit greyscale
 images because the maximal value 255 for white is hard coded!!
*/

static struct {
  int WR1_F_OFFSET;
  int WR1_G_OFFSET;
  int BIN_ERROR;
  int BIN_FOREGROUND;
  int BIN_BACKGROUND;
  int BIN_OK;
  int WR1_BIAS_CROSSOVER;
  int WR1_BLACK_BIAS;
  int WR1_WHITE_BIAS;
  int WR1_BIAS;
  double WR1_BLACK_BIAS_FACTOR;
  double WR1_WHITE_BIAS_FACTOR;
  int wr1_f_tab[512];
  int wr1_g_tab[512];
} wr1_params = {
  /* WR1_F_OFFSET */  255,
  /* WR1_G_OFFSET */  255,
  /* BIN_ERROR */     -1,
  /* BIN_FOREGROUND */ 0,
  /* BIN_BACKGROUND */ 255,
  /* BIN_OK */         0,
  /* WR1_BIAS_CROSSOVER */ 93,
  /* WR1_BLACK_BIAS */ -40,
  /* WR1_WHITE_BIAS */ 40,
  /* WR1_BIAS */       20,
  /* WR1_BLACK_BIAS_FACTOR */ 0.0,
  /* WR1_WHITE_BIAS_FACTOR */ -0.25,
  /* wr1_f_tab */ {
    -62,  -62,  -61,  -61,  -60,  -60,  -59,  -59,
    -58,  -58,  -57,  -57,  -56,  -56,  -54,  -54,
    -53,  -53,  -52,  -52,  -51,  -51,  -50,  -50,
    -49,  -49,  -48,  -48,  -47,  -47,  -46,  -46,
    -45,  -45,  -44,  -44,  -43,  -43,  -42,  -42,
    -41,  -41,  -41,  -41,  -40,  -40,  -39,  -39,
    -38,  -38,  -37,  -37,  -36,  -36,  -36,  -36,
    -35,  -35,  -34,  -34,  -33,  -33,  -33,  -33,
    -32,  -32,  -31,  -31,  -31,  -31,  -30,  -30,
    -29,  -29,  -29,  -29,  -28,  -28,  -27,  -27,
    -27,  -27,  -26,  -26,  -25,  -25,  -25,  -25,
    -24,  -24,  -24,  -24,  -23,  -23,  -23,  -23,
    -22,  -22,  -22,  -22,  -21,  -21,  -21,  -21,
    -20,  -20,  -20,  -20,  -19,  -19,  -19,  -19,
    -18,  -18,  -18,  -18,  -17,  -17,  -17,  -17,
    -16,  -16,  -16,  -16,  -16,  -16,  -15,  -15,
    -15,  -15,  -14,  -14,  -14,  -14,  -14,  -14,
    -13,  -13,  -13,  -13,  -13,  -13,  -12,  -12,
    -12,  -12,  -12,  -12,  -11,  -11,  -11,  -11,
    -11,  -11,  -10,  -10,  -10,  -10,  -10,  -10,
    -9,   -9,   -9,   -9,   -9,   -9,   -8,   -8,
    -8,   -8,   -8,   -8,   -8,   -8,   -7,   -7,
    -7,   -7,   -7,   -7,   -7,   -7,   -6,   -6,
    -6,   -6,   -6,   -6,   -6,   -6,   -5,   -5,
    -5,   -5,   -5,   -5,   -5,   -5,   -4,   -4,
    -3,   -3,   -2,   -2,   -2,   -2,   -2,   -2,
    -2,   -2,   -2,   -2,   -1,   -1,   -1,   -1,
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1,   -1,   -1,   -1,    0,    0,
    1,    1,    2,    2,    2,    2,    2,    2,
    2,    2,    2,    2,    2,    2,    2,    2,
    2,    2,    2,    2,    2,    2,    2,    2,
    2,    2,    2,    2,    2,    2,    2,    2,
    2,    2,    2,    2,    2,    2,    2,    2,
    2,    2,    2,    2,    2,    2,    2,    2,
    2,    2,    2,    2,    2,    2,    2,    2,
    3,    3,    3,    3,    3,    3,    3,    3,
    3,    3,    3,    3,    3,    3,    3,    3,
    3,    3,    3,    3,    3,    3,    4,    4,
    4,    4,    4,    4,    4,    4,    4,    4,
    4,    4,    4,    4,    4,    4,    4,    4,
    4,    4,    4,    4,    4,    4,    4,    4,
    4,    4,    5,    5,    5,    5,    5,    5,
    5,    5,    5,    5,    5,    5,    5,    5,
    5,    5,    6,    6,    6,    6,    6,    6,
    6,    6,    6,    6,    6,    6,    6,    6,
    6,    6,    6,    6,    6,    6,    6,    6,
    6,    6,    6,    6,    6,    6,    6,    6,
    6,    6,    7,    7,    7,    7,    7,    7,
    7,    7,    7,    7,    7,    7,    7,    7,
    7,    7,    7,    7,    7,    7,    7,    7,
    7,    7,    7,    7,    8,    8,    8,    8,
    8,    8,    8,    8,    8,    8,    8,    8,
    8,    8,    8,    8,    8,    8,    8,    8,
    8,    8,    9,    9,    9,    9,    9,    9,
    9,    9,    9,    9,    9,    9,    9,    9,
    9,    9,    9,    9,    9,    9,    9,    9,
    9,    9,    9,    9,    9,    9,    9,    9,
    9,    9,   10,   10,   10,   10,   10,   10,
    10,   10,   10,   10,   10,   10,   10,   10,
    10,   10,   10,   10,   10,   10,   10,    0
  },
  /* wr1_g_tab */ {
    -126, -126, -125, -125, -124, -124, -123, -123,
    -122, -122, -121, -121, -120, -120, -119, -119,
    -118, -118, -117, -117, -116, -116, -115, -115,
    -114, -114, -113, -113, -112, -112, -111, -111,
    -110, -110, -109, -109, -108, -108, -107, -107,
    -106, -106, -105, -105, -104, -104, -103, -103,
    -102, -102, -101, -101, -100, -100,  -99,  -99,
    -98,  -98,  -97,  -97,  -96,  -96,  -95,  -95,
    -94,  -94,  -93,  -93,  -92,  -92,  -91,  -91,
    -90,  -90,  -89,  -89,  -88,  -88,  -87,  -87,
    -86,  -86,  -85,  -85,  -84,  -84,  -83,  -83,
    -82,  -82,  -81,  -81,  -80,  -80,  -79,  -79,
    -78,  -78,  -77,  -77,  -76,  -76,  -75,  -75,
    -74,  -74,  -73,  -73,  -72,  -72,  -71,  -71,
    -70,  -70,  -69,  -69,  -68,  -68,  -67,  -67,
    -66,  -66,  -65,  -65,  -64,  -64,  -63,  -63,
    -61,  -61,  -59,  -59,  -57,  -57,  -54,  -54,
    -52,  -52,  -50,  -50,  -48,  -48,  -46,  -46,
    -44,  -44,  -42,  -42,  -41,  -41,  -39,  -39,
    -37,  -37,  -36,  -36,  -34,  -34,  -33,  -33,
    -31,  -31,  -30,  -30,  -29,  -29,  -27,  -27,
    -26,  -26,  -25,  -25,  -24,  -24,  -23,  -23,
    -22,  -22,  -21,  -21,  -20,  -20,  -19,  -19,
    -18,  -18,  -17,  -17,  -16,  -16,  -15,  -15,
    -14,  -14,  -14,  -14,  -13,  -13,  -12,  -12,
    -12,  -12,  -11,  -11,  -10,  -10,  -10,  -10,
    -9,   -9,   -8,   -8,   -8,   -8,   -7,   -7,
    -7,   -7,   -6,   -6,   -6,   -6,   -5,   -5,
    -5,   -5,   -4,   -4,   -2,   -2,   -2,   -2,
    -2,   -2,   -1,   -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    -1,   -1,   -1,   -1,   -1,   -1,    0,    0,
    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    2,    2,    2,    2,    2,    2,
    4,    4,    5,    5,    5,    5,    6,    6,
    6,    6,    7,    7,    7,    7,    8,    8,
    8,    8,    9,    9,   10,   10,   10,   10,
    11,   11,   12,   12,   12,   12,   13,   13,
    14,   14,   14,   14,   15,   15,   16,   16,
    17,   17,   18,   18,   19,   19,   20,   20,
    21,   21,   22,   22,   23,   23,   24,   24,
    25,   25,   26,   26,   27,   27,   29,   29,
    30,   30,   31,   31,   33,   33,   34,   34,
    36,   36,   37,   37,   39,   39,   41,   41,
    42,   42,   44,   44,   46,   46,   48,   48,
    50,   50,   52,   52,   54,   54,   57,   57,
    59,   59,   61,   61,   63,   63,   64,   64,
    65,   65,   66,   66,   67,   67,   68,   68,
    69,   69,   70,   70,   71,   71,   72,   72,
    73,   73,   74,   74,   75,   75,   76,   76,
    77,   77,   78,   78,   79,   79,   80,   80,
    81,   81,   82,   82,   83,   83,   84,   84,
    85,   85,   86,   86,   87,   87,   88,   88,
    89,   89,   90,   90,   91,   91,   92,   92,
    93,   93,   94,   94,   95,   95,   96,   96,
    97,   97,   98,   98,   99,   99,  100,  100,
    101,  101,  102,  102,  103,  103,  104,  104,
    105,  105,  106,  106,  107,  107,  108,  108,
    109,  109,  110,  110,  111,  111,  112,  112,
    113,  113,  114,  114,  115,  115,  116,  116,
    117,  117,  118,  118,  119,  119,  120,  120,
    121,  121,  122,  122,  123,  123,  124,  124,
    125,  125,  126,  126,  127,  127,  127,    0
  }
};

inline int wr1_bias (int x, int offset)
{
   int result;
   int bias;
   
   x = 256 - x;

   bias = -offset;
   
   if (x < wr1_params.WR1_BIAS_CROSSOVER)
   {
      result = x - bias
	 - (int)(wr1_params.WR1_BLACK_BIAS_FACTOR*(wr1_params.WR1_BIAS_CROSSOVER-x));
   }
   else if (x >= wr1_params.WR1_BIAS_CROSSOVER)
   {
      result = x + bias
	 + (int)(wr1_params.WR1_WHITE_BIAS_FACTOR*(x-wr1_params.WR1_BIAS_CROSSOVER));
   }
   else
      result = x;

/*
   result = x-bias;
*/
   if (result < wr1_params.BIN_FOREGROUND)
      result = wr1_params.BIN_FOREGROUND;
   if (result > wr1_params.BIN_BACKGROUND)
      result = wr1_params.BIN_BACKGROUND;
   
   return  256 - result; 
}


inline int wr1_f (int diff, int *f)
{
  /* if (abs(diff)>wr1_params.WR1_F_OFFSET)
   {
      Warning(2, "wr1_f: Error: diff = %i\n", diff);
      return wr1_params.BIN_ERROR;
      }*/
   f[0] = -wr1_params.wr1_f_tab[wr1_params.WR1_F_OFFSET - diff];
   return wr1_params.BIN_OK;
}

inline int wr1_g (int diff, int *g)
{
  /*   if (abs(diff)>wr1_params.WR1_G_OFFSET)
   {
      Warning(2, "wr1_g: Error: diff = %i\n", diff);   
      return wr1_params.BIN_ERROR;
      }*/
   g[0] = -wr1_params.wr1_g_tab[wr1_params.WR1_G_OFFSET - diff];
   return wr1_params.BIN_OK;
}


/*
 * OneBit white_rohrer_threshold(GreyScale src, 
 *                          int x_lookahead,
 *                          int y_lookahead, 
 *                          int bias_mode,
 *                          int bias_factor,
 *                          int f_factor
 *                          int g_factor);
 */

template<class T>
OneBitImageView* white_rohrer_threshold (const T& in, int x_lookahead, int y_lookahead,
	     int bias_mode, int bias_factor, int f_factor, int g_factor)
{
  int xsize, ysize;
  int x, y;
  int u;
  int prevY;
  int Y = 0;
  int f, g;
  int x_ahead, y_ahead;
  int t;
  int offset = wr1_params.WR1_BIAS;
  //double mu, s_dev;
  FloatPixel mu = 0.0;
  FloatPixel  s_dev = 0.0; 
  int *Z;
  int n;

  typedef ImageFactory<OneBitImageView>::data_type data_type;
  typedef ImageFactory<OneBitImageView>::view_type view_type;
  data_type* bin_data = new data_type(in.size(), in.origin());
  view_type* bin_view = new view_type(*bin_data);  
  

  xsize = in.ncols();
  ysize = in.nrows();
  //  std::cout<<"sizes are "<<ysize<<","<<xsize<<std::endl;
  x_lookahead = x_lookahead % xsize;

  if (bias_mode == 0) 
  {
    mu = image_mean(in);
    s_dev = sqrt(image_variance(in));
    offset = (int)(s_dev - 40) ;
  } 
  else 
    offset = bias_mode;

  Z = new int[2*xsize+1];
  for(n = 0; n< 2*xsize+1; ++n)
    Z[n] = 0;
   
  //Z[1] = prevY = (int)mu;
  Z[0] = prevY = (int)mu;

   for (y=0; y< 1+y_lookahead; y++)
   {
      if (y < y_lookahead)
	 t = xsize;
      else
	 t = x_lookahead;
      for (x=0; x< t; x++)
      {
	 u = in.get(Point(x,y));
	 wr1_f (u-prevY, &f);
	 Y = prevY + f;
	 if (y == 1)
	    Z[x] = (int)mu;
	 else
	 {
	    wr1_g(Y-Z[x], &g);
	    Z[x] = Z[x] + g; 
	 }
      }
      
   }
   x_ahead = 1 + x_lookahead;
   y_ahead = 1 + y_lookahead;
 
   for (y = 0; y < ysize; y++)
   {
      for (x = 0; x < xsize; x++)
      {
	 if (in.get(Point(x,y)) < (bias_factor  
			     * wr1_bias(Z[x_ahead],offset) / 100))
	 {
	    bin_view->set(Point(x,y),black(*bin_view));
	 }
	 
	 else	
	 {
	    bin_view->set(Point(x,y),white(*bin_view));
	 }

	 x_ahead++;
	 if (x_ahead > xsize)
	 {
	    x_ahead = 1;
	    y_ahead++;
	 }
	 if (y_ahead <= ysize)
	 {
	    prevY = Y;
	    wr1_f(in.get(Point(x_ahead,y_ahead))-prevY, &f);    
	    Y = prevY + f_factor * f / 100;
	    wr1_g(Y-Z[x_ahead], &g);
	    Z[x_ahead] = Z[x_ahead] + g_factor * g / 100;
	 }
	 else
	    Z[x_ahead] = Z[x_ahead-1];
      }
   }
 
 delete [] Z;
 Z = NULL;
  
 return bin_view;

}

/*
 *  FloatVector histogram_real_values(GreyScale|Grey16 image);
 *
 *  Returns a histogram of the values in an image. 
 */
template<class T>
FloatVector* histogram_real_values(const T& image) {
    // The histogram is the size of all of the possible values of
    // the pixel type.
    size_t l = std::numeric_limits<typename T::value_type>::max() + 1;
    FloatVector* values = new FloatVector(l);

    // set the list to 0
    std::fill(values->begin(), values->end(), 0);

    typename T::const_row_iterator row = image.row_begin();
    typename T::const_col_iterator col;
    ImageAccessor<typename T::value_type> acc;

    // create the histogram
    for (; row != image.row_end(); ++row)
      for (col = row.begin(); col != row.end(); ++col)
    (*values)[acc.get(col)]++;

    return values;
}

/*
 *  Image* brink_threshold(const T& image);
 *
 *  Calculates threshold for image with Brink and Pendock's minimum-cross    
 *  entropy method and returns corrected image.
 *
 *  References: Brink, A., and Pendock, N. 1996. Minimum cross-entropy
 *  threshold selection. Pattern Recognition 29: 179-188. 
 *
 */
template<class T>
Image* brink_threshold(const T& image)
{
  int i,j;  // for iteration

  static const size_t VEC_DBL_SZ = sizeof(double) * 256; //size of vector
  unsigned long vecSum = 0;     // sum of histogram
  double invHistSum;            // inverse of histogram sum
  int Topt = 0;                 // threshold value
  double locMin;                // local minimum
  int isMinInit = 0;            // flat for minimum initialization

  FloatVector *histoFV;
  histoFV = histogram_real_values(image);   // compute gray histogram 
  unsigned long histo[256];
  for (i = 0; i < 256; i++)
    histo[i] = (*histoFV)[i];   // copy from FloatVector to an array of longs

  double pmf[256];          // pmf (i.e. normalized histogram)
  double m_f[256];          // first foreground moment
  double m_b[256];          // first background moment

  double tmpVec1[256];      // temporary vector 1
  double tmpVec2[256];      // temporary vector 2
  double tmpVec3[256];      // temporary vector 3

  double tmp1[256][256];    // temporary matrix 1
  double tmp2[256][256];    // temporary matrix 2 
  double tmp3[256][256];    // temporary matrix 3
  double tmp4[256][256];    // temporary matrix 4

  double tmpMat1[256][256];  // local temporary matrix 1
  double tmpMat2[256][256];  // local temporary matrix 2
  
  // compute sum of the histogram
  for (i = 0; i < 256; ++i)
        vecSum += histo[i];
        
  // compute inverse of vecSum
  invHistSum = 1.0 / vecSum;    

  // compute normalized histogram (pmf)  
  for (i = 0; i < 256; i++)
    pmf[i] = histo[i] * invHistSum;     // equivalent to dividing by the sum, but faster!
    
  // compute foreground moment
  m_f[0] = 0.0;
  for (i = 1; i < 256; ++i)         
    m_f[i] = i * pmf[i] + m_f[i - 1];

  // compute background moment
  memcpy(m_b, m_f, VEC_DBL_SZ);      

  for (i = 0; i < 256; ++i)
    m_b[i] = m_f[255] - m_b[i];     
    
  // compute brink entropy binarization
  for (i = 0; i < 256; ++i)     
  {
    for (j = 0; j < 256; ++j)
    {
      tmp1[i][j] = m_f[j] / i;

      if ((m_f[j] == 0) || (i == 0)) 
      {
        tmp2[i][j] = 0.0;               
        tmp3[i][j] = 0.0;
      }
      else
      {
        tmp2[i][j] = log(tmp1[i][j]);
        tmp3[i][j] = log(1.0 / tmp1[i][j]);;
      }
      tmp4[i][j] = pmf[i] * (m_f[j] * tmp2[i][j] + i * tmp3[i][j]);
    }
  }

  // compute the diagonal of the cumulative sum of tmp4 and store result in tmpVec1
  memcpy(tmpMat1[0], tmp4[0], VEC_DBL_SZ);   // copies first row of tmp4 to the first row of tmpMat1
  for (i = 1; i < 256; ++i)         // get cumulative sum
    for (j = 0; j < 256; ++j)
      tmpMat1[i][j] = tmpMat1[i-1][j] + tmp4[i][j];
    for (i = 0; i < 256; ++i)       // set to diagonal
      tmpVec1[i] = tmpMat1[i][i];   // tmpVec1 is now the diagonal of the cumulative sum of tmp4
 

  // same operation but for background moment, NOTE: tmp1 through tmp4 get overwritten
  for (i = 0; i < 256; ++i)     
  {
    for (j = 0; j < 256; ++j)
    {
      tmp1[i][j] = m_b[j] / i;          // tmpb0 = m_b_rep ./ g_rep;
      if ((m_b[j] == 0) || (i == 0)) 
      {
        tmp2[i][j] = 0.0;               // replace inf or NaN values with 0
        tmp3[i][j] = 0.0;
            }
            else
            {  
                tmp2[i][j] = log(tmp1[i][j]);
                tmp3[i][j] = log(1.0 / tmp1[i][j]);;
            }
            tmp4[i][j] = pmf[i] * (m_b[j] * tmp2[i][j] + i * tmp3[i][j]);
        }
    }

    // sum columns, subtract diagonal of cumulative sum of tmp4 
    memcpy(tmpVec2, tmp4[0], VEC_DBL_SZ);   // copies first row of tmp4 to the first row of tmpMat2 
    for (i = 1; i < 256; ++i)           
        for (j = 0; j < 256; ++j)
            tmpVec2[j] += tmp4[i][j];   // sums of columns of tmp4 and store result in tmpVec2

    // compute the diagonal of the cumulative sum of tmp4 and store result in tmpVec1
    memcpy(tmpMat2[0], tmp4[0], VEC_DBL_SZ);    // copies first row of tmp4 to the first row of tmpMat2 
    for (i = 1; i < 256; ++i)       // get cumulative sum
        for (j = 0; j < 256; ++j)
          tmpMat2[i][j] = tmpMat2[i-1][j] + tmp4[i][j]; 
    for (i = 0; i < 256; ++i)       // set to diagonal
        tmpVec3[i] = tmpMat2[i][i]; // tmpVec3 is now the diagonal of the cumulative sum of tmpMat2

    for (i = 0; i < 256; ++i)       
        tmpVec2[i] -= tmpVec3[i];
    for (int i = 0; i < 256; ++i)
        tmpVec1[i] += tmpVec2[i];

    // calculate the threshold value
    for (i = 0; i < 256; ++i)
    {
        if (m_f[i] != 0 && m_b[i] != 0)
        {
            if (!isMinInit || tmpVec1[i] < locMin)
            {
                isMinInit = 1;
                locMin = tmpVec1[i];    // gets a new minimum
                Topt = i;
            }
        }
    }
        
    Topt++; // DO I NEED TO ADD ONE
    
    // call threshold from threshold.hpp with image and Topt and return the output
    return threshold(image, Topt, 0);   
}

#endif

