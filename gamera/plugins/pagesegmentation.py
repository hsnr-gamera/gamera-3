#
# Copyright (C) 2007-2009 Christoph Dalitz, Stefan Ruloff, Robert Butz,
#                         Maria Elhachimi, Ilya Stoyanov, Rene Baston
#               2010-2014 Christoph Dalitz
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
from gamera.plugins.listutilities import kernel_density, argmax
from math import sqrt

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
      maximum projection value still considered as belonging to a
      'gap'.

    *gap_treatment*:
      decides how to treat gaps when *noise* is non zero.
      When 0 ('cut'), gaps are cut in the middle and the noise pixels
      in the gap are assigned to the segments.
      When 1 ('ignore'), noise pixels within the gap are not assigned
      to a segment, in other words, they are ignored.
    """
    self_type = ImageType([ONEBIT])
    args = Args([Int('Tx', default = 0), Int('Ty', default = 0), \
		 Int('noise', default = 0), Choice('gap_treatment', ["cut", "ignore"], default=0)])
    return_type = ImageList("ccs")
    author = "Maria Elhachimi and Robert Butz"
    def __call__(image, Tx = 0, Ty = 0, noise = 0, gap_treatment = 0):
	    return _pagesegmentation.projection_cutting(image, Tx, Ty, noise, gap_treatment)
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
    author = "Christoph Dalitz and Iliya Stoyanov"
    def __call__(image, Cx = -1, Cy = -1, Csm = -1):
	return _pagesegmentation.runlength_smearing(image, Cx, Cy, Csm)
    __call__ = staticmethod(__call__)


class bbox_merging(PluginFunction):
    """
    Segments a page by extending and merging the bounding boxes of the
    connected components on the page.

    How much the segments are extended is controlled by the arguments
    *Ex* and *Ey*. Depending on their value, the returned segments
    can be lines or paragraphs or something else.

    The return value is a list of 'CCs' where each 'CC' represents a
    found segment. Note that the input image is changed such that each
    pixel is set to its segment label.

    Arguments:

    *Ex*:
      How much each CC is extended to the left and right before merging.
      When *-1*, it is set to twice the median width of all CCs.

    *Ey*:
      How much each CC is extended to the top and bottom before merging.
      When *-1*, it is set to the median height of all CCs.
      This will typically segment into paragraphs.

      If you want to segment into lines, set *Ey* to something small like
      one sixth of the median symbol height.

    *iterations*:
      After merging intersecting bounding boxes, it can happen that the
      enclosing bounding boxes of different segments still intersect.
      If you do not want this, set *iterations* > 1 (two will typically be
      sufficient). If you however only want actually intersecting bounding
      boxes to be merged, set *iterations* to one.
    """
    self_type = ImageType([ONEBIT])
    return_type = ImageList("ccs")
    args = Args([Int('Ex', default = -1), Int('Ey', default = -1), Int('iterations', default=2)])
    pure_python = True
    author = "Rene Baston, Karl MacMillan, and Christoph Dalitz"

    def __call__(self, Ex=-1, Ey=-1, iterations=2):
        # bbox with contained cc indices
        class Bbox:
            def __init__(self, allccs, indices):
                self.ccs = allccs
                self.indices = indices
                if len(indices) == 1:
                    self.rect = Rect(allccs[indices[0]])
                else:
                    self.rect = allccs[indices[0]].union_images([allccs[i] for i in indices])
            def extend(self, Ex, Ey, img):
                ul_y = max(0, self.rect.ul_y - Ey)
                ul_x = max(0, self.rect.ul_x - Ex)
                lr_y = min(img.lr_y, self.rect.lr_y + Ey)
                lr_x = min(img.lr_x, self.rect.lr_x + Ex)
                nrows = lr_y - ul_y + 1
                ncols = lr_x - ul_x + 1
                self.rect = Rect(Point(ul_x, ul_y), Dim(ncols, nrows))
            def merge(self, other):
                self.indices += other.indices
                self.rect.union(other.rect)
        # does one merging step
        def merge_boxes(bboxes):
            from gamera import graph
            bboxes.sort(lambda b1, b2: b1.rect.ul_y-b2.rect.ul_y)
            g = graph.Graph(graph.UNDIRECTED)
            # build graph where edge means overlap of two boxes
            for i in range(len(bboxes)):
                g.add_node(i)
            for i in range(len(bboxes)):
                for j in range(i+1, len(bboxes)):
                    if bboxes[j].rect.ul_y > bboxes[i].rect.lr_y:
                        break
                    if bboxes[i].rect.intersects(bboxes[j].rect):
                        if not g.has_edge(i,j):
                            g.add_edge(i,j)
            new_bboxes = []
            for sg in g.get_subgraph_roots():
                seg = [n() for n in g.BFS(sg)]
                bbox = bboxes[seg[0]]
                for i in range(1, len(seg)):
                    bbox.merge(bboxes[seg[i]])
                new_bboxes.append(bbox)
            return new_bboxes

        # the actual plugin
        from gamera.core import Dim, Rect, Point, Cc
        from gamera.plugins.listutilities import median

        page = self.image_copy()
        ccs = page.cc_analysis()

        # compute average CC size
        if Ex == -1:
            Ex = 2*median([c.ncols for c in ccs])
        if Ey == -1:
            Ey = median([c.nrows for c in ccs])

        # create merged segments
        bboxes = [Bbox(ccs, [i]) for i in range(len(ccs))]
        for bb in bboxes:
            bb.extend(Ex, Ey, page)
        for i in range(iterations):
            oldlen = len(bboxes)
            bboxes = merge_boxes(bboxes)
            if oldlen == len(bboxes):
                break
        seg_ccs = []
        for i,bbox in enumerate(bboxes):
            label = i+1
            ccs_of_segment = [ccs[j] for j in bbox.indices]
            for cc in ccs_of_segment:
                self.highlight(cc, label)
            seg_ccs.append(Cc(self, label, ccs_of_segment[0].union_rects(ccs_of_segment)))
        return seg_ccs

    __call__ = staticmethod(__call__)


class kise_block_extraction(PluginFunction):
    """
    Segments a page into blocks by Kise's method based on the area Voronoi diagram
    as described in

        K. Kise, A. Sato, M. Iwata: *Segmentation of Page Images Using the
        Area Voronoi Diagram.* Computer Vision and Image Understandig 70,
        pp. 370-382, 1998

    The return value is a list of 'CCs' where each 'CC' represents a
    found segment. Note that the input image is changed such that each
    pixel is set to its segment label.

    The algorithm first builds a CC neighborhood graph and then removes edges from
    this graph based upon the area ratio and distance between adjacent segments.
    The criterion is

        *d/Td1* <= 1  OR  *d/Td2* + *A/Ta* <= 1

    where *Td1* < *Td2* are the two largest peaks in the CC distance distribution and
    A is the area ratio of the adjacent CCs.

    Arguments:

    *Ta*:
      Area ratio in the criterion above.

    *fr*:
      Fraction for determining Td2. It is not chosen as the peak position, but
      larger at the position where the peak has fallen to a fraction *fr* of its
      height.
    """
    self_type = ImageType([ONEBIT])
    return_type = ImageList("ccs")
    args = Args([Float('Ta', default = 40.0), Float('fr', default = 0.34)])
    pure_python = True
    author = "Christoph Dalitz"

    def __call__(self, Ta=40.0, fr=0.34):
        # compute neighborship graph
        from gamera.plugins.geometry import delaunay_from_points
        ccs = self.cc_analysis()
        i = 0
        points = []
        labels = []
        labels2ccs = {}
        for cc in ccs:
            p = cc.contour_samplepoints(15,1)
            cc.points = p
            points += p
            labels += [cc.label] * len(p)
            labels2ccs[cc.label] = cc
        neighbors = delaunay_from_points(points, range(len(points)))

        # compute edge properties
        class Edge(object):
            def __init__(self,cc1,cc2,d2):
                self.cc1 = cc1; self.cc2 = cc2
                self.d = d2
                a = [cc1.black_area()[0], cc2.black_area()[0]]
                self.ar = max(a)/min(a)
        labelneighbors = {}
        for pair in neighbors:
            if (labels[pair[0]] < labels[pair[1]]):
                label1 = labels[pair[0]]; label2 = labels[pair[1]]
            else:
                label1 = labels[pair[1]]; label2 = labels[pair[0]]
            if label1 == label2:
                continue
            p1 = points[pair[0]]
            p2 = points[pair[1]]
            d2 = (p1.x-p2.x)**2 + (p1.y-p2.y)**2
            key = "%i;%i" % (label1,label2)
            if not labelneighbors.has_key(key):
                labelneighbors[key] = Edge(labels2ccs[label1], labels2ccs[label2], d2)
            else:
                e = labelneighbors[key]
                if e.d > d2:
                    e.d = d2
        for e in labelneighbors.itervalues():
            e.d = sqrt(e.d)

        # determine thresholds Td1 and Td2 from distance statistics
        distances = [e.d for e in labelneighbors.itervalues()]
        distances.sort()
        if len(distances) > 50:
            distances = distances[len(distances)/20:(len(distances)-len(distances)/20)]
        x = [float(i*max(distances))/512.0 for i in range(512)]
        density = kernel_density(distances, x, kernel=2)
        local_maxima_i = []
        local_maxima_d = []
        for i in range(1,len(density)-1):
            if density[i] > density[i-1] and density[i] > density[i+1]:
                local_maxima_i.append(i)
                local_maxima_d.append(density[i])
        m1 = argmax(local_maxima_d)
        i1 = local_maxima_i[m1]
        local_maxima_i = [local_maxima_i[i] for i in range(len(local_maxima_i)) if i != m1]
        local_maxima_d = [local_maxima_d[i] for i in range(len(local_maxima_d)) if i != m1]
        i2 = local_maxima_i[argmax(local_maxima_d)]
        if i2 < i1:
            tmp = i2
            i2 = i1
            i1 = tmp
        dmax = density[i2]
        i2 += 1
        while i2 < len(x) - 1:
            if density[i2] < fr*dmax:
                break
            i2 += 1
        Td1 = x[i1]
        Td2 = x[i2]
        #print "Td1 =", Td1, "Td2 =", Td2, "(plugin)"

        # build graph
        from gamera import graph
        g = graph.Graph(graph.UNDIRECTED)
        rgb = self.to_rgb()
        for e in labelneighbors.itervalues():
            if not g.has_node(e.cc1.label):
                g.add_node(e.cc1.label)
            if not g.has_node(e.cc2.label):
                g.add_node(e.cc2.label)
            if (e.d/Td1 <= 1.0) or (e.d/Td2 + e.ar/Ta <= 1):
                g.add_edge(e.cc1.label, e.cc2.label)
            else:
                pass
        
        # split graph into connected subgraphs
        from gamera.core import MlCc
        seglabels = []
        for i,sg in enumerate(g.get_subgraph_roots()):
            seg = [n() for n in g.BFS(sg)]
            seglabels.append(seg)
        segments = []
        for lbs in seglabels:
            mlcc = MlCc([labels2ccs[lb] for lb in lbs])
            segments.append(mlcc.convert_to_cc())

        return segments

    __call__ = staticmethod(__call__)


class sub_cc_analysis(PluginFunction):
    """
    Further subsegments the result of a page segmentation algorithm into
    groups of actual connected components.

    The result of a page segmentation plugin is a list of 'CCs' where
    each 'CC' does not represent a 'connected component', but a page
    segment (typically a line of text). In a practical OCR application
    you will however need the actual connected components (which
    should roughly correspond to the glyphs) in groups of lines. That
    is what this plugin is meant for.

    The input image must be an image that has been processed with a
    page segmentation plugin, i.e. all pixels in the image must be
    labeled with a segment label. The input parameter *cclist* is the
    list of segments returned by the page segmentation algorithm.

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
    by T.M. Breuel (Symposium on Document Image Understanding,
    USA, pp. 209-218, 2003),
    an additional constraint is made for the first criterion by demanding
    that no other segment may be between *a* and *b* that overlaps
    horizontally with both. This constraint for taking multi column
    headings that interrupt columns into account is replaced in this
    implementation with an a priori sort of all textlines by *y*-position.
    This results in a preference of rows over columns (in case of ambiguity)
    in the depth-first-search utilized in the topological sorting.

    .. __: http://iupr1.cs.uni-kl.de/~shared/publications/2003-breuel-sdiut-high-performance-doc-layout-analysis.pdf

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


