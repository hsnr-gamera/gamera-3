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

#ifndef kwm11162001_dimensions_hpp
#define kwm11162001_dimensions_hpp

namespace Gamera {

  /*
   * Point
   *
   * This is a simple class to hold a single coordinate (x/y value pair).
   */
  template<class T>
  class Point {
  public:
    Point() : m_x(0), m_y(0) { }
    Point(T x, T y) { m_x = x; m_y = y; }
    T x() const { return m_x; }
    T y() const { return m_y; }
    void x(T x) { m_x = x; }
    void y(T y) { m_y = y; }
    void move(int x, int y) {
      m_x += x;
      m_y += y;
    }
    bool operator==(const Point& x) const {
      if (m_x == x.m_x && m_y == x.m_y)
        return true;
      else
        return false;
    }
    bool operator!=(const Point& x) const {
      if (m_x != x.m_x || m_y != x.m_y)
	return true;
      else
	return false;
    }
  private:
    T m_x, m_y;
  };

  /*
   * There are size _and_ dimension objects so that users can use
   * whichever coordinate system is most natural.
   *
   * A single point has the dimension Point(0,0)
   * and Size(0, 0) or Dimensions(1, 1)
   */

  /*
   * Size
   *
   * A simple class that holds width and height. These dimensions are
   * refer to nrows or ncols - 1.
   */
  template<class T> class Size {
  public:
    Size() : m_width(1), m_height(1) { }
    Size(T width, T height) {
      m_width = width;
      m_height = height;
    }
    T width() const { return m_width; }
    T height() const { return m_height; }
    void width(T width) { m_width = width; }
    void height(T height) { m_height = height; }
    bool operator==(const Size& other) const {
      if (m_width == other.width() && m_height == other.height())
	return true;
      else
	return false;
    }
    bool operator!=(const Size& other) const {
      if (m_width != other.width() || m_height != other.height())
	return true;
      else
	return false;
    }
  private:
    T m_width, m_height;
  };

  /*
   * Dimensions
   *
   * A simple class that holds nrows and ncols. These dimensions are
   * refer to width or height + 1.
   */
  template<class T> class Dimensions {
  public:
    Dimensions() : m_ncols(1), m_nrows(1) { }
    Dimensions(T rows, T cols) {
      m_ncols = cols;
      m_nrows = rows;
    }
    T ncols() const { return m_ncols; }
    T nrows() const { return m_nrows; }
    void ncols(T ncols) { m_ncols = ncols; }
    void nrows(T nrows) { m_nrows = nrows; }
    bool operator==(const Dimensions& other) const {
      if (m_ncols == other.ncols() && m_nrows == other.nrows())
	return true;
      else
	return false;
    }
    bool operator!=(const Dimensions& other) const {
      if (m_ncols != other.ncols() || m_nrows != other.nrows())
	return true;
      else
	return false;
    }
  private:
    T m_ncols, m_nrows;
  };

