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

class cc_analysis(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = ImageList("ccs")
cc_analysis = cc_analysis()

class splitx(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = ImageList("ccs")
    pure_python = 1

    def __call__(image):
        return [image.subimage(image.ul_y, image.ul_x, image.nrows, image.ncols / 2),
                image.subimage(image.ul_y, image.ul_x + image.ncols / 2, image.nrows, image.ncols / 2)]
splitx = splitx()

class SegmentationModule(PluginModule):
    category = "Segmentation"
    cpp_headers=["segmentation.hpp"]
    cpp_namespaces = ["Gamera"]
    functions = [cc_analysis, splitx]
    author = "Michael Droettboom and Karl MacMillan"
    url = "http://gamera.dkc.jhu.edu/"

module = SegmentationModule()
