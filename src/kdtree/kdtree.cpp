
#include "kdtree.hpp"
#include <algorithm>
#include <stdexcept>
#include <math.h>

namespace Gamera { namespace Kdtree {

//--------------------------------------------------------------
// function object for comparing only dimension d of two vecotrs
//--------------------------------------------------------------
class compare_dimension {
public:
  compare_dimension(size_t dim) { d=dim; }
  bool operator()(const KdNode &p, const KdNode &q) {
    return (p.point[d] < q.point[d]);
  }
  size_t d;
};

//--------------------------------------------------------------
// internal node structure used by kdtree
//--------------------------------------------------------------
kdtree_node::kdtree_node()
{
  dataindex = cutdim = 0;
  loson = hison = (kdtree_node*)NULL;
}
kdtree_node::~kdtree_node()
{
  if (loson) delete loson;
  if (hison) delete hison;
}

//--------------------------------------------------------------
// destructor and constructor of kdtree
//--------------------------------------------------------------
KdTree::~KdTree()
{
  if (root) delete root;
  delete distweights;
}
// distance_type can be 0 (Maximum), 1 (Manhatten), or 2 (Euklidean)
KdTree::KdTree(const KdNodeVector* nodes, int distance_type /*=2*/)
{
  size_t i,j;
  double val;
  // copy over input data
  dimension = nodes->begin()->point.size();
  allnodes = *nodes;
  // initialize distance values
  distweights = new DoubleVector(dimension, 1.0);
  set_distance(distance_type);
  // compute global bounding box
  lobound = nodes->begin()->point;
  upbound = nodes->begin()->point;
  for (i=1; i<nodes->size(); i++) {
    for (j=0; j<dimension; j++) {
      val = allnodes[i].point[j];
      if (lobound[j] > val) lobound[j] = val;
      if (upbound[j] < val) upbound[j] = val;        
    }
  }
  // build tree recursively
  root = build_tree(0,0,allnodes.size());
}

// distance_type can be 0 (Maximum), 1 (Manhatten), or 2 (Euklidean)
void KdTree::set_distance(int distance_type, const DoubleVector* weights /*=NULL*/)
{
  size_t i;
  if (distance_type == 0) {
    distance = distance0;
    coordinate_distance = coordinate_distance0;
  } else if (distance_type == 1) {
    distance = distance1;
    coordinate_distance = coordinate_distance1;   
  } else {
    distance = distance2;
    coordinate_distance = coordinate_distance2;
  }
  for (i=0; i<dimension; i++) {
    if (weights)
      (*distweights)[i] = (*weights)[i];
    else
      (*distweights)[i] = 1.0;
  }
}

//--------------------------------------------------------------
// recursive build of tree
// "a" and "b"-1 are the lower and upper indices
// from "allnodes" from which the subtree is to be built
//--------------------------------------------------------------
kdtree_node* KdTree::build_tree(size_t depth, size_t a, size_t b)
{
  size_t m;
  double temp, cutval;
  kdtree_node* node = new kdtree_node();
  node->lobound = lobound;
  node->upbound = upbound;
  node->cutdim = depth % dimension;
  if (b-a <= 1) {
    node->dataindex = a;
    node->point = allnodes[a].point;
  } else {
    m = (a+b)/2;
    std::nth_element(allnodes.begin()+a, allnodes.begin()+m,
                     allnodes.begin()+b, compare_dimension(node->cutdim));
    node->point = allnodes[m].point;
    cutval = allnodes[m].point[node->cutdim];
    node->dataindex = m;
    if (m-a>0) {
      temp = upbound[node->cutdim];
      upbound[node->cutdim] = cutval;
      node->loson = build_tree(depth+1,a,m);
      upbound[node->cutdim] = temp;
    }
    if (b-m>1) {
      temp = lobound[node->cutdim];
      lobound[node->cutdim] = cutval;
      node->hison = build_tree(depth+1,m+1,b);
      lobound[node->cutdim] = temp;
    }
  }
  return node;
}

//--------------------------------------------------------------
// k nearest neighbor search
// returns the *k* nearest neighbors of *point* in O(log(n)) 
// time. The result is returned in *result* and is sorted by
// distance from *point*.
// The optional search predicate is a callable class (aka "functor")
// derived from KdNodePredicate. When Null (default, no search
// predicate is applied).
//--------------------------------------------------------------
    void KdTree::k_nearest_neighbors(const CoordPoint &point, size_t k, KdNodeVector* result, KdNodePredicate* pred /*=NULL*/)
{
  size_t i;
  KdNode temp;
  searchpredicate = pred;

  result->clear();
  if (k<1) return;
  if (point.size() != dimension)
    throw std::invalid_argument("kdtree::k_nearest_neighbors(): point must be of same dimension as kdtree");


  // collect result of k values in neighborheap
  neighborheap = new std::priority_queue<nn4heap, std::vector<nn4heap>, compare_nn4heap>();
  if (k>allnodes.size()) {
    // when more neighbors asked than nodes in tree, return everything
    k = allnodes.size();
    for (i=0; i<k; i++) {
      if (!(searchpredicate && !(*searchpredicate)(allnodes[i])))
        neighborheap->push(nn4heap(i,distance(allnodes[i].point,point,distweights)));
    }
  } else {
    neighbor_search(point, root, k);
  }

  // copy over result sorted by distance
  // (we must revert the vector for ascending order)
  while (!neighborheap->empty()) {
    i = neighborheap->top().dataindex;
    neighborheap->pop();
    result->push_back(allnodes[i]);
  }
  // beware that less than k results might have been returned
  k = result->size();
  for (i=0; i<k/2; i++) {
    temp = (*result)[i];
    (*result)[i] = (*result)[k-1-i];
    (*result)[k-1-i] = temp;
  }
  delete neighborheap;
}

//--------------------------------------------------------------
// recursive function for nearest neighbor search in subtree
// under *node*. Updates the heap (class member) *neighborheap*.
// returns "true" when no nearer neighbor elsewhere possible
//--------------------------------------------------------------
bool KdTree::neighbor_search(const CoordPoint &point, kdtree_node* node, size_t k)
{
  double curdist, dist;

  curdist = distance(point, node->point, distweights);
  if (!(searchpredicate && !(*searchpredicate)(allnodes[node->dataindex]))) {
    if (neighborheap->size() < k) {
      neighborheap->push(nn4heap(node->dataindex,curdist));
    } else if (curdist < neighborheap->top().distance) {
      neighborheap->pop();
      neighborheap->push(nn4heap(node->dataindex,curdist));
    }
  }
  // first search on side closer to point
  if (point[node->cutdim] < node->point[node->cutdim]) {
    if (node->loson)
      if (neighbor_search(point, node->loson, k))
        return true;
  } else {
    if (node->hison)
      if (neighbor_search(point, node->hison, k))
        return true;
  }
  // second search on farther side, if necessary
  dist = neighborheap->top().distance;
  if (point[node->cutdim] < node->point[node->cutdim]) {
    if (node->hison && bounds_overlap_ball(point,dist,node->hison))
      if (neighbor_search(point, node->hison, k))
        return true;
  } else {
    if (node->loson && bounds_overlap_ball(point,dist,node->loson))
      if (neighbor_search(point, node->loson, k))
        return true;
  }  

  return ball_within_bounds(point, neighborheap->top().distance, node);
}

// returns true when the bounds of *node* overlap with the 
// ball with radius *dist* around *point*
bool KdTree::bounds_overlap_ball(const CoordPoint &point, double dist, kdtree_node* node)
{
  double distsum = 0.0;
  size_t i;
  for (i=0; i<dimension; i++) {
    if (point[i] < node->lobound[i]) { // lower than low boundary
      distsum += coordinate_distance(point[i],node->lobound[i],(*distweights)[i]);
      if (distsum > dist)
        return false;
    }
    else if (point[i] > node->upbound[i]) { // higher than high boundary
      distsum += coordinate_distance(point[i],node->upbound[i],(*distweights)[i]);
      if (distsum > dist)
        return false;
    }
  }
  return true;
}

// returns true when the bounds of *node* completely contain the 
// ball with radius *dist* around *point*
bool KdTree::ball_within_bounds(const CoordPoint &point, double dist, kdtree_node* node)
{
  size_t i;
  for (i=0; i<dimension; i++)
    if (coordinate_distance(point[i],node->lobound[i],(*distweights)[i]) <= dist ||
        coordinate_distance(point[i],node->upbound[i],(*distweights)[i]) <= dist)
      return false;
  return true;
}

//--------------------------------------------------------------
// predefined distance functions
//--------------------------------------------------------------
// Maximum distance (Linfinite norm)
double KdTree::distance0(const CoordPoint &p, const CoordPoint &q, DoubleVector* weights)
{
  size_t i;
  double dist, test;
  dist = (*weights)[0] * fabs(p[0]-q[0]);
  for (i=1; i<p.size();i++) {
    test = (*weights)[i] * fabs(p[i]-q[i]);
    if (test > dist) dist = test;
  }
  return dist;
}
double KdTree::coordinate_distance0(double x, double y, double weight)
{
  return weight*fabs(x-y);
}
// Manhatten distance (L1 norm)
double KdTree::distance1(const CoordPoint &p, const CoordPoint &q, DoubleVector* weights)
{
  size_t i;
  double dist = 0.0;
  for (i=0; i<p.size();i++)
    dist += (*weights)[i]*fabs(p[i]-q[i]);
  return dist;
}
double KdTree::coordinate_distance1(double x, double y, double weight)
{
  return weight*fabs(x-y);
}
// Euklidean distance (L2 norm)
double KdTree::distance2(const CoordPoint &p, const CoordPoint &q, DoubleVector* weights)
{
  size_t i;
  double dist = 0.0;
  for (i=0; i<p.size();i++)
    dist += (*weights)[i]*(p[i]-q[i])*(p[i]-q[i]);
  return dist;
}
double KdTree::coordinate_distance2(double x, double y, double weight)
{
  return weight*(x-y)*(x-y);
}

}} // end namespace Gamera::Kdtree
