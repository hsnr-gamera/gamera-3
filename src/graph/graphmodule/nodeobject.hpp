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


#ifndef _NODEOBJECT_HPP_2356F7048D8CCA
#define _NODEOBJECT_HPP_2356F7048D8CCA
#include "wrapper.hpp"
#include "node.hpp"

struct NodeObject {
   PyObject_HEAD
   Node *_node;
   GraphObject *_graph;
};

void init_NodeType();
PyObject* node_new(Node* n);
bool is_NodeObject(PyObject* self);
PyObject* node_deliver(Node* n, GraphObject* go);

#endif /* _NODEOBJECT_HPP_2356F7048D8CCA */

