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

#ifndef kwm01032002_connected_component_hpp
#define kwm01032002_connected_component_hpp

#include "dimensions.hpp"
#include "image.hpp"
#include "image_view_iterators.hpp"
#include "connected_components_iterators.hpp"
#include "vigra_iterators.hpp"

#include <stdexcept>
#include <exception>
#include <list>

/*
  ConnectedComponent
	
  This class implements a filtered image view. Within a ConnectedComponent
  only those pixels that match the assigned label will be shown. This requires
  the use of a proxy type for iterator types that require an lvalue be returned
  from dereferncing (see CCProxy below). KWM
*/

namespace Gamera {


  template<class T>
  class ConnectedComponent
    : public ImageBase<typename T::value_type> {
  public:
    using ImageBase<typename T::value_type>::ncols;
    using ImageBase<typename T::value_type>::nrows;
    using ImageBase<typename T::value_type>::offset_x;
    using ImageBase<typename T::value_type>::offset_y;

    // standard STL typedefs
    typedef typename T::value_type value_type;
    typedef typename T::pointer pointer;
    typedef typename T::reference reference;
    typedef typename T::difference_type difference_type;
    // Gamera specific
    typedef T data_type;
	
    // Vigra typedefs
    typedef value_type PixelType;

    // convenience typedefs
    typedef ConnectedComponent self;
    typedef ImageBase<typename T::value_type> base_type;

    //
    // CONSTRUCTORS
    //
    ConnectedComponent() : base_type() {
      m_image_data = 0;
      m_label = 0;
    }
    ConnectedComponent(T& image_data, value_type label, size_t offset_y,
		       size_t offset_x, size_t nrows, size_t ncols)
      : base_type(offset_y, offset_x, nrows, ncols), m_label(label) {
      m_image_data = &image_data;
      range_check();
      calculate_iterators();
    }
    ConnectedComponent(T& image_data)
      : base_type(image_data.page_offset_y(), image_data.page_offset_x(),
		  image_data.nrows(), image_data.ncols()) {
      m_image_data = &image_data;
      range_check();
      calculate_iterators();
    }
    ConnectedComponent(T& image_data, value_type label,
		       const Rect& rect)
      : base_type(rect), m_label(label) {
      m_image_data = &image_data;
      range_check();
      calculate_iterators();
    }
    ConnectedComponent(T& image_data, value_type label,
		       const Point& upper_left,
		       const Point& lower_right)
      : base_type(upper_left, lower_right), m_label(label) {
      m_image_data = &image_data;
      range_check();
      calculate_iterators();
    }
    ConnectedComponent(T& image_data, value_type label,
		       const Point& upper_left,
		       const Size& size)
      : base_type(upper_left, size), m_label(label) {
      m_image_data = &image_data;
      range_check();
      calculate_iterators();
    }
    ConnectedComponent(T& image_data, value_type label,
		       const Point& upper_left,
		       const Dimensions& dim)
      : base_type(upper_left, dim), m_label(label) {
      m_image_data = &image_data;
      range_check();
      calculate_iterators();
    }
    //
    // COPY CONSTRUCTORS
    //
    ConnectedComponent(const self& other, size_t offset_y,
		       size_t offset_x, size_t nrows, size_t ncols)
      : base_type(offset_y, offset_x, nrows, ncols) {
      m_image_data = other.m_image_data;
      m_label = other.label();
      range_check();
      calculate_iterators();
    }
    ConnectedComponent(const self& other, const Rect& rect)
      : base_type(rect) {
      m_image_data = other.m_image_data;
      m_label = other.label();
      range_check();
      calculate_iterators();
    }
    ConnectedComponent(const self& other, const Point& upper_left,
		       const Point& lower_right)
      : base_type(upper_left, lower_right) {
      m_image_data = other.m_image_data;
      m_label = other.label();
      range_check();
      calculate_iterators();
    }
    ConnectedComponent(const self& other, const Point& upper_left,
		       const Size& size)
      : base_type(upper_left, size) {
      m_image_data = other.m_image_data;
      m_label = other.label();
      range_check();
      calculate_iterators();
    }
    ConnectedComponent(const self& other, const Point& upper_left,
		       const Dimensions& dim)
      : base_type(upper_left, dim) {
      m_image_data = other.m_image_data;
      m_label = other.label();
      range_check();
      calculate_iterators();
    }
    //
    //  FUNCTION ACCESS
    //
    value_type get(size_t row, size_t col) const {
      value_type tmp = *(m_const_begin + (row * m_image_data->stride()) + col);
      if (tmp == m_label)
	return tmp;
      return 0;
    }
    void set(size_t row, size_t col, value_type value) {
      if (*(m_begin + (row * m_image_data->stride()) + col) == m_label)
	*(m_begin + (row * m_image_data->stride()) + col) = m_label;

    }
    value_type get(const Point& p) const {
      return get(p.y(), p.x());
    }
    void set(const Point& p, value_type value) {
      set(p.y(), p.x(), value);
    }

    //
    // DIMENSIONS
    //
    // redefine the dimensions change function from Rect
    virtual void dimensions_change() {
      range_check();
      calculate_iterators();
    }

    //
    // Misc
    //
    virtual ImageDataBase* data() const { return m_image_data; }
    ImageView<T> parent() {
      return ImageView<T>(*m_image_data, 0, 0, m_image_data->nrows(),
			   m_image_data->ncols());
    }
    ImageView<T> image() {
      return ImageView<T>(*m_image_data, offset_y(), offset_x(), nrows(),
			  ncols());
    }
    value_type label() const {
      return m_label;
    }

    void label(value_type label) {
      m_label = label;
    }

    //
    // Iterators
    //
    typedef CCDetail::RowIterator<self, typename T::iterator> row_iterator;
    row_iterator row_begin() {
      return row_iterator(this, m_begin);
    }
    row_iterator row_end() {
      return row_iterator(this, m_end);
    }

    typedef CCDetail::ColIterator<self, typename T::iterator> col_iterator;
    col_iterator col_begin() {
      return col_iterator(this, m_begin);
    }
    col_iterator col_end() {
      return col_iterator(this, m_begin + ncols());
    }

    //
    // Const Iterators
    //
    typedef CCDetail::ConstRowIterator<const self, typename T::const_iterator> const_row_iterator;
    const_row_iterator row_begin() const {
      return const_row_iterator(this, m_const_begin);
    }
    const_row_iterator row_end() const {
      return const_row_iterator(this, m_const_end);
    }

    typedef CCDetail::ConstColIterator<const self, typename T::const_iterator> const_col_iterator;
    const_col_iterator col_begin() const {
      return const_col_iterator(this, m_const_begin);
    }
    const_col_iterator col_end() const {
      return const_col_iterator(this, m_const_begin + ncols());
    }

    //
    // 2D iterators
    //
    typedef Gamera::ImageIterator<ConnectedComponent, typename T::iterator> Iterator;
    Iterator upperLeft() {
      return Iterator(this, m_image_data->begin(), m_image_data->stride())
	+ Diff2D(offset_x() - m_image_data->page_offset_x(), offset_y() - m_image_data->page_offset_y());
    }
    Iterator lowerRight() {
      return Iterator(this, m_image_data->begin(), m_image_data->stride())
	+ Diff2D(offset_x() + ncols() - m_image_data->page_offset_x(),
		 offset_y() + nrows() - m_image_data->page_offset_y());
    }

    typedef Gamera::ConstImageIterator<const ConnectedComponent,
				  typename T::const_iterator> ConstIterator;
    ConstIterator upperLeft() const {
      return ConstIterator(this, static_cast<const T*>(m_image_data)->begin(), m_image_data->stride())
	+ Diff2D(offset_x() - m_image_data->page_offset_x(),
		 offset_y() - m_image_data->page_offset_y());
    }
    ConstIterator lowerRight() const {
      return ConstIterator(this, static_cast<const T*>(m_image_data)->begin(), m_image_data->stride())
	+ Diff2D(offset_x() + ncols() - m_image_data->page_offset_x(),
		 offset_y() + nrows() - m_image_data->page_offset_y());
    }

    //
    // Vector iterator
    //
    typedef CCDetail::VecIterator<self, row_iterator, col_iterator> vec_iterator;
    vec_iterator vec_begin() { return vec_iterator(row_begin()); }
    vec_iterator vec_end() { return vec_iterator(row_end()); }

    typedef CCDetail::ConstVecIterator<self,
      const_row_iterator, const_col_iterator> const_vec_iterator;
    const_vec_iterator vec_begin() const {
      return const_vec_iterator(row_begin());
    }
    const_vec_iterator vec_end() const {
      return const_vec_iterator(row_end());
    }

    //
    // OPERATOR ACCESS
    //
    col_iterator operator[](size_t n) {
      return col_iterator(this, m_begin + (n * data()->stride())); }
    const_col_iterator operator[](size_t n) const {
      return const_col_iterator(this, m_begin + (n * data()->stride())); }
  private:
    /*
      We pre-compute iterators here in an effort to make begin() and end()
      methods a little faster. Unfortunately we have to to keep around both
      normal and const iterators.
    */
    void calculate_iterators() {
      m_begin = m_image_data->begin()
        // row offset
        + (m_image_data->stride() * (offset_y() - m_image_data->page_offset_y()))
        // col offset
        + (offset_x() - m_image_data->page_offset_x());
      m_end = m_image_data->begin()
        // row offset
        + (m_image_data->stride() * ((offset_y() - m_image_data->page_offset_y()) + nrows()))
        // column offset
        + (offset_x() - m_image_data->page_offset_x());
      const T* cmd = static_cast<const T*>(m_image_data);
      m_const_begin = cmd->begin()
        // row offset
	+ (m_image_data->stride() * (offset_y() - m_image_data->page_offset_y()))
        // col offset
        + (offset_x() - m_image_data->page_offset_x());
      m_const_end = cmd->begin()
        // row offset
        + (m_image_data->stride() * ((offset_y() - m_image_data->page_offset_y()) + nrows()))
        // column offset
        + (offset_x() - m_image_data->page_offset_x());
    }
    void range_check() {
      if (offset_y() + nrows() - m_image_data->page_offset_y() > m_image_data->nrows() ||
	  offset_x() + ncols() - m_image_data->page_offset_x() > m_image_data->ncols()
	  || offset_y() < m_image_data->page_offset_y()
	  || offset_x() < m_image_data->page_offset_x()) {
	char error[1024];
	sprintf(error, "Image view dimensions out of range for data\n");
	sprintf(error, "%s\tnrows %d\n", error, (int)nrows());
	sprintf(error, "%s\toffset_y %d\n", error, (int)offset_y());
	sprintf(error, "%s\tdata nrows %d\n", error, (int)m_image_data->nrows());
	sprintf(error, "%s\tncols %d\n", error, (int)ncols());
	sprintf(error, "%s\toffset_x %d\n", error, (int)offset_x());
	sprintf(error, "%s\tdata ncols %d\n", error,(int)m_image_data->ncols());
	throw std::range_error(error);
      }
    }
    // Pointer to the data for this view
    T* m_image_data;
    // Cached iterators - see calculate_iterators above.
    typename T::iterator m_begin, m_end;
    typename T::const_iterator m_const_begin, m_const_end;
    // The label for this connected-component
    value_type m_label;
  };
}

#endif
