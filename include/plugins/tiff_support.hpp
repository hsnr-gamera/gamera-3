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

  template<class Pixel>
  struct tiff_loader {

  };

  template<>
  struct tiff_loader<OneBitPixel> {
    template<class T>
    void operator()(T& matrix, const std::string& filename) {
      ImageInfo info;
      tiff_info(info, filename);

      // Make certain this is the right type of image
      if (info.ncolors() != 1)
	throw std::invalid_argument("Wrong number of colors for image");
      if (info.depth() != 1)
	throw std::invalid_argument("Wrong image depth");
      // Make certain that the data is the correct size
      matrix.data()->dimensions(info.nrows(), info.ncols());
      // Make certain that the matrix is the correct size
      matrix.dimensions(info.nrows(), info.ncols());

      // open the image
      TIFF* tif = TIFFOpen(filename.c_str(), "r");
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
  };

  template<>
  struct tiff_loader<GreyScalePixel> {
    template<class T>
    void operator()(T& matrix, const std::string& filename) {
      ImageInfo info;
      tiff_info(info, filename);

      // Make certain this is the right type of image
      if (info.ncolors() != 1)
	throw std::invalid_argument("Wrong number of colors for image");
      if (info.depth() != 8)
	throw std::invalid_argument("Wrong image depth");
      // Make certain that the data is the correct size
      matrix.data()->dimensions(info.nrows(), info.ncols());
      // Make certain that the matrix is the correct size
      matrix.dimensions(info.nrows(), info.ncols());

      // open the image
      TIFF* tif = TIFFOpen(filename.c_str(), "r");
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
  };

  template<>
  struct tiff_loader<Grey16Pixel> {
    template<class T>
    void operator()(T& matrix, const std::string& filename) {
      ImageInfo info;
      tiff_info(info, filename);
      
      // Make certain this is the right type of image
      if (info.ncolors() != 1)
	throw std::invalid_argument("Wrong number of colors for image");
      if (info.depth() != 16)
	throw std::invalid_argument("Wrong image depth");
      // Make certain that the data is the correct size
      matrix.data()->dimensions(info.nrows(), info.ncols());
      // Make certain that the matrix is the correct size
      matrix.dimensions(info.nrows(), info.ncols());

      // open the image
      TIFF* tif = TIFFOpen(filename.c_str(), "r");
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
  };

  template<>
  struct tiff_loader<RGBPixel> {
    template<class T>
    void operator()(T& matrix, const std::string& filename) {
      ImageInfo info;
      tiff_info(info, filename);
      
      // Make certain this is the right type of image
      if (info.ncolors() != 3)
	throw std::invalid_argument("Wrong number of colors for image");
      if (info.depth() != 8)
	throw std::invalid_argument("Wrong image depth");
      // Make certain that the data is the correct size
      matrix.data()->dimensions(info.nrows(), info.ncols());
      // Make certain that the matrix is the correct size
      matrix.dimensions(info.nrows(), info.ncols());

      // open the image
      TIFF* tif = TIFFOpen(filename.c_str(), "r");
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
  };
  
}

Image* load_tiff(const char* filename) {
  
  tiff_loader<typename T::value_type> loader;
  loader(matrix, filename);
}


#endif
