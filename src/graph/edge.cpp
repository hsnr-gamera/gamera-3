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

extern "C" {
  static void edge_dealloc(PyObject* self);
  static PyObject* edge___repr__(PyObject* self);
  static PyObject* edge___call__(PyObject* self, PyObject* args, PyObject* kwds);
  static PyObject* edge_get_from_node(PyObject* self);
  static PyObject* edge_get_to_node(PyObject* self);
  static PyObject* edge_get_cost(PyObject* self);
  static int edge_set_cost(PyObject* self, PyObject* obj);
  static PyObject* edge_get_label(PyObject* self);
  static int edge_set_label(PyObject* self, PyObject* obj);
}
inline bool edge_check_alive(EdgeObject* edge);
static PyTypeObject EdgeType = {
  PyObject_HEAD_INIT(NULL)
  0,
};

PyMethodDef edge_methods[] = {
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

inline bool edge_check_alive(EdgeObject* so) {
  if (so->m_graph == NULL) {
    PyErr_SetString(PyExc_RuntimeError, 
		    "The underlying graph of this edge has been destroyed.");
    return false;
  }
  return true;
}

PyObject* edge_new_simple(GraphObject* graph, NodeObject* from_node,
				 NodeObject* to_node, CostType cost, PyObject* label) {
  EdgeObject* so;
  so = (EdgeObject*)EdgeType.tp_alloc(&EdgeType, 0);
  so->m_graph = graph;
  so->m_other = NULL;
  so->m_from_node = (NodeObject*)from_node;
  so->m_to_node = (NodeObject*)to_node;
  so->m_cost = cost;
  so->m_label = label;
  if (label != NULL)
    Py_INCREF(label);
  return (PyObject*)so;
}  

bool is_EdgeObject(PyObject* self) {
  return PyObject_TypeCheck(self, &EdgeType);
}

void edge_dealloc(PyObject* self) {
#if DEBUG
  std::cerr << "edge dealloc " << PyString_AsString(PyObject_Repr(self)) << std::endl;
#endif
  EdgeObject* so = (EdgeObject*)self;
  if (so->m_label != NULL)
    Py_DECREF(so->m_label);
  self->ob_type->tp_free(self);
}


PyObject* edge___repr__(PyObject* self) {
  EdgeObject* so = (EdgeObject*)self;
  if (!edge_check_alive(so))
    return 0;
  PyObject* cost = PyFloat_FromDouble(so->m_cost);
  return PyString_FromFormat("<Edge from %s to %s (%s)>", 
			     PyString_AsString(PyObject_Repr((PyObject*)so->m_from_node)),
			     PyString_AsString(PyObject_Repr((PyObject*)so->m_to_node)),
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

PyObject* edge_get_from_node(PyObject* self) {
  EdgeObject* so = (EdgeObject*)self;
  if (!edge_check_alive(so))
    return 0;
  Py_INCREF((PyObject*)so->m_from_node);
  return (PyObject*)so->m_from_node;
}

PyObject* edge_get_to_node(PyObject* self) {
  EdgeObject* so = (EdgeObject*)self;
  if (!edge_check_alive(so))
    return 0;
  Py_INCREF((PyObject*)so->m_to_node);
  return (PyObject*)so->m_to_node;
}

PyObject* edge_get_cost(PyObject* self) {
  EdgeObject* so = (EdgeObject*)self;
  if (!edge_check_alive(so))
    return 0;
  return PyFloat_FromDouble(so->m_cost);
}

int edge_set_cost(PyObject* self, PyObject* cost) {
  if (!PyFloat_Check(cost)) {
    PyErr_SetString(PyExc_TypeError, "edge: expected a float");
    return -1;
  }
  EdgeObject* so = (EdgeObject*)self;
  if (!edge_check_alive(so))
    return -1;
  so->m_cost = PyFloat_AsDouble(cost);
  return 0;
}

PyObject* edge_get_label(PyObject* self) {
  EdgeObject* so = (EdgeObject*)self;
  if (!edge_check_alive(so))
    return 0;
  Py_INCREF(so->m_label);
  return so->m_label;
}

int edge_set_label(PyObject* self, PyObject* data) {
  EdgeObject* so = (EdgeObject*)self;
  if (!edge_check_alive(so))
    return -1;
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
  EdgeType.tp_dealloc = edge_dealloc;
  EdgeType.tp_repr = edge___repr__;
  EdgeType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  EdgeType.tp_getattro = PyObject_GenericGetAttr;
  EdgeType.tp_alloc = PyType_GenericAlloc;
  EdgeType.tp_free = _PyObject_Del;
  EdgeType.tp_call = edge___call__;
  EdgeType.tp_methods = edge_methods;
  EdgeType.tp_getset = edge_getset;
  PyType_Ready(&EdgeType);
}
