#!/usr/bin/env python

# vi:set tabsize=3:
#
# Copyright (C) 2001-2004 Ichiro Fujinaga, Michael Droettboom,
#                         and Karl MacMillan
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

# We do this first, so that when gamera.__init__ loads gamera.__version__,
# it is in fact the new and updated version
gamera_version = open("version", 'r').readlines()[0].strip()
open("gamera/__version__.py", "w").write("ver = '%s'\n\n" % gamera_version)

import sys, os, glob
from distutils.core import setup, Extension
from gamera import gamera_setup

##########################################
# generate the command line startup scripts
command_line_utils = (
   ('gamera_gui', 'gamera_gui.py',
    """#!%(executable)s
%(header)s
print "Loading GAMERA..."
try:
   from gamera.gui import gui
   gui.run()
except:
   import traceback
   print "Gamera made a fatal error:"
   print
   traceback.print_exc()
   print
   print "Press <ENTER> to exit."
   x = raw_input()
   """), )
   
if sys.platform == 'win32':
   command_line_filename_at = 1
   scripts_directory_name = "Scripts"
else:
   command_line_filename_at = 0
   scripts_directory_name = "bin/"

info = {'executable': sys.executable,
        'header'    :
        """# This file was automatically generated by the\n"""
        """# Gamera setup script on %s.\n""" } #%
for util in command_line_utils:
   if sys.platform == 'win32':
      _, file, content = util
   else:
      file, _, content = util
   fd = open(file, 'w')
   fd.write(content % info)
   fd.close()
os.chmod(file, 0700)

scripts = [x[command_line_filename_at] for x in command_line_utils] + ['gamera_post_install.py']

##########################################
# generate the plugins
plugin_extensions = []
plugins = gamera_setup.get_plugin_filenames('gamera/plugins/')
plugin_extensions = gamera_setup.generate_plugins(
   plugins, "gamera.plugins", True)

########################################
# Non-plugin extensions

ga_files = glob.glob("src/ga/*.cpp")
ga_files.append("src/knncoremodule.cpp")
graph_files = glob.glob("src/graph/*.cpp")

extensions = [Extension("gamera.gameracore",
                        ["src/gameramodule.cpp",
                         "src/sizeobject.cpp",
                         "src/pointobject.cpp",
                         "src/dimensionsobject.cpp",
                         "src/rectobject.cpp",
                         "src/regionobject.cpp",
                         "src/regionmapobject.cpp",
                         "src/rgbpixelobject.cpp",
                         "src/imagedataobject.cpp",
                         "src/imageobject.cpp",
                         "src/imageinfoobject.cpp"
                         ],
                        include_dirs=["include"],
                        **gamera_setup.extras
                        ),
              Extension("gamera.knncore", ga_files,
                        include_dirs=["include", "src/ga", "src"],
                        **gamera_setup.extras),
              Extension("gamera.graph", graph_files,
                        include_dirs=["include", "src", "src/graph"],
                        **gamera_setup.extras)]
extensions.extend(plugin_extensions)

##########################################
# Here's the basic distutils stuff

description = ("This is the Gamera installer. " +
               "Please ensure that Python and wxPython 2.4.0 " +
               "(or later) are installed before proceeding.")


includes = [(os.path.join(gamera_setup.include_path, path),
             glob.glob(os.path.join("include", os.path.join(path, ext))))
            for path, ext in
            ("", "*.hpp"),
            ("plugins", "*.hpp"),
            ("vigra", "*.hxx")]

packages = ['gamera', 'gamera.gui', 'gamera.plugins', 'gamera.toolkits',
            'gamera.backport']

if sys.platform == 'darwin':
   packages.append("gamera.mac")
elif sys.platform == 'win32':
   packages.append("win32")
   if '--compiler=icl' in sys.argv:
       from distutils import ccompiler
       from win32.iclcompiler import ICLCompiler
       ccompiler.compiler_class['icl'] = ('win32.iclcompiler',
                                          'ICLCompiler',
                                          'Intel Compiler Library for Windows')
       ccompiler.new_compiler = ICLCompiler.ret_icl
setup(cmdclass = gamera_setup.cmdclass,
      name = "gamera",
      version=gamera_version,
      url = "http://gamera.sourceforge.net/",
      author = "Michael Droettboom and Karl MacMillan",
      author_email = "gamera-devel@yahoogroups.com",
      ext_modules = extensions,
      description = description,
      packages = packages,
      scripts = scripts,
      data_files=[(os.path.join(gamera_setup.lib_path, "test"), glob.glob("gamera/test/*.tiff"))] + includes)
