/*
 *
 * Copyright (C) 2001-2004
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
#define PI 3.141592653589793238462643382795
#include "gamera.hpp"
#include "gamera_limits.hpp"
#include "morphology.hpp"
#include "plugins/image_utilities.hpp"
#include "pixel.hpp"
#include "vigra/resizeimage.hxx"
#include "plugins/logical.hpp"
#include <exception>
#include <cstdlib>
#include <ctime>
#include <math.h>
#include <time.h>
#include <algorithm>
#include <fstream>
#include <iostream>

using namespace vigra;
//using namespace std;
bool randset=0;
namespace Gamera
{
	template<class T, class U>
	Image* rotate(T &m, float hypot, U bgcolor)
	{
		typedef ImageFactory<T> fact;
		typename fact::data_type* out_data;
		typename fact::view_type* out;
		
		while(hypot<=0.0) hypot+=360;
		while(hypot>=360.0) hypot-=360;

		if(hypot>45.0 && hypot<=135.0)
		{
			out_data = new typename fact::data_type(m.ncols(), m.nrows());
			out = new typename fact::view_type(*out_data, 0, 0, m.ncols(), m.nrows());
			for(size_t i = 0; i<m.nrows(); i++)
				for(size_t j = 0; j<m.ncols();j++)
					out->set(j,out->ncols()-i,(typename fact::view_type::value_type)m.get(i, j));
			hypot-=90;
		}
		else if(hypot>135.0 && hypot<=225.0)
		{
			out_data = new typename fact::data_type(m.nrows(), m.ncols());
			out = new typename fact::view_type(*out_data, 0, 0, m.nrows(), m.ncols());
			for(size_t i = 0; i<m.nrows(); i++)
				for(size_t j = 0; j<m.ncols();j++)
					out->set(out->nrows()-i,out->ncols()-j,(typename fact::view_type::value_type)m.get(i, j));
			hypot-=180;
		}
		else if(hypot>225.0 && hypot<=315.0)
		{
			out_data = new typename fact::data_type(m.ncols(), m.nrows());
			out = new typename fact::view_type(*out_data, 0, 0, m.ncols(), m.nrows());
			for(size_t i = 0; i<m.nrows(); i++)
				for(size_t j = 0; j<m.ncols();j++)
					out->set(out->nrows()-j,i,(typename fact::view_type::value_type)m.get(i, j));
			hypot-=270;
		}
		else
		{
			out_data = new typename fact::data_type(m.nrows(), m.ncols());
			out = new typename fact::view_type(*out_data, 0, 0, m.nrows(), m.ncols());
			size_t i, j;
			for(i = 0; i<m.nrows(); i++)
				for(j = 0; j<m.ncols();j++)
					out->set(i,j,(typename fact::view_type::value_type)m.get(i, j));
		}
		
		//Angle is now within [-45..45]
		//rotate by remaining arbitrary angle

		out = rot45(m, out_data, out, hypot, bgcolor);
		
		image_copy_attributes(m, *out);
		delete out_data;
		return out;
	}
	
	template<class T, class U, class V, class W>
	U* rot45(V v, T* m, U* img, float angle, W bgcolor)
	{
		size_t i;
		
		/*
		 * Performs rotation within -45 and +45 degree angles by applying three shears to the
		 * image.  The first is a horizontal shear defined by tan(angle/2), the second is a vertical
		 * shear by sin(angle), and the third is a repetition of the first horizontal shear.
		 */
		if(angle==0.0)
		{
			return img;
		}

		//------------------------------------------------------------------------------------
		// Declarations/Initialization
		//------------------------------------------------------------------------------------
		
		
		typedef typename ImageFactory<V>::view_type::value_type pixelFormat;
		pixelFormat background = (pixelFormat)(bgcolor);
		
		double dRadAngle = angle * PI / double(180);
		double dSinE = sin (dRadAngle);
		double dTan = tan (dRadAngle / 2.0);
		double dCosE = cos(dRadAngle);
		//------------------------------------------------------------------------------------
		// First shear--horizontal
		//------------------------------------------------------------------------------------
		size_t width1 = img->ncols() + size_t(double(img->nrows()) * fabs(dTan));
		size_t height1 = img->nrows();
		
		T out_data1(height1,width1);
		U* out1 = new U(out_data1, 0, 0, height1, width1);
		
		double d;
		if(dTan >= 0.0) //Positive Angle
			d = dTan * (height1 + 0.5);
		else //Negative Angle
			d = 0;//-dTan * (height1 - 0.5);
		
		for(i = 0; i<height1; i++)
		{
			size_t in = size_t(floor(d));
			double weight = d - in;
			horizShift(img,out1,i,in,background,weight);
			d -= dTan;
		}
		
		//------------------------------------------------------------------------------------
		// Second shear--vertical
		//------------------------------------------------------------------------------------
		size_t width2 = width1;
		size_t height2 = size_t( double(img->ncols()) * fabs(dSinE) + (dCosE*img->nrows())) + 1;
		
		// Allocate image for 2nd shear
		size_t diff = size_t( fabs(dSinE * dTan * img->nrows()) );
		T out_data2(height2,width2);
		U* out2 = new U(out_data2, 0, 0, height2, width2);
		
		if (dSinE >= 0.0) //Positive angle
			d = 0;//dSinE * (img->ncols() - 1.0);
		else //Negative angle
			d = -dSinE * (width2);

		for (i = 0; i < width2; i++)
		{
			size_t in = size_t(floor(d));
			double weight = d - in;
			if(in < out2->nrows()) vertShift(out1, out2, i, in, background, weight,diff);
			d += dSinE;
		}

		//------------------------------------------------------------------------------------
		// Third shear--horizontal
		//------------------------------------------------------------------------------------
		double abstan = fabs(dTan);
		
		diff = size_t(abstan * (height2 - img->nrows() + diff));
		size_t width3 = //size_t(width2 + height2*fabs(dTan) - diff + 1);
				size_t( img->ncols()*fabs(dCosE) + img->nrows()*fabs(dSinE) ) + 1;
		size_t height3 = height2;
		
		T* out_data3 = new T(height3,width3);
		U* out3 = new U(*out_data3, 0, 0, height3, width3);
		
		if (dSinE >= 0.0) //Positive Angle
			d = dTan * height3;
		else // Negative angle
			d = 0;
		
		for(i=0; i<height3; i++)
		{
			size_t in = size_t(floor(d));
			double weight = d - in;
			if(in<out3->ncols()) horizShift(out2, out3, i, in, background, weight,diff);
			d -= dTan;
		}
		
		delete out1;
		delete out2;
		return out3;
	}
	
	template<class T, class U>
	void horizShift(T* orig, T* newbmp, size_t &row, size_t shiftAmount, U bgcolor, double weight, size_t diff=0)
	{
		typedef typename ImageFactory<T>::view_type::value_type pixelFormat;
		size_t i, sourceshift = 0;
		size_t width1 = newbmp->ncols();
		pixelFormat p0, p1, oldPixel;
		p0=p1=oldPixel=bgcolor;
		i=0;
		if(shiftAmount >= diff) 
			shiftAmount -= diff;
		else
		{
			sourceshift = diff - shiftAmount;
			shiftAmount = 0;
		}
		
		for(; i<shiftAmount; i++)
			if(i < width1) newbmp->set(row,i,bgcolor);  //leading background

		borderfunc(p0,p1,oldPixel,orig->get(row,i-shiftAmount+sourceshift),weight,bgcolor);
		
		newbmp->set(row,i, //bgcolor!=pixelFormat(0.0) ? orig->get(row,i-shiftAmount+sourceshift) :
						     	       p0);
		i++;
		
		for(; i<orig->ncols() + shiftAmount - sourceshift; i++)
		{
			filterfunc(p0, p1, oldPixel, orig->get(row,i-shiftAmount+sourceshift), weight);
			if(i>=0 && i<width1) newbmp->set(row,i,p0);
		}
		weight=1.0-weight;
		if(i<width1) newbmp->set(row,i++, norm_weight_avg(bgcolor,p0,weight,1.0-weight));
		for(; i<width1; i++) newbmp->set(row,i,bgcolor); //trailing background
	}
	
	template<class T, class U>
	void vertShift(T* orig, T* newbmp, size_t &col, size_t amount, U bgcolor, double weight, size_t diff=0)
	{
		typedef typename ImageFactory<T>::view_type::value_type pixelFormat;
		size_t i, sourceshift=0;
		
		if(amount >= diff)
			amount -= diff;
		else
		{
			sourceshift = diff-amount;
			amount = 0;
		}
		size_t height1 = newbmp->nrows();
		for(i = 0; i<amount; i++) 
			if(i< height1) newbmp->set(i,col,bgcolor);  //leading background
			
		U p0, p1, oldPixel;
		oldPixel = p0 = p1 = bgcolor;
		
		borderfunc(p0,p1,oldPixel,orig->get(i-amount+sourceshift,col),weight,bgcolor);
		newbmp->set(i,col,p0);
		i++;
		
		for(; i<amount + orig->nrows() - sourceshift; i++)
		{
			if(i-amount+sourceshift >= 0) filterfunc(p0,p1,oldPixel,orig->get(i - amount + sourceshift,col),weight);
			else cout << "Failed read from (" << i-amount+sourceshift << "," << col << ")"<<endl;
			if(i>=0 && i<height1) newbmp->set(i,col,p0);
			else cout << "Failed write to (" << i << "," << col << ")"<<endl;
		}
		if(i<height1) newbmp->set(i++,col,norm_weight_avg(p0,bgcolor,weight,1.0-weight));
		
		for(; i<height1; i++) newbmp->set(i,col,bgcolor); //trailing background
	}
	
	inline void filterfunc(RGBPixel &p0, RGBPixel &p1, RGBPixel &oldPixel, RGBPixel origPixel, double &weight)
	{
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
	
	template<class T>
	inline void borderfunc(T& p0, T& p1, T& oldPixel, T origPixel, double& weight, T bgcolor)
	{
			filterfunc(p0,p1,oldPixel,origPixel,weight);
			p0 = norm_weight_avg(bgcolor,origPixel,weight,1.0-weight);
	}
	
	template<class T>
	Image* wave(T &m, int amplitude, float freq, int direction, int funcType, int offset)
	{
		typedef ImageFactory<T> fact;
		typedef typename fact::view_type::value_type pixelFormat;
		pixelFormat background = pixelFormat(0.0);
		
		typename fact::data_type* in_data, *out_data;
		typename fact::view_type* in, *out;
		size_t (*vertExpand)(size_t), (*horizExpand)(size_t);

		double (*waveType)(float, int);
		if(direction) {vertExpand = &noExpDim; horizExpand = &expDim;}
		else vertExpand = &expDim; horizExpand = &noExpDim;

		switch(funcType)
		{
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
		
		//Take the easy way out and just copy the source image in order to make image types nonambiguous
		in_data = new typename fact::data_type(m.nrows(), m.ncols());
		in = new typename fact::view_type(*in_data, 0, 0, m.nrows(), m.ncols());
		//TODO: Remove this inefficiency by fixing the type ambiguity
		
		out_data = new typename fact::data_type(m.nrows()+vertExpand(amplitude), m.ncols()+horizExpand(amplitude));
		out = new typename fact::view_type(*out_data, 0, 0, m.nrows()+vertExpand(amplitude), m.ncols()+horizExpand(amplitude));
		size_t i, j;
		
		for(i = 0; i<m.nrows(); i++) for(j=0; j<m.ncols(); j++) in->set(i,j, m.get(i,j));
		
		if(direction)
		{
			for(i=0;i<out->nrows();i++)
			{
				double shift = ((double)amplitude/2)*(1-waveType(freq,(int)i-offset));
				horizShift(in, out, i, floor(shift),background, shift-(size_t)shift);
			}
		}
		else
		{
			for(i=0;i<out->ncols();i++)
			{
				double shift = ((double)amplitude/2)*(1-waveType(freq,(int)i-offset));
				vertShift(in, out, i, floor(shift),background, shift - (size_t)shift);
			}
		}
		
		image_copy_attributes(m, *out);
		
		delete in_data, in;
		return out;
	}
	
	inline double sin2(float per, int n)
	{
		if(per==0) return 1;
		return sin(2.0*PI*(double)n/per);
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
		return sin2(per,n)*per/(2*PI*n);
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
	Image* noise(T &m, int amplitude, int direction)
	{
		size_t i,j;
		typedef ImageFactory<T> fact;
		typedef typename fact::view_type::value_type pixelFormat;
		typename fact::data_type* out_data;
		typename fact::view_type* out;
		pixelFormat background = m.get(0,0);
		if(!randset){srand(time(0)); randset=1;}
		size_t (*vertExpand)(size_t), (*horizExpand)(size_t),
			   (*vertShift)(size_t, double), (*horizShift)(size_t, double);
		if(direction)
		{
			vertExpand = &expDim; horizExpand = &noExpDim;
			vertShift = &doShift; horizShift = &noShift;
		}
		else
		{
			vertExpand = &noExpDim; horizExpand = &expDim;
			vertShift = &noShift; horizShift = &doShift;
		}
		
		out_data = new typename fact::data_type(m.nrows()+vertExpand(amplitude), m.ncols()+horizExpand(amplitude));
		out = new typename fact::view_type(*out_data, 0, 0, m.nrows()+vertExpand(amplitude), m.ncols()+horizExpand(amplitude));

		for(i = 0; i<out->nrows(); i++) for(j=0; j<out->ncols(); j++) out->set(i,j, background);
		for(i = 0; i<m.nrows(); i++)
			for(j = 0; j<m.ncols();j++)
			{
				out->set(i+vertShift(amplitude,noisefunc()),
						j+horizShift(amplitude,noisefunc()),
						(typename fact::view_type::value_type)m.get(i, j)
						);
			}
		return out;
	}
	
	template<class T>
	Image* inkrub(T &m, int a)
        {
		typedef ImageFactory<T> fact;
		typedef typename fact::view_type::value_type pixelFormat;
		pixelFormat background = (pixelFormat)m.get(0,0);
		typename fact::data_type* out_data;
		typename fact::view_type* out;
		
		out_data = new typename fact::data_type(m.nrows(), m.ncols());
		out = new typename fact::view_type(*out_data, 0, 0, m.nrows(), m.ncols());
		size_t i, j;

		for(i = 0; i<m.nrows(); i++) for(j = 0; j<m.ncols();j++)
		{
			out->set(i,j,
				 (typename fact::view_type::value_type)m.get(i, j));
		}

		if(!randset){srand(time(0));randset = 1;}
		for(i = 0; i<out->nrows(); i++) for(j = 0; j<out->ncols(); j++)
		{
			pixelFormat px1 = m.get(i,out->ncols() - j - 1),
				    px2 = out->get(i,j);
			if (a*rand()/RAND_MAX == 0) out->set(i,j,norm_weight_avg(px1,px2,0.5,0.5));
		}

		image_copy_attributes(m, *out);

		return out;
	}

	template<class T>
	Image* ink_diffuse(T &m, int type, double dropoff)
	{
		typedef ImageFactory<T> fact;
		typedef typename fact::view_type::value_type pixelFormat;
		pixelFormat background = (pixelFormat)m.get(0,0);
		typename fact::data_type* out_data;
		typename fact::view_type* out;
		out_data = new typename fact::data_type(m.nrows(), m.ncols());
		out = new typename fact::view_type(*out_data, 0, 0, m.nrows(), m.ncols());
		size_t i, j;
		double val, expSum;
		pixelFormat aggColor, currColor;
		if(!randset){srand(time(NULL)); randset=1;}
		
		if(type == 0)
		{
			for(i=0; i<m.nrows(); i++)
			{
				aggColor = m.get(i,0);
				expSum = 0;
				for(j=0; j<m.ncols(); j++)
				{
					val = 1.0/exp((double)j/dropoff);
					expSum += val;
					currColor = m.get(i,j);
					double weight = val / (val + expSum);
					aggColor = norm_weight_avg(aggColor, currColor, 1-weight, weight);
					out->set(i,j,norm_weight_avg(aggColor,currColor,val, 1.0-val));
				}
				
			}
		}
		else if(type == 1)
		{
			for(i=0; i<m.ncols(); i++)
			{
				aggColor = m.get(0,i);
				expSum = 0;
				for(j=0; j<m.nrows(); j++)
				{
					val = 1.0/exp((double)j/dropoff);
					expSum += val;
					currColor = m.get(j,i);
					double weight = val / (val + expSum);
					aggColor = norm_weight_avg(aggColor, currColor, 1-weight, weight);
					out->set(j,i,norm_weight_avg(aggColor,currColor,val, 1.0-val));
				}
			}
		}
		else if(type == 2)//try monte carlo simulation
		{
			for(i = 0; i<m.ncols(); i++) for(j=0; j<m.nrows(); j++) out->set(j,i,m.get(j,i));
			size_t starti, startj;
			double iD, jD;
			starti = iD = (double)m.ncols() * (double)rand() / (double)RAND_MAX;
			startj = jD = (double)m.nrows() * (double)rand() / (double)RAND_MAX;
			size_t index = 0;
			while(iD>0 && iD < m.ncols() && jD>0 && jD<m.nrows() )
			{
				expSum = 0;
				val = 1.0/exp(dist((double)starti, (double)startj, iD, jD)/dropoff);
				expSum += val;
				currColor = out->get(jD,iD);
				double weight = val / (val + expSum);
				aggColor = norm_weight_avg(aggColor, currColor, 1-weight, weight);
				out->set((size_t)jD,(size_t)iD,norm_weight_avg(aggColor,currColor,1.0-val, val));
				iD += sin(2.0*PI*rand()/(double)RAND_MAX);
				jD += cos(2.0*PI*rand()/(double)RAND_MAX);

			}
		}
		image_copy_attributes(m, *out);
		return out;
	}
	
	inline double dist(double i0, double j0, double i1, double j1)
	{
		double quadI = pow(i1-i0,2.0), quadJ = pow(j1-j0,2.0);
		return sqrt(quadI + quadJ);
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
		 return ((pix1 * w1) + (pix2 * w2))/(w1 + w2);
	}
}

#endif
