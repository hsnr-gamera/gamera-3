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

class distance_matrix(PluginFunction):
    self_type = None
    args = Args(ImageList('images'))
    return_type = ImageType([FLOAT])
distance_matrix = distance_matrix()

class unique_distances(PluginFunction):
    self_type = None
    args = Args(Class('images'))
    return_type = Class('unique_distances')
unique_distances = unique_distances()

class DistanceModule(PluginModule):
    cpp_headers=["distance.hpp"]
    cpp_namespace=["Gamera"]
    category = "Distance"
    functions = [distance_matrix, unique_distances]
    author = "Karl MacMillan"
    url = "http://gamera.dkc.jhu.edu/"

module = DistanceModule()

class Distance:
    def __init__(self, features=None, weights=None):
        self.features = features
        if self.features == None:
            self.features = 'all'
        self.weights = weights
        
