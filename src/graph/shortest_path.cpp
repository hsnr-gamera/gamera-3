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

#include "graph.hpp"
#include "node.hpp"
#include "edge.hpp"
#include "shortest_path.hpp"

namespace Gamera { namespace GraphApi {



// -----------------------------------------------------------------------------
void ShortestPath::init_single_source(Graph* g, Node *s) {
   NodePtrIterator *it = g->get_nodes();
   Node* itn; 
   while((itn = it->next()) != NULL) {
      DijkstraNode *n = new DijkstraNode(itn);
      if(itn == s) {
         n->distance = 0;
         queue.push(n);
      }
      nodes[itn] = n;
      //queue.push(n);
   }
   
}



// -----------------------------------------------------------------------------
ShortestPathMap* ShortestPath::dijkstra_shortest_path(Graph* g, Node *source) {
   DfsIterator* it = g->DFS(source);
   Node* node;
   
   while((node = it->next()) != NULL) {
      DijkstraNode *n = new DijkstraNode(node);
      nodes[node] = n;
   }

   delete it;

   nodes[source]->distance = 0;
   queue.push(nodes[source]);

   while(!queue.empty()) {
      DijkstraNode *n = queue.top();
      queue.pop();
      if(!n->visited) {
         n->visited = true;
         EdgePtrIterator* eit = n->node->get_edges();
         Edge* e;
         while((e = eit->next()) != NULL) {
            DijkstraNode* from = nodes[e->from_node];
            DijkstraNode* to = nodes[e->to_node];

            if(from == n && from->distance + e->weight < to->distance) {
               to->distance = from->distance + e->weight;
               to->predecessor = from->node;
               queue.push(to);
            }

            //undirected edges in both directions
            if(!GRAPH_HAS_FLAG(g, FLAG_DIRECTED) && to == n && 
                  to->distance + e->weight < from->distance) {

               from->distance = to->distance + e->weight;
               from->predecessor = to->node;
               queue.push(from);
            }
         }
         
         delete eit;
      }
   }

   /* create map for results */
   ShortestPathMap *result = new ShortestPathMap();
   NodePtrIterator *it2 = g->get_nodes();
   Node* itn;
   while((itn = it2->next()) != NULL) {
      DijkstraPath path;
      Node* n = itn;
      
      DijkstraNode *dn = nodes[n];
      if(dn != NULL) {
         path.cost = dn->distance;
      }
      else
         path.cost = 0;
      while(n != NULL) {
         path.path.push_back(n);
         dn = nodes[n];
         if(dn != NULL)
            n = dn->predecessor;
         else
            n = NULL;
      }
      (*result)[itn] = path;
   }

   delete it2;

   return result;
}



// -----------------------------------------------------------------------------
std::map<Node*,ShortestPathMap*>* ShortestPath::dijkstra_all_pairs_shortest_path(
      Graph* g) {

   std::map<Node*, ShortestPathMap*>* result = new std::map<Node*, ShortestPathMap*>;
   NodePtrIterator* it = g->get_nodes();
   Node* n;
   while((n = it->next()) != NULL) {
      (*result)[n] = dijkstra_shortest_path(g, n);
   }
   delete it;
   return result;
}



// -----------------------------------------------------------------------------
ShortestPath::~ShortestPath() {
   for(std::map<Node*,DijkstraNode*>::iterator it = nodes.begin(); 
         it != nodes.end(); it++) {

      delete it->second;
   }
}



// -----------------------------------------------------------------------------
ShortestPathMap *ShortestPath::faster_all_pairs_shortest_path(Graph* g) {
   size_t nnodes = g->get_nnodes();
   size_t i = 0;
   std::map<Node*, unsigned long> nodes;
   std::vector<cost_t> weights(nnodes*2, std::numeric_limits<cost_t>::max());

   //init nodes
   Node *n;
   NodePtrIterator* nit = g->get_nodes();
   while((n = nit->next()) != NULL) {
      nodes[n] = i;
      i++;
   }

   delete nit;

   //init edges
   Edge *e;
   EdgePtrIterator* eit = g->get_edges();
   while((e = eit->next()) != NULL) {
      size_t from = nodes[e->from_node];
      size_t to = nodes[e->to_node];
      weights[from*nnodes+to] = e->weight;
   }
   delete eit;
   return NULL;
}



}} // end Gamera::GraphApi

