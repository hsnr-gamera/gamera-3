import py.test

from gamera.core import *
init_gamera()

def make_test(inner):
   def _generate_rgb():
      rng = range(0, 255, 8)
      for r in rng:
         for g in rng:
            for b in rng:
               yield RGBPixel(r,g,b)

   def test():
      for type, value in [(ONEBIT, xrange((2 ** 16) - 1)),
                          (GREYSCALE, xrange((2 ** 8) - 1)),
                          (FLOAT, [float(x) for x in xrange(-5, 5)]),
                          (COMPLEX, [complex(x) for x in xrange(-5, 5)]),
                          (RGB, _generate_rgb()),
                          (GREY16, xrange((2 ** 16) - 1))]:
         inner(type, value, DENSE)
      inner(ONEBIT, xrange(0, 2 ** 16 - 1), RLE)
   return test

def _test_image_constructors(type, value, storage):
   def _fail1():
      Image(Size(50, 50), Point(25, 25), type, storage)
   #image = Image(25, 25, 50, 50, type, storage) # deprecated call
   image = Image(Point(25,25), Dim(50,50), type, storage) 
   assert image.data.storage_format == storage
   assert image.data.pixel_type == type
   assert image.ul == Point(25, 25)
   assert image.size == Size(49, 49)
   #assert image.dimensions == Dimensions(50, 50) # deprecated call
   assert image.dim == Dim(50, 50)
   image2 = Image(Point(25, 25), Point(74, 74), type, storage)
   assert Rect(image) == Rect(image2)
   image3 = Image(Point(25, 25), Size(49, 49), type, storage)
   assert Rect(image) == Rect(image3)
   #image4 = Image(Point(25, 25), Dimensions(50, 50), type, storage) # deprecated call
   image4 = Image(Point(25, 25), Dim(50, 50), type, storage)
   assert Rect(image) == Rect(image4)
   #image5 = Image(Rect(25, 25, 50, 50), type, storage) # deprecated call
   image5 = Image(Rect(Point(25,25), Point(74,74)), type, storage)
   assert Rect(image) == Rect(image5)
   image6 = Image(Point(25, 25), Dim(50, 50), type, storage)
   assert Rect(image) == Rect(image6)
   py.test.raises(TypeError, _fail1)

test_image_constructors = make_test(_test_image_constructors)

def _test_subimage(type, value, storage):
   def _fail1():
      image.subimage(Point(20,20), Dim(10,10))
   def _fail2():
      image.subimage(Point(30,30), Dim(50,50))
   #image = Image(25, 25, 50, 50, type, storage) # deprecated call
   image = Image(Point(25,25), Dim(50,50), type, storage)
   #image2 = image.subimage(30, 30, 40, 40) # deprecated call
   image2 = image.subimage(Point(30,30), Dim(40,40))
   assert image2.ul == Point(30, 30)
   assert image2.data == image.data
   image3 = image.subimage(Point(30, 30), Point(69, 69))
   assert Rect(image2) == Rect(image3)
   assert image3.data == image.data
   image4 = image.subimage(Point(30, 30), Size(39, 39))
   assert Rect(image4) == Rect(image2)
   assert image4.data == image.data
   #image5 = image.subimage(Point(30, 30), Dimensions(40, 40)) # deprecated call
   image5 = image.subimage(Point(30, 30), Point(69, 69))
   assert Rect(image5) == Rect(image2)
   assert image5.data == image.data
   image6 = image.subimage(Rect(Point(30,30), Dim(40,40)))
   assert Rect(image6) == Rect(image2)
   assert image6.data == image.data
   image7 = image.subimage(Point(30, 30), Dim(40, 40))
   assert Rect(image7) == Rect(image2)
   assert image7.data == image.data
   py.test.raises(RuntimeError, _fail1)
   py.test.raises(RuntimeError, _fail2)
   #image7 = image2.subimage(35, 35, 20, 20) # deprecated call
   image7 = image2.subimage(Point(35,35), Dim(20,20))
   assert image7.data == image.data
   #image8 = image7.subimage(32, 32, 25, 25) # deprecated call
   image8 = image7.subimage(Point(32,32), Dim(25,25))
   assert image8.ul == Point(32, 32)
   assert image8.data == image.data
test_subimage = make_test(_test_subimage)

def _test_get_set(type, value, storage):
   def _fail1():
      image.set(Point(50, 50), 0)
   def _fail2():
      image.get(Point(50, 50))
   image = Image((25, 25), Dim(50, 50), type, storage)
   for val in value:
      image.set(Point(25, 25), val)
      assert image.get(Point(25, 25)) == val
   for r in range(50):
      for c in range(50):
         image.set((r, c), val)
         assert image.get((r, c)) == val
   py.test.raises(IndexError, _fail1)
   py.test.raises(IndexError, _fail2)
test_get_set = make_test(_test_get_set)
   
def _test_index(type, value, storage):
   def _fail1():
      image[50, 50] = 0
   def _fail2():
      image[50, 50]
   image = Image((25, 25), Dim(50, 50), type, storage)
   for val in value:
      image[25, 25] = val
      assert image[25, 25] == val
   for r in range(50):
      for c in range(50):
         image[r, c] = val
         assert image[r, c] == val
   print len(image)
   for i, p in enumerate(image):
      if p != val:
         print i
      assert p == val
   py.test.raises(IndexError, _fail1)
   py.test.raises(IndexError, _fail2)
test_index = make_test(_test_index)

