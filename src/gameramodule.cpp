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

#include "gameramodule.hpp"

using namespace Gamera;

void init_SizeType(PyObject* module_dict);
void init_PointType(PyObject* module_dict);
void init_DimensionsType(PyObject* module_dict);
void init_RectType(PyObject* module_dict);
void init_ImageDataType(PyObject* module_dict);
void init_ImageType(PyObject* module_dict);


extern "C" {
  void initgameracore(void);
}

PyMethodDef gamera_module_methods[] = {
  {NULL, NULL},
};

DL_EXPORT(void)
initgameracore(void) {
  PyObject* m = Py_InitModule("gameracore", gamera_module_methods);
  PyObject* d = PyModule_GetDict(m);
  init_SizeType(d);
  init_PointType(d);
  init_DimensionsType(d);
  init_RectType(d);
  init_ImageDataType(d);
  init_ImageType(d);
}
