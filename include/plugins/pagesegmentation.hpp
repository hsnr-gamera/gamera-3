/*
 *
 * Copyright (C) 2007-2009 Stefan Ruloff, Maria Elhachimi,
 *                         Ilya Stoyanov, Robert Butz
 *               2010      Tobias Bolten
 *               2007-2011 Christoph Dalitz
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

#ifndef cd20070814_pagesegmentation
#define cd20070814_pagesegmentation

#include <Python.h>
#include <map>
#include <vector>
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <functional>
#include "gamera.hpp"
#include "gameramodule.hpp"
#include "gamera_limits.hpp"
#include "connected_components.hpp"
#include "plugins/listutilities.hpp"
#include "plugins/projections.hpp"
#include "plugins/segmentation.hpp"
#include "plugins/image_utilities.hpp"


namespace Gamera {

/* Function: median
 * Calculates the middle height of the CCs.
 * Used for setting defualt parameters in 
 * runlength_smearing and projection_cutting
 */
int pagesegmentation_median_height(ImageList* ccs) {
  vector<int> ccs_heights;
  ImageList::iterator i;

  if (ccs->empty()) {
    throw std::runtime_error("pagesegmentation_median_height: no CC's found in image.");
  }
  for (i = ccs->begin(); i != ccs->end(); ++i) {
    ccs_heights.push_back( (*i)->nrows() );
  }
  return median(&ccs_heights);
}


/*****************************************************************************
* Run Length Smearing
* IN:   Cx - Minimal length of white runs in the rows
*   Cy - Minimal length of white runs in the columns
*   Csm- Minimal length of white runs row-wise in the almost final image
*       
*   If you choose "-1" the algorithm will determine the
*   median character length in the image to obtain the values for Cx,Cy or 
*   Csm.
******************************************************************************/
template<class T>
ImageList* runlength_smearing(T &image, int Cx, int Cy, int Csm) {
    typedef OneBitImageView view_type;
    typedef OneBitImageData data_type;
    typedef typename T::value_type value_type;

    data_type* img1_data = new data_type(image.size(), image.origin());
    view_type* img1 = new view_type(*img1_data);
    image_copy_fill(image, *img1);
    
    data_type* img2_data = new data_type(image.size(), image.origin());
    view_type* img2 = new view_type(*img2_data);
    image_copy_fill(image, *img2);

    int Ctemp = 0;
    size_t nrows = image.nrows();
    size_t ncols = image.ncols();
    size_t x, y;
    value_type black_val = black(image);
    value_type white_val = white(image);

    // when no values given, guess them from the Cc size statistics
    if (Csm <= 0 || Cy <= 0 || Cx <= 0) {
      ImageList* ccs_temp = cc_analysis(image);
      int Median = pagesegmentation_median_height(ccs_temp);

      for (ImageList::iterator i = ccs_temp->begin(); i != ccs_temp->end(); i++) {
        delete *i;
      }
      delete ccs_temp;

      if (Csm <= 0)
        Csm = 3 * Median;
      if (Cy <= 0)
        Cy = 20 * Median;
      if (Cx <= 0)
        Cx = 20 * Median;
    }

    // horizontal smearing
    for (y = 0; y < nrows; ++y) {
      for (x = 0, Ctemp = 0; x < ncols; ++x) {
        if (is_white(image.get(Point(x, y)))) {
          Ctemp += 1;
        } else {
          if ((0 != Ctemp) && (Ctemp <= Cx)){
            for (int z = 0; z < Ctemp; z++) {
              img1->set(Point(x-z-1, y), black_val);
            }
          }
          Ctemp = 0;
        }
      }
    }

    // vertical smearing
    for (x = 0; x < ncols; ++x) {
      for (y = 0, Ctemp = 0; y < nrows; ++y) {
        if (is_white(image.get(Point(x, y)))) {
          Ctemp += 1;
        } else {
          if ((0 != Ctemp) && (Ctemp <= Cy)) {
            for (int z = 0; z < Ctemp; z++)
              img2->set(Point(x, y-z-1), black_val);
          }
          Ctemp = 0;
        }
      }
    }

    // logical AND between both images
    for(y = 0; y < nrows; ++y) {
      for(x = 0; x < ncols; ++x) {
        if ((is_black(img1->get(Point(x, y))))
            && (is_black(img2->get(Point(x, y))))){
          img1->set(Point(x, y), black_val);
        } else {
          img1->set(Point(x, y), white_val);
        }
      }
    }

    // again horizontal smearing for removal of small holes
    for (y = 0; y < nrows; ++y) {
      for (x = 0, Ctemp = 0; x < ncols; ++x) {
        if (is_white(img1->get(Point(x, y)))) {
          Ctemp += 1;
        } else {
          if ((0 != Ctemp) && (Ctemp <= Csm)){
            for (int z = 0; z < Ctemp; z++)
              img1->set(Point(x-z-1, y), black_val);
          }
          Ctemp = 0;
        }
      }
    }

    ImageList* ccs_AND = cc_analysis(*img1);
    ImageList* return_ccs = new ImageList();

    // create result Cc's 
    ImageList::iterator i;
    for (i = ccs_AND->begin(); i != ccs_AND->end(); ++i) {  
      Cc* cc = dynamic_cast<Cc*>(*i);
      int label = cc->label();
      bool containspixel = false; // some segments may not contain black pixels

      // Methods "get" and "set" operates relative to the image view
      // but the offset of the connected components is not relative
      // to the view. (here: (*i)->offset_x() and (*i)->offset_y())
      //
      // This means that these values must be adjusted for labeling
      // the image view.
      for (y = 0; y < cc->nrows(); ++y) {
        for (x = 0; x < cc->ncols(); ++x) {
          if ( is_black(image.get(Point(x+(*i)->offset_x()-image.offset_x(),
                                        y+(*i)->offset_y()-image.offset_y())))
              && is_black(cc->get(Point(x,y))) ) {
            image.set(Point(x + cc->offset_x() - image.offset_x(),
                            y + cc->offset_y() - image.offset_y()), label);
            containspixel = true;
          }
        }
      }

      // create new CC with the dimensions, offset and label from the
      // smeared image, pointing to the original image.
      if (containspixel) {
        return_ccs->push_back(new ConnectedComponent<data_type>(
                *((data_type*)image.data()),                // Data
                label,                                      // Label
                Point((*i)->offset_x(), (*i)->offset_y()),  // Point
                (*i)->dim())                                // Dim
                );
      }
    }
    
    // clean up
    for (ImageList::iterator i=ccs_AND->begin(); i!=ccs_AND->end(); i++)
      delete *i;
    delete ccs_AND;
    delete img1->data();
    delete img1;
    delete img2->data();
    delete img2;

    return return_ccs;
}


