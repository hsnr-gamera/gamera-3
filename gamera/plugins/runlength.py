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
from gamera.util import warn_deprecated

# New version of functions.  Deprecated versions are below.

class most_frequent_run(PluginFunction):
    """Returns the length of the most frequently occurring run of pixels in
the given color and given direction."""
    self_type = ImageType([ONEBIT])
    args = Args([ChoiceString("color", ["black", "white"]),
                 ChoiceString("direction", ["horizontal", "vertical"])])
    return_type = Int()
    doc_examples = [(ONEBIT, 'black', 'horizontal')]

class most_frequent_runs(PluginFunction):
    """Returns the lengths of the *n* most frequently occurring runs in the given
*color* and *direction*.

*n*
   The number of runlengths to return.  If *n* < 0, all runlengths will be returned.

The return value is a list of 2-tuples.  The first element in the tuple is the
run length, and the second element is its frequency.  The list is sorted
by descending frequency."""
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
    """Returns the histogram of runlengths in the given *color* and *direction*.

*return_value*
   The return value is an integer array.  Each index in the array
   corresponds to a particular run length, and the value at that index
   is the number of times that that run length occurs in the image.
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
    """Removes horizontal runs in the given *color* narrower than a given *length*."""
    def __call__(image, length, color = 'black'):
        return _runlength.filter_narrow_runs(image, length, color)
    __call__ = staticmethod(__call__)

class filter_wide_runs(FilterRuns):
    """Removes horizontal runs in the given *color* wider than a given *length*."""
    def __call__(image, length, color = 'black'):
        return _runlength.filter_wide_runs(image, length, color)
    __call__ = staticmethod(__call__)

class filter_tall_runs(FilterRuns):
    """Removes vertical runs in the given *color* taller than a given *length*."""
    def __call__(image, length, color = 'black'):
        return _runlength.filter_tall_runs(image, length, color)
    __call__ = staticmethod(__call__)

class filter_short_runs(FilterRuns):
    """Removes vertical runs in the given *color* shorter than a given *length*."""
    def __call__(image, length, color = 'black'):
        return _runlength.filter_short_runs(image, length, color)
    __call__ = staticmethod(__call__)

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

class iterate_runs(PluginFunction):
    """Returns nested iterators over the runs in the given *color* and *direction*.

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

def most_frequent_run_dep(color, direction):
    def __call__(image):
        warn_deprecated("""most_frequent_%s_%s_run() is deprecated.

Reason: Functions parameterized by arguments, not by name.

Use most_frequent_run('%s', '%s') instead.""" %
                        (color, direction, color, direction))
        return _runlength.most_frequent_run(image, color, direction)
    return staticmethod(__call__)

class most_frequent_black_horizontal_run(FrequentRun):
    """Returns the length of the most frequently occurring horizontal run of
black pixels.

.. warning::

  most_frequent_black_horizontal_run() is deprecated.

  Reason: Functions parameterized by arguments, not by name.

  Use most_frequent_run('black', 'horizontal') instead.
"""
    __call__ = most_frequent_run_dep("black", "horizontal")

class most_frequent_white_horizontal_run(FrequentRun):
    """Returns the length of the most frequently occurring horizontal run of
white pixels.

.. warning::

  most_frequent_white_horizontal_run() is deprecated.

  Reason: Functions parameterized by arguments, not by name.

  Use most_frequent_run('white', 'horizontal') instead.
"""
    __call__ = most_frequent_run_dep("white", "horizontal")

class most_frequent_black_vertical_run(FrequentRun):
    """Returns the length of the most frequently occurring vertical run of
black pixels.

.. warning::

  most_frequent_black_vertical_run() is deprecated.

  Reason: Functions parameterized by arguments, not by name.

  Use most_frequent_run('black', 'vertical') instead.
"""
    __call__ = most_frequent_run_dep("black", "vertical")

class most_frequent_white_vertical_run(FrequentRun):
    """Returns the length of the most frequently occurring vertical run of
white pixels.

.. warning::

  most_frequent_white_vertical_run() is deprecated.

  Reason: Functions parameterized by arguments, not by name.

  Use most_frequent_run('white', 'vertical') instead.
"""
    __call__ = most_frequent_run_dep("white", "vertical")

