#
# Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, Karl MacMillan
#               2010      Oliver Christen, Christoph Dalitz
#               2011      Christoph Dalitz
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

from gamera.plugin import *

import _contour

#TODO: Change these to out-of-place

class Contour(PluginFunction):
  self_type = ImageType([ONEBIT])
  return_type = FloatVector("contour")
  doc_examples = [(ONEBIT,)]

class contour_top(Contour):
  """
  Returns a float vector containing the contour at the top of the
  image.

  If there are no black pixels in a given column, the value is set to
  inf.
  """
  pass

class contour_bottom(Contour):
  """
  Returns a float vector containing the contour at the bottom of the
  image.

  If there are no black pixels in a given column, the value is set to
  inf.
  """
  pass

class contour_left(Contour):
  """
  Returns a float vector containing the contour at the left of the
  image.

  If there are no black pixels in a given row, the value is set to
  inf.
  """
  pass

class contour_right(Contour):
  """
  Returns a float vector containing the contour at the right of the
  image.

  If there are no black pixels in a given row, the value is set to
  inf.
  """
  pass

class contour_samplepoints(PluginFunction):
  """
  Returns a point vector containing contour points of the given image.
  
  *percentage*:
    return percentage of contour points. The points are selected approximately
    equidistant on the countour.

  *contour*:
    when 0 (\"outer_projection\"), the points returned by *contour_left* etc.
    are used; when 1 (\"full_contour\") the points returned by *outline(1)*
    are used.
  
  In addition to the points determined by the percentage argument the result
  list also contains the four extreme points (topmost, leftmost, bottommost,
  rightmost).
  
  .. code:: Python
   
   	ccs = image.cc_analysis()
   	points = []
   	for cc in ccs:
   	  for samplepoint in cc.contour_samplepoints(50):
   	    points.append(samplepoint)
  """
  self_type = ImageType([ONEBIT])
  author = "Oliver Christen"
  args = Args([Int("percentage", range=(1,100), default=25),
               Choice("contour", ["outer_projection", "full_contour"], default=0)])
  return_type = PointVector("contourpoints")
  doc_examples = [(ONEBIT, 10)]
  
  def __call__(self, percentage=25, contourtype=0): 	
    if percentage < 1 or percentage > 100:
      raise RuntimeError("contour_samplepoints: percentage must be between 1 and 100")
    return _contour.contour_samplepoints(self, percentage, contourtype)
  __call__ = staticmethod(__call__)

class contour_pavlidis(PluginFunction):
    """
    Returns a point list of the outer contour trace found with Pavlidis'
    algorithm (T. Pavlidis: *Algorithms for Grapics and Image Processing.*
    pp. 129-165, Springer, 1982).

    Note that this extracts only the first contour found, so this method
    should be applied to a single connected component. If you have an
    image with more than one connected component, do a CC analysis before,
    as in the following example:

    .. code:: Python

      ccs = img.cc_analysis()
      contours = []
      for cc in ccs:
        contours.append([Point(p.x + cc.offset_x, p.y + cc.offset_y) \\
                         for p in cc.contour_pavlidis()])

    """
    self_type = ImageType([ONEBIT])
    return_type = PointVector("contour")
    author = "Andreas Leuschner"

class ContourModule(PluginModule):
  cpp_headers = ["contour.hpp"]
  category = "Analysis/Contour"
  functions = [contour_top, contour_left, contour_bottom, contour_right,
               contour_samplepoints, contour_pavlidis]
  author = "Michael Droettboom"
  url = "http://gamera.sourceforge.net/"

module = ContourModule()
