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
import new, os, os.path, imp, inspect, sys
from enums import *

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
   author = None
   url = None

   def __init__(self):
      # FIXME - skip this if we can't get gamera.core (i.e.
      # during the build process).
      core = __import__("gamera.core")
      if core is None:
         return
      for function in self.functions:
         function.register(self, self.category)

class Builtin(PluginModule):
   author = "Michael Droettboom and Karl MacMillan"
   url = "http://dkc.jhu.edu/"

class PluginFunction:
   return_type = None
   self_type = ImageType(ALL)
   args = Args([])
   pure_python = 0
   image_types_must_match = 0
   testable = 0
   feature_function = False
   category = None
   doc_examples = []

   def register(cls, module=Builtin, category=None, add_to_image=1):
      cls.module = module
      if cls.category != None:
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
      if add_to_image:
         core.ImageBase.add_plugin_method(cls, func, category)
   register = classmethod(register)

def PluginFactory(name, category=None,
                  return_type=None,
                  self_type=ImageType((RGB,
                                       GREYSCALE,
                                       GREY16,
                                       ONEBIT,
                                       FLOAT)),
                  args=None):
   from gamera import core
   cls = new.classobj(name, (PluginFunction,), {})
   if not category is None:
      cls.category = category
   cls.return_type = return_type
   cls.self_type = self_type
   if args is None:
      cls.args = Args([])
   else:
      cls.args = args
   func = getattr(core.ImageBase, name)
   cls.__call__ = func
   cls.__doc__ = func.__doc__
   cls.module = Builtin
   return cls

def get_config_options(command):
   return os.popen(command).read()
