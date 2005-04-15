#
#
# Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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

# Most important problem with this module: Where do we find libpng?
#
# Linux:
#   We always assume libpng was installed in a default place (/usr or /usr/local)
#
# Mac OS-X:
#   We look for png.h in Fink (/sw/include/png.h) and then
#   /usr/include.  If it's not there, we use the local copy of libpng
#
# MS-Windows:
#   We always use the local copy of libpng

from gamera.plugin import *

class PNG_info(PluginFunction):
    """Returns an ``ImageInfo`` object describing a PNG file.

*image_file_name*
  A PNG image filename"""
    self_type = None
    args = Args([String("image_file_name")])
    return_type = ImageInfo("PNG_info")

class load_PNG(PluginFunction):
   """Loads a PNG format image file.

*image_file_name*
  A PNG image filename

*storage_format* (optional)
  specifies the compression type for the result:

  DENSE (0)
    no compression
  RLE (1)
    run-length encoding compression"""
   self_type = None
   args = Args([FileOpen("image_file_name", "", "*.png"),
                Choice("storage format", ["DENSE", "RLE"])])
   return_type = ImageType([ONEBIT, GREYSCALE, GREY16, RGB, FLOAT])
   def __call__(filename, compression = 0):
      from gamera.plugins import _png_support
      return _png_support.load_PNG(filename, compression)
   __call__ = staticmethod(__call__)
   exts = ['png']

class save_PNG(PluginFunction):
   """Saves the image to a PNG format file."""
   self_type = ImageType(ALL)
   args = Args([FileSave("image_file_name", "image.png", "*.png")])
   exts = ['png']

class PngSupportModule(PluginModule):
    import sys
    import os.path
    category = "File"
    cpp_headers = ["png_support.hpp"]
    internal_png_dir = "src/libpng-1.2.5/"
    internal_zlib_dir = "src/zlib-1.2.1/"
    if sys.platform == 'linux2':
        extra_libraries = ["png"]
    elif sys.platform == 'darwin':
        found_png = None
        for p in ["/sw", "/usr", "/usr/local"]:
            if os.path.exists(os.path.join(p, "include/png.h")):
                found_png = p
                break
        if found_png:
            cpp_include_dirs = [os.path.join(p, "include")]
            library_dirs = [os.path.join(p, "lib")]
            extra_libraries = ["png"]
        else:
            cpp_sources = [os.path.join(internal_png_dir, x) for x in
                           ['png.c', 'pngset.c', 'pngget.c', 'pngrutil.c',
                            'pngtrans.c', 'pngwutil.c', 'pngread.c', 'pngrio.c',
                            'pngwio.c', 'pngwrite.c', 'pngrtran.c', 'pngwtran.c',
                            'pngmem.c', 'pngerror.c', 'pngpread.c']]
            cpp_include_dirs = [internal_png_dir]
            # zlib, which apparently is included in OS-X 10.3 by default
            extra_libraries = ["z"]
    elif sys.platform == 'win32':
        cpp_sources = [os.path.join(internal_png_dir, x) for x in
                       ['png.c', 'pngset.c', 'pngget.c', 'pngrutil.c',
                        'pngtrans.c', 'pngwutil.c', 'pngread.c', 'pngrio.c',
                        'pngwio.c', 'pngwrite.c', 'pngrtran.c', 'pngwtran.c',
                        'pngmem.c', 'pngerror.c', 'pngpread.c',]]
        cpp_sources.extend([os.path.join(internal_zlib_dir, x) for x in
                            ['adler32.c','compress.c','crc32.c','deflate.c','gzio.c',
                             'infback.c','inffast.c','inflate.c','inftrees.c','trees.c',
                             'uncompr.c','zutil.c']])
        cpp_include_dirs = [internal_png_dir, "include/zlib-1.2.1"]
#        extra_libraries = ["z"]
    functions = [save_PNG, PNG_info, load_PNG]
    author = "Michael Droettboom and Albert Bzreckzo"
    url = "http://gamera.dkc.jhu.edu/"
module = PngSupportModule()

PNG_info = PNG_info()
load_PNG = load_PNG()
