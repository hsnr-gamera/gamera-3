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



#ifndef kwm03112002_image_types

#define kwm03112002_image_types



#include "pixel.hpp"

#include "image_data.hpp"

#include "image_view.hpp"

#include "rle_data.hpp"

#include "connected_components.hpp"



#include <list>



/*

  The standard image types.

*/



namespace Gamera {



  /*

    Image Data

   */

  typedef ImageData<GreyScalePixel> GreyScaleImageData;

  typedef ImageData<Grey16Pixel> Grey16ImageData;

  typedef ImageData<FloatPixel> FloatImageData;

  typedef ImageData<RGBPixel> RGBImageData;

  typedef ImageData<ComplexPixel> ComplexImageData;

  typedef ImageData<OneBitPixel> OneBitImageData;

  typedef RleImageData<OneBitPixel> OneBitRleImageData;



  /*

    ImageView

   */

  typedef ImageView<GreyScaleImageData> GreyScaleImageView;

  typedef ImageView<Grey16ImageData> Grey16ImageView;

  typedef ImageView<FloatImageData> FloatImageView;

  typedef ImageView<RGBImageData> RGBImageView;

  typedef ImageView<ComplexImageData> ComplexImageView;

  typedef ImageView<OneBitImageData> OneBitImageView;

  typedef ImageView<OneBitRleImageData> OneBitRleImageView;



  /*

    Connected-components

   */

  typedef ConnectedComponent<OneBitImageData> Cc;

  typedef ConnectedComponent<OneBitRleImageData> RleCc;

  typedef std::list<Cc*> ConnectedComponents;

  typedef std::list<RleCc*> RleConnectedComponents;



  /*

    Enumeration for all of the image types, pixel types, and storage

    types.

  */

  enum PixelTypes {

    ONEBIT,

    GREYSCALE,

    GREY16,

    RGB,

    FLOAT,

    COMPLEX

  };

  

  enum StorageTypes {

    DENSE,

    RLE

  };

  

  /*

    To make the wrapping code a little easier these are all of the

    combinations of pixel and storage types. The order is so that

    the non-compressed views correspond to the PixelTypes.

  */

  enum ImageCombinations {

    ONEBITIMAGEVIEW,

    GREYSCALEIMAGEVIEW,

    GREY16IMAGEVIEW,

    RGBIMAGEVIEW,

    FLOATIMAGEVIEW,

    COMPLEXIMAGEVIEW,

    ONEBITRLEIMAGEVIEW,

    CC,

    RLECC

  };

  

  enum ClassificationStates {

    UNCLASSIFIED,

    AUTOMATIC,

    HEURISTIC,

    MANUAL

  };



  /*

    Factory for types based on an existing image. This makes it easier

    to make a new view from an existing type without worrying whether

    it is a Cc, etc.

  */

  template<class T>

  struct ImageFactory {

    // data types

    typedef typename T::data_type data_type;

    typedef ImageData<typename T::value_type> dense_data_type;

    typedef RleImageData<typename T::value_type> rle_data_type;

    // view types

    typedef ImageView<data_type> view_type;

    typedef ImageView<dense_data_type> dense_view_type;

    typedef ImageView<rle_data_type> rle_view_type;

    // cc types

    typedef ConnectedComponent<data_type> cc_type;

    typedef ConnectedComponent<dense_data_type> dense_cc_type;

    typedef ConnectedComponent<rle_data_type> rle_cc_type;

    typedef std::list<cc_type*> ccs_type;

    typedef std::list<dense_cc_type*> dense_ccs_type;

    typedef std::list<rle_cc_type*> rle_ccs_type;

    // methods for creating new images and views

    static view_type* new_view(const T& view) {

      view_type* nview = new view_type(*((data_type*)view.data()),

				       view.offset_y(), view.offset_x(),

				       view.nrows(), view.ncols());

      return nview;

    }

    static view_type* new_view(const T& view, size_t ul_y, size_t ul_x,

			       size_t nrows, size_t ncols) {

      view_type* nview = new view_type(*((data_type*)view.data()),

				       ul_y, ul_x, nrows, ncols);

      return nview;

    }

    static view_type* new_image(const T& view) {

      data_type* data = new data_type(view.nrows(), view.ncols(),

				      view.offset_y(), view.offset_x());

      view_type* nview = new view_type(*data,

				      view.offset_y(), view.offset_x(),

				      view.nrows(), view.ncols());

      return nview;

    }

  };



  template<>

  struct ImageFactory<RGBImageView> {

    // data types

    typedef RGBImageView::data_type data_type;

    typedef ImageData<RGBImageView::value_type> dense_data_type;

    typedef ImageData<RGBImageView::value_type> rle_data_type;

    // view types

    typedef ImageView<data_type> view_type;

    typedef ImageView<dense_data_type> dense_view_type;

    typedef ImageView<rle_data_type> rle_view_type;

    // cc types

    typedef ConnectedComponent<data_type> cc_type;

    typedef ConnectedComponent<dense_data_type> dense_cc_type;

    typedef ConnectedComponent<rle_data_type> rle_cc_type;

    typedef std::list<cc_type*> ccs_type;

    typedef std::list<dense_cc_type*> dense_ccs_type;

    typedef std::list<rle_cc_type*> rle_ccs_type;

    static view_type* new_view(const RGBImageView& view) {

      view_type* nview = new view_type(*((data_type*)view.data()),

				      view.offset_y(), view.offset_x(),

				      view.nrows(), view.ncols());

      return nview;

    }

