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
// Edge
/* Edges contain a pointer to the node at both ends.
   Edges also store a floating point 'cost' value and an arbitrary Python object
   'label' that can be used to store attributes at the edge.
*/

#ifndef mgd010103_edge_hpp
#define mgd010103_edge_hpp

#include "graphlib.hpp"
#include "iterator.hpp"

#define NUM_EDGE_DATA_ELEMENTS 4

struct Edge {
  GraphObject* m_graph;
  Edge* m_other;
  Node* m_from_node;
  Node* m_to_node;
  CostType m_cost;
  PyObject* m_label;
  Any m_edge_properties[NUM_EDGE_DATA_ELEMENTS];
};

struct EdgeObject {
  PyObject_HEAD
  Edge* m_x;
};

#define EP_VISITED(a) ((a)->m_edge_properties[0].Bool)
#define EP_KNOWN(a) ((a)->m_node_properties[0].Bool)
#define EP_DISTANCE(a) ((a)->m_node_properties[1].Cost)
#define EP_PATH(a) ((a)->m_node_properties[2].NodeObjectPtr)

void init_EdgeType();
PyObject* edgeobject_new(GraphObject* graph, Node* from_node, 
			 Node* to_node, CostType cost = 1.0,
			 PyObject* label = NULL);
PyObject* edgeobject_new(Edge* edge);
Edge* edge_new(GraphObject* graph, Node* from_node, 
	       Node* to_node, CostType cost = 1.0,
	       PyObject* label = NULL);
void edge_dealloc(Edge* so);
bool is_EdgeObject(PyObject* self);

#endif
