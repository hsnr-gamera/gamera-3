/*
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

//////////////////////////////////////////////////////////////////////////////
// Edge
/* Edges contain a pointer to the node at both ends.
   Edges also store a floating point 'cost' value and an arbitrary Python object
   'label' that can be used to store attributes at the edge.
*/

#ifndef mgd010103_edge_hpp
#define mgd010103_edge_hpp

#include "graphlib.hpp"

#define NUM_EDGE_DATA_ELEMENTS 4

struct Edge {
  Edge(GraphObject* graph, Node* from_node,
       Node* to_node, CostType cost = 1.0, PyObject* label = NULL);
  inline ~Edge() {
    if (m_label != NULL)
      if (m_label->ob_refcnt)
	Py_DECREF(m_label);
    m_graph = NULL;
  }
  inline Node* traverse(Node* from_node) {
    //if (HAS_FLAG(m_graph->m_flags, FLAG_DIRECTED))
    //  return m_to_node;
    return (from_node == m_from_node) ? m_to_node : m_from_node;
  }

  GraphObject* m_graph;
  Node* m_from_node;
  Node* m_to_node;
  CostType m_cost;
  PyObject* m_label;
  Any m_edge_properties[NUM_EDGE_DATA_ELEMENTS];
};

struct EdgeObject {
  PyObject_HEAD
  Edge* m_x;
  GraphObject* m_graph;
};

#define EP_VISITED(a) ((a)->m_edge_properties[0].Bool)
#define EP_KNOWN(a) ((a)->m_node_properties[0].Bool)
#define EP_DISTANCE(a) ((a)->m_node_properties[1].Cost)
#define EP_PATH(a) ((a)->m_node_properties[2].NodeObjectPtr)

void init_EdgeType();
PyObject* edgeobject_new(GraphObject* graph, Node* from_node, 
			 Node* to_node, CostType cost = 1.0,
			 PyObject* label = NULL, bool directed = true);
PyObject* edgeobject_new(Edge* edge);
bool is_EdgeObject(PyObject* self);

#endif
