#
#
# Copyright (C) 2002 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#

from gamera.plugin import *

class save_png(PluginFunction):
   """Saves the image to a PNG format file.

.. warning: This is a hacky implementation.
   This currently only supports saving to 24-bit RGB and requires that
   wxPython is installed.  There should be a native ``libpng`` version
   written someday, but in the meantime this provides everything I
   need to generate the documentation."""
   self_type = ImageType(ALL)
   args = Args([FileSave("image_file_name", "image.png", "*.png")])
   pure_python = True
   inited_image_handlers = False
   def __call__(self, filename):
      try:
         import wxPython.wx
      except ImportError, e:
         raise Exception("Cannot save as PNG, since you do not have wxPython installed. " +
                         "Soon, we should have native PNG support and we son't have this silly " +
                         "requirement.")
      if not save_png.inited_image_handlers:
         wxPython.wx.wxInitAllImageHandlers()
         save_png.inited_image_handlers = True
      image = wxPython.wx.wxEmptyImage(self.ncols, self.nrows)
      self.to_buffer(image.GetDataBuffer())
      image.SaveFile(filename, wxPython.wx.wxBITMAP_TYPE_PNG)
   __call__ = staticmethod(__call__)

class PngSupportModule(PluginModule):
    category = "File"
    functions = [save_png]
    pure_python = 1
    author = "Michael Droettboom and Karl MacMillan"
    url = "http://gamera.dkc.jhu.edu/"
module = PngSupportModule()

                         
         
