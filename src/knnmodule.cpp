/*
 *
 * Copyright (C) 2002 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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

#include "gameramodule.hpp"
#include "knn.hpp"

using namespace Gamera;
using namespace Gamera::kNN;

extern "C" {
  void initknn(void);
  static PyObject* knn_classify_using_list(PyObject* self, PyObject* args);
}

/*
  Compute the distance between a known and an unknown feature
  vector with weights.
*/
inline int compute_distance(PyObject* known, PyObject* unknown,
			    PyObject* weights, double* distance) {
  const void* buf;
  int known_len, len;
  if (PyObject_AsReadBuffer(known, &buf, &known_len) < 0)
    return -1;

  double* k = (double*)buf;
  int size = known_len / sizeof(double);

  if (PyObject_AsReadBuffer(unknown, &buf, &len) < 0)
    return -1;
  if (len != known_len) {
    PyErr_SetString(PyExc_IndexError, "Array lengths do not match");
    return -1;
  }

  double* u = (double*)buf;

  if (PyObject_AsReadBuffer(weights, &buf, &len) < 0)
    return -1;
  if (len != known_len) {
    PyErr_SetString(PyExc_IndexError, "Array lengths do not match");
    return -1;
  }

  double* w = (double*)buf;

  *distance = city_block_distance(k, k + size, u, w);
  return 0;
}


#define CHECK_ITEM(tuple) \
  if (!PyTuple_Check(tuple)) { \
    PyErr_SetString(PyExc_TypeError, "Known items must be a tuple!"); \
    return 0; \
  } else if (PyTuple_GET_SIZE(tuple) != 2) { \
    PyErr_SetString(PyExc_IndexError, "Tuple must be 2 elements!"); \
    return 0; \
  }

static char knn_classify_using_list_doc[] = "classify_list(unknown, known)
Classify an unknown object represented by an array using a list of known
objects.";
static PyObject* knn_classify_using_list(PyObject* self, PyObject* args) {
  PyObject* unknown, *known, *weights;
  if (PyArg_ParseTuple(args, "OOO", &known, &unknown, &weights) <= 0) {
    return 0;
  }
  if (!PyList_Check(known)) {
    PyErr_SetString(PyExc_TypeError, "Known features must be a list!");
    return 0;
  }
  int known_size = PyList_Size(known);
  if (known_size <= 0) {
    PyErr_SetString(PyExc_TypeError, "List must be greater than 0.");
    return 0;
  }

  kNearestNeighbors<PyObject*> knn(1);
  for (int i = 0; i < known_size; ++i) {
    PyObject* cur = PyList_GET_ITEM(known, i);
    CHECK_ITEM(cur);
    double distance;
    if (compute_distance(PyTuple_GET_ITEM(cur, 1), unknown, weights,
			 &distance) < 0)
      return 0;
    knn.add(PyTuple_GET_ITEM(cur, 0), distance);
  }
  std::pair<PyObject*, double> answer = knn.majority();
  PyObject* ans = PyTuple_New(2);
  Py_INCREF(answer.first);
  PyTuple_SET_ITEM(ans, 0, answer.first);
  PyTuple_SET_ITEM(ans, 1, PyFloat_FromDouble(answer.second));
  return ans;
}


PyMethodDef knn_methods[] = {
  { "classify_using_list", knn_classify_using_list, METH_VARARGS,
    knn_classify_using_list_doc },
  { NULL, NULL },
};


DL_EXPORT(void) initknn(void) {
  Py_InitModule("knn", knn_methods);
  
}
