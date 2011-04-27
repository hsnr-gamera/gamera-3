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



#include "graphobject.hpp"
#include "graphobject_algorithm.hpp"
#include "iteratorobject.hpp"
#include "bfsdfsiterator.hpp"
#include "nodeobject.hpp"



// -----------------------------------------------------------------------------
/// Helper for converting a ShortestPathMap into a Python Dictionary
inline PyObject* pathmap_to_dict(ShortestPathMap *pathmap) {
   PyObject* pathdict = PyDict_New();
   for(ShortestPathMap::iterator it = pathmap->begin(); 
         it != pathmap->end(); it++) {

      Node* dest_node = it->first;
      DijkstraPath path = it->second;

      PyObject *pathtuple = PyTuple_New(2);
      PyObject *pathlist = PyList_New(0);
 
      PyTuple_SetItem(pathtuple, 0, PyFloat_FromDouble(path.cost));
      PyTuple_SetItem(pathtuple, 1, pathlist);


      for(std::vector<Node*>::iterator pit = path.path.begin(); 
            pit != path.path.end(); pit++) {
         Node* n = *pit;
         PyList_Append(pathlist, dynamic_cast<GraphDataPyObject*>(n->_value)->data);
      }

      PyDict_SetItem(pathdict, dynamic_cast<GraphDataPyObject*>(dest_node->_value)->data, pathtuple);
      Py_DECREF(pathtuple);

   }
   return pathdict;
}



// -----------------------------------------------------------------------------
PyObject* graph_dijkstra_shortest_path(PyObject* self, PyObject* root) {
   INIT_SELF_GRAPH();
   ShortestPathMap *pathmap;
   if(is_NodeObject(root)) 
      pathmap = so->_graph->dijkstra_shortest_path(((NodeObject*)root)->_node);
   else {
      GraphDataPyObject a(root);
      pathmap = so->_graph->dijkstra_shortest_path(&a);
   }
   PyObject* pathdict = pathmap_to_dict(pathmap);
   delete pathmap;
   return pathdict;
}



// -----------------------------------------------------------------------------
PyObject* graph_dijkstra_all_pairs_shortest_path(PyObject* self, PyObject* _) {
   INIT_SELF_GRAPH();
   std::map<Node*, ShortestPathMap*> all_pairs = 
      so->_graph->dijkstra_all_pairs_shortest_path();

   PyObject *res = PyDict_New();

   for(std::map<Node*, ShortestPathMap*>::iterator it = all_pairs.begin();
         it != all_pairs.end(); it++) {
      Node* source_node = it->first;
      ShortestPathMap* path = it->second;
      PyObject* pypath = pathmap_to_dict(path);
      PyObject* pysource = dynamic_cast<GraphDataPyObject*>(source_node->_value)->data;
      PyDict_SetItem(res, pysource, pypath);
      Py_DECREF(pypath);

      delete path;
   }

   return res;
}



// -----------------------------------------------------------------------------
PyObject* graph_all_pairs_shortest_path(PyObject* self, PyObject* _) {
   INIT_SELF_GRAPH();
   std::map<Node*, ShortestPathMap*> all_pairs = 
      so->_graph->all_pairs_shortest_path();

   PyObject *res = PyDict_New();

   for(std::map<Node*, ShortestPathMap*>::iterator it = all_pairs.begin();
         it != all_pairs.end(); it++) {
      Node* source_node = it->first;
      ShortestPathMap* path = it->second;
      PyObject* pypath = pathmap_to_dict(path);
      PyObject* pysource = dynamic_cast<GraphDataPyObject*>(source_node->_value)->data;
      PyDict_SetItem(res, pysource, pypath);
      Py_DECREF(pypath);
      delete path;
   }

   return res;
}



// -----------------------------------------------------------------------------
PyObject* graph_create_spanning_tree(PyObject* self, PyObject* pyobject) {
   INIT_SELF_GRAPH();
   Graph* g;
   if(is_NodeObject(pyobject))
      g = so->_graph->create_spanning_tree(((NodeObject*)pyobject)->_node);
   else {
      GraphDataPyObject a(pyobject);
      g = so->_graph->create_spanning_tree(&a);
   }
   if(g == NULL) {
      PyErr_SetString(PyExc_TypeError, "Graph Type does not match");
      return NULL;
   }

   return (PyObject*)graph_new(g);
}



// -----------------------------------------------------------------------------
// graph_create_minimum_spanning_tree_unique_distances
// -----------------------------------------------------------------------------

/// Sorting class for graph_create_minimum_spanning_tree_unique_distances
struct DistsSorter {
   DistsSorter(FloatImageView* image) { m_image = image; }
      
   bool operator()(const std::pair<size_t, size_t>& a,
         const std::pair<size_t, size_t>& b) {
      return m_image->get(Point(a.second, a.first)) < 
         m_image->get(Point(b.second, b.first));
   }
      
   FloatImageView* m_image;
};



