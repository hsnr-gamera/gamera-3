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

import array
from gamera.plugin import *
import gamera.util

class black_area(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = Int("black_area")
black_area = black_area()

class moments(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = FloatVector("moments", 9)
moments = moments()

class nholes(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = FloatVector("nholes", 2)
nholes = nholes()

class nholes_extended(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = FloatVector("nholes", 8)
nholes_extended = nholes_extended()

class volume(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = Float("volume")
volume = volume()

class area(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = Float("area")
area = area()

class aspect_ratio(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = Float("aspect_ratio")
aspect_ratio = aspect_ratio()

class compactness(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = Float("compactness")
compactness = compactness()

class volume16regions(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = FloatVector("volumes", 16)
volume16regions = volume16regions()

class volume64regions(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = FloatVector("volumes", 64)
volume64regions = volume64regions()

class generate_features(PluginFunction):
    category = "Utility"
    pure_python = 1
    self_type = ImageType([ONEBIT])
    return_type = None
    def __call__(self, features=None):
      if features is None:
         features = self.get_feature_functions()
      if self.feature_functions == features:
         return
      self.feature_functions = features
      self.features = array.array('d')
      for name, function in features:
         result = apply(function.__call__, (self,))
         if function.return_type.length == 1:
            self.features.append(result)
         else:
            self.features.extend(result)
    __call__ = staticmethod(__call__)
generate_features = generate_features()


class FeaturesModule(PluginModule):
    category = "Features"
    cpp_headers=["features.hpp"]
    cpp_namespaces = ["Gamera"]
    functions = [black_area, moments, nholes,
                 nholes_extended, volume, area,
                 aspect_ratio, compactness,
                 volume16regions, volume64regions,
                 generate_features]
    author = "Michael Droettboom and Karl MacMillan"
    url = "http://gamera.dkc.jhu.edu/"
module = FeaturesModule()
