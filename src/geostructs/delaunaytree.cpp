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
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//


//
// This data structure is only available in C++
// For a Delaunay triangulation in Python,
// use the Gamera plugin delaunay_from_points()
//

#include "geostructs/delaunaytree.hpp"

//-------------------------------------------------------------------------
// data structure for computing the two dimensional Delaunay triangulation
//-------------------------------------------------------------------------

namespace Gamera { namespace Delaunaytree {

  // Vertex
  Vertex::Vertex(double x, double y) {
    this->x = x;
    this->y = y;
    this->label = -1;
  }

  Vertex::Vertex(double x, double y, int label) {
    this->x = x;
    this->y = y;
    this->label = label;
  }

  double Vertex::getX() {
    return this->x;
  }

  double Vertex::getY() {
    return this->y;
  }

  int Vertex::getLabel() {
    return this->label;
  }

  inline Vertex operator+(Vertex a, Vertex b) { 
    return Vertex(a.x + b.x, a.y + b.y);
  }

  inline Vertex operator-(Vertex a, Vertex b) { 
    return Vertex(a.x - b.x, a.y - b.y);
  }

  inline double operator*(Vertex a, Vertex b) { 
    return a.x * b.x + a.y * b.y;
  }

  inline double operator^(Vertex a, Vertex b) { 
    return a.x * b.y - a.y * b.x;
  }

  //TriangleFlag
  TriangleFlag::TriangleFlag() {
    this->flag = 0;
  }

  void TriangleFlag::kill() {
    this->flag |= 16;
  }

  bool TriangleFlag::isDead() {
    return this->flag & 16;
  }

  void TriangleFlag::setInfinite(int i) {
    this->flag |= i;
  }

  int TriangleFlag::isInfinite() {
    return this->flag & 7;
  }

  void TriangleFlag::setLastFinite() {
    this->flag |= 8;
  }

  bool TriangleFlag::isLastFinite() {
    return this->flag & 8;
  }

  int TriangleFlag::getFlag() {
    return flag;
  }

  // TriangleList
  TriangleList::TriangleList(TriangleList *list, Triangle *triangle) {
    this->next = list;
    this->triangle = triangle;
  }

  TriangleList::~TriangleList() {
    if(next) {
		delete next;
		next = NULL;
		triangle = NULL;
    }
  }

  Triangle * TriangleList::getTriangle() {
    return this->triangle;
  }

  TriangleList * TriangleList::getNext() {
    return this->next;
  }

  // DelaunayTree
  DelaunayTree::DelaunayTree() {
    this->number = 0;
    this->root = new Triangle(this);
	
    new Triangle(this, root, 0);
    new Triangle(this, root, 1);
    new Triangle(this, root, 2);
	
    this->root->getNeighbor(0)->setNeighbor(1, this->root->getNeighbor(1));
    this->root->getNeighbor(0)->setNeighbor(2, this->root->getNeighbor(2));
    this->root->getNeighbor(1)->setNeighbor(0, this->root->getNeighbor(0));
    this->root->getNeighbor(1)->setNeighbor(2, this->root->getNeighbor(2));
    this->root->getNeighbor(2)->setNeighbor(0, this->root->getNeighbor(0));
    this->root->getNeighbor(2)->setNeighbor(1, this->root->getNeighbor(1));
  }

  DelaunayTree::~DelaunayTree() {

    delete root->getVertex(0);
    delete root->getVertex(1);
    delete root->getVertex(2);

    std::vector<Triangle*>::iterator it;
	
    for( it = triangles.begin() ; it != triangles.end() ; ++it ) {
		delete *it;
    }
  }

  void DelaunayTree::appendTriangle(Triangle *t) {
    this->triangles.push_back(t);
  }

  void DelaunayTree::addVertex(Vertex *v) {
    Triangle *n, *first, *last, *created;
    Vertex *q, *r;
    int i;
	
    this->root->setNumber( ++this->number );
	
    n = this->root->findConflict(v);
    if(!n) {
		return;
    }
	
    n->getFlag()->kill();
	
    for(i = 0; i < (3 - n->getFlag()->isInfinite()); ++i) {
      if( ( v->getX() == n->getVertex(i)->getX() )
          && ( v->getY() == n->getVertex(i)->getY() ) ) {
        throw std::runtime_error("This should NEVER happen :(");
      }
    }
	
    q = n->getVertex(0);
    while( n->getNeighbor( i = n->cwNeighbor(q) )->Conflict(v) ) {
  	n = n->getNeighbor(i);
  	n->getFlag()->kill();
    }
  
    first = last = new Triangle(this, n, v, i);
  
    r = n->getVertex((i+2) % 3);
  
    while(true) {
  	i = n->cwNeighbor(r);
  	if(n->getNeighbor(i)->getFlag()->isDead()) {
        n = n->getNeighbor(i);
        continue;
  	}
  	
  	if(n->getNeighbor(i)->Conflict(v)) {
        n = n->getNeighbor(i);
        n->getFlag()->kill();
        continue;
  	}
  	
  	break;
    }
  
    while(true) {
  	created = new Triangle(this, n, v, i);
  	created->setNeighbor(2, last);
  	last->setNeighbor(1, created);
  	last = created;
  	r = n->getVertex((i+2) % 3);
  	
  	if(r==q)  {
        break;
		}
		
		while(true) {
        i = n->cwNeighbor(r);
        if(n->getNeighbor(i)->getFlag()->isDead()) {
          n = n->getNeighbor(i);
          continue;
        }
  	
        if(n->getNeighbor(i)->Conflict(v)) {
          n = n->getNeighbor(i);
          n->getFlag()->kill();
          continue;
        }
  
        break;
  	}
    }
  
    first->setNeighbor(2, last);
    last->setNeighbor(1, first);
    return;
  }

