//
// Copyright (C) 2010 Tobias Bolten, Oliver Christen
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//


#include "geostructs/colorgraph.hpp"
#include <set>
#include <deque>


//--------------------------------------------------------------
// graph object for coloring a planar graph in linear time
//--------------------------------------------------------------

namespace Gamera { namespace Colorgraph {

  ColorGraph::ColorGraph() {
    this->color_histogramm = NULL;
  }

  ColorGraph::~ColorGraph() {
    delete this->color_histogramm;
  }

  void ColorGraph::add_node(int n) {

    adj_map::iterator it;
    
    it = this->adj.find(n);
    
    if( it == this->adj.end() ) {
        this->adj[n] = neighbor_map();
        this->colors[n] = -1;
    }
  }

  void ColorGraph::remove_node(int n) {

    neighbor_map::iterator neighbor_it;
        
    if( this->adj.count(n) > 0 ) {
        
        for( neighbor_it = this->adj[n].begin() ; neighbor_it != this->adj[n].end() ; neighbor_it++) {
            this->adj[(*neighbor_it).first].erase(n);
        }
        
        this->adj.erase(n);
        this->colors.erase(n);
        
    } else {
        throw std::runtime_error("colorgraph remove_node: Node does not exist.");
    }
  }

  void ColorGraph::add_edge(int u, int v) {
    
    if(u == v) {
        throw std::runtime_error("colorgraph add_egde: Self loops are not allowed.");
    }
    
    this->add_node(u);
    this->add_node(v);
    
    adj_map::iterator adj_it;
    
    adj_it = this->adj.find(u);
    (*adj_it).second[v] = 1;
    
    adj_it = this->adj.find(v);
    (*adj_it).second[u] = 1;
  }

  void ColorGraph::remove_edge(int u, int v) {

    if( this->adj.count(u) + this->adj.count(v) != 2 ) {
        throw std::runtime_error("colorgraph remove_edge: At least one of the nodes does not exist.");
    }
    
    this->adj[u].erase(v);
    this->adj[v].erase(u);
  }

  void ColorGraph::neighbors(int n, NodeVector neighbors) {

    if( neighbors != NULL ) {
        adj_map::iterator adj_it;
        neighbor_map::iterator neighbor_it;
    
        adj_it = this->adj.find(n);
    
        if( adj_it != this->adj.end() ) {
            for( neighbor_it = (*adj_it).second.begin() ; neighbor_it != (*adj_it).second.end() ; neighbor_it++) {
                neighbors->push_back( (*neighbor_it).first );
            }
        } else {
            throw std::runtime_error("colorgraph neighbors: Node does not exist.");
        }
    }
    else {
        throw std::runtime_error("colorgraph neighbors: neighbors have to be != NULL");
    }
  }

  int ColorGraph::size() {
    return this->adj.size();
  }

void ColorGraph::breadth_traversal(std::list<int> *ordering, int start) {
    std::deque<int> nodes_todo;  
    adj_map::iterator adj_it;
    neighbor_map::iterator neighbor_it;


    // Check input parameter
    if( ordering == NULL ) {
        throw std::runtime_error("colorgraph breadth_traversal: null-pointer exception");
    }

    // Create a map for marking already choosen nodes
    std::map<int, bool> node_flags;
    for(adj_it = this->adj.begin(); adj_it != this->adj.end(); adj_it++) {
        node_flags[adj_it->first] = false;
    }  

    // If start is equal to zero no start point is given and the first entry is choosen
    if( start == 0 ) {
        if( this->adj.begin() != this->adj.end() ) {
            nodes_todo.push_back( this->adj.begin()->first );
        }
    }
    else {
        // Is start point in graph?
        if( this->adj.count(start) > 0 ) {
            nodes_todo.push_back( start );
        }
        else {
            throw std::runtime_error("colorgraph breadth_traversal: Node does not exist.");
        }
    }

    int node;
    while( !nodes_todo.empty() ) {
        // Get the first node from the queue
        node = nodes_todo.front();
        nodes_todo.pop_front();

        // Mark node as done        
        node_flags[node] = true;

        // Insert neighbornodes in the queue, if they are not marked and not allready in the queue
        std::map<int, int>::iterator neighbor_it;
        for( neighbor_it = this->adj[node].begin(); neighbor_it != this->adj[node].end(); neighbor_it++) {
            if( !node_flags[neighbor_it->first] ) {
                bool node_in_queue = false;
                for( std::deque<int>::iterator nodes_todo_it = nodes_todo.begin(); nodes_todo_it != nodes_todo.end(); nodes_todo_it++ ) {
                    if( *nodes_todo_it == neighbor_it->first ) {
                        node_in_queue = true;
                    }
                }

                if( !node_in_queue ) {
                    nodes_todo.push_back(neighbor_it->first);
                }
            }
        }

        // Save the node in the output list
        ordering->push_back(node);
    }
}

  void ColorGraph::nodes(NodeVector nodes) {

    if( nodes != NULL ) {
        adj_map::iterator adj_it;
        
        for( adj_it = this->adj.begin() ; adj_it != this->adj.end() ; adj_it++) {
            nodes->push_back( (*adj_it).first );
        }
    }
    else {
        throw std::runtime_error("colorgraph nodes: nodes have to be != NULL");
    }
  }

  void ColorGraph::set_color(int n, int c) {
    
    if( this->adj.count(n) > 0 ) {
        this->colors[n] = c;
    } else {
        throw std::runtime_error("colorgraph set_color: Node does not exist.");
    }
  }

  int ColorGraph::get_color(int n) {
    
    if( this->adj.count(n) > 0 ) {
        return this->colors[n];
    } else {
        throw std::runtime_error("colorgraph get_color: Node does not exist.");
    }
  }

