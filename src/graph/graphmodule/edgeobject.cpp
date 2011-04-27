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


#include "edgeobject.hpp"
#include "nodeobject.hpp"



extern "C" {
   static void edge_dealloc(PyObject* self);
   static PyObject* edge_traverse(PyObject* self, PyObject* pyobject);

   static PyObject* edge___call__(PyObject* self, PyObject* args, PyObject* kwds);
   static PyObject* edge___repr__(PyObject* self);
   static PyObject* edge_get_from_node(PyObject* self);   
   static PyObject* edge_get_to_node(PyObject* self);
   static PyObject* edge_get_cost(PyObject* self);
   static int edge_set_cost(PyObject* self, PyObject* object);
   static PyObject* edge_get_label(PyObject* self);
   static int edge_set_label(PyObject* self, PyObject* object);
}



// -----------------------------------------------------------------------------
/* Python Type Definition                                                    */
// -----------------------------------------------------------------------------
static PyTypeObject EdgeType = {
   PyObject_HEAD_INIT(NULL)
   0,
};



// -----------------------------------------------------------------------------
PyMethodDef edge_methods[] = {
   { CHAR_PTR_CAST "traverse", edge_traverse, METH_O, 
      CHAR_PTR_CAST "**traverse** (*node*)\n\n"
         "Get the other node in an edge"},
   {NULL}
};



// -----------------------------------------------------------------------------
PyGetSetDef edge_getset[] = {
   { CHAR_PTR_CAST "from_node", (getter)edge_get_from_node, 0,
      CHAR_PTR_CAST "node this edge starts from (get)", 0},
   { CHAR_PTR_CAST "to_node", (getter)edge_get_to_node, 0,
      CHAR_PTR_CAST "node this edge points to (get)", 0},
   { CHAR_PTR_CAST "cost", (getter)edge_get_cost, (setter)edge_set_cost,
      CHAR_PTR_CAST "cost assigned to this edge (get/set)", 0},
   { CHAR_PTR_CAST "label", (getter)edge_get_label, (setter)edge_set_label,
      CHAR_PTR_CAST "label assigned to this edge (get/set)", 0},
   { NULL }
};



