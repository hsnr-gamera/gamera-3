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
import morphology, logical

class _LineBase(PluginFunction):
  cpp_source = "feature.hpp"
  category = "Filter/Line"
  self_type = ImageType(["OneBit"])
  return_type = ImageType(["OneBit"])

class outline(_LineBase):
  def __call__(self):
       new_matrix = self.matrix_copy()
       new_matrix.dilate()
       new_matrix.xor_image(self.m)
       return new_matrix
outline = outline()

plugins = [outline]
