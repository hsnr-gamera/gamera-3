# -*- mode: python; indent-tabs-mode: nil; tab-width: 3 -*-
# vim: set tabstop=3 shiftwidth=3 expandtab:
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
import _misc_filters

class outline(PluginFunction):
    """Traces the outline of the image.  This result is obtained by
dilating the image and then XOR'ing the result with the original."""
    self_type = ImageType([ONEBIT])
    return_type = ImageType([ONEBIT])
    doc_examples = [(ONEBIT,)]

class create_gabor_filter(PluginFunction):
    """Computes the convolution of an image with a two dimensional
Gabor function. The result is returned as a float image.

The *orientation* is given in radians, the other parameters are the
center *frequency* (for example 0.375 or smaller) and the two
angular and radial sigmas of the gabor filter.

The energy of the filter is explicitely normalized to 1.0.
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
    

class MiscFiltersModule(PluginModule):
    category = "Filter"
    functions = [outline,create_gabor_filter]
    cpp_headers = ["misc_filters.hpp"]
    author = "Michael Droettboom and Karl MacMillan"
    url = "http://gamera.dkc.jhu.edu/"
module = MiscFiltersModule()
