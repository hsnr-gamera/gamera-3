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

# Note: The convolution exposed here does not allow for the case where the
# logical center of the kernel is different from the physical center.
# Saving that for another day... MGD

########################################
# Convolution methods

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

*kernel_y*
   A kernel for the convolution in the *y* direction.  The
   kernel may either be a FloatImage or a nested Python list of
   floats.

*kernel_x*
   A kernel for the convolution in the *x* direction.  The
   kernel may either be a FloatImage or a nested Python list of
   floats.  If *kernel_x* is omitted, *kernel_y* will be used in the
   *x* direction.

*border_treatment*
   Specifies how to treat the borders of the image.
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
    """Convolves an image in the X directions with a 1D kernel.  This
is equivalent to what the Vigra library calls "Separable Convolution".

Uses code from the Vigra library (Copyright 1998-2002 by Ullrich
Koethe).

*kernel_y*
   A kernel for the convolution in the *x* direction.  The
   kernel may either be a FloatImage or a nested Python list of
   floats.  It must consist of only a single row.

*border_treatment*
   Specifies how to treat the borders of the image.
   See ``convolve`` for information about *border_treatment*
   values."""
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

########################################
# Convolution kernels


class GaussianKernel(PluginFunction):
    """Init as a Gaussian function. The radius of the kernel is 
always 3*standard_deviation.

*standard_deviation*
   The standard deviation of the Gaussian kernel.

Uses code from the Vigra library (Copyright 1998-2002 by Ullrich Koethe).
"""
    self_type = None
    args = Args([Float("standard_deviation", default=1.0)])
    return_type = ImageType([FLOAT])
    category = "Convolution/Kernels"

class GaussianDerivativeKernel(PluginFunction):
    """Init as a Gaussian derivative of order 'order'.  The radius of
the kernel is always 3*std_dev.

*standard_deviation*
   The standard deviation of the Gaussian kernel.

*order*
   The order of the Gaussian kernel.

Uses code from the Vigra library (Copyright 1998-2002 by Ullrich
Koethe).  """
    self_type = None
    args = Args([Float("standard_deviation", default=1.0), Int("order", default=1)])
    return_type = ImageType([FLOAT])
    category = "Convolution/Kernels"

class BinomialKernel(PluginFunction):
    """Creates a binomial filter kernel for use with separable
    convolution of a given radius.

*radius*
   The radius of the kernel.

Uses code from the Vigra library (Copyright 1998-2002 by Ullrich
Koethe).
"""
    self_type = None
    args = Args([Int("radius", default=3)])
    return_type = ImageType([FLOAT])
    category = "Convolution/Kernels"

class AveragingKernel(PluginFunction):
    """Creates an Averaging filter kernel for use with separable
convolution.  The window size is (2*radius+1) * (2*radius+1).

*radius*
   The radius of the kernel.

Uses code from the Vigra library (Copyright 1998-2002 by Ullrich
Koethe).
"""
    self_type = None
    args = Args([Int("radius", default=3)])
    return_type = ImageType([FLOAT])
    category = "Convolution/Kernels"

class SymmetricGradientKernel(PluginFunction):
    """Init as a symmetric gradient filter of the form
       [ 0.5, 0.0, -0.5]

Uses code from the Vigra library (Copyright 1998-2002 by Ullrich Koethe).
"""
    self_type = None
    return_type = ImageType([FLOAT])
    category = "Convolution/Kernels"

class SimpleSharpeningKernel(PluginFunction):
    """Creates a kernel for simple sharpening.

"""
    self_type = None
    args = Args([Float('sharpening_factor', default=0.5)])
    return_type = ImageType([FLOAT])
    category = "Convolution/Kernels"

########################################
# Convolution applications
#
# The following are some applications of convolution built from the above
# parts.  This could have been implemented by calling the corresponding
# Vigra functions directly, but that would have increased the compiled
# binary size of an already large module emmensely.  This approach has
# slightly more overhead, being in Python, but it should hopefully
# not have a significant impact. MGD

class gaussian_smoothing(PluginFunction):
    """Performs gaussian smoothing on an image.

*standard_deviation*
   The standard deviation of the Gaussian kernel.

Uses code from the Vigra library (Copyright 1998-2002 by Ullrich Koethe).
"""
    self_type = ImageType(ALL)
    args = Args([Float("standard_deviation", default=1.0)])
    return_type = ImageType(ALL)
    pure_python = True
    doc_examples = [(GREYSCALE, 1.0), (RGB, 3.0)]
    def __call__(self, std_dev=1.0):
        return self.convolve_xy(
            _convolution.GaussianKernel(std_dev),
            border_treatment = BORDER_TREATMENT_REFLECT)
    __call__ = staticmethod(__call__)

class simple_sharpen(PluginFunction):
    """Perform simple sharpening.

*sharpening_factor*
   The amount of sharpening to perform.

Uses code from the Vigra library (Copyright 1998-2002 by Ullrich Koethe).
"""
    self_type = ImageType(ALL)
    args = Args([Float("sharpening_factor", default=0.5)])
    return_type = ImageType(ALL)
    pure_python = True
    doc_examples = [(GREYSCALE, 1.0), (RGB, 3.0)]
    def __call__(self, sharpening_factor=0.5):
        return self.convolve(
            _convolution.SimpleSharpeningKernel(sharpening_factor),
            border_treatment = BORDER_TREATMENT_REFLECT)
    __call__ = staticmethod(__call__)

class gaussian_gradient(PluginFunction):
    """Calculate the gradient vector by means of a 1st derivatives of
Gaussian filter.

*scale*

Uses code from the Vigra library (Copyright 1998-2002 by Ullrich Koethe).
"""
    self_type = ImageType(ALL)
    args = Args([Float("scale", default=0.5)])
    return_type = ImageType(ALL)
    pure_python = True
    doc_examples = [(GREYSCALE, 1.0), (RGB, 1.0)]
    def __call__(self, scale=1.0):
        smooth = _convolution.GaussianKernel(scale)
        grad = _convolution.GaussianDerivativeKernel(scale, 1)
        tmp_a = self.convolve_x(grad)
        tmp_b = tmp_a.convolve_y(smooth)
        tmp_a = self.convolve_x(smooth)
        return tmp_a.convolve_y(grad)
    __call__ = staticmethod(__call__)

class ConvolutionModule(PluginModule):
    cpp_headers=["convolution.hpp"]
    cpp_namespace=["Gamera"]
    category = "Convolution"
    functions = [convolve, convolve_xy, convolve_x, convolve_y,
                 GaussianKernel, GaussianDerivativeKernel,
                 BinomialKernel, AveragingKernel,
                 SymmetricGradientKernel, SimpleSharpeningKernel,
                 gaussian_smoothing, simple_sharpen,
                 gaussian_gradient]
    author = "Michael Droettboom"
    url = "http://gamera.dkc.jhu.edu/"
module = ConvolutionModule()

BORDER_TREATMENT_AVOID = 0
BORDER_TREATMENT_CLIP = 1
BORDER_TREATMENT_REPEAT = 2
BORDER_TREATMENT_REFLECT = 3
BORDER_TREATMENT_WRAP = 4

GaussianKernel = GaussianKernel()
GaussianDerivativeKernel = GaussianDerivativeKernel()
BinomialKernel = BinomialKernel()
AveragingKernel = AveragingKernel()
SymmetricGradientKernel = SymmetricGradientKernel()
SimpleSharpeningKernel = SimpleSharpeningKernel();
