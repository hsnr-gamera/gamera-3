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


#ifndef _GRAPHOBJECT_HPP_55E85D4C458276
#define _GRAPHOBJECT_HPP_55E85D4C458276

#include "wrapper.hpp"
#include "graph.hpp"
typedef std::map<Edge*, EdgeObject*> EdgeObjectMap;
// GraphObject contains a pointer to Graph;
struct GraphObject {
   PyObject_HEAD
   Graph* _graph;
   EdgeObjectMap *assigned_edgeobjects;
}; 

void init_GraphType(PyObject* dict);
bool is_GraphObject(PyObject* self);
GraphObject* graph_new(flag_t flags = FLAG_DEFAULT);
GraphObject* graph_new(Graph* g);
GraphObject* graph_copy(GraphObject* so, flag_t flags = FLAG_DEFAULT);



#endif /* _GRAPHOBJECT_HPP_55E85D4C458276 */