/*-------------------------------------------------------------------------
 * Functions for projection_cutting:
 * Interne_RXY_Cut(image, Tx, Ty, ccs, noise, label):recursively splits 
 * the image, sets the label and creates the CCs.
 * Start_point(image, ul, lr):search the upper_left point of the sub-image.
 * End_point(image,ul,lr):search the lower_right point of the sub-image.
 * Split_point:searchs the split point of the image
 * rxy_cut(image,Tx,Ty,noise,label):returns the ccs-list
 *-------------------------------------------------------------------------*/


/* Function: Start_Point
 * This funktion is used to search the first black pixel:
 * calculates the coordinates of the begin of the cc
 * returns the coordinates of the upper-left point of subimage
 */



template<class T>
Point proj_cut_Start_Point(T& image, Point ul, Point lr) {
    Point Start;

    for (size_t y = ul.y(); y <= lr.y(); y++) {
    for (size_t x = ul.x(); x <= lr.x(); x++) {
        if ((image.get(Point(x, y))) != 0) {
            Start.x(x);
            Start.y(y);
            goto endLoop1; // unfortunately there is no break(2) in gorgeous C++
        }
    }
    } 
    endLoop1:

    for (size_t x = ul.x(); x <= lr.x(); x++) {
    for (size_t y = ul.y(); y <= lr.y(); y++) {
        if ((image.get(Point(x, y))) != 0) {
            if (Start.x() > x)
                Start.x(x);
            goto endLoop2; // unfortunately there is no break(2) in gorgeous C++
            }
        }
    }
    endLoop2:
    return Start;
}

/* Function: End_Point
 * This funktion is used to search the last black pixel:the lower-right point
 * of subimage calculates the coordinates of the end of the CC.
 */
