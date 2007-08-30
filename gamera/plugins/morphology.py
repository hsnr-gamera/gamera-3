#
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
import _morphology

#TODO: Change these to out-of-place

class erode(PluginFunction):
  """
  Erodes the image by the image morphology method.
  """
  self_type = ImageType([ONEBIT, GREYSCALE, FLOAT])
  doc_examples = [(GREYSCALE,), (ONEBIT,)]
  return_type = ImageType([ONEBIT, GREYSCALE, FLOAT])
  pure_python = True
  def __call__(image):
    return _morphology.erode_dilate(image, 1, 1, 0)
  __call__ = staticmethod(__call__)

class dilate(PluginFunction):
  """
  Dilates the image by the image morphology method.
  """
  self_type = ImageType([ONEBIT, GREYSCALE, FLOAT])
  doc_examples = [(GREYSCALE,), (ONEBIT,)]
  return_type = ImageType([ONEBIT, GREYSCALE, FLOAT])
  pure_python = True
  def __call__(image):
    return _morphology.erode_dilate(image, 1, 0, 0)
  __call__ = staticmethod(__call__)

class erode_dilate(PluginFunction):
  """
  Erodes or dilates the image by the image morphology method.  In case
  of calling with onebit images and shape is set to rectangular
  erode_with_structure/dilate_with_structure is used.

  *ntimes*
    The number of times to perform the operation.
  *direction*
    dilate (0)
      increase the presence of black
    erode (1)
      decrease the presence of black
  *shape*
    rectangular (0)
      use a 3x3 rectangular morphology operator
    octagonal (1)
      use octagonal morphology operator by alternately using
      a 3x3 cross and a 3x3 square structuring element
  """
  self_type = ImageType([ONEBIT, GREYSCALE, FLOAT])
  args = Args([Int('ntimes', range=(0, 10), default=1), \
               Choice('direction', ['dilate', 'erode']), \
               Choice('shape', ['rectangular', 'octagonal'])])
  return_type = ImageType([ONEBIT, GREYSCALE, FLOAT])
  doc_examples = [(GREYSCALE, 10, 0, 1)]

class rank(PluginFunction):
  """
  Within each 3x3 window, set the center pixel to the *n*-th ranked
  value.

  *rank* (1 - 9)
    The rank of the 9 pixels to select for the center.  5 is equivalent to
    the median.
  """
  self_type = ImageType([ONEBIT, GREYSCALE, FLOAT])
  args = Args([Int('rank', range=(1, 9))])
  return_type = ImageType([ONEBIT, GREYSCALE, FLOAT])
  doc_examples = [(GREYSCALE, 2), (GREYSCALE, 5), (GREYSCALE, 8)]

class mean(PluginFunction):
  """
  Within each 3x3 window, set the center pixel to the mean value of
  all 9 pixels.
  """
  self_type = ImageType([GREYSCALE, FLOAT])
  doc_examples = [(GREYSCALE,)]
  return_type = ImageType([ONEBIT, GREYSCALE, FLOAT])

class despeckle(PluginFunction):
  """
  Removes connected components that are smaller than the given size.

  *size*
    The maximum number of pixels in each connected component that
    will be removed.

  This approach to finding connected components uses a pseudo-recursive
  descent, which gets around the hard limit of ~64k connected components
  per page in ``cc_analysis``.  Unfortunately, this approach is much
  slower as the connected components get large, so *size* should be
  kept relatively small.

  *size* == 1 is a special case and runs much faster, since it does not
  require recursion.
  """
  self_type = ImageType([ONEBIT])
  args = Args([Int('cc_size', range=(1, 100))])
  doc_examples = [(ONEBIT,5), (ONEBIT,15)]

class distance_transform(PluginFunction):
  """
  For all background pixels, calculate the distance to the nearest
  object or contour. In the destination image, all pixels corresponding to
  background will be assigned the their distance value, all pixels
  corresponding to objects will be assigned 0.  The result is returned
  as a Float image.

  *norm*:

    0: use chessboard distance (L-infinity norm)

    1: use Manhattan distance (L1 norm)

    2: use Euclidean distance (L2 norm)
  """
  self_type = ImageType([ONEBIT])
  args = Args([Choice("norm", ['chessboard', 'manhattan', 'euclidean'])])
  return_type = ImageType([FLOAT])
  doc_examples = [(ONEBIT,5),]
  author = u"Ullrich K\u00f6the (wrapped from VIGRA by Michael Droettboom"


class dilate_with_structure(PluginFunction):
    """
    Performs a binary morphological dilation with the given structuring
    element.

    Note that it is necessary to specify which point in the
    structuring element shall be treated as origin. This allows for
    arbitrary structuring elements. Examples:

    .. code:: Python

      # same as image.dilate()
      structure = Image(Point(0,0), Point(2,2), ONEBIT)
      structure.fill(1)
      image = image.dilate_with_structure(structure, Point(1,1))

      # same as image.erode_dilate(3,0,0)
      structure = Image(Point(0,0), Point(6,6), ONEBIT)
      structure.fill(1)
      image = image.dilate_with_structure(structure, Point(3,3))

    The implementation is straightforward and can be slow for large
    structuring elements. If you know that your structuring element is
    connected and its origin is black, you can set *only_border* to
    ``True``, because in this case only the border pixels in the image
    need to be considered which can speed up the dilation for some
    images (though not for all).

    References:

      A proof that only the contour pixels need to be dilated for
      connected structuring elements containing their origin is given
      by Luc Vincent in *Morphological Transformations of Binary
      Images with Arbitrary Structuring Elements*, Signal Processing,
      Vol. 22, No. 1, pp. 3-23, January 1991 (see theorem 2.13)
    """
    self_type = ImageType([ONEBIT])
    args = Args([ImageType([ONEBIT],'structuring_element'),
                 Point('origin'),
                 Check('only_border', default=False)])
    return_type = ImageType([ONEBIT])
    author = "Christoph Dalitz"

    def __call__(self, structuring_element, origin, only_border=False):
        return _morphology.dilate_with_structure(self, structuring_element, origin, only_border)

    __call__ = staticmethod(__call__)

class erode_with_structure(PluginFunction):
    """
    Performs a binary morphological erosion with the given structuring
    element.

    Note that it is necessary to specify which point in the
    structuring element shall be treated as origin. This allows for
    arbitrary structuring elements.
    
    Border pixels at which the structuring element extends beyond the
    image dimensions are whitened. In other words the image is padded
    with white pixels before erosion.

    Example:

    .. code:: Python

      # same as image.erode()
      structure = Image(Point(0,0), Point(2,2), ONEBIT)
      structure.fill(1)
      image = image.erode_with_structure(structure, Point(1,1))
    """
    self_type = ImageType([ONEBIT])
    args = Args([ImageType([ONEBIT],'structuring_element'),
                 Point('origin')])
    return_type = ImageType([ONEBIT])
    author = "Christoph Dalitz"

class MorphologyModule(PluginModule):
  cpp_headers = ["morphology.hpp"]
  category = "Morphology"
  functions = [erode_dilate, erode, dilate, rank, mean, despeckle,
               distance_transform, dilate_with_structure, erode_with_structure]
  author = "Michael Droettboom and Karl MacMillan"
  url = "http://gamera.dkc.jhu.edu/"

module = MorphologyModule()

