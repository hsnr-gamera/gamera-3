#
#
# Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#

from gamera.plugin import *
import _gui_support

class to_string(PluginFunction):
    """
    Encodes the image into a 'string' required by wxImage.
    (i.e. 8-bit RGB triplets).
    """
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, RGB, FLOAT, COMPLEX])
    return_type = Class("image_as_string")

class to_buffer(PluginFunction):
    """
    Encodes the image into a 'buffer' required by wx.Image.
    (i.e. 8-bit RGB triplets). If you need to convert a gamera image
    *scaled_image* to a wx.Bitmap, you can do so as follows:

    .. code:: Python
        
      wximg = wx.EmptyImage(scaled_image.ncols, scaled_image.nrows)
      scaled_image.to_buffer(wximg.GetDataBuffer())
      wxbmp = wx.BitmapFromImage(wximg)
    """
    category = "ExternalLibraries"
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, RGB, FLOAT, COMPLEX])
    args = Args(Class("Buffer"))

class draw_cc(PluginFunction):
    """
    Draws a colored Cc over an already initialized wxImage buffer.
    """
    self_type = ImageType([RGB])
    args = Args([ImageType([ONEBIT]),
                 Int("red"), Int("green"), Int("blue")])

class to_buffer_colorize(PluginFunction):
    """
    Encodes the image into a 'buffer' required by wxImage, and
    applies the given color to the foreground and background.
    """
    self_type = ImageType([ONEBIT, GREYSCALE])
    args = Args([Class("Buffer"),
                 Int("red"), Int("green"), Int("blue"),
                 Bool("invert")])

class color_ccs(PluginFunction):
    """
    Returns an RGB image where each connected component of the
    image is colored one of eight different colors.  This function can
    be used to verify that ``cc_analysis`` is working correctly for your
    image.

    It should be noted that this coloring does not take care of
    component adjacency. It may therefore happen that adjacent components
    obtain the same color. If this is not acceptable in your use case,
    have a look at graph_color_ccs_ instead.

    .. _graph_color_ccs: color.html#graph-color-ccs

    *ignore_unlabeled*:
      do not colorize unlabeled pixels (pixel value one), but leave them black

    .. note:: Connected component analysis must already be performed
              on the image (using cc_analysis_, for example) in order
              for this to work.
          
    .. _cc_analysis: segmentation.html#cc-analysis

    **Example 1:**

      .. image:: images/color_ccs.png
    """
    author = "Michael Droettboom, Karl MacMillan, and Robert Butz"
    args = Args([Check('ignore_unlabeled', default=True)])
    category = "Color"
    self_type = ImageType([ONEBIT])
    return_type = ImageType([RGB])
    def __call__(image, ignore_unlabeled=False):
        return _gui_support.color_ccs(image, ignore_unlabeled)
    __call__ = staticmethod(__call__)

# By default, the wxPython-devel RPM puts stuff here, but this
# should be done better
class GuiSupportModule(PluginModule):
    """
    This module provides various functions that support the GUI
    infrastructure.
    """
    category = None
    cpp_headers = ["gui_support.hpp"]
    functions = [to_string, to_buffer, to_buffer_colorize, color_ccs,
                 draw_cc]
    author = "Michael Droettboom and Karl MacMillan"
    url = "http://gamera.sourceforge.net/"
    
module = GuiSupportModule()
