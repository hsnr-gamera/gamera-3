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
#include "edge.hpp"
#include "node.hpp"
#include "bfsdfsiterator.hpp"
#include "spanning_tree.hpp"
#include "shortest_path.hpp"
#include "subgraph_root.hpp"

namespace Gamera { namespace GraphApi {

   
   
// -----------------------------------------------------------------------------
NodeVector *Graph::get_subgraph_roots() {
   NodeVector* roots;
   SubgraphRoots* algo = new SubgraphRoots();
   roots = algo->subgraph_roots(this);
   delete algo;
   return roots;
}



// -----------------------------------------------------------------------------
size_t Graph::get_nsubgraphs() {
   NodeVector *roots = get_subgraph_roots();
   size_t count = roots->size();
   delete roots;
   return count;
}



// -----------------------------------------------------------------------------
size_t Graph::size_of_subgraph(GraphData * value) {
   Node* node = get_node(value);
   if(node != NULL) {
      return size_of_subgraph(node);
   }
   return 0;
}



// -----------------------------------------------------------------------------
bool Graph::is_fully_connected() {
   Node* start = _nodes.front();
   size_t count = 0;
   DfsIterator *it = DFS(start);
   while(it->next() != 0)
      count++;

   delete it;
   return count == get_nnodes();
}



// -----------------------------------------------------------------------------
bool Graph::has_path(GraphData * from_value, GraphData * to_value) {
   Node* from_node = get_node(from_value);
   Node* to_node = get_node(to_value);
   
   if(from_node == NULL || to_node == NULL)
      return false;
   else
      return has_path(from_node, to_node);

}



// -----------------------------------------------------------------------------
bool Graph::has_path(Node* from_node, Node* to_node) {
   DfsIterator it(this, from_node);
   Node* n;
   while((n = it.next()) != NULL) {
      if(to_node == n)
         return true;
   }
   return false;
}



// -----------------------------------------------------------------------------
/// returns size of subgraph with root *node*. please remind that starting node
/// is not counted;
size_t Graph::size_of_subgraph(Node* node) {
   DfsIterator it(this, node);
   size_t count = 0;

   while(it.next() != NULL)
      count++;

   return count;
}



// -----------------------------------------------------------------------------
// algorithms
// -----------------------------------------------------------------------------
BfsIterator* Graph::BFS(Node* node) {
   if(node == NULL)
      return NULL;
   BfsIterator* it = new BfsIterator(this, node);
   return it;
}



// -----------------------------------------------------------------------------
BfsIterator* Graph::BFS(GraphData * value) {
   Node* node = get_node(value);
   return BFS(node);
}



// -----------------------------------------------------------------------------
DfsIterator* Graph::DFS(Node* node) {
   if(node == NULL)
      return NULL;
   DfsIterator* it = new DfsIterator(this, node);
   return it;
}



// -----------------------------------------------------------------------------
DfsIterator* Graph::DFS(GraphData * value) {
   Node* node = get_node(value);
   return DFS(node);
}



// -----------------------------------------------------------------------------
ShortestPathMap* Graph::dijkstra_shortest_path(Node* node) {
   if(node == NULL) {
      return NULL;
   }
   else {
      ShortestPath s;
      return s.dijkstra_shortest_path(this, node);
   }
}



// -----------------------------------------------------------------------------
ShortestPathMap* Graph::dijkstra_shortest_path(GraphData * value) {
   Node *node = get_node(value);
   return dijkstra_shortest_path(node);
}



// -----------------------------------------------------------------------------
std::map<Node*, ShortestPathMap*> Graph::dijkstra_all_pairs_shortest_path() {
   std::map<Node*, ShortestPathMap*> res;
   NodePtrIterator *it = get_nodes();
   Node* n;
   
   while((n=it->next()) != NULL) {
      res[n] = dijkstra_shortest_path(n);
   }   
   delete it;
   return res;
}



// -----------------------------------------------------------------------------
std::map<Node*, ShortestPathMap*> Graph::all_pairs_shortest_path() {
   //multiple dijkstra is better (O(n^2 log n + e)) than algorithm in 
   //old implementation (O(n^3))
   return dijkstra_all_pairs_shortest_path();
}



// -----------------------------------------------------------------------------
Graph *Graph::create_spanning_tree(GraphData * value) {
   Node* n = get_node(value);
   if(n == NULL)
      return NULL;

   return create_spanning_tree(n);
}



// -----------------------------------------------------------------------------
/// creates a spanning tree using DFS technique
Graph *Graph::create_spanning_tree(Node* node) { 
   return SpanningTree::create_spanning_tree(this, node);
}



// -----------------------------------------------------------------------------
Graph *Graph::create_minimum_spanning_tree() { //Kruskal
   return SpanningTree::create_minimum_spanning_tree(this);
}



}} // end Gamera::GraphApi

