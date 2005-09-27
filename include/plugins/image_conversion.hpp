/*
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
	data_type* data = new data_type(image);
	view_type* view = new view_type(*data);
	view->resolution(image.resolution());
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
	try {
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
	} catch (std::exception e) {
	  delete view->data();
	  delete view;
	  throw;
	}
	return view;
      }
    };
    
    template<>
    struct to_rgb_converter<OneBitPixel> {
      template<class T>
      RGBImageView* operator()(const T& image) {
	RGBImageView* view = creator<RGBPixel>::image(image);
	try {
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
	} catch (std::exception e) {
	  delete view->data();
	  delete view;
	  throw;
	}
	return view;
      }
    };

    template<>
    struct to_rgb_converter<GreyScalePixel> {
      RGBImageView* operator()(const GreyScaleImageView& image) {
	RGBImageView* view = creator<RGBPixel>::image(image);

	try {
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
	} catch (std::exception e) {
	  delete view->data();
	  delete view;
	  throw;
	}
	return view;
      }      
    };

    template<>
    struct to_rgb_converter<ComplexPixel> {
      RGBImageView* operator()(const ComplexImageView& image) {
	ComplexPixel max = find_max(image.parent());
	double scale;
	if (max.real() > 0)
	  scale = 255.0 / max.real();
	else
	  scale = 0.0;
	RGBImageView* view = creator<RGBPixel>::image(image);
	try {
	  ComplexImageView::const_row_iterator in_row = image.row_begin();
	  ComplexImageView::const_col_iterator in_col;
	  RGBImageView::row_iterator out_row = view->row_begin();
	  RGBImageView::col_iterator out_col;
	  ComplexRealAccessor in_acc;
	  ImageAccessor<RGBPixel> out_acc;
	  for (; in_row != image.row_end(); ++in_row, ++out_row) {
	    for (in_col = in_row.begin(), out_col = out_row.begin();
		 in_col != in_row.end(); ++in_col, ++out_col) {
	      GreyScalePixel tmp = GreyScalePixel(in_acc(in_col) * scale);
	      out_acc.set(RGBPixel(tmp, tmp, tmp), out_col);
	    }
	  }
	} catch (std::exception e) {
	  delete view->data();
	  delete view;
	  throw;
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
	try {
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
       	} catch (std::exception e) {
	  delete view->data();
	  delete view;
	  throw;
	}
	return view;
      }
    };

    template<>
    struct to_greyscale_converter<OneBitPixel> {
      template<class T>
      GreyScaleImageView* operator()(const T& image) {
	GreyScaleImageView* view = creator<GreyScalePixel>::image(image);

	try {
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
       	} catch (std::exception e) {
	  delete view->data();
	  delete view;
	  throw;
	}
	return view;
      }
    };

    template<>
    struct to_greyscale_converter<RGBPixel> {
      GreyScaleImageView* operator()(const RGBImageView& image) {
	GreyScaleImageView* view = creator<GreyScalePixel>::image(image);

	try {
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
	} catch (std::exception e) {
	  delete view->data();
	  delete view;
	  throw;
	}
	return view;
      }
    };

    template<>
    struct to_greyscale_converter<ComplexPixel> {
      GreyScaleImageView* operator()(const ComplexImageView& image) {
	GreyScaleImageView* view = creator<GreyScalePixel>::image(image);
	try {
	  ComplexPixel max = find_max(image.parent());
	  double scale;
	  if (max.real() > 0)
	    scale = 255.0 / max.real();
	  else
	    scale = 0.0;
	  
	  ComplexImageView::const_row_iterator in_row = image.row_begin();
	  ComplexImageView::const_col_iterator in_col;
	  GreyScaleImageView::row_iterator out_row = view->row_begin();
	  GreyScaleImageView::col_iterator out_col;
	  ComplexRealAccessor in_acc;
	  ImageAccessor<GreyScalePixel> out_acc;
	  for (; in_row != image.row_end(); ++in_row, ++out_row) {
	    for (in_col = in_row.begin(), out_col = out_row.begin();
		 in_col != in_row.end(); ++in_col, ++out_col) {
	      out_acc.set(GreyScalePixel(in_acc(in_col) * scale), out_col);
	    }
	  }
	} catch (std::exception e) {
	  delete view->data();
	  delete view;
	  throw;
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
	
	try {
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
	} catch (std::exception e) {
	  delete view->data();
	  delete view;
	  throw;
	}
	return view;
      }
    };
    
    template<>
    struct to_grey16_converter<RGBPixel> {
      Grey16ImageView* operator()(const RGBImageView& image) {
	Grey16ImageView* view = creator<Grey16Pixel>::image(image);

	try {
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
	} catch (std::exception e) {
	  delete view->data();
	  delete view;
	  throw;
	}
	return view;
      }
    };

    template<>
    struct to_grey16_converter<OneBitPixel> {
      template<class T>
      Grey16ImageView* operator()(const T& image) {
	Grey16ImageView* view = creator<Grey16Pixel>::image(image);

	try {
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
	} catch (std::exception e) {
	  delete view->data();
	  delete view;
	  throw;
	}

	return view;
      }
    };

    template<>
    struct to_grey16_converter<GreyScalePixel> {
      Grey16ImageView* operator()(const GreyScaleImageView& image) {
	Grey16ImageView* view = creator<Grey16Pixel>::image(image);

	try {
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
	} catch (std::exception e) {
	  delete view->data();
	  delete view;
	  throw;
	}

	return view;
      }
    };

    template<>
    struct to_grey16_converter<ComplexPixel> {
      Grey16ImageView* operator()(const ComplexImageView& image) {
	Grey16ImageView* view = creator<Grey16Pixel>::image(image);
	try {
	  ComplexPixel max = find_max(image.parent());
	  double scale;
	  if (max.real() > 0)
	    scale = 255.0 / max.real();
	  else
	    scale = 0.0;
	  
	  ComplexImageView::const_row_iterator in_row = image.row_begin();
	  ComplexImageView::const_col_iterator in_col;
	  Grey16ImageView::row_iterator out_row = view->row_begin();
	  Grey16ImageView::col_iterator out_col;
	  ComplexRealAccessor in_acc;
	  ImageAccessor<Grey16Pixel> out_acc;
	  for (; in_row != image.row_end(); ++in_row, ++out_row) {
	    for (in_col = in_row.begin(), out_col = out_row.begin();
		 in_col != in_row.end(); ++in_col, ++out_col) {
	      out_acc.set(Grey16Pixel(in_acc(in_col) * scale), out_col);
	    }
	  }
	} catch (std::exception e) {
	  delete view->data();
	  delete view;
	  throw;
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
	try {
	  typename T::const_row_iterator in_row = image.row_begin();
	  typename T::const_col_iterator in_col;
	  typename FloatImageView::row_iterator out_row = view->row_begin();
	  typename FloatImageView::col_iterator out_col;
	  typedef typename choose_accessor<T>::real_accessor Accessor;
	  Accessor in_acc = Accessor(in_acc);
	  ImageAccessor<FloatPixel> out_acc;
	  for (; in_row != image.row_end(); ++in_row, ++out_row) {
	    for (in_col = in_row.begin(), out_col = out_row.begin();
		 in_col != in_row.end(); ++in_col, ++out_col) {
	      out_acc.set(FloatPixel(in_acc(in_col)), out_col);
	    }
	  }
       	} catch (std::exception e) {
	  delete view->data();
	  delete view;
	  throw;
	}
	return view;	
      }
    };

    template<>
    struct to_float_converter<RGBPixel> {
      FloatImageView* operator()(const RGBImageView& image) {
	FloatImageView* view = creator<FloatPixel>::image(image);
	try {
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
	} catch (std::exception e) {
	  delete view->data();
	  delete view;
	  throw;
	}

	return view;	
      }
    };

    template<>
    struct to_float_converter<OneBitPixel> {
      template<class T>
      FloatImageView* operator()(const T& image) {
	FloatImageView* view = creator<FloatPixel>::image(image);

	try {
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
		out_acc.set(FloatPixel(1.0), out_col);
	      else
		out_acc.set(FloatPixel(0.0), out_col);	      
	    }
	  }
	} catch (std::exception e) {
	  delete view->data();
	  delete view;
	  throw;
	}

	return view;	
      }
    };

    /*
      Complex
    */
    template<class Pixel>
    struct to_complex_converter {
      template<class T>
      ComplexImageView* operator()(const T& image) {
	ComplexImageView* view = creator<ComplexPixel>::image(image);
	try {
	  typename T::const_row_iterator in_row = image.row_begin();
	  typename T::const_col_iterator in_col;
	  typename ComplexImageView::row_iterator out_row = view->row_begin();
	  typename ComplexImageView::col_iterator out_col;
	  typedef typename choose_accessor<T>::real_accessor InAccessor;
	  InAccessor in_acc = choose_accessor<T>::make_real_accessor(image);
	  typedef typename choose_accessor<ComplexImageView>::real_accessor OutAccessor;
	  OutAccessor out_acc = choose_accessor<ComplexImageView>::make_real_accessor(*view);
	  for (; in_row != image.row_end(); ++in_row, ++out_row) {
	    for (in_col = in_row.begin(), out_col = out_row.begin();
		 in_col != in_row.end(); ++in_col, ++out_col) {
	      out_acc.set(in_acc(in_col), out_col);
	    }
	  }
	} catch (std::exception e) {
	  delete view->data();
	  delete view;
	  throw;
	}

	return view;	
      }
    };
    
    template<>
    struct to_complex_converter<OneBitPixel> {
      template<class T>
      ComplexImageView* operator()(const T& image) {
	ComplexImageView* view = creator<ComplexPixel>::image(image);
	
	try {
	  ComplexImageView::row_iterator out_row = view->row_begin();
	  ComplexImageView::col_iterator out_col;
	  typename T::const_row_iterator in_row = image.row_begin();
	  typename T::const_col_iterator in_col;
	  ImageAccessor<typename T::value_type> in_acc;
	  ImageAccessor<ComplexPixel> out_acc;
	  for (; in_row != image.row_end(); ++in_row, ++out_row) {
	    for (in_col = in_row.begin(), out_col = out_row.begin();
		 in_col != in_row.end(); ++in_col, ++out_col) {
	      OneBitPixel tmp = in_acc.get(in_col);
	      if (is_white(tmp)) {
		out_acc.set(ComplexPixel(1.0, 0.0), out_col);
	      }
	      else {
		out_acc.set(ComplexPixel(0.0, 0.0), out_col);	  
	      }    
	    }
	  }
	} catch (std::exception e) {
	  delete view->data();
	  delete view;
	  throw;
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

  template<class T>
  ComplexImageView* to_complex(const T& image) {
    _image_conversion::to_complex_converter<typename T::value_type> conv;
    return conv(image);    
  }

  template<class T>
  FloatImageView* extract_real(const T& image) {
    FloatImageData* data = new FloatImageData(image.size(), image.origin());
    FloatImageView* view = new FloatImageView(*data, image);
    try {
      typename T::const_row_iterator in_row = image.row_begin();
      typename T::const_col_iterator in_col;
      typename FloatImageView::row_iterator out_row = view->row_begin();
      typename FloatImageView::col_iterator out_col;
      typedef typename choose_accessor<T>::accessor Accessor;
      Accessor in_acc = Accessor(in_acc);
      ImageAccessor<FloatPixel> out_acc;
      for (; in_row != image.row_end(); ++in_row, ++out_row) {
	for (in_col = in_row.begin(), out_col = out_row.begin();
	     in_col != in_row.end(); ++in_col, ++out_col) {
	  out_acc.set(FloatPixel(in_acc(in_col).real()), out_col);
	}
      }
    } catch (std::exception e) {
      delete view;
      delete data;
      throw;
    }
      
    return view;	
  }

  template<class T>
  FloatImageView* extract_imaginary(const T& image) {
    FloatImageData* data = new FloatImageData(image.size(), image.origin());
    FloatImageView* view = new FloatImageView(*data, image);
    try {
      typename T::const_row_iterator in_row = image.row_begin();
      typename T::const_col_iterator in_col;
      typename FloatImageView::row_iterator out_row = view->row_begin();
      typename FloatImageView::col_iterator out_col;
      typedef typename choose_accessor<T>::accessor Accessor;
      Accessor in_acc = Accessor(in_acc);
      ImageAccessor<FloatPixel> out_acc;
      for (; in_row != image.row_end(); ++in_row, ++out_row) {
	for (in_col = in_row.begin(), out_col = out_row.begin();
	     in_col != in_row.end(); ++in_col, ++out_col) {
	  out_acc.set(FloatPixel(in_acc(in_col).imag()), out_col);
	}
      }
    } catch (std::exception e) {
      delete view;
      delete data;
      throw;
    }
    return view;	
  }

  }
#endif
