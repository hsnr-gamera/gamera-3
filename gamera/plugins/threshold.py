# -*- mode: python; indent-tabs-mode: nil; tab-width: 4 -*-
# vim: set tabstop=4 shiftwidth=4 expandtab:

#
# Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, Karl MacMillan
#               2007-2010 Christoph Dalitz and Uma Kompella
#               2014      Christoph Dalitz
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
import _threshold

class threshold(PluginFunction):
    """
    Creates a binary image by splitting along a given global threshold value.

    Pixels that are greater than the given value become white.
    Pixels less than or equal to the given value become black.

    *storage_format* (optional)
      specifies the compression type for the result:

      DENSE (0)
        no compression
      RLE (1)
        run-length encoding compression.
    """
    self_type = ImageType([GREYSCALE, GREY16, FLOAT])
    args = Args([Int("threshold"), Choice("storage format", ['dense', 'rle'])])
    return_type = ImageType([ONEBIT], "output")
    doc_examples = [(GREYSCALE, 128)]
    def __call__(image, threshold, storage_format = 0):
        return _threshold.threshold(image, threshold, storage_format)
    __call__ = staticmethod(__call__)

class otsu_find_threshold(PluginFunction):
    """
    Finds a threshold point using the Otsu algorithm. Reference:

    N. Otsu: *A Threshold Selection Method from Grey-Level
    Histograms.* IEEE Transactions on Systems, Man, and Cybernetics
    (9), pp. 62-66 (1979)
    """
    self_type = ImageType([GREYSCALE])
    return_type = Int("threshold_point")
    doc_examples = [(GREYSCALE,)]

class otsu_threshold(PluginFunction):
    """
    Creates a binary image by splitting along a threshold value
    determined using the Otsu algorithm.

    Equivalent to ``image.threshold(image.otsu_find_threshold())``.

    *storage_format* (optional)
      specifies the compression type for the result:
      
      DENSE (0)
        no compression
      RLE (1)
        run-length encoding compression
    """
    self_type = ImageType([GREYSCALE])
    args = Args(Choice("storage format", ['dense', 'rle']))
    return_type = ImageType([ONEBIT], "output")
    doc_examples = [(GREYSCALE,)]
    def __call__(image, storage_format = 0):
        return _threshold.otsu_threshold(image, storage_format)
    __call__ = staticmethod(__call__)

class tsai_moment_preserving_find_threshold(PluginFunction):
    """
    Finds a threshold point using the Tsai Moment Preserving threshold
    algorithm. Reference:

    W.H. Tsai: *Moment-Preserving Thresholding: A New Approach.*
    Computer Vision Graphics and Image Processing (29), pp. 377-393
    (1985)
    """
    self_type = ImageType([GREYSCALE])
    return_type = Int("threshold_point")
    doc_examples = [(GREYSCALE,)]
    author = "Uma Kompella"

class tsai_moment_preserving_threshold(PluginFunction):
    """
    Creates a binary image by splitting along a threshold value
    determined using the Tsai Moment Preserving Threshold algorithm.

    Equivalent to
    ``image.threshold(image.tsai_moment_preserving_find_threshold())``.

    *storage_format* (optional)
      specifies the compression type for the result:

      DENSE (0)
        no compression
      RLE (1)
        run-length encoding compression
    """
    self_type = ImageType([GREYSCALE])
    args = Args(Choice("storage format", ['dense', 'rle']))
    return_type = ImageType([ONEBIT], "output")
    doc_examples = [(GREYSCALE,)]
    author = "Uma Kompella"
    def __call__(image, storage_format = 0):
        return _threshold.tsai_moment_preserving_threshold(image, storage_format)
    __call__ = staticmethod(__call__)




class abutaleb_threshold(PluginFunction):
    """
    Creates a binary image by using the Abutaleb locally-adaptive
    thresholding algorithm.

    *storage_format* (optional)
      specifies the compression type for the result:

      DENSE (0)
        no compression
      RLE (1)
        run-length encoding compression
    """
    self_type = ImageType([GREYSCALE])
    args = Args(Choice("storage format", ['dense', 'rle']))
    return_type = ImageType([ONEBIT], "output")
    doc_examples = [(GREYSCALE,)]
    def __call__(image, storage_format = 0):
        return _threshold.abutaleb_threshold(image, storage_format)
    __call__ = staticmethod(__call__)

