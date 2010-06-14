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

#include "partitions.hpp"

#define NP_VISITED2(a) ((a)->m_node_properties[2].Bool)

// This should always be at least a 64-bit unsigned
// If compiling on a platform with a larger available native integer length,
// you might want to use that instead
#ifdef _MSC_VER
typedef unsigned __int64 Bitfield;
#define BITFIELD_SIZE 64
#else
typedef unsigned long long Bitfield;
#define BITFIELD_SIZE 64
#endif

class Part {
public:
  inline Part(Bitfield _bits, double _score) :
    bits(_bits), score(_score), begin(), end() {};
  Bitfield bits;
  double score;
  size_t begin, end;
};
typedef std::vector<Part> Parts;
typedef std::vector<Bitfield> Solution;

void print_parts(Parts& parts) {
  std::cerr << "parts =====\n";
  for (size_t i = 0; i < parts.size(); ++i) {
    std::cerr << i << " ";
    Part part = parts[i];
    for (size_t j = 0; j < BITFIELD_SIZE; ++j) {
      if ((Bitfield(1) << j) & part.bits)
	std::cerr << "*";
      else
	std::cerr << "-";
    }
    std::cerr << " " << part.begin << " " << part.end << " " << part.score << "\n";
  }
  std::cerr << "\n";
}

inline Node* graph_optimize_partitions_find_root(Node* root, NodeVector& subgraph) {
  // Find the node with the minimum edges
  NodeQueue node_queue;
  node_queue.push(root);
  size_t min_edges = std::numeric_limits<size_t>::max();
  NP_VISITED(root) = true;
  while (!node_queue.empty()) {
    Node* node;
    node = node_queue.front();
    node_queue.pop();
    subgraph.push_back(node);
    if (node->m_edges.size() < min_edges) {
      min_edges = node->m_edges.size();
      root = node;
    }
    for (EdgeList::iterator j = node->m_edges.begin();
	 j != node->m_edges.end(); ++j) {
      Node* to_node = (*j)->traverse(node);
      if (!NP_VISITED(to_node)) {
	node_queue.push(to_node);
	NP_VISITED(to_node) = true;
      }
    }
  }
  return root;
}

inline void graph_optimize_partitions_number_parts(Node* root, NodeVector& subgraph) {
  // Now visit the graph in the correct order
  NodeQueue node_queue;
  node_queue.push(root);
  NP_VISITED2(root) = true;
  size_t count = 0;
  while (!node_queue.empty()) {
    Node* node;
    node = node_queue.front();
    node_queue.pop();
    subgraph.push_back(node);
    NP_NUMBER(node) = count++;
    for (EdgeList::iterator j = node->m_edges.begin();
	 j != node->m_edges.end(); ++j) {
      Node* to_node = (*j)->traverse(node);
      if (!NP_VISITED2(to_node)) {
	node_queue.push(to_node);
	NP_VISITED2(to_node) = true;
      }
    }
  }
}

inline void graph_optimize_partitions_evaluate_parts(Node* node, const size_t max_parts_per_group,
						     const size_t subgraph_size,
						     NodeVector& node_stack,
						     Bitfield bits,
						     const PyObject* eval_func, Parts& parts) {

  size_t node_number = NP_NUMBER(node);
  node_stack.push_back(node);
  bits |= (Bitfield)1 << node_number;

  // Get the score for this part by building a Python list and
  // passing it to the (Python) evaluation function
  PyObject* result = PyList_New(node_stack.size());
  size_t j = 0;
  for (NodeVector::iterator i = node_stack.begin();
       i != node_stack.end(); ++i, ++j) {
    Py_INCREF((*i)->m_data);
    PyList_SET_ITEM(result, j, (*i)->m_data);
  }

  PyObject* tuple = Py_BuildValue(CHAR_PTR_CAST "(O)", result);
  PyObject* evalobject = PyObject_CallObject(const_cast<PyObject*>(eval_func), tuple);
  Py_DECREF(tuple);
  Py_DECREF(result);

  double eval;
  if (evalobject == NULL)
    eval = -1.0;
  else {
    if (PyFloat_Check(evalobject))
      eval = PyFloat_AsDouble(evalobject);
    else
      eval = -1.0;
    Py_DECREF(evalobject);
  }

  parts.push_back(Part(bits, eval));

  if ((node_stack.size() < max_parts_per_group) &&
      (NP_NUMBER(node) != subgraph_size - 1)) {
    for (EdgeList::iterator i = node->m_edges.begin();
	 i != node->m_edges.end(); ++i) {
      Node* to_node = (*i)->traverse(node);
      if (NP_NUMBER(to_node) > node_number)
	graph_optimize_partitions_evaluate_parts
	  (to_node, max_parts_per_group, subgraph_size,
	   node_stack, bits, eval_func, parts);
    }
  }

  node_stack.pop_back();
  return;
}

