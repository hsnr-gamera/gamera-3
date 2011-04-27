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


#ifndef _EDGEOBJECT_HPP_EBAA99C250B47F
#define _EDGEOBJECT_HPP_EBAA99C250B47F

#include "wrapper.hpp"
#include "edge.hpp"
#include "graphobject.hpp"



/// EdgeObject wraps a Graph's EdgeObject in a Python Object
struct EdgeObject {
   PyObject_HEAD
   Edge* _edge; ///< pointer to Graph's EdgeObject
   GraphObject* _graph;
};



void init_EdgeType();
PyObject* edge_new(Edge* edge);
bool is_EdgeObject(PyObject* self);
PyObject* edge_deliver(Edge* edge, GraphObject* graph);



#endif /* _EDGEOBJECT_HPP_EBAA99C250B47F */

