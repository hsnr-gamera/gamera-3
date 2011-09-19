#ifndef DELAUNAYTREE_20100430_HPP
#define DELAUNAYTREE_20100430_HPP

//
// Copyright (C) 2010 Oliver Christen, Christoph Dalitz
//
// This code is based on the Delaunay_Tree implementation
// http://people.sc.fsu.edu/~burkardt/cpp_src/delaunay_tree_2d/
// with the kind permission by Olivier Devillers.
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//

#include <vector>
#include <map>
#include <set>
#include <stdexcept>

//-------------------------------------------------------------------------
// data structure for computing the two dimensional Delaunay triangulation
//-------------------------------------------------------------------------

namespace Gamera { namespace Delaunaytree {

  class Vertex;
  class TriangleFlag;
  class TriangleList;
  class DelaunayTree;
  class Triangle;

  // Vertex
  class Vertex {
  private:
    double x;
    double y;
    int label;
  public:
    Vertex(double x, double y);
    Vertex(double x, double y, int label);
    double getX();
    double getY();
    int getLabel();
	
    friend Vertex operator+(Vertex a, Vertex b);
    friend Vertex operator-(Vertex a, Vertex b);
    friend double operator*(Vertex a, Vertex b);
    friend double operator^(Vertex a, Vertex b);
  };

  // TriangleFlag
  class TriangleFlag {
  private:
    int flag;
  public:
    TriangleFlag();
    void kill();
    bool isDead();
    void setInfinite(int i);
    int isInfinite();
    void setLastFinite();
    bool isLastFinite();
    int getFlag();
  };

  // TriangleList
  class TriangleList {
  private:
    Triangle *triangle;
    TriangleList *next;
  public:
    TriangleList(TriangleList *list, Triangle *triangle);
    ~TriangleList();
    Triangle * getTriangle();
    TriangleList * getNext();
  };

  // DelaunayTree
  class DelaunayTree {
  private:
    int number;
    Triangle *root;
    std::vector<Triangle*> triangles;
  public:
    DelaunayTree();
    ~DelaunayTree();
    void addVertex(Vertex *v);
    void addVertices(std::vector<Vertex*> *vertices);
    void appendTriangle(Triangle *t);
    void neighboringLabels(std::map<int,std::set<int> > *lbmap);
    void neighboringVertices(std::map<Vertex*,std::set<Vertex*> > *vmap);
  };

  // Triangle
  class Triangle {
  private:
    int number;
    TriangleFlag flag;
    Vertex *vertices[3];
    Triangle *neighbors[3];
    TriangleList *sons;
  public:
    Triangle(DelaunayTree *tree);
    Triangle(DelaunayTree *tree, Triangle *parent, int i);
    Triangle(DelaunayTree *tree, Triangle *parent, Vertex *v, int i);
    ~Triangle();
    bool Conflict(Vertex *v);
    Triangle * findConflict(Vertex *v);
    TriangleFlag * getFlag();
    Vertex * getVertex(int i);
    void setNeighbor(int i, Triangle *t);
    Triangle * getNeighbor(int i);
    void setNumber(int i);
    int getNumber();
    int NeighborIndex(Triangle *t);
    int cwNeighbor(Vertex *v);
    void neighboringVertices(std::map<Vertex*,std::set<Vertex*> > *allneighbors);
    void neighboringLabels(std::map<int,std::set<int> > *allneighbors);
  };

}} // end namespace Gamera::Delaunaytree

#endif
