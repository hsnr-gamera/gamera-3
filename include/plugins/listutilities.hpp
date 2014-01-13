/*
 *
 * Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, Karl MacMillan
 *               2014      Fabian Schmitt
 *               2009-2014 Christoph Dalitz
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

#ifndef mgd04302003_listutilities
#define mgd04302003_listutilities

#include <Python.h>
#include "gameramodule.hpp"
#include "canonicpyobject.hpp"
#include <vector>
#include <algorithm>

namespace Gamera {

  // linear time median implementation for vectors of arithmetic objects
  template<class T>
  T median(std::vector<T>* v, bool inlist=false) {
    T m;
    size_t n = v->size();
    std::nth_element(v->begin(), v->begin() + n/2, v->end());
    m = *(v->begin() + n/2);
    if (!inlist && (0 == n % 2)) {
      std::nth_element(v->begin(), v->begin() + n/2 - 1, v->end());
      m = (m + *(v->begin()+n/2-1)) / 2;
    }
    return m;
  }

  // specialized median implementation for arbitrary Python lists
  PyObject* median_py(PyObject* list, bool inlist=false) {
    size_t n,i;
    PyObject *entry, *retval;
    if(!PyList_Check(list))
      throw std::runtime_error("median: Input argument is no list.");
    n = PyList_Size(list);
    if (0 == n)
      throw std::runtime_error("median: Input list must not be empty.");
    // distinction based on content type
    entry = PyList_GetItem(list,0);
    if (PyFloat_Check(entry)) {
      FloatVector* v = FloatVector_from_python(list);
      if (!v)
        throw std::runtime_error("median: Cannot convert list to float type. Is the list inhomogeneous?");
      double m = median(v, inlist);
      delete v;
      return Py_BuildValue("f",m);
    }
    else if (PyInt_Check(entry)) {
      IntVector* v = IntVector_from_python(list);
      if (!v)
        throw std::runtime_error("median: Cannot convert list to int type. Is the list inhomogeneous?");
      int m = median(v, inlist);
      delete v;
      return Py_BuildValue("i",m);
    }
    else {
      // for arbitrary Python lists, we need a wrapper class to
      // make it passable to a C++ vector<>
      std::vector<canonicPyObject>* v = new std::vector<canonicPyObject>;
      PyTypeObject* type = entry->ob_type;
      for(i=0;i<n;++i) {
        entry = PyList_GetItem(list,i);
        if (!PyObject_TypeCheck(entry,type))
          throw std::runtime_error("median: All list entries must be of the same type.");
        v->push_back(canonicPyObject(entry));
      }  
      std::nth_element(v->begin(), v->begin() + n/2, v->end());
      retval = (v->begin() + n/2)->value;
      delete v;
      Py_INCREF(retval);
      return retval;
    }
  }

  int permute_list(PyObject* list) {
    if (!PyList_Check(list)) {
      PyErr_Format(PyExc_TypeError, "Python list required.");
      return 0;
    }
    size_t n = PyList_Size(list);
    size_t j = 1;
    while (j < n && PyObject_Compare(PyList_GET_ITEM(list, j - 1), PyList_GET_ITEM(list, j)) > -1)
      ++j;
    if (j >= n)
      return 0;
    size_t l = 0;
    PyObject* tmp = PyList_GET_ITEM(list, j);
    while (PyObject_Compare(PyList_GET_ITEM(list, l), tmp) > -1)
      ++l;
  
    PyList_SET_ITEM(list, j, PyList_GET_ITEM(list, l));
    PyList_SET_ITEM(list, l, tmp);

    size_t k = 0;
    l = j - 1;
    while (k < l) {
      tmp = PyList_GET_ITEM(list, k);
      PyList_SET_ITEM(list, k, PyList_GET_ITEM(list, l));
      PyList_SET_ITEM(list, l, tmp);
      ++k;
      --l;
    }
    return 1;
  }

  PyObject* all_subsets(PyObject* a_input, int k) {

    // special treatment for k=0: only one set (the empty set)
    if (k==0) {
      PyObject* result = PyList_New(1);
      PyList_SetItem(result, 0, PyList_New(0));
      return result;
    }

    PyObject* a = PySequence_Fast(a_input, "First argument must be iterable");
    if (a == NULL)
      return 0;

    int n = PySequence_Fast_GET_SIZE(a);
    if (k < 0 || k > n) {
      Py_DECREF(a);
      throw std::runtime_error("k must be between 0 and len(a)");
    }

    PyObject* result = PyList_New(0);
    std::vector<int> indices(k);
    bool start = true;
    int m2 = 0;
    int m = k;
    do {
      if (start) {
        start = false;
      } else {
        if (m2 < n - m)
          m = 0;
        m++;
        m2 = indices[k - m];
      }

      for (int j = 1; j <= m; ++j) 
        indices[k + j - m - 1] = m2 + j;

      PyObject* subset = PyList_New(k);
      for (int j = 0; j < k; ++j) {
        PyObject* item = PySequence_Fast_GET_ITEM(a, indices[j] - 1);
        Py_INCREF(item);
        PyList_SetItem(subset, j, item);
      }
      PyList_Append(result, subset);
      Py_DECREF(subset);
    } while (indices[0] != n - k + 1);
  
    Py_DECREF(a);
    return result;
  }



  FloatVector* kernel_density(FloatVector* values, FloatVector* x, double bw=0.0, int kernel=0)
  {
	if (values->size() == 0)
      throw std::runtime_error("no values given for kernel density estimation");
	if (x->size() == 0)
      throw std::runtime_error("no x given for kernel density estimation");
    if (kernel<0 || kernel>2)
      throw std::runtime_error("kernel must be 0 (rectangular), 1 (triangular), or 2 (gaussian)");

    // copy values because sort changes vector
	FloatVector val_cop = FloatVector(*values);
	std::sort(val_cop.begin(), val_cop.end());
	
	//Silverman's Rule of Thumb
	if (bw == 0.0 && val_cop.size() > 1) {
      // compute variance
      double mu = 0.0;
      for (size_t i = 0; i < val_cop.size(); i++)
		mu += val_cop[i];
      mu /= val_cop.size();
      double var = 0.0;
      for (size_t i = 0; i < val_cop.size(); i++)
		var += (val_cop[i] - mu)*(val_cop[i] - mu);
      var /= (val_cop.size() - 1);
      // compute inter-quartile range
      size_t lq = val_cop.size() / 4;
      size_t uq = (val_cop.size() * 3) / 4;
      double iqr = val_cop[uq] - val_cop[lq];
      // Silverman's rule
      bw = 0.9 * std::min(sqrt(var), iqr/1.34) * pow((double)val_cop.size(),-0.2);
    }
	if (bw == 0.0) // can happen when almost all values are identical
      bw = 1.0;

	const double pre_gaus = 1.0/sqrt(2 * M_PI);
	const double sqrt6 = sqrt(6.0);

	FloatVector* result_vec = new FloatVector(x->size(),0.0);
	for(size_t i = 0; i < x->size(); i++) {
      double result = 0;
      for(size_t j = 0; j < values->size(); j++) {
        double k_x = (x->at(i) - values->at(j)) / bw;
        switch(kernel) {
        case 0:	//rectangular
          if (abs(k_x) <= 1.732051)  // sqrt(3)
            result += 0.2886751;     // 1/(2*sqrt(3))
          break;
        case 1:	//triangular
          if (abs(k_x) <= sqrt6)
            result += (sqrt6 - abs(k_x)) / (sqrt6*sqrt6);
          break;
        case 2:	//gaussian
          result += pre_gaus * exp(-k_x*k_x/2.0);
          break;
        }
      }
      result_vec->at(i) = result / (bw * values->size());
    }
	
	return result_vec;
  }

}

#endif
