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



*storage_format* (optional)

  specifies the compression type for the result:



  DENSE (0)

    no compression

  RLE (1)

    run-length encoding compression"""

    self_type = ImageType([GREYSCALE])

    args = Args([Choice("storage format", ['dense', 'rle']),

                 Int("region size", range=(1, 50), default=20),

                 Int("contrast limit", range=(0, 255), default=5),

                 Check("doubt", "is low")])

    return_type = ImageType([ONEBIT], "output")

    doc_examples = [(GREYSCALE,)]

    def __call__(image, storage_format = 0, region_size = 3, contrast_limit = 128, set_doubt_to_low = 0):

        return _threshold.bernsen_threshold(image, storage_format, region_size, contrast_limit, set_doubt_to_low)

    __call__ = staticmethod(__call__)



class ThresholdModule(PluginModule):

    """This module provides functions that convert images between different

    pixel types.

    """

    category = "Thresholding"

    cpp_headers = ["threshold.hpp"]

    functions = [threshold, otsu_find_threshold, otsu_threshold, abutaleb_threshold,

                 bernsen_threshold]

    author = "Michael Droettboom and Karl MacMillan"

    url = "http://gamera.dkc.jhu.edu/"



module = ThresholdModule()
