/*
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

#ifndef kwm121102_image_conversion
#define kwm121102_image_conversion

#include "gamera.hpp"

using namespace Gamera;
/*
  From RGB
*/
GreyScaleImageView* rgb_to_greyscale(const RGBImageView& image) {
  GreyScaleImageData* data =
    new GreyScaleImageData(image.nrows(), image.ncols(),
			   image.offset_y(), image.offset_x());
  GreyScaleImageView* view =
    new GreyScaleImageView(*data, image.offset_y(), image.offset_x(),
			   image.nrows(), image.ncols());
  RGBImageView::const_row_iterator in_row = image.row_begin();
  RGBImageView::const_col_iterator in_col;
  GreyScaleImageView::row_iterator out_row = view->row_begin();
  GreyScaleImageView::col_iterator out_col;
  ImageAccessor<RGBPixel> in_acc;
  ImageAccessor<GreyScalePixel> out_acc;
  for (; in_row != image.row_end(); ++in_row, ++out_row) {
    for (in_col = in_row.begin(), out_col = out_row.begin();
	 in_col != in_row.end(); ++in_col, ++out_col) {
      out_acc.set(in_acc(in_col).luminance(), out_col);
    }
  }
  return view;
}

Grey16ImageView* rgb_to_grey16(const RGBImageView& image) {
  Grey16ImageData* data =
    new Grey16ImageData(image.nrows(), image.ncols(),
			   image.offset_y(), image.offset_x());
  Grey16ImageView* view =
    new Grey16ImageView(*data, image.offset_y(), image.offset_x(),
			   image.nrows(), image.ncols());
  RGBImageView::const_row_iterator in_row = image.row_begin();
  RGBImageView::const_col_iterator in_col;
  Grey16ImageView::row_iterator out_row = view->row_begin();
  Grey16ImageView::col_iterator out_col;
  ImageAccessor<RGBPixel> in_acc;
  ImageAccessor<Grey16Pixel> out_acc;
  for (; in_row != image.row_end(); ++in_row, ++out_row) {
    for (in_col = in_row.begin(), out_col = out_row.begin();
	 in_col != in_row.end(); ++in_col, ++out_col) {
      out_acc.set(in_acc(in_col).luminance(), out_col);
    }
  }
  return view;
}

FloatImageView* rgb_to_float(const RGBImageView& image) {
  FloatImageData* data =
    new FloatImageData(image.nrows(), image.ncols(),
			   image.offset_y(), image.offset_x());
  FloatImageView* view =
    new FloatImageView(*data, image.offset_y(), image.offset_x(),
			   image.nrows(), image.ncols());
  RGBImageView::const_row_iterator in_row = image.row_begin();
  RGBImageView::const_col_iterator in_col;
  FloatImageView::row_iterator out_row = view->row_begin();
  FloatImageView::col_iterator out_col;
  ImageAccessor<RGBPixel> in_acc;
  ImageAccessor<FloatPixel> out_acc;
  for (; in_row != image.row_end(); ++in_row, ++out_row) {
    for (in_col = in_row.begin(), out_col = out_row.begin();
	 in_col != in_row.end(); ++in_col, ++out_col) {
      out_acc.set(in_acc(in_col).luminance(), out_col);
    }
  }
  return view;
}

/*
  From Greyscale
*/
FloatImageView* greyscale_to_float(const GreyScaleImageView& image) {
  FloatImageData* data =
    new FloatImageData(image.nrows(), image.ncols(),
			   image.offset_y(), image.offset_x());
  FloatImageView* view =
    new FloatImageView(*data, image.offset_y(), image.offset_x(),
			   image.nrows(), image.ncols());
  GreyScaleImageView::const_row_iterator in_row = image.row_begin();
  GreyScaleImageView::const_col_iterator in_col;
  FloatImageView::row_iterator out_row = view->row_begin();
  FloatImageView::col_iterator out_col;
  ImageAccessor<GreyScalePixel> in_acc;
  ImageAccessor<FloatPixel> out_acc;
  for (; in_row != image.row_end(); ++in_row, ++out_row) {
    for (in_col = in_row.begin(), out_col = out_row.begin();
	 in_col != in_row.end(); ++in_col, ++out_col) {
      out_acc.set(float(in_acc(in_col)), out_col);
    }
  }
  return view;
}

Grey16ImageView* greyscale_to_grey16(const GreyScaleImageView& image) {
  Grey16ImageData* data =
    new Grey16ImageData(image.nrows(), image.ncols(),
			   image.offset_y(), image.offset_x());
  Grey16ImageView* view =
    new Grey16ImageView(*data, image.offset_y(), image.offset_x(),
			   image.nrows(), image.ncols());
  GreyScaleImageView::const_row_iterator in_row = image.row_begin();
  GreyScaleImageView::const_col_iterator in_col;
  Grey16ImageView::row_iterator out_row = view->row_begin();
  Grey16ImageView::col_iterator out_col;
  ImageAccessor<GreyScalePixel> in_acc;
  ImageAccessor<Grey16Pixel> out_acc;
  for (; in_row != image.row_end(); ++in_row, ++out_row) {
    for (in_col = in_row.begin(), out_col = out_row.begin();
	 in_col != in_row.end(); ++in_col, ++out_col) {
      out_acc.set(Grey16Pixel(in_acc(in_col)), out_col);
    }
  }
  return view;
}

RGBImageView* greyscale_to_rgb(const GreyScaleImageView& image) {
  RGBImageData* data =
    new RGBImageData(image.nrows(), image.ncols(),
			   image.offset_y(), image.offset_x());
  RGBImageView* view =
    new RGBImageView(*data, image.offset_y(), image.offset_x(),
			   image.nrows(), image.ncols());
  GreyScaleImageView::const_row_iterator in_row = image.row_begin();
  GreyScaleImageView::const_col_iterator in_col;
  RGBImageView::row_iterator out_row = view->row_begin();
  RGBImageView::col_iterator out_col;
  ImageAccessor<GreyScalePixel> in_acc;
  ImageAccessor<RGBPixel> out_acc;
  for (; in_row != image.row_end(); ++in_row, ++out_row) {
    for (in_col = in_row.begin(), out_col = out_row.begin();
	 in_col != in_row.end(); ++in_col, ++out_col) {
      GreyScalePixel tmp = in_acc(in_col);
      out_acc.set(RGBPixel(tmp, tmp, tmp), out_col);
    }
  }
  return view;
}

#endif
