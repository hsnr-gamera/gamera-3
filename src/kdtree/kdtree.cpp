//
// Copyright (C) 2009 Christoph Dalitz
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

#include "kdtree.hpp"
#include <algorithm>
#include <stdexcept>
#include <math.h>
#include <limits>


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
class kdtree_node {
public:
  kdtree_node() {
    dataindex = cutdim = 0;
    loson = hison = (kdtree_node*)NULL;
  }
  ~kdtree_node() {
    if (loson) delete loson;
    if (hison) delete hison;
  }
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

//--------------------------------------------------------------
// different distance metrics
//--------------------------------------------------------------
class DistanceMeasure {
public:
  DistanceMeasure() {}
  virtual ~DistanceMeasure() {}
  virtual double distance(const CoordPoint &p, const CoordPoint &q) = 0;
  virtual double coordinate_distance(double x, double y, size_t dim) = 0;
};
// Maximum distance (Linfinite norm)
class DistanceL0 : virtual public DistanceMeasure
{
  DoubleVector* w;
 public:
  DistanceL0(const DoubleVector* weights = NULL) {
    if (weights) w = new DoubleVector(*weights);
    else         w = (DoubleVector*)NULL;
  }
  ~DistanceL0() {
    if (w) delete w;
  }
  double distance(const CoordPoint &p, const CoordPoint &q) {
    size_t i;
    double dist, test;
    if (w) {
      dist = (*w)[0] * fabs(p[0]-q[0]);
      for (i=1; i<p.size();i++) {
        test = (*w)[i] * fabs(p[i]-q[i]);
        if (test > dist) dist = test;
      }
    } else {
      dist = fabs(p[0]-q[0]);
      for (i=1; i<p.size();i++) {
        test = fabs(p[i]-q[i]);
        if (test > dist) dist = test;
      }
    }
    return dist;
  }
  double coordinate_distance(double x, double y, size_t dim) {
    if (w) return (*w)[dim] * fabs(x-y);
    else   return fabs(x-y);
  }
};
// Manhatten distance (L1 norm)
class DistanceL1 : virtual public DistanceMeasure
{
  DoubleVector* w;
 public:
  DistanceL1(const DoubleVector* weights = NULL) {
    if (weights) w = new DoubleVector(*weights);
    else         w = (DoubleVector*)NULL;
  }
  ~DistanceL1() {
    if (w) delete w;
  }
  double distance(const CoordPoint &p, const CoordPoint &q)
  {
    size_t i;
    double dist = 0.0;
    if (w) {
      for (i=0; i<p.size();i++)
        dist += (*w)[i]*fabs(p[i]-q[i]);
    } else {
      for (i=0; i<p.size();i++)
        dist += fabs(p[i]-q[i]);
    }
    return dist;
  }
  double coordinate_distance(double x, double y, size_t dim)
  {
    if (w) return (*w)[dim] * fabs(x-y);
    else   return fabs(x-y);
  }
};
// Euklidean distance (L2 norm)
class DistanceL2 : virtual public DistanceMeasure
{
  DoubleVector* w;
 public:
  DistanceL2(const DoubleVector* weights = NULL) {
    if (weights) w = new DoubleVector(*weights);
    else         w = (DoubleVector*)NULL;
  }
  ~DistanceL2() {
    if (w) delete w;
  }
  double distance(const CoordPoint &p, const CoordPoint &q)
  {
    size_t i;
    double dist = 0.0;
    if (w) {
      for (i=0; i<p.size();i++)
        dist += (*w)[i]*(p[i]-q[i])*(p[i]-q[i]);
    } else {
      for (i=0; i<p.size();i++)
        dist += (p[i]-q[i])*(p[i]-q[i]);
    }      
    return dist;
  }
  double coordinate_distance(double x, double y, size_t dim)
  {
    if (w) return (*w)[dim] * (x-y)*(x-y);
    else   return (x-y)*(x-y);
  }
};

//--------------------------------------------------------------
// destructor and constructor of kdtree
//--------------------------------------------------------------
KdTree::~KdTree()
{
  if (root) delete root;
  delete distance;
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
  distance = NULL;
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
  if (distance) delete distance;
  if (distance_type == 0) {
    distance = (DistanceMeasure*) new DistanceL0(weights);
  } else if (distance_type == 1) {
    distance = (DistanceMeasure*) new DistanceL1(weights);
  } else {
    distance = (DistanceMeasure*) new DistanceL2(weights);
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
        neighborheap->push(nn4heap(i,distance->distance(allnodes[i].point,point)));
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

  curdist = distance->distance(point, node->point);
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
  if (neighborheap->size() < k) {
    dist = std::numeric_limits<double>::max();
  } else {
    dist = neighborheap->top().distance;
  }
  if (point[node->cutdim] < node->point[node->cutdim]) {
    if (node->hison && bounds_overlap_ball(point,dist,node->hison))
      if (neighbor_search(point, node->hison, k))
        return true;
  } else {
    if (node->loson && bounds_overlap_ball(point,dist,node->loson))
      if (neighbor_search(point, node->loson, k))
        return true;
  }  

  if (neighborheap->size() == k)
    dist = neighborheap->top().distance;
  return ball_within_bounds(point, dist, node);
}

// returns true when the bounds of *node* overlap with the 
// ball with radius *dist* around *point*
bool KdTree::bounds_overlap_ball(const CoordPoint &point, double dist, kdtree_node* node)
{
  double distsum = 0.0;
  size_t i;
  for (i=0; i<dimension; i++) {
    if (point[i] < node->lobound[i]) { // lower than low boundary
      distsum += distance->coordinate_distance(point[i],node->lobound[i],i);
      if (distsum > dist)
        return false;
    }
    else if (point[i] > node->upbound[i]) { // higher than high boundary
      distsum += distance->coordinate_distance(point[i],node->upbound[i],i);
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
    if (distance->coordinate_distance(point[i],node->lobound[i],i) <= dist ||
        distance->coordinate_distance(point[i],node->upbound[i],i) <= dist)
      return false;
  return true;
}

}} // end namespace Gamera::Kdtree
