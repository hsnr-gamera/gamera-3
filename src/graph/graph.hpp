/*
 *
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

#ifndef mgd010103_graph_hpp
#define mgd010103_graph_hpp

#include "graphlib.hpp"
#include "node.hpp"
#include "edge.hpp"
#include "iterator.hpp"

#include "search.hpp"
#include "spanning_tree.hpp"
#include "shortest_path.hpp"
#include "partitions.hpp"

void init_GraphType(PyObject* dict);
bool is_GraphObject(PyObject* self);
GraphObject* graph_new(size_t flags = FLAG_DEFAULT);
GraphObject* graph_copy(GraphObject* so, size_t flags = FLAG_DEFAULT);
void graph_make_directed(GraphObject* so);
void graph_make_undirected(GraphObject* so);
void graph_make_cyclic(GraphObject* so);
void graph_make_acyclic(GraphObject* so);
void graph_make_blob(GraphObject* so);
void graph_make_tree(GraphObject* so);
void graph_make_multi_connected(GraphObject* so);
void graph_make_singly_connected(GraphObject* so, bool maximum_cost = true);
void graph_make_self_connected(GraphObject* so);
void graph_make_not_self_connected(GraphObject* so);
size_t graph_size_of_subgraph(GraphObject* so, Node* node);

inline Node* graph_find_node(GraphObject* so, PyObject* pyobject,
				   bool exception = true);
inline Node* graph_add_node(GraphObject* self, Node* node);
inline Node* graph_add_node(GraphObject* self, PyObject* pyobject);
inline bool graph_remove_node_and_edges(GraphObject* so, Node* node);
inline bool graph_remove_node(GraphObject* so, Node* node);
inline size_t graph_disj_set_find_and_compress(GraphObject* so, size_t x);
inline void graph_disj_set_union_by_height(GraphObject* so, size_t a, size_t b);
inline Edge* graph_add_edge(GraphObject* so, Node* from_node,
			   Node* to_node, CostType cost = 1.0, 
			   PyObject* label = NULL);
inline bool graph_add_edge(GraphObject* so, PyObject* from_object,
			   PyObject* to_object, CostType cost = 1.0,
			   PyObject* label = NULL);
inline bool graph_remove_edge(GraphObject* so, Edge* edge);
inline bool graph_remove_edge(GraphObject* so, Node* from_node,
			      Node* to_node);
inline void graph_remove_all_edges(GraphObject* so);
inline bool graph_has_node(GraphObject* so, Node* node);
inline size_t graph_has_edge(GraphObject* so, Node* from_node,
			     Node* to_node);

inline Node* graph_find_node(GraphObject* so, PyObject* pyobject, bool exception) {
  // Find the node
  Node* node;
  if (is_NodeObject(pyobject))
    return (Node*)((NodeObject*)pyobject)->m_x;
  DataToNodeMap::iterator i = so->m_data_to_node->find(pyobject);
  if (i != so->m_data_to_node->end()) {
    node = i->second;
    return node;
  }
  if (exception)
    PyErr_SetString(PyExc_TypeError, 
		    PyString_AsString
		    (PyString_FromFormat
		     ("Node containing %s is not in the graph", 
		      PyString_AsString(PyObject_Repr(pyobject)))));
  return 0;
}

inline Node* graph_add_node(GraphObject* so, Node* node) {
  so->m_nodes->push_back(node);
  node->m_set_id = so->m_nodes->size();
  node->m_disj_set = 0;
  (*(so->m_data_to_node))[node->m_data] = node;
  node->m_is_subgraph_root = true;
  return node;
}

inline Node* graph_add_node(GraphObject* so, PyObject* pyobject) {
  Node* node = graph_find_node(so, pyobject, false);
  if (node == 0) {
    node = new Node(so, pyobject);
    return graph_add_node(so, node);
  }
  return node;
}

inline bool graph_remove_node_and_edges(GraphObject* so, Node* node) {
  // Remove all edges coming out of that node
  // Funny looping construct is so we can remove an edge and not
  // invalid the iterator

  for (EdgeList::iterator i, j = node->m_edges.begin(); 
       j != node->m_edges.end(); ) {
    i = j++;
    graph_remove_edge(so, *i);
  }  

  if (HAS_FLAG(so->m_flags, FLAG_DIRECTED)) {
    // Remove all edges pointing into that node
    // Funny looping construct is so we can remove an edge and not
    // invalid the iterator
    for (NodeVector::iterator i = so->m_nodes->begin();
	 i != so->m_nodes->end(); ++i) {
      if (*i != node) {
	for (EdgeList::iterator j, k = (*i)->m_edges.begin(); 
	     k != (*i)->m_edges.end(); ) {
	  j = k++;
	  if ((*j)->m_to_node == node)
	    graph_remove_edge(so, *j);
	}
      }  
    }
  }

  // Adjust the disjoint set
  // Fix all the pointers in the nodes themselves
  if (!HAS_FLAG(so->m_flags, FLAG_DIRECTED) ||
      !HAS_FLAG(so->m_flags, FLAG_CYCLIC)) {
    // LongVector* vec = (so->m_disj_set);
    size_t set_id = node->m_set_id;
    // Adjust all m_set_id's
    for (NodeVector::iterator i = so->m_nodes->begin();
	 i != so->m_nodes->end(); ++i) {
      if ((*i)->m_set_id > set_id)
	(*i)->m_set_id--;
      if ((*i)->m_disj_set > (long)set_id)
	(*i)->m_disj_set--;
    }
  }

  // Actually remove the node
  so->m_data_to_node->erase(node->m_data);
  node->m_is_subgraph_root = false;

  for (NodeVector::iterator i = so->m_nodes->begin();
       i != so->m_nodes->end(); ++i)
    if ((*i) == node) {
      so->m_nodes->erase(i);
      break;
    }

  delete node;
  return true;
}

inline bool graph_remove_node(GraphObject* so, Node* node) {
  // Stitch together edges
  for (NodeVector::iterator i = so->m_nodes->begin();
       i != so->m_nodes->end(); ++i)
    for (EdgeList::iterator j = (*i)->m_edges.begin();
	 j != (*i)->m_edges.end(); ++j)
      if ((*j)->traverse(*i) == node)
	for (EdgeList::iterator k = node->m_edges.begin();
	     k != node->m_edges.end(); ++k) 
	  graph_add_edge(so, *i, (*k)->m_to_node, 
			 (*j)->m_cost + (*k)->m_cost);
  
  // Remove node and original edges
  return graph_remove_node_and_edges(so, node);
}

inline size_t graph_disj_set_find_and_compress(GraphObject* so, size_t x) {
  //LongVector* vec = (so->m_disj_set);
  size_t xm1 = x - 1;
  Node* node = (*(so->m_nodes))[xm1];
  long disj_set = node->m_disj_set;
  if (disj_set == (long)x)
    node->m_disj_set = x;
  if (disj_set <= 0)
    return node->m_set_id;
  return node->m_disj_set = graph_disj_set_find_and_compress(so, disj_set);
}

inline void graph_disj_set_union_by_height(GraphObject* so, size_t a, size_t b) {
  //LongVector* vec = (so->m_disj_set);
  Node* node_a = (*(so->m_nodes))[a-1];
  Node* node_b = (*(so->m_nodes))[b-1];
  if (node_b->m_disj_set < node_a->m_disj_set) {
    node_a->m_disj_set = b;
  } else {
    if (node_a->m_disj_set == node_b->m_disj_set)
      node_a->m_disj_set--;
    node_b->m_disj_set = a;
  }
}

// WARNING: Internal function: DO NOT CALL DIRECTLY
inline Edge* graph_add_edge(GraphObject* so, Node* from_node,
			    Node* to_node, CostType cost,
			    PyObject* label) {
  if (!HAS_FLAG(so->m_flags, FLAG_SELF_CONNECTED) && from_node == to_node)
    return false;
  
  /* if (!HAS_FLAG(so->m_flags, FLAG_MULTI_CONNECTED)) {
    for (EdgeList::iterator i = from_node->m_edges.begin();
	 i != from_node->m_edges.end(); ++i)
      if ((*i)->traverse(from_node) == to_node)
	return false;
  } */

