//-*- indent-tabs-mode: nil; -*-
/*
 *
 * Copyright (C) 2001-2009 Ichiro Fujinaga, Michael Droettboom,
 *                         Karl MacMillan, and Christoph Dalitz
 *               2012      David Kolanus
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

/*
  This module implements the low-level parts of the kNN classifier object. This
  implements the generic classifier interface for Gamera.
*/
#include "gameramodule.hpp"
#include "knn.hpp"
#include "knnmodule.hpp"
#include <algorithm>
#include <vector>
#include <map>
#include <string.h>
#include <Python.h>
#include <assert.h>
#include <stdio.h>
// for ga optimization
#include <ga/ga.h>
#include <ga/GASimpleGA.h>
#include <ga/GA1DArrayGenome.h>
#include <ga/GA1DArrayGenome.cpp>
// for rand
#include <stdlib.h>
#include <time.h>

using namespace Gamera;
using namespace Gamera::kNN;

extern "C" {
  DL_EXPORT(void) initknncore(void);
  // Construction/destruction
  static PyObject* knn_new(PyTypeObject* pytype, PyObject* args,
                           PyObject* kwds);
  static void knn_dealloc(PyObject* self);
  static PyObject* knn_instantiate_from_images(PyObject* self, PyObject* args);
  // classification
  static PyObject* knn_classify(PyObject* self, PyObject* args);
  static PyObject* knn_classify_with_images(PyObject* self, PyObject* args);
  static PyObject* knn_leave_one_out(PyObject* self, PyObject* args);
  // distance
  static PyObject* knn_knndistance_statistics(PyObject* self, PyObject* args);
  static PyObject* knn_distance_from_images(PyObject* self, PyObject* args);
  static PyObject* knn_distance_between_images(PyObject* self, PyObject* args);
  static PyObject* knn_distance_matrix(PyObject* self, PyObject* args);
  static PyObject* knn_unique_distances(PyObject* self, PyObject* args);
  // settings
  static PyObject* knn_get_num_k(PyObject* self);
  static int knn_set_num_k(PyObject* self, PyObject* v);
  static PyObject* knn_get_distance_type(PyObject* self);
  static int knn_set_distance_type(PyObject* self, PyObject* v);
  static PyObject* knn_get_confidence_types(PyObject* self);
  static int knn_set_confidence_types(PyObject* self, PyObject* v);
  static PyObject* knn_get_weights(PyObject* self, PyObject* args);
  static PyObject* knn_set_weights(PyObject* self, PyObject* args);
  static PyObject* knn_get_num_features(PyObject* self);
  static int knn_set_num_features(PyObject* self, PyObject* v);
  // saving/loading
  static PyObject* knn_serialize(PyObject* self, PyObject* args);
  static PyObject* knn_unserialize(PyObject* self, PyObject* args);
  // GA for optimization
  static PyObject* knn_ga_create(PyObject* self, PyObject* args);
  static PyObject* knn_ga_destroy(PyObject* self, PyObject* args);
  static PyObject* knn_ga_step(PyObject* self, PyObject* args);
  static PyObject* knn_get_ga_mutation(PyObject* self);
  static int knn_set_ga_mutation(PyObject* self, PyObject* v);
  static PyObject* knn_get_ga_crossover(PyObject* self);
  static int knn_set_ga_crossover(PyObject* self, PyObject* v);
  static PyObject* knn_get_ga_population(PyObject* self);
  static int knn_set_ga_population(PyObject* self, PyObject* v);
}

static PyTypeObject KnnType = {
  PyObject_HEAD_INIT(NULL)
  0,
};

/*
  The KnnObject holds all of the information needed by knn. Unlike
  many of the parts of Gamera, there is a significant amount of
  functionality implemented in this module rather than just a
  wrapper around a C++ objects/code.
*/
struct KnnObject {
  PyObject_HEAD
  // the number of features in each feature vector
  size_t num_features;
  // the total number of feature vectors stored in the database
  size_t num_feature_vectors;
  /*
    The feature vectors. A flat array of doubles (of size num_features
    * num_feature_vectors) is used to store the feature vectors for
    performance reasons (the memory access will be faster than using
    a multi-dimensional data structure). This does not complicate the
    implementation because this array is a fixed size (it is only
    used for non-interactive classification).
  */
  double* feature_vectors;
  // The id_names for the feature vectors
  char** id_names;
  // confidence types to be computed during classification
  std::vector<int> confidence_types;
  // The current weights applied to the distance calculation
  double* weight_vector;
  // a histogram of the id_names for use in leave-one-out
  int* id_name_histogram;
  /*
    The normalization applied to the feature vectors prior to distance
    calculation.
  */
  Normalize* normalize;
  /*
    Temporary storage for the normalized version of the unknown feature
    vector. This is simply to avoid allocating memory for each call to
    classify (and could potentially increase our cache hit rate, but who
    really knows).
  */
  double* normalized_unknown;
  // k - this is k-NN after all
  size_t num_k;
  // the distance type currently being used.
  DistanceType distance_type;
  /*
    GA
  */
  GA1DArrayGenome<double>* genome;
  GASteadyStateGA* ga;
  size_t ga_population;
  double ga_mutation;
  double ga_crossover;
  bool ga_running;
};


PyMethodDef knn_methods[] = {
  { (char *)"classify_with_images", knn_classify_with_images, METH_VARARGS,
    (char *) "(id_name, confidencemap) **classify_with_images** (ImageList *glyphs*, Image *glyph*, bool cross_validation_mode=False, bool do_confidence=True )\n"
    "\nClassifies an unknown image using the given list of images as training data.\n"
    "The *glyph* is classified without setting its classification.  The\n"
    "return value is a tuple of the form ``(id_name,confidencemap)``, where\n"
    "*idname* is a list of the form `idname`_, and *confidencemap* is a\n"
    "map of the form `confidence`_ listing the confidences of the main id.\n"
    "\n"
    ".. _idname: #id-name\n\n"
    ".. _confidence: #confidence"
  },
  { (char *)"instantiate_from_images", knn_instantiate_from_images, METH_VARARGS,
    (char *)"Use the list of images for non-interactive classification." },
  { (char *)"_distance_from_images", knn_distance_from_images, METH_VARARGS, (char *)"" },
  { (char *)"_distance_between_images", knn_distance_between_images, METH_VARARGS, (char *)"" },
  { (char *)"_distance_matrix", knn_distance_matrix, METH_VARARGS, (char *)"" },
  { (char *)"_unique_distances", knn_unique_distances, METH_VARARGS, (char *)"" },
  { (char *)"set_weights", knn_set_weights, METH_VARARGS,
    (char *)"Set the weights used for classification." },
  { (char *)"get_weights", knn_get_weights, METH_VARARGS,
    (char *)"Get the weights used for classification." },
  { (char *)"classify", knn_classify, METH_VARARGS,
    (char *)"" },
  { (char *)"leave_one_out", knn_leave_one_out, METH_VARARGS, (char *)"" },
  { (char *)"_knndistance_statistics", knn_knndistance_statistics, METH_VARARGS,
    (char *)"" },
  { (char *)"serialize", knn_serialize, METH_VARARGS, (char *)"" },
  { (char *)"unserialize", knn_unserialize, METH_VARARGS, (char *)"" },
  { (char *)"_ga_create", knn_ga_create, METH_VARARGS, (char *)"" },
  { (char *)"_ga_destroy", knn_ga_destroy, METH_VARARGS, (char *)"" },
  { (char *)"_ga_step", knn_ga_step, METH_VARARGS, (char *)"" },
  { NULL }
};

