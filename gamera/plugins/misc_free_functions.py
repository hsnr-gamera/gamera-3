#
# Copyright (C) 2010      Christoph Dalitz
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

"""This module contains miscellanous free functions that are no image methods, but are used at different places in the gamera core."""

from gamera.plugin import *
import _misc_free_functions

class range_of_float(PluginFunction):
    """Returns a tuple ``(float_min, float_max)`` containing the range
of a float variable on the used operating system, as given by
``std::numeric_limits<float>``.

This should be the same as ``sys.float_info.min`` and ``sys.float_info.max``,
but these have been introduced into python as late as 2.6, so we need an own
implementation for backward compatibility.
"""
    category = None
    self_type = None
    return_type = Class("min_max")
    author = "Christoph Dalitz"
    def __call__():
        return _misc_free_functions.range_of_float()
    __call__ = staticmethod(__call__)

class MiscFreeFunctionsModule(PluginModule):
   category = None
   cpp_headers=["misc_free_functions.hpp"]
   functions = [range_of_float]
   author = "Christoph Dalitz"
   url = "http://gamera.sourceforge.net/"
module = MiscFreeFunctionsModule()

range_of_float = range_of_float()
