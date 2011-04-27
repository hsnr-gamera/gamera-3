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


#ifndef _NODETRAVERSEITERATOR_HPP_826C06039D50C0
#define _NODETRAVERSEITERATOR_HPP_826C06039D50C0

#include "graph_common.hpp"

namespace Gamera { namespace GraphApi {

   
   
// -----------------------------------------------------------------------------
/**
  * lazy iterator for traversing through a graph node-by-node
  * must be subclassed for using a concrete alrogithm 
  * (BFS, DFS, etc)
  * 
  * next() returns next Node or NULL when there are no more nodes.
  *
  * Please note that chaning the graph invalidates this iterator and 
  * leads to undefined behaviour.
  *
  * initially no nodes are marked as visited
  */
class NodeTraverseIterator {
protected:
   Graph* _graph;
   NodeSet _visited;

public:
   NodeTraverseIterator(Graph* graph) {
      _graph = graph;
   }   

   /// marks the given node as visited
   inline void visit(Node* node) {
      _visited.insert(node);
   }

   /// marks the given node as not visited
   inline void unvisit(Node* node) {
      if(is_visited(node))
         _visited.erase(node);
   }

   /// returns true when the given node is marked as visited
   inline bool is_visited(Node* node) {
      return _visited.count(node) == 1;
   }

   /// returns pointer to next node or NULL when there is no more node
   virtual Node* next() = 0;
};



}} // end Gamera::GraphApi
#endif /* _NODETRAVERSEITERATOR_HPP_826C06039D50C0 */

