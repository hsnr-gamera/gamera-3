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

#ifndef kwm05142002_accessor
#define kwm05142002_accessor

namespace Gamera {

  template<class T>
  class ImageAccessor {
  public:
    typedef T value_type;

    template<class Iterator>
    value_type operator()(const Iterator& i) const {
      return i.get();
    }
    value_type operator()(value_type* i) const {
      return *i;
    }
    value_type operator()(const value_type* i) const {
      return *i;
    }

    template<class Iterator>
    value_type get(const Iterator& i) const {
      return i.get();
    }
    value_type get(value_type* i) const {
      return *i;
    }
    value_type get(const value_type* i) const {
      return *i;
    }

    template<class Iterator>
    void set(const value_type& v, Iterator I) const {
      i.set(v);
    }
    void set(const value_type& v, value_type* i) const {
      *i = v;
    }
  };

}

#endif