PyGetSetDef knn_getset[] = {
  { (char *)"num_k", (getter)knn_get_num_k, (setter)knn_set_num_k,
    (char *)"The value of k used for classification.", 0 },
  { (char *)"distance_type", (getter)knn_get_distance_type, (setter)knn_set_distance_type,
    (char *)"The type of distance calculation used.", 0 },
  { (char *)"confidence_types", (getter)knn_get_confidence_types, (setter)knn_set_confidence_types,
    (char *)"The types of confidences computed during classification.", 0 },
  { (char *)"ga_mutation", (getter)knn_get_ga_mutation, (setter)knn_set_ga_mutation,
    (char *)"The mutation rate for GA optimization.", 0 },
  { (char *)"ga_crossover", (getter)knn_get_ga_crossover, (setter)knn_set_ga_crossover,
    (char *)"The crossover rate for GA optimization.", 0 },
  { (char *)"ga_population", (getter)knn_get_ga_population, (setter)knn_set_ga_population,
    (char *)"The population for GA optimization.", 0 },
  { (char *)"num_features", (getter)knn_get_num_features, (setter)knn_set_num_features,
    (char *)"The current number of features.", 0 },
  { NULL }
};

static PyObject* array_init;

/*
  Convenience function to delete all of the dynamic data used for
  classification.
*/
static void knn_delete_feature_data(KnnObject* o) {
  if (o->feature_vectors != 0) {
    delete[] o->feature_vectors;
    o->feature_vectors = 0;
  }
  if (o->id_names != 0) {
    for (size_t i = 0; i < o->num_feature_vectors; ++i) {
      if (o->id_names[i] != 0)
        delete[] o->id_names[i];
    }
    delete[] o->id_names;
    o->id_names = 0;
  }
  if (o->id_name_histogram != 0) {
    delete[] o->id_name_histogram;
    o->id_name_histogram = 0;
  }
  o->num_feature_vectors = 0;
}

static void set_num_features(KnnObject* o, size_t num_features) {
  if (num_features == o->num_features)
    return;
  /*
    To prevent things from being in an unsafe state we delete all
    of the feature data if the number of features has changed.
  */
  knn_delete_feature_data(o);
  o->num_features = num_features;
  if (o->weight_vector != 0)
    delete[] o->weight_vector;
  o->weight_vector = new double[o->num_features];
  std::fill(o->weight_vector, o->weight_vector + o->num_features, 1.0);
  if (o->normalize != 0)
    delete o->normalize;
  o->normalize = new Normalize(o->num_features);
  if (o->normalized_unknown != 0)
    delete[] o->normalized_unknown;
  o->normalized_unknown = new double[o->num_features];
}

/*
  Create a new kNN object and initialize all of the data.
*/
static PyObject* knn_new(PyTypeObject* pytype, PyObject* args,
                         PyObject* kwds) {
  KnnObject* o;
  o = (KnnObject*)pytype->tp_alloc(pytype, 0);
  /*
    Initialize knn
  */
  o->num_features = 0;
  o->num_feature_vectors = 0;
  o->feature_vectors = 0;
  o->id_names = 0;
  o->id_name_histogram = 0;
  o->weight_vector = 0;
  o->normalize = 0;
  o->normalized_unknown = 0;
  o->num_k = 1;
  o->distance_type = CITY_BLOCK;
  o->confidence_types.push_back(CONFIDENCE_DEFAULT);

  /*
    Initialize the ga
  */
  o->ga_running = false;
  o->ga = 0;
  o->genome = 0;
  o->ga_population = 20;
  o->ga_mutation = 0.01;
  o->ga_crossover = 0.6;

  Py_INCREF(Py_None);
  return (PyObject*)o;
}

/*
  Create and initialize all of the classification data with the given
  number of features and number of feature vectors. Throughout this
  object it is assumed that the number of feature vectors is fixed. This
  is reasonable because if you need to classify using a changing set of
  known images classify_with_images is a much easier choice. Because
  we can assume a fixed number of feature vectors it makes allocation
  easier and also allows certain features (like normalization) to become
  a lot easier.
*/
static int knn_create_feature_data(KnnObject* o, size_t num_feature_vectors) {
  try {
    o->num_feature_vectors = num_feature_vectors;
    assert(o->num_feature_vectors > 0);

    o->feature_vectors = new double[o->num_features * o->num_feature_vectors];
    o->id_names = new char*[o->num_feature_vectors];
    for (size_t i = 0; i < o->num_feature_vectors; ++i)
      o->id_names[i] = 0;
    o->id_name_histogram = new int[o->num_feature_vectors];
  } catch (std::exception e) {
    PyErr_SetString(PyExc_RuntimeError, e.what());
    return -1;
  }
  return 1;
}

// destructor for Python
static void knn_dealloc(PyObject* self) {
  KnnObject* o = (KnnObject*)self;
  knn_delete_feature_data(o);
  if (o->weight_vector != 0)
    delete[] o->weight_vector;
  if (o->normalize != 0)
    delete o->normalize;
  if (o->normalized_unknown != 0)
    delete[] o->normalized_unknown;
  self->ob_type->tp_free(self);
}

/*
  String comparison functors used by the kNearestNeighbors object
*/
struct ltstr {
  bool operator()(const char* s1, const char* s2) const {
    return strcmp(s1, s2) < 0;
  }
};
struct eqstr {
  bool operator()(const char* s1, const char* s2) const {
    return strcmp(s1, s2) == 0;
  }
};

/*
  Take a list of images from Python and instatiate the internal data structures
  for knn - this is used for non-interactive classification using the classify
  method. The major difference between interactive classification and non-interactive
  classification (other than speed) is that the data is normalized for non-interactive
  classification. The feature vectors are normalized in place ahead of time, so when
  the classifier is serialized the data is saved normalized. This is appropriate because
  non-interactive classifiers cannot have feature vectors added or deleted by definition.
*/
static PyObject* knn_instantiate_from_images(PyObject* self, PyObject* args) {
  PyObject* images;
  KnnObject* o = (KnnObject*)self;
  if (o->ga_running == true) {
    PyErr_SetString(PyExc_RuntimeError, "knn: cannot call while ga is active.");
    return 0;
  }
  if (PyArg_ParseTuple(args, CHAR_PTR_CAST "O", &images) <= 0) {
    return 0;
  }
  /*
    Unlike classify_with_images this method requires a list so that the
    size can be known ahead of time. One of the advantages of the non-interactive
    classifier is that the data structures can be more static, so knowing the
    size ahead of time is _much_ easier.
  */
  PyObject* images_seq = PySequence_Fast(images, "First argument must be iterable");
  if (images_seq == NULL)
    return 0;

  // delete all the feature data and initialize the object
  knn_delete_feature_data(o);
  if (o->normalize != 0)
    delete o->normalize;
  o->normalize = new Normalize(o->num_features);

  int images_size = PySequence_Fast_GET_SIZE(images_seq);
  if (images_size == 0) {
    PyErr_SetString(PyExc_ValueError, "Initial database of a non-interactive kNN classifier must have at least one element.");
    Py_DECREF(images_seq);
    return 0;
  }

  /*
    Create all of the data
  */
  if (knn_create_feature_data(o, images_size) < 0) {
    Py_DECREF(images_seq);
    return 0;
  }
  /*
    Copy the id_names and the features to the internal data structures.
  */
  double* tmp_fv;
  Py_ssize_t tmp_fv_len;

  std::map<char*, int, ltstr> id_name_histogram;
  double* current_features = o->feature_vectors;
  for (size_t i = 0; i < o->num_feature_vectors; ++i, current_features += o->num_features) {
    PyObject* cur_image = PySequence_Fast_GET_ITEM(images_seq, i);

    if (image_get_fv(cur_image, &tmp_fv, &tmp_fv_len) < 0) {
      knn_delete_feature_data(o);
      PyErr_SetString(PyExc_ValueError, "knn: could not get features from image");
      goto error;
    }
    if (size_t(tmp_fv_len) != o->num_features) {
      knn_delete_feature_data(o);
      PyErr_SetString(PyExc_ValueError, "knn: feature vector lengths don't match");
      goto error;
    }
    std::copy(tmp_fv, tmp_fv + o->num_features, current_features);
    o->normalize->add(tmp_fv, tmp_fv + o->num_features);
    char* tmp_id_name = NULL;
    int len = 0;
    if (image_get_id_name(cur_image, &tmp_id_name, &len) < 0) {
      knn_delete_feature_data(o);
      PyErr_SetString(PyExc_ValueError, "knn: could not get id name");
      goto error;
    }
    o->id_names[i] = new char[len + 1];
    strncpy(o->id_names[i], tmp_id_name, len + 1);
    id_name_histogram[o->id_names[i]]++;
  }

  /*
    Apply the normalization and store the histogram data for fast access in
    leave-one-out.
  */
  o->normalize->compute_normalization();
  current_features = o->feature_vectors;
  for (size_t i = 0; i < o->num_feature_vectors; ++i, current_features += o->num_features) {
    o->normalize->apply(current_features, current_features + o->num_features);
    o->id_name_histogram[i] = id_name_histogram[o->id_names[i]];
  }

  Py_DECREF(images_seq);
  Py_INCREF(Py_None);
  return Py_None;
 error:
  Py_DECREF(images_seq);
  return 0;
}

