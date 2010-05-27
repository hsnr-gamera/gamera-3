#ifndef colorgraph_20100430_HPP
#define colorgraph_20100430_HPP

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


#include <map>
#include <vector>
#include <string>
#include <list>
#include <stdexcept>

//--------------------------------------------------------------
// graph object for coloring a planar graph in linear time
//--------------------------------------------------------------

namespace Gamera { namespace Colorgraph {

  typedef std::map<int, int> histogramm_map;
  typedef std::map<int, int> neighbor_map;
  typedef std::map<int, int> color_map;
  typedef std::map<int, neighbor_map > adj_map;
  typedef std::vector<int> * NodeVector;
  typedef std::vector<int>::iterator ColorGraphIterator;

  class ColorGraph {
  private:
    adj_map adj;
    color_map colors;
    histogramm_map *color_histogramm;
  public:
    ColorGraph();
    ~ColorGraph();

    void add_node(int n);
    void remove_node(int n);
    void add_edge(int u, int v);
    void remove_edge(int u, int v);
    void neighbors(int n, NodeVector neighbors);
    void nodes(NodeVector nodes);
    int size();

    void breadth_traversal(std::list<int> *ordering, int start = 0);
    
    void set_color(int n, int c);
    int get_color(int n);
    void colorize(int c);
    bool is_valid_coloration();
    histogramm_map * get_color_histogramm();
  };

}} // end namespace Gamera::Colorgraph

#endif
