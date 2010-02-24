# -*- mode: python; indent-tabs-mode: nil; tab-width: 3 -*-
# vim: set tabstop=3 shiftwidth=3 expandtab:
#
# Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, Karl MacMillan
#               2010      Christoph Dalitz
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#

from pyplate import *
from os import path
import os
import sys
import re
from distutils.core import Extension
from distutils.dep_util import newer
from distutils import sysconfig
from gamera import paths, args_wrappers

global std_import
global plugins_to_ignore

# magic_import and magic_import_setup
#
# This allows us to ignore a list of modules passed into
# magic_import_setup. generate_plugin uses this to prevent
# the loading of C++ modules that may not exist yet during
# the build process.
def magic_import(name, globals_={}, locals_={}, fromlist=[], level=-1):
   if fromlist != None and "core" in fromlist:
      fromlist = list(fromlist)
      fromlist.remove("core")

   for x in plugins_to_ignore:
      if name == x:
         return None

   if float(sys.version[0:3]) < 2.45:
      return std_import(name, globals_, locals_, fromlist)
   else:
      return std_import(name, globals_, locals_, fromlist, level)

def magic_import_setup(ignore):
   global plugins_to_ignore
   global std_import
   plugins_to_ignore = ignore
   # Save the standard __import__ function so we can chain to it
   std_import = __builtins__['__import__']
   # Override the __import__ function with our new one
   __builtins__['__import__'] = magic_import

def restore_import():
   global std_import
   __builtins__['__import__'] = std_import

template = Template("""
  [[exec from os import path]]
  [[exec from enums import *]]
  [[exec from plugin import *]]
  [[exec from util import get_pixel_type_name]]

  [[# This should be included first in order to avoid libpng.h/setjmp.h problems. #]]
  [[if module.__class__.__name__ == "PngSupportModule"]]
    #include <png.h>
  [[end]]

  #include \"gameramodule.hpp\"
  #include \"knnmodule.hpp\"

  [[# include the headers that the module needs #]]
  [[for header in module.cpp_headers]]
    #include \"[[header]]\"
  [[end]]

  [[# Standard headers used in the plugins #]]
  #include <string>
  #include <stdexcept>
  #include \"Python.h\"
  #include <list>

  using namespace Gamera;
  [[for x in module.cpp_namespaces]]
    using namespace [[x]];
  [[end]]

  [[# Generate the plugin path and module name from the filename. #]]
  [[# The module name for our purposes will be prefixed with an underscore #]]
  [[exec plug_path, filename = path.split(__file__)]]
  [[exec module_name = '_' + filename.split('.')[0] ]]

  [[# Declare all of the functions - because this is a C++ file we have to #]]
  [[# declare the functions as C functions so that Python can access them #]]
  extern \"C\" {
#ifndef _MSC_VER
    void init[[module_name]](void);
#endif
    [[for function in module.functions]]
      [[if not function.pure_python]]
        static PyObject* call_[[function.__name__]](PyObject* self, PyObject* args);
      [[end]]
    [[end]]
  }

  [[# Create the list of methods for the module - the name of the function #]]
  [[# is derived from the name of the class implementing the function - #]]
  [[# also, the function name is prepended with call_ so that there are no clashes #]]
  [[# with the real plugin functions #]]
  static PyMethodDef [[module_name]]_methods[] = {
    [[for function in module.functions]]
      [[if not function.pure_python]]
        { CHAR_PTR_CAST \"[[function.__name__]]\",
          call_[[function.__name__]], METH_VARARGS,
          CHAR_PTR_CAST [[function.escape_docstring()]]
        },
      [[end]]
    [[end]]
    { NULL }
  };

  [[# Each module can declare several functions so we loop through and generate wrapping #]]
  [[# code for each function #]]
  [[for function in module.functions]]
    [[if not function.pure_python]]
      static PyObject* call_[[function.__name__]](PyObject* self, PyObject* args) {
      [[# this holds the self argument - note that the self passed into the function will #]]
      [[# be Null because this functions is not actually bound to an object #]]

      PyErr_Clear();
      [[if function.self_type == None]]
        [[exec args = function.args.list]]
      [[else]]
        [[exec args = [function.self_type] + function.args.list]]
        [[exec function.self_type.name = 'self']]
      [[end]]
      [[# for each argument insert the appropriate conversion code into the string that will #]]
      [[# be passed to PyArg_ParseTuple and create a variable to hold the result. #]]
      [[if function.return_type != None]]
        [[exec function.return_type.name = 'return']]
        [[exec function.return_type.convert_from_PyObject = True]]
        [[if not function.feature_function]]
          [[function.return_type.declare()]]
        [[end]]
      [[end]]
      [[exec pyarg_format = '']]
      [[for arg in args]]
        [[exec pyarg_format += arg.arg_format]]
        [[arg.declare()]]
      [[end]]

      [[# Now that we have all of the arguments and variables for them we can parse #]]
      [[# the argument tuple. #]]
      [[if function.feature_function]]
         int offset = -1;
         if (PyArg_ParseTuple(args, CHAR_PTR_CAST \"O|i:[[function.__name__]]\",&[[function.self_type.pysymbol]], &offset) <= 0)
           return 0;
      [[else]]
         [[if pyarg_format != '']]
           if (PyArg_ParseTuple(args, CHAR_PTR_CAST \"[[pyarg_format]]:[[function.__name__]]\"
           [[for arg in args]]
             ,
             &[[arg.pysymbol]]
           [[end]]
           ) <= 0)
           return 0;
         [[end]]
      [[end]]

      [[for arg in args]]
        [[arg.from_python()]]
      [[end]]

      [[if function.feature_function]]
         feature_t* feature_buffer = 0;
         if (offset < 0) {
           feature_buffer = new feature_t[ [[function.return_type.length]] ];
         } else {
           if (self_arg->features_len < offset + [[function.return_type.length]]) {
             PyErr_Format(PyExc_ValueError, \"Offset as given (%d) will cause data to be written outside of array of length (%d).  Perhaps the feature array is not initialised?\", offset, (int)self_arg->features_len);
             return 0;
           }
           feature_buffer = self_arg->features + offset;
         }
         [[args[0].call(function, args[1:], [])]]
      [[else]]
        try {
          [[if len(args)]]
            [[args[0].call(function, args[1:], [])]]
          [[else]]
            [[if function.return_type != None]]
              [[function.return_type.symbol]] =
            [[end]]
            [[function.__name__]]([[if function.progress_bar]]ProgressBar("[[function.progress_bar]]")[[else]][[end]]);
          [[end]]
        } catch (std::exception& e) {
          PyErr_SetString(PyExc_RuntimeError, e.what());
          return 0;
        }
      [[end]]

      [[if function.feature_function]]
         if (offset < 0) {
           PyObject* str = PyString_FromStringAndSize((char*)feature_buffer, [[function.return_type.length]] * sizeof(feature_t));
           if (str != 0) {
              [[# This is pretty expensive, but simple #]]
              PyObject* array_init = get_ArrayInit();
              if (array_init == 0)
                return 0;
              PyObject* array = PyObject_CallFunction(
                    array_init, (char *)\"sO\", (char *)\"d\", str);
              Py_DECREF(str);
              delete[] feature_buffer;
              return array;
           } else {
             delete[] feature_buffer;
             return 0;
           }
         } else {
           Py_INCREF(Py_None);
           return Py_None;
         }
      [[else]]
        [[for arg in function.args]]
          [[arg.delete()]]
        [[end]]
        [[if function.return_type == None]]
          Py_INCREF(Py_None);
          return Py_None;
        [[else]]
          [[if isinstance(function.return_type, (ImageType, Class))]]
            if ([[function.return_type.symbol]] == NULL) {
              if (PyErr_Occurred() == NULL) {
                Py_INCREF(Py_None);
                return Py_None;
               } else
                return NULL;
            } else {
              [[function.return_type.to_python()]]
              return return_pyarg;
            }
          [[else]]
            [[function.return_type.to_python()]]
            return return_pyarg;
          [[end]]
        [[end]]
      [[end]]
      }
    [[end]]
  [[end]]

  DL_EXPORT(void) init[[module_name]](void) {
    Py_InitModule(CHAR_PTR_CAST \"[[module_name]]\", [[module_name]]_methods);
  }
  """)

