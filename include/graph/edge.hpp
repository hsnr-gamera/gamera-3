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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _EDGE_HPP_A95093D2DFCAB7
#define _EDGE_HPP_A95093D2DFCAB7
#include "graph_common.hpp"

namespace Gamera { namespace GraphApi {

   
   
// -----------------------------------------------------------------------------
/** The graph containing edge can only be reached by from_node->_graph or 
    * to_node->_graph because every edge has at least two nodes in same graph.
    * So an additional pointer in Edge is waste of memory
    * */
struct Edge {
   Node* from_node;
   Node* to_node;
   bool is_directed; ///< should be same as graph's directed
   cost_t weight;
   void* label;
   
   /** creates a new edge. directed must be the same as the Graph's flag 
    * @param from_node  Node this edge is pointing from
    * @param to_node    Node this edge is pointing to
    * @param weight     positive edge-weight used in some algorithms
    * @param directed   if true, this edge points in both directions
    *                   this should be same as graph's flag DIRECTED
    * @param label      label
    **/
   Edge(Node* from_node, Node* to_node, cost_t weight = 1.0, 
         bool directed = false, void* label = NULL);

   /** removes this from nodes this edge is pointing from/to 
    **/
   void remove_self();

   /** returns other end of edge if it is reachable from the 
    * given node or NULL if not reachable
    **/
   Node* traverse(Node* node);
   Node* traverse(GraphData * value); 
};



}} // end Gamera::GraphApi

#endif /* _EDGE_HPP_A95093D2DFCAB7 */

