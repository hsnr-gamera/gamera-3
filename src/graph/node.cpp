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

#include "node.hpp"

extern "C" {
  static void nodeobject_dealloc(PyObject* self);
  static PyObject* node___call__(PyObject* self, PyObject* args, PyObject* kwds);
  static PyObject* node_get_data(PyObject* self);
  static int node_set_data(PyObject* self, PyObject* data);
  static PyObject* node_get_out_edges(PyObject* self);
  static PyObject* node_get_in_edges(PyObject* self);
}

static PyTypeObject NodeType = {
  PyObject_HEAD_INIT(NULL)
  0,
};

PyMethodDef node_methods[] = {
  { NULL }
};

PyGetSetDef node_getset[] = {
  { "data", (getter)node_get_data, (setter)node_set_data,
    "Data stored in the node (get/set)", 0 },
  { "edges", (getter)node_get_out_edges, 0,
    "Edges pointing out from node (get) (alias for out_edges)", 0 },
  { "out_edges", (getter)node_get_out_edges, 0,
    "Edges pointing out from node (get)", 0 },
  { "in_edges", (getter)node_get_in_edges, 0,
    "Edges pointing in to node (get)", 0 },
  { NULL }
};

Node* node_new(GraphObject* graph, PyObject* data) {
  Node* so = new Node();
  so->m_graph = graph;
  Py_INCREF(data);
  so->m_data = data;
  so->m_out_edges = new EdgeList();
  so->m_in_edges = new EdgeList();
  return so;
}

PyObject* nodeobject_new(Node* node) {
  NodeObject* so;
  so = (NodeObject*)(NodeType.tp_alloc(&NodeType, 0));
  so->m_x = node;
  Py_INCREF(node->m_graph);
  return (PyObject*)so;
}

PyObject* nodeobject_new(GraphObject* graph, PyObject* data) {
  NodeObject* so;
  so = (NodeObject*)(NodeType.tp_alloc(&NodeType, 0));
  so->m_x = node_new(graph, data);
  Py_INCREF(graph);
  return (PyObject*)so;
}

bool is_NodeObject(PyObject* self) {
  return PyObject_TypeCheck(self, &NodeType);
}

void node_dealloc(Node* so) {
  Py_DECREF((PyObject*)so->m_data);
  delete so->m_out_edges;
  delete so->m_in_edges;
  delete so;
}

void nodeobject_dealloc(PyObject* self) {
  NodeObject* so = (NodeObject*)self;
#ifdef DEBUG_DEALLOC
  std::cerr << "node dealloc " << PyString_AsString(PyObject_Repr(self)) << std::endl;
#endif
  Py_DECREF((PyObject*)so->m_x->m_graph);
  self->ob_type->tp_free(self);
}

PyObject* node___repr__(PyObject* self) {
  Node* so = ((NodeObject*)self)->m_x;
  return PyString_FromFormat("<Node of %s>", 
			     PyString_AsString(PyObject_Repr(so->m_data)));
}

PyObject* node___call__(PyObject* self, PyObject* args, PyObject* kwds) {
  PyObject* data = NULL;
  if (PyArg_ParseTuple(args, "|O", &data) <= 0)
    return 0;
  if (data == NULL)
    return node_get_data(self);
  node_set_data(self, data);
  Py_INCREF(Py_None);
  return Py_None;
}  

PyObject* node_get_data(PyObject* self) {
  Node* so = ((NodeObject*)self)->m_x;
  Py_INCREF(so->m_data);
  return so->m_data;
}

int node_set_data(PyObject* self, PyObject* data) {
  Node* so = ((NodeObject*)self)->m_x;
  Py_DECREF(so->m_data);
  so->m_data = data;
  Py_INCREF(so->m_data);
  return 0;
}

PyObject* node_get_out_edges(PyObject* self) {
  Node* so = ((NodeObject*)self)->m_x;
  typedef EdgeIterator<EdgeList> EdgeListIterator;
  EdgeListIterator* iterator = iterator_new<EdgeListIterator>();
  iterator->init(so->m_out_edges->begin(), so->m_out_edges->end());
  return (PyObject*)iterator;
}

PyObject* node_get_in_edges(PyObject* self) {
  Node* so = ((NodeObject*)self)->m_x;
  typedef EdgeIterator<EdgeList> EdgeListIterator;
  EdgeListIterator* iterator = iterator_new<EdgeListIterator>();
  iterator->init(so->m_in_edges->begin(), so->m_in_edges->end());
  return (PyObject*)iterator;
}

void init_NodeType() {
  NodeType.ob_type = &PyType_Type;
  NodeType.tp_name = "gamera.graph.Node";
  NodeType.tp_basicsize = sizeof(NodeObject);
  NodeType.tp_dealloc = nodeobject_dealloc;
  NodeType.tp_repr = node___repr__;
  NodeType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  NodeType.tp_getattro = PyObject_GenericGetAttr;
  NodeType.tp_alloc = PyType_GenericAlloc;
  NodeType.tp_free = _PyObject_Del;
  NodeType.tp_methods = node_methods;
  NodeType.tp_getset = node_getset;
  NodeType.tp_call = node___call__;
  PyType_Ready(&NodeType);
}
