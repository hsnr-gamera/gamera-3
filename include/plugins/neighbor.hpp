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



/* Derived from 

   Klette, R. and P. Zamperoni.  Handbook of Image Processing Operators.

*/



#ifndef kwm12032001_neighbor

#define kwm12032001_neighbor



#include <vector>

#include "gamera.hpp"



namespace Gamera {



/* Steps through the image using a 4x4 window, replacing the center pixel

   with a value determined by a given function.

   This version uses all nine pixels in the 3x3 window:

      ####   0  1  2  3

      ####   4  5  6  7

      ####   8  9 10 11

      ####  12 13 14 15

   The temporary matrix required may be passed in to avoid reallocating for

   each pass of a multi-pass algorithm. (see erode)

   Since there can be no "center" pixel in a 4x4 window, where the center

   is is parameterized (should be templatized).  

   For example, row = 2, col = 1 puts the center at:

      ####

      ####

      #x##

      ####

*/

template<class T, class F, class M>

void neighbor16(const T& m, F& func, M& tmp, 

		size_t center_row = 1, size_t center_col = 1) {

  if (m.nrows() < 4 || m.ncols() < 4)

    return;

  std::vector<typename T::value_type> window(16);



  for (unsigned int row = 0; row < m.nrows() - 4; row++) {

    for (unsigned int col = 0; col < m.ncols() - 4; col++) {

      window[0] = m.get(row, col);

      window[1] = m.get(row, col + 1);

      window[2] = m.get(row, col + 2);

      window[3] = m.get(row, col + 3);

      window[4] = m.get(row + 1, col);

      window[5] = m.get(row + 1, col + 1);

      window[6] = m.get(row + 1, col + 2);

      window[7] = m.get(row + 1, col + 3);

      window[8] = m.get(row + 2, col);

      window[9] = m.get(row + 2, col + 1);

      window[10] = m.get(row + 2, col + 2);

      window[11] = m.get(row + 2, col + 3);

      window[12] = m.get(row + 3, col);

      window[13] = m.get(row + 3, col + 1);

      window[14] = m.get(row + 3, col + 2);

      window[15] = m.get(row + 3, col + 3); 

      tmp.set(row + center_row, 

	      col + center_col,

	      func(window.begin(), window.end()));

    }

  }

}



/* Steps through the image using a 3x3 window, replacing the center pixel

   with a value determined by a given function.

   This version uses all nine pixels in the 3x3 window:

      ###  012

      ###  345

      ###  678

   The temporary matrix required may be passed in to avoid reallocating for

   each pass of a multi-pass algorithm. (see erode)

*/

template<class T, class F, class M>

void neighbor9(const T& m, F func, M& tmp) {

  if (m.nrows() < 3 || m.ncols() < 3)

    return;

  std::vector<typename T::value_type> window(9);



  unsigned int nrows_m1 = m.nrows() - 1;

  unsigned int ncols_m1 = m.ncols() - 1;

  unsigned int nrows_m2 = m.nrows() - 2;

  unsigned int ncols_m2 = m.ncols() - 2;



  // Upper-left

  for (unsigned int i = 0; i < 4; ++i)

    window[i] = white(m);

  window[4] = m.get(0, 0);

  window[5] = m.get(0, 1);

  window[6] = white(m);

  window[7] = m.get(1, 0);

  window[8] = m.get(1, 1);

  tmp.set(0, 0, func(window.begin(), window.end()));

  

  // Upper-right

  window[3] = m.get(0, ncols_m2);

  window[4] = m.get(0, ncols_m1);

  window[6] = m.get(1, ncols_m2);

  window[7] = m.get(1, ncols_m1);

  window[5] = window[8] = white(m);

  tmp.set(0, ncols_m1, func(window.begin(), window.end()));

  

  // Lower-left

  window[1] = m.get(nrows_m2, 0);

  window[2] = m.get(nrows_m2, 1);

  window[4] = m.get(nrows_m1, 0);

  window[5] = m.get(nrows_m1, 1);

  window[6] = white(m);

  window[3] = window[7] = white(m);

  tmp.set(nrows_m1, 0, func(window.begin(), window.end()));



  // Lower-right

  window[0] = m.get(nrows_m2, ncols_m2);

  window[1] = m.get(nrows_m2, ncols_m1);

  window[3] = m.get(nrows_m1, ncols_m2);

  window[4] = m.get(nrows_m1, ncols_m1);

  window[2] = window[4] = white(m);

  tmp.set(nrows_m1, ncols_m1, func(window.begin(), window.end()));



  // Top edge

  window[0] = window[1] = white(m);

  for (unsigned int col = 1; col < ncols_m1; col++) {

    window[3] = m.get(0, col - 1);

    window[4] = m.get(0, col);

    window[5] = m.get(0, col + 1);

    window[6] = m.get(1, col - 1);

    window[7] = m.get(1, col);

    window[8] = m.get(1, col + 1);

    tmp.set(0, col, func(window.begin(), window.end()));

  }



  // Bottom edge

  window[6] = window[7] = window[8] = white(m);

  for (unsigned int col = 1; col < ncols_m1; col++) {

    window[0] = m.get(nrows_m2, col - 1);

    window[1] = m.get(nrows_m2, col);

    window[2] = m.get(nrows_m2, col + 1);

    window[3] = m.get(nrows_m1, col - 1);

    window[4] = m.get(nrows_m1, col);

    window[5] = m.get(nrows_m1, col + 1);

    tmp.set(nrows_m1, col, func(window.begin(), window.end()));

  }



  // Left edge

  window[0] = window[3] = white(m);

  for (unsigned int row = 1; row < nrows_m1; row++) {

    window[1] = m.get(row - 1, 0);

    window[2] = m.get(row - 1, 1);

    window[4] = m.get(row, 0);

    window[5] = m.get(row, 1);

    window[7] = m.get(row + 1, 0);

    window[8] = m.get(row + 1, 1);

    tmp.set(row, 0, func(window.begin(), window.end()));

  }



  // Right edge

  window[2] = window[5] = window[8] = white(m);

  for (unsigned int row = 1; row < nrows_m1; row++) {

    window[0] = m.get(row - 1, ncols_m2);

    window[1] = m.get(row - 1, ncols_m1);

    window[3] = m.get(row, ncols_m2);

    window[4] = m.get(row, ncols_m1);

    window[6] = m.get(row + 1, ncols_m2);

    window[7] = m.get(row + 1, ncols_m1);

    tmp.set(row, ncols_m1, func(window.begin(), window.end()));

  }

  

  // Core of image

  for (int row = 1; row < int(nrows_m1); ++row) {

    for (int col = 1; col < int(ncols_m1); ++col) {

      // This may seem silly, but it's significantly faster than using

      // nine iterators

      typename std::vector<typename T::value_type>::iterator window_it = window.begin();

      for (int ri = -1; ri < 2; ++ri)

	for (int ci = -1; ci < 2; ++ci, ++window_it) 

	  *window_it = m.get(row + ri, col + ci);

      tmp.set(row, col, func(window.begin(), window.end()));

    }

  }

}



/* Steps through the image using a 3x3 window, replacing the center pixel

   with a value determined by a given function.

   This version uses all the eight pixels around the center pixel:

      ###  012

      # #  3 4

      ###  567

   The temporary matrix required may be passed in to avoid reallocating for

   each pass of a multi-pass algorithm. (see erode)

*/

template<class T, class F, class M>

void neighbor8o(const T& m, F& func, M& tmp) {

  if (m.nrows() < 3 || m.ncols() < 3)

    return;

  std::vector<typename T::value_type> window(8);



  unsigned int nrows_m1 = m.nrows() - 1;

  unsigned int ncols_m1 = m.ncols() - 1;

  unsigned int nrows_m2 = m.nrows() - 2;

  unsigned int ncols_m2 = m.ncols() - 2;



  // It's kind of silly to special case corners and edges, but it's more 

  // efficient than all of the if's one would have to do in the inner loop

  

  // Upper-left

  for (unsigned int i = 0; i < 4; ++i)

    window[i] = white(m);

  window[4] = m.get(0, 1);

  window[5] = white(m);

  window[6] = m.get(1, 0);

  window[7] = m.get(1, 1);

  tmp.set(0, 0, func(window.begin(), window.end()));

  

  // Upper-right

  window[3] = m.get(0, ncols_m2);

  window[5] = m.get(1, ncols_m2);

  window[6] = m.get(1, ncols_m1);

  window[4] = window[7] = white(m);

  tmp.set(0, ncols_m1, func(window.begin(), window.end()));

  

  // Lower-left

  window[1] = m.get(nrows_m2, 0);

  window[2] = m.get(nrows_m2, 1);

  window[4] = m.get(nrows_m1, 1);

  window[3] = window[5] = window[6] = white(m);

  tmp.set(nrows_m1, 0, func(window.begin(), window.end()));



  // Lower-right

  window[0] = m.get(nrows_m2, ncols_m2);

  window[1] = m.get(nrows_m2, ncols_m1);

  window[3] = m.get(nrows_m1, ncols_m2);

  window[2] = window[4] = white(m);

  tmp.set(nrows_m1, ncols_m1, func(window.begin(), window.end()));



  // Top edge

  window[0] = window[1] = white(m);

  for (unsigned int col = 1; col < ncols_m1; col++) {

    window[3] = m.get(0, col - 1);

    window[4] = m.get(0, col + 1);

    window[5] = m.get(1, col - 1);

    window[6] = m.get(1, col);

    window[7] = m.get(1, col + 1);

    tmp.set(0, col, func(window.begin(), window.end()));

  }



  // Bottom edge

  window[5] = window[6] = window[7] = white(m);

  for (unsigned int col = 1; col < ncols_m1; col++) {

    window[0] = m.get(nrows_m2, col - 1);

    window[1] = m.get(nrows_m2, col);

    window[2] = m.get(nrows_m2, col + 1);

    window[3] = m.get(nrows_m1, col - 1);

    window[4] = m.get(nrows_m1, col + 1);

    tmp.set(nrows_m1, col, func(window.begin(), window.end()));

  }



  // Left edge

  window[0] = window[3] = white(m);

  for (unsigned int row = 1; row < nrows_m1; row++) {

    window[1] = m.get(row - 1, 1);

    window[2] = m.get(row - 1, 0);

    window[4] = m.get(row, 1);

    window[6] = m.get(row + 1, 0);

    window[7] = m.get(row + 1, 1);

    tmp.set(row, 0, func(window.begin(), window.end()));

  }



  // Right edge

  window[2] = window[4] = window[7] = white(m);

  for (unsigned int row = 1; row < nrows_m1; row++) {

    window[0] = m.get(row - 1, ncols_m2);

    window[1] = m.get(row - 1, ncols_m1);

    window[3] = m.get(row, ncols_m2);

    window[5] = m.get(row + 1, ncols_m2);

    window[6] = m.get(row + 1, ncols_m1);

    tmp.set(row, ncols_m1, func(window.begin(), window.end()));

  }



  // Core of image

  for (unsigned int row = 1; row < nrows_m1; row++) {

    for (unsigned int col = 1; col < ncols_m1; col++) {

      // This may seem silly, but it's significantly faster than using

      // eight iterators

      window[0] = m.get(row - 1, col);

      window[1] = m.get(row - 1, col + 1);

      window[2] = m.get(row, col + 1);

      window[3] = m.get(row + 1, col + 1);

      window[4] = m.get(row + 1, col);

      window[5] = m.get(row + 1, col - 1);

      window[6] = m.get(row, col - 1);

      window[7] = m.get(row - 1, col - 1);

      tmp.set(row, col, func(window.begin(), window.end()));

    }

  }

}



/* Steps through the image using a 3x3 window, replacing the center pixel

   with a value determined by a given function.

   This version uses four surrounding pixels in the 3x3 window:

      # #  0 1

       #    2

      # #  3 4

   The temporary matrix required may be passed in to avoid reallocating for

   each pass of a multi-pass algorithm. (see erode)

*/

template<class T, class F, class M>

void neighbor4x(const T& m, F& func, M& tmp) {

  if (m.nrows() < 3 || m.ncols() < 3)

    return;

  std::vector<typename T::value_type> window(5);



  unsigned int nrows_m1 = m.nrows() - 1;

  unsigned int ncols_m1 = m.ncols() - 1;

  unsigned int nrows_m2 = m.nrows() - 2;

  unsigned int ncols_m2 = m.ncols() - 2;



  // It's kind of silly to special case corners and edges, but it's more 

  // efficient than all of the if's one would have to do in the inner loop



  // Upper left

  window[2] = m.get(0, 0);

  window[4] = m.get(1, 1);

  window[0] = window[1] = window[3] = white(m);

  tmp.set(0, 0, func(window.begin(), window.end()));



  // Upper right

  window[2] = m.get(0, ncols_m1);

  window[3] = m.get(1, ncols_m2);

  window[4] = white(m);

  tmp.set(0, ncols_m1, func(window.begin(), window.end()));



  // Lower left

  window[1] = m.get(nrows_m2, 1);

  window[2] = m.get(nrows_m1, 0);

  window[3] = white(m);

  tmp.set(nrows_m1, 0, func(window.begin(), window.end()));



  // Lower right

  window[0] = m.get(nrows_m2, ncols_m2);

  window[1] = white(m);

  window[2] = m.get(nrows_m1, ncols_m1);

  tmp.set(nrows_m1, ncols_m1, func(window.begin(), window.end()));



  // Top edge

  window[0] = white(m);

  for (unsigned int col = 1; col < ncols_m1; col++) {

    window[2] = m.get(0, col);

    window[3] = m.get(1, col - 1);

    window[4] = m.get(1, col + 1);

    tmp.set(0, col, func(window.begin(), window.end()));

  }



  // Bottom edge

  window[3] = window[4] = white(m);

  for (unsigned int col = 1; col < ncols_m1; col++) {

    window[0] = m.get(nrows_m2, col - 1);

    window[1] = m.get(nrows_m2, col + 1);

    window[2] = m.get(nrows_m1, col);

    tmp.set(nrows_m1, col, func(window.begin(), window.end()));

  }



  // Left edge

  window[0] = white(m);

  for (unsigned int row = 1; row < nrows_m1; row++) {

    window[1] = m.get(row - 1, 1);

    window[2] = m.get(row, 0);

    window[4] = m.get(row + 1, 1);

    tmp.set(row, 0, func(window.begin(), window.end()));

  }



  // Right edge

  window[1] = window[4] = white(m);

  for (unsigned int row = 1; row < nrows_m1; row++) {

    window[0] = m.get(row - 1, ncols_m2);

    window[2] = m.get(row, ncols_m1);

    window[3] = m.get(row + 1, ncols_m2);

    tmp.set(row, ncols_m1, func(window.begin(), window.end()));

  }



  // Core of image

  for (unsigned int row = 1; row < nrows_m1; row++) {

    for (unsigned int col = 1; col < ncols_m1; col++) {

      // This may seem silly, but it's significantly faster than using

      // nine iterators

      window[0] = m.get(row - 1, col - 1);

      window[1] = m.get(row - 1, col + 1);

      window[2] = m.get(row, col);

      window[3] = m.get(row + 1, col - 1);

      window[4] = m.get(row + 1, col + 1);

      tmp.set(row, col, func(window.begin(), window.end()));

    }

  }

}



/* Steps through the image using a 3x3 window, replacing the center pixel

   with a value determined by a given function.

   This version uses four surrounding pixels in the 3x3 window:

       #   0

      ### 123

       #   4

   The temporary matrix required may be passed in to avoid reallocating for

   each pass of a multi-pass algorithm. (see erode)

*/

template<class T, class F, class M>

void neighbor4o(const T& m, F& func, M& tmp) {

  if (m.nrows() < 3 || m.ncols() < 3)

    return;

  std::vector<typename T::value_type> window(5);



  unsigned int nrows_m1 = m.nrows() - 1;

  unsigned int ncols_m1 = m.ncols() - 1;

  unsigned int nrows_m2 = m.nrows() - 2;

  unsigned int ncols_m2 = m.ncols() - 2;

  

  // It's kind of silly to special case corners and edges, but it's more 

  // efficient than all of the if's one would have to do in the inner loop



  // Corners

  // Upper-left

  window[0] = window[1] = white(m);

  window[2] = m.get(0, 0);

  window[3] = m.get(0, 1);

  window[4] = m.get(1, 0);

  tmp.set(0, 0, func(window.begin(), window.end()));



  // Upper-right

  window[1] = m.get(0, ncols_m2);

  window[2] = m.get(0, ncols_m1);

  window[3] = white(m);

  window[4] = m.get(1, ncols_m1);

  tmp.set(0, ncols_m1, func(window.begin(), window.end()));



  // Lower-left

  window[0] = m.get(nrows_m2, 0);

  window[2] = m.get(nrows_m1, 0);

  window[3] = m.get(nrows_m1, 1);

  window[1] = window[4] = white(m);

  tmp.set(nrows_m1, 0, func(window.begin(), window.end()));



  // Lower-right

  window[0] = m.get(nrows_m2, ncols_m1);

  window[1] = m.get(nrows_m1, ncols_m2);

  window[2] = m.get(nrows_m1, ncols_m1);

  window[3] = white(m);

  tmp.set(nrows_m1, ncols_m1, func(window.begin(), window.end()));



  // Top edge

  window[0] = white(m);

  for (unsigned int col = 1; col < ncols_m1; col++) {

    window[1] = m.get(0, col - 1);

    window[2] = m.get(0, col);

    window[3] = m.get(0, col + 1);

    window[4] = m.get(1, col);

    tmp.set(0, col, func(window.begin(), window.end()));

  }



  // Bottom edge

  window[4] = white(m);

  for (unsigned int col = 1; col < ncols_m1; col++) {

    window[0] = m.get(nrows_m2, col);

    window[1] = m.get(nrows_m1, col - 1);

    window[2] = m.get(nrows_m1, col);

    window[3] = m.get(nrows_m1, col + 1);

    tmp.set(nrows_m1, col, func(window.begin(), window.end()));

  }



  // Left edge

  window[1] = white(m);

  for (unsigned int row = 1; row < nrows_m1; row++) {

    window[0] = m.get(row - 1, 0);

    window[2] = m.get(row, 0);

    window[3] = m.get(row, 1);

    window[4] = m.get(row + 1, 0);

    tmp.set(row, 0, func(window.begin(), window.end()));

  }



  // Right edge

  window[3] = white(m);

  for (unsigned int row = 1; row < nrows_m1; row++) {

    window[0] = m.get(row - 1, ncols_m1);

    window[1] = m.get(row, ncols_m2);

    window[2] = m.get(row, ncols_m1);

    window[4] = m.get(row + 1, ncols_m1);

    tmp.set(row, ncols_m1, func(window.begin(), window.end()));

  }



  // Core of image

  for (unsigned int row = 1; row < nrows_m1; row++) {

    for (unsigned int col = 1; col < ncols_m1; col++) {

      // This may seem silly, but it's significantly faster than using

      // nine iterators

      window[0] = m.get(row - 1, col);

      window[1] = m.get(row, col - 1);

      window[2] = m.get(row, col);

      window[3] = m.get(row, col + 1);

      window[4] = m.get(row + 1, col);

      tmp.set(row, col, func(window.begin(), window.end()));

    }

  }

}

}



#endif
