/*
 *
 * Copyright (C) 2001 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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
    "The shortest paths from the given root" }, \
  { "shortest_path", graph_djikstra_shortest_path, METH_O, \
    "The shortest path from the given root" }, \
  { "djikstra_all_pairs_shortest_path", graph_djikstra_all_pairs_shortest_path, METH_NOARGS, \
    "The shortest paths between all nodes (using Djikstra algorithm N times)" }, \
  { "all_pairs_shortest_path", graph_all_pairs_shortest_path, METH_NOARGS, \
    "The shortest paths between all nodes (using tight inner loops)" },

#endif
