/*
 *
 * Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, Karl MacMillan
 *               2010-2016 Christoph Dalitz
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
 */
#ifndef _kwm12032001_deformations
#define _kwm12032001_deformations

#include "plugins/image_utilities.hpp"
#include "vigra/resizeimage.hxx"
#include "vigra/affinegeometry.hxx"
#include "plugins/logical.hpp"
#include "plugins/morphology.hpp"

#include <exception>
#include <cstdlib>
#include <ctime>
#include <math.h>
#include <time.h>
#include <algorithm>

// for backward compatibility:
// plugin rotate has been moved from here to transformation.hpp
#include "transformation.hpp"

namespace Gamera {

inline RGBPixel norm_weight_avg(RGBPixel& pix1, RGBPixel& pix2, double w1=1.0, double w2=1.0)
{
  if(w1 == -w2) w1 = w2 = 1.0;
  return RGBPixel( GreyScalePixel(((pix1.red() * w1) + (pix2.red() * w2))/(w1 + w2)),
		   GreyScalePixel(((pix1.green() * w1) + (pix2.green() * w2))/(w1 + w2)),
		   GreyScalePixel(((pix1.blue() * w1) + (pix2.blue() * w2))/(w1 + w2)));
}

inline OneBitPixel norm_weight_avg(OneBitPixel& pix1, OneBitPixel& pix2, double w1=1.0, double w2=1.0)
{
  if(w1 == -w2) w1 = w2 = 1.0;
  if(((pix1 * w1) + (pix2 * w2))/(w1 + w2) < 0.5) return OneBitPixel(0);
  return OneBitPixel(1);
}

template <class T>
inline T norm_weight_avg(T& pix1, T& pix2, double w1=1.0, double w2=1.0)
{
  return T(((pix1 * w1) + (pix2 * w2))/(w1 + w2));
}

inline void filterfunc(RGBPixel &p0, RGBPixel &p1, RGBPixel &oldPixel, RGBPixel origPixel, double &weight) {
  p0 = origPixel;
  p1 = RGBPixel(  GreyScalePixel(p0.red() * weight),
		  GreyScalePixel(p0.green() * weight),
		  GreyScalePixel(p0.blue() * weight));
  p0 = RGBPixel(  GreyScalePixel(p0.red() - p1.red() + oldPixel.red()),
		  GreyScalePixel(p0.green() - p1.green() + oldPixel.green()),
		  GreyScalePixel(p0.blue() - p1.blue() + oldPixel.blue()));
  oldPixel = p1;
}

template<class T>
inline void filterfunc(T &p0, T &p1, T &oldPixel, T origPixel, double & weight)
{
  p0 = origPixel;
  p1 = (T)(p0 * weight);
  p0 -= (T)(p1 - oldPixel);
  oldPixel = p1;
}

/*
 * borderfunc
 *
 * Corrects the alpha blending of border pixels.
 *
 * p0 - Value 1
 * p1 - Value 2
 * oldPixel - Value3
 * origPixel - Pixel that comes from the source
 * weight - The weight given to the filter
 * bgcolor - The background color
 *
 */
template<class T>
inline void borderfunc(T& p0, T& p1, T& oldPixel, T origPixel, double& weight, T bgcolor)
{
  filterfunc(p0, p1, oldPixel, origPixel, weight);
  p0 = norm_weight_avg(bgcolor, origPixel, weight, 1.0-weight);
}

/*
 *  shear_x
 *  Shears the image horizontally with a shear angle in degrees.
 *
 *    orig_view: The original image view.
 *
 *    new_view: The image view after some rotation leaving a remaining
 *              rotation less than 45 degrees
 *
 *    row: Row number   
 *
 *    shift: Shift.
 *
 *    bgcolor: Background color
 *
 *    diff: Correction factor for negative shear values.
 */
template<class T, class U>
void shear_x(const T &orig, U &newbmp, size_t &row, size_t shiftAmount, typename T::value_type bgcolor, double weight, size_t diff=0)
{
  typedef typename ImageFactory<T>::view_type::value_type pixelFormat;
  size_t i=0, sourceshift=0;
  size_t width1 = newbmp.ncols();
  pixelFormat p0 = bgcolor, p1 = bgcolor, oldPixel = bgcolor;

  if (shiftAmount >= diff) {
    shiftAmount -= diff;
  }
  else {
      sourceshift = diff - shiftAmount;
      shiftAmount = 0;
  }
  
  for (; i<shiftAmount; i++) {
    if (i < width1)
      newbmp.set(Point(i, row), bgcolor);  //leading background
  }
  
  borderfunc(p0, p1, oldPixel, orig.get(Point(i-shiftAmount+sourceshift, row)), weight, bgcolor);
  
  newbmp.set(Point(i, row), p0);
  i++;

  for(; i<orig.ncols() + shiftAmount - sourceshift; i++) {
    filterfunc(p0, p1, oldPixel, orig.get(Point(i-shiftAmount+sourceshift, row)), weight);
    if ((i>=0) && (i<width1))
      newbmp.set(Point(i, row), p0);
  }
  weight=1.0-weight;
  
  if (i<width1)
    newbmp.set(Point(i++, row), norm_weight_avg(bgcolor, p0, weight, 1.0-weight));
  
  for(; i<width1; i++)
    newbmp.set(Point(i, row), bgcolor); //trailing background
}

/*
 *  shear_y
 *  Shears the image vertically with a shear angle in degrees.
 *
 *    orig_view: The original image view.
 *
 *    new_view: The image view after some rotation leaving a remaining
 *              rotation less than 45 degrees
 *
 *    shift: 
 *
 *    bgcolor: Background color
 *
 *    diff:
 */

template<class T, class U>
void shear_y(const T& orig, U& newbmp, size_t &col, size_t shiftAmount, typename T::value_type bgcolor, double weight, size_t diff=0)
{
  typedef typename ImageFactory<T>::view_type::value_type pixelFormat;
  size_t i, sourceshift=0;
  
  if (shiftAmount >= diff) {
    shiftAmount -= diff;
  }
  else {
      sourceshift = diff-shiftAmount;
      shiftAmount = 0;
  }
  
  size_t height1 = newbmp.nrows();
  
  for(i = 0; i<shiftAmount; i++) {
    if(i< height1)
      newbmp.set(Point(col, i), bgcolor);  //leading background
  }

  pixelFormat p0=bgcolor, p1=bgcolor, oldPixel=bgcolor;
  
  borderfunc(p0, p1, oldPixel, orig.get(Point(col, i-shiftAmount+sourceshift)), weight, bgcolor);
  newbmp.set(Point(col, i), p0);
  i++;

  for(; i<shiftAmount + orig.nrows() - sourceshift; i++) {
    if (i + sourceshift >= shiftAmount)
      filterfunc(p0, p1, oldPixel, orig.get(Point(col, i - shiftAmount + sourceshift)),weight);
   
    if ((i>=0) && (i<height1))
      newbmp.set(Point(col, i), p0);
  }
  
  if (i<height1)
    newbmp.set(Point(col, i++), norm_weight_avg(p0, bgcolor, weight, 1.0-weight));
  
  for(; i<height1; i++)
    newbmp.set(Point(col, i), bgcolor); //trailing background
}




inline double sin2(float per, int n)
{
  if(per==0) return 1;
  return sin(2.0*M_PI*(double)n/per);
}

inline double square(float per, int n)
{
  size_t n1 = n%(int)floor(per+0.5);
  
  if(n1<(per/2)) return -1;
  
  return 1;
}

inline double sawtooth(float per, int n)
{
  return 1.0 - 2*(double)abs((n%(size_t)per)-per)/per;
}

inline double triangle(float per, int n)
{
  size_t n1 = n%(size_t)per;
  float quarter = per/4.0;
  if(n1<(3*quarter) && n1>(quarter))
    return 1.0 - 4.0*(n1-quarter)/per;
  
  if(n1<=quarter)
    return (4*n1/per);
  
  return -1.0+ 4.0*((n1-3*quarter)/per);
}

inline double sinc(float per, int n)
{
  if(n==0) return 1.0;
  return sin2(per,n)*per/(2*M_PI*n);
}

inline double noisefunc(void)
{
  return -1.0 + (2.0 * rand()/(RAND_MAX+1.0));
}

inline size_t expDim(size_t amp)
{
  return amp;
}

inline size_t noExpDim(size_t)
{
  return 0;
}

inline size_t doShift(size_t amplitude, double amt)
{
  return (size_t)(((1+amplitude)/2)*(1-amt));
}

inline size_t noShift(size_t, double)
{
  return 0;
}

template<class T>
typename ImageFactory<T>::view_type* wave(const T &src, int amplitude, float freq, int direction, int funcType, int offset, double turbulence, long random_seed=0)
{

  typedef typename ImageFactory<T>::data_type data_type;
  typedef typename ImageFactory<T>::view_type view_type;

  typedef typename T::value_type pixelFormat;

  srand(random_seed);

  pixelFormat background = pixelFormat(0.0);
  
  data_type* new_data;
  view_type* new_view;

  size_t (*vertExpand)(size_t), (*horizExpand)(size_t);
  
  double (*waveType)(float, int);

  if (direction) {
    vertExpand = &noExpDim;
    horizExpand = &expDim;
  }
  else {
    vertExpand = &expDim;
    horizExpand = &noExpDim;
  }
  
  switch(funcType) {
  case 0:
    waveType = &sin2;
    break;
  case 1:
    waveType = &square;
    break;
  case 2:
    waveType = &sawtooth;
    break;
  case 3:
    waveType = &triangle;
      break;
  case 4:
    waveType = &sinc;
    break;
  default:
    waveType = &sin2;
  }
  
  new_data = new data_type(Dim(src.ncols()+horizExpand(amplitude),
			       src.nrows()+vertExpand(amplitude)),
			   src.origin());
  new_view = new view_type(*new_data);

  //image_copy_fill(src, *new_view);  Dimensions must match..

  try {
    typedef typename T::const_row_iterator IteratorI;
    typedef typename view_type::row_iterator IteratorJ;
    IteratorI ir = src.row_begin();
    IteratorJ jr = new_view->row_begin();
    
    for (; ir != src.row_end(); ++ir, ++jr) {
      typename IteratorI::iterator ic = ir.begin();
      typename IteratorJ::iterator jc = jr.begin();
      for (; ic != ir.end(); ++ic, ++jc) {
	*jc = *ic;
      }
    }
    
    
    if (direction) {
      for(size_t i=0; i<new_view->nrows(); i++) {
	double shift = ((double)amplitude/2)*(1-waveType(freq,(int)i-offset))+(turbulence*(rand()/RAND_MAX))+(turbulence/2.0);
	shear_x(src, *new_view, i, (size_t)(floor(shift)), background, (double)(shift-floor(shift)));
      }
    }
    else {
      for(size_t i=0; i<new_view->ncols(); i++) {
	double shift = ((double)amplitude/2)*(1-waveType(freq,(int)i-offset))+(turbulence*(rand()/RAND_MAX))+(turbulence/2.0);
	shear_y(src, *new_view, i, (size_t)(floor(shift)), background, (double)(shift - (size_t)(shift)));
      }
    }
    
    image_copy_attributes(src, *new_view);
  } catch (std::exception e) {
    delete new_view;
    delete new_data;
    throw;
  }

  return new_view;
}


template<class T>
typename ImageFactory<T>::view_type* noise(const T &src, int amplitude, int direction, long random_seed = 0)
{

  typedef typename ImageFactory<T>::data_type data_type;
  typedef typename ImageFactory<T>::view_type view_type;

  typedef typename T::value_type pixelFormat;
  pixelFormat background = src.get(Point(0, 0));

  //image_copy_fill(src, *new_view);

  srand(random_seed);

  size_t (*vertExpand)(size_t), (*horizExpand)(size_t), (*vertShift)(size_t, double), (*horizShift)(size_t, double);
  
  if (direction) {
    vertExpand = &expDim;
    horizExpand = &noExpDim;
    
    vertShift = &doShift;
    horizShift = &noShift;
  }
  else {
    vertExpand = &noExpDim;
    horizExpand = &expDim;

    vertShift = &noShift;
    horizShift = &doShift;
  }

  data_type* new_data = new data_type(Dim(src.ncols()+horizExpand(amplitude),
					  src.nrows()+vertExpand(amplitude)),
				      src.origin());
  
  view_type* new_view = new view_type(*new_data);

  try {
    
    // Iterator initialization
    typedef typename T::const_row_iterator IteratorI;
    typedef typename view_type::row_iterator IteratorJ;
    IteratorI ir = src.row_begin();
    IteratorJ jr = new_view->row_begin();
    
    // In some version of gamera you can use: fill(new_view, background);
    for (; ir != src.row_end(); ++ir, ++jr) {
      typename IteratorI::iterator ic = ir.begin();
      typename IteratorJ::iterator jc = jr.begin();
      for (; ic != ir.end(); ++ic, ++jc) {
	*jc = background;
      }
    }  
    
    for(size_t i = 0; i<src.nrows(); i++) {
      for(size_t j = 0; j<src.ncols();j++) {
	new_view->set(Point(j+horizShift(amplitude,noisefunc()),
			    i+vertShift(amplitude,noisefunc())),
		      src.get(Point(j, i)));
      }
    }
  } catch (std::exception e) {
    delete new_view;
    delete new_data;
    throw;
  }
  
  return new_view;
}

/*
 * Inkrub
 */

template<class T>
typename ImageFactory<T>::view_type* inkrub(const T &src, int a, long random_seed=0) {

  typedef typename ImageFactory<T>::data_type data_type;
  typedef typename ImageFactory<T>::view_type view_type;

  typedef typename T::value_type pixelFormat;
  // pixelFormat background = src.get(0 ,0);

  data_type* new_data = new data_type(src.dim(), src.origin());
  view_type* new_view = new view_type(*new_data);

  try {
    // Iterator initialization
    typedef typename T::const_row_iterator IteratorI;
    typedef typename view_type::row_iterator IteratorJ;
    IteratorI ir = src.row_begin();
    IteratorJ jr = new_view->row_begin();
    
    image_copy_fill(src, *new_view);
    
    srand(random_seed);
    
    for (int i=0; ir != src.row_end(); ++ir, ++jr, i++) {
      typename IteratorI::iterator ic = ir.begin();
      typename IteratorJ::iterator jc = jr.begin();
      for (int j=0; ic != ir.end(); ++ic, ++jc, j++) {
	pixelFormat px2 = *ic;
	pixelFormat px1 = src.get(Point(new_view->ncols()-j-1, i));
	if ((a*rand()/RAND_MAX) == 0)
	  *jc = norm_weight_avg(px1, px2, 0.5, 0.5);
      }
    }
    
    image_copy_attributes(src, *new_view);
  } catch (std::exception e) {
    delete new_view;
    delete new_data;
    throw;
  }

  return new_view;
}

inline double dist(double i0, double j0, double i1, double j1)
{
  double quadI = pow(i1-i0,2.0), quadJ = pow(j1-j0,2.0);
  return sqrt(quadI + quadJ);
}	

template<class T>
typename ImageFactory<T>::view_type* ink_diffuse(const T &src, int type, double dropoff, long random_seed=0)
{
  
  typedef typename ImageFactory<T>::data_type data_type;
  typedef typename ImageFactory<T>::view_type view_type;

  typedef typename T::value_type pixelFormat;
  // pixelFormat background = src.get(0, 0);

  data_type* new_data = new data_type(src.dim(), src.origin());
  view_type* new_view = new view_type(*new_data);

  try {
    // Iterator initialization
    typedef typename T::const_row_iterator IteratorI;
    typedef typename view_type::row_iterator IteratorJ;
    IteratorI ir = src.row_begin();
    IteratorJ jr = new_view->row_begin();
    
    double val, expSum;
    pixelFormat aggColor = pixelFormat();
    pixelFormat currColor = pixelFormat();
    
    srand(random_seed);
    
    if (type == 0) {
      
      for (int j=0; ir != src.row_end(); ++ir, ++jr, j++) {
	typename IteratorI::iterator ic = ir.begin();
	typename IteratorJ::iterator jc = jr.begin();
	aggColor = *ir;
	expSum = 0;
	for (; ic != ir.end(); ++ic, ++jc) {
	  val = 1.0/exp((double)j/dropoff);
	  expSum += val;
	  currColor = *ic;
	  double weight = val / (val + expSum);
	  aggColor = norm_weight_avg(aggColor, currColor, 1-weight, weight);
	  *jc = norm_weight_avg(aggColor, currColor, val, 1.0-val);
	}
      }
    }
    else if(type == 1) {
      
      for (int i=0; ir != src.row_end(); ++ir, ++jr, i++) {
	typename IteratorI::iterator ic = ir.begin();
	typename IteratorJ::iterator jc = jr.begin();
	aggColor = src.get(Point(i, 0));
	expSum = 0;
	for (int j=0; ic != ir.end(); ++ic, ++jc, j++) {
	  val = 1.0/exp((double)j/dropoff);
	  expSum += val;
	  currColor = *ic;
	  double weight = val / (val + expSum);
	  aggColor = norm_weight_avg(aggColor, currColor, 1-weight, weight);
	  new_view->set(Point(i, j), norm_weight_avg(aggColor, currColor, val, 1.0-val));
	}
      }
    }
    else if (type == 2) {
      
      typename T::const_vec_iterator srcIter = src.vec_begin();
      typename view_type::vec_iterator destIter = new_view->vec_end();
      
      for(; srcIter != src.vec_end(); ++srcIter, --destIter) {
	*destIter = *srcIter;
      }
      
      size_t starti, startj;
      double iD, jD;
      iD = (double)src.ncols() * (double)rand() / (double)RAND_MAX;
      starti = (unsigned int)(floor(iD));
      jD = (double)src.nrows() * (double)rand() / (double)RAND_MAX;
      startj = (unsigned int)(floor(jD));
      
      while( ( (iD>0) && (iD < src.ncols())) && ( (jD>0) && (jD<src.nrows()) ) ) {
	expSum = 0;
	val = 1.0/exp(dist((double)starti, (double)startj, iD, jD)/dropoff);
	expSum += val;
	currColor = new_view->get(Point(size_t(floor(iD)), size_t(floor(jD))));
	double weight = val / (val + expSum);
	aggColor = norm_weight_avg(aggColor, currColor, 1-weight, weight);
	new_view->set(Point(size_t(floor(iD)), size_t(floor(jD))), 
		      norm_weight_avg(aggColor, currColor, 1.0-val, val));
	iD += sin(2.0*M_PI*rand()/(double)RAND_MAX);
	jD += cos(2.0*M_PI*rand()/(double)RAND_MAX);
      }
    }
    
    image_copy_attributes(src, *new_view);
  } catch (std::exception e) {
    delete new_view;
    delete new_data;
    throw;
  }
  return new_view;
}


/*
 * Image degradation after Kanungo et al.
 */
template<class T>
typename ImageFactory<T>::view_type* degrade_kanungo(const T &src, float eta, float a0, float a, float b0, float b, int k, int random_seed = 0)
{
  typedef typename ImageFactory<T>::data_type data_type;
  typedef typename ImageFactory<T>::view_type view_type;
  typedef typename T::value_type value_type;
  int d;
  double randval;

  FloatImageView *dt_fore, *dt_back;
  typename T::const_vec_iterator p;
  typename view_type::vec_iterator q;
  FloatImageView::vec_iterator df,db;
  value_type blackval = black(src);
  value_type whiteval = white(src);

  data_type* dest_data = new data_type(src.size(), src.origin());
  view_type* dest = new view_type(*dest_data);
  
  // compute distance transform of foreground and background
  // as dest is not yet needed we abuse it for storing the inverted image
  dt_fore = (FloatImageView*)distance_transform(src, 0);
  for (p=src.vec_begin(), q=dest->vec_begin(); p != src.vec_end(); p++, q++) {
    if (is_black(*p)) *q = whiteval;
    else *q = blackval;
  }
  dt_back = (FloatImageView*)distance_transform(*dest, 0);

  // precompute probabilities (maximum distance 32 should be enough)
  double P_foreground_flip[32];
  double P_background_flip[32];
  for (d=0; d<32; d++) {
    P_foreground_flip[d] = a0*exp(-a*(d+1)*(d+1)) + eta;
    P_background_flip[d] = b0*exp(-b*(d+1)*(d+1)) + eta;
  }

  // flip pixels randomly based on their distance from border
  srand(random_seed);
  for (q=dest->vec_begin(), df=dt_fore->vec_begin(), db=dt_back->vec_begin();
       q != dest->vec_end(); q++, df++, db++) {
    randval = ((double)rand()) / RAND_MAX;
    // note that dest is still inverted => black is background!!
    if (is_black(*q)) {
      d = (int)(*db + 0.5);
      if ((d > 32) || (randval > P_background_flip[d-1]))
        *q = whiteval;
    } else {
      d = (int)(*df + 0.5);
      if ((d > 32) || (randval > P_foreground_flip[d-1]))
        *q = blackval;
    }
  }

  // do a morphological closing
  if (k>1) {
    // build structuring element
    data_type* se_data = new data_type(Dim(k,k), Point(0,0));
    view_type* se = new view_type(*se_data);
    for (q=se->vec_begin(); q!=se->vec_end(); q++)
      *q = blackval;
    view_type* dilated = dilate_with_structure(*dest, *se, Point(k/2,k/2));
    view_type* eroded = erode_with_structure(*dilated, *se, Point(k/2,k/2));
    delete dilated->data(); delete dilated;
    delete dest->data(); delete dest;
    delete se_data; delete se;
    dest = eroded;
  }

  // clean up
  delete dt_fore->data(); delete dt_fore;
  delete dt_back->data(); delete dt_back;

  return dest;
}


/*
 * add white speckles in onebit image
 */
template<class T>
Image* white_speckles(const T &src, float p0, int n, int k, int connectivity = 2, int random_seed = 0)
{
  typedef typename ImageFactory<T>::data_type data_type;
  typedef typename ImageFactory<T>::view_type view_type;
  typedef typename T::value_type value_type;
  double randval;
  size_t x,y;
  size_t maxx = src.ncols() - 1;
  size_t maxy = src.nrows() - 1;
  int i;

  value_type blackval = black(src);
  value_type whiteval = white(src);

  data_type* speckles_data = new data_type(src.size(), src.origin());
  view_type* speckles = new view_type(*speckles_data);

  // create random walk data
  for (y=0; y <= maxy; y++) {
    for (x=0; x <= maxx; x++) {
      Point p(x,y);
      if (is_black(src.get(p)) && (((double)rand()) / RAND_MAX < p0)) {
        speckles->set(p,blackval);
        for (i=0; i<n; i++) {
          if (p.x() == 0 || p.x() == maxx || p.y() == 0 || p.y() == maxy)
            break;
          randval = ((double)rand()) / RAND_MAX;
          if (connectivity == 0) {
            // random rook move
            if (randval < 0.25)      p.x(p.x() + 1);
            else if (randval < 0.5)  p.x(p.x() - 1);
            else if (randval < 0.75) p.y(p.y() + 1);
            else                     p.y(p.y() - 1);
          }
          else if (connectivity == 1) {
            // random bishop move
            if (randval < 0.25)      {p.x(p.x() + 1); p.y(p.y() + 1);}
            else if (randval < 0.5)  {p.x(p.x() + 1); p.y(p.y() - 1);}
            else if (randval < 0.75) {p.x(p.x() - 1); p.y(p.y() + 1);}
            else                     {p.x(p.x() - 1); p.y(p.y() - 1);}
          }
          else {
            // random king move
            if (randval < 0.125)      {p.y(p.y() - 1); p.x(p.x() - 1);}
            else if (randval < 0.25)  {p.y(p.y() - 1);}
            else if (randval < 0.375) {p.y(p.y() - 1); p.x(p.x() + 1);}
            else if (randval < 0.5)   {p.x(p.x() + 1);}
            else if (randval < 0.625) {p.x(p.x() + 1); p.y(p.y() + 1);}
            else if (randval < 0.75)  {p.y(p.y() + 1);}
            else if (randval < 0.875) {p.x(p.x() - 1); p.y(p.y() + 1);}
            else                      {p.x(p.x() - 1);}
          }
          speckles->set(p,blackval);
        }
      }
    }
  }

  // do a morphological closing
  if (k>1) {
    typename view_type::vec_iterator q;
    // build structuring element
    data_type* se_data = new data_type(Dim(k,k), Point(0,0));
    view_type* se = new view_type(*se_data);
    for (q=se->vec_begin(); q!=se->vec_end(); q++)
      *q = blackval;
    view_type* dilated = dilate_with_structure(*speckles, *se, Point(k/2,k/2));
    view_type* closed = erode_with_structure(*dilated, *se, Point(k/2,k/2));
    delete dilated->data(); delete dilated;
    delete speckles->data(); delete speckles;
    delete se_data; delete se;
    speckles = closed;
  }

  // subtract speckles from input image
  for (y=0; y <= maxy; y++) {
    for (x=0; x <= maxx; x++) {
      Point p(x,y);
      if (is_black(speckles->get(p))) {
        speckles->set(p,whiteval);
      } else {
        speckles->set(p,src.get(p));
      }
    }
  }

  return speckles;
}


}

#endif
