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
#include "image_utilities.hpp"

/*
  IMAGE CONVERSION

  These are routines to convert from any image type to a greyscale, grey16, float, or rgb
  image. Conversion to onebit images can be handled by a combination of these routines
  and one of the thresholding algorithms. Some of these conversion routines are simple and
  do not lose data (greyscale -> grey16 for example) while others may lose data (rgb -> greyscale).
  In the cases where data is lost, an effort is made to do something sensible, but it is likely
  that a specialized application would probably want to provide custom conversion routines.
*/
namespace Gamera {

  /*
    All of the guts of the conversions are contained in small objects
    in this namespace. The goal is to allow specialization on pixel
    type without having to use C++ partial specialization (which is
    broken on many compilers).
  */
  namespace _image_conversion {

    // create an image of a given pixel type - size is determined by the
    // image passed in.
    template<class Pixel>
    struct creator {
      template<class T>
      static ImageView<ImageData<Pixel> >* image(const T& image) {
	typedef ImageData<Pixel> data_type;
	typedef ImageView<data_type> view_type;
	data_type* data = new data_type(image.size(), image.offset_y(),
					image.offset_x());
	view_type* view = new view_type(*data, image);
	return view;
      }
    };    

    /*
      TO RGB
    */
    template<class Pixel>
    struct to_rgb_converter {
      template<class T>
      RGBImageView* operator()(const T& image) {
	typename T::value_type max = find_max(image.parent());
	double scale;
	if (max > 0)
	  scale = 255.0 / max;
	else
	  scale = 0.0;
	RGBImageView* view = creator<RGBPixel>::image(image);
	typename T::const_row_iterator in_row = image.row_begin();
	typename T::const_col_iterator in_col;
	typename RGBImageView::row_iterator out_row = view->row_begin();
	typename RGBImageView::col_iterator out_col;
	ImageAccessor<typename T::value_type> in_acc;
	ImageAccessor<RGBPixel> out_acc;
	for (; in_row != image.row_end(); ++in_row, ++out_row) {
	  for (in_col = in_row.begin(), out_col = out_row.begin();
	       in_col != in_row.end(); ++in_col, ++out_col) {
	    GreyScalePixel tmp = GreyScalePixel(in_acc(in_col) * scale);
	    out_acc.set(RGBPixel(tmp, tmp, tmp), out_col);
	  }
	}
	return view;
      }
    };
    
    template<>
    struct to_rgb_converter<OneBitPixel> {
      template<class T>
      RGBImageView* operator()(const T& image) {
	RGBImageView* view = creator<RGBPixel>::image(image);
	typename T::const_row_iterator in_row = image.row_begin();
	typename T::const_col_iterator in_col;
	typename RGBImageView::row_iterator out_row = view->row_begin();
	typename RGBImageView::col_iterator out_col;
	ImageAccessor<OneBitPixel> in_acc;
	ImageAccessor<RGBPixel> out_acc;
	for (; in_row != image.row_end(); ++in_row, ++out_row) {
	  for (in_col = in_row.begin(), out_col = out_row.begin();
	       in_col != in_row.end(); ++in_col, ++out_col) {
	    OneBitPixel tmp = in_acc(in_col);
	    if (is_white(tmp))
	      out_acc.set(white(*view), out_col);
	    else
	      out_acc.set(black(*view), out_col);
	  }
	}
	return view;
      }
    };

    template<>
    struct to_rgb_converter<GreyScalePixel> {
      RGBImageView* operator()(const GreyScaleImageView& image) {
	RGBImageView* view = creator<RGBPixel>::image(image);

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
    };

    /*
      TO GREYSCALE
    */
    template<class Pixel>
    struct to_greyscale_converter {
      template<class T>
      GreyScaleImageView* operator()(const T& image) {
	GreyScaleImageView* view = creator<GreyScalePixel>::image(image);
	typename T::value_type max = find_max(image.parent());
	double scale;
	if (max > 0)
	  scale = 255.0 / max;
	else
	  scale = 0.0;

	typename T::const_row_iterator in_row = image.row_begin();
	typename T::const_col_iterator in_col;
	typename GreyScaleImageView::row_iterator out_row = view->row_begin();
	typename GreyScaleImageView::col_iterator out_col;
	ImageAccessor<typename T::value_type> in_acc;
	ImageAccessor<GreyScalePixel> out_acc;
	for (; in_row != image.row_end(); ++in_row, ++out_row) {
	  for (in_col = in_row.begin(), out_col = out_row.begin();
	       in_col != in_row.end(); ++in_col, ++out_col) {
	    out_acc.set(GreyScalePixel(in_acc(in_col) * scale), out_col);
	  }
	}
	return view;
      }
    };

