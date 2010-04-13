#!/usr/bin/env python

from gamera import core, gendoc

if __name__ == "__main__":
   core.init_gamera()
   gendoc.gendoc(classes=[
      ("gamera.core", "ImageInfo",
       'x_resolution y_resolution ncols nrows depth ncolors'),
      ("gamera.core", "ImageData",
       'nrows ncols page_offset_x page_offset_y stride size bytes pixel_type storage_format'),
      ("gamera.core", "RGBPixel",
       'red green blue hue saturation value cie_x cie_y cie_z cie_Lab_L cie_Lab_a cie_Lab_b cyan magenta yellow')
      ])
   