#ifdef DEBUG_ADT
  for (NodeVector::iterator i = so->m_nodes->begin();
       i != so->m_nodes->end(); ++i)
    std::cerr << (*i)->m_disj_set << " ";
    std::cerr << " ";
#endif  

  bool found_cycle = false;
  bool possible_cycle = true;
  if (!HAS_FLAG(so->m_flags, FLAG_DIRECTED) ||
      !HAS_FLAG(so->m_flags, FLAG_CYCLIC)) {
    size_t to_set_id = graph_disj_set_find_and_compress
      (so, to_node->m_set_id);
    size_t from_set_id = graph_disj_set_find_and_compress
      (so, from_node->m_set_id);
    if (from_set_id != to_set_id) {
      possible_cycle = false;
      graph_disj_set_union_by_height(so, to_set_id, from_set_id);
    }
  }

  if (possible_cycle) {
    if (!HAS_FLAG(so->m_flags, FLAG_DIRECTED) && !HAS_FLAG(so->m_flags, FLAG_CYCLIC))
      return NULL;
    else {
      if (!HAS_FLAG(so->m_flags, FLAG_CYCLIC) || 
	  (HAS_FLAG(so->m_flags, FLAG_DIRECTED) && 
	   to_node->m_is_subgraph_root)) {
	DFSIterator* iterator = iterator_new<DFSIterator>();
	iterator->init(so, to_node);
	// Skip the first node, since we know what it is
	Node* node = (Node*)DFSIterator::next_node(iterator);
	while ((node = (Node*)DFSIterator::next_node(iterator)))
	  if (node == from_node) {
	    found_cycle = true;
	    break;
	  }
      }
      if (!HAS_FLAG(so->m_flags, FLAG_CYCLIC) && found_cycle)
	return NULL;
    }
  }

  Edge* edge = new Edge(so, from_node, to_node, cost, label);
  so->m_edges->push_back(edge);
  from_node->m_edges.push_back(edge);
  if (!HAS_FLAG(so->m_flags, FLAG_DIRECTED))
    to_node->m_edges.push_back(edge);
  if (!found_cycle)
    to_node->m_is_subgraph_root = false;

