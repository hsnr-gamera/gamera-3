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

#ifndef _BFSDFSITERATOR_HPP_F320082952C20F
#define _BFSDFSITERATOR_HPP_F320082952C20F

#include "nodetraverseiterator.hpp"

namespace Gamera { namespace GraphApi {



/// lazy iterator performing a Breath-First-Search starting at *start*
class BfsIterator: public NodeTraverseIterator {
   NodeQueue _queue;
   void init(Node* start);

public:
   BfsIterator(Graph* graph, Node* start): NodeTraverseIterator(graph) {
      init(start);
   };
   Node* next();
};


/// lazy iterator performing a Depth-First-Search starting at *start*
/** this iterator can also be used for recognizing cycles by calling 
 * has_cycles() after a complete search
 * */
class DfsIterator: public NodeTraverseIterator {
   NodeStack _stack;
   std::set<Edge*> _used_edges;
   bool found_cycles;
   void init(Node* start);

public:
   DfsIterator(Graph* graph, Node* start): NodeTraverseIterator(graph) {
      init(start);  
   }
   Node* next();

   bool has_cycles() {
      return found_cycles;
   }

};



}} // end Gamera::GraphApi
#endif /* _BFSDFSITERATOR_HPP_F320082952C20F */

