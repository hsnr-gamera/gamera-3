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

class ExtractFloatChannel(PluginFunction):
    self_type = ImageType([RGB])
    return_type = ImageType([FLOAT])
    doc_examples = [(RGB,)]

class ExtractGreyscaleChannel(PluginFunction):
    self_type = ImageType([RGB])
    return_type = ImageType([FLOAT])
    doc_examples = [(RGB,)]

class hue(ExtractFloatChannel):
    """Returns a FLOAT image where each pixel is a hue value in range [0, 1).
Since the hue space is continuous, the shortest *distance* between 1 and 0 is
0.  For more information, Google HSV color space."""
    pass

class saturation(ExtractFloatChannel):
    """Returns a FLOAT image where each pixel is a saturation value
in range [0, 1).  For more information, Google for HSV color space."""
    pass

class value(ExtractFloatChannel):
    """Returns a FLOAT image where each pixel is a saturation value
in range [0, 1).  For more information, Google for HSV color space."""
    pass

class cie_x(ExtractFloatChannel):
    """Returns a FLOAT image where each pixel is a *x* value in the `CIE 1964 Colorimetric`__ system 
in range [0, 1).

.. __: http://www.isc.tamu.edu/~astro/color/cie_xyz1964.html
"""
    pass

class cie_y(ExtractFloatChannel):
    """Returns a FLOAT image where each pixel is a *y* value in the `CIE 1964
Colorimetric`__ system 
in range [0, 1).

.. __: http://www.isc.tamu.edu/~astro/color/cie_xyz1964.html
"""
    pass

class cie_z(ExtractFloatChannel):
    """Returns a FLOAT image where each pixel is a *z* value in the `CIE 1964
Colorimetric`__ system 
in range [0, 1).

.. __: http://www.isc.tamu.edu/~astro/color/cie_xyz1964.html
"""
    pass

class cyan(ExtractGreyscaleChannel):
    """Returns a GREYSCALE image where each pixel is the cyan component of the
original.
"""
    pass

class magenta(ExtractGreyscaleChannel):
    """Returns a GREYSCALE image where each pixel is the magenta component of the
original.
"""
    pass

class yellow(ExtractGreyscaleChannel):
    """Returns a GREYSCALE image where each pixel is the yellow component of the
original.
"""
    pass

class red(ExtractGreyscaleChannel):
    """Returns a GREYSCALE image where each pixel is the red component of the
original.
"""
    pass

class green(ExtractGreyscaleChannel):
    """Returns a GREYSCALE image where each pixel is the green component of the
original.
"""
    pass

class blue(ExtractGreyscaleChannel):
    """Returns a GREYSCALE image where each pixel is the blue component of the
original.
"""
    pass

class false_color(PluginFunction):
    """Returns a false color representation of the given image.  Low values
are red, mid values are green and high values are blue.  This can help visualize
greyscale images that are not *real* images but are representations of other
kinds of data.
"""
    self_type = ImageType([FLOAT, GREYSCALE])
    return_type = ImageType([RGB], "false_color")
    doc_examples = [(GREYSCALE,)]

class ColorModule(PluginModule):
    category = "Color"
    cpp_headers=["color.hpp"]
    functions = [hue, saturation, value, cyan, magenta, yellow,
                 cie_x, cie_y, cie_z, red, green, blue, false_color]
    author = "Michael Droettboom and Karl MacMillan"
    url = "http://gamera.dkc.jhu.edu/"

module = ColorModule()

del ExtractFloatChannel
del ExtractGreyscaleChannel
