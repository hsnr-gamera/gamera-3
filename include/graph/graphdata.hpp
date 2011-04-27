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

#ifndef _GRAPHDATA_HPP_FAA9ABE84FC3A4
#define _GRAPHDATA_HPP_FAA9ABE84FC3A4

#ifdef __DEBUG_GAPI__
#include <iostream>
#endif



// -----------------------------------------------------------------------------
namespace Gamera { namespace GraphApi {

/** This base-class GraphData defines the interface for storing data in a graph.
 * For using this a derived class must override 
 *     int compare(const GraphData& b);
 * which returns 0 if *this == b, <0 if *this < b and >0 if *this > b.
 *
 * You can see a simple template-based implementation in GraphDataTemplate
 * which could be used for almost every data type whithout deriving a new class.
 *
 * If Data-Types with some reference-counting void incref() and void decref() 
 * are called when the value is needed internally in Graph more than one
 * time. Currently this feature is not used but should be in future.
 * */
struct GraphData {

   //really needed?
   virtual void incref() {};
   virtual void decref() {};

   /** This method returns
    * 0 if *this == b
    * <0 if *this < b
    * >0 if *this > b
    * != 0 if *this != b
    * */
   virtual int compare(const GraphData& b) = 0;
   
   virtual ~GraphData() {

   }

   virtual GraphData* copy() = 0;



   // mappings from comparison operators to int compare(const GraphData&)
   bool operator<(const GraphData& b) {
      return compare(b) < 0;
   }

   bool operator==(const GraphData& b) {
      return compare(b) == 0;
   }

   bool operator>(const GraphData& b) {
      return compare(b) > 0;
   }

   bool operator!=(const GraphData& b) {
      return compare(b) != 0;
   }

   bool operator<=(const GraphData& b) {
      return compare(b) <= 0;
   }

   bool operator>=(const GraphData& b) {
      return compare(b) >= 0;
   }
};


/// Less-Compare for use in value-node-map of Graph
struct GraphDataPtrLessCompare {
   bool operator()(GraphData* a, GraphData* b){
      return *a < *b;
   }
};



}} // end Gamera::GraphApi
#endif /* _GRAPHDATA_HPP_FAA9ABE84FC3A4 */

