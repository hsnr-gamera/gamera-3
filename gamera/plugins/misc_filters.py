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

class outline(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = ImageType([ONEBIT])
outline = outline()

class thin_zs(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = ImageType([ONEBIT])
thin_zs = thin_zs()

class expand_edges(PluginFunction):
    self_type = ImageType([ONEBIT, FLOAT, GREYSCALE, GREY16, RGB])
    args = Args([Int("size", default=1), Int("zeros"), Int("even")])
    return_type = ImageType([ONEBIT, FLOAT, GREYSCALE, GREY16, RGB])
expand_edges = expand_edges()

class MiscFiltersModule(PluginModule):
    category = "Filter"
    functions = [outline, expand_edges]
    cpp_headers = ["misc_filters.hpp"]
    cpp_namespaces = ["Gamera"]
    author = "Michael Droettboom and Karl MacMillan"
    url = "http://gamera.dkc.jhu.edu/"
module = MiscFiltersModule()
