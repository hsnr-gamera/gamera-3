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
import _erode_dilate

class _Morphology(PluginFunction):
  cpp_source = "morphology.hpp"
  category = "Morphology"
  self_type = ImageType(["OneBit"])

class erode(_Morphology):
  def __call__(image):
    _erode_dilate.erode_dilate(image.m, 1, 1, 0)
erode = erode()

class dilate(_Morphology):
  def __call__(image):
    _erode_dilate.erode_dilate(image.m, 1, 0, 0)
dilate = dilate()

class erode_dilate(_Morphology):
  args = Args([Int('number of times', range=(0, 10), default=1), \
               Choice('direction', ['dilate', 'erode']), \
               Choice('window shape', ['rectangular', 'octagonal'])])
erode_dilate = erode_dilate()

plugins = [erode, dilate, erode_dilate]