class FrequentRuns(PluginFunction):
    self_type = ImageType([ONEBIT])
    args = Args([Int("n", default=-1)])
    return_type = Class()
    author = "Michael Droettboom, after an idea by Christoph Dalitz"
    category = "Runlength/Deprecated"
    pure_python = True

def most_frequent_runs_dep(color, direction):
    def __call__(image, n=-1):
        warn_deprecated("""most_frequent_%s_%s_runs(Int *n*) is deprecated.

Reason: Functions parameterized by arguments, not by name.

Use most_frequent_runs(n, '%s', '%s') instead.""" %
                        (color, direction, color, direction))
        return _runlength.most_frequent_runs(image, n, color, direction)
    return staticmethod(__call__)

class most_frequent_black_horizontal_runs(FrequentRuns):
    """Returns the lengths of the *n* most frequently occurring horizontal runs of
black pixels.

*n*
   The number of runlengths to return.  If *n* < 0, all runlengths will be returned.

The return value is a list of 2-tuples.  The first element in the tuple is the
run length, and the second element is its frequency.  The list is sorted
by descending frequency.

.. warning::

  most_frequent_black_horizontal_runs(Int n = -1) is deprecated.

  Reason: Functions parameterized by arguments, not by name.

  Use most_frequent_runs(n, 'black', 'horizontal') instead.
"""
    __call__ = most_frequent_runs_dep("black", "horizontal")

class most_frequent_white_horizontal_runs(FrequentRuns):
    """Returns the lengths of the *n* most frequently occurring horizontal runs of
white pixels.

*n*
   The number of runlengths to return.  If *n* < 0, all runlengths will be returned.

The return value is a list of 2-tuples.  The first element in the tuple is the
run length, and the second element is its frequency.  The list is sorted
by descending frequency.

.. warning::

  most_frequent_white_horizontal_runs(Int n = -1) is deprecated.

  Reason: Functions parameterized by arguments, not by name.

  Use most_frequent_runs(n, 'white', 'horizontal') instead.
"""
    __call__ = most_frequent_runs_dep("white", "horizontal")

class most_frequent_black_vertical_runs(FrequentRuns):
    """Returns the lengths of the *n* most frequently occurring vertical runs of
black pixels.

*n*
   The number of runlengths to return.  If *n* < 0, all runlengths will be returned.

The return value is a list of 2-tuples.  The first element in the tuple is the
run length, and the second element is its frequency.  The list is sorted
by descending frequency.

.. warning::

  most_frequent_black_vertical_runs(Int n = -1) is deprecated.

  Reason: Functions parameterized by arguments, not by name.

  Use most_frequent_runs(n, 'black', 'vertical') instead.
"""
    __call__ = most_frequent_runs_dep("black", "vertical")

class most_frequent_white_vertical_runs(FrequentRuns):
    """Returns the lengths of the *n* most frequently occurring vertical runs of
white pixels.

*n*
   The number of runlengths to return.  If *n* < 0, all runlengths will be returned.

The return value is a list of 2-tuples.  The first element in the tuple is the
run length, and the second element is its frequency.  The list is sorted
by descending frequency.

.. warning::

  most_frequent_white_vertical_runs(Int n = -1) is deprecated.

  Reason: Functions parameterized by arguments, not by name.

  Use most_frequent_runs(n, 'white', 'vertical') instead.
"""
    __call__ = most_frequent_runs_dep("white", "vertical")

