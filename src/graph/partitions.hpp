/*
 *
 * Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, Karl MacMillan
 *               2010      Christoph Dalitz
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

#ifndef mgd010103_partitions_hpp
#define mgd010103_partitions_hpp

#include "graphlib.hpp"
#include "graph.hpp"

extern "C" {
  PyObject* graph_optimize_partitions(PyObject* self, PyObject* args);
}

#define PARTITIONS_METHODS \
  { CHAR_PTR_CAST "optimize_partitions", graph_optimize_partitions, METH_VARARGS, \
    CHAR_PTR_CAST "**optimize_partitions** (*root_node*, *fittness_func*, *max_parts_per_group* = 5, *max_subgraph_size* = 16, criterion = \"min\")\n\n" \
    "A partition is defined as a way to divide a subgraph into groups.  This algorithm finds an optimal\n" \
    "partition according to the given fitness function.\n\n" \
    "  *root_node*\n" \
    "    The root node of the subgraph to be optimized.\n\n" \
    "  *fitness_func*\n" \
    "    A user-provided Python function, that given a partition as a nested list of groups, where each value is a node\n" \
    "    identifier, returns a floating-point score.  Higher values indicate greater fitness.\n\n" \
    "  *max_parts_per_group*\n" \
    "    Limits the number of nodes that will be placed into a single group.\n\n" \
    "  *max_subgraph_size*\n" \
    "    If the subgraph rooted at *root_node* has more than *max_subgraph_size* nodes, the partitions will not be\n" \
    "    optimized.\n\n" \
    "  *criterion*\n" \
    "    Choses the solution with the highest minimum ('min') or highest average ('avg') confidence.\n\n" \
 }, \

#endif
