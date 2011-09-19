/*
 * Copyright (C) 2009-2011 Christoph Dalitz
 *               2010      Oliver Christen
 *               2011      Christian Brandt
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */


#ifndef cd30112009_geometry
#define cd30112009_geometry

#include <map>
#include <set>
#include <algorithm>
#include "gamera.hpp"
#include "vigra/distancetransform.hxx"
#include "vigra/seededregiongrowing.hxx"
#include "geostructs/kdtree.hpp"
#include "geostructs/delaunaytree.hpp"
#include "graph/graph.hpp"
#include "graph/graphdataderived.hpp"
#include "graph/node.hpp"
#include "plugins/contour.hpp"
#include "plugins/draw.hpp"


using namespace Gamera::Kdtree;
using namespace Gamera::Delaunaytree;
using namespace Gamera::GraphApi;
using namespace std;

namespace Gamera {

  // this implementation is based on a sample program included
  // in the VIGRA library by Ulrich Koethe
  template<class T>
  Image* voronoi_from_labeled_image(const T& src, bool white_edges=false) {
    typedef typename T::value_type value_type;
    typedef typename ImageFactory<T>::data_type data_type;
    typedef typename ImageFactory<T>::view_type view_type;
    // vigra's seeded region growing only works on greyscale images
    Grey16ImageData* voronoi_data = new Grey16ImageData(src.size(), src.origin());
    Grey16ImageView* voronoi = new Grey16ImageView(*voronoi_data);
    size_t x,y;
    value_type val, maxlabel;
    map<value_type, bool> all_labels;

    maxlabel = 0;
    for (y=0; y<src.nrows(); ++y) {
      for (x=0; x<src.ncols(); ++x) {
        val = src.get(Point(x,y));
        if (val > 0) {
          voronoi->set(Point(x,y),val);
          all_labels.insert(make_pair(val,true));
          if (val > maxlabel) maxlabel = val;
        } else {
          voronoi->set(Point(x,y),0);
        }
      }
    }
    if (all_labels.size() <= 2) {
      delete voronoi;
      delete voronoi_data;
      throw std::runtime_error("Black pixels must be labeled for Voronoi tesselation.");
    }

    FloatImageData* dist_data = new FloatImageData(src.size(), src.origin());
    FloatImageView* dist = new FloatImageView(*dist_data);

    try {
      // The Voronoi tesselation is done by a watershed segmentation
      // on the distance transform image, which is quite a bit overhead.
      // The algorithm should be significantly faster when the Voronoi
      // cells are computed directly similar to a distance transform.
      // TODO: implement this based on VIGRA's distance transform code
      vigra::distanceTransform(src_image_range(src), dest_image(*dist), 0, 2);
      vigra::ArrayOfRegionStatistics<vigra::SeedRgDirectValueFunctor<float> > 
        statistics((size_t)maxlabel);
      if (white_edges) {
        vigra::seededRegionGrowing(src_image_range(*dist), src_image(*voronoi),
                                   dest_image(*voronoi), statistics, KeepContours);
      } else {
        vigra::seededRegionGrowing(src_image_range(*dist), src_image(*voronoi),
                                   dest_image(*voronoi), statistics, CompleteGrow);
      }
    } catch (std::exception e) {
      delete dist;
      delete dist_data;
      delete voronoi;
      delete voronoi_data;
      throw;
    }

    // distance image no longer needed
    delete dist;
    delete dist_data;

    // copy over result to return value
    data_type* result_data = new data_type(voronoi->size(), voronoi->origin());
    view_type* result = new view_type(*result_data);
    for (y=0; y<voronoi->nrows(); ++y) {
      for (x=0; x<voronoi->ncols(); ++x) {
        result->set(Point(x,y),(value_type)voronoi->get(Point(x,y)));
      }
    }

    // greyscale image no longer needed
    delete voronoi;
    delete voronoi_data;

    return result;
  }


