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

#ifndef kwm07172002_iterator_base
#define kwm07172002_iterator_base

#include "accessor.hpp"

namespace Gamera {

  // some convenience functions
  
  // find the current row
  template<class Mat, class T>
  inline size_t row_number(const Mat* mat, const T curr) {
    return ((curr - mat->data()->begin())
	    / mat->data()->stride()) - mat->offset_y();
  }
  
  // find the current col
  template<class Mat, class T>
  inline size_t col_number(const Mat* mat, const T curr) {
    size_t row = row_number(mat, curr) + mat->offset_y();
    T row_begin = mat->data()->begin() + (row * mat->data()->stride());
    return (curr - row_begin) - mat->offset_x();
  }
  
  template<class Image, class Iterator, class T>
  class RowIteratorBase
    : public std::iterator<std::random_access_iterator_tag, typename Image::value_type,
			   typename Image::difference_type, typename Image::pointer,
			   typename Image::reference>
  {
  public:
    // Convenience typedefs
    typedef std::iterator<std::random_access_iterator_tag, typename Image::value_type,
			  typename Image::difference_type, typename Image::pointer,
			  typename Image::reference> base_type;
    typedef Iterator self;
    
    // Constructor
    RowIteratorBase(Image* image, const T iterator)
      : m_image(image), m_iterator(iterator) { }
    RowIteratorBase() { }
    
    self& operator++() {
      m_iterator += m_image->data()->stride();
      return (self&)*this;
    }
    self operator++(int) {
      self tmp;
      tmp.m_image = m_image;
      tmp.m_iterator = m_iterator;
      m_iterator += m_image->data()->stride();
      return tmp;
    }
    self& operator--() {
      m_iterator -= m_image->data()->stride();
      return (self&)*this;
    }
    self operator--(int) {
      self tmp;
      tmp.m_image = m_image;
      tmp.m_iterator = m_iterator;
      m_iterator -= m_image->data()->stride();
      return tmp;
    }
    self& operator+=(size_t n) {
      m_iterator += m_image->data()->stride() * n;
      return (self&)*this;
    }
    self operator+(size_t n) const {
      self tmp;
      tmp.m_image = m_image;
      tmp.m_iterator = m_iterator + (m_image->data()->stride() * n);
      return tmp;
    }
    self& operator-=(size_t n) {
      m_iterator -= m_image->data()->stride() * n;
      return (self&)*this;
    }
    self operator-(size_t n) const {
      self tmp;
      tmp.m_image = m_image;
      tmp.m_iterator = m_iterator - (m_image->data()->stride() * n);
      return tmp;
    }
    bool operator==(const RowIteratorBase& other) const {
      return m_iterator == other.m_iterator;
    }
    bool operator!=(const RowIteratorBase& other) const {
      return m_iterator != other.m_iterator;
    }
    bool operator<(const RowIteratorBase& other) const {
      return m_iterator < other.m_iterator;
    }
    bool operator>(const RowIteratorBase& other) const {
      return m_iterator > other.m_iterator;
    }
    typename base_type::difference_type operator-(const RowIteratorBase& other) const {
      return (m_iterator - other.m_iterator) / m_image->data()->stride();
    }
    
    size_t row() const {
      return row_number(m_image, m_iterator);
    }
    size_t col() const {
      return col_number(m_image, m_iterator);
    }
  public:
    Image* m_image;
    T m_iterator;
  };

  template<class Image, class Iterator, class T>
  class ColIteratorBase
    : public std::iterator<std::random_access_iterator_tag, typename Image::value_type,
			   typename Image::difference_type, typename Image::pointer,
			   typename Image::reference>
  {
  public:
    // Convenience typedefs
    typedef std::iterator<std::random_access_iterator_tag, typename Image::value_type,
			  typename Image::difference_type, typename Image::pointer,
			  typename Image::reference> base_type;
    typedef Iterator self;
    
    // Constructor
    ColIteratorBase(Image* image, const T iterator) : m_iterator(iterator),
						    m_image(image) { }
    ColIteratorBase() { }

    self& operator++() {
      ++m_iterator;
      return (self&)*this;
    }
    self operator++(int) {
      self tmp;
      tmp.m_image = m_image;
      tmp.m_iterator = m_iterator;
      ++m_iterator;
      return tmp;
    }
    self& operator--() {
      --m_iterator;
      return (self&)*this;
    }
    self operator--(int) {
      self tmp;
      tmp.m_image = m_image;
      tmp.m_iterator = m_iterator;
      --m_iterator;
      return tmp;
    }
    self& operator+=(size_t n) {
      m_iterator += n;
      return (self&)*this;
    }
    self operator+(size_t n) const {
      self tmp;
      tmp.m_image = m_image;
      tmp.m_iterator = m_iterator + n;
      return tmp;
    }
    self& operator-=(size_t n) {
      m_iterator -= n;
      return (self&)*this;
    }
    self operator-(size_t n) const {
      self tmp;
      tmp.m_image = m_image;
      tmp.m_iterator = m_iterator - n;
      return tmp;
    }
    bool operator==(const ColIteratorBase& other) const {
      return m_iterator == other.m_iterator;
    }
    bool operator!=(const ColIteratorBase& other) const {
      return m_iterator != other.m_iterator;
    }
    bool operator<(const ColIteratorBase& other) const {
      return m_iterator < other.m_iterator;
    }
    bool operator>(const ColIteratorBase& other) const {
      return m_iterator > other.m_iterator;
    }
    typename base_type::difference_type operator-(const ColIteratorBase& other) const {
      return (m_iterator - other.m_iterator);
    }

    // Image specific
    size_t row() const {
      return row_number(m_image, m_iterator);
    }
    size_t col() const {
      return col_number(m_image, m_iterator);
    }
  public:
    T m_iterator;
    Image* m_image;
  };

