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

#include "subgraph_root.hpp"

namespace Gamera { namespace GraphApi {

   

void SubgraphRoots::process(SubgraphNode* v) {
   SubgraphNode *w = NULL;

   v->root = true;
   v->visited = true;

   DfsIterator* it = g->DFS(v->n);
   Node* w_node = it->next();
   while((w_node = it->next()) != NULL) {
      w = nodeMap[w_node];
      w->root = false;
      w->visited = true;
   }
   delete it;
}



// -----------------------------------------------------------------------------
NodeVector* SubgraphRoots::subgraph_roots(Graph* g) {
   this->g = g;
   NodePtrIterator *nit = g->get_nodes();
   Node *n = NULL;
   while((n = nit->next()) != NULL) {
      SubgraphNode* t = new SubgraphNode(n);
      nodeMap[n] = t;
   }
   delete nit;
   i = 0;


   for(std::map<Node*, SubgraphNode*>::iterator it = nodeMap.begin(); 
         it != nodeMap.end(); it ++) {
      SubgraphNode* v = it->second;
      if(!v->visited) {
         process(v);
      }
   }

   //create root-vector
   NodeVector *nv = new NodeVector();
   for(std::map<Node*, SubgraphNode*>::iterator it = nodeMap.begin(); 
         it != nodeMap.end(); it ++) {
      if(it->second->root)
         nv->push_back(it->second->n);
      delete it->second;
   }

   return nv;
}



}} // end Gamera::GraphApi

