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
import image_utilities, _image_conversion

class to_rgb(PluginFunction):
    """Converts the given image to an RGB image.

Note, converting an image to one of the same type performs a copy operation.
"""
    self_type = ImageType([ONEBIT, GREYSCALE, FLOAT, COMPLEX, GREY16])
    return_type = ImageType([RGB], "rgb")

    def __call__(self):
        if self.data.pixel_type == RGB:
            return self.image_copy()
        return _image_conversion.to_rgb(self)
    __call__ = staticmethod(__call__)

class to_greyscale(PluginFunction):
    """Converts the given image to a GREYSCALE image.

Note, converting an image to one of the same type performs a copy operation.
"""
    self_type = ImageType([ONEBIT, FLOAT, COMPLEX, GREY16, RGB])
    return_type = ImageType([GREYSCALE], "greyscale")
    doc_examples = [(RGB,)]

    def __call__(self):
        if self.data.pixel_type == GREYSCALE:
            return self.image_copy()
        return _image_conversion.to_greyscale(self)
    __call__ = staticmethod(__call__)

class to_grey16(PluginFunction):
    """Converts the given image to a GREY16 image.

Note, converting an image to one of the same type performs a copy operation.
"""
    self_type = ImageType([ONEBIT, GREYSCALE, FLOAT, COMPLEX, RGB])
    return_type = ImageType([GREY16], "grey16")

    def __call__(self):
        if self.data.pixel_type == GREY16:
            return self.image_copy()
        return _image_conversion.to_grey16(self)
    __call__ = staticmethod(__call__)

class to_float(PluginFunction):
    """Converts the given image to a FLOAT image.

Note, converting an image to one of the same type performs a copy operation.
"""
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, RGB, COMPLEX])
    return_type = ImageType([FLOAT], "float")

    def __call__(self):
        if self.data.pixel_type == FLOAT:
            return self.image_copy()
        return _image_conversion.to_float(self)
    __call__ = staticmethod(__call__)

class to_complex(PluginFunction):
    """Converts the given image to a COMPLEX image.

Note, converting an image to one of the same type performs a copy operation.
"""
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, RGB, FLOAT])
    return_type = ImageType([COMPLEX], "complex")

    def __call__(self):
        if self.data.pixel_type == COMPLEX:
            return self.image_copy()
        return _image_conversion.to_complex(self)
    __call__ = staticmethod(__call__)

class to_onebit(PluginFunction):
    """Converts the given image to a ONEBIT image.  Uses the
otsu_threshold_ algorithm.  For more ways to convert to ONEBIT images,
see the Thresholding_ category.

Note, converting an image to one of the same type performs a copy operation.

.. _otsu_threshold: thresholding.html#otsu-threshold
.. _Thresholding: thresholding.html
"""
    pure_python = True
    self_type = ImageType([FLOAT, COMPLEX, GREYSCALE, GREY16, RGB])
    return_type = ImageType([ONEBIT], "onebit")
    def __call__(self, storage_format=DENSE):
        if self.data.pixel_type == ONEBIT:
            return self.image_copy()
        if self.data.pixel_type != GREYSCALE:
            self = _image_conversion.to_greyscale(self)
        return self.otsu_threshold(storage_format)
    __call__ = staticmethod(__call__)
    doc_examples = [(RGB,), (GREYSCALE,)]
to_onebit = to_onebit

class ImageConversionModule(PluginModule):
    category = "Conversion"
    cpp_headers=["image_conversion.hpp"]
    functions = [to_rgb, to_greyscale, to_grey16, to_float,
                 to_onebit, to_onebit, to_complex]
    author = "Michael Droettboom and Karl MacMillan"
    url = "http://gamera.dkc.jhu.edu/"

module = ImageConversionModule()

