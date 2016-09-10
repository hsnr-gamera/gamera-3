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
"""Data transfer between Gamera and Numeric.
"""

from gamera.plugin import *
from gamera.util import warn_deprecated
from gamera import config

try:
    import numpy.oldnumeric as n
except ImportError:
    try:
        verbose = config.get("verbosity_level")
    except Exception:
        verbose = 0
    if verbose:
        print ('Info: numpy.oldumeric could not be imported.')
else:
    _typecodes = {RGB       : n.UInt8,
                  GREYSCALE : n.UInt8,
                  GREY16    : n.UInt32,
                  ONEBIT    : n.UInt16,
                  FLOAT     : n.Float64,
                  COMPLEX   : n.Complex64 }
    _inverse_typecodes = { n.UInt8     : GREYSCALE,
                           n.UInt32    : GREY16,
                           n.UInt16    : ONEBIT,
                           n.Float64   : FLOAT,
                           n.Complex64 : COMPLEX } 
        
    class from_numeric(PluginFunction):
        """
        Instantiates a Gamera image from a Numeric multi-dimensional
        array *array*.
            
        The array must be one of the following types and will map to
        the corresponding Gamera image type:

        +------------+------------------+
        | Gamera     | Numeric          |
        | type       | type             |
        +============+==================+
        | RGB        | UInt8 (on 3      |
        |            | planes)          |
        +------------+------------------+
        | GREYSCALE  | UInt8            |
        +------------+------------------+
        | GREY16     | UInt32           |
        +------------+------------------+
        | ONEBIT     | UInt16           |
        +------------+------------------+
        | FLOAT      | Float64          |
        +------------+------------------+
        | COMPLEX    | Complex64        |
        +------------+------------------+

        Requires two copying operations;  may fail for very large images.

        To use this function, which is not a method on images, do the
        following:

        .. code:: Python
        
          from gamera.plugins import numeric_io
          image = numeric_io.from_numeric(array)
        """
        self_type = None
        args = Args([Class("array")])
        return_type = ImageType(ALL)
        pure_python = True
        def __call__(array, offset=(0, 0)):
            from gamera.plugins import _string_io
            from gamera.core import Dim
            pixel_type = from_numeric._check_input(array)
            return _string_io._from_raw_string(
                offset,
                Dim(array.shape[1], array.shape[0]),
                pixel_type, DENSE,
                array.tostring())
        __call__ = staticmethod(__call__)

        def _check_input(array):

            shape = array.shape
            typecode = array.dtype.char
            if len(shape) == 3 and shape[2] == 3 and typecode == n.UInt8:
                return RGB
            elif len(shape) == 2:
                if _inverse_typecodes.has_key(typecode):
                    return _inverse_typecodes[typecode]
            raise ValueError('Array is not one of the acceptable types (UInt8 * 3, UInt8, UInt16, UInt32, Float64, Complex64)')
        _check_input = staticmethod(_check_input)

    class to_numeric(PluginFunction):
        """
        Returns an ``Numeric`` array containing a copy of the image's data.

        The array will be one of the following types corresponding to
        each of the Gamera image types:

        +------------+-----------------+
        | Gamera     | Numeric         |
        | type       | type            |
        +============+=================+
        | RGB        | UInt8 (on 3     |
        |            | planes)         |
        +------------+-----------------+
        | GREYSCALE  | UInt8           |
        +------------+-----------------+
        | GREY16     | UInt32          |
        +------------+-----------------+
        | ONEBIT     | UInt16          |
        +------------+-----------------+
        | FLOAT      | Float64         |
        +------------+-----------------+
        | COMPLEX    | Complex64       |
        +------------+-----------------+

        Requires *three* copies, and may fail for very large images.
        """
        self_type = ImageType(ALL)
        return_type = Class("array")
        pure_python = True
        def __call__(image):
            from gamera.plugins import _string_io
            pixel_type = image.data.pixel_type
            shape = (image.nrows, image.ncols)
            typecode = _typecodes[pixel_type]
            if pixel_type == RGB:
                shape += (3,)
            array = n.fromstring(_string_io._to_raw_string(image), typecode)
            return n.resize(array, shape)
        __call__ = staticmethod(__call__)

        def __doc_example1__(images):
            image = images[RGB]
            array = image.to_numeric()
            image0 = from_numeric(array)
            return [image, image0]
        def __doc_example2__(images):
            image = images[GREYSCALE]
            array = image.to_numeric()
            image0 = from_numeric(array)
            return [image, image0]
        def __doc_example4__(images):
            image = images[ONEBIT]
            array = image.to_numeric()
            image0 = from_numeric(array)
            return [image, image0]
        def __doc_example5__(images):
            image = images[FLOAT]
            array = image.to_numeric()
            image0 = from_numeric(array)
            return [image, image0]
        def __doc_example6__(images):
            image = images[COMPLEX]
            array = image.to_numeric()
            image0 = from_numeric(array)
            return [image, image0]
        doc_examples = [__doc_example1__,
                        __doc_example2__,
                        __doc_example4__,
                        __doc_example5__,
                        __doc_example6__]

    class NumericModule(PluginModule):
        category = None #"ExternalLibraries/Numeric"
        author = "Alex Cobb"
        functions = [from_numeric, to_numeric]
        pure_python = True
        url = ('http://www.oeb.harvard.edu/faculty/holbrook/'
               'people/alex/Website/alex.htm')
    module = NumericModule()

    from_numeric = from_numeric()
