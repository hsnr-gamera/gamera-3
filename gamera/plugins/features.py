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
import _features

class black_area(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = FloatVector(length=1)
    feature_function = True

class moments(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = FloatVector(length=9)
    feature_function = True

class nholes(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = FloatVector(length=2)
    feature_function = True

class nholes_extended(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = FloatVector(length=8)
    feature_function = True

class volume(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = FloatVector(length=1)
    feature_function = True

class area(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = FloatVector(length=1)
    feature_function = True

class aspect_ratio(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = FloatVector(length=1)
    feature_function = True

class compactness(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = FloatVector(length=1)
    feature_function = True

class volume16regions(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = FloatVector(length=16)
    feature_function = True

class volume64regions(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = FloatVector(length=64)
    feature_function = True

class zernike_moments(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = FloatVector(length=26)
    feature_function = True

class skeleton_features(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = FloatVector(length=6)
    feature_function = True

class generate_features(PluginFunction):
    category = "Utility"
    pure_python = 1
    self_type = ImageType([ONEBIT])
    return_type = None
    cache = {}
    def __call__(self, features=None):
      if features is None:
         features = self.get_feature_functions()
      if self.feature_functions == features:
         return
      self.feature_functions = features
      features, num_features = features
      if len(self.features) != num_features:
          if not generate_features.cache.has_key(num_features):
              generate_features.cache[num_features] = [0] * num_features
          self.features = array.array('d', generate_features.cache[num_features])
      offset = 0
      for name, function in features:
          function.__call__(self, offset)
          offset += function.return_type.length
    __call__ = staticmethod(__call__)

class FeaturesModule(PluginModule):
    category = "Features"
    cpp_headers=["features.hpp"]
    cpp_namespaces = ["Gamera"]
    functions = [black_area, moments, nholes,
                 nholes_extended, volume, area,
                 aspect_ratio, compactness,
                 volume16regions, volume64regions,
                 generate_features, zernike_moments,
                 skeleton_features]
    author = "Michael Droettboom and Karl MacMillan"
    url = "http://gamera.dkc.jhu.edu/"
module = FeaturesModule()

def get_features_length(features):
    """Given a list of feature functions return the number
    of features that will be generated. This function is
    necessary because each features 'function' can return
    multiple individual float values."""
    from gamera import core
    ff = core.ImageBase.get_feature_functions(features)
    return ff[1]

_empty_feature_vector_cache = None
_feature_functions = None
def _get_empty_feature_vector(features):
    global _empty_feature_vector_cache
    

def generate_features_list(list, feature_functions='all'):
   """Generate features on a list of images using either the feature
   functions passed in or the default features."""
   from gamera import core
   ff = core.Image.get_feature_functions(feature_functions)
   progress = util.ProgressFactory("Generating features...", len(list) / 10)
   try:
      for i, glyph in enumerate(list):
         glyph.generate_features(ff)
         if i % 10 == 0:
             progress.step()
   finally:
       progress.kill()
