# vi:set tabsize=3:
#
# Copyright (C) 2001, 2002 Ichiro Fujinaga, Michael Droettboom,
#                          and Karl MacMillan
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

from gamera.args import *
from gamera import paths
import new, os, os.path, unittest, imp, inspect
from enums import ONEBIT, GREYSCALE, GREY16, RGB, FLOAT

class PluginModule:
   category = None
   cpp_namespaces = []
   cpp_sources = []
   cpp_headers = []
   cpp_defines = []
   cpp_include_dirs = []
   extra_libraries = []
   library_dirs = []
   define_macros = []
   extra_compile_args = []
   extra_link_args = []
   extra_objects = []
   functions = []
   pure_python = 0
   version = "1.0"
   author = ""
   url = ""

   def __init__(self):
      # FIXME - skip this if we can't get gamera.core (i.e.
      # during the build process).
      core = __import__("gamera.core")
      if core is None:
         return
      for function in self.functions:
         function.register(self.category)

   def get_test_suite(self):
      suite = unittest.TestSuite()
      for function in self.functions:
         if function.testable:
            suite.addTest(function.get_test_case())
      return suite

class PluginFunction:
   return_type = None
   self_type = ImageType((ONEBIT, GREYSCALE, GREY16, RGB, FLOAT))
   args = Args([])
   pure_python = 0
   image_types_must_match = 0
   testable = 0

   def register(cls, category=None, add_to_image=1):
      if hasattr(cls, 'category'):
         category = cls.category
      if not hasattr(cls, "__call__"):
         # This loads the actual C++ function if it is not directly
         # linked in the Python PluginFunction class
         parts = cls.__module__.split('.')
         file = inspect.getfile(cls)
         cpp_module_name = '_' + parts[-1]
         directory = os.path.split(file)[0]
         sys.path.append(directory)
         found = imp.find_module(cpp_module_name)
         del sys.path[-1]
         if found:
             module = imp.load_module(cpp_module_name, *found)
         if module == None:
            return
         func = getattr(module, cls.__name__)
      elif cls.__call__ is None:
         func = None
      else:
         func = cls.__call__
      cls.__call__ = staticmethod(func)
      from gamera import core
      if add_to_image and isinstance(cls.self_type, ImageType):
         core.ImageBase.add_plugin_method(cls, func, category)
   register = classmethod(register)

   def get_test_case(self):
      import testing
      return testing.PluginTest(self)

   def test(cls):
      # Testing function goes here
      # This can be overridden to just call the plugin function a
      # bunch of times with different arguments and return a list of
      # results
      import testing
      results = []
      if cls.image_types_must_match:
         for type in cls.self_type.pixel_types:
            if type == FLOAT:
               continue
            self = testing.get_generic_test_image(type)
            params = []
            for i in cls.args:
               if isinstance(i, ImageType):
                  param = testing.get_generic_test_image(type)
               else:
                  param = i.default
               params.append(param)
            cls._do_test(self, params, results)
      else:
         for type in cls.self_type.pixel_types:
            if type == FLOAT:
               continue
            self = testing.get_generic_test_image(type)
            cls._test_recurse(self, [], results)
      return results
   test = classmethod(test)

   def _test_recurse(cls, self, params, results):
      import testing
      if len(params) == len(cls.args.list):
         cls._do_test(self, params, results)
      else:
         arg = cls.args[len(params)]
         if isinstance(arg, ImageType):
            for type in arg.pixel_types:
               if type == FLOAT:
                  continue
               param = testing.get_generic_test_image(type)
               params.append(param)
               cls._test_recurse(self, params, results)
         else:
            param = arg.default
            params.append(param)
            cls._test_recurse(self, params, results)

   _test_recurse = classmethod(_test_recurse)

   def _do_test(cls, self, params, results):
      from gamera import core
      try:
         result = apply(getattr(self, cls.__name__), tuple(params))
      except Exception:
         results.append(str(Exception))
      else:
         if result != None:
            results.append(result)
         results.append(self)
         for i in params:
            if isinstance(i, core.ImageBase):
               results.append(i)
   _do_test = classmethod(_do_test)

def PluginFactory(name, func, category=None,
                  return_type=None,
                  self_type=ImageType((RGB,
                                       GREYSCALE,
                                       GREY16,
                                       ONEBIT,
                                       FLOAT)),
                  args=None):
   cls = new.classobj(name, (PluginFunction,), {})
   if not category is None:
      cls.category = category
   cls.return_type = return_type
   cls.self_type = self_type
   if args is None:
      cls.args = Args([])
   else:
      cls.args = args
   cls.__call__ = func
   return cls

def get_config_options(command):
   return os.popen(command).read()
