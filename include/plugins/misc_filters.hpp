/*
 *
 * Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, Karl MacMillan
 *               2010      Christoph Dalitz, Oliver Christen
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

#ifndef kwm11062002_misc_filters
#define kwm11062002_misc_filters

#include "gamera.hpp"
#include "image_utilities.hpp"
#include "neighbor.hpp"
#include "vigra/gaborfilter.hxx"
#include "convolution.hpp"

using namespace std;

namespace Gamera {

  //---------------------------
  // mean filter
  //---------------------------
  template<class T>
  class Mean {
  public:
    inline T operator() (typename vector<T>::iterator begin,
                         typename vector<T>::iterator end);
  };

  template<class T>
  inline T Mean<T>::operator() (typename vector<T>::iterator begin,
                                typename vector<T>::iterator end) {
    long sum = 0;
    size_t size = end - begin;
    for (; begin != end; ++begin)
      sum += size_t(*begin);
    return T(sum / double(size) + 0.5);
  }


  template<class T>
  typename ImageFactory<T>::view_type* mean(const T &src, unsigned int k=3, size_t border_treatment=0) {
    typedef typename ImageFactory<T>::data_type data_type;
    typedef typename ImageFactory<T>::view_type view_type;

    if (src.nrows() < k || src.ncols() < k)
      return simple_image_copy(src);

    data_type *new_data = new data_type(src.size(), src.origin());
    view_type *new_view = new view_type(*new_data);

    Mean<typename T::value_type> mean_op;
		
    if (border_treatment == 1) { // reflect

      // for reflection, we can use convolve plugin
      // because BORDER_TREATMENT_REFLECT is supported in VIGRA

      // create averaging kernel and do convolution
      FloatImageData *kernel_data = new FloatImageData( Dim(k,k), Point(0,0) );
      FloatImageView *kernel_view = new FloatImageView(*kernel_data);
      FloatPixel fp = 1.0f / (k*k);
      for(unsigned int y = 0 ; y < (*kernel_view).nrows() ; y++) {
        for(unsigned int x = 0 ; x < (*kernel_view).ncols(); x++) {
          (*kernel_view).set( Point(x,y), fp);
        }
      }
      new_view = convolve(src, *kernel_view, vigra::BORDER_TREATMENT_REFLECT);
      delete kernel_view->data();
      delete kernel_view;
    }
    else { // border treatment 'padwhite'

      unsigned int x, y; // window center coordinates
      int ul_x, ul_y; // upper left window coordinates
      int lr_x, lr_y; // lower right window coordinates
      unsigned int min_x, max_x, min_y, max_y;
      vector<typename T::value_type> window(k*k);
      typename T::value_type white_val = white(src);	

      for(y = 0 ; y < src.nrows() ; y++) {
        for(x = 0 ; x < src.ncols(); x++) {

          ul_x = (int)x - k/2;
          ul_y = (int)y - k/2;
          lr_x = (int)x + k/2;
          lr_y = (int)y + k/2;

          // window partially outside the image?
          if (ul_x < 0 || lr_x >= (int)src.ncols() || ul_y < 0 || lr_y >= (int)src.nrows()) {
            size_t i, _x, _y;
            min_x = std::max(0, ul_x);
            max_x = std::min((int)src.ncols()-1, lr_x);
            min_y = std::max(0, ul_y);
            max_y = std::min((int)src.nrows()-1, lr_y);
            i = 0; // position of entries does not matter for mean
            for (_x=min_x; _x <= max_x; _x++)
              for (_y=min_y; _y <= max_y; _y++) {
                window[i] = src.get(Point(_x,_y));
                i++;
              }
            // fill rest with white
            for (; i < k*k; i++) window[i] = white_val;
          }
          else {
            for(size_t i=0 ; i < k*k ; i++) {
              window[i] = src.get(Point(ul_x + (i%k), ul_y + (i/k)));
            }
          }
						
          (*new_view).set(Point(x, y), mean_op(window.begin(), window.end()));
        }
      }
    }

    return new_view;
  }

  //---------------------------
  // rank filter
  //---------------------------
  template<class T>
  class Rank {
    unsigned int rank;
  public:
    Rank<T>(unsigned int rank_) { rank = rank_ - 1; }
    inline T operator() (typename vector<T>::iterator begin,
			 typename vector<T>::iterator end);
  };

  template<class T>
  inline T Rank<T>::operator() (typename vector<T>::iterator begin,
				typename vector<T>::iterator end) {
    nth_element(begin, begin + rank, end);
    return *(begin + rank);
  }

  template<>
  inline OneBitPixel Rank<OneBitPixel>::operator() (vector<OneBitPixel>::iterator begin,
						    vector<OneBitPixel>::iterator end) {
    nth_element(begin, end - rank - 1, end);
    return *(end - rank - 1);
  }

  template<class T>
  typename ImageFactory<T>::view_type* rank(const T &src, unsigned int r, unsigned int k=3, size_t border_treatment=0) {
    typedef typename ImageFactory<T>::data_type data_type;
    typedef typename ImageFactory<T>::view_type view_type;

    if (src.nrows() < k || src.ncols() < k)
      return simple_image_copy(src);

    data_type *new_data = new data_type(src.size(), src.origin());
    view_type *new_view = new view_type(*new_data);

    Rank<typename T::value_type> rank(r);
		
    if(k==3) {
      if (border_treatment == 1) // reflect
        neighbor9reflection(src, rank, *new_view);
      else // clip
        neighbor9(src, rank, *new_view);
      return new_view;
    }
    unsigned int x, y; // window center coordinates
    int ul_x, ul_y; // upper left window coordinates
    int lr_x, lr_y; // lower right window coordinates
    unsigned int min_x, max_x, min_y, max_y;
    typename T::value_type white_val = white(src);
		
    for(y = 0 ; y < src.nrows() ; y++) {
      for(x = 0 ; x < src.ncols(); x++) {

        ul_x = (int)x - k/2;
        ul_y = (int)y - k/2;
        lr_x = (int)x + k/2;
        lr_y = (int)y + k/2;

        vector<typename T::value_type> window(k*k);

        // window partially outside the image?
        if (ul_x < 0 || lr_x >= (int)src.ncols() || ul_y < 0 || lr_y >= (int)src.nrows()) {

          if (border_treatment == 1) { // reflect
            int _x, _y;
            for (size_t i = 0 ; i < k*k ; i++) {
              _x = ul_x + i%k;
              _y = ul_y + i/k;
              if (_x < 0)
                _x = -_x;
              if (_x >= (int)src.ncols())
                _x = src.ncols() - (_x - src.ncols()) - 2;
              if (_y < 0)
                _y = -_y;
              if (_y >= (int)src.nrows())
                _y = src.nrows() - (_y - src.nrows()) - 2;
              window[i] = src.get(Point(_x, _y));
            }
          }
          else { // border treatment 'clip'
            size_t i, _x, _y;
            min_x = std::max(0, ul_x);
            max_x = std::min((int)src.ncols()-1, lr_x);
            min_y = std::max(0, ul_y);
            max_y = std::min((int)src.nrows()-1, lr_y);
            i = 0; // position of entries does not matter for rank
            for (_x=min_x; _x <= max_x; _x++)
              for (_y=min_y; _y <= max_y; _y++) {
                window[i] = src.get(Point(_x,_y));
                i++;
              }
            // fill rest with white
            for (; i < k*k; i++) window[i] = white_val;
          }
        }
        else {
          for(size_t i = 0 ; i < k*k ; i++) {
            window[i] = src.get(Point(ul_x + (i%k), ul_y + (i/k)));
          }
        }
						
        (*new_view).set(Point(x, y), rank( window.begin(), window.end()));
      }
    }
		
    return new_view;
  }


  //---------------------------
  // Gabor filter
  //---------------------------
  template<class T>
  Image* create_gabor_filter(const T& src, double orientation, double frequency, int direction) {

    FloatImageData* dest_data = new FloatImageData(src.size(), src.origin());
    FloatImageView* dest = new FloatImageView(*dest_data);

    image_copy_fill(src, *dest);

    try {
      vigra::createGaborFilter(dest_image_range(*dest), orientation, frequency,
			       vigra::angularGaborSigma(direction, frequency),
			       vigra::radialGaborSigma(frequency));
      
    } catch(std::exception e) {
      delete dest;
      delete dest_data;
      throw std::runtime_error("VIGRA function 'createGaborFilter' failed!");
    }

    return dest;

  }

  //---------------------------
  // kfill
  //---------------------------

  // count core ON pixel
  inline unsigned int kfill_count_core_pixel(OneBitImageView *tmp, int x, int y, Point *c_lr) {
    unsigned int core_pixel = 0;
    for( unsigned int cy = y ; cy <= c_lr->y() ; cy++ ) {
      for( unsigned int cx = x ; cx <= c_lr->x() ; cx++ ) {
        if( (*tmp).get( Point(cx, cy) ) == is_black(tmp)) {
          core_pixel++;
        }
      }
    }
    return core_pixel;
  }
  // set all core pixel to given value
  inline void kfill_set_core_pixel(OneBitImageView *res, int x, int y, Point *c_lr, int v) {
    for( unsigned int cy = y ; cy <= c_lr->y() ; cy++ ) {
      for( unsigned int cx = x ; cx <= c_lr->x() ; cx++ ) {
        (*res).set( Point(cx, cy), v);
      }
    }
  }
  // get n, r, c
  void kfill_get_condition_variables(OneBitImageView *tmp, int k, int x, int y, int size_x, int size_y, int *n, int *r, int *c) {
		
    // upper left corner of current window
    int ul_x;
    int ul_y;
    // upper right corner of current window
    int ur_x;
    int ur_y;
    // lower left corner of current window
    int ll_x;
    int ll_y;
    // lower right corner of current window
    int lr_x;
    int lr_y;

    int nnp = 4*(k-1); // total number of neighborhood pixels
    int* nh_pixel = new int[nnp]; // array for neighborhood pixel
    int nh_pixel_count = 0;
    int corner_pixel_count;
    int nh_ccs;
    OneBitPixel pixelvalue;
		
    // calculate window borders
    ul_x = ( x - 1 );
    ul_y = ( y - 1 );
    ur_x = ( x + k - 2 );
    ur_y = ( y - 1 );
    ll_x = ( x - 1 );
    ll_y = ( y + k - 2 );
    lr_x = ( x + k - 2 );
    lr_y = ( y + k - 2 );
										
    // fill array with neighborhood and count neighborhood ON pixel
    int i = 0;
    for( int ul_to_ur_np = ul_x ; ul_to_ur_np < ur_x ; ul_to_ur_np++ ) {
		
      if(ul_to_ur_np < 0 || y-1 < 0 ) {
        pixelvalue = 0;
      } else {
        pixelvalue = (*tmp).get( Point(ul_to_ur_np, y - 1) );
      }
			
      nh_pixel[i++] = is_black(pixelvalue);
      if (is_black(pixelvalue)) { nh_pixel_count++; }
    }

    for( int ur_to_lr_np = ur_y ; ur_to_lr_np < lr_y ; ur_to_lr_np++ ) {
		
      if(ur_to_lr_np < 0 || x + k - 2 > size_x - 1 ) {
        pixelvalue = 0;
      } else {
        pixelvalue = (*tmp).get( Point(x + k - 2, ur_to_lr_np) );
      }
			
      nh_pixel[i++] = is_black(pixelvalue);
      if (is_black(pixelvalue)) { nh_pixel_count++; }
    }
		
    for( int lr_to_ll_np = lr_x ; lr_to_ll_np > ll_x ; lr_to_ll_np-- ) {
		
      if( lr_to_ll_np > size_x - 1 || y + k - 2 > size_y - 1 ) {
        pixelvalue = 0;
      } else {
        pixelvalue = (*tmp).get( Point(lr_to_ll_np, y + k - 2) );
      }
			
      nh_pixel[i++] = is_black(pixelvalue);
      if (is_black(pixelvalue)) { nh_pixel_count++; }
    }
			
    for( int ll_to_ul_np = ll_y ; ll_to_ul_np > ul_y ; ll_to_ul_np-- ) {
		
      if(x - 1 < 0 || ll_to_ul_np > size_y - 1 ) {
        pixelvalue = 0;
      } else {
        pixelvalue = (*tmp).get( Point(x - 1, ll_to_ul_np) );
      }
			
      nh_pixel[i++] = is_black(pixelvalue);
      if (is_black(pixelvalue)) { nh_pixel_count++; }
    }
						
    // count corner ON pixel
    corner_pixel_count = nh_pixel[(k-1)*0] + nh_pixel[(k-1)*1] + nh_pixel[(k-1)*2] + nh_pixel[(k-1)*3];

    // get ccs in neighborhood
    nh_ccs = 0;
    for(int nhpixel = 0 ; nhpixel < i ; nhpixel++) {
      nh_ccs += abs( nh_pixel[(nhpixel+1)%nnp] - nh_pixel[nhpixel] );
    }
    nh_ccs /= 2;

    *n = nh_pixel_count;
    *r = corner_pixel_count;
    *c = nh_ccs;

    delete[] nh_pixel;
  }
	
  // the actual kfill implementation
  template<class T>
  OneBitImageView * kfill(const T &src, int k, int iterations) {
		
    //
    // create a copy of the original image
    // kfill algorithm sets pixel ON/OFF information in this image
    //
    OneBitImageData *res_data = new OneBitImageData( src.size(), src.origin() );
    OneBitImageView *res = new OneBitImageView(*res_data);
    image_copy_fill(src, *res);
		
    // kfill algorithm reads pixel ON/OFF information from this image
    OneBitImageData *tmp_data = new OneBitImageData( src.size(), src.origin() );
    OneBitImageView *tmp = new OneBitImageView(*tmp_data);
		
    bool changed; // pixels changed in an iteration
    int src_size_x = src.ncols(); // source image size x
    int src_size_y = src.nrows(); // source image size y
		
    int x, y; // windows position (upper left core coordinate)
    Point c_lr; // windows position (lower right core coordinate)
		
    int ncp = (k-2)*(k-2); // number of core pixel
    int core_pixel; // number of ON core pixel
		
    int r; // number of pixel in the neighborhood corners
    int n; // number of neighborhood pixel
    int c; // number of ccs in neighborhood
				
    while(iterations) {
			
      // create a copy from the result image (result of previous iteration or original at first iteration)
      image_copy_fill(*res, *tmp);
			
      // reset changed pixel
      changed =  false;
			
      // move window over the image
      for(y = 0 ; y < src_size_y - (k-3) ; y++) {
        for(x = 0 ; x < src_size_x - (k-3) ; x++) {
					
          // calculate lower right core coordinate
          c_lr.x( x + (k - 3) );
          c_lr.y( y + (k - 3) );

          // count core ON pixel
          core_pixel = kfill_count_core_pixel(tmp, x, y, &c_lr);

          //
          // ON filling requires ALL core pixels to be OFF
          //
          if(core_pixel == 0) {
            // get condition variables
            kfill_get_condition_variables(tmp, k, x, y, src_size_x, src_size_y, &n, &r, &c);
            // condition check
            if( (c <= 1) && ( (n > 3*k - 4) || (n == 3*k - 4) && (r == 2) ) ) {
              kfill_set_core_pixel(res, x, y, &c_lr, 1);
              changed = true;
            }
          }
					
          //
          // OFF filling requires ALL core pixels to be ON
          //				
          if(core_pixel == ncp) {
            kfill_get_condition_variables(tmp, k, x, y, src_size_x, src_size_y, &n, &r, &c);
            n = ( 4*(k-1) ) - n;
            r = 4 - r;
            // condition check
            if( (c <= 1) && ( (n > 3*k - 4) || (n == 3*k - 4) && (r == 2) ) ) {
              kfill_set_core_pixel(res, x, y, &c_lr, 0);
              changed = true;
            }
          }
										
        } // end for x
      } // end for y
						
      if(!changed) {
        break;
      }
			
      iterations--;
    } // end while
		
    delete tmp->data();
    delete tmp;
		
    return res;
  }


  template<class T>
  OneBitImageView * kfill_modified(const T &src, int k) {
    /*
      create a copy of the original image
      kfill algorithm sets pixel ON/OFF information in this image
    */
    OneBitImageData *res_data = new OneBitImageData( src.size(), src.origin() );
    OneBitImageView *res = new OneBitImageView(*res_data);
		
    OneBitImageData *tmp_data = new OneBitImageData( src.size(), src.origin() );
    OneBitImageView *tmp = new OneBitImageView(*tmp_data);
    image_copy_fill(src, *tmp);
		
    int src_size_x = src.ncols(); // source image size x
    int src_size_y = src.nrows(); // source image size y
		
    int x, y; // windows position (upper left core coordinate)
    Point c_lr; // windows position (lower right core coordinate)
		
    int ncp = (k-2) * (k-2);
    float ncp_required = ncp / 2.0f; // number of core pixel required -- modified version
		
    int core_pixel; // number of ON core pixel
		
    int r; // number of pixel in the neighborhood corners
    int n; // number of neighborhood pixel
    int c; // number of ccs in neighborhood
		
    // move window over the image
    for(y = 0 ; y < src_size_y - (k-3) ; y++) {
      for(x = 0 ; x < src_size_x - (k-3) ; x++) {
        // calculate lower right core coordinate
        c_lr.x( x + (k - 3) );
        c_lr.y( y + (k - 3) );
				
        // count core ON pixel
        core_pixel = kfill_count_core_pixel(tmp, x, y, &c_lr);
				
        // ON >= (k-2)^2/2 ?
        if(core_pixel >= ncp_required) {
					
          // Examine in the Neighborhood
          kfill_get_condition_variables(tmp, k, x, y, src_size_x, src_size_y, &n, &r, &c);
          n = ( 4*(k-1) ) - n;
          r = 4 - r;
					
          // eq. satisfied?
          if( (c <= 1) && ( (n > 3*k - 4) || (n == 3*k - 4) && (r == 2) ) ) {
            kfill_set_core_pixel(res, x, y, &c_lr, 0);
          } else {
            kfill_set_core_pixel(res, x, y, &c_lr, 1);
          }			
					
        } else {
					
          // Examine in the Neighborhood
          kfill_get_condition_variables(tmp, k, x, y, src_size_x, src_size_y, &n, &r, &c);

          // eq. satisfied?					
          if( (c <= 1) && ( (n > 3*k - 4) || (n == 3*k - 4) && (r == 2) ) ) {
            kfill_set_core_pixel(res, x, y, &c_lr, 1);
          } else {
            kfill_set_core_pixel(res, x, y, &c_lr, 0);
          }

        }
								
      } // end for x
    } // end for y
		
    delete tmp->data();
    delete tmp;

    return res;
  }

}

#endif