  template<class T>
  void voronoi_from_points(T& src, const PointVector* points, IntVector* labels) {

    // some plausi checks
    if (points->empty())
      throw std::runtime_error("points must not be empty.");
    if (points->size() != labels->size())
      throw std::runtime_error("Number of points must match the number of labels.");

    size_t i,x,y;

    // build kd-tree from points
    KdNodeVector nodes,neighbors;
    CoordPoint p(2);
    for (i=0; i<points->size(); i++) {
      p[0] = (*points)[i].x();
      p[1] = (*points)[i].y();
      KdNode n(p);
      n.data = &((*labels)[i]);
      nodes.push_back(n);
    }
    KdTree tree(&nodes);

    // label all pixels with nearest neighbor label
    for (y=0; y<src.nrows(); ++y) {
      for (x=0; x<src.ncols(); ++x) {
        if (src.get(Point(x,y)) == 0) {
          p[0] = x; p[1] = y;
          tree.k_nearest_neighbors(p, 1, &neighbors);
          src.set(Point(x,y),*((int*)(neighbors[0].data)));
        }
      }
    }
  }


  // returns list of neighboring label pairs
  template<class T>
  PyObject* labeled_region_neighbors(const T& src, bool eight_connectivity=true) {
    size_t x,y,max_x,max_y;

    typedef typename T::value_type value_type;
    max_x = src.ncols()-1;
    max_y = src.nrows()-1;

    // map for storing neighborship relations; to avoid duplicates,
    // we store for each label only *smaller* neighboring labels
    // note that we must use 'int' insetad of 'value_type' because
    // some versions of gcc do not like nested templates
    typedef set<value_type> set_type;
    typedef map<value_type,set_type> map_type;
    map_type neighbors;

    // check bulk of image
    value_type label1,label2;
    //set<value_type> emptyset;
    set_type emptyset;
    for (y=0; y<max_y; ++y) {
      for (x=0; x<max_x; ++x) {
        label1 = src.get(Point(x,y));
        label2 = src.get(Point(x+1,y));
        if (label1 > label2) {
          if (neighbors.find(label1) == neighbors.end())
            neighbors[label1] = emptyset;
          neighbors[label1].insert(label2);
        }
        else if (label2 > label1) {
          if (neighbors.find(label2) == neighbors.end())
            neighbors[label2] = emptyset;
          neighbors[label2].insert(label1);
        }
        label2 = src.get(Point(x,y+1));
        if (label1 > label2) {
          if (neighbors.find(label1) == neighbors.end())
            neighbors[label1] = emptyset;
          neighbors[label1].insert(label2);
        }
        else if (label2 > label1) {
          if (neighbors.find(label2) == neighbors.end())
            neighbors[label2] = emptyset;
          neighbors[label2].insert(label1);
        }
        if (eight_connectivity) {
          label2 = src.get(Point(x+1,y+1));
          if (label1 > label2) {
            if (neighbors.find(label1) == neighbors.end())
              neighbors[label1] = emptyset;
            neighbors[label1].insert(label2);
          }
          else if (label2 > label1) {
            if (neighbors.find(label2) == neighbors.end())
              neighbors[label2] = emptyset;
            neighbors[label2].insert(label1);
          }
        }
      }
    }
    // check last row
    for (x=0; x<max_x; ++x) {
      label1 = src.get(Point(x,max_y));
      label2 = src.get(Point(x+1,max_y));
      if (label1 > label2) {
        if (neighbors.find(label1) == neighbors.end())
          neighbors[label1] = emptyset;
        neighbors[label1].insert(label2);
      }
      else if (label2 > label1) {
        if (neighbors.find(label2) == neighbors.end())
          neighbors[label2] = emptyset;
        neighbors[label2].insert(label1);
      }
    }
    // check last column
    for (y=0; y<max_y; ++y) {
      label1 = src.get(Point(max_x,y));
      label2 = src.get(Point(max_x,y+1));
      if (label1 > label2) {
        if (neighbors.find(label1) == neighbors.end())
          neighbors[label1] = emptyset;
        neighbors[label1].insert(label2);
      }
      else if (label2 > label1) {
        if (neighbors.find(label2) == neighbors.end())
          neighbors[label2] = emptyset;
        neighbors[label2].insert(label1);
      }
    }
    //printf("emptyset.size(): %i\n", emptyset.size());

    // copy result over to return value
    PyObject *retval, *entry, *entry1, *entry2;
    retval = PyList_New(0);
    typename map_type::iterator it1;
    typename set_type::iterator it2;
    for (it1=neighbors.begin(); it1!=neighbors.end(); it1++) {
      entry1 = Py_BuildValue("i", (int)it1->first);
      //printf("Neighbors of %i:", (int)it1->first);
      for (it2=it1->second.begin(); it2!=it1->second.end(); it2++) {
        // beware that PyList_SetItem 'steals' a reference,
        // while PyList_append increases the reference
        entry = PyList_New(2);
        Py_INCREF(entry1);
        PyList_SetItem(entry, 0, entry1);
        entry2 = Py_BuildValue("i", (int)*it2);
        //printf(" %i", (int)*it2);
        PyList_SetItem(entry, 1, entry2);
        PyList_Append(retval, entry);
        Py_DECREF(entry);
      }
      //printf("\n");
      Py_DECREF(entry1);
    }
    return retval;
  }


