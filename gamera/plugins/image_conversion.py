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
    """Converts the given image to an RGB image."""
    self_type = ImageType([ONEBIT, GREYSCALE, FLOAT, GREY16])
    return_type = ImageType([RGB], "rgb")

class to_greyscale(PluginFunction):
    """Converts the given image to a GREYSCALE image."""
    self_type = ImageType([ONEBIT, FLOAT, GREY16, RGB])
    return_type = ImageType([GREYSCALE], "greyscale")
    doc_examples = [(RGB,)]

class to_grey16(PluginFunction):
    """Converts the given image to a GREY16 image."""
    self_type = ImageType([ONEBIT, GREYSCALE, FLOAT, RGB])
    return_type = ImageType([GREY16], "grey16")

class to_float(PluginFunction):
    """Converts the given image to a FLOAT image."""
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, RGB])
    return_type = ImageType([FLOAT], "float")

class to_onebit(PluginFunction):
    """Converts the given image to a ONEBIT image.  Uses the
otsu_threshold_ algorithm.  For more ways to convert to ONEBIT images,
see the Thresholding_ category.

.. _otsu_threshold: thresholding.html#otsu-threshold
.. _Thresholding: thresholding.html
"""
    pure_python = True
    self_type = ImageType([FLOAT, GREYSCALE, GREY16, RGB])
    return_type = ImageType([ONEBIT], "onebit")
    def __call__(self, storage_format=DENSE):
        try:
            image = self.to_greyscale()
        except TypeError:
            image = self
        return image.otsu_threshold(storage_format)
    __call__ = staticmethod(__call__)
    doc_examples = [(RGB,)]
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