/*
  non-interactive classification using the data created by
  instantiate from images.
*/
static PyObject* knn_classify(PyObject* self, PyObject* args) {
  KnnObject* o = (KnnObject*)self;
  if (o->ga_running == true) {
    PyErr_SetString(PyExc_RuntimeError, "knn: cannot call while ga is active.");
    return 0;
  }
  if (o->feature_vectors == 0) {
      PyErr_SetString(PyExc_RuntimeError,
                      "knn: classify called before instantiate from images");
      return 0;
  }
  PyObject* unknown;
  if (PyArg_ParseTuple(args, CHAR_PTR_CAST "O", &unknown) <= 0) {
    return 0;
  }

  if (!is_ImageObject(unknown)) {
    PyErr_SetString(PyExc_TypeError, "knn: unknown must be an image");
    return 0;
  }
  double* fv;
  Py_ssize_t fv_len;
  if (image_get_fv(unknown, &fv, &fv_len) < 0) {
    PyErr_SetString(PyExc_ValueError, "knn: could not get features");
    return 0;
  }
  if (size_t(fv_len) != o->num_features) {
    PyErr_SetString(PyExc_ValueError, "knn: features not the correct size");
    return 0;
  }

  // normalize the unknown
  o->normalize->apply(fv, fv + o->num_features, o->normalized_unknown);
  // create the kNN object
  kNearestNeighbors<char*, ltstr, eqstr> knn(o->num_k);
  knn.confidence_types = o->confidence_types;

  double* current_known = o->feature_vectors;

  for (size_t i = 0; i < o->num_feature_vectors; ++i, current_known += o->num_features) {
    double distance;
    if (o->distance_type == CITY_BLOCK) {
      distance = city_block_distance(current_known, current_known + o->num_features,
                                     o->normalized_unknown, o->weight_vector);
    } else if (o->distance_type == FAST_EUCLIDEAN) {
      distance = fast_euclidean_distance(current_known, current_known + o->num_features,
                                         o->normalized_unknown, o->weight_vector);
    } else {
      distance = euclidean_distance(current_known, current_known + o->num_features,
                                    o->normalized_unknown, o->weight_vector);
    }

    knn.add(o->id_names[i], distance);
  }
  knn.majority();
  knn.calculate_confidences();
  PyObject* ans_list = PyList_New(knn.answer.size());
  for (size_t i = 0; i < knn.answer.size(); ++i) {
    // PyList_SET_ITEM steals references so this code only looks
    // like it leaks. KWM
    PyObject* ans = PyTuple_New(2);
    PyTuple_SET_ITEM(ans, 0, PyFloat_FromDouble(knn.answer[i].second));
    PyTuple_SET_ITEM(ans, 1, PyString_FromString(knn.answer[i].first));
    PyList_SET_ITEM(ans_list, i, ans);
  }
  PyObject* conf_dict = PyDict_New();
  for (size_t i = 0; i < knn.confidence_types.size(); ++i) {
    PyObject* o1 = PyInt_FromLong(knn.confidence_types[i]);
    PyObject* o2 = PyFloat_FromDouble(knn.confidence[i]);
    PyDict_SetItem(conf_dict, o1, o2);
    Py_DECREF(o1);
    Py_DECREF(o2);
  }
  PyObject* result = PyTuple_New(2);
  PyTuple_SET_ITEM(result, 0, ans_list);
  PyTuple_SET_ITEM(result, 1, conf_dict);
  return result;
}

static PyObject* knn_classify_with_images(PyObject* self, PyObject* args) {
  KnnObject* o = (KnnObject*)self;
  if (o->ga_running == true) {
    PyErr_SetString(PyExc_RuntimeError, "knn: cannot call while ga is active.");
    return 0;
  }
  PyObject* unknown, *iterator, *container;
  int cross_validation_mode = 0;
  int do_confidence = 1;
  if (PyArg_ParseTuple(args, CHAR_PTR_CAST "OO|ii", &container, &unknown, &cross_validation_mode, &do_confidence) <= 0) {
    return 0;
  }

  iterator = PyObject_GetIter(container);

  if (iterator == NULL) {
    PyErr_SetString(PyExc_TypeError, "Known features must be iterable.");
    return 0;
  }

  if (!is_ImageObject(unknown)) {
    PyErr_SetString(PyExc_TypeError, "knn: unknown must be an image");
    return 0;
  }

  double* unknown_buf;
  Py_ssize_t unknown_len;
  if (image_get_fv(unknown, &unknown_buf, &unknown_len) < 0) {
      PyErr_SetString(PyExc_ValueError,
                      "knn: error getting feature vector \
                       (This is most likely because features have not been generated.)");
      return 0;
  }

  if (size_t(unknown_len) != o->num_features) {
    PyErr_SetString(PyExc_RuntimeError, "knn: the number of features does not match.");
    return 0;
  }

  kNearestNeighbors<char*, ltstr, eqstr> knn(o->num_k);
  knn.confidence_types = o->confidence_types;

  PyObject* cur;
  while ((cur = PyIter_Next(iterator))) {

    if (!is_ImageObject(cur)) {
      PyErr_SetString(PyExc_TypeError, "knn: non-image in known list");
      return 0;
    }
    if (cross_validation_mode && (cur == unknown))
      continue;
    double distance;
    if (compute_distance(o->distance_type, cur, unknown_buf, &distance, o->weight_vector, unknown_len) < 0) {
      PyErr_SetString(PyExc_ValueError,
                      "knn: error in distance calculation \
                       (This is most likely because features have not been generated.)");
      return 0;
    }

    char* id_name;
    int len;
    if (image_get_id_name(cur, &id_name, &len) < 0)
      return 0;
    knn.add(id_name, distance);
    Py_DECREF(cur);
  }

  knn.majority();
  if (do_confidence)
    knn.calculate_confidences();
  PyObject* ans_list = PyList_New(knn.answer.size());
  for (size_t i = 0; i < knn.answer.size(); ++i) {
    // PyList_SET_ITEM steal references so this code only looks
    // like it leaks. KWM
    PyObject* ans = PyTuple_New(2);
    PyTuple_SET_ITEM(ans, 0, PyFloat_FromDouble(knn.answer[i].second));
    PyTuple_SET_ITEM(ans, 1, PyString_FromString(knn.answer[i].first));
    PyList_SET_ITEM(ans_list, i, ans);
  }
  PyObject* conf_dict = PyDict_New();
  if (do_confidence) {
    for (size_t i = 0; i < knn.confidence_types.size(); ++i) {
      PyObject* o1 = PyInt_FromLong(knn.confidence_types[i]);
      PyObject* o2 = PyFloat_FromDouble(knn.confidence[i]);
      PyDict_SetItem(conf_dict, o1, o2);
      Py_DECREF(o1);
      Py_DECREF(o2);
    }
  }
  PyObject* result = PyTuple_New(2);
  PyTuple_SET_ITEM(result, 0, ans_list);
  PyTuple_SET_ITEM(result, 1, conf_dict);
  return result;
}