// -----------------------------------------------------------------------------
PyObject* graph_create_minimum_spanning_tree_unique_distances(GraphObject* so, 
      PyObject* images, PyObject* uniq_dists) {

   PyObject* images_seq = PySequence_Fast(images, "images must be iteratable");
   if (images_seq == NULL)
      return NULL;

   static PyTypeObject* imagebase = 0;
   if (imagebase == 0) {
      PyObject* mod = PyImport_ImportModule(CHAR_PTR_CAST "gamera.gameracore");
      if (mod == 0) {
         PyErr_SetString(PyExc_RuntimeError, "Unable to load gameracore.\n");
         Py_DECREF(images_seq);
         return 0;
      }
      PyObject* dict = PyModule_GetDict(mod);
      if (dict == 0) {
         PyErr_SetString(PyExc_RuntimeError, "Unable to get module dictionary\n");
         Py_DECREF(images_seq);
         return 0;
      }
      imagebase = (PyTypeObject*)PyDict_GetItemString(dict, "Image");
   }

   // get the matrix
   if (!PyObject_TypeCheck(uniq_dists, imagebase)
         || get_pixel_type(uniq_dists) != Gamera::FLOAT) {
      PyErr_SetString(PyExc_TypeError, "uniq_dists must be a float image.");
      Py_DECREF(images_seq);
      return 0;
   }
   FloatImageView* dists = (FloatImageView*)((RectObject*)uniq_dists)->m_x;
   if (dists->nrows() != dists->ncols()) {
      PyErr_SetString(PyExc_TypeError, "image must be symmetric.");
      Py_DECREF(images_seq);
      return 0;
   }

   // get the graph ready
   so->_graph->remove_all_edges();
   GRAPH_UNSET_FLAG(so->_graph, FLAG_CYCLIC);

   // make the list for sorting
   typedef std::vector<std::pair<size_t, size_t> > index_vec_type;
   index_vec_type indexes(((dists->nrows() * dists->nrows()) - dists->nrows()) / 2);
   size_t row, col, index = 0;
   for (row = 0; row < dists->nrows(); ++row) {
      for (col = row + 1; col < dists->nrows(); ++col) {
         indexes[index].first = row;
         indexes[index++].second = col;
      }
   }
   std::sort(indexes.begin(), indexes.end(), DistsSorter(dists));

   // Add the nodes to the graph and build our map for later
   int images_len = PySequence_Fast_GET_SIZE(images_seq);
   std::vector<Node*> nodes(images_len);
   int i;
   for (i = 0; i < images_len; ++i) {
      GraphDataPyObject* obj = new GraphDataPyObject(PySequence_Fast_GET_ITEM(images_seq, i));
      nodes[i] = so->_graph->add_node_ptr(obj);
      assert(nodes[i] != NULL);
   }
   Py_DECREF(images_seq);

   // create the mst using kruskal
   i = 0;
   while (i < int(indexes.size()) && (int(so->_graph->get_nedges()) 
            < (images_len - 1))) {

      size_t row = indexes[i].first;
      size_t col = indexes[i].second;
      cost_t weight = dists->get(Point(col, row));
      so->_graph->add_edge(nodes[row], nodes[col], weight);
      ++i;
   }

   RETURN_VOID();
}



// -----------------------------------------------------------------------------
PyObject* graph_create_minimum_spanning_tree(PyObject* self, PyObject* args) {
   INIT_SELF_GRAPH();

   PyObject* images = NULL;
   PyObject* uniq_dists = NULL;
   if(PyArg_ParseTuple(args, CHAR_PTR_CAST "|OO:create_minimum_spanning_tree", 
            &images, &uniq_dists) <= 0)
      return NULL;
   
   if (images == NULL || uniq_dists == NULL) {
      Graph* g = so->_graph->create_minimum_spanning_tree(); 
      if(g == NULL) {
         PyErr_SetString(PyExc_TypeError, "Graph Type does not match");
         return NULL;
      }

      return (PyObject*)graph_new(g);

   }
   else
      return graph_create_minimum_spanning_tree_unique_distances(so, images, 
            uniq_dists);

}



// -----------------------------------------------------------------------------
PyObject* graph_BFS(PyObject* self, PyObject* root) {
   INIT_SELF_GRAPH();
   BfsIterator* it = NULL;
   if(is_NodeObject(root))
      it = so->_graph->BFS(((NodeObject*)root)->_node);
   else {
      GraphDataPyObject a(root);
      it = so->_graph->BFS(&a);
   }
   if(it == NULL) {
      PyErr_SetString(PyExc_KeyError, "starting-node not found");
      return NULL;

   }

   NTIteratorObject<BfsIterator>* nti = 
      iterator_new<NTIteratorObject<BfsIterator> >();
   nti->init(it, so);

   return (PyObject*)nti;
}



// -----------------------------------------------------------------------------
PyObject* graph_DFS(PyObject* self, PyObject* root) {
   INIT_SELF_GRAPH();
   DfsIterator* it = NULL;
   if(is_NodeObject(root))
      it = so->_graph->DFS(((NodeObject*)root)->_node);
   else {
      GraphDataPyObject a(root);
      it = so->_graph->DFS(&a);
   }
   if(it == NULL) {
      PyErr_SetString(PyExc_KeyError, "starting-node not found");
      return NULL;

   }

   NTIteratorObject<DfsIterator>* nti = 
      iterator_new<NTIteratorObject<DfsIterator> >();
   nti->init(it, so);

   return (PyObject*)nti;
}



PyObject* graph_get_color(PyObject* self, PyObject* pyobject) {
   INIT_SELF_GRAPH();
   if(is_NodeObject(pyobject)) {
      RETURN_INT(so->_graph->get_color(((NodeObject*)pyobject)->_node));
   }
   else {
      GraphDataPyObject a(pyobject);
      RETURN_INT(so->_graph->get_color(&a));
   }

}


PyObject* graph_colorize(PyObject* self, PyObject* pyobject) {
   INIT_SELF_GRAPH();
   unsigned int ncolors = PyInt_AsUnsignedLongMask(pyobject);
   try {
      so->_graph->colorize(ncolors);
      RETURN_VOID();
   }
   catch (std::runtime_error e) {
      PyErr_SetString(PyExc_RuntimeError, e.what());
      return NULL;
   }
   
}

