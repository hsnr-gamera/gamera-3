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

# From rgb
class rgb_to_greyscale(PluginFunction):
    self_type = ImageType([RGB])
    return_type = ImageType([GREYSCALE], "output")
rgb_to_greyscale = rgb_to_greyscale()

class rgb_to_grey16(PluginFunction):
    self_type = ImageType([RGB])
    return_type = ImageType([GREY16], "output")
rgb_to_grey16 = rgb_to_grey16()

class rgb_to_float(PluginFunction):
    self_type = ImageType([RGB])
    return_type = ImageType([FLOAT], "output")
rgb_to_float = rgb_to_float()

# From GreyScale
class greyscale_to_float(PluginFunction):
    self_type = ImageType([GREYSCALE])
    return_type = ImageType([FLOAT], "output")
greyscale_to_float = greyscale_to_float()

class greyscale_to_grey16(PluginFunction):
    self_type = ImageType([GREYSCALE])
    return_type = ImageType([GREY16], "output")
greyscale_to_grey16 = greyscale_to_grey16()

class greyscale_to_rgb(PluginFunction):
    self_type = ImageType([GREYSCALE])
    return_type = ImageType([RGB], "output")
greyscale_to_rgb = greyscale_to_rgb()

class ImageConversionModule(PluginModule):
    category = "Utility"
    cpp_headers=["image_conversion.hpp"]
    functions = [rgb_to_greyscale, rgb_to_grey16, rgb_to_float,
                 greyscale_to_float, greyscale_to_grey16,
                 greyscale_to_rgb]
    author = "Michael Droettboom and Karl MacMillan"
    url = "http://gamera.dkc.jhu.edu/"

module = ImageConversionModule()
