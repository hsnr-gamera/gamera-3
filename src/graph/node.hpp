/*
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

//////////////////////////////////////////////////////////////////////////////
// Node

/* Node objects contain point to 'data', which can be any valid Python object.
   They also contain a list of edges pointing into and pointing out of them.
*/

#ifndef mgd010103_node_hpp
#define mgd010103_node_hpp

#include "graphlib.hpp"

#define NUM_NODE_DATA_ELEMENTS 4

struct Node {
  Node(GraphObject* graph, PyObject* data);
  inline ~Node() {
    if (m_data->ob_refcnt)
      Py_DECREF(m_data);
    m_graph = NULL;
  }
  GraphObject* m_graph;    // graph object this node is a part of
  PyObject* m_data;        // data stored at this node
  EdgeList m_edges;        // edges going out from this node
  bool m_is_subgraph_root; // is this node a subgraph root?
  size_t m_set_id;         // the position of this node in the graph's node vector
  long m_disj_set;         // another node that can be reached from this node
  Any m_node_properties[NUM_NODE_DATA_ELEMENTS];
};

struct NodeObject {
  PyObject_HEAD
  Node* m_x;
  GraphObject* m_graph;
};

#define NP_VISITED(a) ((a)->m_node_properties[0].Bool)
#define NP_KNOWN(a) ((a)->m_node_properties[3].Bool)
#define NP_DISTANCE(a) ((a)->m_node_properties[1].Cost)
#define NP_NUMBER(a) ((a)->m_node_properties[1].SizeT)
#define NP_PATH(a) ((a)->m_node_properties[2].NodePtr)
#define NP_SUBGRAPH_VISITED(a) ((a)->m_node_properties[3].Bool)

void init_NodeType();
PyObject* nodeobject_new(GraphObject* graph, PyObject *data);
PyObject* nodeobject_new(Node* node);
bool is_NodeObject(PyObject* self);

#endif
