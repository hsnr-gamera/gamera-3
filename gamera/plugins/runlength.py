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

class most_frequent_black_horizontal_run(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = Int("value")
most_frequent_black_horizontal_run = most_frequent_black_horizontal_run()

class most_frequent_white_horizontal_run(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = Int("value")
most_frequent_white_horizontal_run = most_frequent_white_horizontal_run()

class most_frequent_black_vertical_run(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = Int("value")
most_frequent_black_vertical_run = most_frequent_black_vertical_run()

class most_frequent_white_vertical_run(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = Int("value")
most_frequent_white_vertical_run = most_frequent_white_vertical_run()

class filter_narrow_runs(PluginFunction):
    self_type = ImageType([ONEBIT])
    args = Args(Int("max_width"))
filter_narrow_runs = filter_narrow_runs()

class filter_short_runs(PluginFunction):
    self_type = ImageType([ONEBIT])
    args = Args(Int("max_height"))
filter_short_runs = filter_short_runs()

class filter_tall_runs(PluginFunction):
    self_type = ImageType([ONEBIT])
    args = Args(Int("min_height"))
filter_tall_runs = filter_tall_runs()

class filter_wide_runs(PluginFunction):
    self_type = ImageType([ONEBIT])
    args = Args(Int("min_width"))
filter_wide_runs = filter_wide_runs()

class to_rle(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = IntVector("runs")
to_rle = to_rle()

class from_rle(PluginFunction):
    self_type = ImageType([ONEBIT])
    args = Args(Class("runs"))
from_rle = from_rle()

class RunLengthModule(PluginModule):
    category = "Runlength"
    cpp_headers=["runlength.hpp"]
    cpp_namespaces = ["Gamera"]
    functions = [most_frequent_black_horizontal_run,
                 most_frequent_white_horizontal_run,
                 most_frequent_black_vertical_run,
                 most_frequent_white_vertical_run,
                 filter_narrow_runs, filter_short_runs,
                 filter_tall_runs,filter_wide_runs,
                 to_rle, from_rle]
    author = "Michael Droettboom and Karl MacMillan"
    url = "http://gamera.dkc.jhu.edu/"

module = RunLengthModule()
                 
