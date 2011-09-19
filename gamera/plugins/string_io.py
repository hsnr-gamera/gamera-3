#
# Copyright (C) 2005 Alex Cobb
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

"""Transfer of Gamera image data in Python strings. 
"""

from gamera.plugin import *

class _to_raw_string(PluginFunction):
    """
    Returns the image's binary data as a Python string.

    Requires a copying operation;  may fail for very large images.

    This function is not intended to be used directly.  To move data
    to/from Numeric/numarray/PIL, use the functions in numeric_io.py,
    numarray_io.py and pil_io.py respectively.
    """
    self_type = ImageType(ALL)
    return_type = Class("string_from_image")

class _from_raw_string(PluginFunction):
    """
    Instantiates an image from binary data in a Python string.

    Requires a copying operation;  may fail for very large images.
    
    This function is not intended to be used directly.  To move data
    to/from Numeric/numarray/PIL, use the functions in numeric_io.py,
    numarray_io.py and pil_io.py respectively.
    """
    self_type = None
    ## Image constructor uses:
    ##   page_offset_y, page_offset_x,
    ##   nrows, ncols, pixel_format=0, storage_type=0,
    ## so we'll do something similar:
    args = Args([Point("offset"), Dim("dim"),
                 Int("pixel_type"), Int("storage_type"),
                 Class("data_string")])
    return_type = ImageType(ALL)

class StringIOModule(PluginModule):
    category = "ExternalLibraries"
    cpp_headers=["string_io.hpp"]
    functions = [_to_raw_string,
                 _from_raw_string]
    author = "Alex Cobb"
    url = ('http://www.oeb.harvard.edu/faculty/holbrook/'
           'people/alex/Website/alex.htm')
module = StringIOModule()

_from_raw_string = _from_raw_string()
