import py.test

from gamera.core import *
init_gamera()

#
# Tests for nearest neighbor finding with kd-trees
#

from gamera.kdtree import *

#
# input parameter check
#
def test_wrongparams():
    def _kdnode_input(coord):
        node = KdNode(coord)
    def _kdtree_input(nodes):
        tree = KdTree(nodes)
    # all point coordinates must be numeric
    py.test.raises(Exception, _kdnode_input, [1,2,"a"])
    py.test.raises(Exception, _kdnode_input, [])
    # all nodes must be KdNode's and of same dimension
    py.test.raises(Exception, _kdtree_input, [KdNode([1,2]),KdNode([2,3,4])])
    py.test.raises(Exception, _kdtree_input, [KdNode([1,2]),[2,3]])
    py.test.raises(Exception, _kdtree_input, None)
    py.test.raises(Exception, _kdtree_input, [])


#
# k nearest neighbor searches
#
def test_nearest_neighbors():
    points = [(1,1), (2,1), (1,3), (2,4), (3,4),
              (7,2), (8,3), (8,4), (7,5), (7,5)]
    nodes = [KdNode(p) for p in points]
    tree = KdTree(nodes)
    assert [7,2] == tree.k_nearest_neighbors([5.5,3],1)[0].point
    assert [[2,4], [3,4]] == \
        [n.point for n in tree.k_nearest_neighbors([2,4],2)]
    assert [[7,5],[7,5],[8,4]] == \
        [n.point for n in tree.k_nearest_neighbors([7,5],3)]
    assert [[8,4],[8,3],[7,5]] == \
        [n.point for n in tree.k_nearest_neighbors([8,4],3)]
    assert [[3,4],[2,4],[1,3],[2,1],[1,1],[7,5]] == \
        [n.point for n in tree.k_nearest_neighbors([3,4],6)]
