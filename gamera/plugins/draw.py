#
#
# Copyright (C) 2001 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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
  self_type = ImageType([ONEBIT])
  args = Args([Int("x1"), Int("y1"), Int("x2"), Int("y2"), Float("value")])
draw_line = draw_line()

class draw_hollow_rect(PluginFunction):
  self_type = ImageType([ONEBIT])
  args = Args([Int("x1"), Int("y1"), Int("x2"), Int("y2"), Float("value")])
draw_hollow_rect = draw_hollow_rect()

class draw_filled_rect(PluginFunction):
  self_type = ImageType([ONEBIT])
  args = Args([Int("x1"), Int("y1"), Int("x2"), Int("y2"), Float("value")])
draw_filled_rect = draw_filled_rect()

class DrawModule(PluginModule):
  cpp_headers = ["draw.hpp"]
  cpp_namespaces = ["Gamera"]
  category = "Draw"
  functions = [draw_line, draw_hollow_rect, draw_filled_rect]
  author = "Michael Droettboom and Karl MacMillan"
  url = "http://gamera.dkc.jhu.edu/"

module = DrawModule()
