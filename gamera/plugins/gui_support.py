#
#
# Copyright (C) 2001 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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

class to_string(PluginFunction):
    """Encodes the image into a 'string' required by wxImage.
(i.e. 8-bit RGB triplets)."""
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, RGB, FLOAT])
    return_type = Class("image_as_string")

class to_buffer(PluginFunction):
    """Encodes the image into a 'string' required by wxImage.
(i.e. 8-bit RGB triplets)."""
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, RGB, FLOAT])
    args = Args(Class("Buffer"))

class color_ccs(PluginFunction):
    """Returns an RGB image where each connected component of the
image is colored one of eight different colors.  This function can
be used to verify that ``cc_analysis`` is working correctly for your
image.

.. note: Connected component analysis must already be performed on the image
   (using cc_analysis_, for example) in order for this to work.

**Example 1:**

.. image:: images/color_ccs.png"""
    category = "Color"
    self_type = ImageType([ONEBIT])
    return_type = ImageType([RGB])

# By default, the wxPython-devel RPM puts stuff here, but this
# should be done better
wxpython_prefix = "/usr/lib/wxPython/"
class GuiSupportModule(PluginModule):
    """This module provides various functions that support the GUI
    infrastructure."""
    category = None
    cpp_headers = ["gui_support.hpp"]
    cpp_namespaces = ["Gamera"]
    functions = [to_string, to_buffer, color_ccs]
    author = "Michael Droettboom and Karl MacMillan"
    url = "http://gamera.dkc.jhu.edu/"
    
module = GuiSupportModule()
