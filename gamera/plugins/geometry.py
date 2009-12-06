#
#
# Copyright (C) 2009 Christoph Dalitz
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
import _geometry


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
  *Primary Image Segmentation.* Proceedings 17th DAGM-Symposium, Springer,
  1995).

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
    from gamera.core import RGBPixel
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


class GeometryModule(PluginModule):
  cpp_headers = ["geometry.hpp"]
  category = "Geometry"
  cpp_sources=["src/kdtree/kdtree.cpp"]
  functions = [voronoi_from_labeled_image,
               voronoi_from_points,
               labeled_region_neighbors]
  author = "Christoph Dalitz"
  url = "http://gamera.sourceforge.net/"

module = GeometryModule()