#ifdef DEBUG_ADT
  std::cerr << "After";
  for (NodeVector::iterator i = so->m_nodes->begin();
       i != so->m_nodes->end(); ++i)
    std::cerr << (*i)->m_disj_set << " ";
    std::cerr << " ";
#endif

  return edge;
}

inline bool graph_add_edge(GraphObject* so, PyObject* from_pyobject,
			   PyObject* to_pyobject, CostType cost, PyObject* label) {
  Node* from_node, *to_node;
  from_node = graph_add_node(so, from_pyobject);
  to_node = graph_add_node(so, to_pyobject);
  return graph_add_edge(so, from_node, to_node, cost) != NULL;
}

// WARNING: This is an internal function that assumes the edge already exists
inline bool graph_remove_edge(GraphObject* so, Edge* edge) {
  Node* from_node = edge->m_from_node;
  Node* to_node = edge->m_to_node;
  if ((!HAS_FLAG(so->m_flags, FLAG_DIRECTED) || !HAS_FLAG(so->m_flags, FLAG_CYCLIC))) {
    // O(nm)  (m: avg # edges per node)
    // Deal with subgraph roots and disjoint sets
    // I'd love to find a faster way to do this, but...
    if (to_node != from_node) {
      for (NodeVector::iterator i = so->m_nodes->begin();
	   i != so->m_nodes->end(); ++i)
	NP_VISITED(*i) = false;
      to_node->m_disj_set = 0;
      from_node->m_disj_set = 0;
      NodeStack node_stack;
      node_stack.push(to_node);
      node_stack.push(from_node);
      while (!node_stack.empty()) {
	Node* root = node_stack.top();
	node_stack.pop();
	if (!NP_VISITED(root)) {
	  NP_VISITED(root) = true;
	  size_t new_set_id = root->m_set_id;
	  for (EdgeList::iterator j = root->m_edges.begin();
	       j != root->m_edges.end(); ++j) {
	    Node* node = (*j)->traverse(root);
	    if (!(NP_VISITED(node))) {
	      NP_VISITED(node) = true;
	      node->m_disj_set = new_set_id;
	      node_stack.push(node);
	    }
	  }
	}
      }
    }
  }

  if (from_node->m_is_subgraph_root && HAS_FLAG(so->m_flags, FLAG_DIRECTED)) {
    DFSIterator* iterator = iterator_new<DFSIterator>();
    iterator->init(so, to_node);
    Node* node = (Node*)DFSIterator::next_node(iterator);
    while ((node = (Node*)DFSIterator::next_node(iterator))) {
      if (node == from_node) {
	to_node->m_is_subgraph_root = true;
	from_node->m_is_subgraph_root = false;
	break;
      }
    }
  }

  from_node->m_edges.remove(edge);
  if (!HAS_FLAG(so->m_flags, FLAG_DIRECTED))
    to_node->m_edges.remove(edge);

  for (EdgeVector::iterator i = so->m_edges->begin();
       i != so->m_edges->end(); ++i)
    if ((*i) == edge) {
      so->m_edges->erase(i);
      break;
    }
  delete edge;
  return true;
}

