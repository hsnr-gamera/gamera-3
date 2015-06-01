/*
 *
 * Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, Karl MacMillan
 *               2015      Christoph Dalitz
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/*
 Resolution support graciously donated by Damon Li.
*/

#ifndef mgd_png_support
#define mgd_png_support

#include "image_utilities.hpp"
#include <png.h>
#include <stdio.h>
#include <stdint.h>

// TODO: Get/Save resolution information

using namespace Gamera;

#define PNG_BYTES_TO_CHECK 8
#define METER_PER_INCH 0.0254

void PNG_info_specific(const char* filename, FILE* & fp, png_structp& png_ptr, png_infop& info_ptr, png_infop& end_info, png_uint_32& width, png_uint_32& height, int& bit_depth, int& color_type, double& x_resolution, double& y_resolution) {
  fp = fopen(filename, "rb");
  if (!fp)
    throw std::invalid_argument("Failed to open image");

  // Check if a PNG file
  char buf[PNG_BYTES_TO_CHECK];
  if (fread(buf, 1, PNG_BYTES_TO_CHECK, fp) != PNG_BYTES_TO_CHECK) {
    fclose(fp);
    throw std::runtime_error("Image file too small");
  }
  if (png_sig_cmp((png_byte*)buf, 0, PNG_BYTES_TO_CHECK)) {
    fclose(fp);
    throw std::runtime_error("Not a PNG file");
  }
  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, (png_voidp)NULL,
                   NULL, NULL);
  if (!png_ptr) {
    fclose(fp);
    throw std::runtime_error("Could not read PNG header");
  }

  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
    fclose(fp);
    throw std::runtime_error("Could not read PNG info");
  }
  
  end_info = png_create_info_struct(png_ptr);
  if (!end_info) {
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
    fclose(fp);
    throw std::runtime_error("Could not read PNG info");
  }

  if (setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    fclose(fp);
    throw std::runtime_error("error in reading PNG header");
  }

  png_set_sig_bytes(png_ptr, PNG_BYTES_TO_CHECK);

  // Initialize IO
  png_init_io(png_ptr, fp);

  // Read in info
  png_read_info(png_ptr, info_ptr);

  int dummy;

  png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &dummy, &dummy, &dummy);

  //Damon
  x_resolution = (double)png_get_x_pixels_per_meter(png_ptr, info_ptr) * METER_PER_INCH;
  y_resolution = (double)png_get_y_pixels_per_meter(png_ptr, info_ptr) * METER_PER_INCH;
  //Damon: end

}

void PNG_close(FILE* fp, png_structp png_ptr, png_infop info_ptr, png_infop end_info) {
  // As PNG_close() might be called when png_read_image() has not yet been
  // used (premature termination), png_read_end() might crash.
  // We therefore omit the call to png_read_end(), as we are not
  // interested in the comment texts following the image data anyway.
  //png_read_end(png_ptr, end_info);
  png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
  fclose(fp);
}

ImageInfo* PNG_info(char* filename) {
  FILE* fp;
  png_structp png_ptr;
  png_infop info_ptr, end_info;
  png_uint_32 width, height;
  int bit_depth, color_type;
  double x_resolution, y_resolution;
  PNG_info_specific(filename, fp, png_ptr, info_ptr, end_info, width, height, bit_depth, color_type, x_resolution, y_resolution);

  ImageInfo* info = new ImageInfo();
  try {
    // Copy to our own ImageInfo object
    info->m_nrows = height;
    info->m_ncols = width;
    info->m_depth = bit_depth;
    info->m_x_resolution = x_resolution;
    info->m_y_resolution = y_resolution;
    if (color_type == PNG_COLOR_TYPE_PALETTE || color_type == PNG_COLOR_TYPE_RGB ||
    color_type == PNG_COLOR_TYPE_RGB_ALPHA)
      info->m_ncolors = 3;
    else if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
      info->m_ncolors = 1;
  } catch (std::exception e) {
    delete info;
    throw;
  }
  // PNG_close(fp, png_ptr, info_ptr, end_info);
  return info;
}

template<class T>
void load_PNG_simple(T& image, png_structp& png_ptr) {
  typename T::row_iterator r = image.row_begin();
  for (; r != image.row_end(); ++r)
    png_read_row(png_ptr, (png_bytep)(&(*r)), NULL);
}

template<class T>
void load_PNG_grey16(T& image, png_structp& png_ptr) {
  uint16_t* row = new uint16_t[image.ncols()];
  if (byte_order_little_endian())
    png_set_swap(png_ptr);
  try {
    typename T::row_iterator r = image.row_begin();
    for (; r != image.row_end(); ++r) {
      png_read_row(png_ptr, (png_bytep)row, NULL);
      uint16_t* from = row;
      typename T::col_iterator c = r.begin();
      for (; c != r.end(); ++c, ++from) {
        c.set((int)*from);
      }
    }
  } catch (std::exception e) {
    delete[] row;
    throw;
  }
  delete[] row;
}

