/*
 *
 * Copyright (C) 2001-2002 Ichiro Fujinaga, Michael Droettboom,
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
  void thin_zs_get(size_t y, size_t x, T& image, bool* p, size_t& N, size_t& S) {
    size_t y_before = (y == 0) ? 1 : y - 1;
    size_t x_before = (x == 0) ? 1 : x - 1;
    size_t y_after = (y == image.nrows() - 1) ? image.nrows() - 2 : y + 1;
    size_t x_after = (x == image.ncols() - 1) ? image.ncols() - 2 : x + 1;
    p[0] = (is_black(image.get(y_before, x)));
    p[1] = (is_black(image.get(y_before, x_after)));
    p[2] = (is_black(image.get(y, x_after)));
    p[3] = (is_black(image.get(y_after, x_after)));
    p[4] = (is_black(image.get(y_after, x)));
    p[5] = (is_black(image.get(y_after, x_before)));
    p[6] = (is_black(image.get(y, x_before)));
    p[7] = (is_black(image.get(y_before, x_before)));
    N = 0;
    S = 0;
    bool prev = p[7];
    for (size_t i = 0; i < 8; i++) {
      if (p[i]) {
	++N;
	if (!prev)
	  ++S;
      }
      prev = p[i];
    }
  }

  template<class T>
  void thin_zs_flag_bp1(T& thin, T& flag) {
    bool p[8];
    size_t N, S;
    for (size_t y = 0; y < thin.nrows(); ++y)
      for (size_t x = 0; x < thin.ncols(); ++x) {
	thin_zs_get(y, x, thin, p, N, S);
	if ((N <= 6) && (N >= 2) &&
	    (S == 1) &&
	    !(p[0] && p[2] && p[4]) &&
	    !(p[2] && p[4] && p[6]))
	  flag.set(y, x, black(flag));
	else
	  flag.set(y, x, white(flag));
      }
  }
  
  template<class T>
  void thin_zs_flag_bp2(T& thin, T& flag) {
    bool p[8];
    size_t N, S;
    for (size_t y = 0; y < thin.nrows(); ++y)
      for (size_t x = 0; x < thin.ncols(); ++x) {
	thin_zs_get(y, x, thin, p, N, S);
	if ((N <= 6) && (N >= 2) &&
	    (S == 1) &&
	    !(p[0] && p[2] && p[6]) &&
	    !(p[0] && p[4] && p[6]))
	  flag.set(y, x, black(flag));
	else
	  flag.set(y, x, white(flag));
      }
  }
    
  template<class T>
  bool thin_zs_del_fbp(T& thin, T& flag) {
    bool deleted = false;
    for (size_t y = 0; y < thin.nrows(); ++y) 
      for (size_t x = 0; x < thin.ncols(); ++x) {
	if ((is_black(flag.get(y, x))) && (is_black(thin.get(y, x)))) {
	  thin.set(y, x, white(thin));
	  deleted = true;
	}
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
  void thin_hs_diff_image(T& in, T& other) {
    bool temp;
    for (size_t r = 0; r < in.nrows(); ++r)
      for (size_t c = 0; c < in.ncols(); ++c) {
	temp = is_black(in.get(r, c)) ^ is_black(other.get(r, c));
	if (temp) 
	  in.set(r, c, black(in));
        else
	  in.set(r, c, white(in));
      }
  }

  template<class T>
  bool thin_hs_hit_and_miss(T& in, T& H_M, size_t j, size_t k) {
    bool hit_flag, miss_flag, flag;
    Rect rect(0, 0, in.nrows(), in.ncols());

    /* HIT operation */
    flag = true;
    for (size_t r = 0; r < in.nrows(); ++r)
      for (size_t c = 0; c < in.ncols(); ++c) {
	hit_flag = true;
	for (size_t l = 0; l < 3; ++l) 
	  for (size_t m = 0; m < 3; ++m) {
	    if (thin_hs_elements[j][l][m] &&
		rect.contains_point(Point(c + m - 1, r + l - 1)))
	      if (is_white(in.get(r + l - 1, c + m - 1)))
		hit_flag = false;
	  }

	miss_flag = true;
	for (size_t l = 0; l < 3; ++l) 
	  for (size_t m = 0; m < 3; ++m) {
	    if (thin_hs_elements[k][l][m] &&
		rect.contains_point(Point(c + m - 1, r + l - 1)))
	      if (is_black(in.get(r + l - 1, c + m - 1)))
		miss_flag = false;
	  }
	
	if (hit_flag && miss_flag) {
	  H_M.set(r, c, black(H_M));
	  flag = false; 
	} else
	  H_M.set(r, c, white(H_M));
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
    data_type* H_M_data = new data_type(in.size(), in.offset_y(), in.offset_x());
    view_type* H_M_view = new view_type(*H_M_data);
    bool not_finished = true;
    while (not_finished) {
      not_finished = thin_hs_one_pass(*thin_view, *H_M_view);
    }
    delete H_M_view;
    delete H_M_data;
    return thin_view;
  }


  /* THINNING (Lee and Chen algorithm)
     
  BASED on code in Xite.
  
  This version takes much less memory (only requires two buffers vs. five).

  &[1]H.-J. Lee and B. Chen,
  "Recognition of handwritten chinese characters via short
  line segments,"
  Pattern Recognition,
  vol. 25, no. 5, pp. 543-552, 1992.

  Original authors:
  Øivind Due Trier, late one night at Michigan State University.
  */

