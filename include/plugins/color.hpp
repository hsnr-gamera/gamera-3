/*
 * Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, Karl MacMillan
 *               2010      Hasan Yildiz, Tobias Bolten, Oliver Christen,
 *                         Christoph Dalitz
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

#ifndef mgd120203_color
#define mgd120203_color

#include "gamera.hpp"
#include "image_conversion.hpp"

#include <string>
#include <map>
#include <vector>

#include "colorgraph.hpp"
#include "plugins/contour.hpp"
#include "plugins/geometry.hpp"

namespace Gamera {

  using namespace Colorgraph;

  template<class T, class U, class F>
  struct extract_plane {
    U* operator()(const T& image) {
      typedef typename T::value_type from_pixel_type;
      typedef typename U::value_type to_pixel_type;
      U* view = _image_conversion::creator<to_pixel_type>::image(image);
      typename T::const_vec_iterator in = image.vec_begin();
      typename U::vec_iterator out = view->vec_begin();
      ImageAccessor<from_pixel_type> in_acc;
      ImageAccessor<to_pixel_type> out_acc;
      F f;
      for (; in != image.vec_end(); ++in, ++out)
        out_acc.set(f(from_pixel_type(in_acc.get(in))), out);
      return view;
    }
  };

  struct Hue {
    FloatPixel operator()(RGBPixel pixel) {
      return pixel.hue();
    }
  };
  extract_plane<RGBImageView, FloatImageView, Hue> hue;

  struct Saturation {
    FloatPixel operator()(RGBPixel pixel) {
      return pixel.saturation();
    }
  };
  extract_plane<RGBImageView, FloatImageView, Saturation> saturation;

  struct Value {
    FloatPixel operator()(RGBPixel pixel) {
      return pixel.value();
    }
  };
  extract_plane<RGBImageView, FloatImageView, Value> value;

  struct Cyan {
    FloatPixel operator()(RGBPixel pixel) {
      return pixel.cyan();
    }
  };
  extract_plane<RGBImageView, FloatImageView, Cyan> cyan;

  struct Magenta {
    FloatPixel operator()(RGBPixel pixel) {
      return pixel.magenta();
    }
  };
  extract_plane<RGBImageView, FloatImageView, Magenta> magenta;

  struct Yellow {
    FloatPixel operator()(RGBPixel pixel) {
      return pixel.yellow();
    }
  };
  extract_plane<RGBImageView, FloatImageView, Yellow> yellow;

  struct Red {
    FloatPixel operator()(RGBPixel pixel) {
      return pixel.red();
    }
  };
  extract_plane<RGBImageView, FloatImageView, Red> red;

  struct Green {
    FloatPixel operator()(RGBPixel pixel) {
      return pixel.green();
    }
  };
  extract_plane<RGBImageView, FloatImageView, Green> green;

  struct Blue {
    FloatPixel operator()(RGBPixel pixel) {
      return pixel.blue();
    }
  };
  extract_plane<RGBImageView, FloatImageView, Blue> blue;

  struct CIE_X {
    FloatPixel operator()(RGBPixel pixel) {
      return pixel.cie_x();
    }
  };
  extract_plane<RGBImageView, FloatImageView, CIE_X> cie_x;

  struct CIE_Y {
    FloatPixel operator()(RGBPixel pixel) {
      return pixel.cie_y();
    }
  };
  extract_plane<RGBImageView, FloatImageView, CIE_Y> cie_y;

  struct CIE_Z {
    FloatPixel operator()(RGBPixel pixel) {
      return pixel.cie_z();
    }
  };
  extract_plane<RGBImageView, FloatImageView, CIE_Z> cie_z;

  struct CIE_Lab_L {
    FloatPixel operator()(RGBPixel pixel) {
      return pixel.cie_Lab_L();
    }
  };
  extract_plane<RGBImageView, FloatImageView, CIE_Lab_L> cie_Lab_L;

  struct CIE_Lab_a {
    FloatPixel operator()(RGBPixel pixel) {
      return pixel.cie_Lab_a();
    }
  };
  extract_plane<RGBImageView, FloatImageView, CIE_Lab_L> cie_Lab_a;

  struct CIE_Lab_b {
    FloatPixel operator()(RGBPixel pixel) {
      return pixel.cie_Lab_L();
    }
  };
  extract_plane<RGBImageView, FloatImageView, CIE_Lab_L> cie_Lab_b;

  // TODO: Find a cool way to false color Complex images.

  RGBImageView* false_color(const FloatImageView& image) {
    RGBImageView* view = _image_conversion::creator<RGBPixel>::image(image);

    FloatImageView::const_vec_iterator vi = image.vec_begin();
    FloatPixel max = *vi;
    FloatPixel min = *vi;
    for (; vi != image.vec_end(); ++vi) {
      if (*vi > max)
	max = *vi;
      if (*vi < min)
	min = *vi;
    }
    
    FloatPixel scale = (max - min);

    // We don't use a table (as with 8-bit greyscale) because we can get
    // much greater color "depth" this way (The table method only uses
    // 64 values per plane)

    FloatImageView::const_vec_iterator in = image.vec_begin();
    RGBImageView::vec_iterator out = view->vec_begin();
    ImageAccessor<FloatPixel> in_acc;
    ImageAccessor<RGBPixel> out_acc;
    for (; in != image.vec_end(); ++in, ++out) {
      double h = ((in_acc.get(in) - min) / scale) * 4.0;
      size_t i = (size_t)h;
      GreyScalePixel f, q;
      // v == 255, p == 0
      switch (i) {
      case 0:
	f = (GreyScalePixel)((h - (double)i) * 255.0);
	out_acc.set(RGBPixel(255, f, 0), out);
	break;
      case 1:
	q = 255 - (GreyScalePixel)((h - (double)i) * 255.0);
	out_acc.set(RGBPixel(q, 255, 0), out);
	break;
      case 2:
	f = (GreyScalePixel)((h - (double)i) * 255.0);
	out_acc.set(RGBPixel(0, 255, f), out);
	break;
      case 3:
	q = 255 - (GreyScalePixel)((h - (double)i) * 255.0);
	out_acc.set(RGBPixel(0, q, 255), out);
	break;
      case 4: // The end (should only represent a single value)
	out_acc.set(RGBPixel(0, 0, 255), out); 
	break;
      }
    }
    return view;	
  }

  RGBImageView* false_color(const GreyScaleImageView& image) {
    RGBImageView* view = _image_conversion::creator<RGBPixel>::image(image);
    GreyScaleImageView::const_vec_iterator in = image.vec_begin();
    RGBImageView::vec_iterator out = view->vec_begin();
    ImageAccessor<GreyScalePixel> in_acc;
    ImageAccessor<RGBPixel> out_acc;

    // Build a table mapping greyscale values to false color RGBPixels
    RGBPixel table[256];
    size_t i = 0, j = 0;
    for (; i < 64; ++i, j += 4) {
      table[i].red(255); table[i].green(j); table[i].blue(0);
    }
    j -= 4;
    for (; i < 128; ++i, j -= 4) {
      table[i].red(j); table[i].green(255); table[i].blue(0);
    }
    j = 0;
    for (; i < 192; ++i, j += 4) {
      table[i].red(0); table[i].green(255); table[i].blue(j);
    }
    j -= 4;
    for (; i < 256; ++i, j -= 4) {
      table[i].red(0); table[i].green(j); table[i].blue(255);
    }

    // Create RGB based on table
    for (; in != image.vec_end(); ++in, ++out)
      out_acc.set(table[in_acc.get(in)], out);
      
    return view;	
  }

  // replace colors with labels
  // Christoph Dalitz and Hasan Yildiz
  template<class T>
  Image* colors_to_labels(const T &src, PyObject* obj) {
    OneBitImageData* dest_data = new OneBitImageData(src.size(), src.origin());
    OneBitImageView* dest = new OneBitImageView(*dest_data, src.origin(), src.size());

    typedef typename T::value_type value_type;
    value_type value;
    typedef typename OneBitImageView::value_type onebit_value_type;

    char buffer[16];
    std::string buf;
    onebit_value_type label;
    // highest label that can be stored in Onebit pixel type
    onebit_value_type max_value = std::numeric_limits<onebit_value_type>::max();

    std::map<std::string, unsigned int> pixel;
    std::map<std::string, unsigned int>::iterator iter;

    PyObject *itemKey, *itemValue;
    PyObject *testKey;
    Py_ssize_t pos = 0;

    // mapping given how colors are to be mapped to labels
    if (PyDict_Check(obj)) {

      // copy color->label map to C++ map
      long given_label;
      label = 1;
      while (PyDict_Next(obj, &pos, &itemKey, &itemValue)) {
        if (label == max_value) {
          char msg[128];
          sprintf(msg, "More RGB colors than available labels (%i).", max_value);
          throw std::range_error(msg);
        }
        label++;
        testKey = PyObject_Str(itemKey);
        given_label = PyInt_AsLong(itemValue);
        if (given_label < 0)
          throw std::invalid_argument("Labels must be positive integers.");
        if (pixel.find(PyString_AsString(testKey)) == pixel.end()) 
          pixel[PyString_AsString(testKey)] = given_label;
        Py_DECREF(testKey);
      }

      for (size_t y=0; y<src.nrows(); ++y) {
        for (size_t x=0; x<src.ncols(); ++x) {
          value = src.get(Point(x,y));
          // Warning: this assumes a specific string representation of RGBPixel !!
          sprintf(buffer, "(%i, %i, %i)", value.red(), value.green(), value.blue());
          buf = buffer;
          if (pixel.find(buf) != pixel.end()) 
            dest->set(Point(x,y), pixel.find(buf)->second);
        }
      }
    }

    // no mapping given: determine labels automatically by counting
    else if (obj == Py_None) {
      label = 2;
      for (size_t y=0; y<src.nrows(); ++y) {
        for (size_t x=0; x<src.ncols(); ++x) {

          value = src.get(Point(x,y));
          sprintf(buffer, "(%i, %i, %i)", value.red(), value.green(), value.blue());
          buf = buffer;

          // special cases black and white
          if (buf == "(0, 0, 0)" && pixel.find(buf) == pixel.end())
            pixel[buf] = 1;
          if (buf == "(255, 255, 255)" && pixel.find(buf) == pixel.end())
            pixel[buf] = 0;

          // when new color: add to map and increase label counter
          if (pixel.find(buf) == pixel.end()) {
            if (label == max_value) {
              char msg[128];
              sprintf(msg, "More RGB colors than available labels (%i).", max_value);
              throw std::range_error(msg);
            }
            pixel[buf] = label++;
          }

          // replace color with label
          dest->set(Point(x,y), pixel.find(buf)->second);
        }
      }
    }

    // some argument given that is not a mapping color -> label
    else {
      throw std::invalid_argument("Mapping rgb_to_label must be dict or None");
    }

    return dest;
  }


  //-----------------------------------------------------------------------
  // functions for graph coloring of Cc's with different colors
  //-----------------------------------------------------------------------
  template<class T>
  ColorGraph *graph_from_ccs(T &image, ImageVector &ccs, int method) {
    ColorGraph *graph = new ColorGraph();

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
          graph->add_edge(nit1->first, *nit2);
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
        long neighborhood1 = PyInt_AsLong(region1);
        long neighborhood2 = PyInt_AsLong(region2);
        graph->add_edge( (int) neighborhood1, (int) neighborhood2);
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
    ColorGraph *graph = NULL;
    std::vector<RGBPixel*> RGBColors;
    
    // check input parameters
    if( ccs.size() == 0 ) {
      throw std::runtime_error("graph_color_ccs: no CCs given.");
    }
    if( !PyList_Check(colors) ) {
      throw std::runtime_error("graph_color_ccs: colors is no list");
    }
    if( PyList_Size(colors) < 6 ) {
      throw std::runtime_error("graph_color_ccs: coloring algorithm only works with more than five colors");
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

//     // for debugging only: print the result color histogramm
//     std::map<int, int> *color_histogramm = graph->get_color_histogramm();
//     std::map<int, int>::iterator histo_iter;
//     for( histo_iter = color_histogramm->begin(); histo_iter != color_histogramm->end(); histo_iter++) {
//         std::cout << histo_iter->first << " --> " << histo_iter->second << std::endl;
//     }

    // Create the return image
    // Ccs not passed to the function are set black in the result
    typedef TypeIdImageFactory<RGB, DENSE> RGBViewFactory;
    RGBViewFactory::image_type *coloredImage = RGBViewFactory::create(image.origin(), image.dim());
    int label;
    for( size_t y = 0; y < image.nrows(); y++) {
      for( size_t x = 0; x < image.ncols(); x++ ) {
        label = image.get(Point(x,y));
        if( label != 0 ) {
          try {
            int c = graph->get_color(label);
            coloredImage->set(Point(x,y), *RGBColors[c]);
          }
          catch( std::runtime_error runtimeError ) {
            coloredImage->set(Point(x,y), RGBPixel(0,0,0));
          }
        }
      }
    }
    delete graph;

    return coloredImage;
  }


}

#endif
