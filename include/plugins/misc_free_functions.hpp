/*
 *
 * Copyright (C) 2010      Christoph Dalitz
 * 
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

#ifndef cd23022010_mistfreefunctions
#define cd23022010_mistfreefunctions

#include <Python.h>
#include <limits>

namespace Gamera {

  PyObject* range_of_float() {
    return Py_BuildValue("dd",
                         std::numeric_limits<float>::min(),
                         std::numeric_limits<float>::max());
  }


}

#endif