static bool thin_lc_look_up[16][16]= 
  {{true, true, true, true, true, false, true, true, true, true, true, true, true, false, true, true}, /* 0 */ 
   {true, true, true, true, false, true, false, false, true, true, true, true, true, false, true, true}, /* 1 */ 
   {true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true}, /* 2 */ 
   {true, true, true, true, false, false, false, false, true, true, true, true, true, false, true, true}, /* 3 */ 
   {true, false, true, false, true, true, true, false, true, true, true, true, true, false, true, false}, /* 4 */ 
   {false, true, true, false, true, true, false, true, false, false, true, false, false, true, false, true}, /* 5 */ 
   {true, false, true, false, true, false, true, false, true, true, true, true, true, false, true, false}, /* 6 */ 
   {true, false, true, false, false, true, false, true, true, false, true, false, true, true, true, true}, /* 7 */ 
   {true, true, true, true, true, false, true, true, true, true, true, true, true, false, true, true}, /* 8 */ 
   {true, true, true, true, true, false, true, false, true, true, true, true, true, false, true, true}, /* 9 */ 
   {true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true}, /* A */ 
   {true, true, true, true, true, false, true, false, true, true, true, true, true, false, true, false}, /* B */ 
   {true, true, true, true, true, false, true, true, true, true, true, true, true, false, true, true}, /* C */
   {false, false, true, false, false, true, false, true, false, false, true, false, false, true, false, true}, /* D */ 
   {true, true, true, true, true, false, true, true, true, true, true, true, true, false, true, false}, /* E */ 
   {true, true, true, true, false, true, false, true, true, true, true, false, true, true, false, true}};/* F */

  template<class T>
  typename ImageFactory<T>::view_type* thin_lc(const T& in) {
    typedef typename ImageFactory<T>::data_type data_type;
    typedef typename ImageFactory<T>::view_type view_type;
    view_type* thin_view = thin_zs(in);
    for (size_t y = 0; y < thin_view->nrows(); ++y)
      for (size_t x = 0; x < thin_view->ncols(); ++x) {
	if (is_black(thin_view->get(y, x))) {
	  size_t y_before = (y == 0) ? 1 : y - 1;
	  size_t x_before = (x == 0) ? 1 : x - 1;
	  size_t y_after = (y == thin_view->nrows() - 1) ? thin_view->nrows() - 2 : y + 1;
	  size_t x_after = (x == thin_view->ncols() - 1) ? thin_view->ncols() - 2 : x + 1;
	  size_t j = 0;
	  j += (is_black(thin_view->get(y_before, x))) << 0;
	  j += (is_black(thin_view->get(y_before, x_after))) << 1;
	  j += (is_black(thin_view->get(y, x_after))) << 2;
	  j += (is_black(thin_view->get(y_after, x_after))) << 3;
	  size_t i = 0;
	  i += (is_black(thin_view->get(y_after, x))) << 0;
	  i += (is_black(thin_view->get(y_after, x_before))) << 1;
	  i += (is_black(thin_view->get(y, x_before))) << 2;
	  i += (is_black(thin_view->get(y_before, x_before))) << 3;
	  if (!thin_lc_look_up[i][j])
	    thin_view->set(y, x, white(*thin_view));
	}
      }
    return thin_view;
  }

}
#endif
