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

class corelation_weighted(PluginFunction):
    """Returns a floating-point value for how well an image is
corelated to another image placed at a given origin (*x*, *y*).  Uses the
weighted reward/penalty method.

*template*
   The template image.
*y*, *x*
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
"""
    return_type = Float("corelation")
    self_type = ImageType([ONEBIT, GREYSCALE])
    args = Args([ImageType([ONEBIT], "template"),
                 Int("y_offset"), Int("x_offset"),
                 Float("bb"), Float("bw"), Float("wb"), Float("ww")])
    progress_bar = "Correlating"

class corelation_sum(PluginFunction):
    """Returns a floating-point value for how well an image is
corelated to another image placed at a given origin (*x*, *y*).
Uses the sum of absolute distance method.  A higher value indicates
more corelation.

*template*
   The template image.
*y*, *x*
   The displacement of the template on the image.
"""
    return_type = Float("corelation")
    self_type = ImageType([ONEBIT, GREYSCALE])
    args = Args([ImageType([ONEBIT], "template"), Int("y_offset"), Int("x_offset")])
    progress_bar = "Correlating"

class corelation_sum_squares(PluginFunction):
    """Returns a floating-point value for how well an image is
corelated to another image placed at a given origin (*x*, *y*).
Uses the sum of squares method.  A higher value indicates
more corelation.

*template*
   The template image.
*y*, *x*
   The displacement of the template on the image.
"""
    return_type = Float("corelation")
    self_type = ImageType([ONEBIT, GREYSCALE])
    args = Args([ImageType([ONEBIT], "template"), Int("y_offset"), Int("x_offset")])
    progress_bar = "Correlating"

class CorelationModule(PluginModule):
    cpp_headers=["corelation.hpp"]
    category = "Corelation"
    functions = [corelation_weighted, corelation_sum,
                 corelation_sum_squares]
    author = "Michael Droettboom"
    url = "http://gamera.dkc.jhu.edu/"
module = CorelationModule()
