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
#include <algorithm>
#include <string.h>
#include <Python.h>
#include <string>
#include <vector>
#include <functional>

using namespace Gamera;
using namespace Gamera::kNN;

extern "C" {
  void initknn(void);
  static PyObject* knn_new(PyTypeObject* pytype, PyObject* args,
			   PyObject* kwds);
  static void knn_dealloc(PyObject* self);
  static PyObject* knn_instantiate_from_images(PyObject* self, PyObject* args);
  static PyObject* knn_classify(PyObject* self, PyObject* args);
  static PyObject* knn_classify_with_images(PyObject* self, PyObject* args);
  static PyObject* knn_get_interactive(PyObject* self);
}

static PyTypeObject KnnType = {
  PyObject_HEAD_INIT(NULL)
  0,
};

enum DistanceType {
  CITY_BLOCK,
  EUCLIDEAN,
  FAST_EUCLIDEAN
};

struct KnnObject {
  PyObject_HEAD
  size_t num_features;
  size_t num_feature_vectors;
  double* feature_vectors;
  std::vector<std::string>* id_names;
  size_t num_k;
  DistanceType distance_type;
};


PyMethodDef knn_methods[] = {
  { "classify_with_images", knn_classify_with_images, METH_VARARGS,
    "classify an unknown image using a list of images." },
  { "instantiate_from_images", knn_instantiate_from_images, METH_VARARGS,
    "" },
  { "classify", knn_classify, METH_VARARGS,
    "" },
  { NULL }
};

PyGetSetDef knn_getset[] = {
  { "interactive", (getter)knn_get_interactive, 0,
    "bool property indicating whether this object supports interactive classification.", 0 },
  { NULL }
};

// for type checking images - see initknn.
static PyTypeObject* imagebase_type;

static PyObject* knn_new(PyTypeObject* pytype, PyObject* args,
			 PyObject* kwds) {
  KnnObject* o;
  o = (KnnObject*)pytype->tp_alloc(pytype, 0);
  o->num_features = 0;
  o->num_feature_vectors = 0;
  o->feature_vectors = 0;
  o->id_names = 0;
  o->num_k = 1;
  return (PyObject*)o;
}

static void knn_delete_data(KnnObject* o) {
  if (o->feature_vectors != 0)
    delete o->feature_vectors;
  if (o->id_names != 0) {
    delete o->id_names;
  }
  o->num_features = 0;
  o->num_feature_vectors = 0;
}

static void knn_dealloc(PyObject* self) {
  KnnObject* o = (KnnObject*)self;
  knn_delete_data(o);
  self->ob_type->tp_free(self);
}