  //-----------------------------------------------------------------------
  // functions for Delaunay triangulation
  //-----------------------------------------------------------------------
  void delaunay_from_points_cpp(PointVector *pv, IntVector *lv, std::map<int,std::set<int> > *result) {

    // some plausi checks
	if (pv->empty()) {
      throw std::runtime_error("No points for triangulation given.");
    }
    if (pv->size() < 3) {
      throw std::runtime_error("At least three points are required.");
    }
    if (pv->size() != lv->size()) {
      throw std::runtime_error("Number of points must match the number of labels.");
    }

    DelaunayTree dt;
    PointVector::iterator pv_it;
    IntVector::iterator lv_it;
    std::vector<Vertex*> vertices;
    std::vector<Vertex*>::iterator it;

    result->clear();

    pv_it = pv->begin();
    lv_it = lv->begin();

    int x, y;
    while(pv_it != pv->end() && lv_it != lv->end()) {
      x = (*pv_it).x();
      y = (*pv_it).y();
      vertices.push_back(new Vertex(x, y, (*lv_it)));
      ++pv_it;
      ++lv_it;
    }
    random_shuffle(vertices.begin(), vertices.end());
    dt.addVertices(&vertices);
    dt.neighboringLabels(result);
    for(it = vertices.begin() ; it != vertices.end() ; ++it) {
      delete *it;
    }
  }
  
  PyObject* delaunay_from_points(PointVector *pv, IntVector *lv) {
  	PyObject *list, *entry, *label1, *label2;
    std::map<int,std::set<int> > neighbors;
    std::map<int,std::set<int> >::iterator nit1;
    std::set<int>::iterator nit2;
  	
	delaunay_from_points_cpp(pv, lv, &neighbors);
    list = PyList_New(0);
    for (nit1=neighbors.begin(); nit1!=neighbors.end(); ++nit1) {
      for (nit2=nit1->second.begin(); nit2!=nit1->second.end(); nit2++) {
        entry = PyList_New(2);
        label1 = Py_BuildValue("i", nit1->first);
        label2 = Py_BuildValue("i", *nit2);
        PyList_SetItem(entry, 0, label1);
        PyList_SetItem(entry, 1, label2);
        PyList_Append(list, entry);
        Py_DECREF(entry);
      }
    }

  	return list;
  }


