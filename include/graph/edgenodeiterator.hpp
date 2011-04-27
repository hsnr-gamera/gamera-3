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

#ifndef _EDGENODEITERATOR_HPP_B2E5621C5D6D95
#define _EDGENODEITERATOR_HPP_B2E5621C5D6D95

#include "nodetraverseiterator.hpp"
#include "edge.hpp"

namespace Gamera { namespace GraphApi {

   
   
// -----------------------------------------------------------------------------
/// lazy iterator iterating over a NodeVector between given borders
class NodePtrIterator: public NodeTraverseIterator {
protected:
   NodeVector::iterator it;
   NodeVector::iterator _begin;
   NodeVector::iterator _end;

public:
   NodePtrIterator(Graph* graph, NodeIterator begin, NodeIterator end): 
         NodeTraverseIterator(graph) {

      _begin = begin;
      _end = end;
      it = begin;
   }

   Node* next() {
      if(it == _end)
         return NULL;
      
      Node* nextnode = *it;
      it++;
      return nextnode;
   }
};



// -----------------------------------------------------------------------------
/// lazy iterator over a complete NodeVector (given by pointer)
class NodeVectorPtrIterator: public NodePtrIterator {
protected:
   NodeVector* _vec;
public:
   NodeVectorPtrIterator(Graph* graph, NodeVector* vec): 
         NodePtrIterator(graph, vec->begin(), vec->end()) {
      _vec = vec;
   }

   ~NodeVectorPtrIterator() {
      delete _vec;
   }

};



// -----------------------------------------------------------------------------
/// lazy iterator iterating over a EdgeVector between given borders
/// returning all edges if from_node is NULL
/// returning edges with from_node as start_node
class EdgePtrIterator {
protected:
   EdgeVector::iterator it;
   EdgeVector::iterator _begin;
   EdgeVector::iterator _end;
   Graph* _graph;
   Node* _from;

public:
   EdgePtrIterator(Graph* graph, EdgeIterator begin, EdgeIterator end, 
         Node* from_node=NULL) {

      _graph = graph;
      _begin = begin;
      _end = end;
      it = begin;
      _from = from_node;
   }
   Edge* next() {
      if(it == _end)
         return NULL;
      Edge* nextedge = *it;
      it++;
      if(_from && nextedge->from_node != _from)
         return next();

      return nextedge;
   }
};



// -----------------------------------------------------------------------------
/// lazy iterator iterating over a EdgeVector between given borders
/// returning the nodes which are directly reachable starting at from_node
class NodePtrEdgeIterator: public EdgePtrIterator {
   Node* _from_node_node;
public:
   NodePtrEdgeIterator(Graph* graph, EdgeIterator begin, EdgeIterator end, 
         Node* from_node = NULL): 
      EdgePtrIterator(graph, begin, end, NULL) {
        _from_node_node = from_node; 
      }


   Node* next() {
      Edge* e = EdgePtrIterator::next();
      if(e == NULL)
         return NULL;

      Node* n = e->traverse(_from_node_node);
      if(n == NULL)
         return next();
      
      return n;
   }
};



}} // end Gamera::GraphApi
#endif /* _EDGENODEITERATOR_HPP_B2E5621C5D6D95 */

