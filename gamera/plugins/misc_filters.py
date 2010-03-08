# -*- mode: python; indent-tabs-mode: nil; tab-width: 3 -*-
# vim: set tabstop=3 shiftwidth=3 expandtab:
#
# Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, Karl MacMillan
#               2010      Christoph Dalitz
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
import _misc_filters

class rank(PluginFunction):
  """
  Within each 3x3 window, set the center pixel to the *n*-th ranked
  value.

  Note that for ``Onebit`` images, actually *rank(9 - r)* is computed instead
  of *rank(r)*. This has the effect that you do not need to worry whether
  your image is a greyscale or onebit image: in all cases low values
  for *r* will darken the image and high values will light it up.

  *rank* (1 - 9)
    The rank of the 9 pixels to select for the center.  5 is equivalent to
    the median.
  """
  self_type = ImageType([ONEBIT, GREYSCALE, FLOAT])
  args = Args([Int('rank', range=(1, 9))])
  return_type = ImageType([ONEBIT, GREYSCALE, FLOAT])
  doc_examples = [(GREYSCALE, 2), (GREYSCALE, 5), (GREYSCALE, 8)]

class mean(PluginFunction):
  """
  Within each 3x3 window, set the center pixel to the mean value of
  all 9 pixels.
  """
  self_type = ImageType([ONEBIT, GREYSCALE, FLOAT])
  doc_examples = [(GREYSCALE,)]
  return_type = ImageType([ONEBIT, GREYSCALE, FLOAT])

class create_gabor_filter(PluginFunction):
    """
    Computes the convolution of an image with a two dimensional Gabor
    function. The result is returned as a float image.

    The *orientation* is given in radians, the other parameters are
    the center *frequency* (for example 0.375 or smaller) and the two
    angular and radial sigmas of the gabor filter.
    
    The energy of the filter is explicitly normalized to 1.0.
    """
    self_type = ImageType([GREYSCALE])
    return_type = ImageType([FLOAT])
    args = Args([Float("orientation", default=45.0),
                 Float("frequency", default=0.375),
                 Int("direction", default=5)])
    author = u"Ullrich K\u00f6the (wrapped from VIGRA by Uma Kompella)"
    doc_examples = [(GREYSCALE,)]
    def __call__(self,
                 orientation=45.0,
                 frequency=0.375,
                 direction=5):
        return _misc_filters.create_gabor_filter(self,
                                                 orientation,
                                                 frequency,
                                                 direction)
    __call__ = staticmethod(__call__)
    

class kfill(PluginFunction):
    """
    Remove salt and pepper noise in onebit images by applying the *kfill*
    filter *iterations* times.

    In contrast to a rank or mean filter, kfill is designed in such
    a way that it does not merge non touching connected components.
    To this end, the border of the *k* times *k* mask is scanned for
    black pixels and the center is not filled when this would connect
    disjoint pixels on the border. A detailed description of the
    algorithm can be found in

       M. Seul, L. O'Gorman, M.J. Sammon: *Practical Algorithms for
       Image Analysis.* Cambridge University Press, 2000

    The present implementation does not use code from the book, but has
    been written form scratch.
    """
    self_type = ImageType([ONEBIT])
    return_type = ImageType([ONEBIT])
    author = "Oliver Christen"
    args = Args([Int("k", default=3),Int("iterations", default=1),])
    def __call__(self, k=3, iterations=1):
      if k < 3:
        raise RuntimeError("kfill: k must be >= 3")
      if iterations < 1:
        raise RuntimeError("kfill: number of iterations must be > 0")
      return _misc_filters.kfill(self, k, iterations)
    __call__ = staticmethod(__call__)


class MiscFiltersModule(PluginModule):
    category = "Filter"
    functions = [mean, rank, create_gabor_filter, kfill]
    cpp_headers = ["misc_filters.hpp"]
    author = "Michael Droettboom and Karl MacMillan"
    url = "http://gamera.sourceforge.net/"
module = MiscFiltersModule()
