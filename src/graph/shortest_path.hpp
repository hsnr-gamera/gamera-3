/*
 *
 * Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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

#ifndef mgd010103_shortest_path_hpp
#define mgd010103_shortest_path_hpp

#include "graphlib.hpp"
#include "graph.hpp"

extern "C" {
  PyObject* graph_djikstra_shortest_path(PyObject* self, PyObject* root);
  PyObject* graph_djikstra_all_pairs_shortest_path(PyObject* self, PyObject* root);
  PyObject* graph_all_pairs_shortest_path(PyObject* so, PyObject* args);
}
NodeList* graph_djikstra_shortest_path(GraphObject* so, Node* root);

#define SHORTEST_PATH_METHODS \
  { "djikstra_shortest_path", graph_djikstra_shortest_path, METH_O, \
    "**djikstra_shortest_path** (*value* or *node*)\n\n" \
    "Calculates the shortest path from the given node to all other reachable nodes using Djikstra's algorithm.\n\n" \
    "The return value is a dictionary of paths.  The keys are destination node identifiers and the values are tuples of the form\n\n" \
    "  (*distance*, *nodes*)\n\n" \
    "where distance is the distance traveled from the given node to the destination node and\n" \
    "*nodes* is a list of node identifiers in the shortest path to reach the destination node.\n\n" \
    "This algorithm will use the *cost* values associated with each edge if they are given.\n\n" \
  }, \
  { "shortest_path", graph_djikstra_shortest_path, METH_O, \
    "**shortest_path** (*value* or *node*)\n\n" \
    "An alias for djikstra_shortest_path_.\n\n" }, \
  { "djikstra_all_pairs_shortest_path", graph_djikstra_all_pairs_shortest_path, METH_NOARGS, \
    "**djikstra_all_pairs_shortest_path** ()\n\n" \
    "Calculates the shortest paths between all pairs of nodes in the graph by calling djikstra_shortest_path_ multiple times.\n\n" \
    "The return value is a dictionary where the keys are source node identifiers and the values are dictionaries of paths keyed by destination\n" \
    "node identifiers (of the same form as djikstra_shortest_path_).  " \
    "The values of the internal dictionaries are tuples of the form\n\n" \
    "  (*distance*, *nodes*)\n\n" \
    "where distance is the distance traveled from the given node to the destination node and\n" \
    "*nodes* is a list of node identifiers in the shortest path to reach the destination node.\n\n" \
    "This algorithm will use the *cost* values associated with each edge if they are given.\n\n" \
  }, \
  { "all_pairs_shortest_path", graph_all_pairs_shortest_path, METH_NOARGS, \
    "**all_pairs_shortest_path** ()\n\n" \
    "Calculates the shortest paths between all pairs of nodes in the graph using a tight-inner loop that is\n\n" \
    "generally faster than djikstra_all_pairs_shortest_path_ when the graph is reasonably large.\n\n" \
    "The return value is of the same form as djikstra_all_pairs_shortest_path_.\n" \
    "It is a dictionary where the keys are source node identifiers and the values are dictionaries of paths keyed by destination\n" \
    "node identifiers (of the same form as djikstra_shortest_path_).  " \
    "The values of the internal dictionaries are tuples of the form\n\n" \
    "  (*distance*, *nodes*)\n\n" \
    "where distance is the distance traveled from the given node to the destination node and\n" \
    "*nodes* is a list of node identifiers in the shortest path to reach the destination node.\n\n" \
    "This algorithm will use the *cost* values associated with each edge if they are given.\n\n" \
    }, \

#endif
