/*
 *
 * Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom,
 * and Karl MacMillan
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

#ifndef kwm11062002_misc_filters
#define kwm11062002_misc_filters

#include "gamera.hpp"
#include "logical.hpp"
#include "morphology.hpp"
#include "image_utilities.hpp"

namespace Gamera {

  /* THINNING (Zhang and Suen algorithm)
     
  &[1] T. Y. Zhang and C. Y. Suen,
  "A Fast Parallel Algorithm for Thinning Digital Patterns.",
  Comm. ACM, vol. 27, no. 3, pp 236-239, 1984.
  
  &[2] Rafael C. Gonzalez and Paul Wintz,
  "Digital Image Processing.",
  2. edition, 1987, pp. 398-402. 
  
  Based on code in Xite

  Original author:
  Øivind Due Trier, Ifi, UiO
  */

  template<class T>
  inline void thin_zs_get(const size_t& y, const size_t& y_before, const size_t& y_after, 
			  const size_t& x, const T& image, unsigned char& p,
			  size_t& N, size_t& S) {
    size_t x_before = (x == 0) ? 1 : x - 1;
    size_t x_after = (x == image.ncols() - 1) ? image.ncols() - 2 : x + 1;

    p = ((is_black(image.get(Point(x_before, y_before))) << 7) |
	 (is_black(image.get(Point(x_before, y))) << 6) |
	 (is_black(image.get(Point(x_before, y_after))) << 5) |
	 (is_black(image.get(Point(x, y_after))) << 4) |
	 (is_black(image.get(Point(x_after, y_after))) << 3) |
	 (is_black(image.get(Point(x_after, y))) << 2) |
	 (is_black(image.get(Point(x_after, y_before))) << 1) |
	 (is_black(image.get(Point(x, y_before)))));

    N = 0;
    S = 0;
    bool prev = p & (1 << 7);
    for (size_t i = 0; i < 8; ++i) {
      if (p & (1 << i)) {
	++N;
	S += !prev;
	prev = true;
      } else
	prev = false;
    }
  }

  template<class T>
  inline void thin_zs_flag(const T& thin, T& flag, const unsigned char a, const unsigned char b) {
    register unsigned char p;
    size_t N, S; 
    for (size_t y = 0; y < thin.nrows(); ++y) {
      size_t y_before = (y == 0) ? 1 : y - 1;
      size_t y_after = (y == thin.nrows() - 1) ? thin.nrows() - 2 : y + 1;
      for (size_t x = 0; x < thin.ncols(); ++x) {
	if (is_black(thin.get(Point(x, y)))) {
	  thin_zs_get(y, y_before, y_after, x, thin, p, N, S);
	  if ((N <= 6) && (N >= 2) &&
	      (S == 1) &&
	      !((p & a) == a) &&
	      !((p & b) == b))  
	    flag.set(Point(x, y), black(flag));
	  else
	    flag.set(Point(x, y), white(flag));
	}
      }
    }
  }
  
  template<class T>
  inline bool thin_zs_del_fbp(T& thin, const T& flag) {
    bool deleted = false;
    typename T::vec_iterator thin_it = thin.vec_begin();
    typename T::const_vec_iterator flag_it = flag.vec_begin();
    for (; thin_it != thin.vec_end(); ++thin_it, ++flag_it)
      if (is_black(*flag_it) && is_black(*thin_it)) {
	(*thin_it) = white(thin);
	deleted = true;
      }
    return deleted;
  }

  template<class T>
  typename ImageFactory<T>::view_type* thin_zs(const T& in) {
    const unsigned char constants[2][2] = {{21, 84}, {69, 81}};

    typedef typename ImageFactory<T>::data_type data_type;
    typedef typename ImageFactory<T>::view_type view_type;
    data_type* thin_data = new data_type(in.size(), in.origin());
    view_type* thin_view = new view_type(*thin_data);
    image_copy_fill(in, *thin_view);
    if (in.nrows() == 1 || in.ncols() == 1) {
      return thin_view;
    }
    
    data_type* flag_data = 0;
    view_type* flag_view = 0;
    try {
      flag_data = new data_type(in.size(), in.origin());
      flag_view = new view_type(*flag_data);
      
      try {
	bool deleted = true;
	bool constant_i = false;
	while (deleted) {
	  thin_zs_flag(*thin_view, *flag_view, 
		       constants[constant_i][0], constants[constant_i][1]);
	  deleted = thin_zs_del_fbp(*thin_view, *flag_view);
	  constant_i = !constant_i;
	}
      } catch (std::exception e) {
	delete flag_view;
	delete flag_data;
	throw;
      }
    } catch (std::exception e) {
      delete thin_data;
      delete thin_view;
      throw;
    }

    delete flag_view;
    delete flag_data;
    return thin_view;
  }

  /* THINNING (Haralick and Shapiro's morphological algorithm)
     
  &R. M. Haralick and L. G. Shapiro,
  "Computer and Robot Vision",
  Vol. 1, Chapter 5 (especially 5.10.1),
  Addison-Wesley, Reading, Massachusetts, 1992,

  BASED on code in Xite.
  
  This version takes much less memory (only requires two buffers vs. five).

  Original authors:
  Qian Huang (Michigan State University), 
  Øivind Due Trier (BLAB, Ifi, University of Oslo).
  */