// -----------------------------------------------------------------------------
void init_EdgeType() {
   EdgeType.ob_type = &PyType_Type;
   EdgeType.tp_name = CHAR_PTR_CAST "gamera.graph.Edge";
   EdgeType.tp_basicsize = sizeof(EdgeObject);
   EdgeType.tp_dealloc = edge_dealloc;
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



// -----------------------------------------------------------------------------
/* Wrapper Methods                                                           */
// -----------------------------------------------------------------------------
/** edge_new creates a new Python EdgeObject. Note that this edge gets invalid 
 * when it is removed from the graph. This could lead to undefined behaviour.
 * A possibly future solution could be tracking every EdgeObject and throwing
 * exceptions if they are invalidated.
 * */
PyObject* edge_new(Edge* edge) {
   EdgeObject* so;
   so = (EdgeObject*)EdgeType.tp_alloc(&EdgeType, 0);
   so->_edge = edge;
   so->_graph = NULL;
   return (PyObject*)so;
}



// -----------------------------------------------------------------------------
PyObject* edge_deliver(Edge* edge, GraphObject* graph) {
#ifdef __DEBUG_GAPI__
   std::cerr << "edge: " << edge << "\nGraphObject: " << graph << std::endl;
   std::cerr << "edgenodes: " << edge->from_node->_value << 
      edge->to_node->_value << std::endl;
#endif

   if(graph == NULL || edge == NULL)
      return NULL;

#ifdef __DEBUG_GAPI__
   std::cerr << "size: " << graph->assigned_edgeobjects->size() << std::endl;
#endif

   if(graph->assigned_edgeobjects->find(edge) == graph->assigned_edgeobjects->end()) {
      EdgeObject* so = (EdgeObject*)edge_new(edge);
      if(graph && is_GraphObject((PyObject*)graph)) {
         Py_INCREF(graph);
         so->_graph = graph;
         graph->assigned_edgeobjects->insert(std::make_pair(edge, so));
      }
      return (PyObject*)so;
   }
   
   EdgeObject* so = (*graph->assigned_edgeobjects)[edge];
   Py_INCREF(so);
   return (PyObject*)so;
}



// -----------------------------------------------------------------------------
bool is_EdgeObject(PyObject* self) {
   return PyObject_TypeCheck(self, &EdgeType);
}



// -----------------------------------------------------------------------------
static void edge_dealloc(PyObject* self) {
   INIT_SELF_EDGE();
   if(so->_graph) {
      so->_graph->assigned_edgeobjects->erase(so->_edge);
      Py_DECREF(so->_graph);
      so->_graph = NULL;
   } 
   self->ob_type->tp_free(self);
}
 


// -----------------------------------------------------------------------------
static PyObject* edge_traverse(PyObject* self, PyObject* pyobject) {
   INIT_SELF_EDGE();
   Node* other_node = NULL;
   if(is_NodeObject(pyobject)) {
      other_node = so->_edge->traverse(((NodeObject*)pyobject)->_node->_value);
   }
   else {
      GraphDataPyObject a(pyobject);
      other_node = so->_edge->traverse(&a);
   }

   return node_new(other_node);
}

 


// -----------------------------------------------------------------------------
static PyObject* edge___call__(PyObject* self, PyObject* args, PyObject* kwds) {
   PyObject* data = NULL;
   if(PyArg_ParseTuple(args, CHAR_PTR_CAST "|O:Edge.__call__", &data) <= 0)
      return NULL;
   if (data == NULL)
      return edge_get_cost(self);

   edge_set_cost(self, data);
   RETURN_VOID();
}
 


// -----------------------------------------------------------------------------
static PyObject* edge___repr__(PyObject* self) {
   INIT_SELF_EDGE();
   PyObject* from_data = 
      dynamic_cast<GraphDataPyObject*>(so->_edge->from_node->_value)->data;

   PyObject* to_data =  
      dynamic_cast<GraphDataPyObject*>(so->_edge->to_node->_value)->data;

   PyObject* weight = PyFloat_FromDouble(so->_edge->weight);
   Py_INCREF(from_data);
   Py_INCREF(to_data);
   Py_INCREF(weight);
   char* a = PyString_AsString(PyObject_Repr(from_data));
   char* b = PyString_AsString(PyObject_Repr(to_data));
   char* c = PyString_AsString(PyObject_Repr(weight));

   PyObject* ret = PyString_FromFormat("<Edge from %s to %s (%s)>", a,b,c);
   return ret;
}
 


// -----------------------------------------------------------------------------
static PyObject* edge_get_from_node(PyObject* self) {
   INIT_SELF_EDGE();
   return node_deliver(so->_edge->from_node, so->_graph);
}
 


// -----------------------------------------------------------------------------
static PyObject* edge_get_to_node(PyObject* self) {
   INIT_SELF_EDGE();
   return node_deliver(so->_edge->to_node, so->_graph);
}
 


// -----------------------------------------------------------------------------
static PyObject* edge_get_cost(PyObject* self) {
   INIT_SELF_EDGE();
   RETURN_DOUBLE(so->_edge->weight); 
}
 


// -----------------------------------------------------------------------------
static int edge_set_cost(PyObject* self, PyObject* object) {
   if(!PyFloat_Check(object)) {
      PyErr_SetString(PyExc_TypeError, "edge: expected a float");
      return -1;
   }
   INIT_SELF_EDGE();

   so->_edge->weight = PyFloat_AsDouble(object);
   return 0;
}
 


// -----------------------------------------------------------------------------
static PyObject* edge_get_label(PyObject* self) {
   INIT_SELF_EDGE();
   if(so->_edge->label) {
      Py_INCREF((PyObject*)so->_edge->label);
      return (PyObject*)so->_edge->label;
   }
   else {
      RETURN_VOID();
   }
}
 


// -----------------------------------------------------------------------------
static int edge_set_label(PyObject* self, PyObject* object) {
   INIT_SELF_EDGE();
   if(so->_edge->label) {
      Py_DECREF((PyObject*)so->_edge->label);
   }
   so->_edge->label = (void*)object;
   Py_INCREF(object);
   return 0;
}