  //-----------------------------------------------------------------------
  // functions for graph coloring of Cc's with different colors
  //-----------------------------------------------------------------------
  typedef std::map<unsigned int, Image*> LabelCcMap;
  template<class T>
  Graph *graph_from_ccs(T &image, ImageVector &ccs, int method) {
    Graph *graph = new Graph(FLAG_UNDIRECTED);
    graph->make_singly_connected();

    PointVector *pv = new PointVector();
    IntVector *iv = new IntVector();
    ImageVector::iterator iter;

    if( method == 0 || method == 1 ) {
      if( method == 0 ) {
        // method == 0 --> from the CC center points
        for( iter = ccs.begin(); iter != ccs.end(); iter++) {
          Cc* cc = static_cast<Cc*>((*iter).first);
          pv->push_back( cc->center() );
          iv->push_back( cc->label() );
        }        
      }
      else if( method == 1) {
        // method == 1 --> from a 20 percent sample of the contour points
        for( iter = ccs.begin(); iter != ccs.end(); iter++) {
          Cc* cc = static_cast<Cc*>((*iter).first);
          PointVector *cc_pv = contour_samplepoints(*cc, 20);
          PointVector::iterator point_vec_iter;
          for( point_vec_iter = cc_pv->begin(); point_vec_iter != cc_pv->end(); point_vec_iter++ ) {
            pv->push_back(*point_vec_iter);
            iv->push_back(cc->label());
          }
          delete cc_pv;
        }
      }

      // Build the graph
      std::map<int,std::set<int> > neighbors;
      std::map<int,std::set<int> >::iterator nit1;
      std::set<int>::iterator nit2;
      delaunay_from_points_cpp(pv, iv, &neighbors);
      for (nit1=neighbors.begin(); nit1!=neighbors.end(); ++nit1) {
        for (nit2=nit1->second.begin(); nit2!=nit1->second.end(); nit2++) {
           GraphDataLong* a_p = new GraphDataLong(nit1->first);
           GraphDataLong* b_p = new GraphDataLong(*nit2);
           bool del_a = !graph->add_node(a_p);
           bool del_b = !graph->add_node(b_p);
           graph->add_edge(a_p, b_p); 
           if(del_a)
              delete a_p;
           if(del_b)
              delete b_p;
        }
      }
    }
    else if( method == 2 ) {
      // method == 2 --> from the exact area Voronoi diagram
      typedef typename ImageFactory<T>::view_type view_type;
      Image *voronoi       = voronoi_from_labeled_image(image);
      PyObject *labelpairs = labeled_region_neighbors( *((view_type*) voronoi) );
      for (int i = 0; i < PyList_Size(labelpairs); i++) {
        PyObject *adj_list = PyList_GetItem(labelpairs, i);
        PyObject *region1 = PyList_GetItem(adj_list, 0);
        PyObject *region2 = PyList_GetItem(adj_list, 1);
        GraphDataLong* a_p = new GraphDataLong(PyInt_AsLong(region1));
        GraphDataLong* b_p = new GraphDataLong(PyInt_AsLong(region2));
        bool del_a = !graph->add_node(a_p);
        bool del_b = !graph->add_node(b_p);
        graph->add_edge(a_p, b_p); 
        if(del_a)
           delete a_p;
        if(del_b)
           delete b_p;
      }
      delete voronoi->data();
      delete voronoi;
      Py_DECREF(labelpairs);
    }
    else {
      throw std::runtime_error("Unknown method for construction the neighborhood graph");
    }

    delete pv;
    delete iv;
    return graph;
  }