class segmentation_error(PluginFunction):
    """Compares a ground truth segmentation *Gseg* with a segmentation *Sseg*
and returns error count numbers.

The input images must be given in such a way that each segment is
uniquely labeled, similar to the output of a page segmentation
algorithm like `runlength_smearing`_. For ground truth data, such a labeled
image can be obtained from an external color image with `colors_to_labels`_.

.. _`runlength_smearing`: #runlength-smearing
.. _`colors_to_labels`: color.html#colors-to-labels

The two segmentations are compared by building equivalence classes of
overlapping segments as described in

  M. Thulke, V. Margner, A. Dengel:
  *A general approach to quality evaluation of document
  segmentation results.*
  Lecture Notes in Computer Science 1655, pp. 43-57 (1999)

Each class is assigned an error type depending on how many ground truth
and test segments it contains. The return value is a tuple
(*n1,n2,n3,n4,n5,n6)* where each value is the total number of classes
with the corresponding error type:

+------+-----------------------+---------------+----------------------+
| Nr   | Ground truth segments | Test segments | Error type           |
+======+=======================+===============+======================+
| *n1* | 1                     | 1             | correct              |
+------+-----------------------+---------------+----------------------+
| *n2* | 1                     | 0             | missed segment       |
+------+-----------------------+---------------+----------------------+
| *n3* | 0                     | 1             | false positive       |
+------+-----------------------+---------------+----------------------+
| *n4* | 1                     | > 1           | split                |
+------+-----------------------+---------------+----------------------+
| *n5* | > 1                   | 1             | merge                |
+------+-----------------------+---------------+----------------------+
| *n6* | > 1                   | > 1           | splits and merges    |
+------+-----------------------+---------------+----------------------+

The total segmentation error can be computed from these numbers as
*1 - n1 / (n1 + n2 + n3 + n4 + n5 + n6)*. The individual numbers can
be of use to determine what exactly is wrong with the segmentation.

As this function is not an image method, but a free function, it
is not automatically imported with all plugins and you must import
it explicitly with

.. code:: Python

      from gamera.plugins.pagesegmentation import segmentation_error
"""
    self_type = None
    args = Args([ImageType([ONEBIT], 'Gseg'), \
                 ImageType([ONEBIT], 'Sseg')])
    return_type = IntVector("errors", length=6)
    author = "Christoph Dalitz"


# module declaration
class PageSegmentationModule(PluginModule):
    cpp_headers = ["pagesegmentation.hpp"]
    cpp_namespace = ["Gamera"]
    category = "PageSegmentation"
    functions = [projection_cutting, runlength_smearing, bbox_merging, \
                     kise_block_extraction, sub_cc_analysis, textline_reading_order, \
                     segmentation_error]
module = PageSegmentationModule() # create an instance of the module

# free function instances
textline_reading_order = textline_reading_order()
segmentation_error = segmentation_error()
