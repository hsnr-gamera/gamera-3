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

#ifndef _NODE_HPP_6F6639BC0A8223
#define _NODE_HPP_6F6639BC0A8223

#include "graph_common.hpp"
#include "edgenodeiterator.hpp"

namespace Gamera { namespace GraphApi {



struct Node {
   EdgeVector _edges;   /// < edges pointing in/out this node
   GraphData * _value;    /// < nodes's value
   Graph* _graph;

   Node(GraphData * value, Graph* graph = NULL);
   ~Node();
   Node(Node& node);
   Node(Node* node);

   void add_to_graph(Graph* graph) {
      _graph = graph;
   }

   void remove_from_graph() {
      if(_graph != NULL) {
         _graph = NULL;
      }
   }

   /// compares this value with given node's value
   bool operator==(Node& n) {
      return *_value == *n._value;

   }

   /// compares this value with given value
   bool operator==(GraphData * v) {
      return *_value == *v;
   }


   /** Iterator over edges pointing in/out this node
    * @param both_directions  false: only edges pointing out this node
    *                         true: edges pointing in and out this node
    **/
   EdgePtrIterator* get_edges(bool both_directions = false);

   /// Iterator over all nodes reachable from this node
   NodePtrEdgeIterator* get_nodes();

   size_t get_nedges() { return _edges.size() ; }
   size_t get_nnodes() {
      NodePtrEdgeIterator* it = get_nodes();
      size_t count = 0;
      while(it->next() != NULL)
         count++;

      delete it;
      return count;
   }

   bool has_edge_to(Node* node);
   bool has_edge_from(Node* node);

   void add_edge(Edge* edge);
   void remove_edge(Edge* edge);

   /** removes node from graph
    * @param glue    true if neighbor-edges should be glued together
    * */
   void remove_self(bool glue=false);
};



}} // end Gamera::GraphApi
#endif /* _NODE_HPP_6F6639BC0A8223 */

