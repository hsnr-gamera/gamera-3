/*
 *
 * Copyright (C) 2001 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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

#ifndef kwm08142002_knn
#define kwm08142002_knn

#include "gamera_limits.hpp"
#include <vector>
#include <map>
#include <cmath>
#include <algorithm>
#include <exception>
#include <stdexcept>
#include <cassert>

namespace Gamera {
  namespace kNN {
    /*
      DISTANCE FUNCTIONS
    */

    /*
      Compute the weighted distance between a known feature
      and an unknown feature using the city block method.

      IterA: iterator type for the known feature vector
      IterB: iterator type for the unknown feature vector
      IterC: iterator type for the weighting vector
    */
    template<class IterA, class IterB, class IterC>
    inline double city_block_distance(IterA known, const IterA end,
				      IterB unknown, IterC weight, double stop_dist = -1.0) {
      double distance = 0;
      if (stop_dist > 0.0) {
	for (; known != end; ++known, ++unknown, ++weight) {
	  distance += *weight * std::abs(*unknown - *known);
	  if (distance > stop_dist)
	    return std::numeric_limits<double>::max();
	}
      } else {
	for (; known != end; ++known, ++unknown, ++weight)
	  distance += *weight * std::abs(*unknown - *known);
      }
      return distance;
    }


    /*
      Compute the weighted distance between a known feature
      and an unknown feature using the euclidean method.

      IterA: iterator type for the known feature vector
      IterB: iterator type for the unknown feature vector
      IterC: iterator type for the weighting vector
    */
    template<class IterA, class IterB, class IterC>
    inline double euclidean_distance(IterA known, const IterA end,
				     IterB unknown, IterC weight, double stop_dist = -1.0) {
      double distance = 0;
      if (stop_dist > 0.0) {
	for (; known != end; ++known, ++unknown, ++weight) {
	  distance += *weight * std::sqrt((*unknown - *known) * (*unknown - *known));
	  if (distance > stop_dist)
	    return std::numeric_limits<double>::max();
	}
      } else {
	for (; known != end; ++known, ++unknown, ++weight)
	  distance += *weight * std::sqrt((*unknown - *known) * (*unknown - *known));
      }
      return distance;
    }


    /*
      Compute the weighted distance between a known feature
      and an unknown feature using the fast euclidean method.

      IterA: iterator type for the known feature vector
      IterB: iterator type for the unknown feature vector
      IterC: iterator type for the weighting vector
    */
    template<class IterA, class IterB, class IterC>
    inline double fast_euclidean_distance(IterA known, const IterA end,
					  IterB unknown, IterC weight, double stop_dist = -1.0) {
      double distance = 0;
      if (stop_dist > 0.0) {
	for (; known != end; ++known, ++unknown, ++weight) {
	  distance += *weight * ((*unknown - *known) * (*unknown - *known));
	  if (distance > stop_dist)
	    return std::numeric_limits<double>::max();
	}
      } else {
	for (; known != end; ++known, ++unknown, ++weight)
	  distance += *weight * ((*unknown - *known) * (*unknown - *known));
      }
      return distance;
    }

    /*
      NORMALIZE
      
      Normalize is used to compute normalization of the feature vectors in a database
      of known feature vectors and then to apply that normalization to feature
      vectors. It only works with doubles.

      Like the kNearestNeighbors class below, Normalize avoids knowing
      anything about the data structures used for storing the feature
      vectors. The add method is called for each feature vector,
      compute_normalization is called, and then feature vectors can
      be normalized by calling apply.
    */
    class Normalize {
    public:
      Normalize(size_t num_features) {
	m_num_features = num_features;
	m_num_feature_vectors = 0;
	m_norm_vector = new double[m_num_features];
	std::fill(m_norm_vector, m_norm_vector + m_num_features, 0.0);
	m_sum_vector = new double[m_num_features];
	std::fill(m_sum_vector, m_sum_vector + m_num_features, 0.0);
	m_sum2_vector = new double[m_num_features];
	std::fill(m_sum2_vector, m_sum2_vector + m_num_features, 0.0);
      }
      ~Normalize() {
	delete m_norm_vector;
      }
      template<class T>
      void add(T begin, const T end) {
	assert(m_sum_vector != 0 && m_sum2_vector != 0);
	if (size_t(end - begin) != m_num_features)
	  throw std::range_error("Normalize: number features did not match.");
	for (size_t i = 0; begin != end; ++begin, ++i) {
	  m_sum_vector[i] += *begin;
	  m_sum2_vector[i] += *begin * *begin;
	}
	++m_num_feature_vectors;
      }
      void compute_normalization() {
	assert(m_sum_vector != 0 && m_sum2_vector != 0);
	double mean, var, stdev, sum, sum2;
	for (size_t i = 0; i < m_num_features; ++i) {
	  sum = m_sum_vector[i];
	  sum2 = m_sum2_vector[i];
	  mean = sum / m_num_feature_vectors;
	  var = (m_num_feature_vectors * sum2 - sum * sum)
	    / (m_num_feature_vectors * (m_num_feature_vectors - 1));
	  stdev = std::sqrt(var);
	  if (stdev < 0.00001)
	    stdev = 0.00001;
	  m_norm_vector[i] = mean / stdev;
	}
	delete m_sum_vector;
	delete m_sum2_vector;
      }
      // in-place
      template<class T>
      void apply(T begin, const T end) const {
	assert(size_t(end - begin) == m_num_features);
	double* cur = m_norm_vector;
	for (; begin != end; ++begin, ++cur)
	  *begin -= *cur;
      }
      // out-of-place
      template<class T, class U>
      void apply(T in_begin, const T end, U out_begin) const {
	assert(size_t(end - in_begin) == m_num_features);
	double* cur = m_norm_vector;
	for (; in_begin != end; ++in_begin, ++cur, ++out_begin)
	  *out_begin = *in_begin - *cur;
      }
      size_t num_features() const {
	return m_num_features;
      }
      double* get_norm_vector() const {
	return m_norm_vector;
      }
      template<class T>
      void set_norm_vector(T begin, const T end) {
	assert(size_t(end - in_begin) == m_num_features);
	double* cur = m_norm_vector;
	for (; begin != end; ++begin, ++cur)
	  *cur = *begin;	
      }
    private:
      size_t m_num_features;
      size_t m_num_feature_vectors;
      double* m_norm_vector;
      double* m_sum_vector;
      double* m_sum2_vector;
    };

    /*
      K NEAREST NEIGHBORS

      This class holds a list of the k nearest neighbors and provides
      a method of querying for the id of the majority of neighbors. This
      class is meant to be used once - after calling add for each item in
      a database and majority the state of the class is undefined. If another
      search needs to be performed call reset (at which point add for each
      element will need to be called again).
    */
    template<class IdType, class Comp>
    class kNearestNeighbors {
    public:
      /*
	These nested classes are only used in kNearestNeighbors
      */

      /*
	NEIGHBOR
	
	This class holds the information needed for the Nearest Neighbor
	computation.
	
	IdType: the type for the id (possibilities includes longs
	and std::string)
      */
      class Neighbor {
      public:
	Neighbor(IdType id_, double distance_) {
	  id = id_;
	  distance = distance_;
	}
	bool operator<(const Neighbor& other) const {
	  return distance < other.distance;
	}
	IdType id;
	double distance;
      };

      class IdStat {
      public:
	IdStat() {
	  min_distance = std::numeric_limits<double>::max();
	  count = 0;
	}
	IdStat(double distance, size_t c) {
	  min_distance = distance;
	  count = c;
	}
	double min_distance;
	double total_distance;
	size_t count;
      };

      // typedefs for convenience
      typedef IdType id_type;
      typedef Neighbor neighbor_type;
      typedef std::vector<neighbor_type> vec_type;

      // Constructor
      kNearestNeighbors(size_t k = 1) : m_k(k) {
      }
      // Reset the class to its initial state
      void reset() {
	m_nn.clear();
      }
      /*
	Attempt to add a neighbor to the list of k closest
	neighbors. The list of neighbors is always kept sorted
	so that the largest distance is the last element.
      */
      double add(const id_type id, double distance) {
	if (m_nn.size() < m_k) {
	  m_nn.push_back(neighbor_type(id, distance));
	  std::sort(m_nn.begin(), m_nn.end());
	  return -1.0; // -1 means that we haven't found k neighbors yet
	} else if (distance < m_nn.back().distance) {
	  m_nn.back().distance = distance;
	  m_nn.back().id = id;
	  std::sort(m_nn.begin(), m_nn.end());
	}
	return m_nn.back().distance;
      }
      /*
	Find the id of the majority of the k nearest neighbors. This
	includes tie-breaking if necessary.
      */
      std::pair<id_type, double> majority() {
	if (m_nn.size() == 0)
	  throw std::range_error("majority called without enough valid neighbors.");
	// short circuit for k == 1
	if (m_nn.size() == 1) {
	  return std::make_pair(m_nn[0].id, m_nn[0].distance);
	}
	/*
	  Create a histogram of the ids in the nearest neighbors. A map
	  is used because the id_type could be anything. Additionally, even
	  if id_type was an integer there is no garuntee that they are small,
	  ordered numbers (making a vector impractical).
	*/
	typedef std::map<id_type, IdStat, Comp> map_type;
	map_type id_map;
	typename map_type::iterator current;
	for (typename vec_type::iterator i = m_nn.begin();
	     i != m_nn.end(); ++i) {
	  current = id_map.find(i->id);
	  if (current == id_map.end()) {
	    id_map.insert(std::pair<id_type,
			  IdStat>(i->id, IdStat(i->distance, 1)));
	  } else {
	    current->second.count++;
	    current->second.total_distance += i->distance;
	    if (current->second.min_distance > i->distance)
	      current->second.min_distance = i->distance;
	  }
	}
	/*
	  Now that we have the histogram we can take the majority if there
	  is a clear winner, but if not, we need do some sort of tie breaking.
	*/
	if (id_map.size() == 1) {
	  return std::make_pair(id_map.begin()->first,
				id_map.begin()->second.min_distance);
	} else {
	  /*
	    Find the id(s) with the maximum
	  */
	  std::vector<typename map_type::iterator> max;
	  max.push_back(id_map.begin());
	  for (typename map_type::iterator i = id_map.begin();
	       i != id_map.end(); ++i) {
	    if (i->second.count > max[0]->second.count) {
	      max.clear();
	      max.push_back(i);
	    } else if (i->second.count == max[0]->second.count) {
	      max.push_back(i);
	    }
	  }
	  /*
	    If the vector only has 1 element there are no ties and
	    we are done.
	  */
	  if (max.size() == 1)
	    return std::make_pair(max[0]->first, max[0]->second.min_distance);
	  else {
	    /*
	      Tie-break by average distance
	    */
	    typename map_type::iterator min_dist = max[0];
	    for (size_t i = 1; i < max.size(); ++i) {
	      if (max[i]->second.total_distance
		  < min_dist->second.total_distance)
		min_dist = max[i];
	    }
	    return std::make_pair(min_dist->first,
				  min_dist->second.min_distance);
	  }
	}
      }
    private:
      std::vector<neighbor_type> m_nn;
      size_t m_additions; // counter for the number of neighbors added
      size_t m_k;
    };

  } // namespace kNN
} //namespace Gamera

#endif
