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

class despeckle(PluginFunction):
  """Removes connected components that are smaller than the given size.

*size*
  The maximum number of pixels in each connected component that
  will be removed.

This approach to finding connected components uses a pseudo-recursive
descent, which gets around the hard limit of ~64k connected components
per page in ``cc_analysis``.  Unfortunately, this approach is much
slower as the connected components get large, so *size* should be
kept relatively small.

*size* == 1 is a special case and runs much faster, since it does not
require recursion.
  """
  self_type = ImageType([ONEBIT])
  args = Args([Int('cc_size', range=(1, 100))])
  doc_examples = [(ONEBIT,5), (ONEBIT,15)]

class MorphologyModule(PluginModule):
  cpp_headers = ["morphology.hpp", "morphology_line_gaps.hpp"]
  category = "Morphology"
  functions = [erode_dilate, erode, dilate, rank, mean, despeckle]
  author = "Michael Droettboom and Karl MacMillan"
  url = "http://gamera.dkc.jhu.edu/"

module = MorphologyModule()
