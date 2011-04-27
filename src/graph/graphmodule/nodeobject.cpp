/*
 *
 * Copyright (C) 2011 Christian Brandt
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


#include "nodeobject.hpp"
#include "iteratorobject.hpp"
#include "graph.hpp"
#ifdef __DEBUG_GAPI__
#include <iostream>
#endif


extern "C" {
   static void node_dealloc(PyObject* self);
   static PyObject* node___call__(PyObject* self, PyObject* args, PyObject* kwds);
   static PyObject* node___repr__(PyObject* self);
   static PyObject* node_get_edges(PyObject* self);
   static PyObject* node_get_nodes(PyObject* self);
   static PyObject* node_get_nedges(PyObject* self);
   static PyObject* node_get_data(PyObject* self);
}



// -----------------------------------------------------------------------------
/* Python Type Definition                                                     */
// -----------------------------------------------------------------------------
static PyTypeObject NodeType = {
   PyObject_HEAD_INIT(NULL)
   0,
};



// -----------------------------------------------------------------------------
PyMethodDef node_methods[] = {
   { NULL }
};



// -----------------------------------------------------------------------------
PyGetSetDef node_getset[] = {
   { CHAR_PTR_CAST "data", (getter)node_get_data, 0,
     CHAR_PTR_CAST "The value the identified with this node. (get/set)", 0 },
   { CHAR_PTR_CAST "edges", (getter)node_get_edges, 0,
     CHAR_PTR_CAST "An iterator over edges pointing in/out from node (get)", 0 },
   { CHAR_PTR_CAST "nodes", (getter)node_get_nodes, 0,
     CHAR_PTR_CAST "An iterator over nodes that can be reached directly "
        "(through a single edge) from this node (get)", 0 },
   { CHAR_PTR_CAST "nedges", (getter)node_get_nedges, 0,
     CHAR_PTR_CAST "The number of edges pointing in/out of this node (get)", 0 },
   { NULL }
};



// -----------------------------------------------------------------------------
void init_NodeType() {
   NodeType.ob_type = &PyType_Type;
   NodeType.tp_name = CHAR_PTR_CAST "gamera.graph.Node";
   NodeType.tp_basicsize = sizeof(NodeObject);
   NodeType.tp_dealloc = node_dealloc;
   NodeType.tp_repr = node___repr__;
   NodeType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
   NodeType.tp_getattro = PyObject_GenericGetAttr;
   NodeType.tp_alloc = NULL; // PyType_GenericAlloc;
   NodeType.tp_free = NULL; // _PyObject_Del;
   NodeType.tp_methods = node_methods;
   NodeType.tp_getset = node_getset;
   NodeType.tp_call = node___call__;
   NodeType.tp_weaklistoffset = 0;
   PyType_Ready(&NodeType);
}



// -----------------------------------------------------------------------------
/// node_new creates a new Python NodeObject by copying the given note.
PyObject* node_new(Node* n) {
#ifdef __DEBUG_GAPI__
   std::cout<<"node_new(Node* n)\n";
#endif

   if (n == NULL) {
      RETURN_VOID(); 
   }

   NodeObject* so;
   so = (NodeObject*)(NodeType.tp_alloc(&NodeType, 0));
   so->_node=n;
   so->_graph = NULL;
   return (PyObject*)so;
}



// -----------------------------------------------------------------------------
PyObject* node_deliver(Node* n, GraphObject* go) {
   if(n == NULL || go == NULL)
      return NULL;

   GraphDataPyObject* nodedata = dynamic_cast<GraphDataPyObject*>(n->_value);
   if(nodedata->_node == NULL) {
      nodedata->_node = node_new(n);
      ((NodeObject*)nodedata->_node)->_graph = go;
      Py_INCREF(go);
//      Py_INCREF(nodedata->_node);
   }
   else {
      Py_INCREF(nodedata->_node);
   }

   return nodedata->_node;
}



// -----------------------------------------------------------------------------
bool is_NodeObject(PyObject* self) {
   return PyObject_TypeCheck(self, &NodeType);
}



// -----------------------------------------------------------------------------
/* Wrapper Methods                                                            */
// -----------------------------------------------------------------------------



// -----------------------------------------------------------------------------
static void node_dealloc(PyObject* self) {
#ifdef __DEBUG_GAPI__
   std::cout<<"node_dealloc(PyObjecT* self)\n";
#endif

   INIT_SELF_NODE();
   if(so->_node) {
      GraphDataPyObject* nodedata = dynamic_cast<GraphDataPyObject*>(so->_node->_value);
      nodedata->_node = NULL;
   }
   
   if(so->_graph != NULL && is_GraphObject((PyObject*)so->_graph)) {
      Py_DECREF(so->_graph);
   }

   self->ob_type->tp_free(self);
}



// -----------------------------------------------------------------------------
static PyObject* node_get_edges(PyObject* self) {
   INIT_SELF_NODE();
   Node* n = so->_node;
   EdgePtrIterator* it = n->get_edges();

   ETIteratorObject<EdgePtrIterator>* nti = 
      iterator_new<ETIteratorObject<EdgePtrIterator> >();
   nti->init(it, so->_graph);

   return (PyObject*)nti;
}



// -----------------------------------------------------------------------------
static PyObject* node_get_nodes(PyObject* self) {
   INIT_SELF_NODE();
   Node* n = so->_node;
   NodePtrEdgeIterator* it = n->get_nodes();

   NTIteratorObject<NodePtrEdgeIterator>* nti = 
      iterator_new<NTIteratorObject<NodePtrEdgeIterator> >();
   nti->init(it, so->_graph);

   return (PyObject*)nti;
}



// -----------------------------------------------------------------------------
static PyObject* node_get_nedges(PyObject* self) {
   INIT_SELF_NODE();
   size_t nedges = so->_node->get_nedges();
   RETURN_INT(nedges); 
}



// -----------------------------------------------------------------------------
static PyObject* node_get_data(PyObject* self) {
   INIT_SELF_NODE();
   PyObject *data = dynamic_cast<GraphDataPyObject*>(so->_node->_value)->data;
   Py_INCREF(data);
   return data;
}



// -----------------------------------------------------------------------------
static PyObject* node___call__(PyObject* self, PyObject* args, PyObject* kwds) {
   return node_get_data(self);
}



// -----------------------------------------------------------------------------
static PyObject* node___repr__(PyObject* self) {
   PyObject* data = node_get_data(self);
   PyObject* repr = PyObject_Repr(data);
   Py_INCREF(repr);
   PyObject* ret = PyString_FromFormat("<Node of %s>", PyString_AsString(repr));
   Py_DECREF(repr);
   Py_DECREF(data);
   return ret;
}

