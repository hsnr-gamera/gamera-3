#
#
# Copyright (C) 2001-2002 Ichiro Fujinaga, Michael Droettboom,
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
import _tiff_support

class tiff_info(PluginFunction):
    self_type = None
    args = Args([String("image_file_name")])
    return_type = ImageInfo("tiff_info")
tiff_info = tiff_info()

class load_tiff(PluginFunction):
    self_type = None
    args = Args([FileOpen("image_file_name", "", "*.tiff;*.tif"),
                 Choice("storage format", ["DENSE", "RLE"])])
    return_type = ImageType([ONEBIT, GREYSCALE, GREY16, RGB, FLOAT])
    def __call__(filename, compression = 0):
        return _tiff_support.load_tiff(filename, compression)
    __call__ = staticmethod(__call__)
load_tiff = load_tiff()

class save_tiff(PluginFunction):
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, RGB])
    args = Args([FileOpen("image_file_name", "image.tiff", "*.tiff;*.tif")])
    return_type = None
save_tiff = save_tiff()

class TiffSupportModule(PluginModule):
    category = "File"
    cpp_headers = ["tiff_support.hpp"]
    cpp_namespaces = ["Gamera"]
    functions = [tiff_info, load_tiff, save_tiff]
    author = "Michael Droettboom and Karl MacMillan"
    url = "http://gamera.dkc.jhu.edu/"
    extra_libraries = ["tiff", "jpeg", "z"]

module = TiffSupportModule()
