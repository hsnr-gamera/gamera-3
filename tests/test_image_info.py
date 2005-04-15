from gamera.core import *

from gamera.core import *
init_gamera()

def test_image_info():
   def _test_image_info(name, *args):
      for ext in ['png', 'tiff']:
         print "data/%s_generic.%s" % (name, ext)
         info = image_info("data/%s_generic.%s" % (name, ext))
         assert (info.depth, info.ncolors, info.ncols, info.nrows, info.x_resolution, info.y_resolution) == args

   # TODO: Add Grey16
   _test_image_info("OneBit", 1, 1, 70, 100, 0.0, 0.0)
   _test_image_info("RGB", 8, 3, 228, 130, 0.0, 0.0)
   _test_image_info("GreyScale", 8, 1, 97, 68, 0.0, 0.0)
