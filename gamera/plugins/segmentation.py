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

class cc_and_cluster(PluginFunction):
    pure_python = 1
    self_type = ImageType([ONEBIT])
    args = Args([Float('ratio', default = 1.0), Int('distance', default=2)])
    return_type = ImageList("ccs")
    def __call__(image, ratio = 1.0, distance = 2):
        from gamera import cluster
        cc = image.cc_analysis()
        return cluster.cluster(cc, ratio, distance)
    __call__ = staticmethod(__call__)
cc_and_cluster = cc_and_cluster()

class splitx(PluginFunction):
    self_type = ImageType([ONEBIT])
    args = Args([Float("center")])
    return_type = ImageList("splits")
    def __call__(self, image, center=0.5):
        return image.splitx(self._center, center)
splitx = splitx()

class splity(PluginFunction):
    self_type = ImageType([ONEBIT])
    args = Args([Float("center")])
    return_type = ImageList("splits")
    def __call__(self, image, center=0.5):
        return image.splity(self._center, center)
splity = splity()

class splitx_base(PluginFunction):
    pure_python = 1
    self_type = ImageType([ONEBIT])
    return_type = ImageList("splits")
    def __call__(self, image):
        return image.splitx(self._center)
    
class splitx_left(splitx_base):
    _center = 0.25
splitx_left = splitx_left()

class splitx_right(splitx_base):
    _center = 0.75
splitx_right = splitx_right()

class splity_base(PluginFunction):
    pure_python = 1
    self_type = ImageType([ONEBIT])
    return_type = ImageList("splits")
    def __call__(self, image):
        return image.splity(self._center)
    
class splity_top(splity_base):
    _center = 0.25
splity_top = splity_top()

class splity_bottom(splity_base):
    _center = 0.75
splity_bottom = splity_bottom()

class SegmentationModule(PluginModule):
    category = "Segmentation"
    cpp_headers=["segmentation.hpp"]
    cpp_namespaces = ["Gamera"]
    functions = [cc_analysis, splitx, splity, splitx_left, splitx_right,
                 splity_top, splity_bottom]
    author = "Michael Droettboom and Karl MacMillan"
    url = "http://gamera.dkc.jhu.edu/"

module = SegmentationModule()
