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
import _morphology

#TODO: Change these to out-of-place

class erode(PluginFunction):
  """Erodes the image by the image morphology method."""
  self_type = ImageType([ONEBIT, GREYSCALE, FLOAT])
  doc_examples = [(GREYSCALE,), (ONEBIT,)]

class dilate(PluginFunction):
  """Dilates the image by the image morphology method."""
  self_type = ImageType([ONEBIT, GREYSCALE, FLOAT])
  doc_examples = [(GREYSCALE,), (ONEBIT,)]

class erode_dilate(PluginFunction):
  """Erodes or dilates the image by the image morphology method.

*ntimes*
  The number of times to perform the operation.
*direction*
  dilate (0)
    increase the presence of black
  erode (1)
    decrease the presence of black
*shape*
  rectangular (0)
    use a 3x3 rectangular morphology operator
  octagonal (1)
    use a 3x3 octagonal morphology operator
"""
  self_type = ImageType([ONEBIT, GREYSCALE, FLOAT])
  args = Args([Int('ntimes', range=(0, 10), default=1), \
               Choice('direction', ['dilate', 'erode']), \
               Choice('shape', ['rectangular', 'octagonal'])])
  doc_examples = [(GREYSCALE, 10, 0, 1)]

class rank(PluginFunction):
  """Within each 3x3 window, set the center pixel to the *n*-th ranked
value.

*rank* (1 - 9)
  The rank of the 9 pixels to select for the center.  5 is equivalent to
  the median."""
  self_type = ImageType([ONEBIT, GREYSCALE, FLOAT])
  args = Args([Int('rank', range=(1, 9))])
  doc_examples = [(GREYSCALE, 2), (GREYSCALE, 5), (GREYSCALE, 8)]

class mean(PluginFunction):
  """Within each 3x3 window, set the center pixel to the mean value of
all 9 pixels."""
  self_type = ImageType([GREYSCALE, FLOAT])
  doc_examples = [(GREYSCALE,)]

class MorphologyModule(PluginModule):
  cpp_headers = ["morphology.hpp"]
  cpp_namespaces = ["Gamera"]
  category = "Morphology"
  functions = [erode_dilate, erode, dilate, rank, mean]
  author = "Michael Droettboom and Karl MacMillan"
  url = "http://gamera.dkc.jhu.edu/"

module = MorphologyModule()
