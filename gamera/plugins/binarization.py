#
# Copyright (C) 2005      John Ashley Burgoyne and Ichiro Fujinaga
#               2012      Andrew Hankinson
#               2011-2012 Christoph Dalitz
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


# TODO: Add GREY16 compatibility.
# TODO: Add Yanowitz and Bruckstein post-processing (a la Trier and Jain).

"""Adaptive binarization tools."""

from gamera.plugin import *
from gamera.args import NoneDefault
import _binarization

class image_mean(PluginFunction):
    """
    Returns the mean over all pixels of an image as a FLOAT.
    """
    category = "Binarization/RegionInformation"
    return_type = Real("output")
    self_type = ImageType([GREYSCALE,GREY16,FLOAT])
    def __call__(self):
        return _binarization.image_mean(self)
    __call__ = staticmethod(__call__)


class image_variance(PluginFunction):
    """
    Returns the variance over all pixels of an image as a FLOAT.
    """
    category = "Binarization/RegionInformation"
    return_type = Real("output")
    self_type = ImageType([GREYSCALE,GREY16,FLOAT])
    def __call__(self):
        return _binarization.image_variance(self)
    __call__ = staticmethod(__call__)


class mean_filter(PluginFunction):
    """
    Returns the regional mean of an image as a FLOAT.

    *region_size*
      The size of the region in which to calculate a mean.
    """
    category = "Binarization/RegionInformation"
    return_type = ImageType([FLOAT], "output")
    self_type = ImageType([GREYSCALE,GREY16,FLOAT])
    args = Args([Int("region size", default=5)])
    doc_examples = [(GREYSCALE,), (GREY16,), (FLOAT,)]
    def __call__(self, region_size=5):
        return _binarization.mean_filter(self, region_size)
    __call__ = staticmethod(__call__)


class variance_filter(PluginFunction):
    """
    Returns the regional variance of an image as a FLOAT.

    *means*
      Pre-calculated means for each region.

    *region_size*
      The size of the region in which to calculate the variance.
    """
    category = "Binarization/RegionInformation"
    return_type = ImageType([FLOAT], "output")
    self_type = ImageType([GREYSCALE,GREY16,FLOAT])
    args = Args([ImageType([FLOAT], "means"),
                 Int("region size", default=5)])
    def __call__(self, means, region_size=5):
        return _binarization.variance_filter(self, means, region_size)
    __call__ = staticmethod(__call__)


class wiener_filter(PluginFunction):
    """
    Adaptive Wiener filter for de-noising.

    See:
    
    J. Lim. 2001. *Two-Dimensional Signal Processing.* Englewood
    Cliffs: Prentice Hall.

    *region_size*
      The size of the region within which to calculate the filter
      coefficients.

    *noise_variance* 
      Variance of the noise in the image. If negative, estimated
      automatically as the median of local variances.
    """
    category = "Filter"
    return_type = ImageType([GREYSCALE,GREY16,FLOAT], "output")
    self_type = ImageType([GREYSCALE,GREY16,FLOAT])
    args = Args([Int("region size", default=5),
                 Real("noise variance", default=-1.0)])
    doc_examples = [(GREYSCALE,), (GREY16,), (FLOAT,)]
    def __call__(self, region_size=5, noise_variance=-1):
        return _binarization.wiener_filter(self, region_size, noise_variance)
    __call__ = staticmethod(__call__)


class niblack_threshold(PluginFunction):
    """
    Creates a binary image using Niblack's adaptive algorithm.

    Niblack, W. 1986. *An Introduction to Digital Image Processing.* Englewood
    Cliffs, NJ: Prentice Hall.

    Like the QGAR library, there are two extra global thresholds for
    the lightest and darkest regions.

    *region_size*
      The size of the region in which to calculate a threshold.

    *sensitivity*
      The sensitivity weight on the variance.

    *lower bound*
      A global threshold beneath which all pixels are considered black.

    *upper bound*
      A global threshold above which all pixels are considered white.
    """
    return_type = ImageType([ONEBIT], "output")
    self_type = ImageType([GREYSCALE])
    args = Args([Int("region size", default=15),
                 Real("sensitivity", default=-0.2),
                 Int("lower bound", range=(0,255), default=20),
                 Int("upper bound", range=(0,255), default=150)])
    doc_examples = [(GREYSCALE,)]
    def __call__(self, 
                 region_size=15, 
                 sensitivity=-0.2,
                 lower_bound=20,
                 upper_bound=150):
        return _binarization.niblack_threshold(self, 
                                               region_size, 
                                               sensitivity,
                                               lower_bound,
                                               upper_bound)
    __call__ = staticmethod(__call__)

   
