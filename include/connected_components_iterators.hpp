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

#ifndef kwm01022002_connected_component_iterators_hpp
#define kwm01022002_connected_component_iterators_hpp

#include "accessor.hpp"
#include "iterator_base.hpp"

namespace Gamera {
  namespace CCDetail {

    /*
      CCProxy

      This class is used so that the filtered assignment and derefencing can
      happen for the ColIterator and RowIterator (which require that the values returned
      from operator *, ->, and [] return an lvalue). So far it has worked well, but
      more testing probably needs to be done.

      The basic idea is that instead of returning a reference to a value in the image_data, we
      return an object that holds a reference to the data and the label for the ConnectedComponent.
      The object contains a conversion operator and an assignment operator. When conversion to the
      type of the value is requested, the label is checked against the value. If they match, then the
      value is returned, otherwise 0 is returned. Assignment makes similar checks. This type of
      proxying can cause numerous problems, so care is required. KWM
    */

    template<class T, class I>
    class CCProxy {
    public:
      /*
	constructor - we store a pointer to the value so that
	changing the ConnectedComponent from another place (say
	the set function of ConnectedComponent) doesn't invalidate
	this proxy object. This is probably not a big concern. KWM
      */
      CCProxy(I i, T label) : m_iter(i), m_label(label) { }
      // conversion to T
      operator T() const {
	T tmp = m_accessor(m_iter);
	if (tmp == m_label)
	  return tmp;
	else
	  return 0;
      }
      // assignment only happens if the label matches
      void operator=(T value) {
	if (m_accessor(m_iter) == m_label)
	  m_accessor.set(value, m_iter);
      }
    private:
      I m_iter;
      T m_label;
      ImageAccessor<T> m_accessor;
    };
    
    template<class Image, class T> class ColIterator;

    template<class Image, class T>
    class RowIterator : public RowIteratorBase<Image, RowIterator<Image, T>, T> {
    public:
      // Typedefs for rows
      typedef ColIterator<Image, T> iterator;
      typedef typename Image::value_type value_type;
      typedef ImageAccessor<value_type> accessor;

      typedef RowIterator self;
      typedef RowIteratorBase<Image, self, T> base;
      typedef CCProxy<value_type, T> proxy_type;

      // Constructor
      RowIterator(Image* image, const T iterator) : base(image, iterator) { }
      RowIterator() { }

      proxy_type operator*() const {
	return proxy_type(m_iterator, m_image->label());
      }

      value_type get() const {
	if (m_accessor(m_iterator) == m_image->label())
	  return m_accessor(m_iterator);
	else
	  return 0;
      }

      void set(const value_type& v) {
	if (m_accessor(m_iterator) == m_image->label())
	  m_accessor.set(v, m_iterator);
      }

      iterator begin() const {
	return iterator(m_image, m_iterator);
      }

      iterator end() const {
	return iterator(m_image, m_iterator + m_image->ncols());
      }
    private:
      accessor m_accessor;
    };

    template<class Image, class T>
    class ColIterator : public ColIteratorBase<Image, ColIterator<Image, T>, T> {
    public:
      // Typedefs for Cols
      typedef RowIterator<Image, T> iterator;
      typedef typename Image::value_type value_type;
      typedef ImageAccessor<value_type> accessor;

      // Convenience typedefs
      typedef ColIterator self;
      typedef ColIteratorBase<Image, self, T> base;
      typedef CCProxy<value_type, T> proxy_type;

      // Constructor
      ColIterator(Image* image, const T iterator) : base(image, iterator) { }
      ColIterator() { }

      proxy_type operator*() const {
	return proxy_type(m_iterator, m_image->label());
      }      

      // Image specific
      value_type get() const {
	if (m_accessor(m_iterator) == m_image->label())
	  return m_accessor(m_iterator);
	else
	  return 0;
      }
      
      void set(const value_type& v) {
	if (m_accessor(m_iterator) == m_image->label())
	  m_accessor.set(v, m_iterator);
      }

      iterator begin() const {
	return iterator(m_image, m_iterator);
      }
      iterator end() const {
	return iterator(m_image, m_iterator) + m_image->nrows();
      }
    private:
      accessor m_accessor;
    };

