#
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
import _runlength

# New version of functions.  Deprecated versions are below.

class most_frequent_run(PluginFunction):
    """
    Returns the length of the most frequently occurring run of pixels
    in the given color and given direction.
    """
    self_type = ImageType([ONEBIT])
    args = Args([ChoiceString("color", ["black", "white"]),
                 ChoiceString("direction", ["horizontal", "vertical"])])
    return_type = Int()
    doc_examples = [(ONEBIT, 'black', 'horizontal')]

class most_frequent_runs(PluginFunction):
    """
    Returns the lengths of the *n* most frequently occurring runs in
    the given *color* and *direction*.

    *n*
      The number of runlengths to return.  If *n* < 0, all runlengths
      will be returned.

    The return value is a list of 2-tuples.  The first element in the
    tuple is the run length, and the second element is its frequency.
    The list is sorted by descending frequency.
    """
    self_type = ImageType([ONEBIT])
    args = Args([Int("n"),
                 ChoiceString("color", ["black", "white"]),
                 ChoiceString("direction", ["horizontal", "vertical"])])
    return_type = Class()
    author = "Michael Droettboom, after an idea by Christoph Dalitz"
    def __call__(image, n = -1, color = 'black', direction = 'horizontal'):
        return _runlength.most_frequent_runs(image, n, color, direction)
    __call__ = staticmethod(__call__)
    doc_examples = [(ONEBIT, 5, 'black', 'horizontal')]

class run_histogram(PluginFunction):
    """
    Returns the histogram of runlengths in the given *color* and
    *direction*.

    *return_value*
      The return value is an integer array.  Each index in the array
      corresponds to a particular run length, and the value at that
      index is the number of times that that run length occurs in the
      image.
    """
    self_type = ImageType([ONEBIT])
    args = Args([ChoiceString("color", ["black", "white"]),
                 ChoiceString("direction", ["horizontal", "vertical"])])
    return_type = IntVector()
    doc_examples = [(ONEBIT, 'black', 'horizontal')]

class FilterRuns(PluginFunction):
    self_type = ImageType([ONEBIT])
    args = Args([Int("length"), ChoiceString("color", ["black", "white"])])
    doc_examples = [(ONEBIT, 3, 'black')]

class filter_narrow_runs(FilterRuns):
    """
    Removes horizontal runs in the given *color* narrower than a given
    *length*.
    """
    def __call__(image, length, color = 'black'):
        return _runlength.filter_narrow_runs(image, length, color)
    __call__ = staticmethod(__call__)

class filter_wide_runs(FilterRuns):
    """
    Removes horizontal runs in the given *color* wider than a given
    *length*.
    """
    def __call__(image, length, color = 'black'):
        return _runlength.filter_wide_runs(image, length, color)
    __call__ = staticmethod(__call__)

class filter_tall_runs(FilterRuns):
    """
    Removes vertical runs in the given *color* taller than a given
    *length*.
    """
    def __call__(image, length, color = 'black'):
        return _runlength.filter_tall_runs(image, length, color)
    __call__ = staticmethod(__call__)

class filter_short_runs(FilterRuns):
    """
    Removes vertical runs in the given *color* shorter than a given
    *length*.
    """
    def __call__(image, length, color = 'black'):
        return _runlength.filter_short_runs(image, length, color)
    __call__ = staticmethod(__call__)

class to_rle(PluginFunction):
    """
    Encodes a string-based run-length encoded version of the image.

    The numbers alternate between "length of black run" and "length of
    white run".  Runs go left-to-right, top-to-bottom.  Runs rollover
    the right hand edge and continue on the left edge of the next run.

    To decode an RLE string, use from_rle_.
    """
    self_type = ImageType([ONEBIT])
    return_type = String("runs")
    doc_examples = [(ONEBIT,)]

class from_rle(PluginFunction):
    """
    Decodes a string-based run-length encoded version of the image.

    The numbers alternate between "length of black run" and "length of
    white run".  Runs go left-to-right, top-to-bottom.  Runs rollover
    the right hand edge and continue on the left edge of the next run.

    To encode an RLE string, use to_rle_."""
    self_type = ImageType([ONEBIT])
    args = Args(String("runs"))

class iterate_runs(PluginFunction):
    """
    Returns nested iterators over the runs in the given *color* and
    *direction*.

    Each run is returned as a Rect object.
    
    For example, to iterate over all runs:

    .. code:: Python

      for row in image.iterate_black_horizontal_runs():
      # All the runs in each row
      for run in row:
          print run
    """
    self_type = ImageType([ONEBIT])
    args = Args([ChoiceString("color", ["black", "white"]),
                 ChoiceString("direction", ["horizontal", "vertical"])])
    return_type = Class()

###########################################################################    
# Deprecated functions.

class FrequentRun(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = Int()
    category = "Runlength/Deprecated"
    pure_python = True

class FrequentRuns(PluginFunction):
    self_type = ImageType([ONEBIT])
    args = Args([Int("n", default=-1)])
    return_type = Class()
    author = "Michael Droettboom, after an idea by Christoph Dalitz"
    category = "Runlength/Deprecated"
    pure_python = True

class RunHistogram(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = IntVector()
    author = "Michael Droettboom"
    category = "Runlength/Deprecated"
    pure_python = True

class FilterRunsDep(PluginFunction):
    self_type = ImageType([ONEBIT])
    args = Args(Int("size"))
    category = "Runlength/Deprecated"
    pure_python = True

class RunIterator(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = Class("iterator")
    category = "Runlength/Deprecated"
    pure_python = True

class RunLengthModule(PluginModule):
    category = "Runlength"
    cpp_headers=["runlength.hpp"]
    functions = [most_frequent_run,
                 most_frequent_runs,
                 run_histogram,
                 filter_narrow_runs,
                 filter_wide_runs,
                 filter_short_runs,
                 filter_tall_runs,
                 iterate_runs,
                 to_rle, from_rle]

    author = "Michael Droettboom and Karl MacMillan"
    url = "http://gamera.sourceforge.net/"

module = RunLengthModule()
                 
del FrequentRun
del FrequentRuns
del RunHistogram
del FilterRuns
del RunIterator
del FilterRunsDep
