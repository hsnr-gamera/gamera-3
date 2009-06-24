#ifndef __kdtree_HPP
#define __kdtree_HPP

#include <vector>
#include <queue>

namespace Gamera { namespace Kdtree {

typedef std::vector<double> CoordPoint;
typedef std::vector<double> DoubleVector;

// for passing points to the constructor of kdtree
struct KdNode {
  CoordPoint point;
  void* data;
  KdNode(const CoordPoint &p, void* d = NULL) {point = p; data = d;}
  KdNode() {data = NULL;}
};
typedef std::vector<KdNode> KdNodeVector;

// base function object for search predicate in knn search
// returns true when the given KdNode is an admissible neighbor
// To define an own search predicate, derive from this class
// and overwrite the call operator operator()
struct KdNodePredicate {
  virtual bool operator()(const KdNode& kn) const {
    return true;
  }
};

//--------------------------------------------------------
// private helper classes used internally by KdTree
//
// the internal node structure used by kdtree
class kdtree_node {
public:
  kdtree_node();
  ~kdtree_node();
  // index of node data in kdtree array "allnodes"
  size_t dataindex;
  // cutting dimension
  size_t cutdim;
  // value of point
  //double cutval; // == point[cutdim]
  CoordPoint point;
  //  roots of the two subtrees
  kdtree_node *loson, *hison;
  // bounding rectangle of this node's subtree
  CoordPoint lobound, upbound;
};
// helper class for priority queue in k nearest neighbor search
class nn4heap {
public:
  size_t dataindex; // index of actual kdnode in *allnodes*
  double distance;  // distance of this neighbor from *point*
  nn4heap(size_t i, double d) {dataindex = i; distance = d;}
};
class compare_nn4heap {
public:
  bool operator()(const nn4heap &n, const nn4heap &m) {
    return (n.distance < m.distance);
  }
};
//--------------------------------------------------------

// kdtree class
class KdTree {
private:
  // recursive build of tree
  kdtree_node* build_tree(size_t depth, size_t a, size_t b);
  // helper variable for keeping track of subtree bounding box
  CoordPoint lobound, upbound;
  // helper variables and functions for k nearest neighbor search
  std::priority_queue<nn4heap, std::vector<nn4heap>, compare_nn4heap> *neighborheap;
  bool neighbor_search(const CoordPoint &point, kdtree_node* node, size_t k);
  bool bounds_overlap_ball(const CoordPoint &point, double dist, kdtree_node* node);
  bool ball_within_bounds(const CoordPoint &point, double dist, kdtree_node* node);
  // weights for distance computation
  DoubleVector* distweights;
  // search predicate in knn searches
  KdNodePredicate* searchpredicate;
public:
  KdNodeVector allnodes;
  size_t dimension;
  kdtree_node* root;
  // pointers to distance functions between points and coordinates
  double (*distance)(const CoordPoint &p, const CoordPoint &q, DoubleVector* weights);
  double (*coordinate_distance)(double x, double y, double weight);
  // distance_type can be 0 (max), 1 (city block), or 2 (euklid)
  KdTree(const KdNodeVector* nodes, int distance_type=2);
  ~KdTree();
  void set_distance(int distance_type, const DoubleVector* weights = NULL);
  void k_nearest_neighbors(const CoordPoint &point, size_t k, KdNodeVector* result, KdNodePredicate* pred = NULL);
  // predefined distance functions
  static double distance0(const CoordPoint &p, const CoordPoint &q, DoubleVector* weights);
  static double coordinate_distance0(double x, double y, double weight);
  static double distance1(const CoordPoint &p, const CoordPoint &q, DoubleVector* weights);
  static double coordinate_distance1(double x, double y, double weight);
  static double distance2(const CoordPoint &p, const CoordPoint &q, DoubleVector* weights);
  static double coordinate_distance2(double x, double y, double weight);
};

}} // end namespace Gamera::Kdtree

#endif
