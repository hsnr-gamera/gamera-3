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



from gamera.plugin import *



#TODO: Change these to out-of-place



class Contour(PluginFunction):

  self_type = ImageType([ONEBIT])

  return_type = FloatVector("contour")

  doc_examples = [(ONEBIT,)]



class contour_top(Contour):

  """Returns a float vector containing the contour at the top of the image.



If there are no black pixels in a given column, the value is set to inf.

"""

  pass



class contour_bottom(Contour):

  """Returns a float vector containing the contour at the bottom of the image.



If there are no black pixels in a given column, the value is set to inf.

"""

  pass



class contour_left(Contour):

  """Returns a float vector containing the contour at the left of the image.



If there are no black pixels in a given row, the value is set to inf.

"""

  pass



class contour_right(Contour):

  """Returns a float vector containing the contour at the right of the image.



If there are no black pixels in a given row, the value is set to inf.

"""

  pass



class ContourModule(PluginModule):

  cpp_headers = ["contour.hpp"]

  category = "Analysis/Contour"

  functions = [contour_top, contour_left, contour_bottom, contour_right]

  author = "Michael Droettboom"

  url = "http://gamera.dkc.jhu.edu/"



module = ContourModule()
