/*
 *
 * Copyright (C) 2002 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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

#ifndef kwm03112002_matrix_types
#define kwm03112002_matrix_types

#include "pixel.hpp"
#include "matrix_data.hpp"
#include "matrix_view.hpp"
#include "rle_data.hpp"
#include "connected_components.hpp"

#include <list>

/*
  The standard matrix types.
*/

namespace Gamera {

  /*
    Matrix Data
   */
  typedef MatrixData<GreyScalePixel> GreyScaleMatrixData;
  typedef MatrixData<Grey16Pixel> Grey16MatrixData;
  typedef MatrixData<FloatPixel> FloatMatrixData;
  typedef MatrixData<RGBPixel> RGBMatrixData;
  typedef MatrixData<OneBitPixel> OneBitMatrixData;
  typedef RleMatrixData<OneBitPixel> OneBitRleMatrixData;

  /*
    MatrixView
   */
  typedef MatrixView<GreyScaleMatrixData> GreyScaleMatrixView;
  typedef MatrixView<Grey16MatrixData> Grey16MatrixView;
  typedef MatrixView<FloatMatrixData> FloatMatrixView;
  typedef MatrixView<RGBMatrixData> RGBMatrixView;
  typedef MatrixView<OneBitMatrixData> OneBitMatrixView;
  typedef MatrixView<OneBitRleMatrixData> OneBitRleMatrixView;

  /*
    Connected-components
   */
  typedef ConnectedComponent<OneBitMatrixData> CC;
  typedef ConnectedComponent<OneBitRleMatrixData> RleCC;
  typedef std::list<CC> ConnectedComponents;
  typedef std::list<RleCC> RleConnectedComponents;

  /*
    Factory for types.
  */
  template<class T>
  struct matrix_factory {
    typedef MatrixView<typename T::data_type> view_type;
    typedef ConnectedComponent<typename T::data_type> cc_type;
    typedef std::list<cc_type> ccs_type;
  };
}


#endif