  template<class Image, class Row, class Col, class Iterator>
  class VecIteratorBase
    : public std::iterator<std::random_access_iterator_tag, typename Image::value_type,
			   typename Image::difference_type, typename Image::pointer,
			   typename Image::reference>
  {
  public:
    // Convenience typedefs
    typedef std::iterator<std::random_access_iterator_tag, typename Image::value_type,
			  typename Image::difference_type, typename Image::pointer,
			  typename Image::reference> base_type;
    typedef Iterator self;
			
    // Constructor
    VecIteratorBase(const Row iterator)
      : m_rowiterator(iterator), m_coliterator(m_rowiterator.begin()) { }
    VecIteratorBase() { }

    // Operators
    self& operator++() {
      ++m_coliterator;
      // If we are at the end of the row, go down to the next column
      if (m_coliterator == m_rowiterator.end()) {
	++m_rowiterator;
	m_coliterator = m_rowiterator.begin();
      }
      return (self&)*this;
    }
    self operator++(int) {
      self tmp;
      tmp.m_rowiterator = m_rowiterator;
      tmp.m_coliterator = m_coliterator;
      this->operator++();
      return tmp;
    }
    self& operator+=(size_t n) {
      // Find out if we have enough rome to just move the col iterator
      size_t distance_to_col_end = m_rowiterator.end() - m_coliterator;
      // The easy case - we have enough room
      if (distance_to_col_end > n) {
	m_coliterator += n;
      } else {
	size_t left_to_move = n - distance_to_col_end;
	if (left_to_move == 0) {
	  // if we are just moving to the beginning of the next row
	  ++m_rowiterator;
	  m_coliterator = m_rowiterator.begin();
	} else {
	  size_t col_length = m_rowiterator.end() - m_rowiterator.begin();
	  size_t nrows_to_move = (left_to_move / col_length);
	  // move the row iterator - plus one because we always move at least 1 row
	  m_rowiterator += nrows_to_move + 1;
	  // the plus one is for moving to the beginning of a row
	  left_to_move -= (nrows_to_move * col_length);
	  // move the col iterator
	  m_coliterator = m_rowiterator.begin() + left_to_move; 
	}
      }
      return (self&)*this;
    }
    self operator+(size_t n) {
      self tmp;
      tmp.m_rowiterator = m_rowiterator;
      tmp.m_coliterator = m_coliterator;
      tmp += n;
      return tmp;
    }
    self& operator--() {
      // If we are at the begining of the row
      if (m_coliterator == m_rowiterator.begin()) {
	--m_rowiterator;
	m_coliterator = m_rowiterator.end();
      }
      --m_coliterator;
      return (self&)*this;
    }
    self operator--(int) {
      self tmp;
      tmp.m_rowiterator = m_rowiterator;
      tmp.m_coliterator = m_coliterator;
      this->operator--();
      return tmp;
    }
    self& operator-=(size_t n) {
      // Find out if we have enough rome to just move the col iterator
      size_t distance_to_col_begin = m_coliterator - m_rowiterator.begin();
      // The easy case - we have enough room
      if (distance_to_col_begin >= n) {
	m_coliterator -= n;
      } else {
	size_t left_to_move = n - distance_to_col_begin;
	size_t col_length = m_rowiterator.end() - m_rowiterator.begin();
	size_t nrows_to_move = (left_to_move / col_length);
	// move the row iterator - plus one because we always move at least 1 row
	m_rowiterator -= nrows_to_move + 1;
	// the plus one is for moving to the beginning of a row
	left_to_move -= (nrows_to_move * col_length);
	// move the col iterator
	m_coliterator = m_rowiterator.end() - (left_to_move); 
      }
      return (self&)*this;
    }
    self operator-(size_t n) {
      self tmp;
      tmp.m_rowiterator = m_rowiterator;
      tmp.m_coliterator = m_coliterator;
      tmp -= n;
      return tmp;
    }
    bool operator==(const VecIteratorBase& other) const {
      return m_coliterator == other.m_coliterator;
    }
    bool operator!=(const VecIteratorBase& other) const {
      return m_coliterator != other.m_coliterator;
    }
    bool operator<(const VecIteratorBase& other) const {
      return m_coliterator < other.m_coliterator;
    }
    bool operator>(const VecIteratorBase& other) const {
      return m_coliterator > other.m_coliterator;
    }
    typename base_type::difference_type operator-(const self& other) const {
      size_t nrows = m_rowiterator - other.m_rowiterator;
      // simple case - nrows = 0 so we can just compare the coliterators
      if (nrows == 0) {
	return m_coliterator - other.m_coliterator;
      } else {
	size_t other_col_distance = other.m_rowiterator.end() - other.m_coliterator;
	size_t col_distance = m_coliterator - m_rowiterator.begin();
	size_t col_length = m_rowiterator.end() - m_rowiterator.begin();
	return ((nrows - 1) * col_length) + other_col_distance + col_distance;
      }
    }
  public:
    Row m_rowiterator;
    Col m_coliterator;
  };
}

#endif
