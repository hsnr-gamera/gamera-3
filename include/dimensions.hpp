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

#ifndef kwm11162001_dimensions_hpp
#define kwm11162001_dimensions_hpp

#include <vector>
#include <iostream>

/*
 * This file contains the basic geometric types for Gamera. On one hand it is
 * absolutely silly that we have to implement these types of objects instead
 * of using something from the C++ 'standard'. On the other hand, some of these
 * have methods that are somewhat specific to document recognition.
 */

namespace Gamera {

  /**
   * The basic type used for all coordinates. An unsigned type is used - this can
   * be inconvenient at times, but it makes interaction with the image processing
   * layer a little easier.
   */
  typedef size_t coord_t;

  /**
   * Point
   *
   * This is a simple class to hold a single coordinate on a plane (x/y value pair).
   */
  class Point {
  public:
    /**
     * Default constructor - x and y are 0
     */
    Point() : m_x(0), m_y(0) { }

    /**
     * Construct a point for the given x and y coordinates.
     */
    Point(coord_t x, coord_t y) {
      m_x = x;
      m_y = y;
    }

    /// Return the x coordinate.
    coord_t x() const {
      return m_x;
    }

    /// Return the y coordinate.
    coord_t y() const {
      return m_y;
    }

    /// Set the x coordinate.
    void x(coord_t x) {
      m_x = x;
    }

    /// Set the y coordinate.
    void y(coord_t y) {
      m_y = y;
    }
    
    /// Move this point the the coordinates x and y
    void move(int x, int y) {
      m_x += x;
      m_y += y;
    }

    /// Equality operator
    bool operator==(const Point& x) const {
      if (m_x == x.m_x && m_y == x.m_y)
        return true;
      else
        return false;
    }

    /// Inequality operator
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
   * A single point Point(0,0) and Size(0, 0) or Dimensions(1, 1)
   */

  /*
   * Size
   *
   * A simple class that holds width and height. These dimensions are
   * refer to nrows - 1 or ncols - 1.
   */
  class Size {
  public:
    /// Default constructor - set width and height to 0
    Size() : m_width(1), m_height(1) { }
    
    /// Construct a size object from width and height.
    Size(coord_t width, coord_t height) {
      m_width = width;
      m_height = height;
    }

    /// Return the width
    coord_t width() const {
      return m_width;
    }

    /// Return the height
    coord_t height() const {
      return m_height;
    }

    /// Set the width
    void width(coord_t width) {
      m_width = width;
    }

    /// Set the height
    void height(coord_t height) {
      m_height = height;
    }

    /// Equality operator
    bool operator==(const Size& other) const {
      if (m_width == other.width() && m_height == other.height())
	return true;
      else
	return false;
    }
    
    /// Inequality operator
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
    Point center() const { return Point(center_x(), center_y()); }
    coord_t center_x() const { return m_origin.x() + width() / 2; }
    coord_t center_y() const { return m_origin.y() + height() / 2; }
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
    // containment
    bool contains_x(coord_t v) const {
      if (v >= ul_x() && v <= lr_x())
	return true;
      else
	return false;
    }
    bool contains_y(coord_t v) const {
      if (v >= ul_y() && v <= lr_y())
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
    
    Rect expand(size_t expansion) const {
      return Rect(size_t(std::max((long)ul_y() - (long)expansion, 0l)),
		  size_t(std::max((long)ul_x() - (long)expansion, 0l)),
		  nrows() + expansion * 2,
		  ncols() + expansion * 2);
    }

    // intersection
    bool intersects_x(const self& v) const {
      coord_t sul = ul_x();
      coord_t slr = lr_x();
      coord_t vul = v.ul_x();
      coord_t vlr = v.lr_x();
      return (((vul >= sul) && (vul <= slr)) ||
	      ((vlr >= sul) && (vlr <= slr)) ||
	      ((sul >= vul) && (sul <= vlr)) ||
	      ((slr >= vul) && (slr <= vlr)));
    }
    bool intersects_y(const self& v) const {
      coord_t sul = ul_y();
      coord_t slr = lr_y();
      coord_t vul = v.ul_y();
      coord_t vlr = v.lr_y();
      return (((vul >= sul) && (vul <= slr)) ||
	      ((vlr >= sul) && (vlr <= slr)) ||
	      ((sul >= vul) && (sul <= vlr)) ||
	      ((slr >= vul) && (slr <= vlr)));
    }
    bool intersects(const self& v) const {
      return (intersects_x(v) && intersects_y(v));
    }

    Rect intersection(const self& other) const {
      size_t ulx = std::max(ul_x(), other.ul_x());
      size_t uly = std::max(ul_y(), other.ul_y());
      size_t lrx = std::min(lr_x(), other.lr_x());
      size_t lry = std::min(lr_y(), other.lr_y());
      return Rect(Point(ulx, uly), Point(lrx, lry));
    }

    // Equality
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

    // union
    static Rect* union_rects(std::vector<Rect*> &rects) {
      size_t min_x, min_y, max_x, max_y;
      min_x = min_y = std::numeric_limits<size_t>::max();
      max_x = max_y = 0;

      for (std::vector<Rect*>::iterator i = rects.begin();
	   i != rects.end(); ++i) {
	Rect* rect = (*i);
	min_x = std::min(min_x, rect->ul_x());
	min_y = std::min(min_y, rect->ul_y());
	max_x = std::max(max_x, rect->lr_x());
	max_y = std::max(max_y, rect->lr_y());
      }
      return new Rect(Point(min_x, min_y), Point(max_x, max_y));
    }

    void union_rect(const self& other) {
      ul_y(std::min(ul_y(), other.ul_y()));
      lr_y(std::max(lr_y(), other.lr_y()));
      ul_x(std::min(ul_x(), other.ul_x()));
      lr_x(std::max(lr_x(), other.lr_x()));
    }

    // distance
    double distance_euclid(const self& other) {
      return euclid(center_x(), center_y(), other.center_x(), other.center_y());
    }
    coord_t distance_cx(const self& other) {
      coord_t cx = center_x();
      coord_t other_cx = other.center_x();
      if (cx > other_cx)
	return cx - other_cx;
      else
	return other_cx - cx;
    }
    coord_t distance_cy(const self& other) {
      coord_t cy = center_y();
      coord_t other_cy = other.center_y();
      if (cy > other_cy)
	return cy - other_cy;
      else
	return other_cy - cy;
    }
    double distance_bb(const self& other) {
      double min_y = (double)std::min
	(std::min(abs((long)ul_y() - (long)other.ul_y()),
		  abs((long)ul_y() - (long)other.lr_y())),
	 std::min(abs((long)lr_y() - (long)other.ul_y()),
		  abs((long)lr_y() - (long)other.lr_y())));
      double min_x = (double)std::min
	(std::min(abs((long)ul_x() - (long)other.ul_x()),
		  abs((long)ul_x() - (long)other.lr_x())),
	 std::min(abs((long)lr_x() - (long)other.ul_x()),
		  abs((long)lr_x() - (long)other.lr_x())));
      
      return std::sqrt(min_y*min_y + min_x*min_x);
    }
  private:
    double euclid(coord_t x1, coord_t y1, coord_t x2, coord_t y2) {
      double dx1 = (double)x1;
      double dy1 = (double)y1;
      double dx2 = (double)x2;
      double dy2 = (double)y2;
      double a = std::pow(std::abs(dx1 - dx2), 2);
      double b = std::pow(std::abs(dy1 - dy2), 2);
      return std::sqrt(a + b);
    }

  protected:
    virtual void dimensions_change() { }
  private:
    Point m_origin, m_lr;
  };
};

#endif
