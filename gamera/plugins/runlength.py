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
    """Returns the length of the most frequently occurring horizontal run of
black pixels."""
    self_type = ImageType([ONEBIT])
    return_type = Int()
    doc_examples = [(ONEBIT,)]

class most_frequent_white_horizontal_run(PluginFunction):
    """Returns the length of the most frequently occurring horizontal run of
white pixels."""
    self_type = ImageType([ONEBIT])
    return_type = Int()
    doc_examples = [(ONEBIT,)]

class most_frequent_black_vertical_run(PluginFunction):
    """Returns the length of the most frequently occurring vertical run of
black pixels."""
    self_type = ImageType([ONEBIT])
    return_type = Int()
    doc_examples = [(ONEBIT,)]

class most_frequent_white_vertical_run(PluginFunction):
    """Returns the length of the most frequently occurring vertical run of
white pixels."""
    self_type = ImageType([ONEBIT])
    return_type = Int()
    doc_examples = [(ONEBIT,)]

class filter_narrow_runs(PluginFunction):
    """Removes black horizontal runs narrower than a given length."""
    self_type = ImageType([ONEBIT])
    args = Args(Int("size"))
    doc_examples = [(ONEBIT, 5)]

class filter_short_runs(PluginFunction):
    """Removes black vertical runs shorter than a given length."""
    self_type = ImageType([ONEBIT])
    args = Args(Int("size"))
    doc_examples = [(ONEBIT, 5)]

class filter_tall_runs(PluginFunction):
    """Removes black vertical runs taller than a given length."""
    self_type = ImageType([ONEBIT])
    args = Args(Int("size"))
    doc_examples = [(ONEBIT, 10)]

class filter_wide_runs(PluginFunction):
    """Removes black horizontal runs wider than a given length."""
    self_type = ImageType([ONEBIT])
    args = Args(Int("size"))
    doc_examples = [(ONEBIT, 10)]

class to_rle(PluginFunction):
    """Encodes a string-based run-length encoded version of the image.

The numbers alternate between "length of black run" and "length of white run".
Runs go left-to-right, top-to-bottom.
Runs rollover the right hand edge and continue on the left edge of the next run.

To decode an RLE string, use from_rle_."""
    self_type = ImageType([ONEBIT])
    return_type = String("runs")
    doc_examples = [(ONEBIT,)]

class from_rle(PluginFunction):
    """Decodes a string-based run-length encoded version of the image.

The numbers alternate between "length of black run" and "length of white run".
Runs go left-to-right, top-to-bottom.
Runs rollover the right hand edge and continue on the left edge of the next run.

To encode an RLE string, use to_rle_."""
    self_type = ImageType([ONEBIT])
    args = Args(String("runs"))

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
                 
