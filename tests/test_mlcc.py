from gamera.core import *
init_gamera()

def test_mlcc():
   # create test image with different labels
   img=Image((0,0),(8,8))
   img.draw_filled_rect((1,1),(3,3),2)
   img.set((1,5),3)
   img.set((5,1),4)
   img.set((5,5),5)

   # test onebit plugins (highlight, features)
   rgb = img.to_rgb()
   mlcc1 = MlCc(img, 2, Rect((0,0),(8,8)))
   assert round(mlcc1.black_area()[0]) ==  9.0
   rgb.highlight(mlcc1, RGBPixel(255,0,0))
   assert RGBPixel(0,0,0) == rgb.get((5,5))
   mlcc1.add_label(5, Rect((0,0),(8,8)))
   assert round(mlcc1.black_area()[0]) == 10.0
   assert True == mlcc1.has_label(2)
   assert False == mlcc1.has_label(4)
   assert True == mlcc1.has_label(5)
   rgb.highlight(mlcc1,RGBPixel(0,255,0))
   assert RGBPixel(0,255,0) == rgb.get((5,5))

   # test conversion to Cc
   labels = mlcc1.get_labels()
   labels.sort()
   assert [2,5] == labels
   cc = mlcc1.convert_to_cc()
   assert 1 == len(mlcc1.get_labels())
   label = mlcc1.get_labels()[0]
   assert label == img.get((5,5))
   assert label == img.get((2,2))

   # test relabeling
   mlcc2 = MlCc(img, 2, Rect((1,1),(3,3)))
   mlcc2.add_label(3, Rect((1,5),(1,5)))
   mlcc2.add_label(4, Rect((5,1),(5,1)))
   mlcc2.add_label(5, Rect((5,5),(5,5)))
   assert [(1,1), (5,5)] == [mlcc2.ul, mlcc2.lr]
   mlcc3 = mlcc2.relabel([2,3])
   labels = mlcc3.get_labels(); labels.sort()
   assert [2,3] == labels
   assert [(1,1), (3,5)] == [mlcc3.ul, mlcc3.lr]
   mlcc3, mlcc4 = mlcc2.relabel([[2,3],[4]])
   labels = mlcc3.get_labels(); labels.sort()
   assert [2,3] == labels
   labels = mlcc4.get_labels(); labels.sort()
   assert [4] == labels
   assert [(1,1), (3,5)] == [mlcc3.ul, mlcc3.lr]
   assert [(5,1), (5,1)] == [mlcc4.ul, mlcc4.lr]
   mlcc2.remove_label(2)
   mlcc2.remove_label(3)
   labels = mlcc2.get_labels(); labels.sort()
   assert [4,5] == labels
   assert [(5,1),(5,5)] == [mlcc2.ul, mlcc2.lr]

   # test get/set
   mlcc2.set((0,1),3)
   assert 3 == img.get((5,2))
   assert 0 == mlcc2.get((0,1))
   mlcc2.set((0,1),5)
   assert 5 == img.get((5,2))
   assert 5 == mlcc2.get((0,1))
   cc = mlcc2.convert_to_cc()
   cc.set((0,2),20)
   assert 20 == img.get((5,3))
   assert 0 == cc.get((0,2))
   cc.set((0,2),cc.label)
   assert cc.label == img.get((5,3))
   assert cc.label == cc.get((0,2))
