#!/usr/bin/env python

from gamera.core import *
init_gamera()

def test_colorize():
   img0 = load_image("data/testline.png")
   img0 = img0.to_onebit()
   ccs = img0.cc_analysis()
   for i in range(3):
      color = img0.graph_color_ccs(ccs,method=i)
      color.save_PNG("tmp/colors-%d.png" % i)

if __name__ == "__main__":
   test_colorize()
