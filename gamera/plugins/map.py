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

from plugin import *
import _variance_map, _mean_map
import gamera

class _Map(PluginFunction):
  cpp_source = "map.hpp"
  category = "Analysis/Map"
  return_type = ImageType(('Float', 'GreyScale', 'Grey16'), "map")
  self_type = ImageType(('Float', 'GreyScale', 'Grey16'))

class mean_map(_Map):
  args = Args([Int("window_size", (1, 256), 11)])

  def __call__(self, window_size=11):
    map = Matrix(int(float(self.nrows()) / float(window_size)),
                 int(float(self.ncols()) / float(window_size)),
                 "Float")
    _mean_map.mean_map(self.m, map.m)
    return map
mean_map = mean_map()

class variance_map(_Map):
  args = Args([Int("window_size", (1, 256), 11)])
  
  def __call__(self, window_size=11):
    map = gamera.Image(int(float(self.nrows()) / float(window_size)),
                       int(float(self.ncols()) / float(window_size)),
                       "Float")
    _variance_map.variance_map(self, map)
    return map
variance_map = variance_map()

plugins = [mean_map, variance_map]
