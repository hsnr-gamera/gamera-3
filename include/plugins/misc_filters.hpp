/*
 *
 * Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, Karl MacMillan
 *               2010      Christoph Dalitz, Oliver Christen
 *               2011-2012 Christoph Dalitz, David Kolanus
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

#ifndef kwm11062002_misc_filters
#define kwm11062002_misc_filters

#include "gamera.hpp"
#include "image_utilities.hpp"
#include "neighbor.hpp"
#include "vigra/gaborfilter.hxx"
#include "convolution.hpp"
#include <math.h>

using namespace std;

namespace Gamera {

  //---------------------------
  // min/max filter
  //---------------------------
  template<class T>
  typename ImageFactory<T>::view_type* min_max_filter(const T &src, unsigned int k_h=3, int filter=0, unsigned int k_v=0){

    typedef typename ImageFactory<T>::data_type data_type;
    typedef typename ImageFactory<T>::view_type view_type;
    typedef typename T::value_type T_value_type;

    T_value_type defaultval;
    const T_value_type &(*opt)(const T_value_type&,const T_value_type&);

    if(filter==0){
      opt = std::min<T_value_type>;
      defaultval = std::numeric_limits<T_value_type>::max();
    } else {
      opt = std::max<T_value_type>;
      defaultval = std::numeric_limits<T_value_type>::min();
    }

    if(k_v==0)
      k_v=k_h;
    if (src.nrows() < k_v || src.ncols() < k_h)
      return simple_image_copy(src);


    data_type *res_data = new data_type(src.size(), src.origin());
    view_type *res= new view_type(*res_data);
    image_copy_fill(src, *res);

    unsigned int src_nrows = src.nrows();
    unsigned int src_ncols = src.ncols();
    unsigned int shiftsize_v = ((k_v-1)/2);
    unsigned int shiftsize_h = ((k_h-1)/2);
    unsigned int max_length = std::max(src_nrows, src_ncols);
    unsigned int max_shift = std::max(shiftsize_v, shiftsize_h);
    unsigned int k,i,j;
    unsigned int loc_max;


    T_value_type* g_long = new T_value_type[(max_length + max_shift)];
    T_value_type* h_long = new T_value_type[(max_length + max_shift)];

    // init for horizontal processing
    for(k=0; k<shiftsize_h; k++){
      g_long[src_ncols + k] = defaultval;
      h_long[k]=defaultval;
	}

    T_value_type* g = g_long;
    T_value_type* h = h_long+shiftsize_h;


	//Horizontal max
	for(j=0; j<src_nrows; j++){ // x=row, y=col

      //calc subarray g
      for( i=0 ; i<src_ncols ; i+=k_h ){

        g[i] = src.get(Point(i,j));

        for(k=1; k<k_h && (i+k)<src_ncols ; k++){
          g[i+k]=opt(src.get(Point(i+k,j)), g[i+k-1]);
        }
      }

      //calc subarray h
      for( i=0 ; i<src_ncols ; i+=k_h ){
        loc_max = i + k_h;
        loc_max = (src_ncols<loc_max)?src_ncols:loc_max;

        h[loc_max-1] = src.get(Point(loc_max-1,j));

        for(k=2; k<=k_h; k++){
          h[loc_max-k]=opt(src.get(Point(loc_max-k,j)), h[loc_max-k+1]);
        }

      }

      // combine g and h
      for(i=0; i<src_ncols; i++){
        res->set(Point(i,j), opt(g_long[i+shiftsize_h], h_long[i]));
      }
	}

	//init for vertical prozessing
    for(k=0; k<shiftsize_v; k++){
      g_long[src_nrows + k] = defaultval;
      h_long[k]=defaultval;
	}

    g = g_long;
    h = h_long+shiftsize_v;

	//Vertical Max
	for(j=0; j<src_ncols; j++){ // x=row, y=col

      //calc subarray g
      for( i=0 ; i<src_nrows ; i+=k_v ){

        g[i] = res->get(Point(j,i));

        for(k=1; k<k_v && (i+k)<src_nrows ; k++){
          g[i+k]=opt(res->get(Point(j,i+k)), g[i+k-1]);
        }
      }

      //calc subarray h
      for( i=0 ; i<src_nrows ; i+=k_v ){
        loc_max = i + k_v;
        loc_max = (src_nrows<loc_max)?src_nrows:loc_max;

        h[loc_max-1] = res->get(Point(j, loc_max-1));

        for(k=2; k<=k_v; k++){
          h[loc_max-k]=opt(res->get(Point(j, loc_max-k)), h[loc_max-k+1]);
        }

      }

      // combine g and h
      for(i=0; i<src_nrows; i++){
        res->set(Point(j,i), opt(g_long[i+shiftsize_v], h_long[i]));
      }
	}

	delete[] g_long;
	delete[] h_long;

    return res;
  }

  //---------------------------------------------------------------
  // Parametrization of border treatment (padwhite versus reflect)
  //---------------------------------------------------------------
  template<class T>
  class GetPixel4Border{
    const T& _src;
    int _src_ncols;
    int _src_nrows;
    int _border_treatment; // 0=padwhite, 1=reflect
    typename T::value_type _white_val;
    unsigned int _k;

  public:
    GetPixel4Border<T>(const T &src, int border_treatment, unsigned int k):_src(src){
	  _src_ncols=src.ncols();
	  _src_nrows=src.nrows();
	  _border_treatment=border_treatment;
	  _white_val = white(src);
	  _k = k;
    }

    typename T::value_type operator() (int column, int row) {
      // window partially outside the image?
      if (column < 0 || column >= _src_ncols || row < 0 || row >= _src_nrows) {
        if(_border_treatment==1) {
          if (column < 0)
            column = -column;
          if (column >= _src_ncols)
            column = _src_ncols - (column -_src_ncols) - 2;
          if (row < 0)
            row = -row;
          if (row >= _src_nrows)
            row = _src_nrows - (row - _src_nrows) - 2;
        }
        else {
          return _white_val;
        }
      }
      return _src.get(Point(column, row));
    }
  };

  //----------------------------------------------
  // mean filter (David Kolanus)
  //----------------------------------------------
  template<class T>
  typename ImageFactory<T>::view_type* mean(const T &src, unsigned int k=3, size_t border_treatment=1) {
    typedef typename ImageFactory<T>::data_type data_type;
    typedef typename ImageFactory<T>::view_type view_type;
    typedef typename T::value_type T_value_type;

    if (src.nrows() < k || src.ncols() < k)
      return simple_image_copy(src);


    data_type *res_data = new data_type(src.size(), src.origin());
    view_type *res= new view_type(*res_data);

    int src_ncols = src.ncols();
    int src_nrows = src.nrows();

    double window_sum=0.0;
    double kk = 1.0/double(k*k);

    //(col,row)
    int column=0;
    int row=0;
	int r = (k-1)/2;

	//row/column are now center
	GetPixel4Border<T> gp(src, border_treatment, k);
	int ci, ri;
	int r_p=r;
	int r_m=-r;

	for(row=0; row<src_nrows; row++){
      column=0;

      //init sum
      window_sum=0.0;
      for(ri=r_m; ri<=r_p; ri++) {
        for(ci=r_m; ci<=r_p; ci++) {
          window_sum += gp(column+ci, row+ri);
        }
      }

      //calc mean
      res->set( Point(column,row), T_value_type(window_sum*kk + 0.5));

      //go right column....
      for(column=1; column<src_ncols; column++) {
        for(ci=r_m; ci<=r_p; ci++) {
          //sub
          window_sum -= gp(column-1-r,row+ci);

          //add
          window_sum += gp(column+r,row+ci);
        }

        //calc mean
        res->set( Point(column,row), T_value_type(window_sum*kk + 0.5));
      }
	}
    return res;
  }

  //----------------------------------------------------------------
  // rank filter (Christoph Dalitz and David Kolanus)
  //----------------------------------------------------------------
  template<class T>
  class RankHist {
  public:
    unsigned int* hist;
    unsigned int histsize;
    RankHist<T>() {
      unsigned int color;
      histsize = (unsigned int)pow(2.0,8.0*sizeof(T));
      hist = new unsigned int[histsize];
      for(color=0; color<histsize; color++){
        hist[color]=0;
      }
    };
    ~RankHist<T>() { delete[] hist; };
    inline unsigned int operator() (unsigned int r, unsigned int k2);
  };

  template<>
  RankHist<Grey16Pixel>::RankHist() {
    unsigned int color;
    histsize = 65536; // only 16bit
    hist = new unsigned int[histsize];
    for(color=0; color<histsize; color++){
      hist[color]=0;
    }
  }

  template<class T>
  inline unsigned int RankHist<T>::operator() (unsigned int r, unsigned int k2) {
	unsigned int collect = 0;
    unsigned int color;
	for(color=0; color<histsize; color++){
      collect += hist[color];
      if(collect>=r){
        return color;
      }
	}
	return color;
  }

  template<>
  inline unsigned int RankHist<OneBitPixel>::operator() (unsigned int r, unsigned int k2) {
	unsigned int collect = 0;
    unsigned int color;
	collect = 0;
	for(color=0; color<histsize; color++){
      collect += hist[color];
      if(collect>=k2-r+1){
        return color;
      }
	}
	return color;
  }

  template<class T>
  typename ImageFactory<T>::view_type* rank (const T &src, unsigned int rank, unsigned int k=3, size_t border_treatment=1) {
    typedef typename ImageFactory<T>::data_type data_type;
    typedef typename ImageFactory<T>::view_type view_type;
    typedef typename T::value_type T_value_type;

    if (src.nrows() < k || src.ncols() < k)
      return simple_image_copy(src);

    data_type *res_data = new data_type(src.size(), src.origin());
    view_type *res= new view_type(*res_data);
    //image_copy_fill(src, *res);

    int src_ncols = (int)src.ncols();
    int src_nrows = (int)src.nrows();

    //(col,row)
    int column=0;
    int row;
    unsigned int color;
	int r = (k-1)/2;

	//rank class
	RankHist<T_value_type> rk;

	//row/column are now center
	GetPixel4Border<T> gp(src, border_treatment, k);
	int ci, ri;
	int r_p=r;
	int r_m=-r;

	for(row=0; row<src_nrows; row++){
      column=0;

      for(color=0; color<rk.histsize; color++){
        rk.hist[color]=0;
      }
      //init hist
      for(ri=r_m; ri<=r_p; ri++) {
        for(ci=r_m; ci<=r_p; ci++) {
          rk.hist[gp(column+ci, row+ri)]++;
        }
      }

      //calc median
      res->set( Point(column,row), T_value_type(rk(rank,k*k)));

      //go right column....
      for(column=1; column<src_ncols; column++) {
        for(ci=r_m; ci<=r_p; ci++) {
          //sub
          rk.hist[gp(column-1-r,row+ci)]--;

          //add
          rk.hist[gp(column+r,row+ci)]++;
        }

        //calc median
        res->set( Point(column,row), T_value_type(rk(rank,k*k)));
      }
	}

    return res;
  }

  // specialization for FloatImage,
  // because here the histogram based approach does not work
  template<>
  FloatImageView* rank (const FloatImageView &src, unsigned int rank, unsigned int k, size_t border_treatment) {

    if (src.nrows() < k || src.ncols() < k)
      return simple_image_copy(src);

    int x,y;
    size_t i;
    FloatImageData *res_data = new FloatImageData(src.size(), src.origin());
    FloatImageView *res= new FloatImageView(*res_data);

	GetPixel4Border<FloatImageView> gp(src, border_treatment, k);
    vector<FloatPixel> window(k*k);
    
    int radius = (k-1)/2;
    for(y = 0 ; (size_t)y < src.nrows() ; y++) {
      for(x = 0 ; (size_t)x < src.ncols(); x++) {
        for(i = 0 ; i < k*k ; i++) {
          window[i] = gp(x - radius + (i%k), y - radius + (i/k));
        }
        nth_element(window.begin(), window.begin() + rank, window.end());
        res->set(Point(x,y), *(window.begin()+rank));
      }
    }
    return res;
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

