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

class thin_zs(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = ImageType([ONEBIT], "thinned")
thin_zs = thin_zs()

class thin_hs(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = ImageType([ONEBIT], "thinned")
thin_hs = thin_hs()

class thin_lc(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = ImageType([ONEBIT], "thinned")
thin_lc = thin_lc()

class ThinningModule(PluginModule):
    category = "Filter/Thinning"
    functions = [thin_zs, thin_hs, thin_lc]
    cpp_headers = ["thinning.hpp"]
    cpp_namespaces = ["Gamera"]
    author = "Michael Droettboom and Karl MacMillan"
    url = "http://gamera.dkc.jhu.edu/"
module = ThinningModule()
