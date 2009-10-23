from gamera.core import *
init_gamera()

def test_save_image():
   def _test_save_image(name, storage = DENSE):
      for ext in ['png', 'tiff']:
         image = load_image("data/%s_generic.tiff" % name, storage)
         image.save_image("tmp/%s_test.%s" % (name, ext))
         image2 = load_image("tmp/%s_test.%s" % (name, ext))
         assert image._to_raw_string() == image2._to_raw_string()

   # TODO: Add Grey16
   for type in ["OneBit", "GreyScale", "RGB"]:
      _test_save_image(type)
   _test_save_image("OneBit", RLE)

