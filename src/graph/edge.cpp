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

#include "graph/edge.hpp"
#include "graph/node.hpp"

namespace Gamera { namespace GraphApi {

   
   
// -----------------------------------------------------------------------------
Edge::Edge(Node* from_node, Node* to_node, cost_t weight, 
      bool directed, void* label) {

   this->is_directed = directed;
   this->from_node = from_node;
   this->to_node = to_node;
   this->weight = weight;
   this->label = label;
   from_node->add_edge(this);
   to_node->add_edge(this);
}



// -----------------------------------------------------------------------------
void Edge::remove_self() {
   try {
      if(from_node)
         from_node->_edges.remove(this);
      if(to_node)
         to_node->_edges.remove(this);
   }
   catch (...) {
      assert(false);
   }
   from_node = NULL;
   to_node = NULL;
}



// -----------------------------------------------------------------------------
Node* Edge::traverse(Node* node) {
   if(from_node == NULL || to_node == NULL)
      return NULL;
   else if(node == from_node)
      return to_node;
   else if(!is_directed && node == to_node)
      return from_node;
   return NULL;
}



// -----------------------------------------------------------------------------
Node* Edge::traverse(GraphData * value) { 
   if(from_node == NULL || to_node == NULL)
      return NULL;
   else if(*value == *from_node->_value)
      return to_node;
   else if(!is_directed && *value == *to_node->_value)
      return from_node;
   return NULL;
}



}} // end Gamera::GraphApi