//   static bool thin_hs_elements[16][3][3]=
//     {{{true, true, true}, {  false, true,   false}, {  false,   false,   false}},    /*J1*/
//      {{  false,   false,   false}, {  false,   false,   false}, {true, true, true}}, /*K1*/
     
//      {{  false, true,   false}, {  false, true, true}, {  false,   false,   false}}, /*J2*/
//      {{  false,   false,   false}, {true,   false,   false}, {true, true,   false}}, /*K2*/
     
//      {{true,   false,   false}, {true, true,   false}, {true,   false,   false}},    /*J3*/
//      {{  false,   false, true}, {  false,   false, true}, {  false,   false, true}}, /*K3*/
     
//      {{  false, true,   false}, {true, true,   false}, {  false,   false,   false}}, /*J4*/
//      {{  false,   false,   false}, {  false,   false, true}, {  false, true, true}}, /*K4*/
     
//      {{  false,   false, true}, {  false, true, true}, {  false,   false, true}},    /*J5*/
//      {{true,   false,   false}, {true,   false,   false}, {true,   false,   false}}, /*K5*/
     
//      {{  false,   false,   false}, {true, true,   false}, {  false, true,   false}}, /*J6*/
//      {{  false, true, true}, {  false,   false, true}, {  false,   false,   false}}, /*K6*/
     
//      {{  false,   false,   false}, {  false, true,   false}, {true, true, true}},    /*J7*/
//      {{true, true, true}, {  false,   false,   false}, {  false,   false,   false}}, /*K7*/
     
