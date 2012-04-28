#
# Copyright (C) 2009-2012 Christoph Dalitz
#               2010      Oliver Christen, Tobias Bolten
#               2011      Christian Brandt
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
from gamera.args import NoneDefault
import _geometry
try:
  from gamera.core import RGBPixel
except:
  def RGBPixel(*args):
    pass


class voronoi_from_labeled_image(PluginFunction):
  u"""
  Computes the area Voronoi tesselation from an image containing labeled
  Cc's. In the returned onebit image, every pixel is labeled with the
  label value of the closest Cc in the input image.

  To prepare the input image, you can use cc_analysis__. When the Cc's
  only consist of single points, the area Voronoi tesselation is identical
  to the ordinary Voronoi tesselation.

  .. __: segmentation.html#cc-analysis

  The implementation applies a watershed algorithm to the distance transform
  of the input image, a method known as *seeded region growing* (U. K\u00f6the:
  *Primary Image Segmentation.* Proceedings 17th DAGM-Symposium, pp. 554-561,
  Springer, 1995).

  The example shown below is the image *voronoi_cells* as created with
  the the following code:

  .. code:: Python

    # create an area Voronoi tesselation and
    # mark the cells and their edges in color
    ccs = image.cc_analysis()  # labels the image
    voronoi = image.voronoi_from_labeled_image()
    voronoi_cells = voronoi.color_ccs()
    voronoi_cells.highlight(image, RGBPixel(0,0,0))
    voronoi_edges = voronoi.labeled_region_edges()
    voronoi_cells.highlight(voronoi_edges, RGBPixel(255,255,255))
  """
  self_type = ImageType([ONEBIT,GREYSCALE])
  return_type = ImageType([ONEBIT,GREYSCALE])
  def __doc_example1__(images):
    image = images[ONEBIT]
    ccs = image.cc_analysis()
    voronoi = image.voronoi_from_labeled_image()
    voronoi_cells = voronoi.color_ccs()
    voronoi_cells.highlight(image, RGBPixel(0,0,0))
    voronoi_edges = voronoi.labeled_region_edges()
    voronoi_cells.highlight(voronoi_edges, RGBPixel(255,255,255))
    return [image, voronoi_cells]
  doc_examples = [__doc_example1__]
  author = u"Christoph Dalitz, based on code by Ullrich K\u00f6the"

class voronoi_from_points(PluginFunction):
  """
  Computes the Voronoi tesselation from a list of points and point labels.
  The result is directly written to the input image. Each white pixel is
  labeled with the label value of the closest point. Non white pixel in the
  input image are not overwritten.

  The arguments *points* and *labels* specify the points and labels, such
  that ``labels[i]`` is the label of ``points[i]``. Note that the labels
  need not necessarily all be different, which can be useful as an 
  approximation of an area Voronoi tesselation.

  The algorithm is very simple: it stores the points in a `kd-tree`_ and
  then looks up the closest point for each image pixel. This has a runtime
  of *O(N log(n))*, where *N* is the number of image pixels and *n* is the
  number of points. For not too many points, this should be faster than the
  morphological region growing approach of `voronoi_from_labeled_image`_.

.. _`kd-tree`: kdtree.html
.. _`voronoi_from_labeled_image`: #voronoi-from-labeled-image

  The example shown below is the image *voronoi_edges* as created with
  the the following code:

  .. code:: Python

    # create a Voronoi tesselation and mark
    # the cell edges in a second image
    points = [(10,10),(20,30),(32,22),(85,14),(40,70),(80,85)]
    voronoi = Image((0,0),(90,90))
    voronoi.voronoi_from_points(points,[i+2 for i in range(len(points))])
    voronoi_edges = voronoi.labeled_region_edges()
    for p in points:
       voronoi_edges.set(p,1)
  """
  self_type = ImageType([ONEBIT,GREYSCALE])
  args = Args([PointVector("points"),IntVector("labels")])
  return_type = None
  def __doc_example1__(images):
    from gamera.core import Image
    points = [(10,10),(20,30),(32,22),(85,14),(40,70),(80,85)]
    voronoi = Image((0,0),(90,90))
    voronoi.voronoi_from_points(points,[i+2 for i in range(len(points))])
    voronoi_edges = voronoi.labeled_region_edges()
    for p in points:
      voronoi_edges.set(p,1)
    return [voronoi_edges]
  doc_examples = [__doc_example1__]
  author = "Christoph Dalitz"

