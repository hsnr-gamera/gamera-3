/*
 *
 * Copyright (C) 2001-2005
 * Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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
#define _kwn12032001_deformations

#include "plugins/image_utilities.hpp"
#include "vigra/resizeimage.hxx"
#include "plugins/logical.hpp"

#include <exception>
#include <cstdlib>
#include <ctime>
#include <math.h>
#include <time.h>
#include <algorithm>

bool randset=0;

/*
 * Rotate at an arbitrary angle.
 *
 * This algorithm works by first rotating the image within 45 degrees of the desired
 * angle, and then using 'rot45' to do a multipass (3 shear) rotation.
 *
 * src - A view of of the source image
 * angle - Degree of rotation
 * bgcolor - Background color
 *
 */
template<class T>
typename ImageFactory<T>::view_type* rotate(const T &src, double angle, typename T::value_type bgcolor)
{
  
  // Adjust angle to a positve double between 0-360
  while(angle<=0.0) angle+=360;
  while(angle>=360.0) angle-=360;

  // Image initialization
  typedef typename ImageFactory<T>::data_type data_type;
  typedef typename ImageFactory<T>::view_type view_type;

  data_type* new_data;
  view_type* new_view;

  // Rotate 90, 180, or 270 degrees (no interpolation required)

  if ((angle>45.0) && (angle<=135.0)) {
    // Rotate 90 degrees by coping to a cols/rows view.
    new_data = new data_type(src.ncols(), src.nrows(), 0, 0);
    new_view = new view_type(*new_data);

    for (size_t i=0; i < src.nrows(); i++) {
      for (size_t j=0; j < src.ncols(); j++) {
	new_view->set(j, new_view->width()-i, src.get(i, j));
      }
    }
    angle-=90.0;  // Angle is now between 0.0-45.0 degrees
  }
  else if ((angle>135.0) && (angle<=225.0)) {
    // Rotate 180 degrees with a reverse iteration
    new_data = new data_type(src.nrows(), src.ncols(), 0, 0);
    new_view = new view_type(*new_data);
    typename T::const_vec_iterator sourceIter = src.vec_begin();
    typename view_type::vec_iterator destIter = new_view->vec_end();
    
    for(; sourceIter != src.vec_end(); ++sourceIter) {
      destIter--;
      *destIter = *sourceIter;
    }
    
    angle-=180.0;  // Angle is now between 0.0-45.0 degrees
  }
  else if((angle>225.0) && (angle<=315.0)) {
    // Rotate 270 degrees
    new_data = new data_type(src.ncols(), src.nrows(), 0, 0);
    new_view = new view_type(*new_data);

    for (size_t i=0; i < src.nrows(); i++) {
      for (size_t j=0; j < src.ncols(); j++) {
	new_view->set(new_view->height()-j, i,  src.get(i, j));
      }
    }
    angle-=270.0; // Angle is now between 0.0-45.0 degrees
  }
  else {
    // Angle is within 0-45.0 degrees already, send a copy to rot45
    new_data = new data_type(src.nrows(), src.ncols(), src.offset_y(), src.offset_x());
    new_view = new view_type(*new_data);
    typename T::const_vec_iterator sourceIter = src.vec_begin();
    typename view_type::vec_iterator destIter = new_view->vec_begin();
 
    for(; sourceIter != src.vec_end(); ++sourceIter, ++destIter) {
      *destIter = *sourceIter;
    }
  }

  // MGD: Changed to fix memory leak.  If rot45 will do nothing,
  // we can just return new_view, otherwise, we have to delete new_view/new_data
  // before returning.

  double epsilon = 1e-5;

  if (abs(angle) <= epsilon) {
    return new_view;
  } else {
    view_type* result = rot45(*new_view, angle, bgcolor);
    delete new_view;
    delete new_data;
    return result;
  }
}