static PyObject* knn_distance_from_images(PyObject* self, PyObject* args) {
  KnnObject* o = (KnnObject*)self;
  if (o->ga_running == true) {
    PyErr_SetString(PyExc_TypeError, "knn: cannot call while ga is active.");
    return 0;
  }

  PyObject* unknown, *iterator;
  double maximum_distance = std::numeric_limits<double>::max();

  if (PyArg_ParseTuple(args, CHAR_PTR_CAST "OO|d", &iterator, &unknown, &maximum_distance) <= 0) {
    return 0;
  }

  if (!PyIter_Check(iterator)) {
    PyErr_SetString(PyExc_TypeError, "Known features must be iterable.");
    return 0;
  }

  if (!is_ImageObject(unknown)) {
    PyErr_SetString(PyExc_TypeError, "knn: unknown must be an image");
    return 0;
  }

  double* unknown_buf, *weights;
  Py_ssize_t unknown_len;
  if (image_get_fv(unknown, &unknown_buf, &unknown_len) < 0) {
      PyErr_SetString(PyExc_ValueError,
                      "knn: error getting feature vector \
                       (This is most likely because features have not been generated.)");
      return 0;
  }

  weights = o->weight_vector;

  PyObject* cur;
  PyObject* distance_list = PyList_New(0);
  PyObject* tmp_val;
  while ((cur = PyIter_Next(iterator))) {
    if (!is_ImageObject(cur)) {
      PyErr_SetString(PyExc_TypeError, "knn: non-image in known list");
      return 0;
    }
    double distance;
    if (compute_distance(o->distance_type, cur, unknown_buf, &distance, weights, unknown_len) < 0) {
      PyErr_SetString(PyExc_ValueError,
                      "knn: error in distance calculation \
                       (This is most likely because features have not been generated.)");
      return 0;
    }
    tmp_val = Py_BuildValue(CHAR_PTR_CAST "(fO)", distance, cur);
    if (distance < maximum_distance)
      if (PyList_Append(distance_list, tmp_val) < 0)
        return 0;
    Py_DECREF(tmp_val);
    Py_DECREF(cur);
  }

  return distance_list;
}

static PyObject* knn_distance_between_images(PyObject* self, PyObject* args) {
  KnnObject* o = (KnnObject*)self;
  PyObject* imagea, *imageb;
  PyArg_ParseTuple(args, CHAR_PTR_CAST "OO", &imagea, &imageb);

  if (!is_ImageObject(imagea)) {
    PyErr_SetString(PyExc_TypeError, "knn: unknown must be an image");
    return 0;
  }

  if (!is_ImageObject(imageb)) {
    PyErr_SetString(PyExc_TypeError, "knn: known must be an image");
    return 0;
  }

  double distance = 0.0;
  compute_distance(o->distance_type, imagea, imageb, &distance, o->weight_vector,
                   o->num_features);
  return Py_BuildValue(CHAR_PTR_CAST "f", distance);
}

/*
  Create a symmetric float matrix (image) containing all of the
  distances between the images in the list passed in. This is useful
  because it allows you to find the distance between any two pairs
  of images regardless of the order of the pairs. NOTE: the features
  are normalized before performing the distance calculations.
*/
PyObject* knn_distance_matrix(PyObject* self, PyObject* args) {
  KnnObject* o = (KnnObject*)self;
  PyObject* images;
  PyObject* progress = 0;
  long normalize = 1;
  if (PyArg_ParseTuple(args, CHAR_PTR_CAST "O|Oi", &images, &progress, &normalize) <= 0)
    return 0;
  // images is a list of Gamera/Python ImageObjects
  PyObject* images_seq = PySequence_Fast(images, "First argument must be iterable.");
  if (images_seq == NULL)
    return 0;

  int images_len = PySequence_Fast_GET_SIZE(images_seq);
  if (!(images_len > 1)) {
    PyErr_SetString(PyExc_ValueError, "List must have at least two images.");
    Py_DECREF(images_seq);
    return 0;
  }

  // Check the number of features
  double* buf_a, *buf_b;
  Py_ssize_t len_a, len_b;
  PyObject* cur_a, *cur_b;
  cur_a = PySequence_Fast_GET_ITEM(images_seq, 0);
  if (!is_ImageObject(cur_a)) {
    PyErr_SetString(PyExc_TypeError, "knn: expected an image");
    Py_DECREF(images_seq);
    return 0;
  }
  if (image_get_fv(cur_a, &buf_a, &len_a) < 0) {
    Py_DECREF(images_seq);
    return 0;
  }
  double* weights = o->weight_vector;
  if (len_a != (int)o->num_features) {
    PyErr_SetString(PyExc_ValueError, "knn: feature vector lengths don't match.");
    Py_DECREF(images_seq);
    return 0;
  }

  // create the normalization object
  double* tmp_a = new double[len_a];
  double* tmp_b = new double[len_a];
  FloatImageData* data = new FloatImageData(Dim(images_len, images_len));
  FloatImageView* mat = new FloatImageView(*data);

  kNN::Normalize norm(len_a);
  for (int i = 0; i < images_len; ++i) {
    cur_a = PySequence_Fast_GET_ITEM(images_seq, i);
    if (cur_a == NULL)
      goto mat_error;
    if (!is_ImageObject(cur_a)) {
      PyErr_SetString(PyExc_TypeError, "knn: expected an image");
      goto mat_error;
    }
    if (image_get_fv(cur_a, &buf_a, &len_a) < 0)
      goto mat_error;
    if (len_a != (int)o->num_features) {
      PyErr_SetString(PyExc_ValueError, "knn: feature vector lengths don't match.");
      goto mat_error;
    }
    if (normalize)
      norm.add(buf_a, buf_a + len_a);
  }
  if (normalize)
    norm.compute_normalization();

  std::fill(mat->vec_begin(), mat->vec_end(), 0.0);
  // do the distance calculations
  for (int i = 0; i < images_len; ++i) {
    cur_a = PySequence_Fast_GET_ITEM(images_seq, i);
    if (cur_a == NULL)
      goto mat_error;
    if (image_get_fv(cur_a, &buf_a, &len_a) < 0)
      goto mat_error;
    if (normalize)
      norm.apply(buf_a, buf_a + len_a, tmp_a);
    for (int j = i + 1; j < images_len; ++j) {
      cur_b = PySequence_Fast_GET_ITEM(images_seq, j);
      if (cur_b == NULL)
        goto mat_error;
      if (image_get_fv(cur_b, &buf_b, &len_b) < 0)
        goto mat_error;
      if (normalize)
        norm.apply(buf_b, buf_b + len_b, tmp_b);
      double distance;
      if (normalize)
        compute_distance(o->distance_type, tmp_a, len_a, tmp_b, &distance, weights);
      else
        compute_distance(o->distance_type, buf_a, len_a, buf_b, &distance, weights);
      mat->set(Point(j, i), distance);
      mat->set(Point(i, j), distance);
    }
    if (progress)
      PyObject_CallObject(progress, NULL);
  }
  delete[] tmp_a;
  delete[] tmp_b;
  Py_DECREF(images_seq);
  return create_ImageObject(mat);
 mat_error:
  Py_DECREF(images_seq);
  // delete the image
  delete mat; delete data;
  // delete the tmp buffers
  delete[] tmp_a; delete[] tmp_b;
  return 0;
}

