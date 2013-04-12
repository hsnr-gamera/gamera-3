/*
 * Copyright (C) 2001-2009 Ichiro Fujinaga, Michael Droettboom,
 *                         Karl MacMillan, and Christoph Dalitz
 *               2012      David Kolanus, Tobias Bolten
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

#ifndef KnnObject201206
#define KnnObject201206

#include <Python.h>
#include <vector>
#include "gameramodule.hpp"
#include "knn.hpp"
#include "knnmodule.hpp"

namespace Gamera { namespace kNN {
#if 0
  static PyTypeObject KnnType = {
    PyObject_HEAD_INIT(NULL)
    0,
  };
#endif
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

    /*
      The feature vectors.
      A vector to double arrays of size num_features is used to store the
      feature vectors. It is only used for non-interactive classification).
    */
    std::vector<double*> *feature_vectors;

    // The id_names for the feature vectors
    char** id_names;
    // confidence types to be computed during classification
    std::vector<int> confidence_types;
    // The current selected features
    int *selection_vector;
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
      Temporary storage for the unknown feature vector. This is simply to avoid
      allocating memory for each call to classify (and could potentially
      increase our cache hit rate, but who really knows).
    */
    double* unknown;
    // k - this is k-NN after all
    size_t num_k;
    // the distance type currently being used.
    DistanceType distance_type;
  };

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

  static std::pair<int,int> leave_one_out(KnnObject* o, int stop_threshold,
                                          int* selection_vector = 0,
                                          double* weight_vector = 0,
                                          std::vector<long>* indexes = 0) {
    int* selections = selection_vector;
    if (selections == 0) {
      selections = o->selection_vector;
    }

    double* weights = weight_vector;
    if (weights == 0) {
      weights = o->weight_vector;
    }

    assert(o->feature_vectors != 0);
    kNearestNeighbors<char*, ltstr, eqstr> knn(o->num_k);

    int total_correct = 0;
    int total_queries = 0;
    if (indexes == 0) {
      for (size_t i = 0; i < o->feature_vectors->size(); ++i/*, cur += o->num_features*/) {
        // We don't want to do the calculation if there is no
        // hope that kNN will return the correct answer (because
        // there aren't enough examples in the database).
        if (o->id_name_histogram[i] < int((o->num_k + 0.5) / 2)) {
          continue;
        }
        double* current_known;
        double* unknown = (*o->feature_vectors)[i];
        for (size_t j = 0; j < o->feature_vectors->size(); ++j) {
          current_known = (*o->feature_vectors)[j];
          if (i == j)
            continue;
          double distance;
          compute_distance(o->distance_type, current_known, o->num_features,
                           unknown, &distance, selections, weights);
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
      for (size_t i = 0; i < o->feature_vectors->size(); ++i) {
        if (o->id_name_histogram[i] < int((o->num_k + 0.5) / 2))
          continue;
        double* current_known;
        double* unknown = (*o->feature_vectors)[i];
        for (size_t j = 0; j < o->feature_vectors->size(); ++j) {
          current_known = (*o->feature_vectors)[j];
          if (i == j)
            continue;
          double distance;
          if (o->distance_type == CITY_BLOCK) {
            distance = city_block_distance_skip(current_known, unknown, selections, weights,
                                                indexes->begin(), indexes->end());
          } else if (o->distance_type == FAST_EUCLIDEAN) {
            distance = fast_euclidean_distance_skip(current_known, unknown, selections, weights,
                                                    indexes->begin(), indexes->end());
          } else {
            distance = euclidean_distance_skip(current_known, unknown, selections, weights,
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

}} // end of namespaces

#endif
