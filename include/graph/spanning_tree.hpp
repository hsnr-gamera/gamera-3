/*
 *
 * Copyright (C) 2011 Christian Brandt
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


#ifndef _SPANNING_TREE_HPP_6AC2CA9C54D727
#define _SPANNING_TREE_HPP_6AC2CA9C54D727
#include <map>
#include "graph_common.hpp"

namespace Gamera { namespace GraphApi {

   
   
typedef std::set<Edge*> EdgeSet;



class SpanningTree {
   struct mst_compare_func {
      bool operator() (Edge* const& a, Edge* const& b) const {
         return a->weight > b->weight;
      }
   };

public:
   static Graph *create_minimum_spanning_tree(Graph* g);
   static Graph *create_minimum_spanning_tree_kruskal(Graph* g);
   static Graph* create_spanning_tree(Graph* g, Node* n);
};



}} // end Gamera::GraphApi
#endif /* _SPANNING_TREE_HPP_6AC2CA9C54D727 */

