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

#ifndef _GRAPH_HPP_97CAD8A8D00E4D
#define _GRAPH_HPP_97CAD8A8D00E4D

#include "graph_common.hpp"
#include "bfsdfsiterator.hpp"
#include "edgenodeiterator.hpp"
#include "edge.hpp"

namespace Gamera { namespace GraphApi {

   
   
// Colorization
typedef std::vector<unsigned int> Histogram;
typedef std::map<Node*, int> ColorMap;



struct Graph { 
   NodeVector _nodes;      ///< list of nodes in this graph
   EdgeVector _edges;      ///< list of edges in this graph
   ValueNodeMap _valuemap; ///< STL-Map value->node for this graph
   flag_t _flags;

   ColorMap* _colors;
   Histogram* _colorhistogram;


   // --------------------------------------------------------------------------
   // Structure
   // --------------------------------------------------------------------------
   Graph(bool directed , bool check_on_insert=false);
   Graph(flag_t flags = FLAG_FREE);
   ~Graph();
   Graph(Graph &g);
   Graph(Graph *g, flag_t flags = FLAG_FREE);



   // --------------------------------------------------------------------------
   // methods for nodesnodes
   // --------------------------------------------------------------------------
   bool add_node(GraphData * value);
   bool add_node(Node* node);

   /// add node variant returns ptr to added node or existing node
   Node* add_node_ptr(GraphData * value);
   int add_nodes(NodeVector nodes);
   int add_nodes(ValueVector values);

   Node* get_node(GraphData * value);
   NodePtrIterator* get_nodes();
   bool has_node(Node* node);
   bool has_node(GraphData * value);

   void remove_node(Node* node);
   void remove_node(GraphData * value);
   void remove_node_and_edges(Node* node);
   void remove_node_and_edges(GraphData * value);

   // --------------------------------------------------------------------------
   // methods for edges
   // --------------------------------------------------------------------------
   int add_edge(GraphData * from_value, GraphData * to_value, 
         cost_t cost = 1.0, bool directed = false, 
         void* label = NULL);
   int add_edge(Node* from_node, Node* to_node, cost_t cost = 1.0, 
         bool directed = false, void* label = NULL);

   EdgePtrIterator* get_edges();
   bool has_edge(GraphData * from_value, GraphData * to_value);
   bool has_edge(Node* from_node, Node* to_node);

   bool has_edge(Edge* edge) { 
      return has_edge(edge->from_node, edge->to_node); 
   }

   void remove_edge(GraphData * from_value, GraphData * to_value);
   void remove_edge(Node* from_node, Node* to_node);
   void remove_edge(Edge* edge);
   void remove_all_edges();
 
   // --------------------------------------------------------------------------
   // size of graph
   // --------------------------------------------------------------------------
   size_t get_nnodes() { return _nodes.size(); }
   size_t get_nedges() { return _edges.size(); }


   // --------------------------------------------------------------------------
   // flags and restrictions
   // --------------------------------------------------------------------------
   bool conforms_restrictions();
   bool has_flag(flag_t flag) {
      return GRAPH_HAS_FLAG(this, flag);
   }

   bool is_directed();
   bool is_undirected() { return !is_directed();}
   void make_directed();
   void make_undirected();

   bool is_cyclic();
   bool is_acyclic() { return !is_cyclic(); }
   void make_cyclic();
   void make_acyclic();

   void make_tree();
   void make_blob();
   bool is_tree();
   bool is_blob() { return !is_tree(); }

   bool is_multi_connected();
   bool is_singly_connected() { return !is_multi_connected(); }
   void make_multi_connected();
   void make_singly_connected();

   bool is_self_connected();
   bool is_not_self_connected() { return !is_self_connected(); }
   void make_self_connected();
   void make_not_self_connected();

   
   
   // --------------------------------------------------------------------------
   // subgraphs
   // --------------------------------------------------------------------------
   NodeVector *get_subgraph_roots();
   size_t size_of_subgraph(GraphData * value);
   size_t size_of_subgraph(Node* node);
   size_t get_nsubgraphs();
   bool is_fully_connected();


   
   // --------------------------------------------------------------------------
   // algorithms
   // --------------------------------------------------------------------------
   BfsIterator* BFS(Node* node);
   BfsIterator* BFS(GraphData * value);
   DfsIterator* DFS(Node* node);
   DfsIterator* DFS(GraphData * value);

   ShortestPathMap* dijkstra_shortest_path(Node* node);
   ShortestPathMap* dijkstra_shortest_path(GraphData * value);
   ShortestPathMap* shortest_path(Node* node) { 
      return dijkstra_shortest_path(node); 
   }
   ShortestPathMap* shortest_path(GraphData * value) { 
      return dijkstra_shortest_path(value); 
   }

   std::map<Node*, ShortestPathMap*> dijkstra_all_pairs_shortest_path();

   /// currently same as dijkstra_all_pairs_shortest_path
   std::map<Node*, ShortestPathMap*> all_pairs_shortest_path();

   bool has_path(Node* from_node, Node* to_node);
   bool has_path(GraphData * from_value, GraphData * to_value);

   Graph *create_spanning_tree(Node* node);
   Graph *create_spanning_tree(GraphData * value);
   Graph *create_minimum_spanning_tree(); //kruskal

   //optimize_partitions only implemented in Python-wrapper


   // --------------------------------------------------------------------------
   // colorization
   // --------------------------------------------------------------------------
   unsigned int get_color(Node* n);
   unsigned int get_color(GraphData * value) {
      Node* n = get_node(value);
      return get_color(n);
   }
   void set_color(Node* n, unsigned int color);
   void colorize(unsigned int ncolors = 6);
};



}} // end Gamera::GraphApi

#endif /* _GRAPH_HPP_97CAD8A8D00E4D */