/*
  unique_distances takes a list of images and returns all of the unique
  pairs of distances between the images.
*/
PyObject* knn_unique_distances(PyObject* self, PyObject* args) {
  KnnObject* o = (KnnObject*)self;
  PyObject* images;
  PyObject* progress;
  long normalize = 1;
  if (PyArg_ParseTuple(args, CHAR_PTR_CAST "OO|i", &images, &progress, &normalize) <= 0)
    return 0;
  // images is a list of Gamera/Python ImageObjects
  PyObject* images_seq = PySequence_Fast(images, "First argument must be iterable.");
  if (images_seq == NULL)
    return 0;

  int images_len = PySequence_Fast_GET_SIZE(images_seq);
  if (!(images_len > 1)) {
    PyErr_SetString(PyExc_ValueError, "List must have at least two images.");
    Py_DECREF(images_seq);
    return 0;
  }
  // create the 'vector' for the output
  int list_len = ((images_len * images_len) - images_len) / 2;
  FloatImageData* data = new FloatImageData(Dim(list_len, 1));
  FloatImageView* list = new FloatImageView(*data);

  // create a default set of weights for the distance calculation.
  double* buf_a, *buf_b;
  Py_ssize_t len_a, len_b;
  PyObject* cur_a, *cur_b;
  cur_a = PySequence_Fast_GET_ITEM(images_seq, 0);
  if (!is_ImageObject(cur_a)) {
    PyErr_SetString(PyExc_TypeError, "knn: expected an image");
    Py_DECREF(images_seq);
    return 0;
  }
  if (image_get_fv(cur_a, &buf_a, &len_a) < 0) {
    Py_DECREF(images_seq);
    return 0;
  }

  double* weights = o->weight_vector;
  if (len_a != (int)o->num_features) {
    PyErr_SetString(PyExc_ValueError, "knn: feature vector lengths don't match.");
    Py_DECREF(images_seq);
    return 0;
  }

  // create the normalization object
  kNN::Normalize norm(len_a);
  for (int i = 0; i < images_len; ++i) {
    cur_a = PySequence_Fast_GET_ITEM(images_seq, i);
    if (!is_ImageObject(cur_a)) {
      PyErr_SetString(PyExc_TypeError, "knn: expected an image");
      Py_DECREF(images_seq);
      return 0;
    }
    if (cur_a == NULL) {
      Py_DECREF(images_seq);
      return 0;
    }
    if (image_get_fv(cur_a, &buf_a, &len_a) < 0) {
      Py_DECREF(images_seq);
      return 0;
    }
    if (normalize)
      norm.add(buf_a, buf_a + len_a);
  }
  if (normalize)
    norm.compute_normalization();
  double* tmp_a = new double[len_a];
  double* tmp_b = new double[len_a];
  // do the distance calculations
  size_t index = 0;
  for (int i = 0; i < images_len; ++i) {
    cur_a = PySequence_Fast_GET_ITEM(images_seq, i);
    if (cur_a == NULL)
      goto uniq_error;
    if (image_get_fv(cur_a, &buf_a, &len_a) < 0)
      goto uniq_error;
    if (normalize)
      norm.apply(buf_a, buf_a + len_a, tmp_a);
    for (int j = i + 1; j < images_len; ++j) {
      cur_b = PySequence_Fast_GET_ITEM(images_seq, j);
      if (cur_b == NULL)
        goto uniq_error;
      if (image_get_fv(cur_b, &buf_b, &len_b) < 0)
        goto uniq_error;

      if (len_a != len_b) {
        PyErr_SetString(PyExc_ValueError, "Feature vector lengths do not match!");
        goto uniq_error;
      }
      if (normalize)
        norm.apply(buf_b, buf_b + len_b, tmp_b);
      double distance;
      if (normalize)
        compute_distance(o->distance_type, tmp_a, len_a, tmp_b, &distance, weights);
      else
        compute_distance(o->distance_type, buf_a, len_a, buf_b, &distance, weights);
      list->set(Point(index, 0), distance);
      index++;
    }
    // call the progress object
    PyObject_CallObject(progress, NULL);
  }

  delete[] tmp_a; delete[] tmp_b;
  return create_ImageObject(list);
  // in case of error
 uniq_error:
  delete[] tmp_a; delete[] tmp_b;
  delete list;
  delete data;
  return 0;
}

static PyObject* knn_get_num_k(PyObject* self) {
  return Py_BuildValue(CHAR_PTR_CAST "i", ((KnnObject*)self)->num_k);
}

static int knn_set_num_k(PyObject* self, PyObject* v) {
  if (!PyInt_Check(v)) {
    PyErr_SetString(PyExc_TypeError, "knn: expected an int.");
    return -1;
  }
  ((KnnObject*)self)->num_k = PyInt_AS_LONG(v);
  return 0;
}

static PyObject* knn_get_distance_type(PyObject* self) {
  return Py_BuildValue(CHAR_PTR_CAST "i", ((KnnObject*)self)->distance_type);
}

static int knn_set_distance_type(PyObject* self, PyObject* v) {
  if (!PyInt_Check(v)) {
    PyErr_SetString(PyExc_TypeError, "knn: expected an int.");
    return -1;
  }
  ((KnnObject*)self)->distance_type = (DistanceType)PyInt_AS_LONG(v);
  return 0;
}

static PyObject* knn_get_confidence_types(PyObject* self) {
  size_t n,i;
  PyObject* entry;
  KnnObject* o = ((KnnObject*)self);
  n = o->confidence_types.size();
  PyObject* result = PyList_New(n);
  for (i=0; i<n; i++) {
    entry = PyInt_FromLong(o->confidence_types[i]);
    PyList_SetItem(result, i, entry);
  }
  return result;
}

static int knn_set_confidence_types(PyObject* self, PyObject* list) {
  if(!PyList_Check(list)) {
    PyErr_SetString(PyExc_TypeError, "knn: confidence_types must be list.");
    return -1;
  }
  size_t n,i;
  int ct;
  PyObject* entry;
  KnnObject* o = ((KnnObject*)self);
  o->confidence_types.clear();
  n = PyList_Size(list);
  for (i=0; i<n; i++) {
    entry = PyList_GetItem(list, i);
    if (!PyInt_Check(entry)) {
      PyErr_SetString(PyExc_TypeError, "knn: each confidence_type must be int.");
      return -1;
    }
    ct = (ConfidenceTypes)PyInt_AsLong(entry);
    o->confidence_types.push_back(ct);
  }
  return 0;
}

static std::pair<int,int> leave_one_out(KnnObject* o, int stop_threshold,
                                        double* weight_vector = 0,
                                        std::vector<long>* indexes = 0) {
  double* weights = weight_vector;
  if (weights == 0)
    weights = o->weight_vector;

  assert(o->feature_vectors != 0);
  kNearestNeighbors<char*, ltstr, eqstr> knn(o->num_k);

  int total_correct = 0;
  int total_queries = 0;
  if (indexes == 0) {
    for (size_t i = 0; i < o->num_feature_vectors; ++i) {
      // We don't want to do the calculation if there is no
      // hope that kNN will return the correct answer (because
      // there aren't enough examples in the database).
      if (o->id_name_histogram[i] < int((o->num_k + 0.5) / 2)) {
        continue;
      }
      double* current_known = o->feature_vectors;
      double* unknown = &o->feature_vectors[i * o->num_features];
      for (size_t j = 0; j < o->num_feature_vectors; ++j, current_known += o->num_features) {
        if (i == j)
          continue;
        double distance;
        if (o->distance_type == CITY_BLOCK) {
          distance = city_block_distance(current_known, current_known + o->num_features,
                                         unknown, weights);
        } else if (o->distance_type == FAST_EUCLIDEAN) {
          distance = fast_euclidean_distance(current_known, current_known + o->num_features,
                                             unknown, weights);
        } else {
          distance = euclidean_distance(current_known, current_known + o->num_features,
                                        unknown, weights);
        }
        knn.add(o->id_names[j], distance);
      }
      knn.majority();
      if (strcmp(knn.answer[0].first, o->id_names[i]) == 0) {
        total_correct++;
      }
      knn.reset();
      total_queries++;
      if (total_queries - total_correct > stop_threshold)
        return std::make_pair(total_correct, total_queries);
    }
  } else {
    for (size_t i = 0; i < o->num_feature_vectors; ++i) {
      if (o->id_name_histogram[i] < int((o->num_k + 0.5) / 2))
        continue;
      double* current_known = o->feature_vectors;
      double* unknown = &o->feature_vectors[i * o->num_features];
      for (size_t j = 0; j < o->num_feature_vectors; ++j, current_known += o->num_features) {
        if (i == j)
          continue;
        double distance;
        if (o->distance_type == CITY_BLOCK) {
          distance = city_block_distance_skip(current_known, unknown, weights,
                                              indexes->begin(), indexes->end());
        } else if (o->distance_type == FAST_EUCLIDEAN) {
          distance = fast_euclidean_distance_skip(current_known, unknown, weights,
                                                  indexes->begin(), indexes->end());
        } else {
          distance = euclidean_distance_skip(current_known, unknown, weights,
                                             indexes->begin(), indexes->end());
        }

        knn.add(o->id_names[j], distance);
      }
      knn.majority();
      if (strcmp(knn.answer[0].first, o->id_names[i]) == 0) {
        total_correct++;
      }
      knn.reset();
      total_queries++;
      if (total_queries - total_correct > stop_threshold)
        return std::make_pair(total_correct, total_queries);
    }
  }
  return std::make_pair(total_correct, total_queries);
}

