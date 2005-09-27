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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef mgd_convolution
#define mgd_convolution

#include "gamera.hpp"
#include "vigra/stdconvolution.hxx"

using namespace Gamera;

template<class T, class U>
typename ImageFactory<T>::view_type* convolve(const T& src, const U& k, int border_mode) {
  if (k.nrows() > src.nrows() || k.ncols() > src.ncols())
    throw std::runtime_error("The image must be bigger than the kernel.");

  typename ImageFactory<T>::data_type* dest_data =
    new typename ImageFactory<T>::data_type(src.size(), src.ul());
  typename ImageFactory<T>::view_type* dest =
    new typename ImageFactory<T>::view_type(*dest_data);

  // I originally had the following two lines abstracted out in a function,
  // but that seemed to choke and crash gcc 3.3.2
  try {
    typename U::ConstIterator center = k.upperLeft() + Diff2D(k.center_x(), k.center_y());
    tuple5<
      typename U::ConstIterator,
      typename choose_accessor<U>::accessor,
      Diff2D, Diff2D, BorderTreatmentMode> kernel
      (center, choose_accessor<U>::make_accessor(k), 
       Diff2D(-k.center_x(), -k.center_y()),
       Diff2D(k.width() - k.center_x(), k.height() - k.center_y()),
       (BorderTreatmentMode)border_mode);
    
    vigra::convolveImage(src_image_range(src), dest_image(*dest), kernel); 
  } catch (std::exception e) {
    delete dest;
    delete dest_data;
    throw;
  }
  return dest;
}

template<class T, class U>
typename ImageFactory<T>::view_type* convolve_x(const T& src, const U& k, int border_mode) {
  if (k.nrows() > src.nrows() || k.ncols() > src.ncols())
    throw std::runtime_error("The image must be bigger than the kernel.");
  if (k.nrows() != 1)
    throw std::runtime_error("The 1D kernel must have only one row.");

  typename ImageFactory<T>::data_type* dest_data =
    new typename ImageFactory<T>::data_type(src.size(), src.origin());
  typename ImageFactory<T>::view_type* dest =
    new typename ImageFactory<T>::view_type(*dest_data);

  // I originally had the following two lines abstracted out in a function,
  // but that seemed to choke and crash gcc 3.3.2
  try {
    typename U::const_vec_iterator center = k.vec_begin() + k.center_x();
    tuple5<
      typename U::const_vec_iterator,
      typename choose_accessor<U>::accessor,
      int, int, BorderTreatmentMode> kernel
      (center, choose_accessor<U>::make_accessor(k), 
       -int(k.center_x()), int(k.width()) - int(k.center_x()) - 1,
       (BorderTreatmentMode)border_mode);
    
    vigra::separableConvolveX(src_image_range(src), dest_image(*dest), kernel); 
  } catch (std::exception e) {
    delete dest;
    delete dest_data;
    throw;
  }
  return dest;
}

template<class T, class U>
typename ImageFactory<T>::view_type* convolve_y(const T& src, const U& k, int border_mode) {
  if (k.nrows() > src.ncols() || k.ncols() > src.nrows())
    throw std::runtime_error("The image must be bigger than the kernel.");
  if (k.nrows() != 1)
    throw std::runtime_error("The 1D kernel must have only one row.");

  typename ImageFactory<T>::data_type* dest_data =
    new typename ImageFactory<T>::data_type(src.size(), src.origin());
  typename ImageFactory<T>::view_type* dest =
    new typename ImageFactory<T>::view_type(*dest_data);

  // I originally had the following two lines abstracted out in a function,
  // but that seemed to choke and crash gcc 3.3.2
  try {
    typename U::const_vec_iterator center = k.vec_begin() + k.center_x();
    tuple5<
      typename U::const_vec_iterator,
      typename choose_accessor<U>::accessor,
      int, int, BorderTreatmentMode> kernel
      (center, choose_accessor<U>::make_accessor(k), 
       -int(k.center_x()), int(k.width()) - int(k.center_x()) - 1,
       (BorderTreatmentMode)border_mode);
    
    vigra::separableConvolveY(src_image_range(src), dest_image(*dest), kernel); 
  } catch (std::exception e) {
    delete dest;
    delete dest_data;
    throw;
  }
  return dest;
}

FloatImageView* _copy_kernel(const Kernel1D<FloatPixel>& kernel) {
  FloatImageData* dest_data = new FloatImageData(Dim(kernel.size(), 1));
  FloatImageView* dest = new FloatImageView(*dest_data);
  try {
    FloatImageView::vec_iterator iout = dest->vec_begin();
    for (int iin = kernel.left(); iin != kernel.right(); ++iout, ++iin)
      *iout = kernel[iin];
  } catch (std::exception e) {
    delete dest;
    delete dest_data;
    throw;
  }
  return dest;
}

// The following functions generate various kernels useful for
// separable convolution.  It might be possible to avoid the copy
// by creating a new version of ImageData with push_back, or some
// way to set the ImageData m_data member, but in the absense of
// any such hack, this will do for now.  The kernels all tend to be
// quite small, so the copy shouldn't be too bad.

FloatImageView* GaussianKernel(double std_dev) {
  Kernel1D<FloatPixel> kernel;
  kernel.initGaussian(std_dev);
  return _copy_kernel(kernel);
}

FloatImageView* GaussianDerivativeKernel(double std_dev, int order) {
  Kernel1D<FloatPixel> kernel;
  kernel.initGaussianDerivative(std_dev, order);
  return _copy_kernel(kernel);
}

FloatImageView* BinomialKernel(int radius) {
  Kernel1D<FloatPixel> kernel;
  kernel.initBinomial(radius);
  return _copy_kernel(kernel);
}

FloatImageView* AveragingKernel(int radius) {
  Kernel1D<FloatPixel> kernel;
  kernel.initAveraging(radius);
  return _copy_kernel(kernel);
}

FloatImageView* SymmetricGradientKernel() {
  Kernel1D<FloatPixel> kernel;
  kernel.initSymmetricGradient();
  return _copy_kernel(kernel);
}

FloatImageView* SimpleSharpeningKernel(double sf) {
  FloatImageData* dest_data = new FloatImageData(Dim(3, 3));
  FloatImageView* dest = new FloatImageView(*dest_data);
  dest->set(Point(0, 0), -sf/16.0);
  dest->set(Point(1, 0), -sf/8.0);
  dest->set(Point(2, 0), -sf/16.0);
  dest->set(Point(0, 1), -sf/8.0);
  dest->set(Point(1, 1), 1.0+sf*0.75);
  dest->set(Point(2, 1), -sf/8.0);
  dest->set(Point(2, 0), -sf/16.0);
  dest->set(Point(2, 1), -sf/8.0);
  dest->set(Point(2, 2), -sf/16.0);
  return dest;
}

#endif
