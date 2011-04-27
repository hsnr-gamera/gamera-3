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

#ifndef _GRAPH_COMMON_HPP_FCE9E0F296526B
#define _GRAPH_COMMON_HPP_FCE9E0F296526B

#include <map>
#include <list>
#include <iostream>
#include <queue>
#include <stack>
#include <set>
#include <queue>
#include <cassert>
#include <stdexcept>
#include <utility>

#include "graphdata.hpp"

namespace Gamera { namespace GraphApi {



struct Graph;
struct Edge;
struct Node;

// -----------------------------------------------------------------------------
//some data structures used in graph or multiple algorithms
typedef std::list<Node*> NodeVector;
typedef std::list<Edge*> EdgeVector;
typedef std::list<GraphData *> ValueVector;
typedef std::map<GraphData *,Node*, GraphDataPtrLessCompare> ValueNodeMap;
typedef NodeVector::iterator NodeIterator;
typedef ValueVector::iterator ValueIterator;
typedef EdgeVector::iterator EdgeIterator;
typedef std::queue<Node*> NodeQueue; //BFS
typedef std::stack<Node*> NodeStack; //DFS
typedef std::set<Node*> NodeSet;
typedef std::priority_queue<Node*> NodePriorityQueue;


//type representing edge's costs;
typedef double cost_t;

//structures used for return-values of shortest path algorithms
struct DijkstraPath {
   cost_t cost;
   std::vector<Node*> path;
};

typedef std::map<Node*, DijkstraPath> ShortestPathMap;



// -----------------------------------------------------------------------------
//Definition of different flags
typedef unsigned long flag_t;
#define FLAG_DIRECTED 1ul
#define FLAG_CYCLIC 2ul
#define FLAG_BLOB 4ul
#define FLAG_MULTI_CONNECTED 8ul
#define FLAG_SELF_CONNECTED 16ul
#define FLAG_CHECK_ON_INSERT 32ul

#define FLAG_DEFAULT (0xfffful & ~32ul)
#define FLAG_FREE 31ul
#define FLAG_TREE 0ul
#define FLAG_DAG 5ul
#define FLAG_UNDIRECTED 6ul

#define HAS_FLAG(a, b) (((a) & (b)) == (b))
#define SET_FLAG(a, b) ((a) |= (b))
#define UNSET_FLAG(a, b) ((a) &= ~(b))

#define GRAPH_HAS_FLAG(graph, f) HAS_FLAG((graph)->_flags, f)
#define GRAPH_SET_FLAG(graph, f) SET_FLAG((graph)->_flags, f)
#define GRAPH_UNSET_FLAG(graph, f) UNSET_FLAG((graph)->_flags, f)



}} // end Gamera::GraphApi
#endif /* _GRAPH_COMMON_HPP_FCE9E0F296526B */

