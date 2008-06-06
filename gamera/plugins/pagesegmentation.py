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
    """
    Segments a page with the *Iterative Projection Profile Cuttings*
    method.

    The image is split recursively in the horizontal and vertical
    direction by looking for 'gaps' in the projection profile.  A
    'gap' is a contiguous sequence of projection values smaller than
    *noise* pixels. The splitting is done for each gap wider or higher
    than given thresholds *Tx* or *Ty*. When no further split points
    are found, the recursion stops.

    Whether the resulting segments represent lines, columns or
    paragraphs depends on the values for *Tx* and *Ty*. The return
    value is a list of 'CCs' where each 'CC' represents a found
    segment. Note that the input image is changed such that each pixel
    is set to its CC label.

    *Tx*:
      minimum 'gap' width in the horizontal direction.  When set to
      zero, *Tx* is set to the median height of all connected
      component * 7, which might be a reasonable assumption for the
      gap width between adjacent text columns.

    *Ty*:
      minimum 'gap' width in the vertical direction.  When set to
      zero, *Ty* is set to half the median height of all connected
      component, which might be a reasonable assumption for the gap
      width between adjacent text lines.

    *noise*:
      maximum projection value still consideread as belonging to a
      'gap'.
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
    """
    Segments a page with the *Run Length Smearing* algorithm.

    The algorithm converts white horizontal and vertical runs shorter
    than given thresholds *Cx* and *Cy* to black pixels (this is the
    so-called 'run length smearing').

    The intersection of both smeared images yields the page segments
    as black regions. As this typically still consists small white
    horizontal gaps, these gaps narrower than *Csm* are in a final
    step also filled out.

    The return value is a list of 'CCs' where each 'CC' represents a
    found segment. Note that the input image is changed such that each
    pixel is set to its CC label.

    Arguments:

    *Cx*:
      Minimal length of white runs in the rows. When set to *-1*, it
      is set to 20 times the median height of all connected
      components.

    *Cy*:
      Minimal length of white runs in the columns. When set to *-1*,
      it is set to 20 times the median height of all connected
      components.

    *Csm*:
      Minimal length of white runs row-wise in the almost final
      image. When set to *-1*, it is set to 3 times the median height
      of all connected components.
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
    """
    Further subsegments the result of a page segmentation algorithm into
    groups of actual connected components.

    The result of a page segmenattion plugin is a list of 'CCs' where
    each 'CC' does not represent a 'connected component', but a page
    segment (typically a line of text). In a practical OCR application
    you will however need the actual connected components (which
    should roughly corresond to the glyphs) in groups of lines. That
    is what this plugin is meant for.

    The input image must be an image that has been processed with a
    page segmentation plugin, i.e. all pixels in the image must be
    labeled with a segment label. The input parameter *cclist* is the
    list of sgements returned by the page segmentation algorithm.

    The return value is a tuple with two entries:

    - a new image with all pixels labeled according to the new CCs
    - a list of ImageLists, each list containing all connected components
      belonging to the same page segments


    .. note:: The groups will be returned in the same order as given
          in *cclist*.  This means that you can sort the page segments
          by reading order before passing them to *sub_cc_analysis*.

          Note that the order of the returned CCs within each group is
          not well defined. Hence you will generally need to sort each
          subgroup by reading order.
    """
    self_type = ImageType([ONEBIT])
    return_type = Class("img_ccs", tuple)
    args = Args([ImageList('cclist')])
    author = "Stephan Ruloff and Christoph Dalitz"


class textline_reading_order(PluginFunction):
    """
    Sorts a list of Images (CCs) representing textlines by reading order and
    returns the sorted list. Incidentally, this will not only work on
    textlines, but also on paragraphs, but *not* on actual Connected 
    Components.

    The algorithm sorts all lines in topological order, based on
    the following criteria for the pairwise order of two lines:

    - line *a* comes before line *b* when *a* is totally to the left
      of *b* (order \"column before row\")

    - line *a* comes before *b* when both overlap horizontally and
      *a* is above *b* (order within a column)

    In the reference `\"High Performance Document Analysis\"`__
    by T.M. Breuel (2003 Symposium on Document Image Understanding, USA),
    an additional constraint is made for the first criteria by demanding
    that no other segment may be between *a* and *b* that opverlaps
    horizontally with both. This constraint for taking multi column
    headings that interrupt columns into account is replaced in this
    implementation with an a priori sort of all textlines by *y*-position.
    This results in a preference of rows over columns (in case of ambiguity)
    in the depth-first-search utilized in the topological sorting.

    .. __: http://pubs.iupr.org/DATA/2003-breuel-sdiut.pdf

    As this function is not an image method, but a free function, it
    is not automatically imported with all plugins and you must import
    it explicitly with

    .. code:: Python

      from gamera.plugins.pagesegmentation import textline_reading_order

    """
    self_type = None
    return_type = ImageList("orderedccs")
    args = Args([ImageList("lineccs")])
    pure_python = True
    author = "Christoph Dalitz"
    def __call__(lineccs):
        # utilities for Gamera's graph API
        from gamera import graph
        from gamera import graph_util
        class SegForGraph:
            def __init__(self,seg):
                self.segment = seg
                self.label = 0
        #
        # build directed graph of all lines
        #
        G = graph.Graph(graph.FLAG_DAG)
        seg_data = [SegForGraph(s) for s in lineccs]
        # sort by y-position for row over column preference in ambiguities
        seg_data.sort(lambda s,t: s.segment.offset_y - t.segment.offset_y)
        G.add_nodes(seg_data)
        for s in seg_data:
            for t in seg_data:
                if s.segment.offset_x <= t.segment.offset_x + t.segment.ncols and \
                        s.segment.offset_x + s.segment.ncols >= t.segment.offset_x:
                    if s.segment.offset_y < t.segment.offset_y:
                        G.add_edge(s,t)
                elif s.segment.offset_x < t.segment.offset_x:
                        G.add_edge(s,t)
        #
        # compute topoligical sorting by depth-first-search
        #
        segs_sorted = [] # topologically sorted list
        def dfs_visit(node):
            node.data.label = 1
            for nextnode in node.nodes:
                if nextnode.data.label == 0:
                    dfs_visit(nextnode)
            segs_sorted.append(node.data.segment)
        for node in G.get_nodes():
            if node.data.label == 0:
                dfs_visit(node)
        segs_sorted.reverse() # correct that we always appended to the back
        return segs_sorted

    __call__ = staticmethod(__call__)


# module declaration
class PageSegmentationModule(PluginModule):
    cpp_headers = ["pagesegmentation.hpp"]
    cpp_namespace = ["Gamera"]
    category = "PageSegmentation"
    functions = [projection_cutting, runlength_smearing, sub_cc_analysis, textline_reading_order]
module = PageSegmentationModule() # create an instance of the module

# free function instances
textline_reading_order = textline_reading_order()

