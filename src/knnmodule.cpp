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
#include <string.h>

using namespace Gamera;
using namespace Gamera::kNN;

extern "C" {
  void initknn(void);
  static PyObject* knn_new(PyObject* pytype, PyObject* args, PyObject* kwds);
  static void knn_dealloc(PyObject* self);
  static PyObject* knn_classify_using_list(PyObject* self, PyObject* args);
}

struct KnnObject {
  PyObject_HEAD
};

static PyTypeObject KnnType {
  PyObject_HEAD_INIT(NULL)
  0,
};

PyMethodDef knn_methods[] = {
  { "classify_with_images", knn_classify_with_images, METH_VARARGS,
    "foo" },
  { NULL }
};

// for type checking images - see initknn.
static PyTypeObject* imagebase_type;

static PyObject* knn_new(PyObject* pytype, PyObject* args, PyObject* kwds) {
  KnnObject* o;
  o = (KnnObject*)pytype->tp_alloc(pytype, 0);
  return (PyObject*)o;
}

static void knn_dealloc(PyObject* self) {
  self->ob_type->tp_free(self);
}

/*
  get the feature vector from an image. image argument _must_ an image - no
  type checking is performed.
*/
inline int image_get_fv(PyObject* image, double** buf, int* len) {
  ImageObject* x = (ImageObject*)image;
  if (PyObject_AsReadBuffer(x->m_features, (const void**)buf, len) < 0) {
    PyErr_SetString(PyExc_TypeError, "knn: Could not use image as read buffer.");
    return -1;
  }
  return buf;
}

/*
  get the id_name from an image. The image argument _must_ be n image -
  no type checking is performed.
*/
inline int image_get_id_name(PyObject* image, char** id_name) {
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
}

/*
  Compute the distance between a known and an unknown feature
  vector with weights.
*/
inline int compute_distance(PyObject* known, PyObject* unknown, double* distance) {
  double* buf;
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

struct ltstr {
  bool operator()(const char* s1, const char* s2) const {
    return strcmp(s1, s2) < 0;
  }
};

static PyObject* knn_classify_with_images(PyObject* self, PyObject* args) {
  PyObject* unknown, *known, *weights;
  if (PyArg_ParseTuple(args, "OOO", &known, &unknown, &weights) <= 0) {
    return 0;
  }
  if (!PyList_Check(known)) {
    PyErr_SetString(PyExc_TypeError, "Known features must be a list!");
    return 0;
  }
  int known_size = PyList_Size(known);
  if (known_size == 0) {
    PyErr_SetString(PyExc_TypeError, "List must be greater than 0.");
    return 0;
  }

  kNearestNeighbors<PyObject*> knn(1);
  for (int i = 0; i < known_size; ++i) {
    PyObject* cur = PyList_GET_ITEM(known, i);
    
    double distance;
    if (compute_distance(cur, unknown, weights,
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

DL_EXPORT(void) initknn(void) {
  Py_InitModule("knn", knn_methods);
  PyObject* mod = PyImport_ImportModule("gamera.gameracore");
  if (mod == 0) {
    PyErr_SetString(PyExc_RuntimeError, "Unable to load gameracore.\n");
    return;
  }
  PyObject* dict = PyModule_GetDict(mod);
  if (dict == 0) {
    PyErr_SetString(PyExc_RuntimeError, "Unable to get module dictionary\n");
    return;
  }
  imagebase_type = (PyTypeObject*)PyDict_GetItemString(dict, "Image");
}
