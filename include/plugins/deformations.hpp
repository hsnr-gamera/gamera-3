/*
 *
 * Copyright (C) 2001 - 2002
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef kwm12032001_deformations
#define kwm12032001_deformations
#define PI 3.1415926535897932384626433832795
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
#include <algorithm>
#include <iostream>
#include <fstream>
using namespace vigra;

namespace Gamera
{
	template<class T>
	Image* rotateShear(T &m, float hypot) {
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

		out = rot45(m, out_data, out, hypot);

		image_copy_attributes(m, *out);

		return out;
	}

	template<class T, class U, class V>
	U* rot45(V v, T* m, U* img, float angle)
	{
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
		using namespace std;
		
		typedef typename ImageFactory<V>::view_type::value_type pixelFormat;
		pixelFormat background = (pixelFormat)(1.0);
		size_t width1, height1;
		T* out_data1;
		U* out1;
		double dRadAngle = angle * PI / double(180);
		double dSinE = sin (dRadAngle);
		double dTan = tan (dRadAngle / 2.0);
		
		//------------------------------------------------------------------------------------
		// First shear--horizontal
		//------------------------------------------------------------------------------------
		width1 = img->ncols() + int(double(img->nrows()) * fabs(dTan));
		height1 = img->nrows();

		out_data1 = new T(height1,width1);
		out1 = new U(*out_data1, 0, 0, height1, width1);

		size_t i, iShears[height1];
		double weights[height1];
		if (dTan >= 0.0) for(i = 0; i<height1; i++) // Positive angle
		{
			double d = (double(i) + 0.5) * dTan;
			int in = int(floor(d));
			iShears[height1-i-1] = in;
			weights[height1-i-1] = d - in;
		}
		else for(i = 0; i<height1; i++) // Negative angle
		{
			double d = (double(i) + 0.5) * -dTan;
			int in = int(floor(d));
			iShears[i]=in;
			weights[i]=d-in;
		}

		for(i = 0; i<height1; i++) horizShift(img, out1, i, iShears[i], background, weights[i]);
		delete img;
		delete m;
		img = out1;
		
		//------------------------------------------------------------------------------------
		// Second shear--vertical
		//------------------------------------------------------------------------------------
		width1 = img->ncols();
		height1 = size_t(double(img->ncols() * fabs(dSinE))) + img->nrows();

		// Allocate image for 2nd shear
		T* oldod = out_data1;
		out_data1 = new T(height1,width1);
		out1 = new U(*out_data1, 0, 0, height1, width1);
		
		size_t iShearsV[width1];
		double weightsV[width1];
		if (dSinE >= 0.0) for(i = 0; i < width1; i++) //Positive angle
		{
			double d = (double(i)+0.5) * dSinE;
			size_t in = size_t(floor(d));
			iShearsV[width1 - i - 1] = in;
		}
		else for(i = 0; i < width1; i++) //Negative angle
		{
			double d = (double(i)+0.5) * -dSinE;
			size_t in = size_t(floor(d));
			iShearsV[i] = in;
			weightsV[i] = d - in;
		}
		
		for (i = 0; i < width1; i++)
			vertShift(img, out1, i, iShearsV[width1-i-1], background, weightsV[i]);

		delete img;
		delete oldod;
		img = out1;
		

		//------------------------------------------------------------------------------------
		// Third shear--horizontal
		//------------------------------------------------------------------------------------
		width1 = img->ncols() + int(double(img->nrows()) * fabs(dTan));
		height1 = img->nrows();

		oldod = out_data1;
		out_data1 = new T(height1,width1);
		out1 = new U(*out_data1, 0, 0, height1, width1);
		
		size_t iShearsH[height1];
		double weightsH[height1];
		if (dTan >= 0.0) for(i = 0; i<height1; i++) // Positive angle
		{
			double d = (double(i) + 0.5) * dTan;
			int in = int(floor(d));
			iShearsH[height1-i-1] = in;
			weightsH[height1-i-1] = d - in;
		}
		else for(i = 0; i<height1; i++) // Negative angle
		{
			double d = (double(i) + 0.5) * -dTan;
			int in = int(floor(d));
			iShearsH[i]=in;
			weightsH[i]=d-in;
		}
		
		for(i = 0; i<height1; i++) horizShift(img, out1, i, iShearsH[i], background, weightsH[i]);

		delete img;
		delete oldod;
		U* out = out1;
		/* Filtering broke the simplistic border removal algorithm, so this will take some work to remedy
		U* out = removeExcessBorder(out_data1, out1, background);
		delete out1;
		delete out_data1;
		*/
		return out;
	}

	template<class T, class U, class V>
	U* removeExcessBorder(T* img_data, U* img, V background)
	{
		//------------------------------------------------------------------------------------
		// Prune background deadspace
		//------------------------------------------------------------------------------------
		size_t i=0;
		size_t height1 = img->nrows(), width1 = img->ncols();
		double tolerance = 5;
		for(i = 0; i<height1; i++)
		{
			size_t j = 0;

			for(;j<width1; j++)
			{
				if (!equalwithinpct(img->get(i,j),background,tolerance)) goto a;
			}
		}
a:		size_t newTop = i;
		
		for(i = height1-1; i>=0; i--)
		{
			size_t j = 0;

			for(;j<width1; j++)
			{
				if (!equalwithinpct(img->get(i,j),background,tolerance)) goto b;
			}
		}
b:		size_t newBott = i;
		
		for(i = 0; i<width1; i++)
		{
			size_t j = 0;

			for(;j<height1; j++)
			{
				if (!equalwithinpct(img->get(i,j),background,tolerance)) goto c;
			}
		}
c:		size_t newLeft = i;
		
		for(i = width1-1; i>=0; i--)
		{
			size_t j = 0;

			for(;j<height1; j++)
			{
				if (!equalwithinpct(img->get(i,j),background,tolerance)) goto d;
			}
		}
d:		size_t newRight = i;

		bool dimsChanged = false;
		if(width1 != newRight - newLeft)
		{
			width1 = newRight - newLeft;
			dimsChanged = true;
		}
		if(height1 != newBott - newTop)
		{
			height1 = newBott - newTop;
			dimsChanged = true;
		}

		if(!dimsChanged) return img;

		T* out_data1 = new T(height1,width1);
		U* out1 = new U(*out_data1, 0, 0, height1, width1);
		
		for(i = newTop; i<newBott; i++)
		{
			size_t j = newLeft;
			for(; j<newRight; j++)
			{
				out1->set(i-newTop,j-newLeft, img->get(i,j));
			}
		}
		return out1;
	}
	template<class T, class U>
	void horizShift(T* orig, T* newbmp, size_t &row, size_t &amount, U bgcolor, double weight)
	{
		size_t i;
		size_t width1 = newbmp->ncols();
		
		for(i = 0; i<amount; i++) newbmp->set(row,i,bgcolor);  //leading background

		U filt[3];
		filt[2] = bgcolor;

		for(; i<orig->ncols()+amount; i++)
		{
			filt[0] = (U)orig->get(row, i-amount);
			pixelfilt(filt,weight);
			if((i>=0) && (i<width1))
			{
				newbmp->set(row,i,filt[0]);
			}
		}
		for(; i<width1; i++) newbmp->set(row,i,bgcolor); //trailing background
	}

	template<class T, class U>
	void vertShift(T* orig, T* newbmp, size_t &col, size_t &amount, U bgcolor=(U)black(), double weight)
	{
		size_t i;
		size_t height1 = newbmp->nrows();
		for(i = 0; i<amount; i++) newbmp->set(i,col,bgcolor);  //leading background

		U filt[3];
		filt[2] = bgcolor;
		for(; i<orig->nrows()+amount; i++)
		{
			filt[0] = (U)orig->get(i-amount,col);
			pixelfilt(filt,weight);
			if((i>=0) && (i<height1))
			{
				newbmp->set(i,col,filt[0]);
			}
		}
		for(; i<height1; i++) newbmp->set(i,col,bgcolor); //trailing background
	}

	template<class T>
	Image* wave(T &m, int amplitude, float freq, int direction, int funcType, int offset) {
		using namespace std;
		if(amplitude < 0)
		{
			//cerr<<"Could not make a wave with negative amplitude."<<endl<<
			//"Try to rotate by 180 and then apply the desired wave deformation with positive amplitude."<<endl;
			return NULL;
		}
		typedef ImageFactory<T> fact;
		typedef typename fact::view_type::value_type pixelFormat;
		pixelFormat background = (pixelFormat)1.0;
		typename fact::data_type* out_data;
		typename fact::view_type* out;
		size_t (*vertExpand)(size_t), (*horizExpand)(size_t),
			   (*vertShift)(size_t, double), (*horizShift)(size_t, double);
		//Note that these do not correspond to the horizShift and vertShift functions defined above!

		double (*waveType)(float, int);
		if(direction)
		{
			vertExpand = &noExpDim; horizExpand = &expDim;
			vertShift = &noShift; horizShift = &doShift;
		}
		else
		{
			vertExpand = &expDim; horizExpand = &noExpDim;
			vertShift = &doShift; horizShift = &noShift;
		}
		

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

		out_data = new typename fact::data_type(m.nrows()+vertExpand(amplitude), m.ncols()+horizExpand(amplitude));
		out = new typename fact::view_type(*out_data, 0, 0, m.nrows()+vertExpand(amplitude), m.ncols()+horizExpand(amplitude));
		size_t i, j;
		
		for(i = 0; i<out->nrows(); i++) for(j=0; j<out->ncols(); j++) out->set(i,j, background);

		for(i = 0; i<m.nrows(); i++)
			for(j = 0; j<m.ncols();j++)
			{
				out->set(i+vertShift(amplitude,waveType(freq,(int)j-offset)),
						j+horizShift(amplitude,waveType(freq,(int)i-offset)),
						(typename fact::view_type::value_type)m.get(i, j)
						);
			}
		out = removeExcessBorder(out_data,out,background);
		image_copy_attributes(m, *out);
		
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
		float quarter = per/4;
		if(n1<(3*quarter) && n1>(quarter))
			return 1 - 4*(n1-quarter)/per;
		
		if(n1<=quarter)
			return (4*n1/per);

		return -1+ 4*((n1-3*quarter)/per);
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
		srand(time(0));
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
	bool randset=0;
	template<class T>
	Image* inkrub(T &m, int a) {
		using namespace std;
		
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
			out->set(i,
					j,
					(typename fact::view_type::value_type)m.get(i, j)
					);
		}

		if(!randset)
		{
			srand(time(0));
			randset = 1;
		}
/*
		size_t hoffset = size_t(0.5*(double)m.ncols()*((double)rand()/(double)RAND_MAX)),
			   voffset = size_t(0.5*(double)m.nrows()*((double)rand()/(double)RAND_MAX));
*/
		size_t hoffset = 0, voffset = 0;
		for(i = voffset; i<out->nrows(); i++) for(j = hoffset; j<out->ncols(); j++)
		{
			pixelFormat px = m.get(i-voffset, m.ncols() - (j-hoffset));
			if(px!=background && (a*rand()/RAND_MAX == 0)) out->set(i,j,px);
		}

		image_copy_attributes(m, *out);

		return out;
	}

	template<class T>
	inline void floatgreyfilt(T pixArr[3], double &weight){
		pixArr[1] = (T)(weight * pixArr[0]);

		pixArr[0] = (T)(pixArr[0] - (pixArr[1] - pixArr[2]));

		pixArr[2] = pixArr[1];
	}
	inline void pixelfilt(FloatPixel pixArr[3], double weight) {
		floatgreyfilt(pixArr, weight);
	}

	inline void pixelfilt(GreyScalePixel pixArr[3], double &weight) {
		floatgreyfilt(pixArr, weight);
	}

	inline void pixelfilt(Grey16Pixel pixArr[3], double &weight) {
		floatgreyfilt(pixArr, weight);
	}

	inline void pixelfilt(RGBPixel pixArr[3], double &weight)
	{
		pixArr[1] = RGBPixel(GreyScalePixel(pixArr[0].red() * weight),
							 GreyScalePixel(pixArr[0].green() * weight),
							 GreyScalePixel(pixArr[0].blue() * weight));
/*
		pixArr[0].red() = GreyScalePixel(pixArr[0].red() - (pixArr[1].red() - pixArr[2].red()));
		pixArr[0].green() = GreyScalePixel(pixArr[0].green() - (pixArr[1].green() - pixArr[2].green()));
		pixArr[0].blue() = GreyScalePixel(pixArr[0].blue() - (pixArr[1].blue() - pixArr[2].blue()));
*/
		pixArr[0] = RGBPixel(GreyScalePixel(pixArr[0].red() - (pixArr[1].red() - pixArr[2].red())),
							 GreyScalePixel(pixArr[0].green() - (pixArr[1].green() - pixArr[2].green())),
							 GreyScalePixel(pixArr[0].blue() - (pixArr[1].blue() - pixArr[2].blue())));

		pixArr[2] = pixArr[1];
	}

	inline void pixelfilt(OneBitPixel pixArr[3], double &weight) {
		pixArr[1] = (OneBitPixel)(weight * pixArr[0]);

		pixArr[0] = (OneBitPixel)(pixArr[0] - (pixArr[1] - pixArr[2]));

		pixArr[2] = pixArr[1];
	}

	template<class T, class U>
	inline bool ewpGeneric(T a, U b, double pct)
	{
		pct/=100;

		return (a>=b*(1-pct) && a<=b*(1+pct));
	}
	
	template<class T>
	inline bool equalwithinpct(RGBPixel a, T b, double pct)
	{
		pct/=100;
		bool red = (a.red()>=b.red()*(1-pct) && a.red()<=b.red()*(1+pct));
		bool green = (a.green()>=b.green()*(1-pct) && a.green()<=b.green()*(1+pct));
		bool blue = (a.blue()>=b.blue()*(1-pct) && a.blue()<=b.blue()*(1+pct));

		return red && green && blue;
	}
	template<class T, class U>
	inline bool equalwithinpct(T a, U b, double pct)
	{
		pct/=100;

		return (a>=b*(1-pct) && a<=b*(1+pct));
	}
/*
	inline bool equalwithinpct(FloatPixel &a, FloatPixel &b, double &pct)
	{
		return ewpGeneric(a,b,pct);
	}
	inline bool equalwithinpct(GreyScalePixel &a, GreyScalePixel &b, double &pct)
	{
		return ewpGeneric(a,b,pct);
	}

	inline bool equalwithinpct(Grey16Pixel &a, GreyScalePixel &b, double &pct)
	{
		return ewpGeneric(a,b,pct);
	}
	inline bool equalwithinpct(RGBPixel &a, RGBPixel &b, double pct)
	{
		pct/=100;
		bool red = (a.red()>=b.red()*(1-pct) && a.red()<=b.red()*(1+pct));
		bool green = (a.green()>=b.green()*(1-pct) && a.green()<=b.green()*(1+pct));
		bool blue = (a.blue()>=b.blue()*(1-pct) && a.blue()<=b.blue()*(1+pct));

		return red && green && blue;
	}

	inline bool equalwithininput(OneBitPixel &a, OneBitPixel &b, double &pct)
	{
		return a == b;
	}*/
}

#endif
