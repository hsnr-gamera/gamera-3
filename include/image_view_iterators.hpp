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

#ifndef kwm01022002_matrix_view_iterators_hpp
#define kwm01022002_matrix_view_iterators_hpp

#include "accessor.hpp"
#include "iterator_base.hpp"

namespace Gamera {
  namespace MatrixViewDetail {

    template<class Matrix, class T> class ColIterator;

    template<class Matrix, class T>
    class RowIterator : public RowIteratorBase<Matrix, RowIterator<Matrix, T>, T> {
    public:
      // Typedefs for rows
      typedef ColIterator<Matrix, T> iterator;

      // Convenience typedefs
      typedef RowIterator self;
      typedef RowIteratorBase<Matrix, self, T> base;

      // Constructor
      RowIterator(Matrix* matrix, const T iterator) : base(matrix, iterator) { }
      RowIterator() { }

      reference operator*() const {
	return *m_iterator;
      }

      pointer operator->() const {
	return &*m_iterator;
      }

      iterator begin() const {
	return iterator(m_matrix, m_iterator);
      }

      iterator end() const {
	return iterator(m_matrix, m_iterator + m_matrix->ncols());
      }
    };

    template<class Matrix, class T>
    class ColIterator : public ColIteratorBase<Matrix, ColIterator<Matrix, T>, T> {
    public:
      // Typedefs for Cols
      typedef RowIterator<Matrix, T> iterator;

      // Convenience typedefs
      typedef ColIterator self;
      typedef ColIteratorBase<Matrix, self, T> base;

      // Constructor
      ColIterator(Matrix* matrix, const T iterator) : base(matrix, iterator) { }
      ColIterator() { }

      reference operator*() const {
	return *m_iterator;
      }
      pointer operator->() const {
	return &*m_iterator;
      }

      // Matrix specific
      iterator begin() const {
	return iterator(m_matrix, m_iterator);
      }
      iterator end() const {
	return iterator(m_matrix, m_iterator) + m_matrix->nrows();
      }
    };

    template<class Matrix, class T> class ConstColIterator;

    template<class Matrix, class T>
    class ConstRowIterator : public RowIteratorBase<Matrix, ConstRowIterator<Matrix, T>, T> {
    public:
      // Typedefs for rows
      typedef ConstColIterator<Matrix, T> iterator;

      // Convenience typedefs
      typedef ConstRowIterator self;
      typedef RowIteratorBase<Matrix, self, T> base;

      // Constructor
      ConstRowIterator(Matrix* matrix, const T iterator) : base(matrix, iterator) { }
      ConstRowIterator() { }

      typename Matrix::value_type operator*() const {
	return *m_iterator;
      }

      pointer operator->() const {
	return &*m_iterator;
      }

      iterator begin() const {
	return iterator(m_matrix, m_iterator);
      }

      iterator end() const {
	return iterator(m_matrix, m_iterator + m_matrix->ncols());
      }
    };

    template<class Matrix, class T>
    class ConstColIterator : public ColIteratorBase<Matrix, ConstColIterator<Matrix, T>, T> {
    public:
      // Typedefs for Cols
      typedef ConstRowIterator<Matrix, T> iterator;

      // Convenience typedefs
      typedef ConstColIterator self;
      typedef ColIteratorBase<Matrix, self, T> base;

      // Constructor
      ConstColIterator(Matrix* matrix, const T iterator) : base(matrix, iterator) { }
      ConstColIterator() { }

      typename Matrix::value_type operator*() const {
	return *m_iterator;
      }
      pointer operator->() const {
	return &*m_iterator;
      }

      // Matrix specific
      iterator begin() const {
	return iterator(m_matrix, m_iterator);
      }
      iterator end() const {
	return iterator(m_matrix, m_iterator) + m_matrix->nrows();
      }
    };


    template<class Matrix, class Row, class Col>
    class VecIterator : public VecIteratorBase<Matrix, Row, Col,
					       VecIterator<Matrix, Row, Col> > {
    public:
      typedef VecIterator self;
      typedef VecIteratorBase<Matrix, Row, Col, self> base;
      // Constructor
      VecIterator(const Row iterator) : base(iterator) { }
      VecIterator() { }

      // Operators
      reference operator*() const { return *m_coliterator; }
      pointer operator->() const { return &*m_coliterator; }
    };

    template<class Matrix, class Row, class Col>
    class ConstVecIterator : public VecIteratorBase<Matrix, Row, Col,
						    ConstVecIterator<Matrix, Row, Col> > {
    public:
      typedef ConstVecIterator self;
      typedef VecIteratorBase<Matrix, Row, Col, self> base;
      // Constructor
      ConstVecIterator(const Row iterator) : base(iterator) { }
      ConstVecIterator() { }

      // Operators
      typename Matrix::value_type operator*() const { return *m_coliterator; }
      pointer operator->() const { return &*m_coliterator; }
    };

  } // namespace
} // namespace
#endif