class labeled_region_neighbors(PluginFunction):
  """
  For an image containing labeled regions, a list of all label pairs belonging
  to touching regions is returned. When *eight_connectivity* is ``True``
  (default), 8-connectivity is used for neighborship, otherwise
  4-connectivity is used.

  This can be useful for building a Delaunay graph from a Voronoi tesselation
  as in the following example:

  .. code:: Python

    #
    # build Delaunay graph of neighboring (i.e. adjacent) Cc's
    #

    # create map label->Cc for faster lookup later
    ccs = image.cc_analysis()
    label_to_cc = {}
    for c in ccs:
       label_to_cc[c.label] = c

    # compute area Voronoi tesselation and neighborship list
    voronoi = image.voronoi_from_labeled_image()
    labelpairs = voronoi.labeled_region_neighbors()

    # build map of all neighbors for each label for fast lookup
    neighbors = {}
    for label in label_to_cc.keys():
       neighbors[label] = []
    for n in labelpairs:
       neighbors[n[0]].append(n[1])
       neighbors[n[1]].append(n[0])

    # now, all neighbors to a given cc can be looked up with
    neighbors_of_cc = [label_to_cc[label] for label in neighbors[cc.label]]

  """
  self_type = ImageType([ONEBIT])
  args = Args([Check("eight_connectivity",default=True)])
  return_type = Class("labelpairs")
  author = "Christoph Dalitz"
  # wrapper for passing default argument
  def __call__(self, eight_connectivity=True):
      return _geometry.labeled_region_neighbors(self, eight_connectivity)
  __call__ = staticmethod(__call__)

class delaunay_from_points(PluginFunction):
  """
  Computes the Delaunay triangulation directly from a list of points and
  point labels. The result is a list which contains tuples of adjacent labels,
  where in each tuple the smaller label is given first.

  The arguments *points* and *labels* specify the points and nonnegative
  labels, such that ``labels[i]`` is the label of ``points[i]``. Note that
  the labels need not necessarily all be different, which can be useful
  for the computation of a neighborship graph from a set of connected
  components.

  The computation of the Delaunay triangulation is based on the Delaunay
  tree which is a randomized geometric data structure. It is
  described in O. Devillers, S. Meiser, M. Teillaud:
  `Fully dynamic Delaunay triangulation in logarithmic expected time per operation.`__
  Computational Geometry: Theory and Applications 2, pp. 55-80, 1992.

.. __: http://hal.inria.fr/inria-00090678

  This can be useful for building a neighborhood graph as shown in the
  following example:

  .. code:: Python

    from gamera import graph
    from gamera.plugins.geometry import delaunay_from_points
    
    points = [(10,10),(20,30),(32,22),(85,14),(40,70),(80,85)]
    labels = range(len(points))
    neighbors = delaunay_from_points(points, labels)
    
    g = graph.Graph(graph.UNDIRECTED)
    for pair in neighbors:
        g.add_edge(pair[0], pair[1])
  """
  self_type = None
  args = Args([PointVector("points"), IntVector("labels")])
  return_type = Class("labelpairs")
  author = "Oliver Christen (based on code by Olivier Devillers)"


class graph_color_ccs(PluginFunction):
    """
    Returns an RGB Image where each segment is colored with one of the colors
    from *colors* with the constraint that segments adjacent in the 
    neighborship graph have different colors.

    This function can be used to verify that the pagesegmentation 
    e.g. ``runlength_smearing`` is working correctly for your image.

    For coloring the Gamera graph library is used. For more information on the 
    coloring algorithm see Graph.colorize__

    .. __: graph.html#colorize
    
    *ccs*:
        ImageList which contains ccs to be colored. Must be views on
        the image on which this method is called.

    *colors*:
        list of colors (instances of RGBPixel) which will be used for coloring.
        When ``None``, the default set of seven colors given in the example
        below is used.

    *method*:
        Controls the calculation of the neighborhood graph:

            0 = from the CC center points
            (fastest, but can be inaccurate for large CC's)

            1 = from a 20 percent sample of the contour points
            (reasonable compromise between speed and accuracy)

            2 = from the exact area Voronoi diagram
            (can be slow on large images)

    .. code:: Python

       ccs = imgage.cc_analysis()
       colors = [ RGBPixel(150, 0, 0),
                  RGBPixel(0, 250, 0),
                  RGBPixel(0, 0, 255),
                  RGBPixel(250, 0, 255),
                  RGBPixel(50, 150, 50),
                  RGBPixel(0, 190, 255),
                  RGBPixel(230, 190, 20) ]
       rgb = imgage.mycolor_ccs(ccs, colors, 1)

    .. note:: *colors* may not contain less than six colors.

    """
    category = "Color"
    author = "Oliver Christen and Tobias Bolten"
    args = Args([ImageList('ccs'), Class('colors', klass=RGBPixel, list_of=True,default=NoneDefault), Choice('method', ["CC center", "20% contour points", "voronoi diagram"], default=1)])
    self_type = ImageType([ONEBIT])
    return_type = ImageType([RGB])

    def __call__(image, ccs, colors=None, method=1):
      if colors == None:
        from gamera.core import RGBPixel
        colors = [ RGBPixel(150, 0, 0),
                   RGBPixel(0, 250, 0),
                   RGBPixel(0, 0, 255),
                   RGBPixel(250, 0, 255),
                   RGBPixel(50, 150, 50),
                   #RGBPixel(120, 120, 120),
                   #RGBPixel(250, 250, 0),
                   RGBPixel(0, 190, 255),
                   RGBPixel(230, 190, 20),
                   ]
      return _geometry.graph_color_ccs(image, ccs, colors, method)
    __call__ = staticmethod(__call__)


