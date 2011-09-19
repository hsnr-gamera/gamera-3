#
#
# Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, Karl MacMillan
#               2011      Christoph Dalitz
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

from gamera.plugin import *
import image_utilities, _image_conversion

class to_rgb(PluginFunction):
    """
    Converts the given image to an RGB image according to teh following rules:

    - for ONEBIT images, 0 is mapped to (255,255,255) and everything else to (0,0,0)
    - for GREYSCALE and GREY16 images, R=G=B
    - for FLOAT images, the range [min,max] is linearly mapped to the 256 grey values

    Note, converting an image to one of the same type performs a copy operation.
    """
    author = "Michael Droettboom, Karl MacMillan, and Christoph Dalitz"
    self_type = ImageType([ONEBIT, GREYSCALE, FLOAT, GREY16, COMPLEX])
    return_type = ImageType([RGB], "rgb")

    def __call__(self):
        if self.data.pixel_type == RGB:
            return self.image_copy()
        return _image_conversion.to_rgb(self)
    __call__ = staticmethod(__call__)

class to_greyscale(PluginFunction):
    """
    Converts the given image to a GREYSCALE image according to the
    following rules:

    - for ONEBIT images, 0 is mapped to 255 and everything else to 0.
    - for FLOAT images, the range [min,max] is linearly scaled to [0,255]
    - for GREY16 images, the range [0,max] is linearly scaled to [0,255]
    - for RGB images, the luminance is used, which is defined in VIGRA as 0.3*R + 0.59*G + 0.11*B

    Converting an image to one of the same type performs a copy operation.
    """
    author = "Michael Droettboom, Karl MacMillan, and Christoph Dalitz"
    self_type = ImageType([ONEBIT, FLOAT, GREY16, RGB, COMPLEX])
    return_type = ImageType([GREYSCALE], "greyscale")
    doc_examples = [(RGB,)]

    def __call__(self):
        if self.data.pixel_type == GREYSCALE:
            return self.image_copy()
        return _image_conversion.to_greyscale(self)
    __call__ = staticmethod(__call__)

class to_grey16(PluginFunction):
    """
    Converts the given image to a GREY16 image according to the
    following rules:

    - for ONEBIT images, 0 is mapped to 65535 and everything else to 0.
    - for FLOAT images, the range [min,max] is linearly scaled to [0,65535]
    - for GREYSCALE images, pixel values are copied unchanged
    - for RGB images, the luminance is used, which is defined in VIGRA as 0.3*R + 0.59*G + 0.11*B. This results only in a value range [0,255]

    Converting an image to one of the same type performs a copy operation.
    """
    author = "Michael Droettboom, Karl MacMillan, and Christoph Dalitz"
    self_type = ImageType([ONEBIT, GREYSCALE, FLOAT, RGB, COMPLEX])
    return_type = ImageType([GREY16], "grey16")

    def __call__(self):
        if self.data.pixel_type == GREY16:
            return self.image_copy()
        return _image_conversion.to_grey16(self)
    __call__ = staticmethod(__call__)

class to_float(PluginFunction):
    """
    Converts the given image to a FLOAT image according to the following
    rules:

    - for ONEBIT images, 0 is mapped to 0.0 and everything else to 1.0
    - for GREYSCALE and GREY16 images, pixel values are copied unchanged
    - for RGB images, the luminance is used, which is defined in VIGRA as 0.3*R + 0.59*G + 0.11*B

    Converting an image to one of the same type performs a copy operation.
    """
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, RGB, COMPLEX])
    return_type = ImageType([FLOAT], "float")

    def __call__(self):
        if self.data.pixel_type == FLOAT:
            return self.image_copy()
        return _image_conversion.to_float(self)
    __call__ = staticmethod(__call__)

class to_complex(PluginFunction):
    """
    Converts the given image to a COMPLEX image.

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
    """
    Converts the given image to a ONEBIT image. First the image is converted
    and then the otsu_threshold_ algorithm is applied.
    For other ways to convert to ONEBIT images, see the Binarization_ category.

    Converting an image to one of the same type performs a copy operation.

    .. _otsu_threshold: binarization.html#otsu-threshold
    .. _Binarization: binarization.html
    """
    pure_python = True
    self_type = ImageType([FLOAT, GREYSCALE, GREY16, RGB, COMPLEX])
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

class extract_real(PluginFunction):
    """
    Returns a Float image containing only the real values in the given
    complex image.
    """
    self_type = ImageType([COMPLEX])
    return_type = ImageType([FLOAT], "float")

class extract_imaginary(PluginFunction):
    """
    Returns a Float image containing only the imaginary values in the
    given complex image.
    """
    self_type = ImageType([COMPLEX])
    return_type = ImageType([FLOAT], "float")

class ImageConversionModule(PluginModule):
    category = "Conversion"
    cpp_headers=["image_conversion.hpp"]
    functions = [to_rgb, to_greyscale, to_grey16, to_float,
                 to_onebit, to_onebit, to_complex, extract_real,
                 extract_imaginary]
    author = "Michael Droettboom and Karl MacMillan"
    url = "http://gamera.sourceforge.net/"

module = ImageConversionModule()