class bernsen_threshold(PluginFunction):
    """
    Creates a binary image by using the Bernsen algorithm.

    Each point is thresholded by the mean between the maximum and minimum
    value in the surrounding region of size *region_size*. When the difference
    between maximum and minimum is below *contrast_limit* the pixel is set
    to black in case of *doubt_to_black* = ``True``, otherwise to white.

    Reference: J. Bernsen: *Dynamic thresholding of grey-level images.* 
    Proc. 8th International Conference on Pattern Recognition (ICPR8),
    pp. 1251-1255, 1986.

    *storage_format*
      specifies the compression type for the result:

      DENSE (0)
        no compression
      RLE (1)
        run-length encoding compression

    *region_size*
      The size of each region in which to calculate a threshold

    *contrast_limit*
      The minimum amount of contrast required to threshold.

    *doubt_to_black*
      When ``True``, 'doubtful' values are set to black, otherwise to white.
    """
    self_type = ImageType([GREYSCALE])
    args = Args([Choice("storage format", ['dense', 'rle']),
                 Int("region size", range=(1, 50), default=11),
                 Int("contrast limit", range=(0, 255), default=80),
                 Check("doubt_to_black", default=False)])
    return_type = ImageType([ONEBIT], "output")
    doc_examples = [(GREYSCALE,)]
    def __call__(image, storage_format = 0, region_size = 11,
                 contrast_limit = 80, doubt_to_black = False):
        return _threshold.bernsen_threshold(image, storage_format, region_size, contrast_limit, doubt_to_black)
    __call__ = staticmethod(__call__)

class djvu_threshold(PluginFunction):
    """
    Creates a binary image by using the DjVu thresholding algorithm.

    See Section 5.1 in:

      Bottou, L., P. Haffner, P. G. Howard, P. Simard, Y. Bengio and
      Y. LeCun.  1998.  High Quality Document Image Compression with
      DjVu.  AT&T Labs, Lincroft, NJ.

      http://research.microsoft.com/~patrice/PDF/jei.pdf

    This implementation features an additional extension to the
    algorithm described above.  Once the background and foreground
    colors are determined for each block, the image is thresholded by
    interpolating the foreground and background colors between the
    blocks.  This prevents "blockiness" along boundaries of strong
    color change.

    *smoothness*
      The amount of effect that parent blocks have on their children
      blocks.  Higher values will result in more smoothness between
      blocks.  Expressed as a percentage between 0.0 and 1.0.

    *max_block_size*
      The size of the largest block to determine a threshold.

    *min_block_size*
      The size of the smallest block to determine a threshold.

    *block_factor*
      The number of child blocks (in each direction) per parent block.
      For instance, a *block_factor* of 2 results in 4 children per
      parent.
    """
    self_type = ImageType([RGB])
    args = Args([Float("smoothness", default=0.2, range=(0.0, 1.0)),
                 Int("max_block_size", default=512),
                 Int("min_block_size", default=64),
                 Int("block_factor", default=2, range=(1, 8))])
    return_type = ImageType([ONEBIT], "output")
    def __call__(image, smoothness=0.2, max_block_size=512, min_block_size=64,
                 block_factor=2):
        return _threshold.djvu_threshold(image, smoothness, max_block_size,
                                         min_block_size, block_factor)
    __call__ = staticmethod(__call__)
    doc_examples = [(RGB, 0.5, 512, 64, 2)]

class soft_threshold(PluginFunction):
    """
    Does a greyscale transformation that \"smears out\" the threshold *t* by a
    choosable amount *sigma*. This has the effect of a \"soft\" thresholding.

    Each grey value *x* is transformed to *F(x,t,sigma)*, where *F*
    is the CDF of a Gaussian normal distribution with mean *t* and variance
    *sigma^2*.

    As the choice *sigma* = 0 is useless (it is the same as normal
    thresholding), this special value is reserved for an automatic selection
    of *sigma* such that *F(m,t,sigma)* = 0.99, where *m* is the mean grey
    value of all pixels with a grey value greater than *t*.
    """
    self_type = ImageType([GREYSCALE])
    args = Args([Int("t"), Float("sigma", default=0.0)])
    return_type = ImageType([GREYSCALE], "output")
    author = "Christoph Dalitz"
    def __call__(image, t, sigma=0.0):
        return _threshold.soft_threshold(image, t, sigma)
    __call__ = staticmethod(__call__)
    doc_examples = [(GREYSCALE, 128, 25)]


class ThresholdModule(PluginModule):
    """
    This module provides functions that convert images between different
    pixel types.
    """
    category = "Binarization"
    cpp_headers = ["threshold.hpp"]
    functions = [threshold, otsu_find_threshold, otsu_threshold, tsai_moment_preserving_find_threshold, tsai_moment_preserving_threshold, abutaleb_threshold,
                 bernsen_threshold, djvu_threshold, soft_threshold]
    author = "Michael Droettboom and Karl MacMillan"
    url = "http://gamera.sourceforge.net/"

module = ThresholdModule()