  // returns all neighboring labels as a map label->{neighbor1, ...}
  // every neighbor pair is only listed once in the set for the smaller label
  void DelaunayTree::neighboringLabels(std::map<int,std::set<int> > *labelmap) {

    std::map<Vertex*,std::set<Vertex*> > vertexmap;
    std::map<Vertex*,std::set<Vertex*> >::iterator vit1;
    std::set<Vertex*>::iterator vit2;
    std::set<int> emptyset;
    int label1, label2;

    root->setNumber(++number); // termination criterium
    root->neighboringVertices(&vertexmap);

    // TODO: resolve collinear edges
    // (when two edges are collinear, the longer one is to be removed)
    // ...

    // copy over result to labelmap
    for (vit1=vertexmap.begin(); vit1!=vertexmap.end(); vit1++) {
      for (vit2=vit1->second.begin(); vit2!=vit1->second.end(); vit2++) {
        label1 = (vit1->first)->getLabel();
        label2 = (*vit2)->getLabel();
        if (label1 < label2) {
          if (labelmap->find(label1) == labelmap->end())
            (*labelmap)[label1] = emptyset;
          (*labelmap)[label1].insert(label2);
        } else if (label1 > label2) {
          if (labelmap->find(label2) == labelmap->end())
            (*labelmap)[label2] = emptyset;
          (*labelmap)[label2].insert(label1);
        }
      }
    }
  }

  // Triangle
  Triangle::Triangle(DelaunayTree *tree) {
    tree->appendTriangle(this);
	
    this->vertices[0] = new Vertex(1.0, 0.0);
    this->vertices[1] = new Vertex(-0.5, 0.8660254);
    this->vertices[2] = new Vertex(-0.5, -0.8660254);
	
    this->flag.setInfinite(3);
    this->number = 0;
    this->sons = NULL;
  }

  Triangle::Triangle(DelaunayTree *tree, Triangle *parent, int i) {
    tree->appendTriangle(this);
	
    this->vertices[0] = parent->vertices[0];
    this->vertices[1] = parent->vertices[1];
    this->vertices[2] = parent->vertices[2];
	
    this->flag.setInfinite(4);
    this->number = 0;
    this->sons = NULL;
	
    this->neighbors[i] = parent;
    parent->neighbors[i] = this;
  }

  Triangle::Triangle(DelaunayTree *tree, Triangle *parent, Vertex *v, int i) {
    tree->appendTriangle(this);
	
    switch( parent->flag.isInfinite() ) {
    case 0:
		flag.setInfinite(0);
		break;
    case 1:
		if( parent->flag.isLastFinite() ) {
        flag.setInfinite( (i==1) ? 0 : 1 );
		} else {
        flag.setInfinite( (i==2) ? 0 : 1 );
		}
		
		if(flag.isInfinite()) {
        if(parent->flag.isLastFinite()) {
          if(i==0) { flag.setLastFinite(); }
        } else {
          if(i==1) { flag.setLastFinite(); }
        }
		}
		break;
		
    case 2:
		flag.setInfinite( (i==0 ) ? 2 : 1);
		if(i==1) { flag.setLastFinite(); }
		break;
    case 3:
		flag.setInfinite(2);
		break;
    }
	
    number = 0;
    sons = NULL;
    parent->sons = new TriangleList( parent->sons, this );
    parent->neighbors[i]->sons = new TriangleList(parent->neighbors[i]->sons, this);
    parent->neighbors[i]->neighbors[ parent->neighbors[i]->NeighborIndex(parent) ] = this;
    vertices[0] = v;
    neighbors[0] = parent->neighbors[i];
	
    switch(i) {
    case 0:
		vertices[1] = parent->vertices[1];
		vertices[2] = parent->vertices[2];
		break;
	
    case 1:
		vertices[1] = parent->vertices[2];
		vertices[2] = parent->vertices[0];
		break;
	
    case 2:
		vertices[1] = parent->vertices[0];
		vertices[2] = parent->vertices[1];
		break;
    }
  }

  Triangle::~Triangle() {
    if(sons) {
		delete sons;
    }
  }

  Triangle * Triangle::findConflict(Vertex *v) {
    if( !Conflict(v) ) {
		return NULL;
    }
	
    if(!flag.isDead()) {
		return this;
    }
	
    for(TriangleList *l = sons ; l ; l = l->getNext()) {
		if( l->getTriangle()->number != this->number ) {
        l->getTriangle()->number = this->number;
        Triangle *n = l->getTriangle()->findConflict(v);
        if(n) {
          return n;
        }
		}
    }
	
    return NULL;
  }