template<class T>
Point proj_cut_End_Point(T& image, Point ul, Point lr) {
    Point End;
    size_t x, y;

    for (y = lr.y(); y+1 >= ul.y()+1; y--) {
        for (x = lr.x(); x+1 >= ul.x()+1; x--) {
            if ((image.get(Point(x, y))) != 0) {
                End.x(x);
                End.y(y);
                goto endLoop1;
            }
        }
    }
    endLoop1:
    
    for (x = lr.x(); x+1 > ul.x()+1; x--) {
        for (y = lr.y(); y+1 > ul.y()+1; y--) {
            if ((image.get(Point(x,y))) != 0){
                if (End.x()<x)
                    End.x(x);
                goto endLoop2;
            }
        }
    }
    endLoop2:

    return End;
}

/* Function: Split_Point
 * calculates the coordinates of the split_point.
 * The split point is determined
 * by finding the largest possible gaps in the X and Y projection of the image.
 */
template<class T>
IntVector * proj_cut_Split_Point(T& image, Point ul, Point lr, int Tx, int Ty, int noise, int gap_treatment, char direction ) {
    IntVector * SplitPoints = new IntVector(); //empty IntVector
    size_t size;
    lr.x()-ul.x()>lr.y()-ul.y()?size=lr.x()-ul.x():size=lr.y()-ul.y();

    int SplitPoints_Min[size]; // probably no need for such big mem-alloc, but necessary in certain situations
    int SplitPoints_Max[size]; 
    int gap_width = 0; // width of the gap
    int gap_counter = 0; //number of gaps

    if (direction == 'x'){
        // Correct Points for Rect() with offset
        Point a( ul.x() + image.offset_x(), ul.y() + image.offset_y() );
        Point b( lr.x() + image.offset_x(), lr.y() + image.offset_y() );
        IntVector *proj_x = projection_rows(image, Rect(a, b));
        SplitPoints->push_back(ul.y()); // starting point
        
        for (size_t i = 1; i < proj_x->size(); i++) {
            if ((*proj_x)[i] <= noise) {
                gap_width++;
                if (Ty <= gap_width) {// min-gap <= act-gap?
                SplitPoints_Min[gap_counter] = (i + ul.y() - gap_width+1);
                SplitPoints_Max[gap_counter] = (i + ul.y()); // finally set to last point of gap
                }
            } 
            else {
                if (Ty <= gap_width)
                    gap_counter++;
                gap_width = 0;
            }
        }
    delete proj_x;
    }
    else{ // y-direction
        // Correct Points for Rect() with offset
        Point a( ul.x() + image.offset_x(), ul.y() + image.offset_y() );
        Point b( lr.x() + image.offset_x(), lr.y() + image.offset_y() );
        IntVector *proj_y = projection_cols(image, Rect(a, b));
        SplitPoints->push_back(ul.x()); // starting point
        
        for (size_t i = 1; i < proj_y->size(); i++) {
            if ((*proj_y)[i] <= noise) {
                gap_width++;
                if (Tx <= gap_width) {// min-gap <= act-gap?
                SplitPoints_Min[gap_counter] = (i + ul.x() - gap_width+1);
                SplitPoints_Max[gap_counter] = (i + ul.x()); // finally set to last point of gap
                }
            } 
            else {
                if (Tx <= gap_width) 
                    gap_counter++;
                gap_width = 0;
            }
        }
        delete proj_y;
    }
    
    for (int i=0; i<gap_counter; i++){
        if (0==gap_treatment){ // cut exactly in the middle of the gap -> no unlabeled noise pixels
            int mid = (SplitPoints_Min[i] + SplitPoints_Max[i]) / 2;
            SplitPoints_Min[i] = mid;
            SplitPoints_Max[i] = mid;
        }
        SplitPoints->push_back(SplitPoints_Min[i]);
        SplitPoints->push_back(SplitPoints_Max[i]);
    }   
    direction=='x'? SplitPoints->push_back(lr.y()): SplitPoints->push_back(lr.x()); // ending point
    
    return SplitPoints;
}



/* Function: Interne_RXY_Cut
 * This function recursively splits the image in horizontal or 
 * vertical direction.
 * The original image will have all of its pixels "labeled" with a number
 * representing each connected component.
 */