inline void graph_optimize_partitions_find_skips(Parts &parts) {
  for (size_t i = 0; i < parts.size(); ++i) {
    Part& root = parts[i];
    size_t j = i;
    for (; j < parts.size(); ++j)
      if (!(root.bits & parts[j].bits))
	break;
    root.begin = j;

    // Find the position of the left-most set bit
    Bitfield temp = root.bits;
    size_t b = 0;
    while (temp) {
      temp >>= 1;
      ++b;
    }

    // Create a mask of all bits set starting with the left-most set bit
    temp = (1 << (b + 1)) - 1;
    size_t k = j;
    for (; k < parts.size(); ++k)
      if (!(temp & parts[k].bits))
	break;
    root.end = k;
  }

  // print_parts(parts);
}

inline void graph_optimize_partitions_find_solution
 (const Parts& parts, const size_t begin, const size_t end,
  Solution& best_solution, double& best_val, Solution& partial_solution,
  double partial_val, const Bitfield bits, const Bitfield all_bits, const char* criterion) {

  double tmp_val = partial_val;

  if (bits == all_bits) {
    // when criterion = "min", partial_val contains highest minimum confidence
    // when criterion "avg", it contains the sum over all confidences
    if (0 == strcmp(criterion, "avg")) {
      tmp_val = partial_val / partial_solution.size();
    }
    if (tmp_val > best_val){
      best_val = tmp_val;
      best_solution = partial_solution; // Copy
    }      
  }

  for (size_t i = begin; i < end; ++i) {
    const Part& root = parts[i];
    if (!(root.bits & bits)) { // If this part "fits into" the current parts
      partial_solution.push_back(root.bits);
      if (0 == strcmp(criterion, "avg")) {
        tmp_val = partial_val + root.score;
      } else { // criterion == "min"
        tmp_val = std::min(partial_val, root.score);
      }
      graph_optimize_partitions_find_solution
        (parts,
         std::max(begin, root.begin), std::max(end, root.end),
         best_solution, best_val,
         partial_solution, tmp_val,
         bits | root.bits, all_bits, criterion);
      partial_solution.pop_back();
    }
  }
}