// WARNING: This is an internal function that assumes the nodes (but not necessarily
//          the edge) already exist
inline bool graph_remove_edge(GraphObject* so, Node* from_node, Node* to_node) {
  bool found_any = false;
  // Funny looping construct is so we can remove an edge and not
  // invalid the iterator
  for(EdgeList::iterator i, j = from_node->m_edges.begin();
      j != from_node->m_edges.end(); ) {
    i = j++;
    if ((*i)->traverse(from_node) == to_node) {
      graph_remove_edge(so, *i);
      found_any = true;
    }
  }

  if (!found_any) {
    PyErr_SetString(PyExc_TypeError, "There is no edge connecting these nodes");
    return false;
  }
  return true;
}

inline void graph_remove_all_edges(GraphObject* so) {
  for (NodeVector::iterator i = so->m_nodes->begin();
       i != so->m_nodes->end(); ++i) {
    (*i)->m_edges.clear();
    // (*i)->m_in_edges->clear();
    (*i)->m_is_subgraph_root = true;
    (*i)->m_disj_set = 0;
  }
  for (EdgeVector::iterator i = so->m_edges->begin();
       i != so->m_edges->end(); ++i)
    delete *i;
  so->m_edges->clear();
}

inline bool graph_has_node(GraphObject* so, Node* node) {
  for (NodeVector::iterator i = so->m_nodes->begin();
       i != so->m_nodes->end(); ++i)
    if ((*i) == node)
      return true;
  return false;
}

inline size_t graph_has_edge(GraphObject* so, Node* from_node, Node* to_node) {
  if (!graph_has_node(so, to_node)) {
    return 0;
  }
  if (!graph_has_node(so, from_node)) {
    return 0;
  }
  size_t count = 0;
  for (EdgeList::iterator i = from_node->m_edges.begin();
       i != from_node->m_edges.end(); ++i)
    if ((*i)->traverse(from_node) == to_node)
      count++;
  return count;
}

#endif
