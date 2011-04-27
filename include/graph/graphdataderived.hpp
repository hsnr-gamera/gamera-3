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

#ifndef _GRAPHDATADERIVED_HPP_279BB122FA1E4E
#define _GRAPHDATADERIVED_HPP_279BB122FA1E4E

#include "graphdata.hpp"
#include <string>

namespace Gamera { namespace GraphApi {

   
   
// -----------------------------------------------------------------------------
struct GraphDataInt: public GraphData {
   int data;

   GraphDataInt(int d) {
      data = d;
   }

   virtual int compare(const GraphData& b) {
#ifdef __DEBUG_GAPI__
      std::cout << "GrahpDataInt::compare called\n";
#endif
      return data - (dynamic_cast<const GraphDataInt&>(b)).data;
   }

   GraphData* copy() {
      return new GraphDataInt(data);
   }
};



// -----------------------------------------------------------------------------
struct GraphDataLong: public GraphData {
   long data;

   GraphDataLong(long d) {
      data = d;
   }

   virtual int compare(const GraphData& b) {
#ifdef __DEBUG_GAPI__
      std::cout << "GrahpDataInt::compare called\n";
#endif
      return data - (dynamic_cast<const GraphDataLong&>(b)).data;
   }

   GraphData* copy() {
      return new GraphDataLong(data);
   }
};



// -----------------------------------------------------------------------------
struct GraphDataUnsignedInt: public GraphData {
   unsigned int data;

   GraphDataUnsignedInt(unsigned int d) {
      data = d;
   }

   virtual int compare(const GraphData& b) {
#ifdef __DEBUG_GAPI__
      std::cout << "GrahpDataUnsignedInt::compare called\n";
#endif
      const GraphDataUnsignedInt& c = dynamic_cast<const GraphDataUnsignedInt&>(b);
      if(data < c.data)
         return - 1;
      else if(data > c.data)
         return 1;
      else
         return 0;
   }

   GraphData* copy() {
      return new GraphDataUnsignedInt(data);
   }
};



// -----------------------------------------------------------------------------
struct GraphDataString: public GraphData {
   std::string data;

   GraphDataString(std::string d) {
      data = d;
   }

   virtual int compare(const GraphData& b) {
#ifdef __DEBUG_GAPI__
      std::cout << "GrahpDataUnsignedInt::compare called\n";
#endif
      const GraphDataString& c = dynamic_cast<const GraphDataString&>(b);
      if(data < c.data)
         return - 1;
      else if(data > c.data)
         return 1;
      else
         return 0;
   }

   GraphData* copy() {
      return new GraphDataString(data);
   }
};



// -----------------------------------------------------------------------------
template <typename T>
struct GraphDataTemplate: public GraphData {
   T data;

   GraphDataTemplate(T d) {
      data = d;
   }

   virtual int compare(const GraphData& b) {
      const GraphDataTemplate& c = dynamic_cast<const GraphDataTemplate<T>&>(b);
      if(data < c.data)
         return - 1;
      else if(c.data < data)
         return 1;
      else
         return 0;
   }

   GraphData* copy() {
      return new GraphDataTemplate<T>(data);
   }
};



}} //end Gamera::GraphApi
#endif /* _GRAPHDATADERIVED_HPP_279BB122FA1E4E */