PyObject* graph_optimize_partitions(const GraphObject* so, Node* root,
                                    const PyObject* eval_func, const size_t max_parts_per_group,
                                    const size_t max_graph_size, const char* criterion = "min") {

  for (NodeVector::iterator i = so->m_nodes->begin();
       i != so->m_nodes->end(); ++i)
    NP_VISITED(*i) = NP_VISITED2(*i) = false;
  size_t size;
  {
    NodeVector subgraph;
    root = graph_optimize_partitions_find_root(root, subgraph);

    size = subgraph.size();
    // We can't do the grouping if there's more than 64 nodes,
    // so just return them all.  Also, if there's only one node,
    // just trivially return it to save time.
    if (size > BITFIELD_SIZE - 2 || size > max_graph_size || size == 1) {
      // Now, build a Python list of the solution
      PyObject* result = PyList_New(subgraph.size());
      for (size_t i = 0; i < subgraph.size(); ++i) {
        PyObject* subresult = PyList_New(1);
        Py_INCREF(subgraph[i]->m_data);
        PyList_SET_ITEM(subresult, 0, subgraph[i]->m_data);
        PyList_SET_ITEM(result, i, subresult);
      }
      return result;
    }
  }
  {
    Solution best_solution;
    NodeVector subgraph;
    subgraph.reserve(size);
    {
      Parts parts;

      graph_optimize_partitions_number_parts(root, subgraph);

      // That gives us an idea of the number of nodes in the graph,
      // now go through and find the parts

      parts.reserve(size * max_parts_per_group);
      NodeVector node_stack;
      node_stack.reserve(max_parts_per_group);
      for (NodeVector::iterator i = subgraph.begin();
	   i != subgraph.end(); ++i) {
        Bitfield bits = 0;
        graph_optimize_partitions_evaluate_parts(*i, max_parts_per_group, size,
                                                 node_stack, bits, eval_func, parts);
      }

      // Build the skip list
      graph_optimize_partitions_find_skips(parts);

      // Now, we find a solution
      Solution partial_solution;
      best_solution.reserve(size);    // Maximum size the solution can be
      partial_solution.reserve(size); // Maximum size the solution can be
      Bitfield all_bits = (Bitfield(1) << size) - 1;
      double best_val = 0;
      // partial_val carries sum (criterion "avg") or minimum ("min")
      // of confidences in subgroups => different initialization
      double partial_val_init;
      if (0 == strcmp(criterion, "avg")) {
        partial_val_init = 0.0;
      } else { // criterion == "min"
        partial_val_init = std::numeric_limits<double>::max();
      }
      graph_optimize_partitions_find_solution
        (parts, 0, (*(parts.begin())).begin,
         best_solution, best_val,
         partial_solution, partial_val_init,
         0, all_bits, criterion);
    }

    // Now, build a Python list of the solution
    PyObject* result = PyList_New(best_solution.size());
    for (size_t i = 0; i < best_solution.size(); ++i) {
      Bitfield solution_part = best_solution[i];
      size_t c = 0;
      for (size_t b=0; b < BITFIELD_SIZE; ++b) {
	if (((Bitfield)1 << b) & solution_part)
	  ++c;
      }
      // Count the set bits (Kernighan's method) so that we can allocate the
      // correct sized list from the get-go
      // (Kernighan's method does not seem to work on OS-X PPC, so I've
      // replaced it with the above)
      /*    size_t c = 0;
	    for (; solution_part; c++)
	    solution_part &= solution_part - 1; */
      PyObject* subresult = PyList_New(c);
      Bitfield k = (Bitfield)1;
      solution_part = best_solution[i];
      for (size_t j = 0, l = 0; k < solution_part; ++j, k <<= 1)
	if (solution_part & k) {
	  PyObject* data = subgraph[j]->m_data;
	  Py_INCREF(data);
	  PyList_SET_ITEM(subresult, l++, data);
	}
      PyList_SET_ITEM(result, i, subresult);
    }
    return result;
  }
}

PyObject* graph_optimize_partitions(PyObject* self, PyObject* args) {
  GraphObject* so = ((GraphObject*)self);
  PyObject* a, *eval_func;
  int max_parts_per_group = 5;
  int max_graph_size = 16;
  char* criterion = (char*)"min";
  if (PyArg_ParseTuple(args, CHAR_PTR_CAST "OO|iis:optimize_partitions", &a, &eval_func, &max_parts_per_group, &max_graph_size, &criterion) <= 0)
    return 0;
  Node* root = graph_find_node(so, (PyObject*)a);
  if (root == NULL)
    return 0;
  PyObject* result = graph_optimize_partitions(so, root, eval_func, max_parts_per_group, max_graph_size, criterion);
  return result;
}

