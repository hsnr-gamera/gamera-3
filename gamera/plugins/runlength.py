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
import _runlength
import warnings

class FrequentRun(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = Int()
    doc_examples = [(ONEBIT,)]

class most_frequent_black_horizontal_run(FrequentRun):
    """Returns the length of the most frequently occurring horizontal run of
black pixels."""
    pass

class most_frequent_white_horizontal_run(FrequentRun):
    """Returns the length of the most frequently occurring horizontal run of
white pixels."""
    pass

class most_frequent_black_vertical_run(FrequentRun):
    """Returns the length of the most frequently occurring vertical run of
black pixels."""
    pass

class most_frequent_white_vertical_run(FrequentRun):
    """Returns the length of the most frequently occurring vertical run of
white pixels."""
    pass

class FrequentRuns(PluginFunction):
    self_type = ImageType([ONEBIT])
    args = Args([Int("n", default=-1)])
    return_type = Class()
    doc_examples = [(ONEBIT, 10)]
    author = "Michael Droettboom, after an idea by Christoph Dalitz"

class most_frequent_black_horizontal_runs(FrequentRuns):
    """Returns the lengths of the *n* most frequently occurring horizontal runs of
black pixels.

*n*
   The number of runlengths to return.  If *n* < 0, all runlengths will be returned.

The return value is a list of 2-tuples.  The first element in the tuple is the
run length, and the second element is its frequency.  The list is sorted
by descending frequency.
"""
    def __call__(self, n=-1):
        return _runlength.most_frequent_black_horizontal_runs(self, n)
    __call__ = staticmethod(__call__)

class most_frequent_white_horizontal_runs(FrequentRuns):
    """Returns the lengths of the *n* most frequently occurring horizontal runs of
white pixels.

*n*
   The number of runlengths to return.  If *n* < 0, all runlengths will be returned.

The return value is a list of 2-tuples.  The first element in the tuple is the
run length, and the second element is its frequency.  The list is sorted
by descending frequency.
"""
    def __call__(self, n=-1):
        return _runlength.most_frequent_white_horizontal_runs(self, n)
    __call__ = staticmethod(__call__)

class most_frequent_black_vertical_runs(FrequentRuns):
    """Returns the lengths of the *n* most frequently occurring vertical runs of
black pixels.

*n*
   The number of runlengths to return.  If *n* < 0, all runlengths will be returned.

The return value is a list of 2-tuples.  The first element in the tuple is the
run length, and the second element is its frequency.  The list is sorted
by descending frequency.
"""
    def __call__(self, n=-1):
        return _runlength.most_frequent_black_vertical_runs(self, n)
    __call__ = staticmethod(__call__)

class most_frequent_white_vertical_runs(FrequentRuns):
    """Returns the lengths of the *n* most frequently occurring vertical runs of
white pixels.

*n*
   The number of runlengths to return.  If *n* < 0, all runlengths will be returned.

The return value is a list of 2-tuples.  The first element in the tuple is the
run length, and the second element is its frequency.  The list is sorted
by descending frequency.
"""
    def __call__(self, n=-1):
        return _runlength.most_frequent_white_vertical_runs(self, n)
    __call__ = staticmethod(__call__)

class RunHistogram(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = IntVector()
    doc_examples = [(ONEBIT,)]
    author = "Michael Droettboom"

class black_horizontal_run_histogram(RunHistogram):
    """Returns the histogram of the length of black horizontal runs.

*return_value*
   The return value is an integer array, of length == ``image.ncols``.
   Each index in the array corresponds to a particular run length,
   and the value at that index is the number of times that that
   run length occurs in the image.
"""
    pass

class white_horizontal_run_histogram(RunHistogram):
    """Returns the histogram of the length of white horizontal runs.

*return_value*
   The return value is an integer array, of length == ``image.ncols``.
   Each index in the array corresponds to a particular run length,
   and the value at that index is the number of times that that
   run length occurs in the image.
"""
    pass

class black_vertical_run_histogram(RunHistogram):
    """Returns the histogram of the length of black vertical runs.

*return_value*
   The return value is an integer array, of length == ``image.ncols``.
   Each index in the array corresponds to a particular run length,
   and the value at that index is the number of times that that
   run length occurs in the image.
"""
    pass

class white_vertical_run_histogram(RunHistogram):
    """Returns the histogram of the length of white vertical runs.

*return_value*
   The return value is an integer array, of length == ``image.ncols``.
   Each index in the array corresponds to a particular run length,
   and the value at that index is the number of times that that
   run length occurs in the image.
"""
    pass

class FilterRuns(PluginFunction):
    self_type = ImageType([ONEBIT])
    args = Args(Int("size"))

class filter_narrow_runs(FilterRuns):
    pure_python = True
    """Removes black horizontal runs narrower than a given length.

DEPRECATED: Use filter_narrow_black_runs_ instead.
"""
    def __call__(self, length):
        warnings.warn("Use filter_narrow_black_runs instead of filter_narrow_runs.", DeprecationWarning)
        return _runlength.filter_narrow_black_runs(self, length)
    doc_examples = [(ONEBIT, 5)]

class filter_short_runs(FilterRuns):
    pure_python = True
    """Removes black horizontal runs shorter than a given length.

DEPRECATED: Use filter_short_black_runs_ instead.
"""
    def __call__(self, length):
        warnings.warn("Use filter_short_black_runs instead of filter_short_runs.", DeprecationWarning)
        return _runlength.filter_short_black_runs(self, length)
    doc_examples = [(ONEBIT, 5)]

class filter_tall_runs(FilterRuns):
    pure_python = True
    """Removes black horizontal runs taller than a given length.

DEPRECATED: Use filter_tall_black_runs_ instead.
"""
    def __call__(self, length):
        warnings.warn("Use filter_tall_black_runs instead of filter_tall_runs.", DeprecationWarning)
        return _runlength.filter_tall_black_runs(self, length)
    doc_examples = [(ONEBIT, 5)]

class filter_wide_runs(FilterRuns):
    pure_python = True
    """Removes black horizontal runs wider than a given length.

DEPRECATED: Use filter_wide_black_runs_ instead.
"""
    def __call__(self, length):
        warnings.warn("Use filter_wide_black_runs instead of filter_wide_runs.", DeprecationWarning)
        return _runlength.filter_wide_black_runs(self, length)
    doc_examples = [(ONEBIT, 5)]

class filter_narrow_black_runs(FilterRuns):
    """Removes black horizontal runs narrower than a given length."""
    doc_examples = [(ONEBIT, 5)]

class filter_short_black_runs(FilterRuns):
    """Removes black vertical runs shorter than a given length."""
    doc_examples = [(ONEBIT, 5)]

class filter_tall_black_runs(FilterRuns):
    """Removes black vertical runs taller than a given length."""
    doc_examples = [(ONEBIT, 10)]

class filter_wide_black_runs(FilterRuns):
    """Removes black horizontal runs wider than a given length."""
    doc_examples = [(ONEBIT, 10)]

class filter_narrow_white_runs(FilterRuns):
    """Removes white horizontal runs narrower than a given length."""
    doc_examples = [(ONEBIT, 5)]

class filter_short_white_runs(FilterRuns):
    """Removes white vertical runs shorter than a given length."""
    doc_examples = [(ONEBIT, 5)]

class filter_tall_white_runs(FilterRuns):
    """Removes white vertical runs taller than a given length."""
    doc_examples = [(ONEBIT, 10)]

class filter_wide_white_runs(FilterRuns):
    """Removes white horizontal runs wider than a given length."""
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
    functions = [most_frequent_black_horizontal_run,
                 most_frequent_white_horizontal_run,
                 most_frequent_black_vertical_run,
                 most_frequent_white_vertical_run,
                 most_frequent_black_horizontal_runs,
                 most_frequent_white_horizontal_runs,
                 most_frequent_black_vertical_runs,
                 most_frequent_white_vertical_runs,
                 black_horizontal_run_histogram,
                 white_horizontal_run_histogram,
                 black_vertical_run_histogram,
                 white_vertical_run_histogram,
                 filter_narrow_runs, filter_short_runs,
                 filter_tall_runs,filter_wide_runs,
                 filter_narrow_black_runs, filter_short_black_runs,
                 filter_tall_black_runs,filter_wide_black_runs,
                 filter_narrow_white_runs, filter_short_white_runs,
                 filter_tall_white_runs,filter_wide_white_runs,
                 to_rle, from_rle]
    author = "Michael Droettboom and Karl MacMillan"
    url = "http://gamera.dkc.jhu.edu/"

module = RunLengthModule()
                 
del FrequentRun
del FrequentRuns
del RunHistogram
del FilterRuns