//      {{  false,   false,   false}, {  false, true, true}, {  false, true,   false}}, /*J8*/
//      {{true, true,   false}, {true,   false,   false}, {  false,   false,   false}}};/*K8*/

  static unsigned char thin_hs_elements[16][3] = {{0x7, 0x2, 0x0}, {0x0, 0x0, 0x7}, {0x2, 0x6, 0x0}, {0x0, 0x1, 0x3}, {0x1, 0x3, 0x1}, {0x4, 0x4, 0x4}, {0x2, 0x3, 0x0}, {0x0, 0x4, 0x6}, {0x4, 0x6, 0x4}, {0x1, 0x1, 0x1}, {0x0, 0x3, 0x2}, {0x6, 0x4, 0x0}, {0x0, 0x2, 0x7}, {0x7, 0x0, 0x0}, {0x0, 0x6, 0x2}, {0x3, 0x1, 0x0}};

  template<class T>
  inline void thin_hs_diff_image(T& in, const T& other) {
    typename T::vec_iterator in_it = in.vec_begin();
    typename T::const_vec_iterator other_it = other.vec_begin();
    for (; in_it != in.vec_end(); ++in_it, ++other_it) {
      if (is_black(*in_it) ^ is_black(*other_it)) 
	*in_it = black(in);
      else
	*in_it = white(in);
    }
  }

  template<class T>
  inline bool thin_hs_hit_and_miss(const T& in, T& H_M, 
				   const size_t& j, const size_t& k) {
    bool flag;

    /* HIT operation */
    flag = false;
    for (size_t r = 1; r < in.nrows() - 1; ++r) {
      for (size_t c = 1; c < in.ncols() - 1; ++c) {
	for (size_t l = 0; l < 3; ++l) {
	  for (size_t m = 0; m < 3; ++m) {
	    if (is_white(in.get(Point(c + m - 1, r + l - 1)))) {
	      if (thin_hs_elements[j][l] & (1 << m))
		goto remove;
	    } else {
	      if (thin_hs_elements[k][l] & (1 << m))
		goto remove;
	    }
	  }
	}

	H_M.set(Point(c, r), black(H_M));
	flag = true; 
	continue;
      remove:
	H_M.set(Point(c, r), white(H_M));
      }
    }

    return flag;
  }

  template<class T>
  bool thin_hs_one_pass(T& in, T& H_M) {
    bool update_flag = false;
    for (size_t i = 0; i < 8; ++i) {
      size_t j = i * 2;
      size_t k = j + 1;
      if (thin_hs_hit_and_miss(in, H_M, j, k)) {
	thin_hs_diff_image(in, H_M);
	update_flag = true;
      }
    }
    return update_flag;
  }

  template<class T>
  typename ImageFactory<T>::view_type* thin_hs(const T& in) {
    typedef typename ImageFactory<T>::data_type data_type;
    typedef typename ImageFactory<T>::view_type view_type;
    Dim new_size(in.ncols() + 2, in.nrows() + 2);
    bool upper_left_origin = (in.ul_x() == 0) || (in.ul_y() == 0);
    Point new_origin;
    if (upper_left_origin)
      new_origin = Point(0, 0);
    else
      new_origin = Point(in.ul_x() - 1, in.ul_y() - 1);
    data_type* thin_data = new data_type(new_size, new_origin);
    view_type* thin_view = new view_type(*thin_data);
    try {
      for (size_t y = 0; y != in.nrows(); ++y)
	for (size_t x = 0; x != in.ncols(); ++x)
	  thin_view->set(Point(x + 1, y + 1), in.get(Point(x, y)));
      if (in.nrows() == 1 || in.ncols() == 1)
	goto end;
      data_type* H_M_data = new data_type(new_size, new_origin);
      view_type* H_M_view = new view_type(*H_M_data);
      try {
	bool not_finished = true;
	while (not_finished)
	  not_finished = thin_hs_one_pass(*thin_view, *H_M_view);
      } catch (std::exception e) {
	delete H_M_view;
	delete H_M_data;
	throw;
      }
      delete H_M_view;
      delete H_M_data;
    } catch (std::exception e) {
      delete thin_view;
      delete thin_data;
      throw;
    }
  end:
    if (upper_left_origin) {
      data_type* new_data = new data_type(in.size(), in.origin());
      view_type* new_view = new view_type(*new_data);
      for (size_t y = 0; y != in.nrows(); ++y)
	for (size_t x = 0; x != in.ncols(); ++x)
	  new_view->set(Point(x, y), thin_view->get(Point(x + 1, y + 1)));
      delete thin_view;
      delete thin_data;
      return new_view;
    } else {
      delete thin_view;
      thin_view = new view_type(*thin_data, in);
      return thin_view;
    }
  }


  /* THINNING (Lee and Chen algorithm)
     
  BASED on code in Xite.
  
  This version takes much less memory (only requires two buffers vs. five) than
  the Xite implementation.

  &[1]H.-J. Lee and B. Chen,
  "Recognition of handwritten chinese characters via short
  line segments,"
  Pattern Recognition,
  vol. 25, no. 5, pp. 543-552, 1992.

  Original authors:
  Øivind Due Trier, late one night at Michigan State University.
  */

