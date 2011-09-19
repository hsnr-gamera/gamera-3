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

#include "graph_common.hpp"
#include "bfsdfsiterator.hpp"
#include "node.hpp"
#include "edge.hpp"
#include "graph.hpp"

namespace Gamera { namespace GraphApi {

   
   
// -----------------------------------------------------------------------------
void BfsIterator::init(Node* start) {
   visit(start);
   _queue.push(start);
}



// -----------------------------------------------------------------------------
Node* BfsIterator::next() {
   if(_queue.empty())
      return NULL;

   Node* current = _queue.front();
   _queue.pop();
   
   for(EdgeIterator it = current->_edges.begin(); 
         it != current->_edges.end(); it++) {

      Node* n = (*it)->traverse(current);
      if(n != NULL && !is_visited(n)) {
         visit(n);
         _queue.push(n);
      }
   }

   return current;
}



// -----------------------------------------------------------------------------
void DfsIterator::init(Node* start) {
   found_cycles = false;
   visit(start);
   _stack.push(start);
}



// -----------------------------------------------------------------------------
Node* DfsIterator::next() {
   if(_stack.empty())
      return NULL;

   Node* current = _stack.top();
   _stack.pop();

   for(EdgeIterator it = current->_edges.begin(); 
         it != current->_edges.end(); it++) {

      Node* n = (*it)->traverse(current);
      if(n != NULL && !is_visited(n)) {
         visit(n);
         _stack.push(n);
         _used_edges.insert(*it);
      }
      else if(!found_cycles && n != NULL && 
            _used_edges.find(*it) == _used_edges.end()) {

         found_cycles = true;
      }
   }

   return current;
}



}} // end Gamera::GraphApi

