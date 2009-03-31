from gamera.core import *
from math import sqrt

def test_rect():
   r1 = Rect(Point(25, 25), Point(74, 64))
   assert r1.ul == Point(25, 25)
   assert r1.ul_x == 25
   assert r1.ul_y == 25
   assert r1.lr == Point(74, 64)
   assert r1.lr_x == 74
   assert r1.lr_y == 64
   assert r1.size == Size(49, 39)
   assert r1.dim == Dim(50, 40)
   assert r1.nrows == 40
   assert r1.ncols == 50
   assert r1.height == 39
   assert r1.width == 49
   assert r1.ur == Point(74, 25)
   assert r1.ur_x == 74
   assert r1.ur_y == 25
   assert r1.ll == Point(25, 64)
   assert r1.ll_x == 25
   assert r1.ll_y == 64

   assert r1.center == Point(49, 44)
   assert r1.center_x == 49
   assert r1.center_y == 44

   assert r1.contains_x(40)
   assert r1.contains_y(40)
   assert not r1.contains_x(24)
   assert not r1.contains_y(24)
   assert r1.contains_point(Point(74, 64))
   assert not r1.contains_point(Point(75, 65))

   assert Rect(Point(0,0), Point(100,100)).contains_rect(r1)
   assert r1.contains_rect(Rect(Point(30,30), Point(30,40)))
   assert not r1.contains_rect(Rect(Point(20,30), Point(60,40)))

   assert Rect(Point(0,0), Point(100,100)).intersects(r1)
   assert r1.intersects(Rect(Point(30,30), Point(60,70)))
   assert r1.intersects(Rect(Point(20,30), Point(60,40)))
   assert not r1.intersects(Rect(Point(0,0), Point(20,20)))

   assert r1.union_rects([r1, Rect(Point(0,0), Point(20,20))]) == Rect(Point(0,0), Point(74,64))

   distance_euclid = r1.distance_euclid(Rect(Point(0,0), Point(25,25)))
   assert distance_euclid > 48.9 and distance_euclid < 49.0

   assert r1.distance_bb(Rect(Point(0,0), Point(20,20))) == sqrt(5*5 + 5*5)
   assert r1.distance_cx(Rect(Point(0,0), Point(20,20))) == 39
   assert r1.distance_cy(Rect(Point(0,0), Point(20,20))) == 34
