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

#ifndef KWM12172002_distance
#define KWM12172002_distance

#include "knn.hpp"

#include <stdexcept>
#include <algorithm>

#ifndef GAMERA_NO_PYTHON
  #include "Python.h"
  #include "knnmodule.hpp"
#endif

namespace Gamera {
  
  Image* distance_matrix(std::vector<Image*>& images) {
    
    if (images.size() == 0)
      throw std::out_of_range("list must be greater than 0.");

    int features_len = images[0]->features_len;
    if (features_len <= 0)
      throw std::out_of_range("no features found.");

    double* weights = new double[features_len];
    std::fill(weights, weights + features_len, 1.0);
    FloatImageData* data = new FloatImageData(images.size(), images.size());
    FloatImageView* mat = new FloatImageView(*data, 0, 0, images.size(), images.size());
    
    for (size_t i = 0; i < images.size(); ++i) {
      for (size_t j = 0; j < images.size(); ++j) {
	if (images[j]->features_len != features_len)
	  throw std::out_of_range("feature vectors not the same size");
	double distance = kNN::city_block_distance(images[i]->features,
						   images[i]->features + features_len,
						   images[j]->features, weights);
	mat->set(i, j,(float)distance);
      }
    }
    delete[] weights;
    return mat;
  }

  /*
    unique_distances takes a list of images and returns all of the unique
    pairs of distances between the images. In the Python version it returns
    a list of tuples ( (image, image, distance) ).
  */
#ifndef GAMERA_NO_PYTHON
  PyObject* unique_distances(PyObject* images) {
    // images is a list of Gamera/Python ImageObjects
    if (!PyList_Check(images)) {
      PyErr_SetString(PyExc_TypeError, "Images must be a list.");
      return 0;
    }
    int images_len = PyList_Size(images);
    if (!(images_len > 1)) {
      PyErr_SetString(PyExc_TypeError, "List must have at least two images.");
      return 0;
    }
    // create the list for the output
    int list_len = ((images_len * images_len) - images_len) / 2;
    PyObject* list = PyList_New(list_len);
    size_t index = 0;
    // create a default set of weights for the distance calculation.
    double* buf_a, *buf_b;
    int len_a, len_b;
    PyObject* cur_a, *cur_b;
    cur_a = PyList_GET_ITEM(images, 0);
    if (image_get_fv(cur_a, &buf_a, &len_a) < 0)
      return 0;
    double* weights = new double[len_a];
    std::fill(weights, weights + len_a, 1.0);
    // create the normalization object
    kNN::Normalize norm(len_a);
    for (int i = 0; i < images_len; ++i) { 
      cur_a = PyList_GetItem(images, i);
      if (cur_a == NULL) {
	delete[] weights;
	return 0;
      }
      if (image_get_fv(cur_a, &buf_a, &len_a) < 0) {
	delete[] weights;
	return 0;
      }
      norm.add(buf_a, buf_a + len_a);
    }
    norm.compute_normalization();
    double* tmp_a = new double[len_a];
    double* tmp_b = new double[len_a];
    // do the distance calculations
    for (int i = 0; i < images_len; ++i) { 
      cur_a = PyList_GetItem(images, i);
      if (cur_a == NULL) {
	delete[] weights; delete[] tmp_a; delete[] tmp_b;
	return 0;
      }
      if (image_get_fv(cur_a, &buf_a, &len_a) < 0) {
	delete[] weights; delete[] tmp_a; delete[] tmp_b;
	return 0;
      }
      norm.apply(buf_a, buf_a + len_a, tmp_a);
      for (int j = i + 1; j < images_len; ++j) {
	cur_b = PyList_GetItem(images, j);
	if (cur_b == NULL) {
	  delete[] weights; delete[] tmp_a; delete[] tmp_b;
	  return 0;
	}
	if (image_get_fv(cur_b, &buf_b, &len_b) < 0) {
	  delete[] weights; delete[] tmp_a; delete[] tmp_b;
	  return 0;
	}
	if (len_a != len_b) {
	  PyErr_SetString(PyExc_RuntimeError, "Feature vector lengths do not match!");
	  delete[] weights; delete[] tmp_a; delete[] tmp_b;
	  return 0;
	}
	norm.apply(buf_b, buf_b + len_b, tmp_b);
	double distance;
	compute_distance(CITY_BLOCK, tmp_a, len_a, tmp_b, &distance, weights);
	PyList_SET_ITEM(list, index, Py_BuildValue("(dOO)", distance, cur_a, cur_b));
	index++;
      }
    }
    delete[] weights; delete[] tmp_a; delete[] tmp_b;
    if (PyList_Sort(list) < 0)
      return 0;
    return list;
  }
#endif
}

#endif