struct ltstr {
  bool operator()(const char* s1, const char* s2) const {
    return strcmp(s1, s2) < 0;
  }
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
  get the id_name from an image. The image argument _must_ be n image -
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

static PyObject* knn_instantiate_from_images(PyObject* self, PyObject* args) {
  PyObject* images;
  KnnObject* o = (KnnObject*)self;
  if (PyArg_ParseTuple(args, "O", &images) <= 0) {
    return 0;
  }
  if (!PyList_Check(images)) {
    PyErr_SetString(PyExc_TypeError, "knn: images must be a list!");
    return 0;
  }
  int images_size = PyList_Size(images);
  if (images_size == 0) {
    PyErr_SetString(PyExc_TypeError, "List must be greater than 0.");
    return 0;
  }
  knn_delete_data(o);
  o->num_feature_vectors = images_size;
  std::cout << o->num_feature_vectors << std::endl;

  PyObject* first_image = PyList_GET_ITEM(images, 0);
  if (!PyObject_TypeCheck(first_image, imagebase_type)) {
    PyErr_SetString(PyExc_TypeError, "knn: expected an image");
    return 0;
  }

  double* tmp_fv;
  int tmp_fv_len;
  if (image_get_fv(first_image, &tmp_fv, &tmp_fv_len) < 0) {
    PyErr_SetString(PyExc_TypeError, "knn: could not get features from image");
    return 0;
  }
  o->num_features = tmp_fv_len;
  o->feature_vectors = new double[(o->num_feature_vectors + 1)* o->num_features];
  o->id_names = new std::vector<std::string>;
  double* current_features = o->feature_vectors;
  for (size_t i = 0; i < o->num_feature_vectors; ++i, current_features += o->num_features) {
    //std::cout << i << std::endl;
    PyObject* cur_image = PyList_GetItem(images, i);
    if (image_get_fv(cur_image, &tmp_fv, &tmp_fv_len) < 0) {
      knn_delete_data(o);
      PyErr_SetString(PyExc_TypeError, "knn: could not get features from image");
      return 0;
    }
    if (size_t(tmp_fv_len) != o->num_features) {
      knn_delete_data(o);
      PyErr_SetString(PyExc_TypeError, "knn: feature vector lengths don't match");
      return 0;      
    }
    for (size_t feature = 0; feature < o->num_features; ++feature) {
      current_features[i] = tmp_fv[feature];
    }
    char* tmp_id_name;
    int len;
    if (image_get_id_name(cur_image, &tmp_id_name, &len) < 0) {
      knn_delete_data(o);
      PyErr_SetString(PyExc_TypeError, "knn: could not get id name");
      return 0;
    }
    std::string id_string(tmp_id_name, len);
    o->id_names->push_back(id_string);
  }
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject* knn_classify(PyObject* self, PyObject* args) {
  KnnObject* o = (KnnObject*)self;
  if (o->feature_vectors == 0) {
      PyErr_SetString(PyExc_RuntimeError,
		      "knn: classify called before instantiate from images");
      return 0;          
  }
  PyObject* unknown;
  if (PyArg_ParseTuple(args, "O", &unknown) <= 0) {
    return 0;
  }

  if (!PyObject_TypeCheck(unknown, imagebase_type)) {
    PyErr_SetString(PyExc_TypeError, "knn: unknown must be an image");
    return 0;
  }
  double* fv;
  int fv_len;
  if (image_get_fv(unknown, &fv, &fv_len) < 0) {
    PyErr_SetString(PyExc_TypeError, "knn: could not get features");
    return 0;
  }
  if (size_t(fv_len) != o->num_features) {
    PyErr_SetString(PyExc_TypeError, "knn: features not the correct size");
    return 0;
  }

  kNearestNeighbors<std::string, std::less<std::string> > knn(3);
  double* current_known = o->feature_vectors;
  double* weights = new double[o->num_features];
  std::fill(weights, weights + o->num_features, 1.0);
  for (size_t i = 0; i < o->num_features; ++i)
    std::cout << fv[i] << " ";
  std::cout << std::endl;
  for (size_t i = 0; i < o->num_feature_vectors; ++i, current_known += o->num_features) {
    double distance = city_block_distance(current_known, current_known + o->num_features,
					  fv, weights);
    knn.add((*o->id_names)[i], distance);
  }
  std::pair<std::string, double> answer = knn.majority();
  PyObject* ans = PyTuple_New(2);
  PyTuple_SET_ITEM(ans, 0, PyFloat_FromDouble(answer.second));
  PyTuple_SET_ITEM(ans, 1, PyString_FromString(answer.first.c_str()));
  PyObject* ans_list = PyList_New(1);
  PyList_SET_ITEM(ans_list, 0, ans);
  return ans_list;
}


/*
  Compute the distance between a known and an unknown feature
  vector with weights.
*/
inline int compute_distance(PyObject* known, PyObject* unknown, double* weights,
			    double* distance) {
  double* known_buf, *unknown_buf;
  int known_len, unknown_len;

  if (image_get_fv(unknown, &unknown_buf, &unknown_len) < 0)
    return -1;

  if (image_get_fv(known, &known_buf, &known_len) < 0)
    return -1;

  if (unknown_len != known_len) {
    PyErr_SetString(PyExc_IndexError, "Array lengths do not match");
    return -1;
  }

  *distance = city_block_distance(known_buf, known_buf + known_len, unknown_buf,
				  weights);
  return 0;
}

static PyObject* knn_classify_with_images(PyObject* self, PyObject* args) {
  PyObject* unknown, *known;
  if (PyArg_ParseTuple(args, "OO", &known, &unknown) <= 0) {
    return 0;
  }

  if (!PyList_Check(known)) {
    PyErr_SetString(PyExc_TypeError, "Known features must be a list!");
    return 0;
  }
  printf("supports iter %\n", PyIter_Check(known));

  int known_size = PyList_Size(known);
  if (known_size == 0) {
    PyErr_SetString(PyExc_TypeError, "List must be greater than 0.");
    return 0;
  }

  if (!PyObject_TypeCheck(unknown, imagebase_type)) {
    PyErr_SetString(PyExc_TypeError, "knn: unknown must be an image");
    return 0;
  }
    

  /*
    create an empty weight vector.
  */
  double* weights;
  int len;
  if (image_get_fv(unknown, &weights, &len) < 0)
    return 0;
  weights = new double[len];
  std::fill(weights, weights + len, 1.0);

  kNearestNeighbors<char*, ltstr> knn(1);
  for (int i = 0; i < known_size; ++i) {
    PyObject* cur = PyList_GET_ITEM(known, i);
    
    if (!PyObject_TypeCheck(cur, imagebase_type)) {
      PyErr_SetString(PyExc_TypeError, "knn: non-image in known list");
    }
    double distance;
    if (compute_distance(cur, unknown, weights, &distance) < 0)
      return 0;
    
    char* id_name;
    int len;
    if (image_get_id_name(cur, &id_name, &len) < 0)
      return 0;
    knn.add(id_name, distance);
  }
  delete weights;

  std::pair<char*, double> answer = knn.majority();
  PyObject* ans = PyTuple_New(2);
  PyTuple_SET_ITEM(ans, 0, PyFloat_FromDouble(answer.second));
  PyTuple_SET_ITEM(ans, 1, PyString_FromString(answer.first));
  PyObject* ans_list = PyList_New(1);
  PyList_SET_ITEM(ans_list, 0, ans);
  return ans_list;
}

static PyObject* knn_get_interactive(PyObject* self) {
  Py_INCREF(Py_True);
  return Py_True;
}


PyMethodDef knn_module_methods[] = {
  { NULL }
};

DL_EXPORT(void) initknn(void) {
  PyObject* m = Py_InitModule("gamera.knn", knn_module_methods);
  PyObject* d = PyModule_GetDict(m);

  KnnType.ob_type = &PyType_Type;
  KnnType.tp_name = "gamera.knn.kNN";
  KnnType.tp_basicsize = sizeof(KnnObject);
  KnnType.tp_dealloc = knn_dealloc;
  KnnType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  KnnType.tp_new = knn_new;
  KnnType.tp_getattro = PyObject_GenericGetAttr;
  KnnType.tp_alloc = PyType_GenericAlloc;
  KnnType.tp_free = _PyObject_Del;
  KnnType.tp_methods = knn_methods;
  KnnType.tp_getset = knn_getset;
  PyDict_SetItemString(d, "kNN", (PyObject*)&KnnType);

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
