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
import _logical
  
class LogicalCombine(PluginFunction):
  self_type = ImageType([ONEBIT])
  return_type = ImageType([ONEBIT])
  args = Args([ImageType([ONEBIT], "other"), Check("in_place", default=False)])

class and_image(LogicalCombine):
  """
  Perform the AND operation on two images.

  Since it would be difficult to determine what exactly to do if the images
  are a different size, the two images must be the same size.

  *in_place*
    If true, the operation will be performed in-place, changing the
    contents of the current image.

  See or_image_ for some usage examples.
  """
  def __call__(self, other, in_place=False):
    return _logical.and_image(self, other, in_place)
  __call__ = staticmethod(__call__)

  def __doc_example1__(images):
    from gamera.core import Dim
    onebit = images[ONEBIT].subimage((0, 0), Dim(70, 68))
    greyscale = images[GREYSCALE].to_onebit().subimage((0, 0), Dim(70, 68))
    return [onebit, greyscale, onebit.and_image(greyscale, False)]
  doc_examples = [__doc_example1__]

class or_image(LogicalCombine):
  """
  Perform the OR operation on two images.

  Since it would be difficult to determine what exactly to do if the
  images are a different size, the two images must be the same size.

  *in_place*
    If true, the operation will be performed in-place, changing the
    contents of the current image.

  Usage examples:

  Using logical functions in different ways will generally involve
  creating temporary subimages for regions of interest.  Subimages are
  very lightweight objects that keep track of a bounding box and refer
  to the underlying data, therefore creating/destroying a number of
  these on the fly should not have a significant impact on
  performance.

  Padding an image.

  .. code:: Python

    def pad_image(image, padding):
      new_image = Image(0, 0,
                        image.nrows + padding * 2, image.ncols + padding * 2,
                        ONEBIT, DENSE)
      new_image.subimage((padding, padding), Dim(image.nrows, image.ncols)).or_image(image, True)
      return new_image

  Stamping an image over a larger image.  Use subimage to change the
  destination of the stamp.

  .. code:: Python

    # stamp: a small stamp image
    # paper: a larger destination image
    for x in range(0, 100, 10):
      paper.subimage(0, x, stamp.nrows, stamp.ncols).or_image(stamp, True)

  Putting part of a source image on the upper-left corner of a
  destination image.

  .. code:: Python

    # src: a source image
    # dest: a destination image
    dest.or_image(src.subimage(50, 50, 25, 25), True)

  Removing a connected component from its original image.

  .. code:: Python

    # src: the original image
    # cc: a cc on that image
    src.clip_image(cc).xor_image(cc, True)
  """
  def __call__(self, other, in_place=False):
    return _logical.or_image(self, other, in_place)
  __call__ = staticmethod(__call__)

  def __doc_example1__(images):
    from gamera.core import Dim
    onebit = images[ONEBIT].subimage((0, 0), Dim(70, 68))
    greyscale = images[GREYSCALE].to_onebit().subimage((0, 0), Dim(70, 68))
    return [onebit, greyscale, onebit.or_image(greyscale, False)]
  doc_examples = [__doc_example1__]

class xor_image(LogicalCombine):
  """
  Perform the XOR operation on two images.

  Since it would be difficult to determine what exactly to do if the images
  are a different size, the two images must be the same size.

  *in_place*
    If true, the operation will be performed in-place, changing the
    contents of the current image.

  See or_image_ for some usage examples.
  """
  def __call__(self, other, in_place=False):
    return _logical.xor_image(self, other, in_place)
  __call__ = staticmethod(__call__)

  def __doc_example1__(images):
    from gamera.core import Dim
    onebit = images[ONEBIT].subimage((0, 0), Dim(70, 68))
    greyscale = images[GREYSCALE].to_onebit().subimage((0, 0), Dim(70, 68))
    return [onebit, greyscale, onebit.xor_image(greyscale, False)]
  doc_examples = [__doc_example1__]

class LogicalModule(PluginModule):
  """
  This module provides methods to perform basic logical (bitwise)
  operations on images.
  """
  category = "Combine/Logical"
  cpp_headers = ["logical.hpp"]
  functions = [and_image, or_image, xor_image]
  author = "Michael Droettboom"
  url = "http://gamera.sourceforge.net/"

module = LogicalModule()

del LogicalCombine
