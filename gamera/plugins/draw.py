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

class draw_line(PluginFunction):
  """Draws a straight line between two points.

*x1*:
  Starting *x* coordinate.
*y1*:
  Starting *y* coordinate.
*x2*:
  Ending *x* coordinate.
*y2*:
  Ending *y* coordinate.
*value*:
  The pixel value to set for the line.

.. note:: This needs to be extended to support more pixel types."""
  self_type = ImageType([ONEBIT])
  args = Args([Int("x1"), Int("y1"), Int("x2"), Int("y2"), Float("value")])
  doc_examples = [(ONEBIT, 5, 5, 20, 25, 1)]

class draw_hollow_rect(PluginFunction):
  """Draws a hollow rectangle.

*x1*:
  Starting *x* coordinate.
*y1*:
  Starting *y* coordinate.
*x2*:
  Ending *x* coordinate.
*y2*:
  Ending *y* coordinate.
*value*:
  The pixel value to set for the lines.

.. note:: This needs to be extended to support more pixel types."""
  self_type = ImageType([ONEBIT])
  args = Args([Int("x1"), Int("y1"), Int("x2"), Int("y2"), Float("value")])
  doc_examples = [(ONEBIT, 5, 5, 20, 25, 1)]

class draw_filled_rect(PluginFunction):
  """Draws a filled rectangle.

*x1*:
  Starting *x* coordinate.
*y1*:
  Starting *y* coordinate.
*x2*:
  Ending *x* coordinate.
*y2*:
  Ending *y* coordinate.
*value*:
  The pixel value to set for the rectangle.

.. note:: This needs to be extended to support more pixel types."""
  self_type = ImageType([ONEBIT])
  args = Args([Int("x1"), Int("y1"), Int("x2"), Int("y2"), Float("value")])
  doc_examples = [(ONEBIT, 5, 5, 20, 25, 1)]

class flood_fill(PluginFunction):
  """Flood fills from the given point using the given color.  This is similar
to the "bucket" tool found in many paint programs.

*x*:
  Starting *x* coordinate.
*y*:
  Starting *y* coordinate.
*color*:
  The pixel value to set for the rectangle.

.. note:: This needs to be extended to support more pixel types."""
  self_type = ImageType([ONEBIT])
  args = Args([Int("x"), Int("y"), Float("color")])
  doc_examples = [(ONEBIT, 5, 5, 0)]

class remove_border(PluginFunction):
  """This is a special case of the flood_fill algorithm that is designed to
remove dark borders produced by photocopiers or flatbed scanners around the
border of the image.

.. note:: This needs to be extended to support more pixel types."""
  self_type = ImageType([ONEBIT])

class DrawModule(PluginModule):
  cpp_headers = ["draw.hpp"]
  cpp_namespaces = ["Gamera"]
  category = "Draw"
  functions = [draw_line, draw_hollow_rect, draw_filled_rect, flood_fill,
               remove_border]
  author = "Michael Droettboom and Karl MacMillan"
  url = "http://gamera.dkc.jhu.edu/"

module = DrawModule()
