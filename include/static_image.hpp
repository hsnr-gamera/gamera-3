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



#ifndef kwm01102002_static_image

#define kwm01102002_static_image



/*

  This is a convenience class that encapsulates a view

  and image data. KWM

 */



#include "image_view.hpp"

#include "image_data.hpp"



namespace Gamera {



  template<class T>

  class StaticImage : public ImageView<ImageData<T> > {

  protected:

    using ImageView<ImageData<T> >::calculate_iterators;

    using ImageView<ImageData<T> >::range_check;



  public:

    using ImageView<ImageData<T> >::nrows;

    using ImageView<ImageData<T> >::ncols;



    typedef Rect rect_type;

    typedef Size size_type;

    typedef Point point_type;

    typedef Dimensions dimensions_type;

    typedef StaticImage self;

    typedef ImageView<ImageData<T> > view_type;

    /*

      The only difference between this class and the standard

      ImageView is that there is a member for the data. These

      constructors call the base class constructor (passing false

      to suppress range checking because the data is not correctly

      sized when the base class is constructed - we cannot override this

      order because the base class is always constructed before the members

      are initialized - sigh), size the data,

      do a range check, and then calculate the iterators. To resize

      the data when the size of the view changes, it is only necessary

      to override the dimensions_changed method (see below).

    */

    StaticImage(size_t rows = 1, size_t cols = 1)

      : view_type(m_data, 0, 0, rows, cols, false) {

      m_data.dimensions(nrows(), ncols());

      range_check();

      calculate_iterators();

    }

    StaticImage(const point_type& lower_right)

      : view_type(m_data, point_type(), lower_right, false) {

      m_data.dimensions(nrows(), ncols());

      range_check();

      calculate_iterators();

    }

    StaticImage(const size_type& size)

      : view_type(m_data, point_type(), size, false) {

      m_data.dimensions(nrows(), ncols());

      range_check();

      calculate_iterators();

    }

    StaticImage(const dimensions_type& dim)

      : view_type(m_data, point_type(), dim, false) {

      m_data.dimensions(nrows(), ncols());

      range_check();

      calculate_iterators();

    }

  protected:

    virtual void dimensions_change() {

      m_data.dimensions(nrows(), ncols());

      range_check();

      calculate_iterators();      

    }

  private:

    ImageData<T> m_data;

  };



};



#endif
