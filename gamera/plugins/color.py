#
#
# Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, Karl MacMillan
#               2010      Christoph Dalitz, Tobias Bolten, Oliver Christen
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

import _color

class ExtractFloatChannel(PluginFunction):
    self_type = ImageType([RGB])
    return_type = ImageType([FLOAT])
    doc_examples = [(RGB,)]

class ExtractGreyscaleChannel(PluginFunction):
    self_type = ImageType([RGB])
    return_type = ImageType([FLOAT])
    doc_examples = [(RGB,)]

class hue(ExtractFloatChannel):
    """
    Returns a FLOAT image where each pixel is a hue value in HSV space
    in range [0, 1).  Since the hue space is continuous, the shortest
    *distance* between 1 and 0 is 0.
    """
    pass

class saturation(ExtractFloatChannel):
    """
    Returns a FLOAT image where each pixel is a saturation value in
    HSV space in range [0, 1).
    """
    pass

class value(ExtractFloatChannel):
    """
    Returns a FLOAT image where each pixel is the value in
    HSV space in range [0, 1).  For more information, Google for HSV color
    space.
    """
    pass

class cie_x(ExtractFloatChannel):
    """
    Returns a FLOAT image where each pixel is a *x* value in the `CIE
    1964 Colorimetric`__ system in range [0, 1).

    .. __: http://www.isc.tamu.edu/~astro/color/cie_xyz1964.html
    """
    pass

class cie_y(ExtractFloatChannel):
    """
    Returns a FLOAT image where each pixel is a *y* value in the `CIE
    1964 Colorimetric`__ system in range [0, 1).

    .. __: http://www.isc.tamu.edu/~astro/color/cie_xyz1964.html
    """
    pass

class cie_z(ExtractFloatChannel):
    """
    Returns a FLOAT image where each pixel is a *z* value in the `CIE
    1964 Colorimetric`__ system in range [0, 1).

    .. __: http://www.isc.tamu.edu/~astro/color/cie_xyz1964.html
    """
    pass

class cie_Lab_L(ExtractFloatChannel):
    """
    Returns a FLOAT image where each pixel is an *L* value in the 
    CIE L*a*b* color space. For an introduction to the different color
    spaces, see A. Ford and A. Roberts: `Color Space Concersions`__ (1998).

    The present conversion uses the RGB to Lab conversion routine from VIGRA.

    .. __: http://www.poynton.com/PDFs/coloureq.pdf
    """
    pass

class cie_Lab_a(ExtractFloatChannel):
    """
    Returns a FLOAT image where each pixel is an *a* value in the 
    CIE L*a*b* color space. For an introduction to the different color
    spaces, see A. Ford and A. Roberts: `Color Space Concersions`__ (1998).

    The present conversion uses the RGB to Lab conversion routine from VIGRA.

    .. __: http://www.poynton.com/PDFs/coloureq.pdf
    """
    pass

class cie_Lab_b(ExtractFloatChannel):
    """
    Returns a FLOAT image where each pixel is a *b* value in the 
    CIE L*a*b* color space. For an introduction to the different color
    spaces, see A. Ford and A. Roberts: `Color Space Concersions`__ (1998).

    The present conversion uses the RGB to Lab conversion routine from VIGRA.

    .. __: http://www.poynton.com/PDFs/coloureq.pdf
    """
    pass

class cyan(ExtractGreyscaleChannel):
    """
    Returns a GREYSCALE image where each pixel is the cyan component
    of the RGB original.
    """
    pass

class magenta(ExtractGreyscaleChannel):
    """
    Returns a GREYSCALE image where each pixel is the magenta
    component of the RGB original.
    """
    pass

class yellow(ExtractGreyscaleChannel):
    """
    Returns a GREYSCALE image where each pixel is the yellow component
    of the RGB original.
    """
    pass

class red(ExtractGreyscaleChannel):
    """
    Returns a GREYSCALE image where each pixel is the red component of
    the RGB original.
    """
    pass

class green(ExtractGreyscaleChannel):
    """
    Returns a GREYSCALE image where each pixel is the green component
    of the original.
    """
    pass

class blue(ExtractGreyscaleChannel):
    """
    Returns a GREYSCALE image where each pixel is the blue component
    of the RGB original.
    """
    pass

