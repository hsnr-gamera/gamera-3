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
import popen2

class to_string(PluginFunction):
    """Encodes the image into a 'string' required by wxImage.
(i.e. 8-bit RGB triplets)."""
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, RGB, FLOAT])
    return_type = Class("image_as_string")
to_string = to_string()

class scaled_to_string(PluginFunction):
    """Encodes the image at the given scaling into a 'string'
    required by wxImage. (i.e. 8-bit RGB triplets)."""
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, RGB, FLOAT])
    args = Args([Float("scaling"),Class("Buffer")])
scaled_to_string = scaled_to_string()

class to_buffer(PluginFunction):
    """Encodes the image into a 'string' required by wxImage.
(i.e. 8-bit RGB triplets)."""
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, RGB, FLOAT])
    args = Args(Class("Buffer"))
to_buffer = to_buffer()

# By default, the wxPython-devel RPM puts stuff here, but this
# should be done better
wxpython_prefix = "/usr/lib/wxPython/"
class GuiSupportModule(PluginModule):
    """This module provides various functions that support the GUI
    infrastructure."""
    category = None
    cpp_headers = ["gui_support.hpp"]
    functions = [to_string, scaled_to_string, to_buffer]
    author = "Michael Droettboom and Karl MacMillan"
    url = "http://gamera.dkc.jhu.edu/"
    
module = GuiSupportModule()
