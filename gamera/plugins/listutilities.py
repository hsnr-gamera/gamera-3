#
# Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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

class permute_list(PluginFunction):
   """
   Permutes the given list (in place) one step.

   Returns ``True`` if there are more permutations to go.  Returns
   ``False`` if permutations are done.

   Example usage::

     >>>from gamera.plugins import listutilities
     >>>a = [1,2,3]
     >>>while listutilities.permute_list(a):
     ...    print a
     ...
     [2, 1, 3]
     [1, 3, 2]
     [3, 1, 2]
     [2, 3, 1]
     [3, 2, 1]
   """
   self_type = None
   args = Args([Class("list")])
   return_type = Int("continuaton")

class all_subsets(PluginFunction):
   self_type = None
   args = Args([Class("list"), Int("size")])
   return_type = Class("subsets")

class ListUtilitiesModule(PluginModule):
   category = "List"
   cpp_headers=["listutilities.hpp"]
   functions = [permute_list, all_subsets]
   author = "Michael Droettboom and Karl MacMillan"
   url = "http://gamera.dkc.jhu.edu/"
module = ListUtilitiesModule()

permute_list = permute_list()
all_subsets = all_subsets()
