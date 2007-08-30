#
# Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom,
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

class bounding_box_grouping_function(PluginFunction):
    """
    Given two rectangles *a*, *b*, and a given *threshold* distance
    (in pixels), returns ``True`` if the two rectangles are closer
    than *threshold*.
    """
    self_type = None
    args = Args([Rect("a"), Rect("b"), Int("threshold")])
    return_type = Check("connected")

class shaped_grouping_function(PluginFunction):
    """
    Given two connected components *a*, *b*, and a given *threshold*
    distance (in pixels), returns ``True`` if any pixel in *a* are
    closer than *threshold* to any pixel in *b*.
    """
    self_type = None
    args = Args([ImageType(ONEBIT, "a"), ImageType(ONEBIT, "b"), Int("threshold")])
    return_type = Check("connected")

class polar_distance(PluginFunction):
    """
    Returns a tuple containing the normalized distance, polar
    direction, and non-normalized polar distance to another glyph
    (based on center of bounding boxes).
    """
    self_type = ImageType(ALL)
    return_type = FloatVector("polar")
    args = Args([ImageType(ALL, "other")])

class polar_match(PluginFunction):
    self_type = None
    return_type = Int("check")
    args = Args([Float('r1'), Float('q1'), Float('r2'), Float('q2')])

class least_squares_fit(PluginFunction):
    """
    Performs a least squares fit on a given list of points.

    The result is a tuple of the form (*m*, *b*, *q*) where *m* is the
    slope of the line, *b* is the *y*-offset, and *q* is the gamma fit
    of the line to the points.  (This assumes the same statistical
    significance for all points.
    
    See Numerical Recipes in C, section 15.2__ for more information.

    .. __: http://www.library.cornell.edu/nr/bookcpdf/c15-2.pdf
    """
    self_type = None
    return_type = Class("a_b_q")
    args = Args([PointVector("points")])

class least_squares_fit_xy(PluginFunction):
    """
    Identical to *least_squares_fit* for line angles below 45 degrees.
    For lines with a more vertical slope a least square fit of *x = my
    + b* is done instead.

    The result is a tuple of the form (*m*, *b*, *q*, *x_of_y*) where
    *m, b, q* are the same as in *least_squares_fit*, but the integer
    value *x_of_y* determines the actual meaning of the parameters *m*
    and *b*:

    When *x_of_y* is zero, *y = mx + b*. Otherwise *x = my + b*.
    """
    self_type = None
    return_type = Class("a_b_q_xofy")
    args = Args([PointVector("points")])
    author = "Christoph Dalitz"

class RelationalModule(PluginModule):
    cpp_headers = ["structural.hpp"]
    category = "Relational"
    functions = [polar_distance, polar_match,
                 bounding_box_grouping_function,
                 shaped_grouping_function,
                 least_squares_fit, least_squares_fit_xy]
    author = "Michael Droettboom and Karl MacMillan"
    url = "http://gamera.dkc.jhu.edu/"

module = RelationalModule()

bounding_box_grouping_function = bounding_box_grouping_function()
shaped_grouping_function = shaped_grouping_function()
least_squares_fit = least_squares_fit()
least_squares_fit_xy = least_squares_fit_xy()
