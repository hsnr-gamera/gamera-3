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

class outline(PluginFunction):
    pure_python = 1
    self_type = ImageType([ONEBIT])
    return_type = ImageType([ONEBIT])
    def __call__(image):
        new_image = image.image_copy()
        new_image.dilate()
        new_image.xor_image(image)
        return new_image
    __call__ = staticmethod(__call__)

class MiscFiltersModule(PluginModule):
    pure_python = 1
    category = "Filter"
    functions = [outline]
    author = "Michael Droettboom and Karl MacMillan"
    url = "http://gamera.dkc.jhu.edu/"
module = MiscFiltersModule()