/*
 * Performs rotation within -45 and +45 degree angles by applying three shears to the
 * image.  The first is a horizontal shear defined by tan(angle/2), the second is a vertical
 * shear by sin(angle), and the third is a repetition of the first horizontal shear.
 */

template<class T>
typename ImageFactory<T>::view_type* rot45(T &src, float angle, typename T::value_type bgcolor)
{
  //------------------------------------------------------------------------------------
  // Declarations/Initialization
  //------------------------------------------------------------------------------------
 
  typedef typename ImageFactory<T>::data_type data_type;
  typedef typename ImageFactory<T>::view_type view_type;  
  
  typedef typename ImageFactory<T>::view_type::value_type pixelFormat;
  
  pixelFormat background = bgcolor;

  size_t i;
  
  double dRadAngle = angle * M_PI / double(180.0);
  double dSinE = sin(dRadAngle);
  double dTan = tan(dRadAngle / 2.0);
  double dCosE = cos(dRadAngle);
  
  //------------------------------------------------------------------------------------
  // First shear--horizontal
  //------------------------------------------------------------------------------------
  size_t width1 = src.ncols() + size_t( double(src.nrows()) * fabs(dTan) );
  size_t height1 = src.nrows();

  data_type* shear1_data = new data_type(height1, width1, src.offset_y(), src.offset_x());
  view_type* shear1_view = new view_type(*shear1_data);
  
  double d;
  if (dTan >= 0.0) // Positive Angle
    d = dTan * (height1 + 0.5);
  else // Negative Angle
    d = 0;

  for(i = 0; i<height1; i++) {
    size_t in = size_t(floor(d));
    double weight = d - in;
    shear_x(src, *shear1_view, i, (size_t)(in), background, (double)(weight));
    d -= dTan;
  }
  
  //------------------------------------------------------------------------------------
  // Second shear--vertical
  //------------------------------------------------------------------------------------
  size_t width2 = width1;
  size_t height2 = size_t( double(src.ncols()) * fabs(dSinE) + (dCosE*src.nrows()) ) + 1;
  
  // Allocate image for 2nd shear
  size_t diff = size_t( fabs(dSinE * dTan * src.nrows()) );
  data_type* shear2_data = new data_type(height2, width2, src.offset_y(), src.offset_x());
  view_type* shear2_view = new view_type(*shear2_data);
  
  if (dSinE >= 0.0) // Positive angle
    d = 0;
  else // Negative angle
    d = -dSinE * (width2);
  
  for (i = 0; i < width2; i++) {
    size_t in = size_t(floor(d));
    double weight = d - in;
    if(in < shear2_view->nrows()) {
      shear_y(*shear1_view, *shear2_view, i, in, background, weight, diff);
    }
    d += dSinE;
  }
  
  //------------------------------------------------------------------------------------
  // Third shear--horizontal
  //------------------------------------------------------------------------------------
  double abstan = fabs(dTan);
  
  diff = size_t(abstan * (height2 - src.nrows() + diff));
  size_t width3 = size_t( src.ncols()*fabs(dCosE) + src.nrows()*fabs(dSinE) ) + 1;
  size_t height3 = height2;
  
  data_type* shear3_data = new data_type(height3, width3, src.offset_y(), src.offset_x());
  view_type* shear3_view = new view_type(*shear3_data);
  
  if (dSinE >= 0.0) // Positive Angle
    d = dTan * height3;
  else // Negative angle
    d = 0;
  
  for(i=0; i<height3; i++) {
    size_t in = size_t(floor(d));
    double weight = d - in;
    if (in < shear3_view->ncols()) {
      shear_x(*shear2_view, *shear3_view, i, in, background, weight, diff);
    }
    d -= dTan;
  }

  delete shear1_view;
  delete shear1_data;
  delete shear2_view;
  delete shear2_data;
  return shear3_view;
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
      newbmp.set(row, i, bgcolor);  //leading background
  }
  
  borderfunc(p0, p1, oldPixel, orig.get(row, i-shiftAmount+sourceshift), weight, bgcolor);
  
  newbmp.set(row,i, p0);
  i++;

  for(; i<orig.ncols() + shiftAmount - sourceshift; i++) {
    filterfunc(p0, p1, oldPixel, orig.get(row,i-shiftAmount+sourceshift), weight);
    if ((i>=0) && (i<width1))
      newbmp.set(row, i, p0);
  }
  weight=1.0-weight;
  
  if (i<width1)
    newbmp.set(row, i++, norm_weight_avg(bgcolor, p0, weight, 1.0-weight));
  
  for(; i<width1; i++)
    newbmp.set(row, i, bgcolor); //trailing background
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
      newbmp.set(i, col, bgcolor);  //leading background
  }

  pixelFormat p0=bgcolor, p1=bgcolor, oldPixel=bgcolor;
  
  borderfunc(p0, p1, oldPixel, orig.get(i-shiftAmount+sourceshift,col), weight, bgcolor);
  newbmp.set(i, col, p0);
  i++;

  for(; i<shiftAmount + orig.nrows() - sourceshift; i++) {
    if ((i-shiftAmount+sourceshift) >= 0)
      filterfunc(p0, p1, oldPixel, orig.get(i - shiftAmount + sourceshift, col),weight);
   
    if ((i>=0) && (i<height1))
      newbmp.set(i, col, p0);
  }
  
  if (i<height1)
    newbmp.set(i++, col, norm_weight_avg(p0, bgcolor, weight, 1.0-weight));
  
  for(; i<height1; i++)
    newbmp.set(i, col, bgcolor); //trailing background
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
typename ImageFactory<T>::view_type* wave(const T &src, int amplitude, float freq, int direction, int funcType, int offset)
{

  typedef typename ImageFactory<T>::data_type data_type;
  typedef typename ImageFactory<T>::view_type view_type;

  typedef typename T::value_type pixelFormat;

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
  
  new_data = new data_type(src.nrows()+vertExpand(amplitude),
			   src.ncols()+horizExpand(amplitude),
			   src.offset_x(),
			   src.offset_y());

  new_view = new view_type(*new_data);

  //image_copy_fill(src, *new_view);  Dimensions must match..

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
	  double shift = ((double)amplitude/2)*(1-waveType(freq,(int)i-offset));
	  shear_x(src, *new_view, i, (size_t)(floor(shift)), background, (double)(shift-floor(shift)));
    }
  }
  else {
      for(size_t i=0; i<new_view->ncols(); i++) {
	  double shift = ((double)amplitude/2)*(1-waveType(freq,(int)i-offset));
	  shear_y(src, *new_view, i, (size_t)(floor(shift)), background, (double)(shift - (size_t)(shift)));
      }
    }
  
  image_copy_attributes(src, *new_view);
  
  return new_view;
}


