/*
 *
 * Copyright (C) 2001-2009 Ichiro Fujinaga, Michael Droettboom,
 * Karl MacMillan, and Christoph Dalitz
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

#include <string>
#include <exception>
#include <stdexcept>
#include <bitset>
#include "gamera.hpp"
#include <tiffio.h>

namespace Gamera {

// forward declarations
ImageInfo* tiff_info(const char* filename);
Image* load_tiff(const char* filename, int compressed);
template<class T>
void save_tiff(const T& matrix, const char* filename);

/*
  Get information about tiff images

  This function gets informtion about tiff images and places it in and
  ImageInfo object.  See image_info.hpp for more information.
*/
ImageInfo* tiff_info(const char* filename) {
  TIFFErrorHandler saved_handler = TIFFSetErrorHandler(NULL);
  TIFF* tif = 0;
  tif = TIFFOpen(filename, "r");
  if (tif == 0) {
    TIFFSetErrorHandler(saved_handler);
    throw std::invalid_argument("Failed to open image header");
  }
  // Create this later so it isn't leaked if filename does
  // not exist or is not a TIFF file.
  ImageInfo* info = new ImageInfo();

  /*
    The tiff library seems very sensitive to type yet provides only a
    stupid non-type-checked interface.  The following seems to work well
    (notice that resolution is floating point).  KWM 6/6/01
   */
  try {
    unsigned short tmp;
    uint32 size;
    TIFFGetFieldDefaulted(tif, TIFFTAG_IMAGEWIDTH, &size);
    info->ncols((size_t)size);
    TIFFGetFieldDefaulted(tif, TIFFTAG_IMAGELENGTH, &size);
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
    TIFFGetFieldDefaulted(tif, TIFFTAG_PHOTOMETRIC, &tmp);
    info->inverted(tmp == PHOTOMETRIC_MINISWHITE);
    
    TIFFClose(tif);
  } catch (std::exception e) {
    TIFFSetErrorHandler(saved_handler);
    delete info;
  }
  TIFFSetErrorHandler(saved_handler);
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
          tmp = pixel_traits<OneBitPixel>::black();
        else
          tmp = pixel_traits<OneBitPixel>::white(); 
        matrix.set(Point(j, i), tmp);
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
    if (info.inverted()) {
      for (size_t i = 0; i < info.nrows(); i++, mi++) {
        mj = mi.begin();
        TIFFReadScanline(tif, buf, i);
        data = (unsigned char *)buf;
        for (size_t j = 0; j < info.ncols(); j++, mj++) {
          *mj = 255 - data[j];
        }
      }
    } else {
      for (size_t i = 0; i < info.nrows(); i++, mi++) {
        mj = mi.begin();
        TIFFReadScanline(tif, buf, i);
        data = (unsigned char *)buf;
        for (size_t j = 0; j < info.ncols(); j++, mj++) {
          *mj = data[j];
        }
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

    template<class Pixel>
  struct tiff_saver {

  };


  // runtime test for endianness
  // (safer and less cumbersome than querying compiler macros)
  bool byte_order_little_endian() {
    long numberone = 1;
    return (*((char*)(&numberone)));
  }

  template<>
  struct tiff_saver<OneBitPixel> {
    template<class T>
    void operator()(const T& matrix, TIFF* tif) {
      TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
      tdata_t buf = _TIFFmalloc(TIFFScanlineSize(tif));
      if (!buf)
        throw std::runtime_error("Error allocating scanline");
      TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISWHITE);
      std::bitset<32> bits;
      unsigned long* data = (unsigned long *)buf;
      bool little_endian = byte_order_little_endian();
      typename T::const_vec_iterator it = matrix.vec_begin();
      for (size_t i = 0; i < matrix.nrows(); i++) {
        size_t bit_index = 0;
        int k = 31;
        for (size_t j = 0; j < matrix.ncols(); k--) {
          if (k < 0) {
            data[bit_index] = bits.to_ulong();
            if (little_endian)
              byte_swap32((unsigned char *)&data[bit_index]);
            bit_index++;
            k = 32;
            continue;
          }
          if (is_black(*it))
            bits[k] = 1;
          else
            bits[k] = 0;
          j++;
          it++;
        }
        // The last 32 pixels need to be saved, even if they are not full
        if (k != 31) {
          data[bit_index] = bits.to_ulong();
          if (little_endian)
            byte_swap32((unsigned char *)&data[bit_index]);
        }
        TIFFWriteScanline(tif, buf, i);
      }
      _TIFFfree(buf);
    }
  };

  template<>
  struct tiff_saver<GreyScalePixel> {
    template<class T>
    void operator()(const T& matrix, TIFF* tif) {
      TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
      tdata_t buf = _TIFFmalloc(TIFFScanlineSize(tif));
      if (!buf)
        throw std::runtime_error("Error allocating scanline");
      typename T::value_type pix;
      unsigned char* data = (unsigned char *)buf;
      for (size_t i = 0; i < matrix.nrows(); i++) {
        for (size_t j = 0; j < matrix.ncols(); j++) {
          pix = matrix[i][j];
          data[j] = (unsigned char)pix;
        }
        TIFFWriteScanline(tif, buf, i);
      }
      _TIFFfree(buf);
    }
  };

  template<>
  struct tiff_saver<Grey16Pixel> {
    template<class T>
    void operator()(const T& matrix, TIFF* tif) {
      TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
      tdata_t buf = _TIFFmalloc(TIFFScanlineSize(tif));
      if (!buf)
        throw std::runtime_error("Error allocating scanline");
      typename T::value_type pix;
      unsigned short* data = (unsigned short *)buf;
      for (size_t i = 0; i < matrix.nrows(); i++) {
        for (size_t j = 0; j < matrix.ncols(); j++) {
          pix = matrix[i][j];
          data[j] = (unsigned short)pix;
        }
        TIFFWriteScanline(tif, buf, i);
      }
      _TIFFfree(buf);
    }
  };

  template<>
  struct tiff_saver<RGBPixel> {
    template<class T>
    void operator()(const T& matrix, TIFF* tif) {
      TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
      tdata_t buf = _TIFFmalloc(TIFFScanlineSize(tif));
      if (!buf)
        throw std::runtime_error("Error allocating scanline");
      typename T::value_type pix;
      unsigned char* data = (unsigned char *)buf;
      for (size_t i = 0; i < matrix.nrows(); i++) {
        for (size_t j = 0, k = 0; j < matrix.ncols(); j++) {
          pix = matrix[i][j];
          data[k++] = pix.red();
          data[k++] = pix.green();
          data[k++] = pix.blue();
        }
        TIFFWriteScanline(tif, buf, i);
      }
      _TIFFfree(buf);
    }
  };
}

Image* load_tiff(const char* filename, int storage) {
  TIFFErrorHandler saved_handler = TIFFSetErrorHandler(NULL);
  ImageInfo* info = tiff_info(filename);
  if (info->ncolors() == 1) {
    if (info->depth() == 1) {
      if (storage == DENSE) {
        typedef TypeIdImageFactory<ONEBIT, DENSE> fact_type;
        fact_type::image_type*
          image = fact_type::create(Point(0, 0), Dim(info->ncols(), info->nrows()));
        image->resolution(info->x_resolution());
        tiff_load_onebit(*image, *info, filename);
        delete info;
        TIFFSetErrorHandler(saved_handler);
        return image;
      } else {
        typedef TypeIdImageFactory<ONEBIT, RLE> fact_type;
        fact_type::image_type*
          image = fact_type::create(Point(0, 0), Dim(info->ncols(), info->nrows()));
        image->resolution(info->x_resolution());
        tiff_load_onebit(*image, *info, filename);
        delete info;
        TIFFSetErrorHandler(saved_handler);
        return image;
      }
    }
  }
  if (storage == RLE) {
    delete info;
    TIFFSetErrorHandler(saved_handler);
    throw std::runtime_error("Pixel type must be OneBit to use RLE data.");
  }
  if (info->ncolors() == 3) {
    typedef TypeIdImageFactory<RGB, DENSE> fact;
    fact::image_type* image =
      fact::create(Point(0, 0), Dim(info->ncols(), info->nrows()));
    tiff_load_rgb(*image, *info, filename);
    delete info;
    TIFFSetErrorHandler(saved_handler);
    return image;
  } else if (info->depth() == 8) {
    typedef TypeIdImageFactory<GREYSCALE, DENSE> fact_type;
    fact_type::image_type*
      image = fact_type::create(Point(0, 0), Dim(info->ncols(), info->nrows()));
    image->resolution(info->x_resolution());
    tiff_load_greyscale(*image, *info, filename);
    delete info;
    TIFFSetErrorHandler(saved_handler);
    return image;
  } else if (info->depth() == 16) {
    typedef TypeIdImageFactory<GREY16, DENSE> fact_type;
    fact_type::image_type*
      image = fact_type::create(Point(0, 0), Dim(info->ncols(), info->nrows()));
    image->resolution(info->x_resolution());
    tiff_load_greyscale(*image, *info, filename);
    delete info;
    TIFFSetErrorHandler(saved_handler);
    return image;
  }
  delete info;
  TIFFSetErrorHandler(saved_handler);
  throw std::runtime_error("Unable to load image of this type!");
  return 0;
}

template<class T>
void save_tiff(const T& matrix, const char* filename) {
  TIFF* tif = 0;
  tif = TIFFOpen(filename, "w");
  if (tif == 0)
    throw std::invalid_argument("Failed to create image.");

  TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, matrix.ncols());
  TIFFSetField(tif, TIFFTAG_IMAGELENGTH, matrix.nrows());
  TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, matrix.depth());
  TIFFSetField(tif, TIFFTAG_XRESOLUTION, matrix.resolution());
  TIFFSetField(tif, TIFFTAG_YRESOLUTION, matrix.resolution());
  TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, matrix.ncolors());
  TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);


  tiff_saver<typename T::value_type> saver;
  saver(matrix, tif);
        
  TIFFClose(tif);
}

}
#endif
