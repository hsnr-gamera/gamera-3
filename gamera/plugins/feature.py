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
import line, _volume, gamera
import sys

class _FeatureBase(PluginFunction):
  cpp_source = "feature.hpp"
  category = "Feature"
  return_type = Float("feature")

class area(_FeatureBase):
  def __call__(self):
    return float(self.nrows() * self.ncols()) / self.scaling
area = area()

class aspect_ratio(_FeatureBase):
  def __call__(self):
    return float(self.ncols()) / float(self.nrows())
aspect_ratio = aspect_ratio()

class compactness(_FeatureBase):
  self_type = ImageType(["OneBit"])

  def __call__(self):
    o = self.outline()
    volume = self.volume()
    if volume != 0:
      return o.volume() / volume * self.scaling
    else:
      return sys.maxint
compactness = compactness()

class height(_FeatureBase):
  def __call__(self):
    return self.nrows()
height = height()

class moments(_FeatureBase):
  self_type = ImageType(["OneBit"])
moments = moments()

class nholes(_FeatureBase):
  self_type = ImageType(["OneBit"])
nholes = nholes()

class volume(_FeatureBase):
  self_type = ImageType(["OneBit"])
volume = volume()

class volume4regions(_FeatureBase):
  self_type = ImageType(["OneBit"])
  def __call__(self):
    half_rows = (self.nrows() - 1) / 2
    half_cols = (self.ncols() - 1) / 2
    nw = gamera.SubImage(self, 0, 0,
                          half_rows + 1, half_cols + 1)
    ne = gamera.SubImage(self, 0, half_cols,
                          half_rows + 1, half_cols + 1)
    sw = gamera.SubImage(self, half_rows, 0,
                          half_rows + 1, half_cols + 1)
    se = gamera.SubImage(self, half_rows, half_cols,
                          half_rows + 1, half_cols + 1)
    return map(lambda x: x.volume() * x.scaling, (nw, ne, sw, se))
volume4regions = volume4regions()

class width(_FeatureBase):
  def __call__(self):
    return self.ncols()
width = width()

class FeatureModule(PluginModule):
  cpp_headers = ["feature.hpp"]
  category = "Features"
  functions = [area, aspect_ratio, compactness, volume, height, moments, nholes, volume4regions]
  author = "Michael Droettboom and Karl MacMillan"
  url = "http://gamera.dkc.jhu.edu/"

module = FeatureModule()
