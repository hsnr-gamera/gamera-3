/*
 *
 * Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, Karl MacMillan
 *               2011      Christoph Dalitz
 *               2011      Christian Brandt
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

#ifndef _GRAPHOBJECT_ALGORITHM_HPP_80F4CF20EC34D5
#define _GRAPHOBJECT_ALGORITHM_HPP_80F4CF20EC34D5

#include "wrapper.hpp"
#include "partitions.hpp"

extern "C" {
  PyObject* graph_dijkstra_shortest_path(PyObject* self, PyObject* root);
  PyObject* graph_dijkstra_all_pairs_shortest_path(PyObject* self, PyObject* _);
  PyObject* graph_all_pairs_shortest_path(PyObject* self, PyObject* _);
  PyObject* graph_create_spanning_tree(PyObject* self, PyObject* pyobject);
  PyObject* graph_create_minimum_spanning_tree(PyObject* so, PyObject* args);
  PyObject* graph_BFS(PyObject* self, PyObject* args);
  PyObject* graph_DFS(PyObject* self, PyObject* args);
  PyObject* graph_get_color(PyObject* self, PyObject* pyobject);
  PyObject* graph_colorize(PyObject* self, PyObject* pyobject);
}


// -----------------------------------------------------------------------------
#define SHORTEST_PATH_METHODS \
      { CHAR_PTR_CAST "dijkstra_shortest_path", graph_dijkstra_shortest_path, METH_O, \
      CHAR_PTR_CAST "**dijkstra_shortest_path** (*value* or *node*)\n\n" \
      "Calculates the shortest path from the given node to all other reachable " \
      "nodes using Djikstra's algorithm.\n\n" \
      "The return value is a dictionary of paths.  The keys are destination node " \
      "identifiers and the values are tuples of the form\n\n" \
      "  (*distance*, *nodes*)\n\n" \
      "where distance is the distance traveled from the given node to the "\
      "destination node and *nodes* is a list of node identifiers in the "\
      "shortest path to reach the destination node.\n\n" \
      "This algorithm will use the *cost* values associated with each edge if "\
      "they are given.\n\n" \
  }, \
{ CHAR_PTR_CAST "shortest_path", graph_dijkstra_shortest_path, METH_O, \
    CHAR_PTR_CAST "**shortest_path** (*value* or *node*)\n\n" \
    "An alias for dijkstra_shortest_path_.\n\n" }, \
  { CHAR_PTR_CAST "dijkstra_all_pairs_shortest_path", \
     graph_dijkstra_all_pairs_shortest_path, METH_NOARGS, \
    CHAR_PTR_CAST "**dijkstra_all_pairs_shortest_path** ()\n\n" \
    "Calculates the shortest paths between all pairs of nodes in the graph by "\
    "calling dijkstra_shortest_path_ multiple times.\n\n" \
    "The return value is a dictionary where the keys are source node identifiers"\
    " and the values are dictionaries of paths keyed by destination\n" \
    "node identifiers (of the same form as dijkstra_shortest_path_).  " \
    "The values of the internal dictionaries are tuples of the form\n\n" \
    "  (*distance*, *nodes*)\n\n" \
    "where distance is the distance traveled from the given node to the " \
    "destination node and *nodes* is a list of node identifiers in the shortest " \
    "path to reach the destination node.\n\n" \
    "This algorithm will use the *cost* values associated with each edge if " \
    "they are given.\n\n" \
  }, \
  { CHAR_PTR_CAST "all_pairs_shortest_path", graph_all_pairs_shortest_path, METH_NOARGS, \
    CHAR_PTR_CAST "**all_pairs_shortest_path** ()\n\n" \
     "An alias for dijkstra_all_pairs_shortest_path_.\n\n" }, 

#define SPANNING_TREE_METHODS \
  { CHAR_PTR_CAST "create_spanning_tree", graph_create_spanning_tree, METH_O, \
    CHAR_PTR_CAST "**create_spanning_tree** (*value* or *node*)\n\n" \
    "Returns a new graph which is a (probably non-optimal) spanning tree of all nodes reachable from the given node. This tree is created using DFS." }, \
  { CHAR_PTR_CAST "create_minimum_spanning_tree", graph_create_minimum_spanning_tree, METH_VARARGS, \
    CHAR_PTR_CAST "**create_minimum_spanning_tree** ()\n\n" \
    "Creates a minimum spanning tree of the entire graph in place using Kruskal's algorithm.\n" \
    "A minimum spanning tree connects all nodes using the minimum total edge cost.\n" \
  }, \


#define SEARCH_METHODS \
  { CHAR_PTR_CAST "BFS", graph_BFS, METH_O, \
    CHAR_PTR_CAST "**BFS** (*value* or *node*)\n\n" \
    "A lazy iterator that returns the nodes in breadth-first order starting from the given *value* or *node*. Note that the starting node is included in the returned nodes." }, \
  { CHAR_PTR_CAST "DFS", graph_DFS, METH_O, \
    CHAR_PTR_CAST "**DFS** (*value* or *node*)\n\n" \
    "A lazy iterator that returns the nodes in depth-first order starting from the given *value* or *node*.  Note that the starting node is included in the returned nodes." }, \


#define COLOR_METHODS \
  { CHAR_PTR_CAST "colorize", graph_colorize, METH_O, \
    CHAR_PTR_CAST "**colorize** (*ncolors*)\n\n" \
    "This method colors the graph using a count of *ncolors* colors with the " \
    "constraint that adjacent nodes have different colors. When the number of " \
    "colors is not big enough for the given graph, a runtime_error is raised.\n" \
    "The graph coloring algorithm is based on the \"6-COLOR\" alorithm for planar " \
    "graphs, as described in:\n\n" \
    "   D. Matula, Y. Shiloach, R. Tarjan:\n"\
    "   `Two linear-time algorithms for five-coloring a planar graph.`__\n" \
    "   Tech Rep STAN-CS-80-830, Computer Science Dep., Stanford Univ., 1980\n\n" \
    ".. __: ftp://db.stanford.edu/pub/cstr/reports/cs/tr/80/830/CS-TR-80-830.pdf\n\n"\
    "We have modified the algorithm in such way that the color distribution is\n"\
    "balanced, i.e. that each color is assigned approximately to the same\n"\
    "number of nodes (also known as \"equitable coloring\").\n\n"\
    ".. note:: The coloring algorithm works in linear time and is only\n" \
    "   guaranteed to work for planar graphs and *ncolors* > 5. When the algorithm runs out\n" \
    "   of colors (e.g., because the graph is not planar), an exception is thrown." \
  }, \
  { CHAR_PTR_CAST "get_color", graph_get_color, METH_O, \
    CHAR_PTR_CAST "**get_color** (*value* or *node*)\n\n" \
    "Returns the color of the node after the graph being colorized with colorize_." }, \



#define ALGORITHM_METHODS \
   PARTITIONS_METHODS \
   SHORTEST_PATH_METHODS \
   SPANNING_TREE_METHODS \
   SEARCH_METHODS \
   COLOR_METHODS


#endif /* _GRAPHOBJECT_ALGORITHM_HPP_80F4CF20EC34D5 */