class RunHistogram(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = IntVector()
    author = "Michael Droettboom"
    category = "Runlength/Deprecated"
    pure_python = True

def histogram_dep(color, direction):
    def __call__(image):
        warn_deprecated("""%s_%s_run_histogram() is deprecated.

Reason: Functions parameterized by arguments, not by name.

Use run_histogram('%s', '%s') instead.""" %
                        (color, direction, color, direction))
        return _runlength.run_histogram(image, color, direction)
    return staticmethod(__call__)

class black_horizontal_run_histogram(RunHistogram):
    """Returns the histogram of the length of black horizontal runs.

*return_value*
   The return value is an integer array, of length == ``image.ncols``.
   Each index in the array corresponds to a particular run length,
   and the value at that index is the number of times that that
   run length occurs in the image.

.. warning::

  black_horizontal_run_histogram() is deprecated.

  Reason: Functions parameterized by arguments, not by name.

  Use run_histogram('black', 'horizontal') instead.
"""
    __call__ = histogram_dep("black", "horizontal")

class white_horizontal_run_histogram(RunHistogram):
    """Returns the histogram of the length of white horizontal runs.

*return_value*
   The return value is an integer array, of length == ``image.ncols``.
   Each index in the array corresponds to a particular run length,
   and the value at that index is the number of times that that
   run length occurs in the image.

.. warning::

  white_horizontal_run_histogram() is deprecated.

  Reason: Functions parameterized by arguments, not by name.

  Use run_histogram('white', 'horizontal') instead.
"""
    __call__ = histogram_dep("white", "horizontal")

class black_vertical_run_histogram(RunHistogram):
    """Returns the histogram of the length of black vertical runs.

*return_value*
   The return value is an integer array, of length == ``image.ncols``.
   Each index in the array corresponds to a particular run length,
   and the value at that index is the number of times that that
   run length occurs in the image.

.. warning::

  black_vertical_run_histogram() is deprecated.

  Reason: Functions parameterized by arguments, not by name.

  Use run_histogram('black', 'vertical') instead.
"""
    __call__ = histogram_dep("black", "vertical")

class white_vertical_run_histogram(RunHistogram):
    """Returns the histogram of the length of white vertical runs.

*return_value*
   The return value is an integer array, of length == ``image.ncols``.
   Each index in the array corresponds to a particular run length,
   and the value at that index is the number of times that that
   run length occurs in the image.

.. warning::

  white_vertical_run_histogram() is deprecated.

  Reason: Functions parameterized by arguments, not by name.

  Use run_histogram('white', 'vertical') instead.
"""
    __call__ = histogram_dep("white", "vertical")

class FilterRunsDep(PluginFunction):
    self_type = ImageType([ONEBIT])
    args = Args(Int("size"))
    category = "Runlength/Deprecated"
    pure_python = True

def filter_dep(direction, color = ""):
    def __call__(image, n):
        warn_deprecated("""filter_%s_%s_runs() is deprecated.

Reason: Functions parameterized by arguments, not by name.

Use filter_%s_runs(n, '%s') instead.""" %
                        (direction, color, direction, color))
        return getattr(_runlength, "filter_%s_runs" % direction)(image, n, color)
    return staticmethod(__call__)

class filter_narrow_black_runs(FilterRunsDep):
    """Removes black horizontal runs narrower than a given length.

.. warning::

  filter_narrow_black_runs(Int *length*) is deprecated.

  Reason: Functions parameterized by arguments, not by name.

  Use filter_narrow_runs(length, 'black') instead.
"""
    __call__ = filter_dep("narrow", "black")

class filter_wide_black_runs(FilterRunsDep):
    """Removes black horizontal runs wider than a given length.

.. warning::

  filter_wide_black_runs(Int *length*) is deprecated.

  Reason: Functions parameterized by arguments, not by name.

  Use filter_wide_runs(length, 'black') instead.
"""
    __call__ = filter_dep("wide", "black")

class filter_tall_black_runs(FilterRunsDep):
    """Removes black vertical runs taller than a given length.

.. warning::

  filter_tall_black_runs(Int *length*) is deprecated.

  Reason: Functions parameterized by arguments, not by name.

  Use filter_tall_runs(length, 'black') instead.
"""
    __call__ = filter_dep("tall", "black")

class filter_short_black_runs(FilterRunsDep):
    """Removes black vertical runs shorter than a given length.

.. warning::

  filter_short_black_runs(Int *length*) is deprecated.

  Reason: Functions parameterized by arguments, not by name.

  Use filter_short_runs(length, 'black') instead.
"""
    __call__ = filter_dep("short", "black")

class filter_narrow_white_runs(FilterRunsDep):
    """Removes white horizontal runs narrower than a given length.

.. warning::

  filter_narrow_white_runs(Int *length*) is deprecated.

  Reason: Functions parameterized by arguments, not by name.

  Use filter_narrow_runs(length, 'white') instead.
"""
    __call__ = filter_dep("narrow", "white")

class filter_wide_white_runs(FilterRunsDep):
    """Removes white horizontal runs wider than a given length.

.. warning::

  filter_wide_white_runs(Int *length*) is deprecated.

  Reason: Functions parameterized by arguments, not by name.

  Use filter_wide_runs(length, 'white') instead.
"""
    __call__ = filter_dep("wide", "white")

class filter_tall_white_runs(FilterRunsDep):
    """Removes white vertical runs taller than a given length.

.. warning::

  filter_tall_white_runs(Int *length*) is deprecated.

  Reason: Functions parameterized by arguments, not by name.

  Use filter_tall_runs(length, 'white') instead.
"""
    __call__ = filter_dep("tall", "white")

class filter_short_white_runs(FilterRunsDep):
    """Removes white vertical runs shorter than a given length.

.. warning::

  filter_short_white_runs(Int *length*) is deprecated.

  Reason: Functions parameterized by arguments, not by name.

  Use filter_short_runs(length, 'white') instead.
"""
    __call__ = filter_dep("short", "white")

class RunIterator(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = Class("iterator")
    category = "Runlength/Deprecated"
    pure_python = True

def iterate_runs_dep(color, direction):
    def __call__(image):
        warn_deprecated("""iterate_%s_%s_runs() is deprecated.

Reason: Functions parameterized by arguments, not by name.

Use iterate_runs('%s', '%s') instead.""" %
                        (color, direction, color, direction))
        return _runlength.iterate_runs(image, color, direction)
    return staticmethod(__call__)

class iterate_black_horizontal_runs(RunIterator):
    """Returns nested iterators over the black horizontal runs in the image.

Each run is returned as a Rect object.

For example, to iterate over all runs:

.. code:: Python

  for row in image.iterate_black_horizontal_runs():
     # All the runs in each row
     for run in row:
         print run

.. warning::

  iterate_black_horizontal_runs() is deprecated.

  Reason: Functions parameterized by arguments, not by name.

  Use iterate_runs('black', 'horizontal') instead.    
"""
    __call__ = iterate_runs_dep("black", "horizontal")

class iterate_black_vertical_runs(RunIterator):
    """Returns nested iterators over the black vertical runs in the image.

Each run is returned as a Rect object.

See iterate_black_horizontal_runs_ for a usage example.

.. warning::

  iterate_black_vertical_runs() is deprecated.

  Reason: Functions parameterized by arguments, not by name.

  Use iterate_runs('black', 'vertical') instead.    
"""
    __call__ = iterate_runs_dep("black", "vertical")

class iterate_white_horizontal_runs(RunIterator):
    """Returns nested iterators over the white horizontal runs in the image.

Each run is returned as a Rect object.

See iterate_black_horizontal_runs_ for a usage example.

.. warning::

  iterate_white_horizontal_runs() is deprecated.

  Reason: Functions parameterized by arguments, not by name.

  Use iterate_runs('white', 'horizontal') instead.    
"""
    __call__ = iterate_runs_dep("white", "horizontal")

class iterate_white_vertical_runs(RunIterator):
    """Returns nested iterators over the white vertical runs in the image.

Each run is returned as a Rect object.

See iterate_black_horizontal_runs_ for a usage example.

.. warning::

  iterate_white_vertical_runs() is deprecated.

  Reason: Functions parameterized by arguments, not by name.

  Use iterate_runs('white', 'vertical') instead.    
"""
    __call__ = iterate_runs_dep("white", "vertical")

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

    # Deprecated functions below
    functions += [most_frequent_black_horizontal_run,
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
                 filter_narrow_black_runs, filter_short_black_runs,
                 filter_tall_black_runs,filter_wide_black_runs,
                 filter_narrow_white_runs, filter_short_white_runs,
                 filter_tall_white_runs,filter_wide_white_runs,
                 iterate_black_horizontal_runs,
                 iterate_black_vertical_runs,
                 iterate_white_horizontal_runs,
                 iterate_white_vertical_runs]
    author = "Michael Droettboom and Karl MacMillan"
    url = "http://gamera.dkc.jhu.edu/"

module = RunLengthModule()
                 
del FrequentRun
del FrequentRuns
del RunHistogram
del FilterRuns
del RunIterator
del FilterRunsDep