template<class T>
typename ImageFactory<T>::view_type* noise(const T &src, int amplitude, int direction)
{

  typedef typename ImageFactory<T>::data_type data_type;
  typedef typename ImageFactory<T>::view_type view_type;

  typedef typename T::value_type pixelFormat;
  pixelFormat background = src.get(0,0);

  //image_copy_fill(src, *new_view);


  if (!randset) {
    srand(time(0));
    randset=1;
  }

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

  data_type* new_data = new data_type(src.nrows()+vertExpand(amplitude),
					       src.ncols()+horizExpand(amplitude),
					       src.offset_x(),
					       src.offset_y());

  view_type* new_view = new view_type(*new_data);

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
	new_view->set(i+vertShift(amplitude,noisefunc()),
		 j+horizShift(amplitude,noisefunc()),
		 src.get(i, j)
		 );
    }
  }
  
  return new_view;
}

/*
 * Inkrub
 */

template<class T>
typename ImageFactory<T>::view_type* inkrub(const T &src, int a) {

  typedef typename ImageFactory<T>::data_type data_type;
  typedef typename ImageFactory<T>::view_type view_type;

  typedef typename T::value_type pixelFormat;
  // pixelFormat background = src.get(0 ,0);

  data_type* new_data = new data_type(src.nrows(), src.ncols(), src.offset_x(), src.offset_y());
  view_type* new_view = new view_type(*new_data);

  // Iterator initialization
  typedef typename T::const_row_iterator IteratorI;
  typedef typename view_type::row_iterator IteratorJ;
  IteratorI ir = src.row_begin();
  IteratorJ jr = new_view->row_begin();

  image_copy_fill(src, *new_view);

  if (!randset) {
    srand(time(0));
    randset = 1;
  } 

  for (int i=0; ir != src.row_end(); ++ir, ++jr, i++) {
    typename IteratorI::iterator ic = ir.begin();
    typename IteratorJ::iterator jc = jr.begin();
    for (int j=0; ic != ir.end(); ++ic, ++jc, j++) {
      pixelFormat px2 = *ic;
      pixelFormat px1 = src.get(i, new_view->ncols()-j-1);
      if ((a*rand()/RAND_MAX) == 0)
	*jc = norm_weight_avg(px1, px2, 0.5, 0.5);
    }
  }
  
  image_copy_attributes(src, *new_view);
  
  return new_view;
}

