#
#
# Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
#               2010 Christoph Dalitz
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

"""The transformation module contains plugins for geometric transformations
like rotating or resizing."""

from gamera.plugin import * 
try:
  from gamera.core import RGBPixel
except:
  def RGBPixel(*args):
    pass
from gamera.gui import has_gui
from gamera.util import warn_deprecated
from gamera.args import NoneDefault
import sys
import _transformation

class rotate(PluginFunction):
    """
    Rotates an image.

    *angle*
      The angle of rotation (in degrees)

    *bgcolor*
      The color to use for pixels outside of the original image bounds.
      When *bgcolor* is ``None``, white is used.

    *order*
      The order of the spline used for interpolation.  Must be between 1 - 3.
    """
    category = "Transformation"
    self_type = ImageType(ALL)    
    return_type = ImageType(ALL)
    args = Args([Float("angle"), Pixel("bgcolor", default=NoneDefault), Int("order", range=(1,3), default=1)])
    args.list[0].rng = (-180,180)
    doc_examples = [(RGB, 32.0, RGBPixel(255, 255, 255), 3), (COMPLEX, 15.0, 0.0j, 3)]
    author = u"Michael Droettboom (With code from VIGRA by Ullrich K\u00f6the)"
    def __call__(self, angle, bgcolor=None, order=1):
      if (bgcolor == None):
          bgcolor = self.white()
      return _transformation.rotate(self, angle, bgcolor, order)
    __call__ = staticmethod(__call__)
    
class resize(PluginFunction):
    """
    Returns a resized copy of an image. In addition to size, the type
    of interpolation can be specified, with a tradeoff between speed
    and quality.

    If you need to maintain the aspect ratio of the original image,
    consider using scale_ instead.

    *dim*
      The size of the resulting image.

    *interp_type* [None|Linear|Spline]
      The type of interpolation used to resize the image.  Each option
      is progressively higher quality, yet slower.

    .. _scale: #scale
    """
    category = "Transformation"
    self_type = ImageType(ALL)
    args = Args([Dim("dim"), Choice("interp_type", ["None", "Linear", "Spline"])])
    return_type = ImageType(ALL)

class scale(PluginFunction):
    """
    Returns a scaled copy of the image. In addition to scale, the type
    of interpolation can be specified, with a tradeoff between speed
    and quality.

    If you need to change the aspect ratio of the original image,
    consider using resize_ instead.

    *scale*
      A scaling factor.  Values greater than 1 will result in a larger image.
      Values less than 1 will result in a smaller image.

    *interp_type* [None|Linear|Spline]
      The type of interpolation used to resize the image.  Each option is
      progressively higher quality, yet slower.

    .. _resize: #resize
    """
    category = "Transformation"
    self_type = ImageType(ALL)
    args= Args([Real("scaling"),
                Choice("interp_type", ["None", "Linear", "Spline"])])
    return_type = ImageType(ALL)
    doc_examples = [(RGB, 0.5, 2), (RGB, 2.0, 2)]

class shear_row(PluginFunction):
    """
    Shears a given row by a given amount.

    *row*
      The row number to shear.

    *distance*
      The number of pixels to move the row.  Positive values
      move the row to the right.  Negative values move the row
      to the left.
    """
    category = "Transformation"
    self_type = ImageType(ALL)
    args = Args([Int('row'), Int('distance')])
    doc_examples = [(ONEBIT, 50, 10)]

class shear_column(PluginFunction):
    """
    Shears a given column by a given amount.

    *column*
      The column number to shear.

    *distance*
      The number of pixels to move the column.  Positive values
      move the column downward.  Negative values move the column
      upward.
    """
    category = "Transformation"
    self_type = ImageType(ALL)
    args = Args([Int('column'), Int('distance')])
    doc_examples = [(ONEBIT, 50, 10)]

class mirror_horizontal(PluginFunction):
    """
    Flips the image across the horizontal (*x*) axis.
    """
    category = "Transformation"
    self_type = ImageType(ALL)
    doc_examples = [(RGB,)]

class mirror_vertical(PluginFunction):
    """
    Flips the image across the vertical (*y*) axis.
    """
    category = "Transformation"
    self_type = ImageType(ALL)
    doc_examples = [(RGB,)]


class TransformationModule(PluginModule):
    cpp_headers=["transformation.hpp"]
    category = "Transformation"
    functions = [rotate, resize, scale,
                 shear_row, shear_column,
                 mirror_horizontal, mirror_vertical]
    author = "Michael Droettboom, Karl MacMillan, and Christoph Dalitz"
    url = "http://gamera.sourceforge.net/"
module = TransformationModule()

