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

#ifndef mgd_arithmetic
#define mgd_arithmetic

#include <functional>
#include "gamera.hpp"

template<class T, class U, class FUNCTOR>
inline 
typename ImageFactory<T>::view_type* 
arithmetic_combine(T& a, const U& b, const FUNCTOR& functor, bool in_place) {
  if (a.nrows() != b.nrows() || a.ncols() != b.ncols())
    throw std::runtime_error("Images must be the same size.");
  
  typedef typename T::value_type TVALUE;
  typedef typename ImageFactory<T>::view_type VIEW;

  if (in_place) {
    typename T::vec_iterator ia = a.vec_begin();
    typename U::const_vec_iterator ib = b.vec_begin();
    typename choose_accessor<T>::accessor ad = choose_accessor<T>::make_accessor(a);
    for (; ia != a.vec_end(); ++ia, ++ib) {
      ad.set(NumericTraits<TVALUE>::fromPromote
	     (functor(typename NumericTraits<TVALUE>::Promote(*ia),
		      typename NumericTraits<TVALUE>::Promote(*ib))),
	     ia);
    }

    // Returning NULL is converted to None by the wrapper mechanism
    return NULL;
  } else {
    typename ImageFactory<T>::data_type* dest_data =
      new typename ImageFactory<T>::data_type(a.size(), a.origin());
    VIEW* dest = new VIEW(*dest_data, a);
    typename T::vec_iterator ia = a.vec_begin();
    typename U::const_vec_iterator ib = b.vec_begin();
    typename VIEW::vec_iterator id = dest->vec_begin();
    typename choose_accessor<VIEW>::accessor ad = choose_accessor<VIEW>::make_accessor(*dest);

    // Vigra's combineTwoImages does not clip back to one of the standard
    // Gamera image types, so we have to do this differently ourselves.  MGD
    
    try {
      for (; ia != a.vec_end(); ++ia, ++ib, ++id) {
	ad.set(NumericTraits<TVALUE>::fromPromote
	       (functor(typename NumericTraits<TVALUE>::Promote(*ia),
			typename NumericTraits<TVALUE>::Promote(*ib))),
	       id);
      }
    } catch (std::exception e) {
      delete dest;
      delete dest_data;
      throw;
    }
    return dest;
  }
}

template<class T, class U>
typename ImageFactory<T>::view_type* 
add_images(T& a, const U& b, bool in_place=true) {
  typedef typename T::value_type TVALUE;
  typedef typename NumericTraits<TVALUE>::Promote PROMOTE;
  return arithmetic_combine(a, b, std::plus<PROMOTE>(), in_place);
}

template <class T>
struct my_minus : public std::binary_function<T,T,T>
{
  T operator()(const T& x, const T& y) const { 
    typedef typename NumericTraits<T>::Promote PROMOTE;
    return std::minus<T>()(x, y); 
  }
};

template<>
struct my_minus<OneBitPixel> : public std::binary_function<OneBitPixel, OneBitPixel, OneBitPixel>
{
  OneBitPixel operator()(const OneBitPixel& x, const OneBitPixel& y) const {
    // Note the result is inverted, because the default accessor performs an invert.
    // GAAAH!
    if (is_black(x) && !is_black(y))
      return pixel_traits<OneBitPixel>::white();
    else
      return pixel_traits<OneBitPixel>::black();
  }
};

template<class T, class U>
typename ImageFactory<T>::view_type* 
subtract_images(T& a, const U& b, bool in_place=true) {
  typedef typename T::value_type TVALUE;
  return arithmetic_combine(a, b, my_minus<TVALUE>(), in_place);
}

template<class T, class U>
typename ImageFactory<T>::view_type* 
multiply_images(T& a, const U& b, bool in_place=true) {
  typedef typename T::value_type TVALUE;
  typedef typename NumericTraits<TVALUE>::Promote PROMOTE;
  return arithmetic_combine(a, b, std::multiplies<PROMOTE>(), in_place);
}

template<class T, class U>
typename ImageFactory<T>::view_type* 
divide_images(T& a, const U& b, bool in_place=true) {
  typedef typename T::value_type TVALUE;
  typedef typename NumericTraits<TVALUE>::Promote PROMOTE;
  return arithmetic_combine(a, b, std::divides<PROMOTE>(), in_place);
}

#endif 
