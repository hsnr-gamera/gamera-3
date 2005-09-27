from gamera.gameracore import FloatPoint as FP
from math import sqrt

def test_floatpoint():
   p = FP(2, 3)
   assert p.x == 2.0 and p.y == 3.0

   assert FP(2, 2) + FP(1, 2) == FP(3, 4)
   assert FP(2, 2) * FP(2, 3) == FP(4, 6)
   assert FP(2, 2) - FP(2, 4) == FP(0, -2)
   assert FP(2, 2) / FP(2, 3) == FP(1, 2.0/3.0)

   assert FP(0, 0).distance(FP(-3, -3)) == sqrt(18)

   assert repr(FP(2, 3)) == "FloatPoint(2, 3)"

def test_floatpoint_leak():
   for i in xrange(1000):
      p = FP(i, i)
