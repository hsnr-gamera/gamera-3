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

The coordinates can be specified either by four integers or two Points:

  *y1*:
    Starting *y* coordinate.
  *x1*:
    Starting *x* coordinate.
  *y2*:
    Ending *y* coordinate.
  *x2*:
    Ending *x* coordinate.

**or**

  *a*:
    The start ``Point``.
  *b*:
    The end ``Point``.

*value*:
  The pixel value to set for the line.

Based on Po-Han Lin's "Extremely Fast Line Algorithm", which is based
on the classical Breshenham's algorithm.

Freely useable in non-commercial applications as long as credits to
Po-Han Lin and link to http://www.edepot.com is provided in source
code and can been seen in compiled executable.
"""
  self_type = ImageType(ALL)
  args = Args([Int("y1"), Int("x1"), Int("y2"), Int("x2"), Pixel("value")])
  doc_examples = [(ONEBIT, 5, 5, 20, 25, 1), (RGB, 5, 5, 20, 25, RGBPixel(255, 0, 0))]
  authors = "Michael Droettboom based on Po-Han Lin's Extremely Fast Line Algorithm"
  def __call__(self, *args):
    if len(args) == 5:
      return _draw.draw_line(self, *args)
    else:
      try:
        a = args[0]
        b = args[1]
        value = args[2]
        return _draw.draw_line(self, a.y, a.x, b.y, b.x, value)
      except KeyError, AttributeError:
        raise ValueError("Arguments are incorrect.")
  __call__ = staticmethod(__call__)

class draw_hollow_rect(PluginFunction):
  """Draws a hollow rectangle.

The coordinates can be specified either by four integers or two Points:

  *y1*:
    Starting *y* coordinate.
  *x1*:
    Starting *x* coordinate.
  *y2*:
    Ending *y* coordinate.
  *x2*:
    Ending *x* coordinate.

**or**

  *a*:
    The start ``Point``.
  *b*:
    The end ``Point``.

*value*:
  The pixel value to set for the lines.
"""
  self_type = ImageType(ALL)
  args = Args([Int("y1"), Int("x1"), Int("y2"), Int("x2"), Pixel("value")])
  doc_examples = [(ONEBIT, 5, 5, 20, 25, 1), (RGB, 5, 5, 20, 25, RGBPixel(255, 0, 0))]

  def __call__(self, *args):
    if len(args) == 5:
      return _draw.draw_hollow_rect(self, *args)
    else:
      try:
        a = args[0]
        b = args[1]
        value = args[2]
        return _draw.draw_hollow_rect(self, a.y, a.x, b.y, b.x, value)
      except KeyError, AttributeError:
        raise ValueError("Arguments are incorrect.")
  __call__ = staticmethod(__call__)

class draw_filled_rect(PluginFunction):
  """Draws a filled rectangle.

The coordinates can be specified either by four integers or two Points:

  *y1*:
    Starting *y* coordinate.
  *x1*:
    Starting *x* coordinate.
  *y2*:
    Ending *y* coordinate.
  *x2*:
    Ending *x* coordinate.

**or**

  *a*:
    The start ``Point``.
  *b*:
    The end ``Point``.

*value*:
  The pixel value to set for the rectangle.
"""
  self_type = ImageType(ALL)
  args = Args([Int("y1"), Int("x1"), Int("y2"), Int("x2"), Pixel("value")])
  doc_examples = [(ONEBIT, 5, 5, 20, 25, 1), (RGB, 5, 5, 20, 25, RGBPixel(255, 0, 0))]

  def __call__(self, *args):
    if len(args) == 5:
      return _draw.draw_filled_rect(self, *args)
    else:
      try:
        a = args[0]
        b = args[1]
        value = args[2]
        return _draw.draw_filled_rect(self, a.y, a.x, b.y, b.x, value)
      except KeyError, AttributeError:
        raise ValueError("Arguments are incorrect.")
  __call__ = staticmethod(__call__)

class draw_bezier_curve(PluginFunction):
  """Draws a bezier curve

The coordinates can be specified either by six integers or three Points:

  *y1*:
    Starting *y* coordinate.
  *x1*:
    Starting *x* coordinate.
  *y2*:
    Ending *y* coordinate.
  *x2*:
    Ending *x* coordinate.
  *y3*
    Control point *y* coordinate.
  *x3*
    Control point *x* coordinate.

**or**

  *a*:
    The start ``Point``.
  *b*:
    The end ``Point``.
  *c*
    The control ``Point``.

*value*:
  The pixel value to set for the curve.
"""
  self_type = ImageType(ALL)
  args = Args([Int("y1"), Int("x1"), Int("y2"), Int("x2"),
               Int("y3"), Int("x3"), Pixel("value")])
  doc_examples = [(ONEBIT, 5, 5, 20, 25, 15, 20, 1)]
  def __call__(self, *args):
    if len(args) == 7:
      return _draw.draw_bezier_curve(self, *args)
    else:
      try:
        a = args[0]
        b = args[1]
        c = args[2]
        value = args[3]
        return _draw.draw_bezier_curve(self, a.y, a.x, b.y, b.x,
                                       c.y, c.x, value)
      except KeyError, AttributeError:
        raise ValueError("Arguments are incorrect.")
  __call__ = staticmethod(__call__)

class flood_fill(PluginFunction):
  """Flood fills from the given point using the given color.  This is similar
to the "bucket" tool found in many paint programs.

The coordinates can be specified either by two integers or one Point:

  *y*:
    Starting *y* coordinate.
  *x*:
    Starting *x* coordinate.

**or**

  *a*:
    The start ``Point``.
  *b*:
    The end ``Point``.

*color*:
  The pixel value to set for the rectangle.
"""
  self_type = ImageType([GREYSCALE, FLOAT, ONEBIT, RGB])
  args = Args([Int("y"), Int("x"), Pixel("color")])
  doc_examples = [(ONEBIT, 58, 10, 0)]

  def __call__(self, *args):
    if len(args) == 3:
      return _draw.flood_fill(self, *args)
    else:
      try:
        a = args[0]
        value = args[1]
        return _draw.flood_fill(self, a.y, a.x, value)
      except KeyError, AttributeError:
        raise ValueError("Arguments are incorrect.")
  __call__ = staticmethod(__call__)

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
  functions = [draw_line, draw_bezier_curve,
               draw_hollow_rect, draw_filled_rect, flood_fill,
               remove_border, highlight]
  author = "Michael Droettboom"
  url = "http://gamera.dkc.jhu.edu/"

module = DrawModule()
