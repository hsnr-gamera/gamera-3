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

  typedef size_t coord_t;
  /*
   * Point
   *
   * This is a simple class to hold a single coordinate (x/y value pair).
   */

  class Point {
  public:
    Point() : m_x(0), m_y(0) { }
    Point(coord_t x, coord_t y) { m_x = x; m_y = y; }
    coord_t x() const { return m_x; }
    coord_t y() const { return m_y; }
    void x(coord_t x) { m_x = x; }
    void y(coord_t y) { m_y = y; }
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
    coord_t m_x, m_y;
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

  class Size {
  public:
    Size() : m_width(1), m_height(1) { }
    Size(coord_t width, coord_t height) {
      m_width = width;
      m_height = height;
    }
    coord_t width() const { return m_width; }
    coord_t height() const { return m_height; }
    void width(coord_t width) { m_width = width; }
    void height(coord_t height) { m_height = height; }
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
    coord_t m_width, m_height;
  };

  /*
   * Dimensions
   *
   * A simple class that holds nrows and ncols. These dimensions are
   * refer to width or height + 1.
   */

  class Dimensions {
  public:
    Dimensions() : m_ncols(1), m_nrows(1) { }
    Dimensions(coord_t rows, coord_t cols) {
      m_ncols = cols;
      m_nrows = rows;
    }
    coord_t ncols() const { return m_ncols; }
    coord_t nrows() const { return m_nrows; }
    void ncols(coord_t ncols) { m_ncols = ncols; }
    void nrows(coord_t nrows) { m_nrows = nrows; }
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
    coord_t m_ncols, m_nrows;
  };

  /*
   * Rect
   *
   * A rectangle class
   */

  class Rect {
  public:
    typedef Rect self;

    Rect() : m_origin(0, 0), m_lr(1, 1) { }
    Rect(coord_t origin_y, coord_t origin_x, coord_t nrows, coord_t ncols)
      : m_origin(origin_x, origin_y),
      	m_lr(origin_x + ncols - 1, origin_y + nrows - 1) { }
    Rect(const Point& upper_left, const Point& lower_right)
      : m_origin(upper_left), m_lr(lower_right) { }
    Rect(const Point& upper_left, const Size& size)
      : m_origin(upper_left), m_lr(upper_left.x() + size.width(),
				   upper_left.y() + size.height()) { }
    Rect(const Point& upper_left, const Dimensions& dim)
      : m_origin(upper_left), m_lr(upper_left.x() + dim.ncols() - 1,
				   upper_left.y() + dim.nrows() - 1) { }
    virtual ~Rect() { }
    // Get
    Point ul() const { return m_origin; }
    coord_t ul_x() const { return m_origin.x(); }
    coord_t ul_y() const { return m_origin.y(); }
    Point ur() const { return Point(m_lr.x(), m_origin.y()); }
    coord_t ur_x() const { return m_lr.x(); }
    coord_t ur_y() const { return m_origin.y(); }
    Point lr() const { return m_lr; }
    coord_t lr_x() const { return m_lr.x(); }
    coord_t lr_y() const { return m_lr.y(); }
    Point ll() const { return Point(m_origin.x(), m_lr.y()); }
    coord_t ll_x() const { return m_origin.x(); }
    coord_t ll_y() const { return m_lr.y(); }
    Dimensions dimensions() const {
      return Dimensions(nrows(), ncols());
    }
    Size size() const { return Size(width(), height()); }
    coord_t ncols() const { return m_lr.x() - m_origin.x() + 1; }
    coord_t nrows() const { return m_lr.y() - m_origin.y() + 1; }
    coord_t width() const { return m_lr.x() - m_origin.x(); }
    coord_t height() const { return m_lr.y() - m_origin.y(); }
    coord_t offset_x() const { return m_origin.x(); }
    coord_t offset_y() const { return m_origin.y(); }
    // Set
    void ul(const Point& ul) { m_origin = ul; dimensions_change(); }
    void ul_x(coord_t v) { m_origin.x(v); dimensions_change(); }
    void ul_y(coord_t v) { m_origin.y(v); dimensions_change(); }
    void ur(const Point& ur) {
      m_lr.x(ur.x()); m_origin.y(ur.y()); dimensions_change();
    }
    void ur_x(coord_t v) { m_lr.x(v); dimensions_change(); }
    void ur_y(coord_t v) { m_origin.y(v); dimensions_change(); }
    void lr(const Point& lr) { m_lr = lr; dimensions_change(); }
    void lr_x(coord_t v) { m_lr.x(v); dimensions_change(); }
    void lr_y(coord_t v) { m_lr.y(v); dimensions_change(); }
    void ll(const Point& ll) {
      m_origin.x(ll.x());
      m_lr.y(ll.y());
      dimensions_change();
    }
    void ll_x(coord_t v) { m_origin.x(v); dimensions_change(); }
    void ll_y(coord_t v) { m_lr.y(v); dimensions_change(); }
    void dimensions(const Dimensions& dim) {
      nrows(dim.nrows());
      ncols(dim.ncols());
      dimensions_change();
    }
    void dimensions(coord_t nrows, coord_t ncols) {
      this->nrows(nrows);
      this->ncols(ncols);
      dimensions_change();
    }
    void ncols(coord_t v) {
      m_lr.x(m_origin.x() + v - 1);
      dimensions_change();
    }
    void nrows(coord_t v) {
      m_lr.y(m_origin.y() + v - 1);
      dimensions_change();
    }
    void size(const Size& size) {
      width(size.width());
      height(size.height());
      dimensions_change();
    }
    void size(coord_t width, coord_t height) {
      this->width(width);
      this->height(height);
      dimensions_change();
    }
    void width(coord_t width) {
      m_lr.x(m_origin.x() + width);
      dimensions_change();
    }
    void height(coord_t height) {
      m_lr.y(m_origin.y() + height);
      dimensions_change();
    }
    void rect_set(coord_t origin_y, coord_t origin_x, coord_t nrows, coord_t ncols) {
      m_origin.x(origin_x);
      m_origin.y(origin_y);
      m_lr.x(origin_x + ncols - 1);
      m_lr.y(origin_y + nrows - 1);
      dimensions_change();
    }
    void rect_set(const Point& upper_left, const Point& lower_right) {
      m_origin = upper_left;
      m_lr = lower_right;
      dimensions_change();
    }
    void rect_set(const Point& upper_left, const Size& size) {
      m_origin = upper_left;
      this->size(size);
      dimensions_change();
    }
    void rect_set(const Point& upper_left, const Dimensions& dim) {
      m_origin = upper_left;
      dimensions(dim);
      dimensions_change();
    }
    void offset_x(coord_t v) { m_origin.x(v); dimensions_change(); }
    void offset_y(coord_t v) { m_origin.y(v); dimensions_change(); }
    void move(int x, int y) {
      m_origin.move(x, y);
      m_lr.move(x, y);
    }
    // intersection
    bool contains_x(coord_t v) const {
      if (v >= m_origin.x() && v <= lr_x())
	return true;
      else
	return false;
    }
    bool contains_y(coord_t v) const {
      if (v >= m_origin.y() && v <= lr_y())
	return true;
      else
	return false;
    }
    bool contains_point(const Point& v) const {
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
    Point m_origin, m_lr;
  };

};

#endif
