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

#include <cmath>

namespace Gamera {
  namespace kNN {

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

  } // namespace kNN
} //namespace Gamera

#endif
