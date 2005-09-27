#
# Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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
import _threshold

class threshold(PluginFunction):
    """Creates a binary image by splitting along a given threshold value.

Pixels that are greater than the given value become white.
Pixels less than the given value become black.

*storage_format* (optional)
  specifies the compression type for the result:

  DENSE (0)
    no compression
  RLE (1)
    run-length encoding compression"""
    self_type = ImageType([GREYSCALE, GREY16, FLOAT])
    args = Args([Int("threshold"), Choice("storage format", ['dense', 'rle'])])
    return_type = ImageType([ONEBIT], "output")
    doc_examples = [(GREYSCALE, 128)]
    def __call__(image, threshold, storage_format = 0):
        return _threshold.threshold(image, threshold, storage_format)
    __call__ = staticmethod(__call__)

class otsu_find_threshold(PluginFunction):
    """Finds a threshold point using the Otsu algorithm"""
    self_type = ImageType([GREYSCALE])
    return_type = Int("threshold_point")
    doc_examples = [(GREYSCALE,)]

class otsu_threshold(PluginFunction):
    """Creates a binary image by splitting along a threshold value determined
using the Otsu algorithm.

Equivalent to ``image.threshold(image.otsu_find_threshold())``.

*storage_format* (optional)
  specifies the compression type for the result:

  DENSE (0)
    no compression
  RLE (1)
    run-length encoding compression"""
    self_type = ImageType([GREYSCALE])
    args = Args(Choice("storage format", ['dense', 'rle']))
    return_type = ImageType([ONEBIT], "output")
    doc_examples = [(GREYSCALE,)]
    def __call__(image, storage_format = 0):
        return _threshold.otsu_threshold(image, storage_format)
    __call__ = staticmethod(__call__)

class abutaleb_threshold(PluginFunction):
    """Creates a binary image by using the Abutaleb locally-adaptive thresholding
algorithm.

*storage_format* (optional)
  specifies the compression type for the result:

  DENSE (0)
    no compression
  RLE (1)
    run-length encoding compression"""
    self_type = ImageType([GREYSCALE])
    args = Args(Choice("storage format", ['dense', 'rle']))
    return_type = ImageType([ONEBIT], "output")
    doc_examples = [(GREYSCALE,)]
    def __call__(image, storage_format = 0):
        return _threshold.abutaleb_threshold(image, storage_format)
    __call__ = staticmethod(__call__)

class bernsen_threshold(PluginFunction):
    """Creates a binary image by using the Bernsen algorithm.

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

*doubt*
  When True, *doubt* is low.
"""
    self_type = ImageType([GREYSCALE])
    args = Args([Choice("storage format", ['dense', 'rle']),
                 Int("region size", range=(1, 50), default=20),
                 Int("contrast limit", range=(0, 255), default=5),
                 Check("doubt", "is low")])
    return_type = ImageType([ONEBIT], "output")
    doc_examples = [(GREYSCALE,)]
    def __call__(image, storage_format = 0, region_size = 3,
                 contrast_limit = 128, set_doubt_to_low = 0):
        return _threshold.bernsen_threshold(image, storage_format, region_size, contrast_limit, set_doubt_to_low)
    __call__ = staticmethod(__call__)

class djvu_threshold(PluginFunction):
    """Creates a binary image by using the DjVu thresholding algorithm.

See Section 5.1 in:

Bottou, L., P. Haffner, P. G. Howard, P. Simard, Y. Bengio and
Y. LeCun.  1998.  High Quality Document Image Compression with DjVu.  AT&T
Labs, Lincroft, NJ.

http://research.microsoft.com/~patrice/PDF/jei.pdf

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
    doc_examples = [(RGB, 0.5, 512, 64, 2)]

class ThresholdModule(PluginModule):
    """This module provides functions that convert images between different
    pixel types.
    """
    category = "Thresholding"
    cpp_headers = ["threshold.hpp"]
    functions = [threshold, otsu_find_threshold, otsu_threshold, abutaleb_threshold,
                 bernsen_threshold, djvu_threshold]
    author = "Michael Droettboom and Karl MacMillan"
    url = "http://gamera.dkc.jhu.edu/"

module = ThresholdModule()
