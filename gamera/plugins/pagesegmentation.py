#
# Copyright (C) 2007 Christoph Dalitz, Stefan Ruloff, Maria Elhachimi,
#                    Ilya Stoyanov
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
import _pagesegmentation

class projection_cutting(PluginFunction):
	"""Segments a page with the *Iterative Projection Profile Cuttings*
method.

The image is split recursively in the horizontal and
vertical direction by looking for 'gaps' in the projection profile.
A 'gap' is a contiguous sequence of projection values smaller than
*noise* pixels. The splitting is done for each gap wider or higher than
given thresholds *Tx* or *Ty*. When no further split points are found, the
recursion stops.

Whether the resulting segments represent lines, columns or paragraphs
depends on the values for *Tx* and *Ty*. The return value is a list of
'CCs' where each 'CC' represents a found segment. Note that the input image
is changed such that each pixel is set to its CC label.

*Tx*:
  minimum 'gap' width in the horizontal direction.
  When set to zero, *Tx* is set to the median height of all
  connected component * 7, which might be a reasonable assumption for
  the gap width between adjacent text columns.

*Ty*:
  minimum 'gap' width in the vertical direction.
  When set to zero, *Ty* is set to half the median height of all
  connected component, which might be a reasonable assumption for
  the gap width between adjacent text lines.

*noise*:
  maximum projection value still consideread as belonging to a 'gap'.
"""
	self_type = ImageType([ONEBIT])
	args = Args([Int('Tx', default = 0), Int('Ty', default = 0), \
		Int('noise', default = 0)])
	return_type = ImageList("ccs")
	author = "Maria Elhachimi"
	def __call__(image, Tx = 0, Ty = 0, noise = 0):
		return _pagesegmentation.projection_cutting(image, Tx, Ty, noise)
	__call__ = staticmethod(__call__)

class runlength_smearing(PluginFunction):
	"""Segments a page with the *Run Length Smearing* algorithm.

The algorithm converts white horizontal and vertical runs shorter than
given thresholds *Cx* and *Cy* to black pixels (this is the so-called
'run length smearing').
The intersection of both smeared images yields the page segments as black
regions. As this typically still consists small white horizontal gaps,
these gaps narrower than *Csm* are in a final step also filled out.

The return value is a list of
'CCs' where each 'CC' represents a found segment. Note that the input image
is changed such that each pixel is set to its CC label.

Arguments:

*Cx*:
  Minimal length of white runs in the rows. When set to *-1*, it is set
  to 20 times the median height of all connected components.

*Cy*:
  Minimal length of white runs in the columns. When set to *-1*, it is
  set to 20 times the median height of all connected components.

*Csm*:
  Minimal length of white runs row-wise in the almost final image. When
  set to *-1*, it is set to 3 times the median height of all
  connected components.
"""
	self_type = ImageType([ONEBIT])
	return_type = ImageList("ccs")
	args = Args([Int('Cx', default = -1), Int('Cy', default = -1), \
		Int('Csm', default = -1)])
	author = "Iliya Stoyanov"
	def __call__(image, Cx = -1, Cy = -1, Csm = -1):
		return _pagesegmentation.runlength_smearing(image, Cx, Cy, Csm)
	__call__ = staticmethod(__call__)


class sub_cc_analysis(PluginFunction):
	"""Further subsegments the result of a page segmentation algorithm into
groups of actual connected components.

The result of a page segmenattion plugin is a list of 'CCs' where each
'CC' does not represent a 'connected component', but a page segment
(typically a line of text). In a practical OCR application you will
however need the actual connected components (which should roughly corresond
to the glyphs) in groups of lines. That is what this plugin is meant for.

The input image must be an image that has been processed with a page
segmentation plugin, i.e. all pixels in the image must be labeled with a
segment label. The input parameter *cclist* is the list of sgements returned 
by the page segmentation algorithm.

The return value is a tuple with two entries:

 - a new image with all pixels labeled according to the new CCs
 - a list of ImageLists, each list containing all connected components
   belonging to the same page segments


.. note:: The groups will be returned in the same order as given in *cclist*.
          This means that you can sort the page segments by reading order
          before passing them to *sub_cc_analysis*.

          Note that the order of the returned CCs within each group is not
          well defined. Hence you will generally need to sort each subgroup
          by reading order.
"""
	self_type = ImageType([ONEBIT])
	return_type = Class("img_ccs", tuple)
	args = Args([ImageList('cclist')])
	author = "Stephan Ruloff and Christoph Dalitz"


# module declaration
class PageSegmentationModule(PluginModule):
    cpp_headers = ["pagesegmentation.hpp"]
    cpp_namespace = ["Gamera"]
    category = "PageSegmentation"
    functions = [projection_cutting, runlength_smearing, sub_cc_analysis]
module = PageSegmentationModule() # create an instance of the module

