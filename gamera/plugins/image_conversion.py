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
to_rgb = to_rgb()

class to_greyscale(PluginFunction):
    self_type = ImageType([ONEBIT, GREYSCALE, FLOAT, GREY16])
    return_type = ImageType([GREYSCALE], "greyscale")
to_greyscale = to_greyscale()

class to_grey16(PluginFunction):
    self_type = ImageType([ONEBIT, GREYSCALE, FLOAT, RGB])
    return_type = ImageType([GREY16], "grey16")
to_grey16 = to_grey16()

class to_float(PluginFunction):
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, RGB])
    return_type = ImageType([FLOAT], "float")
to_float = to_float()

class to_hue(PluginFunction):
    self_type = ImageType([RGB])
    return_type = ImageType([FLOAT], "hue")
to_hue = to_hue()

class to_saturation(PluginFunction):
    self_type = ImageType([RGB])
    return_type = ImageType([FLOAT], "saturation")
to_saturation = to_saturation()

class to_value(PluginFunction):
    self_type = ImageType([RGB])
    return_type = ImageType([FLOAT], "value")
to_value = to_value()

class to_CIE_X(PluginFunction):
    self_type = ImageType([RGB])
    return_type = ImageType([FLOAT], "cie_x")
to_CIE_X = to_CIE_X()

class to_CIE_Y(PluginFunction):
    self_type = ImageType([RGB])
    return_type = ImageType([FLOAT], "cie_y")
to_CIE_Y = to_CIE_Y()

class to_CIE_Z(PluginFunction):
    self_type = ImageType([RGB])
    return_type = ImageType([FLOAT], "cie_z")
to_CIE_Z = to_CIE_Z()

class to_cyan(PluginFunction):
    self_type = ImageType([RGB])
    return_type = ImageType([GREYSCALE], "cyan")
to_cyan = to_cyan()

class to_magenta(PluginFunction):
    self_type = ImageType([RGB])
    return_type = ImageType([GREYSCALE], "magenta")
to_magenta = to_magenta()

class to_yellow(PluginFunction):
    self_type = ImageType([RGB])
    return_type = ImageType([GREYSCALE], "yellow")
to_yellow = to_yellow()

class to_false_color(PluginFunction):
    self_type = ImageType([FLOAT, GREYSCALE])
    return_type = ImageType([RGB], "false_color")
to_false_color = to_false_color()

class ImageConversionModule(PluginModule):
    category = "Utility"
    cpp_headers=["image_conversion.hpp"]
    cpp_namespaces = ["Gamera"]
    functions = [to_rgb, to_greyscale, to_grey16, to_float, to_hue,
                 to_saturation, to_value, to_cyan, to_magenta, to_yellow,
                 to_false_color, to_CIE_X, to_CIE_Y, to_CIE_Z]
    author = "Michael Droettboom and Karl MacMillan"
    url = "http://gamera.dkc.jhu.edu/"

module = ImageConversionModule()