template<class T>
void projection_cutting_intern(T& image, Point ul, Point lr, ImageList* ccs, 
        int Tx, int Ty, int noise, int gap_treatment, char direction, int& label) {
    
    Point Start = proj_cut_Start_Point(image, ul, lr);
    Point End = proj_cut_End_Point(image, ul, lr);
    IntVector * SplitPoints = proj_cut_Split_Point(image, Start, End, Tx, Ty, noise, gap_treatment, direction);
    IntVector::iterator It;
    
    ul.x(Start.x());
    ul.y(Start.y());
    lr.x(End.x());
    lr.y(End.y());
    
        

    if (!(direction=='y' && SplitPoints->size() == 2)){ // ending condition, SplitPoints==2 => only Start- and Endpoint no gaps
        if (direction=='x'){
            direction = 'y';
            for(It = SplitPoints->begin(); It != SplitPoints->end(); It++){
                Point begin, end; // note the lowercase of end, which is not End
                begin.x(Start.x());
                begin.y(*It);
                It++;
                end.x(End.x());
                end.y(*It);
                projection_cutting_intern(image, begin, end, ccs, Tx, Ty, noise, gap_treatment, direction, label);
            }
        }
        else { // direction==y
            direction = 'x';
            for(It = SplitPoints->begin(); It != SplitPoints->end(); ++It){
                Point begin, end; // note the lowercase of end, which is not End
                begin.x(*It);
                begin.y(Start.y());
                It++;
                end.x(*It);
                end.y(End.y());
                projection_cutting_intern(image, begin, end, ccs, Tx, Ty, noise, gap_treatment, direction, label);
            }
        }
    } else {
        label++;
        for (size_t y = ul.y(); y <= lr.y(); y++) {
            for (size_t x = ul.x(); x <= lr.x(); x++) {
                if((image.get(Point(x, y))) != 0){
                    image.set(Point(x, y), label);
                }
            }
        }

        Point cc(Start.x() + image.offset_x(), Start.y() + image.offset_y());
        ccs->push_back(
                new ConnectedComponent<typename T::data_type>(
                    *((typename T::data_type*)image.data()),
                    OneBitPixel(label),
                    cc,
                    Dim((End.x() - Start.x() + 1), (End.y() - Start.y() + 1))
                )
            );
    }
    delete SplitPoints;
}

/*
 * Function: rxy_cut 
 * Returns a list of ccs found in the image.
 */
template<class T>
ImageList* projection_cutting(T& image, int Tx, int Ty, int noise, int gap_treatment) {
    int Label = 1;
    char direction = 'x';

    if (noise < 0) {
        noise = 0;
    }

    // set default values
    if (Tx < 1 || Ty < 1) {
        ImageList* ccs_temp = cc_analysis(image);
        int Median = pagesegmentation_median_height(ccs_temp);
        for (ImageList::iterator i = ccs_temp->begin(); 
                i != ccs_temp->end(); i++) {
            delete *i;
        }
        delete ccs_temp;
        if (Tx < 1) {
          Tx = Median * 7;
        }
        if (Ty < 1) {
          if (Median > 1) Ty = Median / 2;
          else Ty = 1;
        }
    }
    // set minimal gap_width
    /*if (Tx <= 2){
        if (gap_treatment)
            Tx=2;
        else
            Tx=3;
    }
    if (Ty <= 2){
        if (gap_treatment)
            Ty=2;
        else
            Ty=3;
    }*/

    ImageList* ccs = new ImageList();
    Point ul, lr;

    ul.x(0);
    ul.y(0);
    lr.x(image.ncols() - 1);
    lr.y(image.nrows() - 1);
    projection_cutting_intern(image, ul, lr, ccs, Tx, Ty, noise, gap_treatment, direction, Label);
    
    return ccs;
}




