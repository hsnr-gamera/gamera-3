/*
 *
 * Copyright (C) 2010 Tobias Bolten, Oliver Christen
 *               2011 Christian Brandt
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

#include <limits>
#include <algorithm>
#include "graph/graph.hpp"
#include "graph/node.hpp"
#include "graph/graphdataderived.hpp"
namespace Gamera { namespace GraphApi {

   
   
// -----------------------------------------------------------------------------
unsigned int Graph::get_color(Node* n) {
   if(_colors == NULL)
      throw std::runtime_error("Graph::get_color: Graph is not colorized");

   
   ColorMap::iterator it = _colors->find(n);
   if(it == _colors->end())
      throw std::runtime_error("Graph::get_color: Node is not colorized");
   
   return it->second;
}



// -----------------------------------------------------------------------------
void Graph::set_color(Node* n, unsigned int color) {
   if(_colors == NULL) {
      _colors = new ColorMap();
   }
   (*_colors)[n] = color;
}



// -----------------------------------------------------------------------------
/**
 * algorithm from 
 * ftp://db.stanford.edu/pub/cstr/reports/cs/tr/80/830/CS-TR-80-830.pdf
 * */
void Graph::colorize(unsigned int ncolors) {
   if (ncolors < 6) {
      throw std::runtime_error("Graph::colorize: insufficient colors. "
            "ncolors has to be at least 6");
   }
   typedef std::list<Node*> NodeList;
   typedef std::map<int, NodeList*> DegreeMap;
   typedef std::map<Node*, int> NodeDegreeMap;
   NodeDegreeMap nodedegrees;
   DegreeMap degrees;
   NodePtrIterator* n_it;
   Node* n;

   // --------------------------------------------------------------------------
   //Step 1: form degree_list
   n_it = get_nodes();
   while((n = n_it->next()) != NULL) {
      size_t nnodes = n->get_nnodes();
      if(degrees[nnodes] == NULL)
         degrees[nnodes] = new NodeList;
      degrees[nnodes]->push_back(n);
      nodedegrees[n] = nnodes;
   }
   delete n_it;
   
   // --------------------------------------------------------------------------
   //Step 2
   //
   std::vector<Node*> removed(get_nnodes());

   for(int i = get_nnodes()-1; i >= 0; i--) {
      Node* to_be_removed = NULL;

      //find first Node of smallest degree
      for(DegreeMap::iterator it = degrees.begin(); it != degrees.end(); it++) {
         if(it->second->size() > 0) {
            to_be_removed = it->second->front();
            removed[i] = to_be_removed;
            it->second->pop_front();
            break;
         } 
      }

      if(to_be_removed == NULL)
         throw std::runtime_error("Something went wrong when colorizing");


      NodePtrEdgeIterator* neighbors = to_be_removed->get_nodes();
      Node* neighbor;
      while((neighbor = neighbors->next()) != NULL) {
         int degree = nodedegrees[neighbor];
         if(degree == -1) // not in a degree list
            continue;

         NodeList::iterator it = std::find(degrees[degree]->begin(), 
               degrees[degree]->end(), neighbor);
         
         if(it != degrees[degree]->end()) {
            degrees[degree]->erase(it);
            nodedegrees[n] = degree -1;
            if(degree >= 0) {
               if(degrees[degree-1] == NULL)
                  degrees[degree-1] = new NodeList;

               //insert at degree-1
               degrees[degree-1]->push_back(neighbor);
            }
         }
      }

      delete neighbors;
     

   }


   // --------------------------------------------------------------------------
   //Step 3
   
   //initialize histogram with 0
   if(_colorhistogram != NULL) {
      delete _colorhistogram;
   }
   _colorhistogram = new Histogram(ncolors, 0);

   n_it = get_nodes();
   try {
      for(std::vector<Node*>::iterator it = removed.begin(); it != removed.end(); it++) {
   //   while ((n = n_it->next()) != NULL) {
         n = *it;
         if(n == NULL)
            continue;

         std::vector<bool> available_colors(ncolors, true);

         //erase colors which are set in the neighborhood of the node
         NodePtrEdgeIterator* neighbors = n->get_nodes();
         Node* neighbor;
         unsigned int neighbor_color;
         while((neighbor = neighbors->next()) != NULL) {
            try {
               neighbor_color = get_color(neighbor);
               available_colors[neighbor_color] = false;
#ifdef __DEBUG_GAPI__
               std::cerr<< neighbor_color << ",";
#endif
            }
            catch(std::runtime_error) {
               //node is not colorized
            }
         }
#ifdef __DEBUG_GAPI__
         std::cerr << std::endl;
#endif
         delete neighbors;

         int color = -1;
         unsigned int mincount = std::numeric_limits<unsigned int>::max();
         for(unsigned int i = 0; i < ncolors; i++) {
            unsigned int count = (*_colorhistogram)[i];
#ifdef __DEBUG_GAPI__
            std::cerr << "color: " << i <<"count: " << count << std::endl;
#endif
            if(available_colors[i] == true && (color == -1 || count <= mincount)) {
               color = i;
               mincount = count;
#ifdef __DEBUG_GAPI__
               std::cerr << "new color: " << color << std::endl;
#endif
            }
         }

         if(color < 0) {
#ifdef __DEBUG_GAPI__
            GraphDataLong* dat = dynamic_cast<GraphDataLong*>(n->_value);
            if(dat)
               std::cerr << "no more colors at label: " << dat->data << std::endl;
#endif
            throw std::runtime_error("not enough colors for this graph");
         }
#ifdef __DEBUG_GAPI__
         std::cerr << "decided color: " << color << std::endl;
#endif
         set_color(n, color);
         (*_colorhistogram)[color]++;
      }
   }
   catch(std::runtime_error) {
      //free degreemap
      for(DegreeMap::iterator it = degrees.begin(); it != degrees.end(); it++) {
         delete it->second;
      }
      delete n_it;
      throw;
   }

   for(DegreeMap::iterator it = degrees.begin(); it != degrees.end(); it++) {
      delete it->second;
   }
   delete n_it;
}



}} // end Gamera::GraphApi

