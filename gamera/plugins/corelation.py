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

"""Various functions related to corelation (template matching)."""

from gamera.plugin import *
from gamera import util
import _corelation

class corelation_weighted(PluginFunction):
    """Returns a floating-point value for how well an image is
corelated to another image placed at a given origin (*x*, *y*).  Uses the
weighted reward/penalty method.

*template*
   The template image.
*offset* or *y*, *x*
   The displacement of the template on the image.

*bb*, *bw*, *wb*, *ww*
   The rewards and penalties for different combinations of pixels.  The
   first letter in the arugment name indicates the color of the template;
   the second letter indicates the color of the source image.  For instance,
   the value of *bw* will be applied to the result when the template pixel
   is black and the source image pixel is white.

   +--------+--------+------------------+
   | Image  |        | Template         |
   |        +--------+---------+--------+
   |        |        | black   | white  |
   |        +--------+---------+--------+
   |        | black  | *bb*    | *wb*   |
   |        +--------+---------+--------+
   |        | white  | *bw*    | *ww*   |
   +--------+--------+---------+--------+

.. warning::

  The (*y*, *x*) form is deprecated.

  Reason: (x, y) coordinate consistency.
"""
    return_type = Float("corelation")
    self_type = ImageType([ONEBIT, GREYSCALE])
    args = Args([ImageType([ONEBIT], "template"),
                 Point("offset"),
                 Float("bb"), Float("bw"), Float("wb"), Float("ww")])
    def __call__(self, *args):
        if len(args) == 6:
            return _corelation.corelation_weighted(self, *args)
        elif len(args) == 7:
            template, y, x, bb, bw, wb, ww = args
            util.warn_deprecated("""corelation_weighted(template, y, x, bb, bw, wb, ww) is deprecated.

Reason: (x, y) coordinate consistency.

Use corelation_weighted(template, (x, y), bb, bw, wb, ww) instead.""")
            result = _corelation.corelation_weighted(self, template, (x, y), bb, bw, wb, ww)
            return result
        raise ValueError("Arguments to corelation_weighted incorrect.")
    __call__ = staticmethod(__call__)

class corelation_sum(PluginFunction):
    """Returns a floating-point value for how well an image is
corelated to another image placed at a given origin (*x*, *y*).
Uses the sum of absolute distance method.  A higher value indicates
more corelation.

*template*
   The template image.
*offset* or *y*, *x*
   The displacement of the template on the image.

.. warning::

  The (*y*, *x*) form is deprecated.

  Reason: (x, y) coordinate consistency.
"""
    return_type = Float("corelation")
    self_type = ImageType([ONEBIT, GREYSCALE])
    args = Args([ImageType([ONEBIT], "template"), Point("offset")])
    progress_bar = "Correlating"
    def __call__(self, *args):
        if len(args) == 2:
            return _corelation.corelation_sum(self, *args)
        elif len(args) == 3:
            template, y, x = args
            util.warn_deprecated("""corelation_sum(template, y, x) is deprecated.

Reason: (x, y) coordinate consistency.

Use corelation_sum(template, (x, y)) instead.""")
            result = _corelation.corelation_sum(self, template, (x, y))
            return result
        raise ValueError("Arguments to corelation_sum incorrect.")
    __call__ = staticmethod(__call__)

class corelation_sum_squares(PluginFunction):
    """Returns a floating-point value for how well an image is
corelated to another image placed at a given origin (*x*, *y*).
Uses the sum of squares method.  A higher value indicates
more corelation.

*template*
   The template image.
*offset* or *y*, *x*
   The displacement of the template on the image.

.. warning::

  The (*y*, *x*) form is deprecated.

  Reason: (x, y) coordinate consistency.
"""
    return_type = Float("corelation")
    self_type = ImageType([ONEBIT, GREYSCALE])
    args = Args([ImageType([ONEBIT], "template"), Point("offset")])
    progress_bar = "Correlating"
    def __call__(self, *args):
        if len(args) == 2:
            return _corelation.corelation_sum_squares(self, *args)
        elif len(args) == 3:
            template, y, x = args
            util.warn_deprecated("""corelation_sum_squares(template, y, x) is deprecated.

Reason: (x, y) coordinate consistency.

Use corelation_sum_squares(template, (x, y)) instead.""")
            result = _corelation.corelation_sum_squares(self, template, (x, y))
            return result
        raise ValueError("Arguments to corelation_sum_squares incorrect.")
    __call__ = staticmethod(__call__)

class CorelationModule(PluginModule):
    cpp_headers=["corelation.hpp"]
    category = "Corelation"
    functions = [corelation_weighted, corelation_sum,
                 corelation_sum_squares]
    author = "Michael Droettboom"
    url = "http://gamera.dkc.jhu.edu/"
module = CorelationModule()