  template<class T>
  RGBImageView* graph_color_ccs(T &image, ImageVector &ccs, PyObject *colors, int method) {
    Graph *graph = NULL;
    std::vector<RGBPixel*> RGBColors;
    
    // check input parameters
    if( ccs.size() == 0 ) {
      throw std::runtime_error("graph_color_ccs: no CCs given.");
    }
    if( !PyList_Check(colors) ) {
      throw std::runtime_error("graph_color_ccs: colors is no list");
    }
    if( PyList_Size(colors) < 6 ) {
      throw std::runtime_error("graph_color_ccs: coloring algorithm only works "
            "with more than five colors");
    }

    // extract the colors
    for( int i = 0; i < PyList_Size(colors); i++) {
      PyObject *Py_RGBPixel = PyList_GetItem(colors, i);
      RGBPixel *RGBPixel    = ((RGBPixelObject*) Py_RGBPixel )->m_x;
      RGBColors.push_back(RGBPixel);
    }

    // build the graph from the given ccs
    graph = graph_from_ccs(image, ccs, method);

    // volor the graph
    graph->colorize( PyList_Size(colors) );

    // Create the return image
    // Ccs not passed to the function are set black in the result
    typedef TypeIdImageFactory<RGB, DENSE> RGBViewFactory;
    
    RGBViewFactory::image_type *coloredImage = 
       RGBViewFactory::create(image.origin(), image.dim());
    
    int label;
    for( size_t y = 0; y < image.nrows(); y++) {
      for( size_t x = 0; x < image.ncols(); x++ ) {
        label = image.get(Point(x,y));
        if( label != 0 ) {
          try {
             GraphDataLong d(label);
             Node* n = graph->get_node(&d);
             unsigned int c = graph->get_color(n);
             coloredImage->set(Point(x,y), *RGBColors[c]);
          }
          catch( std::runtime_error runtimeError ) {
            coloredImage->set(Point(x,y), RGBPixel(0,0,0));
          }
        }
      }
    }

    NodePtrIterator* it = graph->get_nodes();
    Node* n;
    
    while((n = it->next()) != NULL) {
      delete dynamic_cast<GraphDataLong*>(n->_value);
    }

    delete it;
    delete graph;

    return coloredImage;
  }

  //------------------------------------------------------------------
  // convex hull computation with Graham's scan algorithm.
  // See Cormen et al.: Introduction to Algorithms. 2nd ed., p. 949
  //------------------------------------------------------------------

  inline bool greater_distance(const Point& origin, const Point& p1, const Point& p2) {
    double dx2 = (double)p2.x() - origin.x();
    double dx1 = (double)p1.x() - origin.x();
    double dy2 = (double)p2.y() - origin.y();
    double dy1 = (double)p1.y() - origin.y();
    if (dy1*dy1+dx1*dx1 > dy2*dy2+dx2*dx2) {
      return true;
    }
    return false;
  }

  // positive when p0p1 clockwise oriented compared to p0p2
  // zero when all points collinear
  inline double clockwise_orientation(const Point& p0, const Point& p1, const Point& p2) {
    return ((double)p1.x() - p0.x())*((double)p2.y() - p0.y()) -
      ((double)p2.x() - p0.x())*((double)p1.y() - p0.y());
  }

  class CompareCounterclockwise {
  public:
    Point origin;
    CompareCounterclockwise(Point _origin) {
      origin = _origin;
    }
    inline bool operator()(const Point& p1, const Point& p2) const {
      return clockwise_orientation(origin, p1, p2) > 0.0;
    }
  };

