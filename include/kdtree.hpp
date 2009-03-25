#ifndef __kdtree_HPP
#define __kdtree_HPP

#include <vector>
#include <queue>

namespace Gamera { namespace Kdtree {

typedef std::vector<double> CoordPoint;

// for passing points to the constructor of kdtree
struct kdnode {
  CoordPoint point;
  void* data;
  kdnode(const CoordPoint &p, void* d = NULL) {point = p; data = d;}
  kdnode() {data = NULL;}
};
typedef std::vector<kdnode> KdnodeVector;

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

// kdtree class
class kdtree {
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
public:
  KdnodeVector allnodes;
  size_t dimension;
  kdtree_node* root;
  // pointers to distance functions between points and coordinates
  double (*distance)(const CoordPoint &p, const CoordPoint &q);
  double (*coordinate_distance)(double x, double y);
  // distance_type can be 0 (max), 1 (city block), or 2 (euklid)
  kdtree(const KdnodeVector* nodes, int distance_type=2);
  ~kdtree();
  void k_nearest_neighbors(const CoordPoint &point, size_t k, KdnodeVector* result);
  // predefined distance functions
  static double distance0(const CoordPoint &p, const CoordPoint &q);
  static double coordinate_distance0(double x, double y);
  static double distance1(const CoordPoint &p, const CoordPoint &q);
  static double coordinate_distance1(double x, double y);
  static double distance2(const CoordPoint &p, const CoordPoint &q);
  static double coordinate_distance2(double x, double y);
};

}} // end namespace Gamera::Kdtree

#endif
