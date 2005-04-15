from gamera.core import *
init_gamera()

def test_rle1():
   image1 = load_image("data/testline.png")
   image2 = load_image("data/testline.png", RLE)

   # Check basic RLE image loading
   assert image2.pixel_type_name == "OneBit"
   assert image2.nrows == 44
   assert image2.ncols == 907
   assert image2.black_area()[0] == 5174.0

   # Compare RLE to DENSE image
   assert image1._to_raw_string() == image2._to_raw_string()
   assert image1.to_rle() == image2.to_rle()

   # Run something complicated, like cc_analysis
   image1.cc_analysis()
   image2.cc_analysis()

   # compare the results
   assert image1.color_ccs().to_string() == image2.color_ccs().to_string()
   assert image1._to_raw_string() == image2._to_raw_string()

   # Do some more complicated stuff, particularly things that
   # use iterators in unusual ways
   assert image1.projection_rows() == image2.projection_rows()
   assert image1.projection_cols() == image2.projection_cols()
   assert image1.most_frequent_black_vertical_run() == image2.most_frequent_black_vertical_run()
   assert image1.most_frequent_black_horizontal_run() == image2.most_frequent_black_horizontal_run()

   
