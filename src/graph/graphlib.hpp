/*
 * Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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

#ifndef mgd010103_graphlib_hpp
#define mgd010103_graphlib_hpp

//#define DEBUG_DEALLOC
//#define DEBUG

#include <Python.h>
#include <iostream>
#include <set>
#include <map>
#include <queue>
#include <stack>
#include <list>
#include <vector>
#include <exception>
#include "gamera_limits.hpp"
#include "canonicpyobject.hpp"

//////////////////////////////////////////////////////////////////////////////
// Forward references
struct IteratorObject;
struct Node;
struct NodeObject;
struct Edge;
struct EdgeObject;
struct GraphObject;

typedef double CostType; // Matches a Python 'float'

//////////////////////////////////////////////////////////////////////////////
// Various container types used throughout the algorithms
typedef std::set<Node*> NodeSet;
typedef std::set<Edge*> EdgeSet;
typedef std::list<Node*> NodeList;
typedef std::list<Edge*> EdgeList;
typedef std::vector<Node*> NodeVector;
typedef std::vector<Edge*> EdgeVector;
typedef std::stack<Node*> NodeStack;
typedef std::stack<Edge*> EdgeStack;
typedef std::queue<Node*> NodeQueue;
typedef std::queue<Edge*> EdgeQueue;
//typedef std::map<PyObject*, Node*> DataToNodeMap;
typedef std::map<canonicPyObject, Node*> DataToNodeMap;
typedef std::map<Node*, Edge*> NodeToEdgeMap;

union Any {
  int Int;
  size_t SizeT;
  long Long;
  bool Bool;
  float Float;
  double Double;
  CostType Cost;
  Node* NodePtr;
  void* VoidPtr;
  NodeSet* NodeSet_;
};

struct GraphObject {
  PyObject_HEAD
  size_t m_flags;
  NodeVector* m_nodes;
  EdgeVector* m_edges;
  DataToNodeMap* m_data_to_node;
};

#define FLAG_DIRECTED 1
#define FLAG_CYCLIC 2
#define FLAG_BLOB 4
#define FLAG_MULTI_CONNECTED 8
#define FLAG_SELF_CONNECTED 16

#define FLAG_DEFAULT 0xffff
#define FLAG_FREE 31
#define FLAG_TREE 0
#define FLAG_DAG 5
#define FLAG_UNDIRECTED 14

#define HAS_FLAG(a, b) (a & b)
#define SET_FLAG(a, b) (a |= b)
#define UNSET_FLAG(a, b) (a &= ~b)

#endif
