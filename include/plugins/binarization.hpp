/*
 *
 * Copyright (C) 2005 John Ashley Burgoyne and Ichiro Fujinaga
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef jab18112005_binarization
#define jab18112005_binarization

#include "gamera.hpp"
#include "math.h"
#include <numeric>
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


#endif

