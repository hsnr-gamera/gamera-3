#ifndef lcninja_edgedetect
#define lcninja_edgedetect

#include "gamera.hpp"
#include "vigra/edgedetection.hxx"

using namespace Gamera;

template<class T>
typename ImageFactory<T>::view_type* difference_of_exponential_edge_image(const T& src, double scale, double gradient_threshold, unsigned int min_edge_length) {
  if ((scale < 0) || (gradient_threshold < 0))
    throw std::runtime_error("The scale and gradient_threshold must be greater than 0");

  typename ImageFactory<T>::data_type* dest_data =
    new typename ImageFactory<T>::data_type(src.size(), src.offset_y(), src.offset_x());

  typename ImageFactory<T>::view_type* dest =
    new typename ImageFactory<T>::view_type(*dest_data, src);

  vigra::differenceOfExponentialEdgeImage(src_image_range(src), dest_image(*dest), scale, gradient_threshold);

  if (min_edge_length > 0)
    vigra::removeShortEdges(dest_image_range(*dest), min_edge_length, NumericTraits<typename T::value_type>::one());

  return dest;
}

template<class T>
typename ImageFactory<T>::view_type* difference_of_exponential_crack_edge_image(const T& src, double scale, double gradient_threshold, unsigned int min_edge_length, unsigned int close_gaps, unsigned int beautify) {
  if ((scale < 0) || (gradient_threshold < 0))
    throw std::runtime_error("The scale and gradient threshold must be greater than 0");

  typename ImageFactory<T>::data_type* dest_data =
    new typename ImageFactory<T>::data_type(src.nrows() * 2, src.ncols() *2, src.offset_y(), src.offset_x());

  typename ImageFactory<T>::view_type* dest =
    new typename ImageFactory<T>::view_type(*dest_data);

  vigra::differenceOfExponentialCrackEdgeImage(src_image_range(src), dest_image(*dest), scale, gradient_threshold, NumericTraits<typename T::value_type>::one());

  if (min_edge_length > 0)
    vigra::removeShortEdges(dest_image_range(*dest), min_edge_length, NumericTraits<typename T::value_type>::one());

  if (close_gaps)
    vigra::closeGapsInCrackEdgeImage(dest_image_range(*dest), NumericTraits<typename T::value_type>::one());
  
  if (beautify)
    vigra::beautifyCrackEdgeImage(dest_image_range(*dest), NumericTraits<typename T::value_type>::one(), NumericTraits<typename T::value_type>::zero());

  return dest;
}

template<class T>
typename ImageFactory<T>::view_type* canny_edge_image(const T& src, double scale, double gradient_threshold) {
  if ((scale < 0) || (gradient_threshold < 0))
    throw std::runtime_error("The scale and gradient threshold must be >= 0");

  typename ImageFactory<T>::data_type* dest_data =
    new typename ImageFactory<T>::data_type(src.size(), src.offset_y(), src.offset_x());

  typename ImageFactory<T>::view_type* dest =
    new typename ImageFactory<T>::view_type(*dest_data, src);

  vigra::cannyEdgeImage(src_image_range(src), dest_image(*dest), scale, gradient_threshold, NumericTraits<typename T::value_type>::one());
  return dest;
}

#endif