/*
  Leave-one-out cross validation
*/
static PyObject* knn_leave_one_out(PyObject* self, PyObject* args) {
  KnnObject* o = (KnnObject*)self;
  PyObject* indexes = 0;
  int stop_threshold = std::numeric_limits<int>::max();
  if (PyArg_ParseTuple(args, CHAR_PTR_CAST "|Oi", &indexes, &stop_threshold) <= 0)
    return 0;
  if (o->feature_vectors == 0) {
    PyErr_SetString(PyExc_RuntimeError,
                    "knn: leave_one_out called before instantiate_from_images.");
    return 0;
  }
  if (indexes == 0) {
    // If we don't have a list of indexes, just do the leave_one_out
    std::pair<int, int> ans = leave_one_out(o, std::numeric_limits<int>::max());
    return Py_BuildValue(CHAR_PTR_CAST "(ii)", ans.first, ans.second);
  } else {
    // Get the list of indexes
    PyObject* indexes_seq = PySequence_Fast(indexes, "Indexes must be an iterable list of indexes.");
    if (indexes_seq == NULL)
      return 0;

    int indexes_size = PySequence_Fast_GET_SIZE(indexes_seq);
    // Make certain that there aren't too many indexes
    if (indexes_size > (int)o->num_features) {
      PyErr_SetString(PyExc_ValueError, "knn: index list too large for data");
      Py_DECREF(indexes_seq);
      return 0;
    }
    // copy the indexes into a vector
    std::vector<long> idx(indexes_size);
    for (int i = 0; i < indexes_size; ++i) {
      PyObject* tmp = PySequence_Fast_GET_ITEM(indexes_seq, i);
      if (!PyInt_Check(tmp)) {
        PyErr_SetString(PyExc_TypeError, "knn: expected indexes to be ints");
        Py_DECREF(indexes_seq);
        return 0;
      }
      idx[i] = PyInt_AS_LONG(tmp);
    }
    // make certain that none of the indexes are out of range
    for (size_t i = 0; i < idx.size(); ++i) {
      if (idx[i] > (long)(o->num_features - 1)) {
        PyErr_SetString(PyExc_IndexError, "knn: index out of range in index list");
        Py_DECREF(indexes_seq);
        return 0;
      }
    }
    // do the leave-one-out
    std::pair<int, int> ans = leave_one_out(o, stop_threshold, o->weight_vector, &idx);
    return Py_BuildValue(CHAR_PTR_CAST "(ii)", ans.first, ans.second);
  }
}

/*
  statistics of average distance to k nearest neighbors
*/
static PyObject* knn_knndistance_statistics(PyObject* self, PyObject* args) {
  KnnObject* o = (KnnObject*)self;
  PyObject* progress = 0;
  int k;
  size_t i,j;
  if (PyArg_ParseTuple(args, CHAR_PTR_CAST "|iO", &k, &progress) <= 0)
    return 0;
  if (o->feature_vectors == 0) {
    PyErr_SetString(PyExc_RuntimeError,
                    "knn: knndistance_statistics called before instantiate_from_images.");
    return 0;
  }
  if (k <= 0) {
    k = o->num_k;
  }
  if (k > (int)o->num_feature_vectors - 1) {
    PyErr_SetString(PyExc_RuntimeError,
                    "knn: knndistance_statistics requires more than k training samples.");
    return 0;
  }
  PyObject* entry;
  PyObject* result = PyList_New(o->num_feature_vectors);
  double *feature_i, *feature_j;
  double distance;
  kNearestNeighbors<char*, ltstr, eqstr> knn((size_t)k);
  for (i=0; i<o->num_feature_vectors; i++) {
    knn.reset();
    // find k nearest neighbors of i-th prototype
    feature_i = &o->feature_vectors[i * o->num_features];
    for (j=0; j<o->num_feature_vectors; j++) {
      if (j==i) continue;
      feature_j = &o->feature_vectors[j * o->num_features];
      // compute distance
      if (o->distance_type == CITY_BLOCK) {
        distance = city_block_distance(feature_i, feature_i + o->num_features,
                                       feature_j, o->weight_vector);
      } else if (o->distance_type == FAST_EUCLIDEAN) {
        distance = fast_euclidean_distance(feature_i, feature_i + o->num_features,
                                           feature_j, o->weight_vector);
      } else {
        distance = euclidean_distance(feature_i, feature_i + o->num_features,
                                      feature_j, o->weight_vector);
      }
      // store distance in kNearestNeighbors
      knn.add(o->id_names[j], distance);
    }
    // compute average distance
    distance = 0.0;
    for (j=0; j < knn.m_nn.size(); ++j) {
      distance += knn.m_nn[j].distance;
    }
    distance = distance / k;
    entry = PyTuple_New(2);
    PyTuple_SET_ITEM(entry, 0, PyFloat_FromDouble(distance));
    PyTuple_SET_ITEM(entry, 1, PyString_FromString(o->id_names[i]));
    PyList_SetItem(result, i, entry);
    if (progress)
      PyObject_CallObject(progress, NULL);
  }
  return result;
}