def generate_plugin(plugin_filename, location, compiling_gamera,
                    extra_compile_args=[], extra_link_args=[], libraries=[],
                    define_macros=[]):
  from gamera import gamera_setup

  plug_path, filename = path.split(plugin_filename)
  module_name = filename.split('.')[0]
  cpp_filename = path.join(plug_path, "_" + module_name + ".cpp")

  regenerate = False
  if newer(plugin_filename, cpp_filename) or '-f' in sys.argv:
    regenerate = True

  sys.path.append(plug_path)

  #import plugin
  plugin_module = __import__(module_name)
  if not hasattr(plugin_module, 'module'):
     return None
  if plugin_module.module.pure_python:
     return None

  # see if any of the header files have changed since last time
  # we compiled
  include_dirs = (["include", plug_path, "include/plugins"] +
                  plugin_module.module.cpp_include_dirs)
  if not compiling_gamera:
     include_dirs.extend(gamera_setup.get_gamera_include_dirs())
  if not regenerate:
    for header in plugin_module.module.cpp_headers:
      found_header = 0
      for include_dir in include_dirs:
        header_filename = path.join(include_dir, header)
        if path.exists(header_filename):
          found_header = 1
          if newer(header_filename, cpp_filename):
            regenerate = True
            break
          break
      if regenerate:
        break

  if regenerate:
    print "generating wrappers for", module_name, "plugin"
    template.execute_file(cpp_filename, plugin_module.__dict__)
  else:
    print "skipping wrapper generation for", module_name, "plugin (output up-to-date)"

  # make the a distutils extension class for this plugin
  cpp_files = [cpp_filename]
  for file in plugin_module.module.cpp_sources:
    cpp_files.append(file)

  extra_libraries = plugin_module.module.extra_libraries
  # This is to make up for a bug in distutils.
  if '--compiler=mingw32' in sys.argv or not sys.platform == 'win32':
     if "stdc++" not in extra_libraries:
        extra_libraries.append("stdc++")
  return Extension(location + "._" + module_name, cpp_files,
                   include_dirs=include_dirs,
                   library_dirs=plugin_module.module.library_dirs,
                   libraries=extra_libraries,
                   extra_compile_args=plugin_module.module.extra_compile_args + extra_compile_args,
                   extra_link_args=plugin_module.module.extra_link_args + extra_link_args,
                   define_macros=plugin_module.module.define_macros + define_macros,
                   extra_objects=plugin_module.module.extra_objects)