    template<class Image, class T> class ConstColIterator;

    template<class Image, class T>
    class ConstRowIterator : public RowIteratorBase<Image, ConstRowIterator<Image, T>, T> {
    public:
      // Typedefs for rows
      typedef ConstColIterator<Image, T> iterator;

      // Convenience typedefs
      typedef ConstRowIterator self;
      typedef RowIteratorBase<Image, self, T> base;
      typedef typename Image::value_type value_type;
      typedef ImageAccessor<value_type> accessor;

      // Constructor
      ConstRowIterator(Image* image, const T iterator) : base(image, iterator) { }
      ConstRowIterator() { }

      typename Image::value_type operator*() const {
	return get();
      }

      value_type get() const {
	if (m_accessor(m_iterator) == m_image->label())
	  return m_accessor(m_iterator);
	else
	  return 0;
      }

      iterator begin() const {
	return iterator(m_image, m_iterator);
      }

      iterator end() const {
	return iterator(m_image, m_iterator + m_image->ncols());
      }
    private:
      accessor m_accessor;
    };

    template<class Image, class T>
    class ConstColIterator : public ColIteratorBase<Image, ConstColIterator<Image, T>, T> {
    public:
      // Typedefs for Cols
      typedef RowIterator<Image, T> iterator;

      // Convenience typedefs
      typedef ConstColIterator self;
      typedef ColIteratorBase<Image, self, T> base;
      typedef typename Image::value_type value_type;
      typedef ImageAccessor<value_type> accessor;

      // Constructor
      ConstColIterator(Image* image, const T iterator) : base(image, iterator) { }
      ConstColIterator() { }

      typename Image::value_type operator*() const {
	return get();
      }

      // Image specific
      value_type get() const {
	if (m_accessor(m_iterator) == m_image->label())
	  return m_accessor(m_iterator);
	else
	  return 0;
      }

      iterator begin() const {
	return iterator(m_image, m_iterator);
      }
      iterator end() const {
	return iterator(m_image, m_iterator) + m_image->nrows();
      }
    private:
      accessor m_accessor;
    };


    template<class Image, class Row, class Col>
    class VecIterator : public VecIteratorBase<Image, Row, Col,
					       VecIterator<Image, Row, Col> > {
    public:
      typedef VecIterator self;
      typedef VecIteratorBase<Image, Row, Col, self> base;
      typedef typename Image::value_type value_type;
      typedef CCProxy<value_type, typename Image::data_type::iterator> proxy_type;
      typedef ImageAccessor<value_type> accessor;

      // Constructor
      VecIterator(const Row iterator) : base(iterator) { }
      VecIterator() { }

      // Operators
      proxy_type operator*() const {
	return proxy_type(m_coliterator.m_iterator, m_coliterator.m_image->label());
      }

      value_type get() const {
	if (m_accessor(m_coliterator) == m_coliterator.m_image->label())
	  return m_accessor(m_coliterator);
	else
	  return 0;
      }
      
      void set(const value_type& v) {
	if (m_accessor(m_coliterator) == m_coliterator.m_image->label())
	  m_accessor.set(v, m_coliterator);
      }
    private:
      accessor m_accessor;
    };

    template<class Image, class Row, class Col>
    class ConstVecIterator : public VecIteratorBase<Image, Row, Col,
						    ConstVecIterator<Image, Row, Col> > {
    public:
      typedef ConstVecIterator self;
      typedef VecIteratorBase<Image, Row, Col, self> base;
      typedef typename Image::value_type value_type;
      typedef ImageAccessor<value_type> accessor;

      // Constructor
      ConstVecIterator(const Row iterator) : base(iterator) { }
      ConstVecIterator() { }

      // Operators
      typename Image::value_type operator*() const {
	return get();
      }

      value_type get() const {
	if (m_accessor(m_coliterator) == m_coliterator.m_image->label())
	  return m_accessor(m_coliterator);
	else
	  return 0;
      }
    private:
      accessor m_accessor;
    };

  } // namespace
} // namespace
#endif
