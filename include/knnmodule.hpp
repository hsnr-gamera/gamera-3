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

/*
  This holds various functions for using k-NN and Python together.
*/
#ifndef KWM12172002_knnmodule
#define KWM12172002_knnmodule

#include "knn.hpp"
#include <Python.h>

using namespace Gamera;
using namespace Gamera::kNN;

/*
  This enum is for selecting between the various methods of
  computing the distance between two floating-point feature
  vectors.
*/
enum DistanceType {
  CITY_BLOCK,
  EUCLIDEAN,
  FAST_EUCLIDEAN
};


/*
  get the feature vector from an image. image argument _must_ an image - no
  type checking is performed.
*/
inline int image_get_fv(PyObject* image, double** buf, int* len) {
  ImageObject* x = (ImageObject*)image;

  if (PyObject_CheckReadBuffer(x->m_features) < 0) {
    return -1;
  }

  if (PyObject_AsReadBuffer(x->m_features, (const void**)buf, len) < 0) {
    PyErr_SetString(PyExc_TypeError, "knn: Could not use image as read buffer.");
    return -1;
  }
  if (*len == 0) {
    return -1;
  }
  *len = *len / sizeof(double);
  return 0;
}

/*
  get the id_name from an image. The image argument _must_ be an image -
  no type checking is performed.
*/
inline int image_get_id_name(PyObject* image, char** id_name, int* len) {
  ImageObject* x = (ImageObject*)image;

  // PyList_Size shoule type check the argument
  if (PyList_Size(x->m_id_name) < 1) {
    PyErr_SetString(PyExc_TypeError, "knn: id_name not a list or list is empty.");
    return -1;
  }
  PyObject* id_tuple = PyList_GET_ITEM(x->m_id_name, 0);
  if (PyTuple_Size(id_tuple) != 2) {
    PyErr_SetString(PyExc_TypeError, "knn: id_name is not a tuple or is the wrong size.");
    return -1;
  }
  PyObject* id = PyTuple_GET_ITEM(id_tuple, 1);
  *id_name = PyString_AsString(id);
  if (*id_name == 0) {
    PyErr_SetString(PyExc_TypeError, "knn: could not get string from id_name tuple.");
    return -1;
  }
  *len = PyString_GET_SIZE(id);
  return 0;
}

/*
  Compute the distance between two feature vectors.
*/
inline void compute_distance(DistanceType distance_type, const double* known_buf, int known_len,
			     const double* unknown_buf, double* distance, const double* weights) {

  if (distance_type == CITY_BLOCK) {
    *distance = city_block_distance(known_buf, known_buf + known_len, unknown_buf,
				    weights);
  } else if (distance_type == FAST_EUCLIDEAN) {
    *distance = euclidean_distance(known_buf, known_buf + known_len, unknown_buf,
				   weights);
  } else {
    *distance = euclidean_distance(known_buf, known_buf + known_len, unknown_buf,
				   weights);
  }
}


/*
  Compute the distance between a known and an unknown image
  with weights. This version takes an image and a buffer
  for the unknown image.
*/
inline int compute_distance(DistanceType distance_type, PyObject* known, double* unknown_buf,
			    double* distance, double* weights, int unknown_len) {
  double* known_buf;
  int known_len;

  if (image_get_fv(known, &known_buf, &known_len) < 0)
    return -1;

  if (unknown_len != known_len) {
    PyErr_SetString(PyExc_IndexError, "Array lengths do not match");
    return -1;
  }

  compute_distance(distance_type, known_buf, known_len, unknown_buf, distance, weights);

  return 0;
}

/*
  Compute the distance between a known and an unknown image with weights. This version takes
  an image and a buffer for the unknown image. Arguments must be images - no type checking
  is performed.
*/
inline int compute_distance(DistanceType distance_type, PyObject* known, PyObject* unknown,
			    double* distance, double* weights, int weights_len) {
  double *known_buf, *unknown_buf;
  int known_len, unknown_len;

  if (image_get_fv(known, &known_buf, &known_len) < 0)
    return -1;

  if (image_get_fv(unknown, &unknown_buf, &unknown_len) < 0)
    return -1;

  if (unknown_len != known_len) {
    PyErr_SetString(PyExc_IndexError, "Array lengths do not match");
    return -1;
  }

  if (unknown_len != weights_len) {
    PyErr_SetString(PyExc_IndexError, "Array lengths do not match");
    return -1;
  }
  
  compute_distance(distance_type, known_buf, known_len, unknown_buf, distance, weights);

  return 0;
}

#endif
