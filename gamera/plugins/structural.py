#
# Copyright (C) 2001, 2002 Ichiro Fujinaga, Michael Droettboom,
#                          and Karl MacMillan
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

"""The relational module contains plugins for computing the relationships
between glyphs."""

from gamera.plugin import * 

class polar_distance(PluginFunction):
    """Returns a tuple containing the normalized distance, polar direction,
    and non-normalized polar distance to another glyph (based on center points."""
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])
    return_type = FloatVector("polar")
    args = Args([ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB], "other")])

class polar_match(PluginFunction):
    """Returns a tuple containing the normalized distance, polar direction,
    and non-normalized polar distance to another glyph (based on center points."""
    self_type = None
    return_type = Int("check")
    args = Args([Float('r1'), Float('q1'), Float('r2'), Float('q2')])

class RelationalModule(PluginModule):
    cpp_headers = ["structural.hpp"]
    cpp_namespace = ["Gamera"]
    category = "Relational"
    functions = [polar_distance, polar_match]
    author = "Michael Droettboom and Karl MacMillan"
    url = "http://gamera.dkc.jhu.edu/"

module = RelationalModule()