class convex_hull_from_points(PluginFunction):
    """Returns the polygon vertices of the convex hull of the given list of
points.

The function uses Graham's scan algorithm as described e.g. in Cormen et al.:
*Introduction to Algorithms.* 2nd ed., MIT Press, p. 949, 2001
"""
    self_type = None
    args = Args([PointVector("points")])
    return_type = PointVector("convexhull")
    author = "Christian Brandt and Christoph Dalitz"


class convex_hull_as_points(PluginFunction):
   """Returns the vertex points of the convex hull of all black pixels
in the given image.

Actually not all black pixels are required for computing the convex hull,
but only the left and right contour pixels of the image. This follows
from the fact that, when a point is invisible both from the left and the
right, it lies on the connection axis between two visible points and thus
cannot be a vertex point of the convex hull.
   """
   self_type = ImageType([ONEBIT])
   args = Args([])
   return_type = PointVector("convexhull")
   author = "Christoph Dalitz"


class convex_hull_as_image(PluginFunction):
   """Returns an image containing the polygon of the convex hull calculated
by convex_hull_as_points_.

.. _convex_hull_as_points: geometry.html#convex_hull_as_points
   """
   self_type = ImageType([ONEBIT])
   args = Args([Check("filled",default=False)])
   return_type = ImageType([ONEBIT])
   author = "Christoph Dalitz"
   doc_examples = [(ONEBIT,)]

   def __call__(image, filled=False):
       return _geometry.convex_hull_as_image(image, filled)
   __call__ = staticmethod(__call__)


class max_empty_rect(PluginFunction):
   """Returns the maximum area empty rect that fits into the image without
containing any of the black image pixels. This problem is in the literature
generally known as the *Largest Empty Rectangle Problem*.

Reference: D. Vandevoorde: `\"The Maximal Rectangle Problem.\"`__ Dr. Dobb's,
April 1998.

.. __: http://www.drdobbs.com/database/184410529
   """
   self_type = ImageType([ONEBIT])
   args = Args([])
   return_type = Rect("max_epmty_rect")
   author = "Deveed Vandevoorde and Christoph Dalitz"
   def __doc_example1__(images):
       from gamera.core import Image
       from gamera.core import Point as P
       img = Image((0,0),(90,90))
       points = [P(10,10),P(20,30),P(32,22),P(85,14),P(40,70),P(80,85)]
       for p in points:
           img.draw_filled_rect((p.x-2,p.y-2),(p.x+1,p.y+1),1)
       r = img.max_empty_rect()
       rgb = img.to_rgb()
       rgb.draw_hollow_rect(r,RGBPixel(255,0,0))
       rgb.highlight(img,RGBPixel(0,0,0))
       return [rgb]
   doc_examples = [__doc_example1__]


class GeometryModule(PluginModule):
  cpp_headers = ["geometry.hpp"]
  category = "Geometry"
  import glob
  cpp_sources=["src/geostructs/kdtree.cpp", "src/geostructs/delaunaytree.cpp"] + glob.glob("src/graph/*.cpp")
  functions = [voronoi_from_labeled_image,
               voronoi_from_points,
               labeled_region_neighbors,
               delaunay_from_points,
               graph_color_ccs,
               convex_hull_from_points,
               convex_hull_as_points,
               convex_hull_as_image,
               max_empty_rect]
  author = "Christoph Dalitz"
  url = "http://gamera.sourceforge.net/"

module = GeometryModule()

delaunay_from_points = delaunay_from_points()
convex_hull_from_points = convex_hull_from_points()
