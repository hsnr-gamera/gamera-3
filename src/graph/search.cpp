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

#include "search.hpp"

PyObject* graph_BFS(PyObject* self, PyObject* pyobject) {
  GraphObject* so = ((GraphObject*)self);
  NodeObject* root;
  root = graph_find_node(so, pyobject);
  if (root == 0)
    return 0;
  BFSIterator* iterator = iterator_new_simple<BFSIterator>();
  iterator->init(so, root);
  return (PyObject*)iterator;
}

PyObject* graph_DFS(PyObject* self, PyObject* pyobject) {
  GraphObject* so = ((GraphObject*)self);
  NodeObject* root;
  root = graph_find_node(so, pyobject);
  if (root == 0)
    return 0;
  DFSIterator* iterator = iterator_new_simple<DFSIterator>();
  iterator->init(so, root);
  return (PyObject*)iterator;
}

