#
# Copyright (C) 2001-2005 Michael Droettboom and Robert Ferguson
#               2009      Christoph Dalitz
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

"""Edge detectors based on first and second derivatives, and related
post-processing from the VIGRA Computer Vision Library."""

from gamera.plugin import *
import _edgedetect

########################################
# Edge detection methods

class difference_of_exponential_edge_image(PluginFunction):
      u"""
      EXPERIMENTAL

      Detect and mark edges in an edge image using the Shen/Castan
      zero-crossing detector.

      Uses code from the VIGRA Computer Vision Library (Copyright
      1998-2007 by Ullrich K\u00f6the).

      *scale*
        The scale relates to the value b of the exponential filter.

      *gradient_threshold*
	Whenever the gradient at a zero crossing is greater than the
	given gradient_threshold, an edge point is marked in the
	destination image on the darker side of the zero crossing (the
	zero crossing occurs between pixels).

      *min_edge_length*
	Removes all edges shorter than the number of pixels specified
	(0 retains all edges).  Values near 10 are suggested.
      """
      self_type = ImageType([GREYSCALE, GREY16, FLOAT])
      args = Args([Real("scale", (0, 1e300), default=0.8),
                   Real("gradient_threshold", (0, 1e300), default=4.0),
                   Int("min_edge_length", (0, 32000), default=0)])
      return_type = ImageType([GREYSCALE, GREY16, FLOAT])
      def __call__(self, scale=0.8, gradient_threshold=4.0, min_edge_length=0):
            return _edgedetect.difference_of_exponential_edge_image(
                  self, scale, gradient_threshold, min_edge_length)
      __call__ = staticmethod(__call__)
      doc_examples = [(GREYSCALE,)]

class difference_of_exponential_crack_edge_image(PluginFunction):
      u"""EXPERIMENTAL

      Detect and mark edges in a crack edge image using the
      Shen/Castan zero-crossing detector.

      Uses code from the VIGRA Computer Vision Library (Copyright
      1998-2007 by Ullrich K\u00f6the).

      *scale*
	The scale relates to the value b of the exponential filter.

      *gradient_threshold*
	Whenever the gradient at a zero crossing is greater than the
	given gradient threshold, an edge point is marked in the
	destination image on the darker side of the zero crossing (the
	zero crossing occurs between pixels).

      *min_edge_length*
	Removes all edges shorter than the number of pixels specified
	(0 retains all edges).  Values near 10 are suggested.

      *close_gaps*
        Close one pixel wide gaps.

      *beautify*
        See the VIGRA Docs.
      """
      self_type = ImageType([GREYSCALE, GREY16, FLOAT])
      args = Args([Real("scale", (0, 1e300), default=0.8),
                   Real("gradient_threshold", (0, 1e300), default=4.0),
                   Int("min_edge_length", (0, 32000), default=0),
                   Check("close_gaps", default=False), 
                   Check("beautify", default=False)])
      return_type = ImageType([GREYSCALE, GREY16, FLOAT])
      def __call__(self, scale=0.8, gradient_threshold=4.0, min_edge_length=0,
                   close_gaps=False, beautify=False):
            return _edgedetect.difference_of_exponential_crack_edge_image(
                  self, scale,
                  gradient_threshold, min_edge_length, close_gaps, beautify)
      __call__ = staticmethod(__call__)
      doc_examples = [(GREYSCALE,)]

class canny_edge_image(PluginFunction):
      u"""EXPERIMENTAL

      Detect and mark edges in an edge image using Canny's algorithm.

      Uses code from the VIGRA Computer Vision Library (Copyright
      1998-2007 by Ullrich K\u00f6the).

      *scale*
	The scale relates to the value b of the exponential filter.

      *gradient_threshold*
	This operator first calls cannyEdgelList() to generate an
	edgel list for the given image. Than it scans this list and
	selects edgels whose strength is above the given
	gradient_threshold.
      """
      self_type = ImageType([GREYSCALE, GREY16, FLOAT])
      args = Args([Real("scale", [0, 1e300], default=0.8),
                   Real("gradient_threshold", [0, 1e300], default=4.0)])
      return_type = ImageType([GREYSCALE, GREY16, FLOAT])
      def __call__(self, scale=0.8, gradient_threshold=4.0):
            return _edgedetect.canny_edge_image(self, scale, gradient_threshold)
      __call__ = staticmethod(__call__)
      doc_examples = [(GREYSCALE,)]

class labeled_region_edges(PluginFunction):
  """
  Pixels with a label different from one of its neighboring pixels
  are marked black in the returned image.

  When *mark_both* is ``True``, both edges of the region border are
  marked, resulting in a two pixel wide edge.
  """
  self_type = ImageType([ONEBIT, GREYSCALE, GREY16, RGB])
  args = Args([Check("mark_both", default=False)])
  return_type = ImageType([ONEBIT])
  author = "Christoph Dalitz"
  # wrapper for passing default argument
  def __call__(self, mark_both=False):
      return _edgedetect.labeled_region_edges(self, mark_both)
  __call__ = staticmethod(__call__)

class outline(PluginFunction):
    """
    Traces the outline of the image.  This result is obtained by
    dilating the image and then XOR'ing the result with the original.
    """
    self_type = ImageType([ONEBIT])
    return_type = ImageType([ONEBIT])
    doc_examples = [(ONEBIT,)]

class EdgeDetect(PluginModule):
      category = "Edge"
      cpp_headers=["edgedetect.hpp"]
      functions = [difference_of_exponential_edge_image,
                   difference_of_exponential_crack_edge_image,
                   canny_edge_image,
                   labeled_region_edges,
                   outline]
      author = u"Ullrich K\u00f6the (wrapped by Robert Ferguson)"
      url = "http://gamera.sourceforge.net/"
module = EdgeDetect()