  /*
   * Rect
   *
   * A rectangle class
   */
  template<class T>
  class Rect {
  public:
    typedef Point<T> point_type;
    typedef Size<T> size_type;
    typedef Dimensions<T> dimensions_type;
    typedef Rect self;
    Rect() : m_origin(0, 0), m_lr(1, 1) { }
    Rect(T origin_y, T origin_x, T nrows, T ncols)
      : m_origin(origin_x, origin_y),
      	m_lr(origin_x + ncols - 1, origin_y + nrows - 1) { }
    Rect(const point_type& upper_left, const point_type& lower_right)
      : m_origin(upper_left), m_lr(lower_right) { }
    Rect(const point_type& upper_left, const size_type& size)
      : m_origin(upper_left), m_lr(upper_left.x() + size.width(),
				   upper_left.y() + size.height()) { }
    Rect(const point_type& upper_left, const dimensions_type& dim)
      : m_origin(upper_left), m_lr(upper_left.x() + dim.ncols() - 1,
				   upper_left.y() + dim.nrows() - 1) { }
    virtual ~Rect() { }
    // Get
    point_type ul() const { return m_origin; }
    T ul_x() const { return m_origin.x(); }
    T ul_y() const { return m_origin.y(); }
    point_type ur() const { return point_type(m_lr.x(), m_origin.y()); }
    T ur_x() const { return m_lr.x(); }
    T ur_y() const { return m_origin.y(); }
    point_type lr() const { return m_lr; }
    T lr_x() const { return m_lr.x(); }
    T lr_y() const { return m_lr.y(); }
    point_type ll() const { return point_type(m_origin.x(), m_lr.y()); }
    T ll_x() const { return m_origin.x(); }
    T ll_y() const { return m_lr.y(); }
    dimensions_type dimensions() const {
      return dimensions_type(nrows(), ncols());
    }
    size_type size() const { return size_type(width(), height()); }
    T ncols() const { return m_lr.x() - m_origin.x() + 1; }
    T nrows() const { return m_lr.y() - m_origin.y() + 1; }
    T width() const { return m_lr.x() - m_origin.x(); }
    T height() const { return m_lr.y() - m_origin.y(); }
    T offset_x() const { return m_origin.x(); }
    T offset_y() const { return m_origin.y(); }
    // Set
    void ul(const point_type& ul) { m_origin = ul; dimensions_change(); }
    void ul_x(T v) { m_origin.x(v); dimensions_change(); }
    void ul_y(T v) { m_origin.y(v); dimensions_change(); }
    void ur(const point_type& ur) {
      m_lr.x(ur.x()); m_origin.y(ur.y()); dimensions_change();
    }
    void ur_x(T v) { m_lr.x(v); dimensions_change(); }
    void ur_y(T v) { m_origin.y(v); dimensions_change(); }
    void lr(const point_type& lr) { m_lr = lr; dimensions_change(); }
    void lr_x(T v) { m_lr.x(v); dimensions_change(); }
    void lr_y(T v) { m_lr.y(v); dimensions_change(); }
    void ll(const point_type& ll) {
      m_origin.x(ll.x());
      m_lr.y(ll.y());
      dimensions_change();
    }
    void ll_x(T v) { m_origin.x(v); dimensions_change(); }
    void ll_y(T v) { m_lr.y(v); dimensions_change(); }
    void dimensions(const dimensions_type& dim) {
      nrows(dim.nrows());
      ncols(dim.ncols());
      dimensions_change();
    }
    void dimensions(T nrows, T ncols) {
      this->nrows(nrows);
      this->ncols(ncols);
      dimensions_change();
    }
    void ncols(T v) {
      m_lr.x(m_origin.x() + v - 1);
      dimensions_change();
    }
    void nrows(T v) {
      m_lr.y(m_origin.y() + v - 1);
      dimensions_change();
    }
    void size(const size_type& size) {
      width(size.width());
      height(size.height());
      dimensions_change();
    }
    void size(T width, T height) {
      this->width(width);
      this->height(height);
      dimensions_change();
    }
    void width(T width) {
      m_lr.x(m_origin.x() + width);
      dimensions_change();
    }
    void height(T height) {
      m_lr.y(m_origin.y() + height);
      dimensions_change();
    }
    void rect_set(T origin_y, T origin_x, T nrows, T ncols) {
      m_origin.x(origin_x);
      m_origin.y(origin_y);
      m_lr.x(origin_x + ncols - 1);
      m_lr.y(origin_y + nrows - 1);
      dimensions_change();
    }
    void rect_set(const point_type& upper_left, const point_type& lower_right) {
      m_origin = upper_left;
      m_lr = lower_right;
      dimensions_change();
    }
    void rect_set(const point_type& upper_left, const size_type& size) {
      m_origin = upper_left;
      this->size(size);
      dimensions_change();
    }
    void rect_set(const point_type& upper_left, const dimensions_type& dim) {
      m_origin = upper_left;
      dimensions(dim);
      dimensions_change();
    }
    void offset_x(T v) { m_origin.x(v); dimensions_change(); }
    void offset_y(T v) { m_origin.y(v); dimensions_change(); }
    void move(int x, int y) {
      m_origin.move(x, y);
      m_lr.move(x, y);
    }
    // intersection
    bool contains_x(T v) const {
      if (v >= m_origin.x() && v <= lr_x())
	return true;
      else
	return false;
    }
    bool contains_y(T v) const {
      if (v >= m_origin.y() && v <= lr_y())
	return true;
      else
	return false;
    }
    bool contains_point(const Point<T>& v) const {
      if (contains_x(v.x()) && contains_y(v.y()))
	return true;
      else
	return false;
    }
    bool contains_rect(const self& v) const {
      if (contains_point(v.ul()) && contains_point(v.lr()))
	return true;
      else
	return false;
    }
    bool operator==(const Rect& other) const {
      if (m_origin == other.m_origin && m_lr == other.m_lr)
	return true;
      else
	return false;
    }
    bool operator!=(const Rect& other) const {
      if (m_origin != other.m_origin || m_lr != other.m_lr)
	return true;
      else
	return false;
    }
  protected:
    virtual void dimensions_change() { }
  private:
    Point<T> m_origin, m_lr;
  };

};

#endif