template<class T>
void load_PNG_onebit(T& image, png_structp& png_ptr) {
  png_set_invert_mono(png_ptr);
#if PNG_LIBPNG_VER > 10399
  png_set_expand_gray_1_2_4_to_8(png_ptr);
#else
  png_set_gray_1_2_4_to_8(png_ptr);
#endif

  png_bytep row = new png_byte[image.ncols()];
  try {
    typename T::row_iterator r = image.row_begin();
    for (; r != image.row_end(); ++r) {
      png_read_row(png_ptr, row, NULL);
      png_bytep from = row;
      typename T::col_iterator c = r.begin();
      for (; c != r.end(); ++c, ++from) {
        if (*from)
          c.set(pixel_traits<OneBitPixel>::black());
        else
          c.set(pixel_traits<OneBitPixel>::white());
      }
    }
  } catch (std::exception e) {
    delete[] row;
    throw;
  }
  delete[] row;
}

Image* load_PNG(const char* filename, int storage) {
  FILE* fp;
  png_structp png_ptr;
  png_infop info_ptr, end_info;
  png_uint_32 width, height;
  int bit_depth, color_type;
  double x_resolution, y_resolution;
  PNG_info_specific(filename, fp, png_ptr, info_ptr, end_info, width, height, bit_depth, color_type, x_resolution, y_resolution);

  // libpng exception handling
  if (setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    fclose(fp);
    throw std::runtime_error("error in reading PNG data");
  }

  //Damon
  double reso = (x_resolution + y_resolution) / 2.0;

  //Damon: end
  // if (color_type & PNG_COLOR_MASK_ALPHA)
  png_set_strip_alpha(png_ptr);

  if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_PALETTE ||
      color_type == PNG_COLOR_TYPE_RGB_ALPHA) {
    if (storage == RLE) {
      PNG_close(fp, png_ptr, info_ptr, end_info);
      throw std::runtime_error("Pixel type must be OneBit to use RLE data.");
    }
    if (bit_depth > 8) {
#if PNG_LIBPNG_VER >= 10504
      png_set_scale_16(png_ptr);
#else
      png_set_strip_16(png_ptr);
#endif
    }
    else if (bit_depth < 8) {
      png_set_expand(png_ptr);
    }
    if (color_type == PNG_COLOR_TYPE_PALETTE)
      png_set_palette_to_rgb(png_ptr);
    typedef TypeIdImageFactory<RGB, DENSE> fact;
    fact::image_type* image =
      fact::create(Point(0, 0), Dim(width, height));
    load_PNG_simple(*image, png_ptr);
    //Damon
    image->resolution(reso);
    //Damon: end    
    PNG_close(fp, png_ptr, info_ptr, end_info);
    return image;
  } else if (color_type == PNG_COLOR_TYPE_GRAY ||
             color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
    if (bit_depth == 1) {
      if (storage == DENSE) {
        typedef TypeIdImageFactory<ONEBIT, DENSE> fact;
        fact::image_type* image =
          fact::create(Point(0, 0), Dim(width, height));
        load_PNG_onebit(*image, png_ptr);
        //Damon
        image->resolution(reso);
        //Damon: end    
        PNG_close(fp, png_ptr, info_ptr, end_info);
        return image;
      } else {
        typedef TypeIdImageFactory<ONEBIT, RLE> fact;
        fact::image_type* image =
          fact::create(Point(0, 0), Dim(width, height));
        load_PNG_onebit(*image, png_ptr);
        //Damon
        image->resolution(reso);
        //Damon: end    
        PNG_close(fp, png_ptr, info_ptr, end_info);
        return image;
      } 
    } else if (bit_depth <= 8) {
      if (storage == RLE) {
        PNG_close(fp, png_ptr, info_ptr, end_info);
        throw std::runtime_error("Pixel type must be OneBit to use RLE data.");
      }
      if (bit_depth < 8) {
#if PNG_LIBPNG_VER > 10399
        png_set_expand_gray_1_2_4_to_8(png_ptr);
#else
        png_set_gray_1_2_4_to_8(png_ptr);
#endif
      }
      typedef TypeIdImageFactory<GREYSCALE, DENSE> fact_type;
      fact_type::image_type*
        image = fact_type::create(Point(0, 0), Dim(width, height));
      load_PNG_simple(*image, png_ptr);
      //Damon
      image->resolution(reso);
      //Damon: end  
      PNG_close(fp, png_ptr, info_ptr, end_info);
      return image;
    } else if (bit_depth == 16) {
      if (storage == RLE) {
        PNG_close(fp, png_ptr, info_ptr, end_info);
        throw std::runtime_error("Pixel type must be OneBit to use RLE data.");
      }
      typedef TypeIdImageFactory<GREY16, DENSE> fact_type;
      fact_type::image_type*
        image = fact_type::create(Point(0, 0), Dim(width, height));
      load_PNG_grey16(*image, png_ptr);
      //Damon
      image->resolution(reso);
      //Damon: end  
      PNG_close(fp, png_ptr, info_ptr, end_info);
      return image;
    }
  }
  PNG_close(fp, png_ptr, info_ptr, end_info);
  throw std::runtime_error("PNG file is an unsupported type");
}

