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

#include <cstddef>
#include <cmath>

namespace Gamera {

  template<class T> class ImageData {
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

    /*
      Constructors
    */
    ImageData(size_t nrows = 1, size_t ncols = 1, size_t page_offset_y = 0,
	       size_t page_offset_x = 0) {
      m_size = nrows * ncols;
      m_stride = ncols;
      m_page_offset_x = page_offset_x;
      m_page_offset_y = page_offset_y;
      m_data = 0;
      create_data();
    }
    ImageData(const Size<size_t>& size, size_t page_offset_y = 0,
	       size_t page_offset_x = 0) {
      m_size = (size.height() + 1) * (size.width() + 1);
      m_stride = size.width() + 1;
      m_page_offset_x = page_offset_x;
      m_page_offset_y = page_offset_y;
      m_data = 0;
      create_data();
    }
    ImageData(const Dimensions<size_t>& dim, size_t page_offset_y = 0,
	       size_t page_offset_x = 0) {
      m_size = dim.nrows() * dim.ncols();
      m_stride = dim.ncols();
      m_page_offset_x = page_offset_x;
      m_page_offset_y = page_offset_y;
      m_data = 0;
      create_data();
    }
    ~ImageData() {
      if (m_data != 0) {
	delete[] m_data;
      }
    }
    
    /*
      Various information about dimensions.
    */
    size_t stride() const { return m_stride; }
    size_t ncols() const { return m_stride; }
    size_t nrows() const { return size() / m_stride; }
    size_t page_offset_x() const { return m_page_offset_x; }
    size_t page_offset_y() const { return m_page_offset_y; }
    size_t size() const { return m_size; }
    size_t bytes() const { return m_size * sizeof(T); }
    double mbytes() const { return (m_size * sizeof(T)) / 1048576.0; }

    /*
      Setting dimensions
    */
    void page_offset_x(size_t x) { m_page_offset_x = x; }
    void page_offset_y(size_t y) { m_page_offset_y = y; }
    void nrows(size_t nrows) { resize(nrows * ncols()); }
    void ncols(size_t ncols) { m_stride = ncols; resize(nrows() * m_stride); }
    void dimensions(size_t rows, size_t cols) { m_stride = cols; resize(rows * cols); }
    void resize(size_t size) {
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
  private:
    void create_data() {
      if (m_size > 0)
	m_data = new T[m_size];
    }
    size_t m_size;
    size_t m_stride;
    size_t m_page_offset_x;
    size_t m_page_offset_y;
    T* m_data;
  };
}

#endif
