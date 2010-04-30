import py.test

from gamera.core import *
init_gamera()

#
# Tests for Voronoi tesselation and Delaunay triangulation
#

from gamera.plugins.geometry import delaunay_from_points

#
# input parameter check
#
def test_wrongparams():
    def _voronoi_from_points_input(points,labels):
        image = Image((0,0),(20,20))
        image.voronoi_from_points(points,labels)
    # number of points and labels must match
    py.test.raises(Exception, _voronoi_from_points_input, [(2,2),(5,5)], [2,3,4])
    # labels must be int's and points must be coordinates
    py.test.raises(Exception, _voronoi_from_points_input, [(2,2),(5,5)], [(2,2),(5,5)])
    # TODO: no plausi check in Gamera's passing of PointVector => crash
    #py.test.raises(Exception, _voronoi_from_points_input, [2,,5], [2,5])

#
# voronoi tesselation by image labeling
#
def test_voronoi_cell_labeling():
    img = Image((0,0),(9,9))
    points = [(2,2),(3,2),(3,8),(6,5)]
    labels = [2,2,3,4]
    for i in range(len(points)):
        img.set(points[i],labels[i])
    # voronoi_from_labeled_image
    voronoi1 = img.voronoi_from_labeled_image()
    assert 2 == voronoi1.get((2,4))
    assert 3 == voronoi1.get((4,8))
    assert 4 == voronoi1.get((5,4))
    labelpairs = voronoi1.labeled_region_neighbors()
    assert [2,4] in labelpairs or [4,2] in labelpairs
    assert [2,3] in labelpairs or [3,2] in labelpairs
    assert [4,3] in labelpairs or [3,4] in labelpairs
    # voronoi_from_points
    voronoi2 = Image(img)
    voronoi2.voronoi_from_points(points,labels)
    assert 2 == voronoi2.get((2,4))
    assert 3 == voronoi2.get((4,8))
    assert 4 == voronoi2.get((5,4))
    assert [2,4] in labelpairs or [4,2] in labelpairs
    assert [2,3] in labelpairs or [3,2] in labelpairs
    assert [4,3] in labelpairs or [3,4] in labelpairs

#
# delaunay triangulation
#
def test_delaunay_triangulation():
    # all labels different
    points = [(50,50),(25,100),(50,150),(150,75),(150,125)]
    edges = delaunay_from_points(points,range(len(points)))
    assert len(edges) == 7
    assert [0,1] in edges or [1,0] in edges
    assert [0,2] in edges or [2,0] in edges
    assert [0,3] in edges or [3,0] in edges
    assert [0,4] in edges or [4,0] in edges
    assert [1,2] in edges or [2,1] in edges
    assert [2,4] in edges or [4,2] in edges
    assert [3,4] in edges or [4,3] in edges
    # some doublettes
    points = [(50,50),(25,100),(50,150),(150,75),(150,125)]
    labels = [0,1,2,3,3]
    edges = delaunay_from_points(points,labels)
    assert len(edges) == 5
    assert [0,1] in edges or [1,0] in edges
    assert [0,2] in edges or [2,0] in edges
    assert [0,3] in edges or [3,0] in edges
    assert [1,2] in edges or [2,1] in edges
    assert [2,3] in edges or [3,2] in edges
    # TODO: test collinear edge resolution