    template<>
    struct to_greyscale_converter<OneBitPixel> {
      template<class T>
      GreyScaleImageView* operator()(const T& image) {
	GreyScaleImageView* view = creator<GreyScalePixel>::image(image);

	typename T::const_row_iterator in_row = image.row_begin();
	typename T::const_col_iterator in_col;
	typename GreyScaleImageView::row_iterator out_row = view->row_begin();
	typename GreyScaleImageView::col_iterator out_col;
	ImageAccessor<OneBitPixel> in_acc;
	ImageAccessor<GreyScalePixel> out_acc;
	for (; in_row != image.row_end(); ++in_row, ++out_row) {
	  for (in_col = in_row.begin(), out_col = out_row.begin();
	       in_col != in_row.end(); ++in_col, ++out_col) {
	    OneBitPixel tmp = in_acc(in_col);
	    if (is_white(tmp))
	      out_acc.set(white(*view), out_col);
	    else
	      out_acc.set(black(*view), out_col);
	  }
	}
	return view;
      }
    };

    template<>
    struct to_greyscale_converter<RGBPixel> {
      GreyScaleImageView* operator()(const RGBImageView& image) {
	GreyScaleImageView* view = creator<GreyScalePixel>::image(image);

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
    };
  
    /*
      Grey16
    */
    template<class Pixel>
    struct to_grey16_converter {
      template<class T>
      Grey16ImageView* operator()(const T& image) {
	Grey16ImageView* view = creator<Grey16Pixel>::image(image);

	typename T::value_type max = find_max(image.parent());
	double scale;
	if (max > 0)
	  scale = 255.0 / max;
	else
	  scale = 0.0;

	typename T::const_row_iterator in_row = image.row_begin();
	typename T::const_col_iterator in_col;
	typename Grey16ImageView::row_iterator out_row = view->row_begin();
	typename Grey16ImageView::col_iterator out_col;
	ImageAccessor<typename T::value_type> in_acc;
	ImageAccessor<Grey16Pixel> out_acc;
	for (; in_row != image.row_end(); ++in_row, ++out_row) {
	  for (in_col = in_row.begin(), out_col = out_row.begin();
	       in_col != in_row.end(); ++in_col, ++out_col) {
	    out_acc.set(Grey16Pixel(in_acc(in_col) * scale), out_col);
	  }
	}
	return view;
      }
    };
    
    template<>
    struct to_grey16_converter<RGBPixel> {
      Grey16ImageView* operator()(const RGBImageView& image) {
	Grey16ImageView* view = creator<Grey16Pixel>::image(image);

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
    };

    template<>
    struct to_grey16_converter<OneBitPixel> {
      template<class T>
      Grey16ImageView* operator()(const T& image) {
	Grey16ImageView* view = creator<Grey16Pixel>::image(image);

	typename T::const_row_iterator in_row = image.row_begin();
	typename T::const_col_iterator in_col;
	typename Grey16ImageView::row_iterator out_row = view->row_begin();
	typename Grey16ImageView::col_iterator out_col;
	ImageAccessor<OneBitPixel> in_acc;
	ImageAccessor<Grey16Pixel> out_acc;
	for (; in_row != image.row_end(); ++in_row, ++out_row) {
	  for (in_col = in_row.begin(), out_col = out_row.begin();
	       in_col != in_row.end(); ++in_col, ++out_col) {
	    OneBitPixel tmp = in_acc(in_col);
	    if (is_white(tmp))
	      out_acc.set(white(*view), out_col);
	    else
	      out_acc.set(black(*view), out_col);
	  }
	}
	return view;
      }
    };

