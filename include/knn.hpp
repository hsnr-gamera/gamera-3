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

#include <vector>
#include <map>
#include <cmath>
#include "gamera_limits.hpp"

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
				      IterB unknown, IterC weight) {
      double distance = 0;
      for (; known != end; ++known, ++unknown, ++weight)
	distance += *w * std::abs(*known - *unknown);
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
				     IterB unknown, IterC weight) {
      double distance = 0;
      for (; known != end; ++known, ++unknown, ++weight)
	distance += *w * std::sqrt(*known * *known - *unknown * *unknown);
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
					  IterB unknown, IterC weight) {
      double distance = 0;
      for (; known != end; ++known, ++unknown, ++weight)
	distance += *w * (*known * *known - *unknown * *unknown);
      return distance;
    }

    /*
      NEIGHBOR

      This class holds the information needed for the Nearest Neighbor
      computation.

      IdType: the type for the id (possibilities includes longs and std::string)
    */
    template<class IdType>
    class Neighbor {
    public:
      Neighbor() : id() {
	/*
	  we initialize distance to max so that there is
	  special case in kNearestNeighbors::add for when
	  the vector is not full
	*/
	distance = std::numeric_traits<double>::max();
      }
      bool operator<(const Neigbor& other) {
	return distance < other.distance;
      }
      IdType id;
      double distance;
    };

    namespace {

      class IdStat {
      public:
	IdStat() {
	  min_distance = std::numeric_traits<double>::max();
	  count = 0;
	}
	IdStat(double distance, size_t c) {
	  min_distance = distance;
	  count = c;
	}
	double min_distance;
	size_t count;
      };
    }

    /*
      K NEAREST NEIGHBORS

      This class holds a list of the k nearest neighbors and provides
      a method of querying for the id of the majority of neighbors. This
      class is meant to be used once - after calling add for each item in
      a database and majority the state of the class is undefined. If another
      search needs to be performed call reset.
    */
    template<class IdType>
    class kNearestNeighbors {
    public:
      // typedefs for convenience
      typedef IdType id_type;
      typedef Neighbor<IdType> neighbor_type;
      typedef std::vector<neighbor_type> vec_type;
      // Constructor
      kNearestNeighbors(size_t k = 1) : m_nn(k), m_k(k) {
	m_additions = 0;
      }
      // Reset the class to its initial state
      void reset() {
	m_nn.clear();
	m_nn.resize(m_k);
	m_additions = 0;
      }
      /*
	Attempt to add a neighbor to the list of k closest
	neighbors. The list of neighbors is always kept sorted
	so that the largest distance is the last element.
      */
      void add(double distance, const id_type& id) {
	m_additions++;
	if (distance < m_nn.back().distance) {
	  m_nn.back().distance = distance;
	  m_nn.back().id = id;
	  std::sort(m_nn.begin(), m_nn.end());
	}
      }
      /*
	Find the id of the majority of the k nearest neighbors. This
	includes tie-breaking if necessary.
      */
      id_type majority() {
	/*
	  make certain that we have at least k valid
	  entries in the list of neighbors. This is probably not
	  a problem most of the time, but it is important to
	  check.
	*/
	if (m_additions < m_nn.size())
	  m_nn.resize(m_additions);
	// short circuit for k == 1
	if (m_nn.size() == 1)
	  return m_nn[0];
	/*
	  Create a histogram of the ids in the nearest neighbors. A map
	  is used because the id_type could be anything. Additionally, even
	  if id_type was an integer there is no garuntee that they are small,
	  ordered numbers (making a vector impractical).
	*/
	typedef std::map<id_type, IdStat> map_type;
	map_type id_map;
	for (vec_type::iterator i = m_nn.begin(); i != m_nn.end(); ++i) {
	  id_map::iterator current = id_map.find(i->id);
	  if (current == id_map.end()) {
	    id_map.insert(std::pair<id_type,
			  IdStat>(i->id, IdStat(i->distance, 1)));
	  } else {
	    current->second.count++;
	    
	  }
	}
	/*
	  Now that we have the histogram we can take the majority if there
	  is a clear winner, but if not, we need do some sort of tie breaking.
	*/
	if (id_map.size() == 1)
	  return id_map.begin()->first;
	else {
	  /*
	    Find the id(s) with the maximum
	  */
	  std::vector<map_type::iterator> max;
	  max.push_back(id_map.begin());
	  for (map_type::iterator i = id_map.begin() + 1;
	       i != id_map.end(); ++i) {
	    if (i->second > max[0]->second) {
	      max.clear();
	      max.push_back(i);
	    } else if (i->second == max[0]->second) {
	      max.push_back(i);
	    }
	  }
	  /*
	    If the vector only has 1 element there are no ties and
	    we are done.
	  */
	  if (max.size() == 1)
	    return max[0]->first;
	  else {
	    
	  }
	}
      }

    private:
      std::vector<neighbor_type> m_nn;
      size_t m_additions; // counter for the number of neighbors added
      size_t m_k;
    }

  } // namespace kNN
} //namespace Gamera

#endif
