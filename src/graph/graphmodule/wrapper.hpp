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


#ifndef _WRAPPER_HPP_940FFAA9288997
#define _WRAPPER_HPP_940FFAA9288997

////////////////#define __DEBUG_GAPI__

#include <Python.h>
#include "gameramodule.hpp"
//##include "graphdata_pyobject.hpp"

#ifdef __DEBUG_GAPI__
#include <iostream>
#endif

#ifdef CHAR_PTR_CAST
#undef CHAR_PTR_CAST
#endif

#define CHAR_PTR_CAST (char*)
#include "graph_common.hpp"
#include "graphdatapyobject.hpp"
using namespace Gamera::GraphApi;
class GraphObject;
class EdgeObject;
class NodeObject;



// -----------------------------------------------------------------------------
// some wrappers for easier handling of self-parameters and return values
#define INIT_SELF_GRAPH() GraphObject* so = ((GraphObject*)self)
#define INIT_SELF_EDGE() EdgeObject* so = ((EdgeObject*)self)
#define INIT_SELF_NODE() NodeObject* so = ((NodeObject*)self)
#define RETURN_BOOL(a) {PyObject *_ret_ = PyBool_FromLong((long)(a)); return _ret_;}
#define RETURN_INT(a) {return PyInt_FromLong((long)(a));}
#define RETURN_VOID() {PyObject *_ret_ = Py_None; Py_INCREF(_ret_); return _ret_;}
#define RETURN_DOUBLE(a) {return PyFloat_FromDouble((a));}

#endif /* _WRAPPER_HPP_940FFAA9288997 */

