from gamera.core import *

from gamera.core import *
init_gamera()

def test_nested_list():
   def _test_nested_list(name, type, storage = DENSE):
      image = load_image("data/%s_generic.tiff" % name, storage)
      nested = image.to_nested_list()
      image2 = nested_list_to_image(nested, type)
      assert image._to_raw_string() == image2._to_raw_string()

   # TODO: Add Grey16
   for name, type in [("OneBit", ONEBIT), ("GreyScale", GREYSCALE), ("RGB", RGB)]:
      _test_nested_list(name, type)
   _test_nested_list("OneBit", ONEBIT, RLE)
