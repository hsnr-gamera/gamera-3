import py.test

from gamera.core import *
init_gamera()


def test_load_image_rgb_rle():
   def _load_image_fail1():
      image = load_image("not_a_path")
   py.test.raises(IOError, _load_image_fail1)

def test_load_image_onebit():
   image = load_image("data/testline.tiff")
   assert image.pixel_type_name == "OneBit"
   assert image.nrows == 44
   assert image.ncols == 907
   assert image.black_area()[0] == 5174.0
   assert image.storage_format_name == "Dense"

   image2 = load_image("data/testline.png")
   assert image2.pixel_type_name == "OneBit"
   assert image2.nrows == 44
   assert image2.ncols == 907
   assert image2.black_area()[0] == 5174.0
   assert image.storage_format_name == "Dense"

   assert image.to_string() == image2.to_string()
   assert image.to_rle() == image2.to_rle()

def test_load_image_greyscale():
   greyscale = load_image("data/GreyScale_generic.tiff")
   assert greyscale.pixel_type_name == "GreyScale"
   assert greyscale.nrows == 68
   assert greyscale.ncols == 97
   assert greyscale.storage_format_name == "Dense"

   greyscale2 = load_image("data/GreyScale_generic.png")
   assert greyscale2.pixel_type_name == "GreyScale"
   assert greyscale2.nrows == 68
   assert greyscale2.ncols == 97
   assert greyscale2.storage_format_name == "Dense"

   assert greyscale.to_string() == greyscale2.to_string()

def test_load_image_greyscale_rle():
   def _load_image_greyscale_rle1():
      greyscale3 = load_image("data/GreyScale_generic.png", RLE)
   def _load_image_greyscale_rle2():
      greyscale3 = load_image("data/GreyScale_generic.tiff", RLE)
   py.test.raises(Exception, _load_image_greyscale_rle1)
   py.test.raises(Exception, _load_image_greyscale_rle2)

def test_load_image_rgb():
   rgb = load_image("data/RGB_generic.tiff")
   assert rgb.pixel_type_name == "RGB"
   assert rgb.nrows == 130
   assert rgb.ncols == 228
   assert rgb.storage_format_name == "Dense"

   rgb2 = load_image("data/RGB_generic.png")
   assert rgb2.pixel_type_name == "RGB"
   assert rgb2.nrows == 130
   assert rgb2.ncols == 228
   assert rgb.storage_format_name == "Dense"

   assert rgb.to_string() == rgb2.to_string()

def load_image_rgb_rle1():
   rgb3 = load_image("data/RGB_generic.png", RLE)

def load_image_rgb_rle2():
   rgb3 = load_image("data/RGB_generic.tiff", RLE)

def test_load_image_grey16():
   grey16 = load_image("data/Grey16_generic.tiff")
   assert grey16.pixel_type_name == "Grey16"
   assert grey16.nrows == 72
   assert grey16.ncols == 83
   assert grey16.storage_format_name == "Dense"

   grey16_2 = load_image("data/Grey16_generic.png")
   assert grey16_2.pixel_type_name == "Grey16"
   assert grey16_2.nrows == 72
   assert grey16_2.ncols == 83
   assert grey16.storage_format_name == "Dense"

   # assert grey16.to_string() == grey16_2.to_string()

def load_image_grey16_rle1():
   grey16 = load_image("data/Grey16_generic.png", RLE)

def load_image_grey16_rle2():
   grey16 = load_image("data/Grey16_generic.tiff", RLE)

# def test_load_image_grey16_rle():
#    py.test.raises(Exception, load_image_grey16_rle1)
#    py.test.raises(Exception, load_image_grey16_rle2)