  void ColorGraph::colorize(int c = 6) {
    if( c <= 5 ) {
        throw std::runtime_error("colorgraph colorize: insufficient colors - c has to be at least 6");
    }
    
    std::map<int, std::list<int> > degree_map; // key: degree, value: list of nodes with this degree
    std::map<int, std::list<int> >::iterator degree_map_it;
    
    adj_map adjcpy; // graph copy
    
    neighbor_map::iterator neighbor_it;
    adj_map::iterator adj_it;
    
    // create copy of adjmap, fill degree_map (step 1)
    for( adj_it = this->adj.begin() ; adj_it != this->adj.end() ; adj_it++) {
        
        for( neighbor_it = (*adj_it).second.begin() ; neighbor_it != (*adj_it).second.end() ; neighbor_it++) {
            adjcpy[ (*adj_it).first ][ (*neighbor_it).first ] = (*neighbor_it).second;
        }
        
        degree_map[ (*adj_it).second.size() ].push_back( (*adj_it).first );
    }
        

    int n = adjcpy.size();
    NodeVector removed = new std::vector<int>(n,0); // saves removed node in each step of step 2

    // step 2
    for(int i = adjcpy.size() ; i > 0 ; --i) {
        int tbr; // node to be removed
        int j; // degree of the to be removed node
        
        // find smallest degree list j and get first vertex tbr
        for (degree_map_it = degree_map.begin() ; degree_map_it != degree_map.end() ; ++degree_map_it) {
            if( (*degree_map_it).second.size() > 0 ) {
                tbr = (*degree_map_it).second.front();
                (*removed)[i-1] = tbr;
                j = (*degree_map_it).first;
                (*degree_map_it).second.pop_front();
                break;
            }
         }

        // for all neighbors of the removed node ..
        for( neighbor_it = adjcpy[tbr].begin() ; neighbor_it != adjcpy[tbr].end() ; ++neighbor_it) {
            degree_map[ adjcpy[(*neighbor_it).first].size() ].remove((*neighbor_it).first); // delete from list with current degree
            degree_map[ adjcpy[(*neighbor_it).first].size() - 1 ].push_back( (*neighbor_it).first ); // and insert at degree - 1
        }
        
        for( neighbor_it = adjcpy[tbr].begin() ; neighbor_it != adjcpy[tbr].end() ; neighbor_it++) {
            adjcpy[(*neighbor_it).first].erase(tbr); // delete neighbors of the removed node from the graph copy
        }
        adjcpy.erase(tbr); // remove "removed" node from the graph copy
        
    }
    
    //step 3 - coloring
    // "balanced" coloring: choose the at least often used color
    if( this->color_histogramm != NULL ) {
        this->color_histogramm->clear();
    }
    else {
        this->color_histogramm = new histogramm_map();
    }

    std::map<int, int>::iterator color_histogramm_it;

    // Initialise the color counting structure
    for(int i = 0; i < c; i++) {
        (*(this->color_histogramm))[i] = 0;
    }

    for(std::vector<int>::iterator rit = removed->begin() ; rit != removed->end() ; ++rit) {
        std::set<int> avcolors;
        for(int i = 0 ; i < c ; ++i) {
            avcolors.insert(i);
        }
        
        // erase the colors which are set in the neighborhood of the node
        for( neighbor_it = this->adj[(*rit)].begin() ; neighbor_it != this->adj[(*rit)].end() ; neighbor_it++) {
            std::set<int>::iterator avcolors_it = avcolors.find(this->get_color((*neighbor_it).first));

            if(avcolors_it != avcolors.end() ) {
                avcolors.erase( avcolors_it );
            }
        }

        // create a map which contains all colors we are allowed to choose from
        std::map<int, int> possible_colors;
        std::map<int, int>::iterator possible_colors_it;
        for(color_histogramm_it = this->color_histogramm->begin() ; color_histogramm_it != this->color_histogramm->end() ; color_histogramm_it++ ) {
            if( avcolors.count( color_histogramm_it->first ) > 0 ) {
                possible_colors.insert(std::pair<int, int>(color_histogramm_it->first, color_histogramm_it->second));
            }
        }

        // Should never happen
        if( possible_colors.size() == 0 ) {
            throw std::runtime_error("colorgraph colorize: no valied color assignment possible :( ");
        }


        // Determine the at least often used color (from the possible colors for this node)
        possible_colors_it = possible_colors.begin();
        int min_color_index = possible_colors_it->first;
        int min_color_color = possible_colors_it->second;
        while( possible_colors_it != possible_colors.end() ) {
            if( possible_colors_it->second < min_color_color ) {
                min_color_index = possible_colors_it->first;
                min_color_color = possible_colors_it->second;
            }

            possible_colors_it++;
        }
        
        // Color the node
        this->set_color((*rit), min_color_index);
        (*(this->color_histogramm))[min_color_index]++;
        
    }    
    
    delete removed; 
  }

  bool ColorGraph::is_valid_coloration() {
    
    adj_map::iterator adj_it;
    neighbor_map::iterator neighbor_it;
    
    for( adj_it = this->adj.begin() ; adj_it != this->adj.end() ; adj_it++) {
        int node_color = this->colors[(*adj_it).first];
        
        for( neighbor_it = (*adj_it).second.begin() ; neighbor_it != (*adj_it).second.end() ; neighbor_it++) {
            if (node_color == this->colors[(*neighbor_it).first] || node_color == -1) {
                return false;
            }
        }
        
    }
    
    return true;    
  }

  histogramm_map* ColorGraph::get_color_histogramm() {
    return this->color_histogramm;
  }

}} // end namespace Gamera::Colorgraph
