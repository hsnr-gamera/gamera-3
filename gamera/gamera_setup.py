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

"""Utilities to make writing setup.py files for Gamera extensions easier"""

from distutils.core import setup, Extension
from distutils.util import get_platform
from distutils.sysconfig import get_python_lib
import sys, os, time
import glob

# If gamera.generate is imported gamera.__init__.py will
# also be imported, which won't work until the build is
# finished. To get around this, the gamera directory is
# added to the path and generate is imported directly
sys.path.append("gamera")
import generate

def check_python_version():
   """Make certain that the Python version that is running meets the minimum
   requirements for Gamera. Currently the minimum is 2.2, but this function
   will be updated as the requirements change."""
   if float(''.join([str(x) for x in sys.version_info[0:3]])) < 221:
      print "Gamera requires Python version 2.2.1 or later."
      print "You are running the following Python version:"
      print sys.version
      sys.exit(1)

def get_plugin_filenames(path):
   """Return all of the python plugin files in a specified path. This is not
   the same as glob.glob('*.py') in that it removes __init__.py files and
   normalizes the path in an os independent way."""
   plugins = glob.glob(path + "/*.py")
   norm_plugins = []
   for x in plugins:
      norm_plugins.append(os.path.normpath(os.path.abspath(x)))
   plugins = norm_plugins
   try:
      path = os.path.normpath(os.path.abspath(path + "/__init__.py"))
      plugins.remove(path)
   except:
      pass
   return plugins

def generate_plugins(plugins, location, compiling_gamera=0, extra_compile_args=[], extra_link_args=[], libraries=[]):
   """Generate the necessary cpp wrappers from a list of python plugin
   filenames. The regeneration only happens if it is necessary (either
   the python file has changed or one of the files that it depends on
   has changed). A distutiles extension class is created for each plugin
   that needs to be compiled."""

   # Create the list of modules to ignore at import - because
   # we are in the middle of the build process a lot of C++
   # plugins don't yet exist. By preventing the import of
   # the core of gamera and all of the plugins we allow the
   # plugins to be imported for the build process to examine
   # them. Some of this is unnecessary for external plugins,
   # but it shouldn't hurt.
   ignore = ["core", "gamera.core", "gameracore"]
   for x in plugins:
      plug_path, filename = os.path.split(x)
      module_name = "_" + filename.split('.')[0]
      ignore.append(module_name)
   generate.magic_import_setup(ignore)
   
   plugin_extensions = []
   for x in plugins:
      extension = generate.generate_plugin(
         x, location, compiling_gamera, extra_compile_args,
         extra_link_args, libraries)
      if not extension is None:
         plugin_extensions.append(extension)
   
   generate.restore_import()
   return plugin_extensions