    static view_type* new_view(const RGBImageView& view, size_t ul_y,

			       size_t ul_x, size_t nrows, size_t ncols) {

      view_type* nview = new view_type(*((data_type*)view.data()),

				       ul_y, ul_x, nrows, ncols);

      return nview;

    }

    static view_type* new_image(const RGBImageView& view) {

      data_type* data = new data_type(view.nrows(), view.ncols(),

				      view.offset_y(), view.offset_x());

      view_type* nview = new view_type(*data,

				      view.offset_y(), view.offset_x(),

				      view.nrows(), view.ncols());

      return nview;

    }

  };



  template<>

  struct ImageFactory<ComplexImageView> {

    // data types

    typedef ComplexImageView::data_type data_type;

    typedef ImageData<ComplexImageView::value_type> dense_data_type;

    typedef ImageData<ComplexImageView::value_type> rle_data_type;

    // view types

    typedef ImageView<data_type> view_type;

    typedef ImageView<dense_data_type> dense_view_type;

    typedef ImageView<rle_data_type> rle_view_type;

    // cc types

    typedef ConnectedComponent<data_type> cc_type;

    typedef ConnectedComponent<dense_data_type> dense_cc_type;

    typedef ConnectedComponent<rle_data_type> rle_cc_type;

    typedef std::list<cc_type*> ccs_type;

    typedef std::list<dense_cc_type*> dense_ccs_type;

    typedef std::list<rle_cc_type*> rle_ccs_type;

    static view_type* new_view(const ComplexImageView& view) {

      view_type* nview = new view_type(*((data_type*)view.data()),

				      view.offset_y(), view.offset_x(),

				      view.nrows(), view.ncols());

      return nview;

    }

    static view_type* new_view(const ComplexImageView& view, size_t ul_y,

			       size_t ul_x, size_t nrows, size_t ncols) {

      view_type* nview = new view_type(*((data_type*)view.data()),

				       ul_y, ul_x, nrows, ncols);

      return nview;

    }

    static view_type* new_image(const ComplexImageView& view) {

      data_type* data = new data_type(view.nrows(), view.ncols(),

				      view.offset_y(), view.offset_x());

      view_type* nview = new view_type(*data,

				      view.offset_y(), view.offset_x(),

				      view.nrows(), view.ncols());

      return nview;

    }

  };



  /*

    TypeIdImageFactory



    This factory type can be used to easily create new images using the

    enums above.

  */

  template<int Pixel, int Storage>

  struct TypeIdImageFactory {



  };

  

  template<>

  struct TypeIdImageFactory<ONEBIT, DENSE> {

    typedef OneBitImageData data_type;

    typedef OneBitImageView image_type;

    static image_type* create(size_t offset_y, size_t offset_x,

			      size_t nrows, size_t ncols) {

      data_type* data = new data_type(nrows, ncols, offset_y, offset_x);

      return new image_type(*data, offset_y, offset_x, nrows, ncols);

    }

  };



  template<>

  struct TypeIdImageFactory<ONEBIT, RLE> {

    typedef OneBitRleImageData data_type;

    typedef OneBitRleImageView image_type;

    static image_type* create(size_t offset_y, size_t offset_x,

			      size_t nrows, size_t ncols) {

      data_type* data = new data_type(nrows, ncols, offset_y, offset_x);

      return new image_type(*data, offset_y, offset_x, nrows, ncols);

    }

  };

  

  template<>

  struct TypeIdImageFactory<GREYSCALE, DENSE> {

    typedef GreyScaleImageData data_type;

    typedef GreyScaleImageView image_type;

    static image_type* create(size_t offset_y, size_t offset_x,

			      size_t nrows, size_t ncols) {

      data_type* data = new data_type(nrows, ncols, offset_y, offset_x);

      return new image_type(*data, offset_y, offset_x, nrows, ncols);

    }

  };



  template<>

  struct TypeIdImageFactory<GREY16, DENSE> {

    typedef Grey16ImageData data_type;

    typedef Grey16ImageView image_type;

    static image_type* create(size_t offset_y, size_t offset_x,

			      size_t nrows, size_t ncols) {

      data_type* data = new data_type(nrows, ncols, offset_y, offset_x);

      return new image_type(*data, offset_y, offset_x, nrows, ncols);

    }

  };



  template<>

  struct TypeIdImageFactory<RGB, DENSE> {

    typedef RGBImageData data_type;

    typedef RGBImageView image_type;

    static image_type* create(size_t offset_y, size_t offset_x,

			      size_t nrows, size_t ncols) {

      data_type* data = new data_type(nrows, ncols, offset_y, offset_x);

      return new image_type(*data, offset_y, offset_x, nrows, ncols);

    }

  };





  template<>

  struct TypeIdImageFactory<COMPLEX, DENSE> {

    typedef ComplexImageData data_type;

    typedef ComplexImageView image_type;

    static image_type* create(size_t offset_y, size_t offset_x,

			      size_t nrows, size_t ncols) {

      data_type* data = new data_type(nrows, ncols, offset_y, offset_x);

      return new image_type(*data, offset_y, offset_x, nrows, ncols);

    }

  };



  template<>

  struct TypeIdImageFactory<FLOAT, DENSE> {

    typedef FloatImageData data_type;

    typedef FloatImageView image_type;

    static image_type* create(size_t offset_y, size_t offset_x,

			      size_t nrows, size_t ncols) {

      data_type* data = new data_type(nrows, ncols, offset_y, offset_x);

      return new image_type(*data, offset_y, offset_x, nrows, ncols);

    }

  };



}





#endif
