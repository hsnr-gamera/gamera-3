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

#include "edge.hpp"

Edge::Edge(GraphObject* graph, Node* from_node,
	   Node* to_node, CostType cost, PyObject* label) :
  m_graph(graph), m_from_node(from_node), m_to_node(to_node),
  m_cost(cost), m_label(label) {
  if (label != NULL)
    Py_INCREF(label);
}

extern "C" {
  static void edgeobject_dealloc(PyObject* self);
  static PyObject* edge___repr__(PyObject* self);
  static PyObject* edge___call__(PyObject* self, PyObject* args, PyObject* kwds);
  static PyObject* edge_traverse(PyObject* self, PyObject* node);
  static PyObject* edge_get_from_node(PyObject* self);
  static PyObject* edge_get_to_node(PyObject* self);
  static PyObject* edge_get_cost(PyObject* self);
  static int edge_set_cost(PyObject* self, PyObject* obj);
  static PyObject* edge_get_label(PyObject* self);
  static int edge_set_label(PyObject* self, PyObject* obj);
}
static PyTypeObject EdgeType = {
  PyObject_HEAD_INIT(NULL)
  0,
};

PyMethodDef edge_methods[] = {
  { "traverse", edge_traverse, METH_O,
    "Get the 'other' node in an edge." },
  { NULL }
};

PyGetSetDef edge_getset[] = {
  { "from_node", (getter)edge_get_from_node, 0,
    "Node this edge starts from (get)", 0 },
  { "to_node", (getter)edge_get_to_node, 0,
    "Node this edge points to (get)", 0 },
  { "cost", (getter)edge_get_cost, (setter)edge_set_cost,
    "Cost of traversing this node (get/set)", 0 },
  { "label", (getter)edge_get_label, (setter)edge_set_label,
    "An arbitrary label attached to the edge (get/set)", 0 },
  { NULL }
};

PyObject* edgeobject_new(Edge* edge) {
  EdgeObject* so;
  so = (EdgeObject*)EdgeType.tp_alloc(&EdgeType, 0);
  so->m_x = edge;
  so->m_graph = edge->m_graph;
  Py_INCREF(edge->m_graph);
  return (PyObject*)so;
}  

PyObject* edgeobject_new(GraphObject* graph, Node* from_node,
			 Node* to_node, CostType cost, PyObject* label) {
  EdgeObject* so;
  so = (EdgeObject*)EdgeType.tp_alloc(&EdgeType, 0);
  so->m_x = new Edge(graph, from_node, to_node, cost, label);
  so->m_graph = graph;
  Py_INCREF(graph);
  return (PyObject*)so;
}  

bool is_EdgeObject(PyObject* self) {
  return PyObject_TypeCheck(self, &EdgeType);
}

void edgeobject_dealloc(PyObject* self) {
#ifdef DEBUG_DEALLOC
  std::cerr << "edgeobject dealloc " << PyString_AsString(PyObject_Repr(self)) << std::endl;
#endif
  EdgeObject* so = (EdgeObject*)self;
  PyObject* graph = (PyObject*)so->m_graph;
  self->ob_type->tp_free(self);
  Py_DECREF(graph);
}

PyObject* edge___repr__(PyObject* self) {
  Edge* so = ((EdgeObject*)self)->m_x;
  PyObject* cost = PyFloat_FromDouble(so->m_cost);
  return PyString_FromFormat("<Edge from %s to %s (%s)>", 
			     PyString_AsString(PyObject_Repr((PyObject*)so->m_from_node->m_data)),
			     PyString_AsString(PyObject_Repr((PyObject*)so->m_to_node->m_data)),
			     PyString_AsString(PyObject_Repr(cost)));
}

PyObject* edge___call__(PyObject* self, PyObject* args, PyObject* kwds) {
  PyObject* data = NULL;
  if (PyArg_ParseTuple(args, "|O", &data) <= 0)
    return 0;
  if (data == NULL)
    return edge_get_cost(self);
  edge_set_cost(self, data);
  Py_INCREF(Py_None);
  return Py_None;
}  

PyObject* edge_traverse(PyObject* self, PyObject* node_object) {
  Edge* so = ((EdgeObject*)self)->m_x;
  if (!is_NodeObject(node_object)) {
    PyErr_SetString(PyExc_TypeError, "edge: expected a node");
    return 0;
  }
  Node* node = ((NodeObject*)node_object)->m_x;
  return nodeobject_new(so->traverse(node));
}

PyObject* edge_get_from_node(PyObject* self) {
  Edge* so = ((EdgeObject*)self)->m_x;
  return nodeobject_new(so->m_from_node);
}

PyObject* edge_get_to_node(PyObject* self) {
  Edge* so = ((EdgeObject*)self)->m_x;
  return nodeobject_new(so->m_to_node);
}

PyObject* edge_get_cost(PyObject* self) {
  Edge* so = ((EdgeObject*)self)->m_x;
  return PyFloat_FromDouble(so->m_cost);
}

int edge_set_cost(PyObject* self, PyObject* cost) {
  if (!PyFloat_Check(cost)) {
    PyErr_SetString(PyExc_TypeError, "edge: expected a float");
    return -1;
  }
  Edge* so = ((EdgeObject*)self)->m_x;
  so->m_cost = PyFloat_AsDouble(cost);
  return 0;
}

PyObject* edge_get_label(PyObject* self) {
  Edge* so = ((EdgeObject*)self)->m_x;
  Py_INCREF(so->m_label);
  return so->m_label;
}

int edge_set_label(PyObject* self, PyObject* data) {
  Edge* so = ((EdgeObject*)self)->m_x;
  if (so->m_label != NULL)
    Py_DECREF(so->m_label);
  so->m_label = data;
  Py_INCREF(so->m_label);
  return 0;
}

void init_EdgeType() {
  EdgeType.ob_type = &PyType_Type;
  EdgeType.tp_name = "gamera.graph.Edge";
  EdgeType.tp_basicsize = sizeof(EdgeObject);
  EdgeType.tp_dealloc = edgeobject_dealloc;
  EdgeType.tp_repr = edge___repr__;
  EdgeType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  EdgeType.tp_getattro = PyObject_GenericGetAttr;
  EdgeType.tp_alloc = NULL; // PyType_GenericAlloc;
  EdgeType.tp_free = NULL; // _PyObject_Del;
  EdgeType.tp_call = edge___call__;
  EdgeType.tp_methods = edge_methods;
  EdgeType.tp_getset = edge_getset;
  EdgeType.tp_weaklistoffset = 0;
  PyType_Ready(&EdgeType);
}
