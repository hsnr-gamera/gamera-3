/*
 *
 * Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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

#ifndef kwm10092002_logical
#define kwm10092002_logical

#include "gamera.hpp"
#include <functional>
#include <exception>

namespace Gamera {

template<class T, class U, class FUNCTOR>
inline typename ImageFactory<T>::view_type* 
logical_combine(T& a, const U& b, const FUNCTOR& functor, bool in_place) {
  if (a.nrows() != b.nrows() || a.ncols() != b.ncols())
    throw std::runtime_error("Images must be the same size.");
  
  typedef typename T::value_type TVALUE;
  typedef typename ImageFactory<T>::view_type VIEW;

  if (in_place) {
    typename T::vec_iterator ia = a.vec_begin();
    typename U::const_vec_iterator ib = b.vec_begin();
    typename choose_accessor<T>::accessor ad = choose_accessor<T>::make_accessor(a);
    for (; ia != a.vec_end(); ++ia, ++ib) {
      bool b = functor(is_black(*ia), is_black(*ib));
      if (b)
	ad.set(white(a), ia);
      else
	ad.set(black(a), ia);
    }

    // Returning NULL is converted to None by the wrapper mechanism
    return NULL;
  } else {
    typename ImageFactory<T>::data_type* dest_data =
      new typename ImageFactory<T>::data_type(a.size(), a.origin());
    VIEW* dest = new VIEW(*dest_data);
    typename T::vec_iterator ia = a.vec_begin();
    typename U::const_vec_iterator ib = b.vec_begin();
    typename VIEW::vec_iterator id = dest->vec_begin();
    typename choose_accessor<VIEW>::accessor ad = choose_accessor<VIEW>::make_accessor(*dest);
    
    try {

    // Vigra's combineTwoImages does not clip back to one of the standard
    // Gamera image types, so we have to do this differently ourselves.  MGD

      for (; ia != a.vec_end(); ++ia, ++ib, ++id) {
	bool b = functor(is_black(*ia), is_black(*ib));
	if (b)
	  ad.set(white(a), id);
	else
	  ad.set(black(a), id);
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
and_image(T& a, const U& b, bool in_place=true) {
  typedef typename T::value_type value_type;
  return logical_combine(a, b, std::logical_and<bool>(), in_place);
}

template<class T, class U>
typename ImageFactory<T>::view_type* 
or_image(T& a, const U& b, bool in_place=true) {
  typedef typename T::value_type value_type;
  return logical_combine(a, b, std::logical_or<bool>(), in_place);
};

// We make our own, since logical_xor is not in STL
template <class _Tp>
struct logical_xor : public std::binary_function<_Tp,_Tp,bool>
{
  bool operator()(const _Tp& __x, const _Tp& __y) const { return __x ^ __y; }
};

template<class T, class U>
typename ImageFactory<T>::view_type* 
xor_image(T& a, const U& b, bool in_place=true) {
  typedef typename T::value_type value_type;
  return logical_combine(a, b, logical_xor<bool>(), in_place);
}

}
#endif
