#
#
# Copyright (C) 2002 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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

class to_rgb(PluginFunction):
    self_type = ImageType([ONEBIT, GREYSCALE, FLOAT, GREY16])
    return_type = ImageType([RGB], "rgb")

class to_greyscale(PluginFunction):
    self_type = ImageType([ONEBIT, FLOAT, GREY16, RGB])
    return_type = ImageType([GREYSCALE], "greyscale")

class to_grey16(PluginFunction):
    self_type = ImageType([ONEBIT, GREYSCALE, FLOAT, RGB])
    return_type = ImageType([GREY16], "grey16")

class to_float(PluginFunction):
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, RGB])
    return_type = ImageType([FLOAT], "float")

class to_onebit(PluginFunction):
    pure_python = True
    self_type = ImageType([FLOAT, GREY16, RGB])
    return_type = ImageType([ONEBIT], "onebit")
    def __call__(self, storage_format=DENSE):
        image = self.to_greyscale()
        return image.otsu_threshold(storage_format)
    __call__ = staticmethod(__call__)
to_onebit_twostep = to_onebit

class to_onebit(PluginFunction):
    pure_python = True
    self_type = ImageType([GREYSCALE])
    return_type = ImageType([ONEBIT], "onebit")
    def __call__(self, storage_format=DENSE):
        return self.otsu_threshold(storage_format)
    __call__ = staticmethod(__call__)

class ImageConversionModule(PluginModule):
    category = "Conversion"
    cpp_headers=["image_conversion.hpp"]
    cpp_namespaces = ["Gamera"]
    functions = [to_rgb, to_greyscale, to_grey16, to_float,
                 to_onebit, to_onebit_twostep]
    author = "Michael Droettboom and Karl MacMillan"
    url = "http://gamera.dkc.jhu.edu/"

module = ImageConversionModule()

