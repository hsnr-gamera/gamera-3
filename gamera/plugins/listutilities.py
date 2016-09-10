#
# Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, Karl MacMillan
#               2014      Fabian Schmitt
#               2009-2014 Christoph Dalitz
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#

from gamera.plugin import *
import _listutilities

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
   category = "List"
   self_type = None
   args = Args([Class("list")])
   return_type = Int("continuaton")

class all_subsets(PluginFunction):
   """Returns all subsets of size *size* of the given list."""
   category = "List"
   self_type = None
   args = Args([Class("list"), Int("size")])
   return_type = Class("subsets")

class median(PluginFunction):
    """Compute the median from a list of values in linear time.

This implementation works both with builtin numeric types like *int* or
*float*, and with user defined types. For user defined type, you
must implement the "less than" operator (`__lt__`), as in the following
example:

.. code:: Python

   class P:
      x = 0; y = 0
      def __init__(self, x, y):
          self.x = x; self.y = y
      def __lt__(self, other):
          return (self.x < other.x)
      def __eq__(self, other):
          return (self.x == other.x)

   a = [P(0,0), P(1,1), P(2,0)]
   p = median(a)

When the parameter *inlist* is ``True``, the median is always a list entry,
even for lists of even size. Otherwise, the median is for an
even size list of *int* or *float* values the mean between the two middle
values. So if you need the median for a pivot element, set *inlist* to
``True``.

For user defined types, the returned median is always a list
entry, because arithmetic computations do not make sense in this case.

.. note::

   This is *not* the median image filter that replaces each pixel value
   with the median of its neighborhood. For this purpose, see the
   rank__ plugin.

.. __: morphology.html#rank
"""
    category = "List"
    pure_python = 1
    self_type = None
    return_type = Class("m")
    args = Args([Class("list"),
                 Check("inlist", check_box="always from list", default=False)])
    author = "Christoph Dalitz"
    def __call__(list, inlist=False):
        return _listutilities.median_py(list, inlist)
    __call__ = staticmethod(__call__)

class median_py(PluginFunction):
    """This is only for Gamera's Python-C++ interface."""
    category = None
    self_type = None
    return_type = Class("m")
    args = Args([Class("list"), Check("inlist")])
    author = "Christoph Dalitz"

class kernel_density(PluginFunction):
    """Computes the kernel density for *values* at the specified
*x*-positions. Reference: S.J. Sheather: \"Density Estimation.\"
Statistical Science 19 pp. 588-597 (2004).

Arguments:

  *values*
     Sample values from which the density is to be estimated.

  *x*
     For each value in *x*, the density at this position is returned
     in the returned float vector.

  *bw*
     Band width, i.e. the parameter *h* in the kernel density estimator.
     when set to zero, Silverman's rule of thumb is used, which sets the
     bandwidth to 0.9 min{sigma, iqr/1.34} n^(-1/5).

  *kernel*
     The kernel function that weights the values (0 = rectangular, 
     1 = triangular, 2 = Gaussian). A Gaussian kernel produces the smoothest
     result, but is slightly slower than the other two.

     Note that the kernels are normalized to variance one, which means that
     the rectangular kernel has support [-sqrt(3), +sqrt(3)], and the
     triangular kernel has support [-sqrt(6), sqrt(6)].
"""
    category = "List"
    self_type = None
    return_type = FloatVector()
    args = Args([FloatVector("values"), FloatVector("x"), Float("bw", default=0.0), Choice("kernel", choices=["rectangular", "triangular", "gaussian"], default=0)])
    author = "Christoph Dalitz and Fabian Schmitt"
    def __call__(values, x, bw=0.0, kernel=0):
        return _listutilities.kernel_density(values, x, bw, kernel)
    __call__ = staticmethod(__call__)


class argmax(PluginFunction):
    """Returns the index of the maximum in a list.
"""
    category = "List"
    pure_python = 1
    self_type = None
    return_type = Int()
    args = Args([FloatVector("x")])
    author = "Christoph Dalitz"
    def __call__(x):
        mi = 0
        mv = x[0]
        for i,v in enumerate(x):
            if v > mv:
                mi = i; mv = v
        return mi
    __call__ = staticmethod(__call__)

class argmin(PluginFunction):
    """Returns the index of the minimum in a list.
"""
    category = "List"
    pure_python = 1
    self_type = None
    return_type = Int()
    args = Args([FloatVector("x")])
    author = "Christoph Dalitz"
    def __call__(x):
        mi = 0
        mv = x[0]
        for i,v in enumerate(x):
            if v < mv:
                mi = i; mv = v
        return mi
    __call__ = staticmethod(__call__)

class ListUtilitiesModule(PluginModule):
   category = None
   cpp_headers=["listutilities.hpp"]
   functions = [permute_list, all_subsets, median, median_py,
                kernel_density, argmax, argmin]
   author = "Michael Droettboom and Karl MacMillan"
   url = "http://gamera.sourceforge.net/"
module = ListUtilitiesModule()

permute_list = permute_list()
all_subsets = all_subsets()
median = median()
kernel_density = kernel_density()
argmax = argmax()
argmin = argmin()
