/*
 *
 * Copyright (C) 2001-2004 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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

#ifndef kwm01022002_image_view_iterators_hpp
#define kwm01022002_image_view_iterators_hpp

#include "accessor.hpp"
#include "iterator_base.hpp"

namespace Gamera {
  namespace ImageViewDetail {

    template<class Image, class T> class ColIterator;

    template<class Image, class T>
    class RowIterator : public RowIteratorBase<Image, RowIterator<Image, T>, T> {
	public:
      using RowIteratorBase<Image, RowIterator<Image, T>, T>::m_iterator;
      using RowIteratorBase<Image, RowIterator<Image, T>, T>::m_image;

      // Typedefs for rows
      typedef ColIterator<Image, T> iterator;

      // Convenience typedefs
      typedef RowIterator self;
      typedef RowIteratorBase<Image, self, T> base;
      typedef typename base::value_type value_type;
      typedef ImageAccessor<value_type> accessor;

      // Constructor
      RowIterator(Image* image, const T iterator) : base(image, iterator) { }
      RowIterator() { }

      typename base::reference operator*() const {
	return *m_iterator;
      }

      typename base::pointer operator->() const {
	return &*m_iterator;
      }

      value_type get() const {
	return m_accessor(m_iterator);
      }

      void set(const value_type& v) {
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
      using ColIteratorBase<Image, ColIterator<Image, T>, T>::m_iterator;
      using ColIteratorBase<Image, ColIterator<Image, T>, T>::m_image;

      // Typedefs for Cols
      typedef RowIterator<Image, T> iterator;

      // Convenience typedefs
      typedef ColIterator self;
      typedef ColIteratorBase<Image, self, T> base;
      typedef typename base::value_type value_type;
      typedef ImageAccessor<value_type> accessor;

      // Constructor
      ColIterator(Image* image, const T iterator) : base(image, iterator) { }
      ColIterator() { }

      typename base::reference operator*() const {
	return *m_iterator;
      }
      typename base::pointer operator->() const {
	return &*m_iterator;
      }

      // Image specific
      value_type get() const {
	return m_accessor(m_iterator);
      }

      void set(const value_type& v) {
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
      using RowIteratorBase<Image, ConstRowIterator<Image, T>, T>::m_iterator;
      using RowIteratorBase<Image, ConstRowIterator<Image, T>, T>::m_image;

      // Typedefs for rows
      typedef ConstColIterator<Image, T> iterator;

      // Convenience typedefs
      typedef ConstRowIterator self;
      typedef RowIteratorBase<Image, self, T> base;
      typedef typename base::value_type value_type;
      typedef ImageAccessor<value_type> accessor;

      // Constructor
      ConstRowIterator(Image* image, const T iterator) : base(image, iterator) { }
      ConstRowIterator() { }

      typename Image::value_type operator*() const {
	return *m_iterator;
      }

      typename base::pointer operator->() const {
	return &*m_iterator;
      }

      value_type get() const {
	return m_accessor(m_iterator);
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
      using ColIteratorBase<Image, ConstColIterator<Image, T>, T>::m_iterator;
      using ColIteratorBase<Image, ConstColIterator<Image, T>, T>::m_image;

      // Typedefs for Cols
      typedef ConstRowIterator<Image, T> iterator;

      // Convenience typedefs
      typedef ConstColIterator self;
      typedef ColIteratorBase<Image, self, T> base;
      typedef typename base::value_type value_type;
      typedef ImageAccessor<value_type> accessor;

      // Constructor
      ConstColIterator(Image* image, const T iterator) : base(image, iterator) { }
      ConstColIterator() { }

      typename Image::value_type operator*() const {
	return *m_iterator;
      }
      typename base::pointer operator->() const {
	return &*m_iterator;
      }

      // Image specific
 
      value_type get() const {
	return m_accessor(m_iterator);
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
      using VecIteratorBase<Image, Row, Col, VecIterator<Image, Row, Col> >::m_coliterator;
      
      typedef VecIterator self;
      typedef VecIteratorBase<Image, Row, Col, self> base;
      typedef typename base::value_type value_type;
      typedef ImageAccessor<value_type> accessor;
      // Constructor
      VecIterator(const Row iterator) : base(iterator) { }
      VecIterator() { }

      // Operators
      typename base::reference operator*() const { return *m_coliterator; }
      typename base::pointer operator->() const { return &*m_coliterator; }
      value_type get() const {
	return m_accessor(m_coliterator);
      }
      void set(const value_type& v) {
	m_accessor.set(v, m_coliterator);
      }
    private:
      accessor m_accessor;
    };

    template<class Image, class Row, class Col>
    class ConstVecIterator : public VecIteratorBase<Image, Row, Col,
						    ConstVecIterator<Image, Row, Col> > {
	public:
      using VecIteratorBase<Image, Row, Col, ConstVecIterator<Image, Row, Col> >::m_coliterator;

      typedef ConstVecIterator self;
      typedef VecIteratorBase<Image, Row, Col, self> base;
      typedef typename base::value_type value_type;
      typedef ImageAccessor<value_type> accessor;

      // Constructor
      ConstVecIterator(const Row iterator) : base(iterator) { }
      ConstVecIterator() { }

      // Operators
      typename Image::value_type operator*() const { return *m_coliterator; }
      typename base::pointer operator->() const { return &*m_coliterator; }
      value_type get() const {
	return m_accessor(m_coliterator);
      }
    private:
      accessor m_accessor;
    };

  } // namespace
} // namespace
#endif
