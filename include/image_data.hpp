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

/*
  The ImageData class is dense storage for Gamera matrices. This class is used
  rather than a standard vector so that we can control the iterator type - the
  Vigra iterators assume that the iterator type is T* and some std::vectors
  don't use that as the iterator type.
*/

#ifndef kwm11162001_image_data_hpp
#define kwm11162001_image_data_hpp

#include "dimensions.hpp"

#include <cstddef>
#include <cmath>
#include <iostream>

namespace Gamera {

  class ImageDataBase {
  public:
    ImageDataBase(size_t nrows = 1, size_t ncols = 1, size_t page_offset_y = 0,
		  size_t page_offset_x = 0) {
      m_size = nrows * ncols;
      m_stride = ncols;
      m_page_offset_x = page_offset_x;
      m_page_offset_y = page_offset_y;
      m_user_data = 0;
    }
    ImageDataBase(const Size& size, size_t page_offset_y = 0,
		  size_t page_offset_x = 0) {
      m_size = (size.height() + 1) * (size.width() + 1);
      m_stride = size.width() + 1;
      m_page_offset_x = page_offset_x;
      m_page_offset_y = page_offset_y;
      m_user_data = 0;
    }
    ImageDataBase(const Dimensions& dim, size_t page_offset_y = 0,
		  size_t page_offset_x = 0) {
      m_size = dim.nrows() * dim.ncols();
      m_stride = dim.ncols();
      m_page_offset_x = page_offset_x;
      m_page_offset_y = page_offset_y;
      m_user_data = 0;
    }
    virtual ~ImageDataBase() { }

    /*
      Various information about dimensions.
    */
    size_t stride() const { return m_stride; }
    size_t ncols() const { return m_stride; }
    size_t nrows() const { return size() / m_stride; }
    size_t page_offset_x() const { return m_page_offset_x; }
    size_t page_offset_y() const { return m_page_offset_y; }
    size_t size() const { return m_size; }
    virtual size_t bytes() const = 0;
    virtual double mbytes() const = 0;

    /*
      Setting dimensions
    */
    void page_offset_x(size_t x) { m_page_offset_x = x; }
    void page_offset_y(size_t y) { m_page_offset_y = y; }
    virtual void nrows(size_t nrows) {
      do_resize(nrows * m_stride);
    }
    virtual void ncols(size_t ncols) {
      m_stride = ncols;
      do_resize((m_size / m_stride) * m_stride);
    }
    virtual void dimensions(size_t rows, size_t cols) = 0;
  public:
    void* m_user_data;
  protected:
    virtual void do_resize(size_t size) = 0;
    size_t m_size;
    size_t m_stride;
    size_t m_page_offset_x;
    size_t m_page_offset_y;
  };

  template<class T>
  class ImageData : public ImageDataBase {
  public:
    /*
      Standard typedefs
    */
    typedef T value_type;
    typedef T& reference;
    typedef T* pointer;
    typedef int difference_type;
    typedef T* iterator;
    typedef const T* const_iterator;

    ImageData(size_t nrows = 1, size_t ncols = 1, size_t page_offset_y = 0,
	      size_t page_offset_x = 0) : ImageDataBase(nrows, ncols,
							page_offset_y,
							page_offset_x) {
      m_data = 0;
      create_data();
    }
    ImageData(const Size& size, size_t page_offset_y = 0,
	      size_t page_offset_x = 0) : ImageDataBase(size,
							page_offset_y,
							page_offset_x) {
      m_data = 0;
      create_data();
    }
    ImageData(const Dimensions& dim, size_t page_offset_y = 0,
	      size_t page_offset_x = 0) : ImageDataBase(dim,
							page_offset_y,
							page_offset_x) {
      m_data = 0;
      create_data();
    }

    /*
      Destructor
    */
    virtual ~ImageData() {
      if (m_data != 0) {
	delete[] m_data;
      }
    }
    
    virtual size_t bytes() const { return m_size * sizeof(T); }
    virtual double mbytes() const { return (m_size * sizeof(T)) / 1048576.0; }
    virtual void dimensions(size_t rows, size_t cols) {
      m_stride = cols; do_resize(rows * cols); }


    /*
      Iterators
    */
    iterator begin() { return m_data; }
    iterator end() { return m_data + m_size; }
    const_iterator begin() const { return m_data; }
    const_iterator end() const { return m_data + m_size; }

    /*
      Operators
    */
    T& operator[](size_t n) { return m_data[n]; }
  protected:
    virtual void do_resize(size_t size) {
      if (size > 0) {
	size_t smallest = std::min(m_size, size);
	m_size = size;
	T* new_data = new T[m_size];
	for (size_t i = 0; i < smallest; ++i)
	  new_data[i] = m_data[i];
	if (m_data)
	  delete[] m_data;
	m_data = new_data;
      } else {
	if (m_data)
	  delete[] m_data;
	m_data = 0;
	m_size = 0;
      }
    }
  private:
    void create_data() {
      if (m_size > 0)
	m_data = new T[m_size];
    }

    T* m_data;
  };
}

#endif