/*
sub_cc_analysis
@param cclist The list of CCs inside the image

@return A tuple with two values
    1. the image with the new labels from the new CCs
    2. a list of ImageLists
        a list-entry is a cc_analysis of a cclist from the argument
*/
template<class T>
PyObject* sub_cc_analysis(T& image, ImageVector &cclist) {
    unsigned int pos;
    int label = 2; // one is reserved for unlabeled pixels
    OneBitImageData *ret_image;
    OneBitImageView *ret_view;
    OneBitImageData *temp_image;
    OneBitImageView *temp_view;
    Cc* cc;
    ImageVector::iterator iv;
    ImageList::iterator il;
    typename T::value_type Black = black(image);

    ret_image = new OneBitImageData(image.dim(), image.origin());
    ret_view = new OneBitImageView(*ret_image, image.origin(), image.dim());
    temp_image = new OneBitImageData(image.dim(), image.origin());
    temp_view = new OneBitImageView(*temp_image, image.origin(), image.dim());

    // Generate a list to store the CCs of all lines
    PyObject *return_cclist = PyList_New(cclist.size());
    
    for (iv = cclist.begin(), pos = 0; iv != cclist.end(); iv++, pos++) {
        cc = static_cast<Cc*>(iv->first);

        // copy the needed CC from the original image(image) 
        // to the temporary image temp_view
        for (size_t y = 0; y < cc->nrows(); y++) {
          for (size_t x = 0; x < cc->ncols(); x++) {
            if (!is_white(cc->get(Point(x, y)))) {
              temp_view->set(Point(x+cc->offset_x()-temp_view->offset_x(), y+cc->offset_y()-temp_view->offset_y()), Black);
            }
          }
        }
        
        // generate a temp image for the cc_analysis,
        // it's simply a copy of one cclist entry
        OneBitImageView *cc_temp = new OneBitImageView(*temp_image, cc->origin(), cc->dim() );

        // Cc_analysis of one list entry
        ImageList* ccs_orig = cc_analysis(*cc_temp);

        ImageList* return_ccs = new ImageList();
        il = ccs_orig->begin();
        while (il != ccs_orig->end()) {
            cc = static_cast<Cc*>(*il);
           
            return_ccs->push_back(
                    new ConnectedComponent<typename T::data_type>(
                        *((typename T::data_type*)ret_view->data()),
                        OneBitPixel(label),
                        cc->origin(),
                        cc->dim()
                    )
                );
    
            // Copy CC over to return image
            for (size_t y = 0; y < cc->nrows(); y++) {
              for (size_t x = 0; x < cc->ncols(); x++) {
                if (!is_white(cc->get(Point(x, y)))) {
                  ret_view->set(Point(x+cc->offset_x()-ret_view->offset_x(), y+cc->offset_y()-ret_view->offset_y()), label);
                }
              }
            }

            // delete the temporary used CCs from the cc_analysis
            delete *il;

            il++;
            label++; // we use consecutive labels in return image
        }
        // remove copy of Cc in temporary image and clean up
        fill_white(*cc_temp);
        delete ccs_orig;
        delete cc_temp;

        // Set the Imagelist into the PyList
        // ImageList must be converted to be a valid datatype for the PyList
        PyList_SetItem(return_cclist, pos, ImageList_to_python(return_ccs));
        delete return_ccs;
    }
    // delete temporary image
    delete temp_view;
    delete temp_image;

    // Finaly create the return type, a tuple with a image 
    // and a list of ImageLists
    PyObject *return_values = PyTuple_New(2);
    PyTuple_SetItem(return_values, 0, create_ImageObject(ret_view));
    PyTuple_SetItem(return_values, 1, return_cclist);

    return return_values;
}


//
// evaluation of segmentation
//

// for distinguishing Ccs from Gccs and Sccs
class CcLabel {
public:
  char image; // 'G' or 'S'
  int  cclabel;
  CcLabel(char i, int c) {image = i; cclabel = c;}
  friend int operator<(const CcLabel& c1, const CcLabel& c2) { 
    if (c1.image == c2.image) return (c1.cclabel < c2.cclabel);
    else return c1.image < c2.image;
  }
};

