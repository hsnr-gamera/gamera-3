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
import _and_image, _or_image, _xor_image

  
class _LogicalBase(PluginFunction):
  self_type = ImageType(['OneBit'])
  args = Args([ImageType(['OneBit'])])

class and_image(_LogicalBase):
  pass
and_image = and_image()

class or_image(_LogicalBase):
  pass
or_image = or_image()

class xor_image(_LogicalBase):
  pass
xor_image = xor_image()

class LogicalModule(PluginModule):
  category = "Logical"
  cpp_headers = ["logical.hpp"]
  functions = [and_image, or_image, xor_image]

module = LogicalModule()
