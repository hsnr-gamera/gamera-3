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

#ifndef kwm10222002_tiff_support
#define kwm10222002_tiff_support

#include <tiffio.h>
#include <string>
#include <exception>
#include <stdexcept>
#include <bitset>
#include "gamera.hpp"

using namespace Gamera;

// forward declarations
ImageInfo* tiff_info(const char* filename);
Image* load_tiff(const char* filename, int compressed);

/*
  Get information about tiff images

  This function gets informtion about tiff images and places it in and
  ImageInfo object.  See image_info.hpp for more information.
*/
ImageInfo* tiff_info(const char* filename) {
  ImageInfo* info = new ImageInfo();
  TIFF* tif = 0;
  tif = TIFFOpen(filename, "r");
  if (tif == 0) {
    throw std::invalid_argument("Failed to open image");
  }

  /*
    The tiff library seems very sensitive to type yet provides only a
    stupid non-type-checked interface.  The following seems to work well
    (notice that resolution is floating point).  KWM 6/6/01
   */
  unsigned short tmp;
  int size;
  TIFFGetFieldDefaulted(tif, TIFFTAG_IMAGEWIDTH, (int)&size);
  info->ncols((size_t)size);
  TIFFGetFieldDefaulted(tif, TIFFTAG_IMAGELENGTH, (int)&size);
  info->nrows((size_t)size);
  TIFFGetFieldDefaulted(tif, TIFFTAG_BITSPERSAMPLE, &tmp);
  info->depth((size_t)tmp);
  float res;
  TIFFGetFieldDefaulted(tif, TIFFTAG_XRESOLUTION, &res);
  info->x_resolution(res);
  TIFFGetFieldDefaulted(tif, TIFFTAG_YRESOLUTION, &res);
  info->y_resolution(res);
  TIFFGetFieldDefaulted(tif, TIFFTAG_SAMPLESPERPIXEL, &tmp);
  info->ncolors((size_t)tmp);

  TIFFClose(tif);
  return info;
}
namespace {

  template<class T>
  void tiff_load_onebit(T& matrix, ImageInfo& info, const char* filename) {
    // open the image
    TIFF* tif = TIFFOpen(filename, "r");
    tdata_t buf = _TIFFmalloc(TIFFScanlineSize(tif));
    
    // load the data
    for (size_t i = 0; i < info.nrows(); i++) {
      TIFFReadScanline(tif, buf, i);
      char* data = (char *)buf;
      std::bitset<8> bits;
      int tmp;
      for (size_t j = 0, k = 7, bit_index = 0; j < info.ncols(); j++, k--) {
	if (k == 7) {
	  bits = data[bit_index];
	  bit_index++;
	}
	if (bits[k])
	  tmp = 1;
	else
	  tmp = 0;
	matrix.set(i, j, tmp);
	if (k == 0)
	  k = 8;
      }
    }
    // do the cleanup
    _TIFFfree(buf);
    TIFFClose(tif);
  }

  template<class T>
  void tiff_load_greyscale(T& matrix, ImageInfo& info, const char* filename) {
    // open the image
    TIFF* tif = TIFFOpen(filename, "r");
    tdata_t buf = _TIFFmalloc(TIFFScanlineSize(tif));
    
    typename T::row_iterator mi = matrix.row_begin();
    typename T::col_iterator mj;
    unsigned char* data;
    for (size_t i = 0; i < info.nrows(); i++, mi++) {
      mj = mi.begin();
      TIFFReadScanline(tif, buf, i);
      data = (unsigned char *)buf;
      for (size_t j = 0; j < info.ncols(); j++, mj++) {
	*mj = data[j];
      }
    }
    
    // do the cleanup
    _TIFFfree(buf);
    TIFFClose(tif);
  }

  template<class T>
  void tiff_load_grey16(T& matrix, ImageInfo& info, const char* filename) {
    // open the image
    TIFF* tif = TIFFOpen(filename, "r");
    tdata_t buf = _TIFFmalloc(TIFFScanlineSize(tif));
    
    typename T::row_iterator mi = matrix.row_begin();
    typename T::col_iterator mj;
    unsigned short* data;
    for (size_t i = 0; i < info.nrows(); i++, mi++) {
      mj = mi.begin();
      TIFFReadScanline(tif, buf, i);
      data = (unsigned short *)buf;
      for (size_t j = 0; j < info.ncols(); j++, mj++) {
	*mj = data[j];
      }
    }
    
    // do the cleanup
    _TIFFfree(buf);
    TIFFClose(tif);
  }

  template<class T>
  void tiff_load_rgb(T& matrix, ImageInfo& info, const char* filename) {
    // open the image
    TIFF* tif = TIFFOpen(filename, "r");
    tdata_t buf = _TIFFmalloc(TIFFScanlineSize(tif));
    
    typename T::row_iterator mi = matrix.row_begin();
    typename T::col_iterator mj;
    unsigned char* data;
    for (size_t i = 0; i < info.nrows(); i++, mi++) {
      mj = mi.begin();
      TIFFReadScanline(tif, buf, i);
      data = (unsigned char *)buf;
      for (size_t j = 0; j < info.ncols() * 3; j += 3, mj++) {
	(*mj).red(data[j]);
	(*mj).green(data[j + 1]);
	(*mj).blue(data[j + 2]);
      }
    }
    // do the cleanup
    _TIFFfree(buf);
    TIFFClose(tif);
  }
}

Image* load_tiff(const char* filename, int storage) {
  ImageInfo* info = tiff_info(filename);
  if (info->ncolors() == 3) {
    TypeIdImageFactory<RGB, DENSE> fact;
    TypeIdImageFactory<RGB, DENSE>::image_type* image =
      fact.create(0, 0, info->nrows(), info->ncols());
    tiff_load_rgb(*image, *info, filename);
    return image;
  } else if (info->ncolors() == 1) {
    if (info->depth() == 1) {
      if (storage == DENSE) {
	TypeIdImageFactory<ONEBIT, DENSE> fact;
	TypeIdImageFactory<ONEBIT, DENSE>::image_type*
	  image = fact.create(0, 0, info->nrows(), info->ncols());
	tiff_load_onebit(*image, *info, filename);
	printf("hi");
	return image;
      } else {
	TypeIdImageFactory<ONEBIT, RLE> fact;
	TypeIdImageFactory<ONEBIT, RLE>::image_type*
	  image = fact.create(0, 0, info->nrows(), info->ncols());
	tiff_load_onebit(*image, *info, filename);
	return image;
      }
    } else if (info->depth() == 8) {
	TypeIdImageFactory<GREYSCALE, DENSE> fact;
	TypeIdImageFactory<GREYSCALE, DENSE>::image_type*
	  image = fact.create(0, 0, info->nrows(), info->ncols());
	tiff_load_greyscale(*image, *info, filename);
	return image;
    } else if (info->depth() == 16) {
	TypeIdImageFactory<GREY16, DENSE> fact;
	TypeIdImageFactory<GREY16, DENSE>::image_type*
	  image = fact.create(0, 0, info->nrows(), info->ncols());
	tiff_load_greyscale(*image, *info, filename);
	return image;
    } else {
      PyErr_SetString(PyExc_RuntimeError, "Unable to load image of this type!");
      return 0;
    }
  } else {
    PyErr_SetString(PyExc_RuntimeError, "Unable to load image of this type!");
    return 0;
  }

  delete info;
}


#endif
