/*
 *
 * Copyright (C) 2002 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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

#ifndef KWM06292002_imagemodule
#define KWM06292002_imagemodule

#include "gamera.hpp"
#include <Python.h>

struct SizeObject {
  PyObject_HEAD
  IntSize* m_x;
};

extern bool is_SizeObject(PyObject* x);
extern PyObject* create_SizeObject(const IntSize& d);

struct DimensionsObject {
  PyObject_HEAD
  IntDimensions* m_x;
};

extern bool is_DimensionsObject(PyObject* x);
extern PyObject* create_DimensionsObject(const IntDimensions& d);

struct PointObject {
  PyObject_HEAD
  IntPoint* m_x;
};

extern bool is_PointObject(PyObject* x);
extern PyObject* create_PointObject(const IntPoint& p);

struct RectObject {
  PyObject_HEAD
  IntRect* m_x;
};

bool is_RectObject(PyObject* x);

#endif
