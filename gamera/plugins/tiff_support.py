#
#
# Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom,
# and Karl MacMillan
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

from gamera.plugin import *
import sys
import glob
import _tiff_support

class tiff_info(PluginFunction):
    """
    Returns an ``ImageInfo`` object describing a TIFF file.

    *image_file_name*
      A TIFF image filename"""
    self_type = None
    args = Args([String("image_file_name")])
    return_type = ImageInfo("tiff_info")

class load_tiff(PluginFunction):
    """
    Loads a TIFF file from disk.

    *image_file_name*
      A TIFF image filename

    *storage_format* (optional)
      specifies the compression type for the result:

      DENSE (0)
        no compression
      RLE (1)
        run-length encoding compression
    """
    self_type = None
    args = Args([FileOpen("image_file_name", "", "*.tiff;*.tif"),
                 Choice("storage format", ["DENSE", "RLE"])])
    return_type = ImageType([ONEBIT, GREYSCALE, GREY16, RGB, FLOAT])
    def __call__(filename, compression = 0):
        return _tiff_support.load_tiff(filename, compression)
    __call__ = staticmethod(__call__)
    exts = ["tiff", "tif"]
load_tiff_class = load_tiff
load_tiff = load_tiff()

class save_tiff(PluginFunction):
    """
    Saves an image to disk in TIFF format.

    *image_file_name*
      A TIFF image filename
    """
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, RGB])
    args = Args([FileSave("image_file_name", "image.tiff", "*.tiff;*.tif")])
    return_type = None
    exts = ["tiff", "tif"]

class TiffSupportModule(PluginModule):
    category = "File"
    cpp_headers = ["tiff_support.hpp"]
    if sys.platform == 'win32':
        cpp_sources = glob.glob("src/libtiff/*.c")
        try:
            cpp_sources.remove("src/libtiff\\tif_unix.c")
        except:
            pass
        extra_compile_args = ['-Dunix']
    elif sys.platform == 'cygwin':
        cpp_sources = glob.glob("src/libtiff/*.c")
        try:
            cpp_sources.remove("src/libtiff/tif_win32.c")
        except:
            pass
        extra_compile_args = ['-Dunix']
    elif sys.platform == 'darwin':
        cpp_sources = glob.glob("src/libtiff/*.c")
        try:
	   cpp_sources.remove("src/libtiff/tif_win32.c")
	except:
	   pass
	extra_compile_args = ['-Dunix']
    else:
        extra_libraries = ["tiff"]
    functions = [tiff_info, load_tiff_class, save_tiff]
    cpp_include_dirs = ["src/libtiff"]
    author = "Michael Droettboom and Karl MacMillan"
    url = "http://gamera.dkc.jhu.edu/"

module = TiffSupportModule()

tiff_info = tiff_info()
