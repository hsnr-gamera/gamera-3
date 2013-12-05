/*
 *
 * Copyright (C) 2013      Christian Brandt and Christoph Dalitz
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

#ifndef _FOURIER_FEATURES_HPP
#define _FOURIER_FEATURES_HPP


#include "gamera.hpp"
#include "contour.hpp"
#include "geometry.hpp"
#include "segmentation.hpp"
#include "geostructs/kdtree.hpp"
#include <complex>
#include <cmath>
#include <numeric>
#include <vector>


namespace Gamera  {
typedef std::complex<double> Complex;
typedef std::vector<Complex> ComplexVector;
typedef std::vector<FloatPoint> FloatPointVector;

//------------------------------------------------------------------------
// note that cut offset is the count of value to take from the left + from the right
FloatVector* cutComplexDftAbs(ComplexVector* in, int numCoeff) {
    int dftSize = (signed)in->size();
    if(numCoeff % 2 == 0) {
        throw std::runtime_error("even number of coefficients in "
                                 "cutComplexDft is not allowed");
    }

    FloatVector *c_k = new FloatVector(numCoeff);

    int numCoeffHalf = numCoeff/2;
    if(dftSize < numCoeff) {
        numCoeffHalf = dftSize/2;
    }
    int targetIdx = 0;

    for(int k = 0; k <= numCoeffHalf; k++) {
        Complex sum(0.0,0.0);
        Complex prod(1.0,0.0);
        Complex expfac =
            std::exp(Complex(0.0, (-2 * M_PI * k) / dftSize));
        for(int t = 0; t < dftSize; t++) {
            sum += (*in)[t] * prod;
            prod *= expfac;
        }
        sum /= dftSize;
        (*c_k)[targetIdx] = std::abs(sum);
        targetIdx++;
    }

    if(dftSize < numCoeff) {
        targetIdx = numCoeff-numCoeffHalf;
    }

    for(int k = dftSize - numCoeffHalf; k < dftSize; k++) {
        Complex sum(0.0,0.0);
        Complex prod(1.0,0.0);
        Complex expfac =
            std::exp(Complex(0.0, (-2 * M_PI * k) / dftSize));
        for(int t = 0; t < dftSize; t++) {
            sum += (*in)[t] * prod;
            prod *= expfac;
        }

        sum /= dftSize;
        (*c_k)[targetIdx] = std::abs(sum);
        targetIdx++;
    }

    return c_k;
}



//------------------------------------------------------------------------
double getCrMax(const FloatVector* c_k, size_t start=1, size_t end=0) { 
    double absMax = 0;
    if (end == 0) end = c_k->size();

    for(size_t i = start; i < end; i++) { 
        double curAbs = ((*c_k)[i]);
        if(curAbs > absMax) {
            absMax = curAbs;
        }  
    }

	return absMax;
}



//------------------------------------------------------------------------
void interpolatePoints(FloatPointVector* res,
                       Point a, Point b) {
    FloatPoint q(a.x(), a.y());
    FloatPoint p(b.x(), b.y());

    int dist = (int)q.distance(p);
    FloatPoint dq = (p - q) / FloatPoint(dist,dist);

    for(int n = 1; n < dist; n++) {
        q = q + dq;
        res->push_back(q);
    }
    res->push_back(p);
}



//------------------------------------------------------------------------
FloatPointVector* interpolatePolygonPoints(PointVector*  points) {
    size_t pointCount = points->size();
    FloatPointVector* res = new FloatPointVector;

    for(size_t i = 0; i < pointCount; i++) {
        interpolatePoints(res, (*points)[(i-1 + pointCount) % pointCount],
                          (*points)[i]);
    }

    return res;
}



//------------------------------------------------------------------------
FloatVector* minimumContourHullDistances(FloatPointVector* hullPoints,
        PointVector* contourPoints) {
    FloatVector* res = new FloatVector(hullPoints->size());
    Gamera::Kdtree::KdNodeVector nodes;
    for(size_t i = 0; i < contourPoints->size(); i++) {
        Gamera::Kdtree::CoordPoint p;
        p.push_back((*contourPoints)[i].x());
        p.push_back((*contourPoints)[i].y());
        nodes.push_back(KdNode(p));
    }

    Gamera::Kdtree::KdTree tree(&nodes);

    for(size_t i = 0; i < hullPoints->size(); i++) {
        Gamera::Kdtree::KdNodeVector neighbors;
        Gamera::Kdtree::CoordPoint point;
        double x = (*hullPoints)[i].x();
        double y = (*hullPoints)[i].y();
        point.push_back(x);
        point.push_back(y);

        tree.k_nearest_neighbors(point, 1, &neighbors);

        double dx = neighbors[0].point[0] - x;
        double dy = neighbors[0].point[1] - y;
        double dist = sqrt(dx*dx + dy*dy);

        if(dist < 1.0) {
            dist = 0.0;
        }

        (*res)[i] = dist;
    }

    return res;
}



//------------------------------------------------------------------------
void floatFourierDescriptorBrokenA(FloatPointVector* interpolatedHullPoints,
        PointVector* contourPoints, FloatVector* distances, int numCoeff,
        feature_t* buf) {
    // calculate cplx_dat(t) = r(t) - jd(t)
	size_t interpolatedHullSize = interpolatedHullPoints->size();
    ComplexVector* cplx_dat = new ComplexVector(interpolatedHullSize);

    double meanX=0, meanY = 0;
    for(size_t i = 0; i < interpolatedHullSize; i++) {
        meanX += (*interpolatedHullPoints)[i].x();
        meanY += (*interpolatedHullPoints)[i].y();
    }

    meanX /= interpolatedHullPoints->size();
    meanY /= interpolatedHullPoints->size();

    for(size_t i = 0; i < interpolatedHullSize; i++) {
        register double x_r = (*interpolatedHullPoints)[i].x() - meanX;
        register double y_r = (*interpolatedHullPoints)[i].y() - meanY;
        double r = sqrt(x_r*x_r + y_r*y_r);
        (*cplx_dat)[i] = Complex(r, (*distances)[i]);
    }

    FloatVector* c_k = cutComplexDftAbs(cplx_dat, numCoeff+1);

    delete cplx_dat;

    // restricting on the first coefficients is better
    double cr = getCrMax(c_k, 0, numCoeff/2);

    for(size_t k = 0; k < (size_t)numCoeff/2; k++) {
		buf[2*k] = (*c_k)[k] / cr;
		buf[2*k+1] = (*c_k)[numCoeff-k] / cr;
    } 

    delete c_k;
}



//------------------------------------------------------------------------
template <class T>
void fourier_broken(T &m, feature_t* buf) {
    int dftCount = FDLENGTH;

	typename ImageFactory<T>::view_type *tmp = simple_image_copy(m);
    // get contour points for each CC
    ImageList* ccs = cc_analysis(*tmp);
    PointVector p;

    for(ImageList::iterator cc_it = ccs->begin(); cc_it != ccs->end();
            cc_it++) {
        Cc* cc = static_cast<Cc*>(*cc_it);
        Point orig = cc->origin();
        PointVector* cc_p = contour_pavlidis(*cc);

        for(PointVector::iterator p_it = cc_p->begin();
                p_it != cc_p->end(); p_it++) {

            p.push_back(*p_it + orig);
        }
        delete *cc_it;
        delete cc_p;
    } 
    delete ccs;
	delete tmp->data();
	delete tmp;

    if (p.size() == 0) {
      for (int k = 0; k < dftCount; k++)
        buf[k] = 0.0;
      return;
    }
    else if (p.size() == 1) {
      buf[0] = 1.0;
      for (int k = 1; k < dftCount; k++)
        buf[k] = 0.0;
      return;
    }

    //  calculate convex hull and interpolate points
    PointVector* hullPoints = convex_hull_from_points(&p);
    FloatPointVector* interpolatedHullPoints =
        interpolatePolygonPoints(hullPoints);

    FloatVector* distances = minimumContourHullDistances(interpolatedHullPoints, &p);

    floatFourierDescriptorBrokenA(interpolatedHullPoints, &p, distances, dftCount, buf);

    delete hullPoints;
    delete interpolatedHullPoints;
    delete distances;
}


}
#endif
