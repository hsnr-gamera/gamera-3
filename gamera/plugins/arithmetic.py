#
#
# Copyright (C) 2001-2004 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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

"""The image utilities module contains plugins for copy, rotating, resizing,
and computing histograms."""

from gamera.plugin import *
import _arithmetic

ARITHMETIC_TYPES = [GREYSCALE, GREY16, FLOAT, RGB]

########################################
# Convolution methods

class add_images(PluginFunction):
    """Adds the corresponding pixels of two images together.

The two images must be the same type.

Since it would be difficult to determine what exactly to do if the images
are a different size, the two images must be the same size.  Use .subimage
on either image to crop appropriately if necessary for your specific case.

*in_place*
   If true, the operation will be performed in-place, changing the
   contents of the current image.
"""
    self_type = ImageType(ARITHMETIC_TYPES)
    args = Args([ImageType(ARITHMETIC_TYPES, 'other'), Check("in_place", default=False)])
    return_type = ImageType(ARITHMETIC_TYPES)
    image_types_must_match = True

    def __call__(self, other, in_place=False):
       return _arithmetic.add_images(self, other, in_place)
    __call__ = staticmethod(__call__)

class subtract_images(PluginFunction):
    """Adds the pixels of another image from the current image.

The two images must be the same type.

Since it would be difficult to determine what exactly to do if the images
are a different size, the two images must be the same size.  Use .subimage
on either image to crop appropriately if necessary for your specific case.

*in_place*
   If true, the operation will be performed in-place, changing the
   contents of the current image.
"""
    self_type = ImageType(ARITHMETIC_TYPES)
    args = Args([ImageType(ARITHMETIC_TYPES, 'other'), Check("in_place", default=False)])
    return_type = ImageType(ARITHMETIC_TYPES)
    image_types_must_match = True

    def __call__(self, other, in_place=False):
       return _arithmetic.subtract_images(self, other, in_place)
    __call__ = staticmethod(__call__)

class divide_images(PluginFunction):
    """Divides the pixels of the current image by the pixels of
another image.

The two images must be the same type.

Since it would be difficult to determine what exactly to do if the images
are a different size, the two images must be the same size.  Use .subimage
on either image to crop appropriately if necessary for your specific case.

*in_place*
   If true, the operation will be performed in-place, changing the
   contents of the current image.
"""
    self_type = ImageType([GREYSCALE, GREY16, FLOAT])
    args = Args([ImageType([GREYSCALE, GREY16, FLOAT], 'other'), Check("in_place", default=False)])
    return_type = ImageType([GREYSCALE, GREY16, FLOAT])
    image_types_must_match = True

    def __call__(self, other, in_place=False):
       return _arithmetic.divide_images(self, other, in_place)
    __call__ = staticmethod(__call__)

class multiply_images(PluginFunction):
    """Multiplies the corresponding pixels of two images together.

The two images must be the same type.

Since it would be difficult to determine what exactly to do if the images
are a different size, the two images must be the same size.  Use .subimage
on either image to crop appropriately if necessary for your specific case.

*in_place*
   If true, the operation will be performed in-place, changing the
   contents of the current image.
"""
    self_type = ImageType(ARITHMETIC_TYPES)
    args = Args([ImageType(ARITHMETIC_TYPES, 'other'), Check("in_place", default=False)])
    return_type = ImageType(ARITHMETIC_TYPES)
    image_types_must_match = True

    def __call__(self, other, in_place=False):
       return _arithmetic.multiply_images(self, other, in_place)
    __call__ = staticmethod(__call__)
    
class ArithmeticModule(PluginModule):
    cpp_headers=["arithmetic.hpp"]
    cpp_namespace=["Gamera"]
    category = "Combine/Arithmetic"
    functions = [add_images, subtract_images, multiply_images, divide_images]
    author = "Michael Droettboom"
    url = "http://gamera.dkc.jhu.edu/"
module = ArithmeticModule()
    