  PointVector* convex_hull_from_points(PointVector *points) {
    if (points->size() == 0)
      throw std::runtime_error("No points given to convex hull computation.");

    unsigned int min_y = std::numeric_limits<unsigned int>::max();
    unsigned int min_x = std::numeric_limits<unsigned int>::max();
    size_t i;
    size_t min_i = 0;
    // get topmost point
    for (i=0; i < points->size(); i++) {
      if (points->at(i).y() < min_y) {
        min_y = points->at(i).y();
        min_x = points->at(i).x();
        min_i = i;
      }
      else if (points->at(i).y() == min_y && points->at(i).x() < min_x) {
        min_x = points->at(i).x();
        min_i = i;
      }
    }
    // and remember it as origin
    points->at(min_i) = points->at(0);
    points->at(0) = Point(min_x,min_y);
    Point origin = points->at(0);

    // sort remaining points counter clockwise
    CompareCounterclockwise comparefunc(origin);
    std::sort(points->begin()+1, points->end(),comparefunc);

    // of collinear points, only keep the farest from origin
    PointVector sortedpoints;
    sortedpoints.push_back(origin);
    size_t start_i = 1;
    while (start_i < points->size() && points->at(start_i) == origin)
      start_i++; // beware of doublets in the point vector
    if (start_i < points->size()) {
      sortedpoints.push_back(points->at(start_i));
      start_i++;
    }
    for (i=start_i; i<points->size(); i++) {
      if (points->at(i) == sortedpoints.back())
        continue;
      if (0 != clockwise_orientation(origin, points->at(i), sortedpoints.back())) {
        sortedpoints.push_back(points->at(i));
      }
      else if (greater_distance(origin,points->at(i),sortedpoints.back())) {
        sortedpoints.pop_back();
        sortedpoints.push_back(points->at(i));
      }
    }

    // do Graham's scan
    PointVector* S = new PointVector;
    if (sortedpoints.size() < 3) {
      for (i=0; i < sortedpoints.size(); i++)
        S->push_back(sortedpoints[i]);
      return S;
    }
    S->push_back(sortedpoints[0]);
    S->push_back(sortedpoints[1]);
    S->push_back(sortedpoints[2]);

    for(i = 3; i < sortedpoints.size(); i++) {
      Point top = S->at(S->size()-1);
      Point ntt = S->at(S->size()-2);
      Point p = sortedpoints[i];
      while (S->size() > 2 && clockwise_orientation(top,p,ntt) <= 0.0) {
        S->pop_back();
        top = S->at(S->size()-1);
        ntt = S->at(S->size()-2);
      }
      S->push_back(p);
    }

    return S;
  }

  template<class T>
  PointVector* convex_hull_as_points(const T& src) {
    PointVector *contour_points = new PointVector();
    PointVector::iterator found;

    FloatVector *left = contour_left(src);
    FloatVector *right = contour_right(src);
    FloatVector::iterator it;
    size_t y;

    for(it = left->begin(), y=0; it != left->end() ; it++, y++) {
      if( *it != std::numeric_limits<double>::infinity() ) {
        contour_points->push_back(Point((int)*it,y));
      }
    }
    for(it = right->begin(), y=0; it != right->end() ; it++, y++) {
      if( *it != std::numeric_limits<double>::infinity() ) {
        contour_points->push_back(Point((int)src.ncols()-*it,y));
      }
    }
    PointVector *output = convex_hull_from_points(contour_points);

    delete left;
    delete right;
    delete contour_points;

    return output;
  }

  template<class T>
    Image* convex_hull_as_image(const T& src, bool filled) {
    typedef typename T::value_type value_type;
    
    typedef typename ImageFactory<OneBitImageView>::view_type view_type;
    OneBitImageData* res_data = new OneBitImageData(src.size(),src.origin());
    OneBitImageView* res = new OneBitImageView(*res_data,src.origin(),src.size());

    PointVector* hullpoints = convex_hull_as_points(src);
    for (size_t i=1; i< hullpoints->size(); i++)
      draw_line(*res,hullpoints->at(i-1),hullpoints->at(i),black(*res));
    draw_line(*res,hullpoints->back(),hullpoints->front(),black(*res));

    delete hullpoints;

    if (filled) {
      size_t x,y,from,to;
      for (y=0; y<res->nrows(); y++) {
        from = to = res->ncols();
        from = 0;
        while (from < res->ncols() && is_white(res->get(Point(from,y))))
          from++;
        if (from >= res->ncols()-1) continue;
        to = res->ncols()-1;
        while (to > 0 && is_white(res->get(Point(to,y))))
          to--;
        for (x=from+1; x<to; x++)
          res->set(Point(x,y),black(*res));
      }
    }

    return res;
  }

} // namespace Gamera
#endif

