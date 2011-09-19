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
#include "node.hpp"
#include "edge.hpp"
#include "spanning_tree.hpp"

namespace Gamera { namespace GraphApi {



// -----------------------------------------------------------------------------
Graph *SpanningTree::create_minimum_spanning_tree_kruskal(Graph* g) {
   if(g->is_directed()) //Kruskal-algorithm is only for undirected graphs
      return NULL;

   Graph* tree = new Graph(FLAG_TREE);
   std::priority_queue<Edge*, std::vector<Edge*>, mst_compare_func> edgeQueue;
   EdgePtrIterator *eit = g->get_edges();
   Edge* e;
   while((e = eit->next()) != NULL) {
      edgeQueue.push(e);
   }
   
   delete eit;
   NodePtrIterator *nit = g->get_nodes();
   Node* n;
   while((n=nit->next()) != NULL) {
      tree->add_node((n)->_value->copy());
   }

   delete nit;
   while(!edgeQueue.empty() && (tree->get_nnodes()-1) > tree->get_nedges()) {
      Edge* edge = edgeQueue.top();
      edgeQueue.pop();
      bool path_fromto = tree->has_path(edge->from_node->_value, 
            edge->to_node->_value);

      bool path_tofrom = tree->has_path(edge->to_node->_value, 
            edge->from_node->_value);

      if (!path_fromto && !path_tofrom) {
         tree->add_edge(edge->from_node->_value, edge->to_node->_value, 
               edge->weight, false);
      }
   }
 
   return tree; 
}



// -----------------------------------------------------------------------------
Graph *SpanningTree::create_minimum_spanning_tree(Graph *g) {
   return create_minimum_spanning_tree_kruskal(g);
}



// -----------------------------------------------------------------------------
Graph *SpanningTree::create_spanning_tree(Graph* g, Node* root) {
   if(root == NULL)
      throw std::runtime_error("create_spanning_tree NULL exception");

   Graph *t = new Graph(FLAG_DAG);
   NodeSet visited;
   NodeStack node_stack;
   node_stack.push(root);

   while(!node_stack.empty()) {
      Node* n = node_stack.top();
      node_stack.pop();
      visited.insert(n);
      Node* tree_node1 = t->add_node_ptr(n->_value);
     
      EdgePtrIterator* eit = n->get_edges();
      Edge* e;
      while((e = eit->next()) != NULL) {
         Node* inner_node = e->traverse(n);
         if(inner_node != NULL && visited.count(inner_node) == 0) {
            Node* tree_node2 = t->add_node_ptr(inner_node->_value);
            t->add_edge(tree_node1, tree_node2, e->weight, e->label);
            node_stack.push(inner_node);
            visited.insert(inner_node);
         } 
      }
      delete eit;
   }

   return t;
}



}} // end Gamera::GraphApi