/*
  Serialize and unserialize save and restore the internal data of the kNN object
  to/from a fast and compact binary format. This allows a user to create a file that
  can be used to create non-interactive classifiers in a very fast way.

  ARGUMENTS

  This function takes a filename and a list of features - the python wrapper of this
  class handles providing the list of features.

  FORMAT

  The format is designed to be as simple as possible. First is a header consisting
  of the file format version (currently 1), then the size and settings of the data,
  and finally the data.

  HEADER

  size             what
  ------------------------------------------
  unsigned long    version
  unsigned long    number of k
  unsigned long    number of feaures
  unsigned long    number of feature vectors
  unsigned long    number of feature names
  na               list of feature names in the format
                   of unsigned long (length - including null)
                   and char[]

  DATA

  The data as stored a list of id_names followed by the feature vectors.
  The id_names are stored as:

  size             what
  ------------------------------------------
  unsigned long    length of string
  char[]           id_name

  There are, of course, num_feature_vectors id_names. Next is the data which is
  simply written directly - i.e. num_feature_vectors arrays of doubles of length
  num_features.

*/
static PyObject* knn_serialize(PyObject* self, PyObject* args) {
  KnnObject* o = (KnnObject*)self;
  char* filename;
  PyObject* features;
  if (PyArg_ParseTuple(args, CHAR_PTR_CAST "sO", &filename, &features) <= 0) {
    return 0;
  }

  // type check the features
  if (!PyList_Check(features)) {
    PyErr_SetString(PyExc_TypeError, "knn: list of features must be a list.");
    return 0;
  }

  unsigned long feature_size = PyList_GET_SIZE(features);

  FILE* file = fopen(filename, "w+b");
  if (file == 0) {
    PyErr_SetString(PyExc_IOError, "knn: error opening file.");
    return 0;
  }

  if (o->feature_vectors == 0) {
    PyErr_SetString(PyExc_RuntimeError, "knn: serialize called before instatiate from images.");
    fclose(file);
    return 0;
  }

  // write the header info
  unsigned long version = 1;
  if (fwrite((const void*)&version, sizeof(unsigned long), 1, file) != 1) {
    PyErr_SetString(PyExc_IOError, "knn: problem writing to a file.");
    fclose(file);
    return 0;
  }
  unsigned long num_k = (unsigned long)o->num_k;
  if (fwrite((const void*)&num_k, sizeof(unsigned long), 1, file) != 1) {
    PyErr_SetString(PyExc_IOError, "knn: problem writing to a file.");
    fclose(file);
    return 0;
  }
  unsigned long num_features = (unsigned long)o->num_features;
  if (fwrite((const void*)&num_features, sizeof(unsigned long), 1, file) != 1) {
    PyErr_SetString(PyExc_IOError, "knn: problem writing to a file.");
    fclose(file);
    return 0;
  }
  unsigned long num_feature_vectors = (unsigned long)o->num_feature_vectors;
  if (fwrite((const void*)&num_feature_vectors, sizeof(unsigned long), 1, file) != 1) {
    PyErr_SetString(PyExc_IOError, "knn: problem writing to a file.");
    fclose(file);
    return 0;
  }

  // write the feature names
  if (fwrite((const void*)&feature_size, sizeof(unsigned long), 1, file) != 1) {
    PyErr_SetString(PyExc_IOError, "knn: problem writing to a file.");
    fclose(file);
    return 0;
  }

  for (size_t i = 0; i < feature_size; ++i) {
    PyObject* cur_string = PyList_GET_ITEM(features, i);
    unsigned long string_size = PyString_GET_SIZE(cur_string) + 1;
    if (fwrite((const void*)&string_size, sizeof(unsigned long), 1, file) != 1) {
      PyErr_SetString(PyExc_IOError, "knn: problem writing to a file.");
      fclose(file);
      return 0;
    }
    if (fwrite((const void*)PyString_AS_STRING(cur_string),
               sizeof(char), string_size, file) != string_size) {
      PyErr_SetString(PyExc_IOError, "knn: problem writing to a file.");
      fclose(file);
      return 0;
    }
  }

  for (size_t i = 0; i < o->num_feature_vectors; ++i) {
    unsigned long len = strlen(o->id_names[i]) + 1; // include \0
    if (fwrite((const void*)&len, sizeof(unsigned long), 1, file) != 1) {
      PyErr_SetString(PyExc_IOError, "knn: problem writing to a file.");
      fclose(file);
      return 0;
    }
    if (fwrite((const void*)o->id_names[i], sizeof(char), len, file) != len) {
      PyErr_SetString(PyExc_IOError, "knn: problem writing to a file.");
      fclose(file);
      return 0;
    }
  }

  if (fwrite((const void*)o->normalize->get_norm_vector(),
             sizeof(double), o->num_features, file) != o->num_features) {
    PyErr_SetString(PyExc_IOError, "knn: problem writing to a file.");
    fclose(file);
    return 0;
  }

  if (fwrite((const void*)o->weight_vector, sizeof(double), o->num_features, file)
      != o->num_features) {
    PyErr_SetString(PyExc_IOError, "knn: problem writing to a file.");
    fclose(file);
    return 0;
  }

  // write the data
  double* cur = o->feature_vectors;
  for (size_t i = 0; i < o->num_feature_vectors; ++i, cur += o->num_features) {
    if (fwrite((const void*)cur, sizeof(double), o->num_features, file)
        != o->num_features) {
      PyErr_SetString(PyExc_IOError, "knn: problem writing to a file.");
      fclose(file);
      return 0;
    }
  }
  fclose(file);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject* knn_unserialize(PyObject* self, PyObject* args) {
  KnnObject* o = (KnnObject*)self;
  char* filename;
  if (PyArg_ParseTuple(args, CHAR_PTR_CAST "s", &filename) <= 0)
    return 0;

  FILE* file = fopen(filename, "rb");
  if (file == 0) {
    PyErr_SetString(PyExc_IOError, "knn: error opening file.");
    return 0;
  }

  unsigned long version, num_k, num_features, num_feature_vectors, num_feature_names;
  if (fread((void*)&version, sizeof(unsigned long), 1, file) != 1) {
    PyErr_SetString(PyExc_IOError, "knn: problem reading file.");
    fclose(file);
    return 0;
  }
  if (version != 1) {
    PyErr_SetString(PyExc_IOError, "knn: unknown version of knn file.");
    fclose(file);
    return 0;
  }
  if (fread((void*)&num_k, sizeof(unsigned long), 1, file) != 1) {
    PyErr_SetString(PyExc_IOError, "knn: problem reading file.");
    fclose(file);
    return 0;
  }
  if (fread((void*)&num_features, sizeof(unsigned long), 1, file) != 1) {
    PyErr_SetString(PyExc_IOError, "knn: problem reading file.");
    fclose(file);
    return 0;
  }
  if (fread((void*)&num_feature_vectors, sizeof(unsigned long), 1, file) != 1) {
    PyErr_SetString(PyExc_IOError, "knn: problem reading file.");
    fclose(file);
    return 0;
  }
  if (fread((void*)&num_feature_names, sizeof(unsigned long), 1, file) != 1) {
    PyErr_SetString(PyExc_IOError, "knn: problem reading file.");
    fclose(file);
    return 0;
  }
  PyObject* feature_names = PyList_New(num_feature_names);
  for (size_t i = 0; i < num_feature_names; ++i) {
    unsigned long string_size;
    if (fread((void*)&string_size, sizeof(unsigned long), 1, file) != 1) {
      PyErr_SetString(PyExc_RuntimeError, "knn: problem reading file.");
      fclose(file);
      return 0;
    }
    char tmp_string[1024];
    if (fread((void*)&tmp_string, sizeof(char), string_size, file) != string_size) {
      PyErr_SetString(PyExc_IOError, "knn: problem reading file.");
      fclose(file);
      return 0;
    }
    PyList_SET_ITEM(feature_names, i,
                    PyString_FromStringAndSize((const char*)&tmp_string, string_size - 1));
  }

  knn_delete_feature_data(o);
  set_num_features(o, (size_t)num_features);
  if (knn_create_feature_data(o, (size_t)num_feature_vectors) < 0) {
    fclose(file);
    return 0;
  }
  o->num_k = num_k;

  std::map<char*, int, ltstr> id_name_histogram;
  for (size_t i = 0; i < o->num_feature_vectors; ++i) {
    unsigned long len;
    if (fread((void*)&len, sizeof(unsigned long), 1, file) != 1) {
      PyErr_SetString(PyExc_IOError, "knn: problem reading file.");
      fclose(file);
      return 0;
    }
    o->id_names[i] = new char[len];
    if (fread((void*)o->id_names[i], sizeof(char), len, file) != len) {
      PyErr_SetString(PyExc_IOError, "knn: problem reading file.");
      fclose(file);
      return 0;
    }
    id_name_histogram[o->id_names[i]]++;
  }

  double* tmp_norm = new double[o->num_features];
  if (fread((void*)tmp_norm, sizeof(double), o->num_features, file) != o->num_features) {
    PyErr_SetString(PyExc_IOError, "knn: problem reading file.");
    delete[] tmp_norm;
    fclose(file);
    return 0;
  }
  o->normalize->set_norm_vector(tmp_norm, tmp_norm + o->num_features);
  delete[] tmp_norm;
  if (fread((void*)o->weight_vector, sizeof(double), o->num_features, file) != o->num_features) {
    PyErr_SetString(PyExc_IOError, "knn: problem reading file.");
    fclose(file);
    return 0;
  }

  double* cur = o->feature_vectors;
  for (size_t i = 0; i < o->num_feature_vectors; ++i, cur += o->num_features) {
    if (fread((void*)cur, sizeof(double), o->num_features, file) != o->num_features) {
      PyErr_SetString(PyExc_IOError, "knn: problem reading file.");
      fclose(file);
      return 0;
    }
    o->id_name_histogram[i] = id_name_histogram[o->id_names[i]];
  }

  fclose(file);
  return feature_names;
}

static PyObject* knn_get_weights(PyObject* self, PyObject* args) {
  KnnObject* o = (KnnObject*)self;
  PyObject* arglist = Py_BuildValue(CHAR_PTR_CAST "(s)", "d");
  PyObject* array = PyEval_CallObject(array_init, arglist);
  if (array == 0) {
    PyErr_SetString(PyExc_IOError, "knn: Error creating array.");
    return 0;
  }
  Py_DECREF(arglist);
  PyObject* result;
  for (size_t i = 0; i < o->num_features; ++i) {
    result = PyObject_CallMethod(array, (char *)"append", (char *)"f", o->weight_vector[i]);
    if (result == 0)
      return 0;
    Py_DECREF(result);
  }
  Py_DECREF(arglist);
  return array;
}

static PyObject* knn_set_weights(PyObject* self, PyObject* args) {
  KnnObject* o = (KnnObject*)self;
  PyObject* array;
  if (PyArg_ParseTuple(args, CHAR_PTR_CAST "O", &array) <= 0) {
    return 0;
  }
  Py_ssize_t len;
  double* weights;
  if (!PyObject_CheckReadBuffer(array)) {
    PyErr_SetString(PyExc_RuntimeError, "knn: Error getting weight array buffer.");
    return 0;
  }
  if ((PyObject_AsReadBuffer(array, (const void**)&weights, &len) != 0)) {
    PyErr_SetString(PyExc_RuntimeError, "knn: Error getting weight array buffer.");
    return 0;
  }
  if (size_t(len) != o->num_features * sizeof(double)) {
    PyErr_SetString(PyExc_ValueError, "knn: weight vector is not the correct size.");
    return 0;
  }
  for (size_t i = 0; i < o->num_features; ++i) {
    o->weight_vector[i] = weights[i];
  }
  Py_INCREF(Py_None);
  return Py_None;
}

/*
  GA
*/
float Fitness(GAGenome & g) {
  GA1DArrayGenome<double> & genome = (GA1DArrayGenome<double> &)g;
  KnnObject* knn = (KnnObject*)genome.userData();

  std::pair<int,int> ans = leave_one_out(knn, std::numeric_limits<int>::max(), genome());

  return (float)ans.first / ans.second;
}

void Initializer(GAGenome& genome) {
  GA1DArrayGenome<double>& g = (GA1DArrayGenome<double>&)genome;
  for (int i = 0; i < g.length(); i++) {
    g.gene(i, rand() / (RAND_MAX + 1.0));
  }
}

static PyObject* knn_ga_create(PyObject* self, PyObject* args) {
  KnnObject* o = (KnnObject*)self;
  o->ga_running = true;
  srand(time(0));

  Py_BEGIN_ALLOW_THREADS
  if (o->ga != 0)
    delete o->ga;
  if (o->genome != 0)
    delete o->genome;
  o->genome = new GA1DArrayGenome<double>(o->num_features, Fitness);
  o->genome->userData(o);
  o->genome->initializer(Initializer);

  GARandomSeed();
  o->ga = new GASteadyStateGA(*o->genome);
  o->ga->populationSize(o->ga_population);
  o->ga->nGenerations(1);
  o->ga->pMutation(o->ga_mutation);
  o->ga->pCrossover(o->ga_crossover);
  o->ga->initialize();
  Py_END_ALLOW_THREADS
  return Py_BuildValue(CHAR_PTR_CAST "f", o->ga->statistics().initial());
}

static PyObject* knn_ga_destroy(PyObject* self, PyObject* args) {
  KnnObject* o = (KnnObject*)self;
  if (o->ga != 0) {
    delete o->ga;
    o->ga = 0;
  }
  if (o->genome != 0) {
    delete o->genome;
    o->genome = 0;
  }
  o->ga_running = false;
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject* knn_ga_step(PyObject* self, PyObject* args) {
  KnnObject* o = (KnnObject*)self;
  Py_BEGIN_ALLOW_THREADS
  o->ga->populationSize(o->ga_population);
  o->ga->pMutation(o->ga_mutation);
  o->ga->pCrossover(o->ga_crossover);
  o->ga->step();
  Py_END_ALLOW_THREADS
  GA1DArrayGenome<double>& g =(GA1DArrayGenome<double>&)o->ga->statistics().bestIndividual();
  for (size_t i = 0; i < o->num_features; i++)
    o->weight_vector[i] = g.gene(i);
  return Py_BuildValue(CHAR_PTR_CAST "f", o->ga->statistics().maxEver());
}

static PyObject* knn_get_ga_mutation(PyObject* self) {
  KnnObject* o = (KnnObject*)self;
  return Py_BuildValue(CHAR_PTR_CAST "f", o->ga_mutation);
}

static int knn_set_ga_mutation(PyObject* self, PyObject* v) {
  KnnObject* o = (KnnObject*)self;
  if (!PyFloat_Check(v)) {
    PyErr_SetString(PyExc_TypeError, "knn: must be a floating-point number.");
    return -1;
  }
  o->ga_mutation = PyFloat_AS_DOUBLE(v);
  return 0;
}

static PyObject* knn_get_ga_crossover(PyObject* self) {
  KnnObject* o = (KnnObject*)self;
  return Py_BuildValue(CHAR_PTR_CAST "f", o->ga_crossover);
}

static int knn_set_ga_crossover(PyObject* self, PyObject* v) {
  KnnObject* o = (KnnObject*)self;
  if (!PyFloat_Check(v)) {
    PyErr_SetString(PyExc_TypeError, "knn: must be a floating-point number.");
    return -1;
  }
  o->ga_crossover = PyFloat_AS_DOUBLE(v);
  return 0;
}

static PyObject* knn_get_ga_population(PyObject* self) {
  KnnObject* o = (KnnObject*)self;
  return Py_BuildValue(CHAR_PTR_CAST "i", o->ga_population);
}

static int knn_set_ga_population(PyObject* self, PyObject* v) {
  KnnObject* o = (KnnObject*)self;
  if (!PyInt_Check(v)) {
    PyErr_SetString(PyExc_TypeError, "knn: must be a floating-point number.");
    return -1;
  }
  o->ga_population = size_t(PyInt_AS_LONG(v));
  return 0;
}

static PyObject* knn_get_num_features(PyObject* self) {
  KnnObject* o = (KnnObject*)self;
  return Py_BuildValue(CHAR_PTR_CAST "i", o->num_features);
}

static int knn_set_num_features(PyObject* self, PyObject* v) {
  KnnObject* o = (KnnObject*)self;
  if (!PyInt_Check(v)) {
    PyErr_SetString(PyExc_TypeError, "knn: must be an integer.");
    return -1;
  }
  set_num_features(o, PyInt_AS_LONG(v));
  return 0;
}

PyMethodDef knn_module_methods[] = {
  { NULL }
};

DL_EXPORT(void) initknncore(void) {
  PyObject* m = Py_InitModule(CHAR_PTR_CAST "gamera.knncore", knn_module_methods);
  PyObject* d = PyModule_GetDict(m);

  KnnType.ob_type = &PyType_Type;
  KnnType.tp_name = CHAR_PTR_CAST "gamera.knncore.kNN";
  KnnType.tp_basicsize = sizeof(KnnObject);
  KnnType.tp_dealloc = knn_dealloc;
  KnnType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  KnnType.tp_new = knn_new;
  KnnType.tp_getattro = PyObject_GenericGetAttr;
  KnnType.tp_alloc = NULL; // PyType_GenericAlloc;
  KnnType.tp_free = NULL; // _PyObject_Del;
  KnnType.tp_methods = knn_methods;
  KnnType.tp_getset = knn_getset;
  PyType_Ready(&KnnType);
  PyDict_SetItemString(d, "kNN", (PyObject*)&KnnType);
  PyDict_SetItemString(d, "CITY_BLOCK",
                       Py_BuildValue(CHAR_PTR_CAST "i", CITY_BLOCK));
  PyDict_SetItemString(d, "EUCLIDEAN",
                       Py_BuildValue(CHAR_PTR_CAST "i", EUCLIDEAN));
  PyDict_SetItemString(d, "FAST_EUCLIDEAN",
                       Py_BuildValue(CHAR_PTR_CAST "i", FAST_EUCLIDEAN));

  PyObject* array_dict = get_module_dict("array");
  if (array_dict == 0) {
    return;
  }
  array_init = PyDict_GetItemString(array_dict, "array");
  if (array_init == 0) {
    PyErr_SetString(PyExc_RuntimeError, "Unable to get array init method\n");
    return;
  }
}