// the plugin function
template<class T, class U>
IntVector* segmentation_error(T &Gseg, U &Sseg) {

  ImageList* Gccs = ccs_from_labeled_image(Gseg);
  ImageList* Sccs = ccs_from_labeled_image(Sseg);
  ImageList::iterator ccs_it;
  size_t x,y;
  int classlabel, Gclasslabel, Sclasslabel;
  CcLabel Gcclabel('G',0), Scclabel('S',0), cclabel('A',0);
  map<CcLabel,int> classoflabel; // cclabel -> classlabel
  multimap<int,CcLabel> labelsofclass; // classlabel -> cclabel
  typedef multimap<int,CcLabel>::iterator mm_iterator;
  mm_iterator mmit;
  pair<mm_iterator,mm_iterator> fromto;
  vector<CcLabel> tmplabels;
  vector<CcLabel>::iterator vit;

  // check for overlaps from Gseg
  for (ccs_it = Gccs->begin(), classlabel = 0; ccs_it != Gccs->end(); ++ccs_it, ++classlabel) {
    Gclasslabel = classlabel;
    Cc* cc = static_cast<Cc*>(*ccs_it);
    Gcclabel.cclabel = cc->label();
    classoflabel[Gcclabel] = Gclasslabel;
    labelsofclass.insert(make_pair(Gclasslabel,Gcclabel));
    for (y=0; y < cc->nrows(); y++)
      for (x=0; x < cc->ncols(); x++) {
        Scclabel.cclabel = Sseg.get(Point(cc->ul_x() + x, cc->ul_y() + y));
        // in case of overlap:
        if (Scclabel.cclabel) {
          // check whether segment from S is new
          if (classoflabel.find(Scclabel) == classoflabel.end()) {
            classoflabel[Scclabel] = Gclasslabel;
            labelsofclass.insert(make_pair(Gclasslabel,Scclabel));
          } else {
            Sclasslabel = classoflabel[Scclabel];
            if (Sclasslabel != Gclasslabel) {
              // unite both classes, i.e. move Sclasslabel into Gclasslabel
              tmplabels.clear();
              fromto = labelsofclass.equal_range(Sclasslabel);
              for (mmit = fromto.first; mmit != fromto.second; ++mmit) {
                cclabel = mmit->second;
                classoflabel[cclabel] = Gclasslabel;
                tmplabels.push_back(cclabel);
              }
              labelsofclass.erase(Sclasslabel);
              for (vit = tmplabels.begin(); vit != tmplabels.end(); ++vit)
                labelsofclass.insert(make_pair(Gclasslabel,*vit));
            }
          }
        }
      }
  }

  // check for CCs from Sseg without overlap (false positives)
  for (ccs_it = Sccs->begin(); ccs_it != Sccs->end(); ++ccs_it) {
    Cc* cc = static_cast<Cc*>(*ccs_it);
    Scclabel.cclabel = cc->label();
    if (classoflabel.find(Scclabel) == classoflabel.end()) {
        classlabel++;
        classoflabel[Scclabel] = classlabel;
        labelsofclass.insert(make_pair(classlabel,Scclabel));
    }
  }

  // build up class population numbers and classify error types
  int n1,n2,n3,n4,n5,n6,nG,nS;
  n1 = n2 = n3 = n4 = n5 = n6 = 0;
  for (mmit = labelsofclass.begin(); mmit != labelsofclass.end(); ) {
    nG = nS = 0;
    fromto = labelsofclass.equal_range(mmit->first);
    for (mmit = fromto.first; mmit != fromto.second; ++mmit) {
      if (mmit->second.image == 'G') nG++; else nS++;
    }
    // determine error category
    if (nG == 1 && nS == 1) n1++;
    else if (nG == 1 && nS == 0) n2++;
    else if (nG == 0 && nS == 1) n3++;
    else if (nG == 1 && nS  > 1) n4++;
    else if (nG  > 1 && nS == 1) n5++;
    else if (nG  > 1 && nS  > 1) n6++;
    else printf("Plugin segment_error: empty equivalence"
                " constructed which should not happen\n");
  }

  // clean up
  for (ccs_it = Sccs->begin(); ccs_it != Sccs->end(); ++ccs_it)
    delete *ccs_it;
  delete Sccs;
  for (ccs_it = Gccs->begin(); ccs_it != Gccs->end(); ++ccs_it)
    delete *ccs_it;
  delete Gccs;

  // build return value
  IntVector* errors = new IntVector();
  errors->push_back(n1); errors->push_back(n2);
  errors->push_back(n3); errors->push_back(n4);
  errors->push_back(n5); errors->push_back(n6);
  return errors;
}

} // end of namespace Gamera

#endif

