# -*- mode: python; indent-tabs-mode: nil; tab-width: 3 -*-
# vim: set tabstop=3 shiftwidth=3 expandtab:
#
# Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom,
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#

from __future__ import generators
import os, sys, dircache, glob, imp  # Python standard library

if 1:
   def dummy():
      pass

lib = os.path.dirname(os.path.realpath(dummy.func_code.co_filename))
lib_gui = os.path.realpath(os.path.join(lib, "gui"))
# Figure out if we are in the source directory or installed
plugins = os.path.realpath(os.path.join(lib, "plugins"))
doc = os.path.realpath(os.path.join(lib, "doc"))
sys.path.append(plugins)
plugins_src = ""
toolkits = os.path.realpath(os.path.join(lib, "toolkits"))
test = os.path.realpath(os.path.join(lib, "test"))
test_results = os.path.realpath(os.path.join(lib, "test/results"))

def get_toolkit_names(dir):
   toolkits = []
   listing = dircache.listdir(dir)
   dircache.annotate(dir, listing)
   for toolkit in listing:
      if toolkit.endswith(".py") and toolkit != "__init__.py":
         toolkits.append(toolkit[:-3])
      elif toolkit.endswith("module.so"):
         toolkits.append(toolkit[:-9])
      elif (toolkit.endswith("/") and
            "__init__.py" in dircache.listdir(os.path.join(dir, toolkit))):
         toolkits.append(toolkit[:-1])
   return toolkits

def get_directory_of_modules(dir, base=''):
   modules = glob.glob(os.path.join(dir, "*.py"))
   names = [os.path.basename(x).split('.')[0] for x in modules]
   mods = []
   suffixes = imp.get_suffixes()
   for i in suffixes:
      if i[0] == '.py':
         suffix = i
         break
   for m, name in zip(modules, names):
      try:
         module = imp.load_module(base + name, file(m, 'r'), m, suffix)
         mods.append(module)
      except Exception, e:
         print e
   return mods

def import_directory(dir, gl, lo, verbose=0):
   modules = glob.glob(os.path.join(dir, "*.py"))
   modules = [os.path.basename(x).split('.')[0] for x in modules]
   if verbose:
      sys.stdout.write("Loading plugins: " + "-" * 40 + "\n")
   column = 0
   first = 1
   result = []
     
   for m in modules:
      if m == '__init__':
         continue
      try:
         module = __import__(m, gl, lo, [])
         failed = 0
      except Exception, e:
         failed = e
      if failed:
         display = '[%s %s]' % (m, str(failed))
      else:
         display = m
         result.append(module)
      if m != modules[-1]:
         display += ", "
      column += len(display)
      if verbose:
         if column > 70:
            sys.stdout.write("\n")
            column = len(display)
         sys.stdout.write(display)
         sys.stdout.flush()
   if verbose:
      sys.stdout.write("\n")
   return result