class sauvola_threshold(PluginFunction):
    """
    Creates a binary image using Sauvola's adaptive algorithm.

    Sauvola, J. and M. Pietikainen. 2000. Adaptive document image
    binarization.  *Pattern Recognition* 33: 225--236.

    Like the QGAR library, there are two extra global thresholds for
    the lightest and darkest regions.

    *region_size*
      The size of the region in which to calculate a threshold.

    *sensitivity*
      The sensitivity weight on the adjusted variance.

    *dynamic_range*
      The dynamic range of the variance.

    *lower bound*
      A global threshold beneath which all pixels are considered black.

    *upper bound*
      A global threshold above which all pixels are considered white.
    """
    return_type = ImageType([ONEBIT], "output")
    self_type = ImageType([GREYSCALE])
    args = Args([Int("region size", default=15),
                 Real("sensitivity", default=0.5),
                 Int("dynamic range", range=(1, 255), default=128),
                 Int("lower bound", range=(0,255), default=20),
                 Int("upper bound", range=(0,255), default=150)])
    doc_examples = [(GREYSCALE,)]
    def __call__(self, 
                 region_size=15, 
                 sensitivity=0.5, 
                 dynamic_range=128,
                 lower_bound=20,
                 upper_bound=150):
        return _binarization.sauvola_threshold(self, 
                                               region_size, 
                                               sensitivity, 
                                               dynamic_range,
                                               lower_bound,
                                               upper_bound)
    __call__ = staticmethod(__call__)

class gatos_background(PluginFunction):
    """
    Estimates the background of an image according to Gatos et al.'s
    method. See:

    Gatos, Basilios, Ioannis Pratikakis, and Stavros
    J. Perantonis. 2004. An adaptive binarization technique for low
    quality historical documents. *Lecture Notes in Computer
    Science* 3163: 102--113.

    *region_size* 
      Region size for interpolation.

    *binarization*
      A preliminary binarization of the image.
    """
    category = "Binarization/RegionInformation"
    return_type = ImageType([GREYSCALE], "output")
    self_type = ImageType([GREYSCALE])
    args = Args([ImageType([ONEBIT], "binarization"),
                 Int("region size", default=15)])
    def __call__(self, binarization, region_size=15):
        return _binarization.gatos_background(self, binarization, region_size)
    __call__ = staticmethod(__call__)
        

class gatos_threshold(PluginFunction):
    """
    Thresholds an image according to Gatos et al.'s method. See:

    Gatos, Basilios, Ioannis Pratikakis, and Stavros
    J. Perantonis. 2004. An adaptive binarization technique for low
    quality historical documents. *Lecture Notes in Computer
    Science* 3163: 102-113.

    *background*
      Estimated background of the image.

    *binarization*
      A preliminary binarization of the image.

    Use the default settings for the other parameters unless you know
    what you are doing.
    """
    return_type = ImageType([ONEBIT], "output")
    self_type = ImageType([GREYSCALE])
    args = Args([ImageType([GREYSCALE], "background"),
                 ImageType([ONEBIT], "binarization"),
                 Real("q", default=0.6),
                 Real("p1", default=0.5),
                 Real("p2", default=0.8)])
    def __call__(self, background, binarization, q=0.6, p1=0.5, p2=0.8):
        return _binarization.gatos_threshold(self, 
                                             background, 
                                             binarization, 
                                             q, 
                                             p1, 
                                             p2)
    __call__ = staticmethod(__call__)