class false_color(PluginFunction):
    """
    Returns a false color representation of the given image.  Low
    values are red, mid values are green and high values are blue.
    This can help visualize greyscale images that are not *real*
    images but are representations of other kinds of data.
    """
    self_type = ImageType([FLOAT, GREYSCALE])
    return_type = ImageType([RGB], "false_color")
    doc_examples = [(GREYSCALE,)]

class colors_to_labels(PluginFunction):
    """
    Converts an RGB image to a labeled onebit image.

    Each RGB color is replaced by the label specified in the mapping
    *rgb_to_label*. RGB values not listed in *rgb_to_label* are white
    in the returned onebit image. When no mapping *rgb_to_label* is
    provided, each different RGB color is replaced by a unique label.

    This is mostly useful for reading manually labeled groundtruth
    data from color PNG files. Example:

    .. code:: Python

      # map red to label 3, and green to label 5
      labeled = rgb.colors_to_labels( {RGBPixel(255,0,0): 3, RGBPixel(0,255,0): 5} )

    A typical use case of this plugin is in combination
    with ccs_from_labeled_image_.

    .. _ccs_from_labeled_image: utility.html#ccs-from-labeled-image
    """
    self_type = ImageType([RGB])
    return_type = ImageType([ONEBIT])
    args = Args([Class("rgb_to_label", dict)])
    author = "Christoph Dalitz and Hasan Yildiz"
    def __call__(self, dict=None):
      return _color.colors_to_labels(self, dict)
    __call__ = staticmethod(__call__)


class graph_color_ccs(PluginFunction):
    """
    Returns an RGB Image where each segment is colored with one of the colors
    from *colors* with the constraint that segments adjacent in the 
    neighborship graph have different colors.

    This function can be used to verify that the pagesegmentation 
    e.g. ``cc_analysis`` is working correctly for your image.

    The graph coloring algorithm is based on the "6-COLOR" alorithm for
    planar graphs, as described in:

        D. Matula, Y. Shiloach, R. Tarjan:
        `Two linear-time algorithms for five-coloring a planar graph.`__
        Tech Rep STAN-CS-80-830, Computer Science Dep., Stanford Univ., 
        Stanford, Calif., 1980

.. __: ftp://db.stanford.edu/pub/cstr/reports/cs/tr/80/830/CS-TR-80-830.pdf

    We have modified the algorithm in such way that the color distribution is
    balanced, i.e. that each color is assigned approximately to the same
    number of nodes (also known as \"equitable coloring\").

    *ccs*:
        ImageList which contains ccs to be colored. Must be views on
        the image an which this method is called.

    *colors*:
        list of colors (instances of RGBPixel) which will be used for coloring

    *method*:
        Controls the calculation of the neighborhood graph:
                0 = from the CC center points
                1 = from a 20 percent sample of the contour points
                2 = from the exact area Voronoi diagram

    .. code:: Python

       ccs = imgage.cc_analysis()
       colors = [ RGBPixel(180, 0, 0),
                  RGBPixel(0, 255, 0),
                  RGBPixel(0, 0, 255),
                  RGBPixel(255, 200, 20),
                  RGBPixel(255, 0, 255),
                  RGBPixel(50, 150, 50) ]
       rgb = imgage.mycolor_ccs(ccs, colors, 1)

    .. note:: *colors* may not contain less than six colors.

    """
    author = "Oliver Christen and Tobias Bolten"
    args = Args([ImageList('ccs'), Class('colors'), Choice('method', ["CC center", "20% contour points", "voronoi diagram"], default=1)])
    category = "Coloring"
    self_type = ImageType([ONEBIT])
    return_type = ImageType([RGB])

    def __call__(image, ccs, colors, method=1):
        return _color.graph_color_ccs(image, ccs, colors, method)
    __call__ = staticmethod(__call__)


class ColorModule(PluginModule):
    category = "Color"
    cpp_headers = ["color.hpp"]
    cpp_sources = ["src/geostructs/colorgraph.cpp", "src/geostructs/delaunaytree.cpp"]
    functions = [hue, saturation, value, cyan, magenta, yellow,
                 cie_x, cie_y, cie_z, cie_Lab_L, cie_Lab_a, cie_Lab_b,
                 red, green, blue, false_color,
                 colors_to_labels, graph_color_ccs]
    author = "Michael Droettboom and Karl MacMillan"
    url = "http://gamera.sourceforge.net/"

module = ColorModule()

del ExtractFloatChannel
del ExtractGreyscaleChannel
