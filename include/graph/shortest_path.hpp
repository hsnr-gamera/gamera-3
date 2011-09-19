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

#ifndef _SHORTEST_PATH_HPP_64D06E271FB0FA
#define _SHORTEST_PATH_HPP_64D06E271FB0FA

#include <map>
#include <vector>
#include <limits>
#include <queue>
#include "node.hpp"
#include "edge.hpp"
#include "graph_common.hpp"

namespace Gamera { namespace GraphApi {



// -----------------------------------------------------------------------------
/// helper-class for remembering distances and predecessors in 
/// Dijkstra's algorithm
struct DijkstraNode {
   Node* node;
   cost_t distance;
   Node* predecessor;
   bool visited;
   
   DijkstraNode(Node *n) {
      node = n;
      distance = std::numeric_limits<cost_t>::max();
      predecessor = NULL;
      visited = false;
   }

   bool operator<(DijkstraNode &d2) {
      return distance < d2.distance;
   }
};



// -----------------------------------------------------------------------------
/// class encapsulating Dijkstra's algorithm
class ShortestPath {
protected:
   std::map<Node*,DijkstraNode*> nodes;
  
   /// comparator for comparing distances in min-queue
   struct dijkstra_min_cmp {
      bool operator() (DijkstraNode* const &a, DijkstraNode* const &b) const {
         return a->distance > b->distance;
      }
   };

   std::priority_queue<DijkstraNode*, std::vector<DijkstraNode*>, dijkstra_min_cmp> queue;

   NodeSet shortestpath;
   Graph* _graph;
   Node* _sourcenode;


   
   void init_single_source(Graph* g, Node *s);

   
   
   // --------------------------------------------------------------------------
   inline void relax(Edge* e) {
      DijkstraNode* from, *to;
      from = nodes[e->from_node];
      to = nodes[e->to_node];
      if(to->distance > from->distance + e->weight) {
         to->distance = from->distance + e->weight;
         to->predecessor = e->from_node;
         if(shortestpath.find(to->node) == shortestpath.end())
            queue.push(to);
      }
      if(!e->is_directed && from->distance > to->distance + e->weight) {
         from->distance = to->distance + e->weight;
         from->predecessor = e->to_node;
         if(shortestpath.find(from->node) == shortestpath.end())
            queue.push(from);
      }
   }


public:
   ShortestPathMap* dijkstra_shortest_path(Graph* g, Node *source);
   std::map<Node*,ShortestPathMap*>* dijkstra_all_pairs_shortest_path(Graph* g);
   ~ShortestPath();
   ShortestPathMap* faster_all_pairs_shortest_path(Graph *g);

};



}} // end Gamera::GraphApi
#endif /* _SHORTEST_PATH_HPP_64D06E271FB0FA */

