/*
 *
 * Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
 *               2011      Christian Brandt
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
#include "graphobject.hpp"
#include "edgeobject.hpp"



extern "C" {
  DL_EXPORT(void) initgraph(void);
}



// Factory wrapper for creating graph easier
template<flag_t F>
static PyObject* Factory(PyObject* self, PyObject* args) {
  PyObject *a = NULL;
  if (PyArg_ParseTuple(args, CHAR_PTR_CAST "|O", &a) <= 0)
    return 0;
  if (a == NULL)
    return (PyObject*)graph_new(F);
  if (is_GraphObject(a))
    return (PyObject*)graph_copy((GraphObject*)a, F);
  PyErr_SetString(PyExc_TypeError, "Invalid argument type (must be Graph)");
  return 0;
}



// defines some convenience wrappers for creating graphs easier.
PyMethodDef graph_module_methods[] = {
  { CHAR_PTR_CAST "Tree", Factory<FLAG_TREE>, METH_VARARGS,
    CHAR_PTR_CAST "Create a new Tree" },
  { CHAR_PTR_CAST "FreeGraph", Factory<FLAG_FREE>, METH_VARARGS,
    CHAR_PTR_CAST "Create a new freeform Graph" },
  { CHAR_PTR_CAST "Free", Factory<FLAG_FREE>, METH_VARARGS,
    CHAR_PTR_CAST "Create a new freeform Graph" },
  { CHAR_PTR_CAST "DAG", Factory<FLAG_DAG>, METH_VARARGS,
    CHAR_PTR_CAST "Create a new directed acyclic graph" },
  { CHAR_PTR_CAST "Undirected", Factory<FLAG_UNDIRECTED>, METH_VARARGS,
    CHAR_PTR_CAST "Create a new undirected (cyclic) graph" },
  {NULL}
};



DL_EXPORT(void) initgraph(void) {
  PyObject* m = Py_InitModule(CHAR_PTR_CAST "gamera.graph", graph_module_methods);
  PyObject* d = PyModule_GetDict(m);

  init_NodeType();
  init_EdgeType();
  init_GraphType(d);

  PyDict_SetItemString(d, "DEFAULT", PyInt_FromLong(FLAG_DEFAULT));
  PyDict_SetItemString(d, "DIRECTED", PyInt_FromLong(FLAG_DIRECTED));
  PyDict_SetItemString(d, "CYCLIC", PyInt_FromLong(FLAG_CYCLIC));
  PyDict_SetItemString(d, "BLOB", PyInt_FromLong(FLAG_BLOB));
  PyDict_SetItemString(d, "MULTI_CONNECTED", PyInt_FromLong(FLAG_MULTI_CONNECTED));
  PyDict_SetItemString(d, "SELF_CONNECTED", PyInt_FromLong(FLAG_SELF_CONNECTED));
  PyDict_SetItemString(d, "UNDIRECTED", PyInt_FromLong(FLAG_UNDIRECTED));
  PyDict_SetItemString(d, "TREE", PyInt_FromLong(FLAG_TREE));
  PyDict_SetItemString(d, "FREE", PyInt_FromLong(FLAG_FREE));
  PyDict_SetItemString(d, "FLAG_DAG", PyInt_FromLong(FLAG_DAG));
  PyDict_SetItemString(d, "CHECK_ON_INSERT", PyInt_FromLong(FLAG_CHECK_ON_INSERT));
}

