/*
 *
 * Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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

#define GAMERACORE_INTERNAL
#include "gameramodule.hpp"

using namespace Gamera;

/*
  These are prototyped here because this is the only file in which
  they will be used. They are defined in the .cpp files for each of
  the types.
*/
void init_SizeType(PyObject* module_dict);
void init_PointType(PyObject* module_dict);
void init_DimensionsType(PyObject* module_dict);
void init_RectType(PyObject* module_dict);
void init_RegionType(PyObject* module_dict);
void init_RegionMapType(PyObject* module_dict);
void init_RGBPixelType(PyObject* module_dict);
void init_ImageDataType(PyObject* module_dict);
void init_ImageType(PyObject* module_dict);
void init_ImageInfoType(PyObject* module_dict);
void init_ListUtilities(PyObject* module_dict);
void init_IteratorType(PyObject* module_dict);

extern "C" {
  DL_EXPORT(void) initgameracore(void);
}

PyMethodDef gamera_module_methods[] = {
  {NULL, NULL },
};

DL_EXPORT(void)
initgameracore(void) {
  PyObject* m = Py_InitModule("gameracore", gamera_module_methods);
  PyObject* d = PyModule_GetDict(m);
  
  init_SizeType(d);
  init_PointType(d);
  init_DimensionsType(d);
  init_RectType(d);
  init_RegionType(d);
  init_RegionMapType(d);
  init_RGBPixelType(d);
  init_ImageDataType(d);
  init_ImageType(d);
  init_ImageInfoType(d);
  init_IteratorType(d);
}
