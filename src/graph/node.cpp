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

#include "graph/node.hpp"
#include "graph/graph.hpp"
#include "graph/edge.hpp"

namespace Gamera { namespace GraphApi {

   
   
// -----------------------------------------------------------------------------
Node::Node(GraphData * value, Graph* graph) {
   _value = value;
   _graph = graph;
}



// -----------------------------------------------------------------------------
Node::~Node() {
#ifdef __DEBUG_GAPI__
//   std::cout << "~Node\n";  
#endif
}



// -----------------------------------------------------------------------------
Node::Node(Node& node) {
   _value = node._value;
   _graph = node._graph;
}



// -----------------------------------------------------------------------------
Node::Node(Node* node) {
   _value = node->_value;
   _graph = node->_graph;
}



// -----------------------------------------------------------------------------
EdgePtrIterator* Node::get_edges(bool both_directions) {
   Node* from_node = NULL;
   if(_graph->is_directed() && !both_directions)
      from_node = this;

   EdgePtrIterator* it= new EdgePtrIterator(_graph, _edges.begin(), 
         _edges.end(), from_node);
   return it;
}



// -----------------------------------------------------------------------------
NodePtrEdgeIterator* Node::get_nodes() {
   Node* from_node = NULL;
   from_node = this;

   NodePtrEdgeIterator* it = new NodePtrEdgeIterator(_graph, _edges.begin(), 
         _edges.end(), from_node);
   return it;
}



// -----------------------------------------------------------------------------
bool Node::has_edge_to(Node* node) {
   Edge* e;
   bool ret = false;
   EdgePtrIterator* it = get_edges();
   while((e = it->next()) != NULL && ret==false) {
      if (e->to_node == node)
         ret = true;
   }
   delete it;
   return ret;
}



// -----------------------------------------------------------------------------
bool Node::has_edge_from(Node* node) {
   Edge* e;
   bool ret = false;
   EdgePtrIterator* it = get_edges();
   while((e = it->next()) != NULL && ret==false) {
      if (e->from_node == node)
         ret = true;
   }
   delete it;
   return ret;
}



// -----------------------------------------------------------------------------
void Node::add_edge(Edge* edge) {
   if(edge->from_node == this || edge->to_node == this) {
      _edges.push_back(edge);
   }
   else
      throw std::runtime_error("edge not valid for this node");
}



// -----------------------------------------------------------------------------
void Node::remove_edge(Edge* edge) {
   _edges.remove(edge);
}



// -----------------------------------------------------------------------------
void Node::remove_self(bool glue) {
   EdgeVector to_remove;
   typedef std::pair<Node*, cost_t> NodeWeight;
   typedef std::vector<NodeWeight> NodeWeightVector;
   NodeWeightVector in_nodes, out_nodes;

   if(glue) {
      EdgePtrIterator* it = get_edges(true);
      Edge* e;
      while((e = it->next()) != NULL) {
         if(*e->from_node->_value == *this->_value && 
               *e->to_node->_value != *this->_value) {

            out_nodes.push_back(NodeWeight(e->to_node, e->weight));
         }
         else if(*e->to_node->_value == *this->_value && 
               *e->from_node->_value != *this->_value) {
            
            in_nodes.push_back(NodeWeight(e->from_node, e->weight));
         }

         to_remove.push_back(e);
      }
      delete it;
   }
   else {
      for(EdgeIterator it = _edges.begin(); it != _edges.end(); it++) {
         to_remove.push_back(*it);
      }
   }

   size_t i = _edges.size();
   for(EdgeIterator it = to_remove.begin(); it != to_remove.end(); it++) {
      Edge* e = *it;
      if(e->to_node != NULL && e->from_node != NULL) {
         if (e->to_node != this && e->to_node != NULL)
            e->to_node->_edges.remove(e);
         if (e->from_node != this && e->from_node != NULL)
            e->from_node->_edges.remove(e);
     
         e->to_node = NULL;
         e->from_node = NULL; 
         _graph->_edges.remove(e);
         e->weight = 2000;
         delete e;
      }
      i--;
   }

   if(glue) {
      for(NodeWeightVector::iterator iit = in_nodes.begin(); 
            iit != in_nodes.end(); iit++) {

         for(NodeWeightVector::iterator oit = out_nodes.begin(); 
               oit != out_nodes.end(); oit++) {

            Node* from_node = iit->first;
            Node* to_node = oit->first;
            if(from_node != to_node && from_node != this && to_node != this)
               _graph->add_edge(from_node, to_node, 
                     iit->second+oit->second, _graph->is_directed());
         }
      }
   }
   
   remove_from_graph();
}



}} // end Gamera::GraphApi

