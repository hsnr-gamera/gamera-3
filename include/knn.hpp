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

    /*
      K NEAREST NEIGHBORS

    */
    template<class IdType>
    class kNearestNeighbors {
    public:
      typedef IdType id_type;
      typedef Neighbor<IdType> neighbor_type;
      typedef std::vector<neighbor_type> vec_type;

      kNearestNeighbors(size_t k = 1) : m_nn(k) {
	m_additions = 0;
      }

      void add(double distance, const id_type& id) {
	m_additions++;
	size_t last = knn.size() - 1;
	if (distance < m_nn[last].distance) {
	  m_nn[last].distance = distance;
	  m_nn[last].id = id;
	  std::sort(m_nn.begin(), m_nn.end());
	}
      }
      id_type majority() {
	// short circuit for k == 1
	if (m_nn.size() == 1)
	  return m_nn[0];
	/*
	  Create a histogram of the ids in the nearest neighbors. A map
	  is used because the id_type could be anything. Additionally, even
	  if id_type was an integer there is no garuntee that they are small,
	  ordered numbers (making a vector impractical).
	*/
	typedef std::multimap<id_type, size_t> map_type;
	map_type id_map;
	for (vec_type::iterator i = m_nn.begin(); i != m_nn.end(); ++i) {
	  id_map[i->id] += 1;
	}
	/*
	  Now that we have the histogram we can take the majority if there
	  is a clear winner, but if not, we need do some sort of tie breaking.
	*/
	if (id_map.size() == 1)
	  return id_map.begin()->first;
	else {

	}
	std::vector<map_type::iterator> max;
	max.push_back(id_map.begin());
	for (map_type::iterator i = id_map.begin() + 1; i != id_map.end(); ++i) {
	  if (i->second > max->second)
	    max = i;
	}
      }
      std::pair<id_type, double> majority_with_distance() {
	id_type maj = majority();
      }
    private:
      std::vector<neighbor_type> m_nn;
      size_t m_additions;
    }

  } // namespace kNN
} //namespace Gamera

#endif
