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

#ifndef kwm01262002_region
#define kwm01262002_region

#include "dimensions.hpp"
#include "gamera_limits.hpp"
#include <list>
#include <map>
#include <algorithm>
#include <exception>
#include <string>

#include <iostream>

/*
  REGIONS

  Regions are designed to be a generic mapping between a region on an image
  (as represented by a rectangle) and some values (represented as a key
  value pair (string/double)). By convention regions should contain a key
  called "scaling" that returns the default value for that region. This is
  used in feature functions that are dependant on some relative value (i.e.
  the height of glyphs in music recognition is scaled by the size of the 
  staff space + the size of the staff line).

  REGIONMAP

  A regionmap is a list of regions with a function to add regions and a
  function to lookup regions based on position.
*/

namespace Gamera {

  template<class V>
  class RegionTemplate : public Rect {
  public:
    typedef std::map<std::string, V> map_type;
    RegionTemplate(size_t origin_y = 0, size_t origin_x = 0,
		   size_t nrows = 1, size_t ncols = 1) :
      Rect(origin_y, origin_x, nrows, ncols), m_value_map() { }
    RegionTemplate(const Point& ul, const Point& lr) :
      Rect(ul, lr) { }
    RegionTemplate(const Rect& r) :
      Rect(r) { }
    RegionTemplate(const Point& ul, const Size& size)
      : Rect(ul, size) {}
    RegionTemplate(const Point& ul, const Dimensions& dim)
      : Rect(ul, dim) {}
    V get(const std::string& key) const {
      typename map_type::const_iterator i = m_value_map.find(key);
      if (i != m_value_map.end())
	return i->second;
      else
	throw std::invalid_argument("Key does not exist");
    }
    void add(const std::string& key, V x) {
      m_value_map[key] = x;
    }
  private:
    map_type m_value_map;
  };
  
  namespace region_detail {
    template<class T> struct intersect {
      intersect(const T& v) : m_rect(v) {}
      inline bool operator()(const T& other) {
	return other.contains_rect(m_rect);
      }
      T m_rect;
    };

    // check to see if b is aligned vertically with a
    template<class T, class U>
    inline bool vertically_intersected(const T& a, const U& b) {
      if ((b.ul_x() >= a.ul_x() && b.ul_x() <= a.lr_x())
	  || (b.lr_x() >= a.ul_x() && b.lr_x() <= a.lr_x()))
	return true;
      else
	return false;
    }

    // distance from the top of a to the bottom of b
    template<class T, class U>
    inline int distance_above(const T& a, const U& b) {
      // check that b really is above a
      if (b.lr_y() >= a.ul_y())
	return b.lr_y() - a.ul_y();
      else
	return -int(a.ul_y() - b.lr_y());
    }

    // distance from the bottom of a to the top of b
    template<class T, class U>
    inline int distance_below(const T& a, const U& b) {
      // check that b really is below a
      if (b.ul_y() <= a.lr_y())
	return a.lr_y() - b.ul_y();
      else
	return -(int)(b.ul_y() - a.lr_y());
    }
  }

  template<class T>
  class RegionMapTemplate : public std::list<RegionTemplate<T> > {
  public:
    using std::list<RegionTemplate<T> >::begin;
    using std::list<RegionTemplate<T> >::end;

    typedef RegionMapTemplate self;
    typedef RegionTemplate<T> region_type;
    typedef Rect rect_t;
    RegionMapTemplate() : std::list<region_type>(0) { }
    virtual ~RegionMapTemplate() { }
    void add_region(const region_type& x) {
      push_back(x);
    }
    virtual region_type lookup(const rect_t& r) {
      typename self::iterator answer =
	std::find_if(begin(), end(), region_detail::intersect<rect_t>(r));
      if (answer != end())
	return *answer;
      else {
	// if we weren't contained in the rectangle we need to find the closest
	// by going straight up and down
	typename self::iterator closest = begin();
	typename self::iterator i = begin();
	int closest_distance = std::numeric_limits<int>::max();
	for (; i != end(); ++i) {
	  if (region_detail::vertically_intersected(r, *i)) {
	    // get the distance above
	    int current_distance = region_detail::distance_above(r, *i);
	    // if we aren't really above, get the distance below. Because we
	    // know that the rectangles don't intersect, these cases really
	    // are exclusive
	    if (current_distance < 0) {
	      current_distance = region_detail::distance_below(r, *i);
	    }
	    if (current_distance < closest_distance)
	      closest = i;
	  }
	}
	return *closest;
      }
    }
  };

}
#endif

