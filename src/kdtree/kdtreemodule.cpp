/*
 *
 * Copyright (C) 2009 Christoph Dalitz
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

#include <Python.h>
#include "gameramodule.hpp"
#include "kdtree.hpp"

// these classes are used from kdtree.hpp:
//using Gamera::Kdtree::KdTree;
//using Gamera::Kdtree::KdNode;
//using Gamera::Kdtree::KdNodePredicate;
//using Gamera::Kdtree::KdNodeVector;
//using Gamera::Kdtree::CoordPoint;
//using Gamera::Kdtree::DoubleVector;


//======================================================================
// interface for KdNode class
//======================================================================

struct KdNodeObject {
  PyObject_HEAD
  PyObject* point;
  PyObject* data;
};

extern "C" {
  static PyObject* kdnode_new(PyTypeObject* pytype, PyObject* args,
			     PyObject* kwds);
  static void kdnode_dealloc(PyObject* self);
  static PyObject* kdnode_get_point(PyObject* self);
  static PyObject* kdnode_get_data(PyObject* self);
}

static PyTypeObject KdNodeType = {
  PyObject_HEAD_INIT(NULL)
  0,
};


static PyObject* kdnode_new(PyTypeObject* pytype, PyObject* args, PyObject* kwds) {
  KdNodeObject* self;
  size_t n, i;
  PyObject* point;
  PyObject* data = NULL;
  PyObject* sequence;
  PyObject* entry;
  // do some plausibility checks
  if (PyArg_ParseTuple(args, CHAR_PTR_CAST "O|O:kdnode_new", &sequence, &data) <= 0)
    return 0;
  if(!PySequence_Check(sequence)) {
    PyErr_SetString(PyExc_RuntimeError, "KdNode: given point must be sequence of numbers");
    return 0;
  }
  n = PySequence_Size(sequence);
  if (0 == n) {
    PyErr_SetString(PyExc_RuntimeError, "KdNode: given point list must not be empty");
    return 0;
  }
  point = PySequence_List(sequence);
  for(i=0;i<n;++i) {
    entry = PyList_GetItem(point,i);
    if (!PyFloat_Check(entry) && !PyInt_Check(entry)) {
      PyErr_SetString(PyExc_RuntimeError, "KdNode: given point must be list of numbers");
      Py_DECREF(point);
      return 0;
    }
  }
  // copy over properties to internal data structure
  self = (KdNodeObject*)(KdNodeType.tp_alloc(&KdNodeType, 0));
  self->point = point;
  if (data) Py_INCREF(data);
  self->data = data;
  return (PyObject*)self;
}

static void kdnode_dealloc(PyObject* self) {
  PyObject* data = ((KdNodeObject*)self)->data;
  PyObject* point = ((KdNodeObject*)self)->point;
  Py_DECREF(point);
  if (data) {
    Py_DECREF(data);
  }
  self->ob_type->tp_free(self);
}

static PyObject* kdnode_get_point(PyObject* self) {
  KdNodeObject* so = (KdNodeObject*)self;
  Py_INCREF(so->point);
  return so->point;
}

static PyObject* kdnode_get_data(PyObject* self) {
  KdNodeObject* so = (KdNodeObject*)self;
  if (so->data) {
    Py_INCREF(so->data);
      return so->data;
  } else {
    Py_INCREF(Py_None);
    return Py_None;
  }
}

PyMethodDef kdnode_methods[] = {
  { NULL }
};

PyGetSetDef kdnode_getset[] = {
  { (char *)"point", (getter)kdnode_get_point, 0,
    (char *)"geometric point of the kd-node", 0 },
  { (char *)"data", (getter)kdnode_get_data, 0,
    (char *)"data stroed with the kd-node", 0 },
  { NULL }
};

void init_KdNodeType(PyObject* d) {
  KdNodeType.ob_type = &PyType_Type;
  KdNodeType.tp_name = CHAR_PTR_CAST "gamera.kdtree.KdNode";
  KdNodeType.tp_basicsize = sizeof(KdNodeObject);
  KdNodeType.tp_dealloc = kdnode_dealloc;
  KdNodeType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  KdNodeType.tp_new = kdnode_new;
  KdNodeType.tp_getattro = PyObject_GenericGetAttr;
  KdNodeType.tp_alloc = NULL; // PyType_GenericAlloc;
  KdNodeType.tp_free = NULL; // _PyObject_Del;
  KdNodeType.tp_methods = kdnode_methods;
  KdNodeType.tp_getset = kdnode_getset;
  KdNodeType.tp_weaklistoffset = 0;
  KdNodeType.tp_doc = CHAR_PTR_CAST
    "**KdNode** (*point*, *data* = ``None``)\n\n"        \
    "The ``KdNode`` constructor creates a new node for use in a kd-tree.\n\n" \
    "*point* must not be of the Gamera data type ``Point``, but a sequence of numerical values. The optional parameter *data* can be used to store arbitrary additonal information connected to the location *point*.";
  PyType_Ready(&KdNodeType);
  PyDict_SetItemString(d, "KdNode", (PyObject*)&KdNodeType);
}


//======================================================================
// interface for KdTree class
//======================================================================

struct KdTreeObject {
  PyObject_HEAD
  size_t dimension;
  Kdtree::KdTree* tree;
  // the nodes are stored in the property kdnode.data
  // of the nodes in tree->allnodes
};

extern "C" {
  static PyObject* kdtree_new(PyTypeObject* pytype, PyObject* args,
			     PyObject* kwds);
  static void kdtree_dealloc(PyObject* self);
  static PyObject* kdtree_get_dimension(PyObject* self);
}

static PyTypeObject KdTreeType = {
  PyObject_HEAD_INIT(NULL)
  0,
};


static PyObject* kdtree_new(PyTypeObject* pytype, PyObject* args, PyObject* kwds) {
  KdTreeObject* self;
  int distance_type=2;
  size_t i,j,n,dimension;
  PyObject* list = NULL;
  PyObject *obj1,*obj2;
  Kdtree::KdNodeVector nodes4tree;
  // do some plausibility checks and extract basic properties
  if (PyArg_ParseTuple(args, CHAR_PTR_CAST "O|i:kdtree_new", &list, &distance_type) <= 0)
    return 0;
  if(!PyList_Check(list)) {
    PyErr_SetString(PyExc_RuntimeError, "KdTree: given nodes must be list of KdNode's");
    return 0;
  }
  n = PyList_Size(list);
  if (0 == n) {
    PyErr_SetString(PyExc_RuntimeError, "KdTree: Given node list must not be empty");
    return 0;
  }
  obj1 = PyList_GetItem(list,0);
  if (!PyObject_TypeCheck(obj1, &KdNodeType)) {
    PyErr_SetString(PyExc_RuntimeError, "KdTree: given nodes must be list of KdNode's");
    return 0;
  }
  // constructor of KdNode does a number of plausi checks on property point
  // => the following two lines should be guaranteed to work
  obj2 = PyObject_GetAttrString(obj1,"point");
  dimension = PyList_Size(obj2);
  Py_DECREF(obj2);
  // prepare nodes for C++ class kdtree
  Kdtree::CoordPoint p(dimension);
  for (i=0; i<n; i++) {
    obj1 = PyList_GetItem(list,i);
    if (!PyObject_TypeCheck(obj1, &KdNodeType)) {
      PyErr_SetString(PyExc_RuntimeError, "KdTree: given nodes must be list of KdNode's");
      return 0;
    }
    obj2 = PyObject_GetAttrString(obj1,"point"); // beware: INCREF
    if (PyList_Size(obj2) != (int)dimension) {
      Py_DECREF(obj2);
      PyErr_SetString(PyExc_RuntimeError, "KdTree: all node points must have same dimension");
      return 0;
    }
    for (j=0; j<dimension; j++) {
      p[j] = PyFloat_AsDouble(PyList_GetItem(obj2,j));
    }
    // we store the KdNode object in kdnode.data
    nodes4tree.push_back(Kdtree::KdNode(p,(void*)obj1));
    Py_INCREF(obj1); // node object
    Py_DECREF(obj2); // no longer needed point property
  }
  // copy over parsed stuff to data structure
  self = (KdTreeObject*)(KdTreeType.tp_alloc(&KdTreeType, 0));
  self->dimension = dimension;
  self->tree = new Kdtree::KdTree(&nodes4tree,distance_type);
  return (PyObject*)self;
}

static void kdtree_dealloc(PyObject* self) {
  size_t i;
  Kdtree::KdTree* tree = ((KdTreeObject*)self)->tree;
  for (i=0; i<tree->allnodes.size(); i++) {
    Py_DECREF((PyObject*)tree->allnodes[i].data);
  }
  delete tree;
  self->ob_type->tp_free(self);
}

static PyObject* kdtree_get_dimension(PyObject* self) {
  KdTreeObject* so = (KdTreeObject*)self;
  return PyInt_FromLong((long)(so->dimension));
}

static PyObject* kdtree_set_distance(PyObject* self, PyObject* args) {
  KdTreeObject* so = (KdTreeObject*)self;
  int distance_type;
  size_t n,i;
  PyObject* weights = NULL;
  PyObject* entry;
  if (PyArg_ParseTuple(args, CHAR_PTR_CAST "i|O", &distance_type, &weights) <= 0) {
    return 0;
  }
  Kdtree::DoubleVector wvector(so->dimension, 1.0);
  if (weights) {
    if(!PySequence_Check(weights)) {
      PyErr_SetString(PyExc_RuntimeError, "KdTree.set_distance: weights must be list of floats");
      return 0;
    }
    n = PySequence_Size(weights);
    if (n != so->dimension) {
      PyErr_SetString(PyExc_RuntimeError, "KdTree.set_distance: weight list must have length of KdTree.dimension");
      return 0;
    }
    // copy over input data
    for(i=0;i<n;++i) {
      entry = PySequence_GetItem(weights,i);
      if (PyFloat_Check(entry)) {
        wvector[i] = PyFloat_AsDouble(entry);
      } else if  (PyInt_Check(entry)) {
        wvector[i] = (double)PyInt_AsLong(entry);
      } else {
        PyErr_SetString(PyExc_RuntimeError, "KdTree.set_distance: weights must be numeric");
        Py_DECREF(entry);
        return 0;
      }
      Py_DECREF(entry);
    }
  }
  // actual C++ function call
  so->tree->set_distance(distance_type, &wvector);
  Py_INCREF(Py_None);
  return Py_None;
}

// helper class for passing over search predicate to k_nearest_neighbor
struct KdNodePredicate_Py : public Gamera::Kdtree::KdNodePredicate {
  PyObject* pyfunctor;
  KdNodePredicate_Py(PyObject* pf) {
    pyfunctor = pf;
    Py_INCREF(pf);
  }
  ~KdNodePredicate_Py() {
    Py_DECREF(pyfunctor);
  }
  bool operator()(const Gamera::Kdtree::KdNode& kn) const {
    // build python object KdNode from C++ object
    // and pass it to comparison function
    PyObject *point, *kdnode, *result, *data;
    bool retval;
    size_t i;
    //printf("KdNodePredicate_Py called\n");
    data = (PyObject*)kn.data;
    point = PyList_New(kn.point.size());
    for (i=0; i<kn.point.size(); i++) {
      PyList_SetItem(point, i, PyFloat_FromDouble(kn.point[i]));
    }
    if (data) {
      kdnode = PyObject_CallFunctionObjArgs((PyObject*)&KdNodeType,point,data,NULL);
    } else {
      kdnode = PyObject_CallFunctionObjArgs((PyObject*)&KdNodeType,point,NULL);
    }
    result = PyObject_CallFunctionObjArgs(pyfunctor,kdnode,NULL);
    retval = PyObject_IsTrue(result);
    Py_DECREF(result);
    Py_DECREF(kdnode);
    Py_DECREF(point);
    return retval;
  }
};

static PyObject* kdtree_k_nearest_neighbors(PyObject* self, PyObject* args) {
  KdTreeObject* so = (KdTreeObject*)self;
  Kdtree::CoordPoint point(so->dimension);
  PyObject *list, *entry;
  PyObject *predicate = NULL;
  int k;
  size_t i,n;
  Kdtree::KdNodeVector result;
  if (PyArg_ParseTuple(args, CHAR_PTR_CAST "Oi|O", &list, &k, &predicate) <= 0) {
    return 0;
  }
  if (predicate && !PyCallable_Check(predicate)) {
    PyErr_SetString(PyExc_RuntimeError, "KdTree.k_nearest_neighbor: search predicate must be callable");
    return 0;
  }
  if(!PySequence_Check(list)) {
    PyErr_SetString(PyExc_RuntimeError, "KdTree.k_nearest_neighbor: given point must be list or tuple of numbers");
    return 0;
  }
  n = PySequence_Size(list);
  if (n != so->dimension) {
    PyErr_SetString(PyExc_RuntimeError, "KdTree.k_nearest_neighbor: given point must have same dimension as KdTree");
    return 0;
  }
  // copy over input data
  for(i=0;i<n;++i) {
    entry = PySequence_GetItem(list,i);
    if (PyFloat_Check(entry)) {
      point[i] = PyFloat_AsDouble(entry);
    } else if  (PyInt_Check(entry)) {
      point[i] = (double)PyInt_AsLong(entry);
    } else {
      PyErr_SetString(PyExc_RuntimeError, "KdTree.k_nearest_neighbor: point coordinates must be numbers");
      Py_DECREF(entry);
      return 0;
    }
    Py_DECREF(entry);
  }
  // actual C++ function call
  if (predicate) {
    KdNodePredicate_Py searchpredicate(predicate);
    so->tree->k_nearest_neighbors(point, (size_t)k, &result, &searchpredicate);
  } else {
    so->tree->k_nearest_neighbors(point, (size_t)k, &result);
  }
  // copy over result data
  list = PyList_New(result.size());
  for (i=0; i<result.size(); i++) {
    entry = (PyObject*)result[i].data;
    Py_INCREF(entry);
    PyList_SetItem(list, i, entry);
  }
  return list;
}


PyMethodDef kdtree_methods[] = {
  { (char *)"set_distance", kdtree_set_distance, METH_VARARGS,
    (char *)"**set_distance** (*distance_type*, *weights* = ``None``)\n\nSets the distance metrics used in subsequent k nearest neighbor searches.\n\n*distance_type* can be 0 (Linfinite or maximum norm), 1 (L1 or city block norm), or 2 (L2 or euklidean norm).\n\n*weights* is a list of floating point values, where each specifies a weight for a coordinate index in the distance computation. When weights are provided, the weight list must have exactly *d* entries, where *d* is the dimension of the kdtree. When no weights are provided, all coordinates are equally weighted with 1.0." },
  { (char *)"k_nearest_neighbors", kdtree_k_nearest_neighbors, METH_VARARGS,
    (char *)"**k_nearest_neighbors** (*point*, *k*, *predicate* = ``None``)\n\nReturns the *k* nearest neighbors to the given *point* in O(log(n)) time. The parameter *point* must not be of Gamera's data type ``Point``, but a list or tuple of numbers representing the coordinates. *point* must be of the same dimension as the kd-tree.\n\nThe result is a list of nodes ordered by distance from *point*,i.e. the closest node is the first. If your query point happens to coincide with a node, you can skip it by simply removing the first entry form the result list.\n\nThe optional parameter *predicate* is a function or callable class that takes a ``KdNode`` as argument and returns ``False`` when this node shall not be among the returned neighbors." },
  { NULL }
};

PyGetSetDef kdtree_getset[] = {
  { (char *)"dimension", (getter)kdtree_get_dimension, 0,
    (char *)"Dimension of the kd-tree", 0 },
  { NULL }
};

void init_KdTreeType(PyObject* d) {
  KdTreeType.ob_type = &PyType_Type;
  KdTreeType.tp_name = CHAR_PTR_CAST "gamera.kdtree.KdTree";
  KdTreeType.tp_basicsize = sizeof(KdTreeObject);
  KdTreeType.tp_dealloc = kdtree_dealloc;
  KdTreeType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  KdTreeType.tp_new = kdtree_new;
  KdTreeType.tp_getattro = PyObject_GenericGetAttr;
  KdTreeType.tp_alloc = NULL; // PyType_GenericAlloc;
  KdTreeType.tp_free = NULL; // _PyObject_Del;
  KdTreeType.tp_methods = kdtree_methods;
  KdTreeType.tp_getset = kdtree_getset;
  KdTreeType.tp_weaklistoffset = 0;
  KdTreeType.tp_doc = CHAR_PTR_CAST
    "**KdTree** (*nodes*, *distance_type* = 2)\n\n"        \
    "The ``KdTree`` constructor creates a new kd tree in *O(n*log(n))* time from the given list of nodes.\n\n" \
    "The nodes in the list *nodes* must be of type ``KdNode``. The dimension of the tree is automatically taken from the length of *nodes[0].point*.\n\n"
    "The parameter *distance_type* specifies the distance measure that is to be used for nearest neighbor searches. It can be 0 (Linfinite or maximum norm), 1 (L1 or city block norm), or 2 (L2 or euklidean norm).";

  PyType_Ready(&KdTreeType);
  PyDict_SetItemString(d, "KdTree", (PyObject*)&KdTreeType);
}


//======================================================================
// interface for python module
//======================================================================

extern "C" {
  DL_EXPORT(void) initkdtree(void);
}

PyMethodDef kdtree_module_methods[] = {
  {NULL}
};

DL_EXPORT(void) initkdtree(void) {
  PyObject* m = Py_InitModule(CHAR_PTR_CAST "gamera.kdtree", kdtree_module_methods);
  PyObject* d = PyModule_GetDict(m);

  init_KdNodeType(d);
  init_KdTreeType(d);
}