// static bool thin_lc_look_up[16][16]= 
//   {{false, false, false, false, false, true, false, false, false, false, false, false, false, true, false, false}, /* 0 */ 
//    {false, false, false, false, true, false, true, true, false, false, false, false, false, true, false, false}, /* 1 */ 
//    {false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false}, /* 2 */ 
//    {false, false, false, false, true, true, true, true, false, false, false, false, false, true, false, false}, /* 3 */ 
//    {false, true, false, true, false, false, false, true, false, false, false, false, false, true, false, true}, /* 4 */ 
//    {true, false, false, true, false, false, true, false, true, true, false, true, true, false, true, false}, /* 5 */ 
//    {false, true, false, true, false, true, false, true, false, false, false, false, false, true, false, true}, /* 6 */ 
//    {false, true, false, true, true, false, true, false, false, true, false, true, false, false, false, false}, /* 7 */ 
//    {false, false, false, false, false, true, false, false, false, false, false, false, false, true, false, false}, /* 8 */ 
//    {false, false, false, false, false, true, false, true, false, false, false, false, false, true, false, false}, /* 9 */ 
//    {false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false}, /* A */ 
//    {false, false, false, false, false, true, false, true, false, false, false, false, false, true, false, true}, /* B */ 
//    {false, false, false, false, false, true, false, false, false, false, false, false, false, true, false, false}, /* C */
//    {true, true, false, true, true, false, true, false, true, true, false, true, true, false, true, false}, /* D */ 
//    {false, false, false, false, false, true, false, false, false, false, false, false, false, true, false, true}, /* E */ 
//    {false, false, false, false, true, false, true, false, false, false, false, true, false, false, true, false}};/* F */

  static unsigned short thin_lc_look_up[16] = {0x2020, 0x20d0, 0x0,    0x20f0, 
					       0xa08a, 0x5b49, 0xa0aa, 0xa5a, 
					       0x2020, 0x20a0, 0x0,    0xa0a0, 
					       0x2020, 0x5b5b, 0xa020, 0x4850};

  template<class T>
  typename ImageFactory<T>::view_type* thin_lc(const T& in) {
    typedef typename ImageFactory<T>::data_type data_type;
    typedef typename ImageFactory<T>::view_type view_type;

    // Chain to thin_zs
    view_type* thin_view = thin_zs(in);
    if (in.nrows() == 1 || in.ncols() == 1) {
      return thin_view;
    }

    try {
      size_t nrows = thin_view->nrows();
      size_t ncols = thin_view->ncols();
      typename view_type::vec_iterator it = thin_view->vec_begin();
      for (size_t y = 0; y < nrows; ++y) {
	size_t y_before = (y == 0) ? 1 : y - 1;
	size_t y_after = (y == nrows - 1) ? nrows - 2 : y + 1;
	for (size_t x = 0; x < ncols; ++x, ++it) {
	  if (is_black(*it)) {
	    size_t x_before = (x == 0) ? 1 : x - 1;
	    size_t x_after = (x == ncols - 1) ? ncols - 2 : x + 1;
	    
	    size_t j = ((is_black(thin_view->get(Point(x_after, y_after))) << 3) |
			(is_black(thin_view->get(Point(x_after, y))) << 2) |
			(is_black(thin_view->get(Point(x_after, y_before))) << 1) |
			(is_black(thin_view->get(Point(x, y_before)))));
	    
	    size_t i = ((is_black(thin_view->get(Point(x_before, y_before))) << 3) |
			(is_black(thin_view->get(Point(x_before, y))) << 2) |
			(is_black(thin_view->get(Point(x_before, y_after))) << 1) |
			(is_black(thin_view->get(Point(x, y_after)))));
	    
	    if (thin_lc_look_up[i] & (1 << j))
	      *it = white(*thin_view);
	  }
	}
      }
    } catch (std::exception e) {
      delete thin_view->data();
      delete thin_view;
    }
    return thin_view;
  }

}
#endif