class white_rohrer_threshold(PluginFunction):
    """
    Creates a binary image using White and Rohrer's dynamic thresholding
    algorithm. It is the first of the two algorithms described in:

    J. M. White and G. D. Rohrer. 1983. Image thresholding for optical
    character recognition and other applications requiring character
    image extraction.  *IBM J. Res. Dev.* 27(4), pp. 400-411

    The algorithm uses a 'running' average instead of true average of
    the gray values in the neighborhood.  The lookahead parameter
    gives the number of lookahead pixels used in the biased running
    average that is used in deciding the threshold at each pixel
    location.

    *x_lookahead*
      the number of lookahead pixels in the horizontal direction for
      computing the running average. White and Rohrer suggest a value
      of 8 for a 240 dpi scanning resolution.

    *y_lookahead*
      number of lines used for further averaging from the horizontal
      averages.

    The other parameters are for calculating biased running average.
    Without bias the thresholding decision would be determined by
    noise fluctuations in uniform areas.

    This implementation uses code from XITE__.

    .. __: http://www.ifi.uio.no/forskning/grupper/dsb/Software/Xite/

    .. note::

       Permission to use, copy, modify and distribute this software
       and its documentation for any purpose and without fee is hereby
       granted, provided that this copyright notice appear in all
       copies and that both that copyright notice and this permission
       notice appear in supporting documentation and that the name of
       B-lab, Department of Informatics or University of Oslo not be
       used in advertising or publicity pertaining to distribution of
       the software without specific, written prior permission.

       B-LAB DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
       INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
       FITNESS, IN NO EVENT SHALL B-LAB BE LIABLE FOR ANY SPECIAL,
       INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
       RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
       ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
       ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
       THIS SOFTWARE.
    """
    return_type = ImageType([ONEBIT], "onebit")
    self_type = ImageType([GREYSCALE])
    args = Args([Int("x lookahead", default=8),
                 Int("y lookahead", default=1),
                 Int("bias mode", default=0),
                 Int("bias factor", default=100),
                 Int("f factor",default=100),
                 Int("g factor",default=100)])
    author = "Uma Kompella (using code from the XITE library)"
    doc_examples = [(GREYSCALE,)]

    def __call__(self, x_lookahead=8, y_lookahead=1, bias_mode=0,
                 bias_factor=100, f_factor=100, g_factor=100):
        return _binarization.white_rohrer_threshold(
            self, 
            x_lookahead, 
            y_lookahead,
            bias_mode,
            bias_factor,
            f_factor,
            g_factor)
    __call__ = staticmethod(__call__)


class shading_subtraction(PluginFunction):
    """
    Thresholds an image after subtracting a -possibly shaded- background.

    First the background image is extracted with a maximum filter with a
    *k\*k* window, and this image is subtracted from the original image.
    On the difference image, a threshold is applied, and the inverted
    image thereof is the binarization result.

    Parameters:

    *k*
      Window size of the maximum filter. Must be odd. For decent results,
      it must be chosen so large that every window includes at least one
      background pixel.

    *threshold*
      Threshold applied to the difference image. A possibly reasonable
      value might lie around 20. When ``None``, the threshold is
      determined automatically with otsu_find_threshold_.

    .. _otsu_find_threshold: binarization.html#otsu-find-threshold

    Reference: K.D. Toennies: *Grundlagen der Bildverarbeitung.* 
    Pearson Studium, 2005, p.202
    """
    author = "Christoph Dalitz"
    return_type = ImageType([ONEBIT], "onebit")
    self_type = ImageType([GREYSCALE])
    args = Args([Int("k", default=7), Int("threshold", default=NoneDefault)])
    pure_python = True
    doc_examples = [(GREYSCALE,)]

    def __call__(self, k=7, threshold=None):
        #background = self.rank(k*k,k,border_treatment=1)
        background = self.min_max_filter(k,1)
        backfloat = background.to_float()
        imgfloat = self.to_float()
        difffloat = backfloat.subtract_images(imgfloat)
        if threshold is None:
            diffgrey = difffloat.to_greyscale()
            diffgrey.invert()
            return diffgrey.otsu_threshold()
        else:
            onebit = difffloat.threshold(threshold)
            onebit.invert()
            return onebit

    __call__ = staticmethod(__call__)

class brink_threshold(PluginFunction):
    """
    Calculates threshold for image with Brink and Pendock's minimum-cross    
    entropy method and returns corrected image. It is best used for binarising
    images with dark, near-black foreground and significant bleed-through.
    To that end, it generally predicts lower thresholds than other
    thresholding algorithms.

    Reference: A.D. Brink, N.E. Pendock: Minimum cross-entropy threshold selection.
    Pattern Recognition 29 (1), 1996. 179-188.
    """
    author = "Johanna Devaney, Brian Stern"
    self_type = ImageType([GREYSCALE])
    return_type = ImageType([ONEBIT], "onebit")
    doc_examples = [(GREYSCALE,)]
    def __call__(self):
        return _binarization.brink_threshold(self)
    __call__ = staticmethod(__call__)


class BinarizationGenerator(PluginModule):
    category = "Binarization"
    cpp_headers = ["binarization.hpp"]
    functions = [image_mean,
                 image_variance,
                 mean_filter,
                 variance_filter,
                 wiener_filter,
                 niblack_threshold, 
                 sauvola_threshold,
                 gatos_background,
                 gatos_threshold,
                 white_rohrer_threshold,
                 shading_subtraction,
                 brink_threshold]
    author = "John Ashley Burgoyne and Ichiro Fujinaga"
    url = "http://gamera.sourceforge.net/"

module = BinarizationGenerator()