    template<>
    struct to_grey16_converter<GreyScalePixel> {
      Grey16ImageView* operator()(const GreyScaleImageView& image) {
	Grey16ImageView* view = creator<Grey16Pixel>::image(image);

	GreyScaleImageView::const_row_iterator in_row = image.row_begin();
	GreyScaleImageView::const_col_iterator in_col;
	Grey16ImageView::row_iterator out_row = view->row_begin();
	Grey16ImageView::col_iterator out_col;
	ImageAccessor<GreyScalePixel> in_acc;
	ImageAccessor<Grey16Pixel> out_acc;
	for (; in_row != image.row_end(); ++in_row, ++out_row) {
	  for (in_col = in_row.begin(), out_col = out_row.begin();
	       in_col != in_row.end(); ++in_col, ++out_col) {
	    GreyScalePixel tmp = in_acc(in_col);
	    out_acc.set(tmp, out_col);
	  }
	}
	return view;
      }
    };

    /*
      Float
    */
    template<class Pixel>
    struct to_float_converter {
      template<class T>
      FloatImageView* operator()(const T& image) {
	FloatImageView* view = creator<FloatPixel>::image(image);
	typename T::const_row_iterator in_row = image.row_begin();
	typename T::const_col_iterator in_col;
	typename FloatImageView::row_iterator out_row = view->row_begin();
	typename FloatImageView::col_iterator out_col;
	ImageAccessor<typename T::value_type> in_acc;
	ImageAccessor<FloatPixel> out_acc;
	for (; in_row != image.row_end(); ++in_row, ++out_row) {
	  for (in_col = in_row.begin(), out_col = out_row.begin();
	       in_col != in_row.end(); ++in_col, ++out_col) {
	    out_acc.set(FloatPixel(in_acc.get(in_col)), out_col);
	  }
	}
	return view;	
      }
    };

    template<>
    struct to_float_converter<RGBPixel> {
      FloatImageView* operator()(const RGBImageView& image) {
	FloatImageView* view = creator<FloatPixel>::image(image);
	RGBImageView::const_row_iterator in_row = image.row_begin();
	RGBImageView::const_col_iterator in_col;
	FloatImageView::row_iterator out_row = view->row_begin();
	FloatImageView::col_iterator out_col;
	ImageAccessor<RGBImageView::value_type> in_acc;
	ImageAccessor<FloatPixel> out_acc;
	for (; in_row != image.row_end(); ++in_row, ++out_row) {
	  for (in_col = in_row.begin(), out_col = out_row.begin();
	       in_col != in_row.end(); ++in_col, ++out_col) {
	    out_acc.set(FloatPixel(in_acc.get(in_col).luminance()), out_col);
	  }
	}
	return view;	
      }
    };

    template<>
    struct to_float_converter<OneBitPixel> {
      template<class T>
      FloatImageView* operator()(const T& image) {
	FloatImageView* view = creator<FloatPixel>::image(image);

	FloatImageView::row_iterator out_row = view->row_begin();
	FloatImageView::col_iterator out_col;
	typename T::const_row_iterator in_row = image.row_begin();
	typename T::const_col_iterator in_col;
	ImageAccessor<typename T::value_type> in_acc;
	ImageAccessor<FloatPixel> out_acc;
	for (; in_row != image.row_end(); ++in_row, ++out_row) {
	  for (in_col = in_row.begin(), out_col = out_row.begin();
	       in_col != in_row.end(); ++in_col, ++out_col) {
	    OneBitPixel tmp = in_acc.get(in_col);
	    if (is_white(tmp))
	      out_acc.set(FloatPixel(1), out_col);
	    else
	      out_acc.set(FloatPixel(0), out_col);	      
	  }
	}
	return view;	
      }
    };

  }

  template<class T>
  RGBImageView* to_rgb(const T& image) {
    _image_conversion::to_rgb_converter<typename T::value_type> conv;
    return conv(image);
  }

  template<class T>
  GreyScaleImageView* to_greyscale(const T& image) {
    _image_conversion::to_greyscale_converter<typename T::value_type> conv;
    return conv(image);    
  }

  template<class T>
  Grey16ImageView* to_grey16(const T& image) {
    _image_conversion::to_grey16_converter<typename T::value_type> conv;
    return conv(image);    
  }

  template<class T>
  FloatImageView* to_float(const T& image) {
    _image_conversion::to_float_converter<typename T::value_type> conv;
    return conv(image);    
  }

}
#endif
