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

#include "spanning_tree.hpp"
#include "gameramodule.hpp"
#include "gamera.hpp"

using namespace Gamera;

GraphObject* graph_create_spanning_tree(GraphObject* so, Node* root) {
  GraphObject* tree = graph_new(FLAG_DAG);
  NodeVector::iterator i = so->m_nodes->begin();
  for (; i != so->m_nodes->end(); ++i)
    NP_VISITED(*i) = false;
  NodeStack node_stack;
  node_stack.push(root);
  while (!node_stack.empty()) {
    Node* node = node_stack.top();
    node_stack.pop();
    NP_VISITED(node) = true;
    Node* new_node1 = graph_add_node(tree, node->m_data);
    for (EdgeList::iterator j = node->m_edges.begin();
			j != node->m_edges.end(); ++j) {
      Node* inner_node = (*j)->traverse(node);
      if (!NP_VISITED(inner_node)) {
		  Node* new_node2 = graph_add_node(tree, inner_node->m_data);
		  graph_add_edge(tree, new_node1, new_node2, (*j)->m_cost);
		  node_stack.push(inner_node);
		  NP_VISITED(inner_node) = true;
      }
    }
  }
  return tree;
}

PyObject* graph_create_spanning_tree(PyObject* self, PyObject* pyobject) {
  GraphObject* so = ((GraphObject*)self);
  Node* root;
  root = graph_find_node(so, pyobject);
  if (root == NULL)
    return 0;
  return (PyObject*)graph_create_spanning_tree(so, root);
}


struct minimum_spanning_queue_comp_func
{
  bool operator()(Edge* const& a, Edge* const& b) const { 
    return a->m_cost > b->m_cost;
  }
};

GraphObject* graph_create_minimum_spanning_tree(GraphObject* so) {
  // Kruskal's algorithm
  typedef std::priority_queue<Edge*, std::vector<Edge*>,
    minimum_spanning_queue_comp_func> EdgeQueue;
  EdgeQueue edge_queue;

  for (NodeVector::iterator i = so->m_nodes->begin();
       i != so->m_nodes->end(); ++i) {
	 for (EdgeList::iterator j = (*i)->m_edges.begin();
	      j != (*i)->m_edges.end(); ++j) {
	   edge_queue.push(*j);
	 }
  }
  
  size_t flags = so->m_flags;
  UNSET_FLAG(flags, FLAG_CYCLIC);
  GraphObject* tree = graph_new(flags);
  tree->m_nodes->reserve(so->m_nodes->size());
  for (NodeVector::iterator i = so->m_nodes->begin();
       i != so->m_nodes->end(); ++i) {
    graph_add_node(tree, (*i)->m_data);
  }
  
  while (!edge_queue.empty() && tree->m_edges->size() < tree->m_nodes->size() - 1) {
    Edge* edge = edge_queue.top();
    edge_queue.pop();
    graph_add_edge(tree, edge->m_from_node->m_data, edge->m_to_node->m_data,
		   edge->m_cost, edge->m_label);
  }
  return tree;
}

namespace {
  struct DistsSorter {
    DistsSorter(FloatImageView* image) { m_image = image; }
    bool operator()(const std::pair<size_t, size_t>& a,
		    const std::pair<size_t, size_t>& b) {
      return m_image->get(a.first, a.second) < m_image->get(b.first, b.second);
    }
    FloatImageView* m_image;
  };
}

PyObject* graph_minimum_spanning_tree_unique_distances(GraphObject* so, PyObject* images,
						       PyObject* uniq_dists) {
  if (!PyList_Check(images)) {
    PyErr_SetString(PyExc_TypeError, "images must be a list.");
    return 0;
  }
  
  static PyTypeObject* imagebase = 0;
  if (imagebase == 0) {
    PyObject* mod = PyImport_ImportModule("gamera.gameracore");
    if (mod == 0) {
      PyErr_SetString(PyExc_RuntimeError, "Unable to load gameracore.\n");
      return 0;
    }
    PyObject* dict = PyModule_GetDict(mod);
    if (dict == 0) {
      PyErr_SetString(PyExc_RuntimeError, "Unable to get module dictionary\n");
      return 0;
    }
    imagebase = (PyTypeObject*)PyDict_GetItemString(dict, "Image");
  }
  // get the matrix
  if (!PyObject_TypeCheck(uniq_dists, imagebase)
      || get_pixel_type(uniq_dists) != Gamera::FLOAT) {
    PyErr_SetString(PyExc_TypeError, "uniq_dists must be a float image.");
    return 0;
  }
  FloatImageView* dists = (FloatImageView*)((RectObject*)uniq_dists)->m_x;
  if (dists->nrows() != dists->ncols()) {
    PyErr_SetString(PyExc_TypeError, "image must be symmetric.");
    return 0;
  }
  
  // get the graph ready
  graph_remove_all_edges(so);
  graph_make_acyclic(so);
  
  // make the list for sorting
  typedef std::vector<std::pair<size_t, size_t> > index_vec_type;
  index_vec_type indexes(((dists->nrows() * dists->nrows()) - dists->nrows()) / 2);
  size_t row, col, index = 0;
  for (row = 0; row < dists->nrows(); ++row) {
    for (col = row + 1; col < dists->nrows(); ++col) {
      indexes[index].first = row;
      indexes[index++].second = col;
    }
  }
  std::sort(indexes.begin(), indexes.end(), DistsSorter(dists));

  // Add the nodes to the graph and build our map for later
  int images_len = PyList_Size(images);
  std::vector<Node*> nodes(images_len);
  int i;
  for (i = 0; i < images_len; ++i) {
    nodes[i] = graph_add_node(so, PyList_GET_ITEM(images, i));
  }
  
  // create the mst using kruskal
  i = 0;
  while (i < int(indexes.size()) && (int(so->m_edges->size()) < (images_len - 1))) {
    size_t row = indexes[i].first;
    size_t col = indexes[i].second;
    graph_add_edge(so, nodes[row], nodes[col], dists->get(row, col));
    ++i;
  }
  Py_INCREF(Py_None);
  return Py_None;
}

PyObject* graph_create_minimum_spanning_tree(PyObject* self, PyObject* args) {
  PyObject* images = 0;
  PyObject* uniq_dists = 0;
  if (PyArg_ParseTuple(args, "|OO", &images, &uniq_dists) <= 0)
	 return 0;
  GraphObject* so = ((GraphObject*)self);
  if (images == 0 || uniq_dists == 0)
    return (PyObject*)graph_create_minimum_spanning_tree(so);
  else
    return graph_minimum_spanning_tree_unique_distances(so, images, uniq_dists);
}

