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

#include "graph.hpp"
#include "edge.hpp"
#include "node.hpp"

namespace Gamera { namespace GraphApi {

   
   
// -----------------------------------------------------------------------------
bool Graph::is_directed() {
   return GRAPH_HAS_FLAG(this, FLAG_DIRECTED); 
}



// -----------------------------------------------------------------------------
/// sets each edge to directed and adds the same in other direction
void Graph::make_directed() {
   EdgePtrIterator* it = get_edges();
   Edge* e;
   GRAPH_SET_FLAG(this, FLAG_DIRECTED);
   EdgeVector edges;
   while((e = it->next()) != NULL) {
      e->is_directed = true;
      edges.push_back(e);
   }

   delete it;
   for(EdgeIterator it = edges.begin(); it != edges.end(); it++) {
      Edge* e = *it;
      add_edge(e->to_node, e->from_node, e->weight, true, e->label);
   }
}



// -----------------------------------------------------------------------------
/// This is a small helper-struct for remembering edges in make_undirected
struct smallEdge {
   Node *_to, *_from;
   smallEdge(Node* from, Node* to) {
      _to = to; _from = from;
   }
};



// -----------------------------------------------------------------------------
/// sets each edge as undirected and tries to remove edges in other direction
void Graph::make_undirected() {
   if(is_undirected())
      return;
   std::vector<smallEdge*> remove;
   {
      EdgePtrIterator *it = get_edges();
      Edge* e;
      while((e = it->next()) != NULL) {
         Node* from = e->from_node;
         Node* to = e->to_node;
         e->is_directed = false;
         if(has_edge(to,from)) {
            remove.push_back(new smallEdge(to, from));      
         }
      }

      delete it;
   }
   for(std::vector<smallEdge*>::iterator it = remove.begin(); 
         it != remove.end(); it++) {
      try {
         remove_edge((*it)->_from, (*it)->_to);
      }
      catch (std::runtime_error) {
         std::cerr << "somthing went wrong" << std::endl;
      }
      delete *it;
   }
   GRAPH_UNSET_FLAG(this, FLAG_DIRECTED);
}



// -----------------------------------------------------------------------------
bool Graph::is_cyclic() {
   if(get_nedges() == 0) //trivial case
      return false;
   else if(get_nnodes() == 1) //another trivial case
      return true;

   bool cyclic = false;
   if(is_directed()) { //similar algorithm to make_acyclic
      NodeStack node_stack;
      std::set<Node*> visited;
      if (get_nedges() != 0) {
         NodePtrIterator *i = get_nodes();
         Node* n;
         while((n = i->next()) != NULL && !cyclic) {
            if (visited.count(n) == 0) {
               node_stack.push(n);
               while (!node_stack.empty() && !cyclic) {
                  Node* node = node_stack.top();
                  node_stack.pop();
                  visited.insert(node);

                  EdgePtrIterator *it = node->get_edges();
                  Edge* e;
                  while ((e = it->next()) != NULL && !cyclic) {
                     Node* inner_node = e->traverse(node);
                     if(inner_node) {
                        if(visited.count(inner_node) != 0) {
                           cyclic = true;
                        }
                        else  {
                           node_stack.push(inner_node);
                           visited.insert(inner_node);
                        }
                     }
                  }

                  delete it;
               }
            }
         }

         delete i;
      }
   }  
   else {
      NodeVector* roots = NULL;
      roots = get_subgraph_roots();

      for(NodeVector::iterator rit = roots->begin(); 
         rit != roots->end() && !cyclic; rit++) {
         //tests for cycles in each subgraph
         DfsIterator *it = DFS(*rit);
         while(it->next() != NULL)
            ;
         cyclic = cyclic || it->has_cycles();
         delete it;
      }
      delete roots;
   }
   return cyclic;
}



// -----------------------------------------------------------------------------
// only sets flag to allow cycles
void Graph::make_cyclic() {
   GRAPH_SET_FLAG(this, FLAG_CYCLIC);
}



// -----------------------------------------------------------------------------
/** make_acyclic removes any cycles from the graph. 
 * Another probably better way for doing this could be spanning_tree 
 *
 * This implementation works only with directed graphs. As a solution the
 * graph is converted to a directed graph and back to an undirected after 
 * running the algorithm.
 * */
void Graph::make_acyclic() {
   typedef std::set<Edge*> EdgeSet;
   EdgeSet edges_to_remove;
   NodeStack node_stack;
   std::set<Node*> visited;  
   bool undirected = is_undirected();
   
   if(undirected)
      make_directed();

   if (get_nedges() != 0) {
      NodePtrIterator *i = get_nodes();
      Node* n;
      while((n = i->next()) != NULL) {
         if (visited.count(n) == 0) {
            if (node_stack.size())
               throw std::runtime_error("Error in graph_make_acyclic. "
                     "This error should never be raised.  Please report "
                     "it to the author.");
            node_stack.push(n);
            while (!node_stack.empty()) {
               Node* node = node_stack.top();
               node_stack.pop();
               visited.insert(node);

               EdgePtrIterator *it = node->get_edges();
               Edge* e;
               while ((e = it->next()) != NULL) {
                  Node* inner_node = e->traverse(node);
                  if(inner_node) {
                     if(visited.count(inner_node) != 0) {
                        edges_to_remove.insert(e);
                     }
                     else  {
                        node_stack.push(inner_node);
                        visited.insert(inner_node);
                     }
                  }
               }

               delete it;
            }
         }
      }

      for(EdgeSet::iterator it = edges_to_remove.begin(); 
            it != edges_to_remove.end(); it++) {
         remove_edge(*it); 
      }

      delete i;
   }
   if(undirected)
      make_undirected();

   GRAPH_UNSET_FLAG(this, FLAG_CYCLIC);
}

  

// -----------------------------------------------------------------------------
bool Graph::is_tree() {
   return !is_cyclic() && !is_directed();
}



// -----------------------------------------------------------------------------
// only sets flag
void Graph::make_multi_connected() {
   GRAPH_SET_FLAG(this, FLAG_MULTI_CONNECTED);
}



// -----------------------------------------------------------------------------
// unsets flag and removes multiple edges
void Graph::make_singly_connected() {
   typedef std::pair<Node*, Node*> edgepair;
   EdgeVector to_remove;
   std::set<edgepair > edgeset;
   EdgePtrIterator* it = get_edges();
   Edge* e;
   if(is_directed()) {
      while((e = it->next()) != NULL) {
         edgepair ep(e->from_node, e->to_node);
         if(edgeset.count(ep) > 0)
            to_remove.push_back(e);
         else
            edgeset.insert(ep);
      }
   }
   else {
      while((e = it->next()) != NULL) {  
         edgepair ep(std::min(e->from_node, e->to_node), 
                     std::max(e->from_node, e->to_node));
         if(edgeset.count(ep) > 0)
            to_remove.push_back(e);
         else
            edgeset.insert(ep);
      }
   }

   delete it;
   for(EdgeIterator it = to_remove.begin(); it != to_remove.end(); it++) {
      remove_edge(*it);
   }

   GRAPH_UNSET_FLAG(this, FLAG_MULTI_CONNECTED);
}



// -----------------------------------------------------------------------------
// only sets flag
void Graph::make_self_connected() {
   GRAPH_SET_FLAG(this, FLAG_SELF_CONNECTED);
}



// -----------------------------------------------------------------------------
//unsets flag and remove self-connections
void Graph::make_not_self_connected() {
   std::vector<smallEdge*> remove;
   {
      EdgePtrIterator *it = get_edges();
      Edge* e;
      while((e = it->next()) != NULL) {
         Node* from = e->from_node;
         Node* to = e->to_node;
         if(to == from) {
            remove.push_back(new smallEdge(to, from));      
         }
      }
      delete it;
   }
   for(std::vector<smallEdge*>::iterator it = remove.begin(); 
         it != remove.end(); it++) {
      remove_edge((*it)->_from, (*it)->_to);
      delete *it;
   }

   GRAPH_UNSET_FLAG(this, FLAG_SELF_CONNECTED);
}



// -----------------------------------------------------------------------------
bool Graph::conforms_restrictions() {
   if(!GRAPH_HAS_FLAG(this, FLAG_CYCLIC) && is_cyclic())
      return false;
   if(!GRAPH_HAS_FLAG(this, FLAG_MULTI_CONNECTED) && is_multi_connected())
      return false;
   if(!GRAPH_HAS_FLAG(this, FLAG_SELF_CONNECTED) && is_self_connected()) {
      return false;
   }

   return true;
}



// -----------------------------------------------------------------------------
bool Graph::is_multi_connected() {
   std::set<std::pair<Node*, Node*> > edgeset;
   EdgePtrIterator* it = get_edges();
   Edge* e;
   if(is_directed()) {
      while((e = it->next()) != NULL) {
         edgeset.insert(std::pair<Node*,Node*>(e->from_node, e->to_node));
      }
   }
   else {
      while((e = it->next()) != NULL) {
         edgeset.insert(std::pair<Node*,Node*>(
                  std::min(e->from_node, e->to_node), 
                  std::max(e->from_node, e->to_node)
                  ));
      }
   }

   delete it;
   return edgeset.size() != get_nedges();
}



// -----------------------------------------------------------------------------
bool Graph::is_self_connected() {
   bool selfconnections = false;
   EdgePtrIterator* it = get_edges();
   Edge* e;
   while((e = it->next()) != NULL && !selfconnections) {
      if( *e->from_node->_value == *e->to_node->_value) {
         selfconnections = true;
      }
   }

   delete it;
   return selfconnections;
}



// -----------------------------------------------------------------------------
void Graph::make_tree() {
   GRAPH_SET_FLAG(this, FLAG_TREE);
   make_undirected();
   make_acyclic();
}



// -----------------------------------------------------------------------------
void Graph::make_blob() {
   GRAPH_UNSET_FLAG(this, FLAG_TREE);
}



}} // end Gamera::GraphApi