inline double dist(double i0, double j0, double i1, double j1)
{
  double quadI = pow(i1-i0,2.0), quadJ = pow(j1-j0,2.0);
  return sqrt(quadI + quadJ);
}	

template<class T>
typename ImageFactory<T>::view_type* ink_diffuse(const T &src, int type, double dropoff)
{
  
  typedef typename ImageFactory<T>::data_type data_type;
  typedef typename ImageFactory<T>::view_type view_type;

  typedef typename T::value_type pixelFormat;
  // pixelFormat background = src.get(0, 0);

  data_type* new_data = new data_type(src.nrows(), src.ncols(), src.offset_x(), src.offset_y());
  view_type* new_view = new view_type(*new_data);

  // Iterator initialization
  typedef typename T::const_row_iterator IteratorI;
  typedef typename view_type::row_iterator IteratorJ;
  IteratorI ir = src.row_begin();
  IteratorJ jr = new_view->row_begin();

  double val, expSum;
  pixelFormat aggColor, currColor;

  if (!randset) {
    srand(time(NULL));
    randset=1;
  }
  
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
      aggColor = src.get(0, i);
      expSum = 0;
      for (int j=0; ic != ir.end(); ++ic, ++jc, j++) {
	val = 1.0/exp((double)j/dropoff);
	expSum += val;
	currColor = *ic;
	double weight = val / (val + expSum);
	aggColor = norm_weight_avg(aggColor, currColor, 1-weight, weight);
	new_view->set(j, i, norm_weight_avg(aggColor, currColor, val, 1.0-val));
      }
    }
  }
  else if (type == 2) {

    typename T::const_vec_iterator srcIter = src.vec_begin();
    typename view_type::vec_iterator destIter = new_view->vec_end();

    for(; srcIter != src.vec_end(); ++srcIter, --destIter) {
      *destIter = *srcIter;
    }

    // THIS IS CONFUSING YOU DO IT

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
      currColor = new_view->get(static_cast<unsigned int>(floor(jD)), static_cast<unsigned int>(floor(iD)));
      double weight = val / (val + expSum);
      aggColor = norm_weight_avg(aggColor, currColor, 1-weight, weight);
      new_view->set((unsigned int)(floor(jD)), (unsigned int)(floor(iD)), norm_weight_avg(aggColor, currColor, 1.0-val, val));
      iD += sin(2.0*M_PI*rand()/(double)RAND_MAX);
      jD += cos(2.0*M_PI*rand()/(double)RAND_MAX);
    }
  }

  image_copy_attributes(src, *new_view);
  return new_view;
}

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

 #endif