template<class P>
struct PNG_saver {
  template<class T>
  void operator()(T& image, png_structp png_ptr) {
    typename T::row_iterator r = image.row_begin();
    for (; r != image.row_end(); ++r)
      png_write_row(png_ptr, (png_bytep)(&(*r)));
  }
};

template<>
struct PNG_saver<OneBitPixel> {
  template<class T>
  void operator()(T& image, png_structp png_ptr) {
    png_bytep row = new png_byte[image.ncols()];
    try {
      typename T::row_iterator r = image.row_begin();
      for (; r != image.row_end(); ++r) {
    png_bytep from = row;
    typename T::col_iterator c = r.begin();
    for (; c != r.end(); ++c, ++from) {
      if (is_black(c.get()))
        *from = 0;
      else
        *from = 255;
    }
    png_write_row(png_ptr, row);
      }
    } catch (std::exception e) {
      delete[] row;
      throw;
    }
    delete[] row;
  }
};

template<>
struct PNG_saver<FloatPixel> {
  template<class T>
  void operator()(T& image, png_structp png_ptr) {
    FloatPixel max = 0;
    max = find_max(image.parent());
    if (max > 0)
      max = 255.0 / max;
    else 
      max = 0;

    png_bytep row = new png_byte[image.ncols()];
    try {
      typename T::row_iterator r = image.row_begin();
      for (; r != image.row_end(); ++r) {
    png_bytep from = row;
    typename T::col_iterator c = r.begin();
    for (; c != r.end(); ++c, ++from) {
      *from = (png_byte)(*c * max);
    }
    png_write_row(png_ptr, row);
      }
    } catch (std::exception e) {
      delete[] row;
      throw;
    }
    delete[] row;
  }
};

template<>
struct PNG_saver<ComplexPixel> {
  template<class T>
  void operator()(T& image, png_structp png_ptr) {
    ComplexPixel temp = find_max(image.parent());
    FloatPixel max;
    if (temp.real() > 0)
      max = 255.0 / temp.real();
    else 
      max = 0;

    png_bytep row = new png_byte[image.ncols()];
    try {
      typename T::row_iterator r = image.row_begin();
      for (; r != image.row_end(); ++r) {
    png_bytep from = row;
    typename T::col_iterator c = r.begin();
    for (; c != r.end(); ++c, ++from) {
      *from = (png_byte)((*c).real() * max);
    }
    png_write_row(png_ptr, row);
      }
    } catch (std::exception e) {
      delete[] row;
      throw;
    }
    delete[] row;
  }
};

template<>
struct PNG_saver<Grey16Pixel> {
  template<class T>
  void operator()(T& image, png_structp png_ptr) {
    png_bytep row = new png_byte[image.ncols() * 2];
    try {
      typename T::row_iterator r = image.row_begin();
      for (; r != image.row_end(); ++r) {
    typename T::col_iterator c = r.begin();
    unsigned short* from = (unsigned short *)row;
    for (; c != r.end(); ++c, ++from)
      *from = (unsigned short)(*c && 0xffff);
    png_write_row(png_ptr, row);
      }
    } catch (std::exception e) {
      delete[] row;
      throw;
    }
    delete[] row;
  }
};

template<class T>
void save_PNG(T& image, const char* filename) {
  FILE* fp = fopen(filename, "wb");
  if (!fp)
    throw std::invalid_argument("Failed to open image");
  
  png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr) {
    fclose(fp);
    throw std::runtime_error("Couldn't create PNG header");
  }

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
    fclose(fp);
    throw std::runtime_error("Couldn't create PNG header");
  }         

  if (setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);
    throw std::runtime_error("Unknown PNG error");
  }
  
  png_uint_32 width = image.ncols();
  png_uint_32 height = image.nrows();
  int bit_depth;
  if (image.depth() == 32)
    bit_depth = 16;
  else if (image.depth() == 64)
    bit_depth = 8;
  else if (image.depth() == 128)
    bit_depth = 8;
  else
    bit_depth = image.depth();
  int color_type = (image.ncolors() == 3) ? PNG_COLOR_TYPE_RGB : PNG_COLOR_TYPE_GRAY;
  png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth, color_type, 
           PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
           PNG_FILTER_TYPE_DEFAULT);
  
  //Damon 
  png_uint_32 res_x = (png_uint_32)(image.resolution() / METER_PER_INCH);
  png_uint_32 res_y = (png_uint_32)(image.resolution() / METER_PER_INCH);
  int unit_type = PNG_RESOLUTION_METER;
  png_set_pHYs(png_ptr, info_ptr, res_x, res_y, unit_type);
  //Damon:end

  png_init_io(png_ptr, fp);
  png_write_info(png_ptr, info_ptr);
  png_set_packing(png_ptr);
  
  PNG_saver<typename T::value_type> saver;
  saver(image, png_ptr);
  
  png_write_end(png_ptr, info_ptr);
  png_destroy_write_struct(&png_ptr, &info_ptr);
  fclose(fp);
}

#endif
