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
  void thin_zs_get(size_t y, size_t x, const T& image, unsigned char& p,
		   size_t& N, size_t& S) {
    size_t y_before = (y == 0) ? 1 : y - 1;
    size_t x_before = (x == 0) ? 1 : x - 1;
    size_t y_after = (y == image.nrows() - 1) ? image.nrows() - 2 : y + 1;
    size_t x_after = (x == image.ncols() - 1) ? image.ncols() - 2 : x + 1;

    size_t a,b,c,d,e,f,g,h;

    a = is_black(image.get(y_before, x)) ? 2 : 0;
    b = is_black(image.get(y_before, x_after)) ? 4 : 0;
    c = is_black(image.get(y, x_after)) ? 8 : 0;
    d = is_black(image.get(y_after, x_after)) ? 16 : 0;
    e = is_black(image.get(y_after, x)) ? 32 : 0;
    f = is_black(image.get(y_after, x_before)) ? 64 : 0;
    g = is_black(image.get(y, x_before)) ? 128 : 0;
    h = is_black(image.get(y_before, x_before)) ? 256 : 0;

    p = a | b | c | d | e | f | g | h;

    N = 0;
    S = 0;
    bool prev;
    if (p & (1 << 7)) {
      prev = true;
    } else {
      prev = false;
    }
    for (unsigned char p_copy = p; p_copy; p_copy >>= 1) {
      if (p_copy & 1) {
	++N;
	S += !prev;
      }
      prev = p_copy & 1;
    }
  }

  template<class T>
  void thin_zs_flag_bp1(const T& thin, T& flag) {
    register unsigned char p;
    size_t N, S;
    for (size_t y = 0; y < thin.nrows(); ++y)
      for (size_t x = 0; x < thin.ncols(); ++x) {
	thin_zs_get(y, x, thin, p, N, S);
	if ((N <= 6) && (N >= 2) &&
	    (S == 1) &&
	    !((p & 21) == 21) && // 00010101
	    !((p & 84) == 84))   // 01010100
	  flag.set(y, x, black(flag));
	else
	  flag.set(y, x, white(flag));
      }
  }
  
  template<class T>
  void thin_zs_flag_bp2(const T& thin, T& flag) {
    register unsigned char p;
    size_t N, S;
    for (size_t y = 0; y < thin.nrows(); ++y)
      for (size_t x = 0; x < thin.ncols(); ++x) {
	thin_zs_get(y, x, thin, p, N, S);
	if ((N <= 6) && (N >= 2) &&
	    (S == 1) &&
	    !((p & 69) == 69) && // 01000101
	    !((p & 81) == 81))   // 01010001
	  flag.set(y, x, black(flag));
	else
	  flag.set(y, x, white(flag));
      }
  }
    
  template<class T>
  bool thin_zs_del_fbp(T& thin, const T& flag) {
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
    typedef typename ImageFactory<T>::data_type data_type;
    typedef typename ImageFactory<T>::view_type view_type;
    data_type* thin_data = new data_type(in.size(), in.offset_y(), in.offset_x());
    view_type* thin_view = new view_type(*thin_data);
    image_copy_fill(in, *thin_view);
    if (in.nrows() == 1 || in.ncols() == 1) {
      return thin_view;
    }
    data_type* flag_data = new data_type(in.size(), in.offset_y(), in.offset_x());
    view_type* flag_view = new view_type(*flag_data);
    
    bool deleted = true;
    while (deleted) {
      thin_zs_flag_bp1(*thin_view, *flag_view);
      deleted = thin_zs_del_fbp(*thin_view, *flag_view);
      thin_zs_flag_bp2(*thin_view, *flag_view);
      deleted = deleted || thin_zs_del_fbp(*thin_view, *flag_view);
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

  static bool thin_hs_elements[16][3][3]=
    {{{true, true, true}, {  false, true,   false}, {  false,   false,   false}}, /*J1*/
     {{  false,   false,   false}, {  false,   false,   false}, {true, true, true}}, /*K1*/
     
     {{  false, true,   false}, {  false, true, true}, {  false,   false,   false}}, /*J2*/
     {{  false,   false,   false}, {true,   false,   false}, {true, true,   false}}, /*K2*/
     
     {{true,   false,   false}, {true, true,   false}, {true,   false,   false}}, /*J3*/
     {{  false,   false, true}, {  false,   false, true}, {  false,   false, true}}, /*K3*/
     
     {{  false, true,   false}, {true, true,   false}, {  false,   false,   false}}, /*J4*/
     {{  false,   false,   false}, {  false,   false, true}, {  false, true, true}}, /*K4*/
     
     {{  false,   false, true}, {  false, true, true}, {  false,   false, true}}, /*J5*/
     {{true,   false,   false}, {true,   false,   false}, {true,   false,   false}}, /*K5*/
     
     {{  false,   false,   false}, {true, true,   false}, {  false, true,   false}}, /*J6*/
     {{  false, true, true}, {  false,   false, true}, {  false,   false,   false}}, /*K6*/
     
     {{  false,   false,   false}, {  false, true,   false}, {true, true, true}}, /*J7*/
     {{true, true, true}, {  false,   false,   false}, {  false,   false,   false}}, /*K7*/
     
     {{  false,   false,   false}, {  false, true, true}, {  false, true,   false}}, /*J8*/
     {{true, true,   false}, {true,   false,   false}, {  false,   false,   false}}};/*K8*/

  template<class T>
  void thin_hs_diff_image(T& in, const T& other) {
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
  bool thin_hs_hit_and_miss(const T& in, T& H_M, size_t j, size_t k) {
    bool hit_flag, miss_flag, flag;
    size_t nrows = in.nrows() - 1;
    size_t ncols = in.ncols() - 1;

    /* HIT operation */
    flag = true;
    typename T::vec_iterator H_M_it = H_M.vec_begin();
    for (size_t r = 0; r < in.nrows(); ++r)
      for (size_t c = 0; c < in.ncols(); ++c, ++H_M_it) {
	hit_flag = true;
	const size_t l_start = r == 0;
	const size_t l_end = 3 - (r == nrows);
	const size_t m_start = c == 0;
	const size_t m_end = 3 - (c == ncols);
	for (size_t l = l_start; l < l_end; ++l) 
	  for (size_t m = m_start; m < m_end; ++m)
	    if (thin_hs_elements[j][l][m] &&
		is_white(in.get(r + l - 1, c + m - 1)))
	      hit_flag = false;

	miss_flag = true;
	for (size_t l = l_start; l < l_end; ++l) 
	  for (size_t m = m_start; m < m_end; ++m)
	    if (thin_hs_elements[k][l][m] &&
		is_black(in.get(r + l - 1, c + m - 1)))
	      miss_flag = false;
	
	if (hit_flag && miss_flag) {
	  *H_M_it = black(H_M);
	  flag = false; 
	} else
	  *H_M_it = white(H_M);
      }

    return flag;
  }

  template<class T>
  bool thin_hs_one_pass(T& in, T& H_M) {
    bool update_flag = false;
    for (size_t i = 0; i < 8; ++i) {
      size_t j = i * 2;
      size_t k = j + 1;
      bool result = !thin_hs_hit_and_miss(in, H_M, j, k);
      if (result) {
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
    data_type* thin_data = new data_type(in.size(), in.offset_y(), in.offset_x());
    view_type* thin_view = new view_type(*thin_data);
    image_copy_fill(in, *thin_view);
    if (in.nrows() == 1 || in.ncols() == 1)
      return thin_view;
    data_type* H_M_data = new data_type(in.size(), in.offset_y(), in.offset_x());
    view_type* H_M_view = new view_type(*H_M_data);
    bool not_finished = true;
    while (not_finished)
      not_finished = thin_hs_one_pass(*thin_view, *H_M_view);
    delete H_M_view;
    delete H_M_data;
    return thin_view;
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

static bool thin_lc_look_up[16][16]= 
  {{false, false, false, false, false, true, false, false, false, false, false, false, false, true, false, false}, /* 0 */ 
   {false, false, false, false, true, false, true, true, false, false, false, false, false, true, false, false}, /* 1 */ 
   {false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false}, /* 2 */ 
   {false, false, false, false, true, true, true, true, false, false, false, false, false, true, false, false}, /* 3 */ 
   {false, true, false, true, false, false, false, true, false, false, false, false, false, true, false, true}, /* 4 */ 
   {true, false, false, true, false, false, true, false, true, true, false, true, true, false, true, false}, /* 5 */ 
   {false, true, false, true, false, true, false, true, false, false, false, false, false, true, false, true}, /* 6 */ 
   {false, true, false, true, true, false, true, false, false, true, false, true, false, false, false, false}, /* 7 */ 
   {false, false, false, false, false, true, false, false, false, false, false, false, false, true, false, false}, /* 8 */ 
   {false, false, false, false, false, true, false, true, false, false, false, false, false, true, false, false}, /* 9 */ 
   {false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false}, /* A */ 
   {false, false, false, false, false, true, false, true, false, false, false, false, false, true, false, true}, /* B */ 
   {false, false, false, false, false, true, false, false, false, false, false, false, false, true, false, false}, /* C */
   {true, true, false, true, true, false, true, false, true, true, false, true, true, false, true, false}, /* D */ 
   {false, false, false, false, false, true, false, false, false, false, false, false, false, true, false, true}, /* E */ 
   {false, false, false, false, true, false, true, false, false, false, false, true, false, false, true, false}};/* F */

  template<class T>
  typename ImageFactory<T>::view_type* thin_lc(const T& in) {
    typedef typename ImageFactory<T>::data_type data_type;
    typedef typename ImageFactory<T>::view_type view_type;
    view_type* thin_view = thin_zs(in);
    if (in.nrows() == 1 || in.ncols() == 1) {
      image_copy_fill(in, *thin_view);
      return thin_view;
    }
    size_t nrows = thin_view->nrows();
    size_t ncols = thin_view->ncols();
    typename view_type::vec_iterator it = thin_view->vec_begin();
    for (size_t y = 0; y < nrows; ++y)
      for (size_t x = 0; x < ncols; ++x, ++it) {
	if (is_black(*it)) {
	  size_t y_before = (y == 0) ? 1 : y - 1;
	  size_t x_before = (x == 0) ? 1 : x - 1;
	  size_t y_after = (y == nrows - 1) ? nrows - 2 : y + 1;
	  size_t x_after = (x == ncols - 1) ? ncols - 2 : x + 1;

	  size_t a, b, c, d;
	  
	  a = is_black(thin_view->get(y_before, x)) ? 1 : 0;
	  b = is_black(thin_view->get(y_before, x_after)) ? 2 : 0;
	  c = is_black(thin_view->get(y, x_after)) ? 4 : 0;
	  d = is_black(thin_view->get(y_after, x_after)) ? 8 : 0;

	  size_t j = a | b | c | d;

	  a = is_black(thin_view->get(y_after, x)) ? 1 : 0;
	  b = is_black(thin_view->get(y_after, x_before)) ? 2 : 0;
	  c = is_black(thin_view->get(y, x_before)) ? 4 : 0;
	  d = is_black(thin_view->get(y_before, x_before)) ? 8 : 0;

	  size_t i = a | b | c | d;

	  if (thin_lc_look_up[i][j])
	    *it = white(*thin_view);
	}
      }
    return thin_view;
  }

}
#endif