  bool Triangle::Conflict(Vertex *v) {
    switch( this->flag.isInfinite() ) {
    case 4:
		return false;
    case 3:
		return true;
    case 2:
		return ( ( *v - *vertices[0]  ) * ( *vertices[1] + *vertices[2] ) >= 0 );
    case 1:
		if( this->flag.isLastFinite() ) {
        return (  (( *v - *vertices[2] ) ^ ( *vertices[2] - *vertices[0] )) >= 0 );
		} else {
        return (  (( *v - *vertices[0] ) ^ ( *vertices[0] - *vertices[1] )) >= 0 );
		}
    case 0:
		double x = v->getX();
      double y = v->getY();
      
      double x0 = vertices[0]->getX();
      double y0 = vertices[0]->getY();
      double x1 = vertices[1]->getX();
      double y1 = vertices[1]->getY();
      double x2 = vertices[2]->getX();
      double y2 = vertices[2]->getY();
      
      x1 -= x0;
      y1 -= y0;
      x2 -= x0;
      y2 -= y0;
      x -= x0;
      y -= y0;
      
      double z1 = (x1*x1)+(y1*y1);
      double z2 = (x2*x2)+(y2*y2);
      
      double alpha = (y1*z2)-(z1*y2);
      double beta = (x2*z1)-(x1*z2);
      double gamma = (x1*y2)-(y1*x2);
      
      return ((alpha*x)+(beta*y)+gamma*((x*x)+(y*y)) <= 0);
    }
	
    return false;
  }

  int Triangle::NeighborIndex(Triangle *t) {
    return ( (this->neighbors[0] == t) ? 0 : ( (this->neighbors[1] == t) ? 1 : 2) );
  }

  int Triangle::cwNeighbor(Vertex *v) {
    return ( (v == this->vertices[0]) ? 2 : ( (v == vertices[1]) ? 0 : 1) );
  }

  TriangleFlag * Triangle::getFlag() {
    return &this->flag;
  }

  Vertex * Triangle::getVertex(int i) {
    return this->vertices[i];
  }

  void Triangle::setNeighbor(int i, Triangle *t) {
    this->neighbors[i] = t;
  }

  Triangle * Triangle::getNeighbor(int i) {
    return this->neighbors[i];
  }

  void Triangle::setNumber(int i) {
    this->number = i;
  }

  int Triangle::getNumber() {
    return this->number;
  }

  // adds the neighboring vertices into the map neighbors
  void Triangle::neighboringVertices(std::map<Vertex*,std::set<Vertex*> > *allneighbors) {
    TriangleList* l;
    std::set<Vertex*> emptyset;

    if( flag.isDead() ) {
      for( l = sons; l; l = l->getNext() ) {
        if( l->getTriangle()->number != number ) {
          l->getTriangle()->number = number;
          l->getTriangle()->neighboringVertices(allneighbors);
        }
      }
      return;
    }
	
    if(neighbors[0]->number != number) {
      if(!flag.isInfinite()) {
        if(vertices[1]->getLabel() >= 0 && vertices[2]->getLabel() >= 0) {
          if (allneighbors->find(vertices[1]) == allneighbors->end())
            (*allneighbors)[vertices[1]] = emptyset;
          (*allneighbors)[vertices[1]].insert(vertices[2]);
          if (allneighbors->find(vertices[2]) == allneighbors->end())
            (*allneighbors)[vertices[2]] = emptyset;
          (*allneighbors)[vertices[2]].insert(vertices[1]);
        }
      }
    }
	
    if (neighbors[1]->number != number) {
      if (!flag.isInfinite() || 
          (flag.isInfinite() == 1 && flag.isLastFinite())) {
        if (vertices[2]->getLabel() >= 0 && vertices[0]->getLabel() >= 0) {
          if (allneighbors->find(vertices[2]) == allneighbors->end())
            (*allneighbors)[vertices[2]] = emptyset;
          (*allneighbors)[vertices[2]].insert(vertices[0]);
          if (allneighbors->find(vertices[0]) == allneighbors->end())
            (*allneighbors)[vertices[0]] = emptyset;
          (*allneighbors)[vertices[0]].insert(vertices[2]);
        }
      	
      }
    }
	
    if (neighbors[2]->number != number) {
      if (!flag.isInfinite() ||
          (flag.isInfinite() == 1 && !flag.isLastFinite())) {
        if (vertices[0]->getLabel() >= 0 && vertices[1]->getLabel() >= 0) {
          if (allneighbors->find(vertices[1]) == allneighbors->end())
            (*allneighbors)[vertices[1]] = emptyset;
          (*allneighbors)[vertices[1]].insert(vertices[0]);
          if (allneighbors->find(vertices[0]) == allneighbors->end())
            (*allneighbors)[vertices[0]] = emptyset;
          (*allneighbors)[vertices[0]].insert(vertices[1]);
        }
      }
    }    
  }


}} // end namespace Gamera::Delaunaytree
