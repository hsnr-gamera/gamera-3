/*
 *
 * Copyright (C) 2001-2004 Ichiro Fujinaga, Michael Droettboom,
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

#ifndef mgd_png_support
#define mgd_png_support

#include <png.h>
#include <stdio.h>
#include "gamera.hpp"
#include "image_utilities.hpp"

// TODO: Get/Save resolution information

using namespace Gamera;

#define PNG_BYTES_TO_CHECK 8

void PNG_info_specific(const char* filename, FILE* & fp, png_structp& png_ptr, png_infop& info_ptr, png_infop& end_info, png_uint_32& width, png_uint_32& height, int& bit_depth, int& color_type) {
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
    throw std::runtime_error("Unknown PNG error");
  }

  png_set_sig_bytes(png_ptr, PNG_BYTES_TO_CHECK);

  // Initialize IO
  png_init_io(png_ptr, fp);

  // Read in info
  png_read_info(png_ptr, info_ptr);

  int dummy;

  png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &dummy, &dummy, &dummy);
}

void PNG_close(FILE* fp, png_structp png_ptr, png_infop info_ptr, png_infop end_info) {
  png_read_end(png_ptr, end_info);
  png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
  fclose(fp);
}

ImageInfo* PNG_info(char* filename) {
  FILE* fp;
  png_structp png_ptr;
  png_infop info_ptr, end_info;
  png_uint_32 width, height;
  int bit_depth, color_type;
  PNG_info_specific(filename, fp, png_ptr, info_ptr, end_info, width, height, bit_depth, color_type);

  ImageInfo* info = new ImageInfo();
  // Copy to our own ImageInfo object
  info->m_nrows = height;
  info->m_ncols = width;
  info->m_depth = bit_depth;
  if (color_type == PNG_COLOR_TYPE_PALETTE || color_type == PNG_COLOR_TYPE_RGB ||
      color_type == PNG_COLOR_TYPE_RGB_ALPHA)
    info->m_ncolors = 3;
  else if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    info->m_ncolors = 1;

  PNG_close(fp, png_ptr, info_ptr, end_info);
  return info;
}

template<class T>
void load_PNG_simple(T& image, png_structp& png_ptr) {
  typename T::row_iterator r = image.row_begin();
  for (; r != image.row_end(); ++r)
    png_read_row(png_ptr, (png_bytep)(&(*r)), NULL);
}

template<class T>
void load_PNG_onebit(T& image, png_structp& png_ptr) {
  png_set_invert_mono(png_ptr);
  png_set_gray_1_2_4_to_8(png_ptr);

  png_bytep row = new png_byte[image.ncols()];
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
  delete[] row;
}

Image* load_PNG(const char* filename, int storage) {
  FILE* fp;
  png_structp png_ptr;
  png_infop info_ptr, end_info;
  png_uint_32 width, height;
  int bit_depth, color_type;
  PNG_info_specific(filename, fp, png_ptr, info_ptr, end_info, width, height, bit_depth, color_type);

  if (color_type & PNG_COLOR_MASK_ALPHA)
    png_set_strip_alpha(png_ptr);

  if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_PALETTE ||
      color_type == PNG_COLOR_TYPE_RGB_ALPHA) {
    if (color_type == PNG_COLOR_TYPE_PALETTE)
      png_set_palette_to_rgb(png_ptr);
    typedef TypeIdImageFactory<RGB, DENSE> fact;
    fact::image_type* image =
      fact::create(0, 0, height, width);
    load_PNG_simple(*image, png_ptr);
    PNG_close(fp, png_ptr, info_ptr, end_info);
    return image;
  } else if (color_type == PNG_COLOR_TYPE_GRAY ||
	     color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
    if (bit_depth == 1) {
      if (storage == DENSE) {
	typedef TypeIdImageFactory<ONEBIT, DENSE> fact;
	fact::image_type* image =
	  fact::create(0, 0, height, width);
	load_PNG_onebit(*image, png_ptr);
	PNG_close(fp, png_ptr, info_ptr, end_info);
	return image;
      } else {
	typedef TypeIdImageFactory<ONEBIT, RLE> fact;
	fact::image_type* image =
	  fact::create(0, 0, height, width);
	load_PNG_onebit(*image, png_ptr);
	PNG_close(fp, png_ptr, info_ptr, end_info);
	return image;
      }	
    } else if (bit_depth <= 8) {
      if (bit_depth < 8)
	png_set_gray_1_2_4_to_8(png_ptr);
      typedef TypeIdImageFactory<GREYSCALE, DENSE> fact_type;
      fact_type::image_type*
	image = fact_type::create(0, 0, height, width);
      load_PNG_simple(*image, png_ptr);
      PNG_close(fp, png_ptr, info_ptr, end_info);
      return image;
    } else if (bit_depth == 16) {
      typedef TypeIdImageFactory<GREY16, DENSE> fact_type;
      fact_type::image_type*
	image = fact_type::create(0, 0, height, width);
      load_PNG_simple(*image, png_ptr);
      PNG_close(fp, png_ptr, info_ptr, end_info);
      return image;
    }
  }
  PNG_close(fp, png_ptr, info_ptr, end_info);
  throw std::runtime_error("PNG file in an unsupported type");
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
    typename T::row_iterator r = image.row_begin();
    for (; r != image.row_end(); ++r) {
      png_bytep from = row;
      typename T::col_iterator c = r.begin();
      for (; c != r.end(); ++c, ++from) {
	*from = (png_byte)(*c * max);
      }
      png_write_row(png_ptr, row);
    }
    delete[] row;
  }
};

template<>
struct PNG_saver<Grey16Pixel> {
  template<class T>
  void operator()(T& image, png_structp png_ptr) {
    png_bytep row = new png_byte[image.ncols() * 2];
    typename T::row_iterator r = image.row_begin();
    for (; r != image.row_end(); ++r) {
      typename T::col_iterator c = r.begin();
      unsigned short* from = (unsigned short *)row;
      for (; c != r.end(); ++c, ++from)
	*from = (unsigned short)(*c && 0xffff);
      png_write_row(png_ptr, row);
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
   else
     bit_depth = image.depth();
   int color_type = (image.ncolors() == 3) ? PNG_COLOR_TYPE_RGB : PNG_COLOR_TYPE_GRAY;
   png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth, color_type, 
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT);

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
