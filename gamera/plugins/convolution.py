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
from gamera.plugins import image_utilities
import _convolution

class convolve(PluginFunction):
    """Convolves an image with a given kernel.

Uses code from the Vigra library (Copyright 1998-2002 by Ullrich Koethe).

*kernel*
   A kernel for the convolution.  The kernel may either be a FloatImage
   or a nested Python list of floats.

*border_treatment*
   Specifies how to treat the borders of the image.  Must be one of the following:

   - BORDER_TREATMENT_AVOID (0)

       do not operate on a pixel where the kernel does not fit in the image

   - BORDER_TREATMENT_CLIP (1)

       clip kernel at image border (this is only useful if the kernel is >= 0 everywhere)

   - BORDER_TREATMENT_REPEAT (2)

       repeat the nearest valid pixel

   - BORDER_TREATMENT_REFLECT (3)

       reflect image at last row/column

   - BORDER_TREATMENT_WRAP (4)

       wrap image around (periodic boundary conditions)
"""
    self_type = ImageType(ALL)
    args = Args([ImageType([FLOAT], 'kernel'),
                 Choice('border_treatment',
                        ['avoid', 'clip', 'repeat', 'reflect', 'wrap'],
                        default=1)])
    return_type = ImageType(ALL)

    def __call__(self, kernel, border_treatment=1):
        from gamera.gameracore import FLOAT
        if type(kernel) == list:
            kernel = image_utilities.nested_list_to_image(kernel, FLOAT)
        return _convolution.convolve(self, kernel, border_treatment)
    __call__ = staticmethod(__call__)

class convolve_xy(PluginFunction):
    """Convolves an image in both X and Y directions with 1D kernels.
This is equivalent to what the Vigra library calls "Separable Convolution".

Uses code from the Vigra library (Copyright 1998-2002 by Ullrich
Koethe).

*kernel_y* A kernel for the convolution in the *y* direction.  The
   kernel may either be a FloatImage or a nested Python list of
   floats.

*kernel_x* A kernel for the convolution in the *x* direction.  The
   kernel may either be a FloatImage or a nested Python list of
   floats.  If *kernel_x* is omitted, *kernel_y* will be used in the
   *x* direction.

*border_treatment* Specifies how to treat the borders of the image.
   See ``convolve`` for information about *border_treatment*
   values."""
    self_type = ImageType(ALL)
    args = Args([ImageType([FLOAT], 'kernel_y'),
                 ImageType([FLOAT], 'kernel_x'),
                 Choice('border_treatment',
                        ['avoid', 'clip', 'repeat', 'reflect', 'wrap'],
                        default=1)])
    return_type = ImageType(ALL)
    pure_python = True

    def __call__(self, kernel_y, kernel_x=None, border_treatment=1):
        from gamera.gameracore import FLOAT
        if kernel_x is None:
            kernel_x = kernel_y
        if kernel_y == kernel_x:
            if type(kernel_y) == list:
                kernel_x = kernel_y = image_utilities.nested_list_to_image(kernel_y, FLOAT)
        else:
            if type(kernel_y) == list:
                kernel_y = image_utilities.nested_list_to_image(kernel_y, FLOAT)
            if type(kernel_x) == list:
                kernel_x = image_utilities.nested_list_to_image(kernel_x, FLOAT)
        result = _convolution.convolve_x(self, kernel_x, border_treatment)
        return _convolution.convolve_y(result, kernel_y, border_treatment)
    __call__ = staticmethod(__call__)

class convolve_x(PluginFunction):
    """Convolves an image in the X directions with a 1D kernel.
This is equivalent to what the Vigra library calls "Separable Convolution".

Uses code from the Vigra library (Copyright 1998-2002 by Ullrich Koethe).

*kernel_x*
   A kernel for the convolution in the *x* direction.  The kernel may either be a FloatImage
   or a nested Python list of floats.  It must consist of only a single row.

*border_treatment*
   Specifies how to treat the borders of the image.  See ``convolve`` for information
   about *border_treatment* values."""
    self_type = ImageType(ALL)
    args = Args([ImageType([FLOAT], 'kernel_x'),
                 Choice('border_treatment',
                        ['avoid', 'clip', 'repeat', 'reflect', 'wrap'],
                        default=1)])
    return_type = ImageType(ALL)

    def __call__(self, kernel, border_treatment=1):
        from gamera.gameracore import FLOAT
        if type(kernel) == list:
            kernel = image_utilities.nested_list_to_image(kernel, FLOAT)
        return _convolution.convolve_x(self, kernel, border_treatment)
    __call__ = staticmethod(__call__)

class convolve_y(PluginFunction):
    """Convolves an image in the X directions with a 1D kernel.
This is equivalent to what the Vigra library calls "Separable Convolution".

Uses code from the Vigra library (Copyright 1998-2002 by Ullrich Koethe).

*kernel_y*
   A kernel for the convolution in the *x* direction.  The kernel may either be a FloatImage
   or a nested Python list of floats.  It must consist of only a single row.

*border_treatment*
   Specifies how to treat the borders of the image.  See ``convolve`` for information
   about *border_treatment* values."""
    self_type = ImageType(ALL)
    args = Args([ImageType([FLOAT], 'kernel_y'),
                 Choice('border_treatment',
                        ['avoid', 'clip', 'repeat', 'reflect', 'wrap'],
                        default=1)])
    return_type = ImageType(ALL)

    def __call__(self, kernel, border_treatment=1):
        from gamera.gameracore import FLOAT
        if type(kernel) == list:
            kernel = image_utilities.nested_list_to_image(kernel, FLOAT)
        return _convolution.convolve_y(self, kernel, border_treatment)
    __call__ = staticmethod(__call__)

class ConvolutionModule(PluginModule):
    cpp_headers=["convolution.hpp"]
    cpp_namespace=["Gamera"]
    category = "Convolution"
    functions = [convolve, convolve_xy, convolve_x, convolve_y]
    author = "Michael Droettboom"
    url = "http://gamera.dkc.jhu.edu/"
module = ConvolutionModule()

BORDER_TREATMENT_AVOID = 0
BORDER_TREATMENT_CLIP = 1
BORDER_TREATMENT_REPEAT = 2
BORDER_TREATMENT_REFLECT = 3
BORDER_TREATMENT_WRAP = 4
