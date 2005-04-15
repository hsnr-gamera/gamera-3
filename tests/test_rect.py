from gamera.core import *
from math import sqrt

def test_rect():
   r1 = Rect(25, 25, 40, 50)
   assert r1.ul == Point(25, 25)
   assert r1.ul_x == 25
   assert r1.ul_y == 25
   assert r1.lr == Point(74, 64)
   assert r1.lr_x == 74
   assert r1.lr_y == 64
   assert r1.size == Size(49, 39)
   assert r1.dimensions == Dimensions(40, 50)
   assert r1.nrows == 40
   assert r1.ncols == 50
   assert r1.height == 39
   assert r1.width == 49
   assert r1.center == Point(49, 44)
   assert r1.center_x == 49
   assert r1.center_y == 44

   assert r1.contains_x(40)
   assert r1.contains_y(40)
   assert not r1.contains_x(24)
   assert not r1.contains_y(24)
   assert r1.contains_point(Point(74, 64))
   assert not r1.contains_point(Point(75, 65))

   assert Rect(0, 0, 100, 100).contains_rect(r1)
   assert r1.contains_rect(Rect(30, 30, 30, 40))
   assert not r1.contains_rect(Rect(20, 30, 60, 40))

   assert Rect(0, 0, 100, 100).intersects(r1)
   assert r1.intersects(Rect(30, 30, 30, 40))
   assert r1.intersects(Rect(20, 30, 60, 40))
   assert not r1.intersects(Rect(0, 0, 20, 20))

   assert r1.union_rects([r1, Rect(0, 0, 20, 20)]) == Rect(0, 0, 65, 75)

   distance_euclid = r1.distance_euclid(Rect(0, 0, 25, 25))
   assert distance_euclid > 48.9 and distance_euclid < 49.0

   assert r1.distance_bb(Rect(0, 0, 20, 20)) == sqrt(6*6 + 6*6)
   assert r1.distance_cx(Rect(0, 0, 20, 20)) == 40
   assert r1.distance_cy(Rect(0, 0, 20, 20)) == 35
