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

from gamera.plugin import *
import _draw
try:
  from gamera.core import RGBPixel
except:
  def RGBPixel(*args):
    pass

class draw_line(PluginFunction):
  """Draws a straight line between two points.

*y1*:
  Starting *y* coordinate.
*x1*:
  Starting *x* coordinate.
*y2*:
  Ending *y* coordinate.
*x2*:
  Ending *x* coordinate.
*value*:
  The pixel value to set for the line.
"""
  self_type = ImageType(ALL)
  args = Args([Int("y1"), Int("x1"), Int("y2"), Int("x2"), Pixel("value")])
  doc_examples = [(ONEBIT, 5, 5, 20, 25, 1), (RGB, 5, 5, 20, 25, RGBPixel(255, 0, 0))]

class draw_hollow_rect(PluginFunction):
  """Draws a hollow rectangle.

*y1*:
  Starting *y* coordinate.
*x1*:
  Starting *x* coordinate.
*y2*:
  Ending *y* coordinate.
*x2*:
  Ending *x* coordinate.
*value*:
  The pixel value to set for the lines.
"""
  self_type = ImageType(ALL)
  args = Args([Int("y1"), Int("x1"), Int("y2"), Int("x2"), Pixel("value")])
  doc_examples = [(ONEBIT, 5, 5, 20, 25, 1), (RGB, 5, 5, 20, 25, RGBPixel(255, 0, 0))]

class draw_filled_rect(PluginFunction):
  """Draws a filled rectangle.

*y1*:
  Starting *y* coordinate.
*x1*:
  Starting *x* coordinate.
*y2*:
  Ending *y* coordinate.
*x2*:
  Ending *x* coordinate.
*value*:
  The pixel value to set for the rectangle.
"""
  self_type = ImageType(ALL)
  args = Args([Int("y1"), Int("x1"), Int("y2"), Int("x2"), Pixel("value")])
  doc_examples = [(ONEBIT, 5, 5, 20, 25, 1), (RGB, 5, 5, 20, 25, RGBPixel(255, 0, 0))]

class flood_fill(PluginFunction):
  """Flood fills from the given point using the given color.  This is similar
to the "bucket" tool found in many paint programs.

*y*:
  Starting *y* coordinate.
*x*:
  Starting *x* coordinate.
*color*:
  The pixel value to set for the rectangle.
"""
  self_type = ImageType([GREYSCALE, FLOAT, ONEBIT, RGB])
  args = Args([Int("y"), Int("x"), Pixel("color")])
  doc_examples = [(ONEBIT, 58, 10, 0)]

class remove_border(PluginFunction):
  """This is a special case of the flood_fill algorithm that is designed to
remove dark borders produced by photocopiers or flatbed scanners around the
border of the image."""
  self_type = ImageType([ONEBIT])

class highlight(PluginFunction):
  """Highlights a connected component on a given image using the given color.
Self must be an RGB image (usually the original image.)

*cc*
   A one-bit connected component from the image

*color*
   An RGBPixel color value used to color the *cc*."""
  self_type = ImageType([RGB])
  args = Args([ImageType([ONEBIT], "cc"), Pixel("color")])

class DrawModule(PluginModule):
  cpp_headers = ["draw.hpp"]
  cpp_namespaces = ["Gamera"]
  category = "Draw"
  functions = [draw_line, draw_hollow_rect, draw_filled_rect, flood_fill,
               remove_border, highlight]
  author = "Michael Droettboom and Karl MacMillan"
  url = "http://gamera.dkc.jhu.edu/"

module = DrawModule()
