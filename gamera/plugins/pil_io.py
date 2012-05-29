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
"""Data transfer between Gamera and the Python Imaging Library.
"""

from gamera.plugin import *
from gamera import config

try:
    #import Image as PIL
    from PIL import Image as PIL
except ImportError:
    try:
        verbose = config.get("verbosity_level")
    except:
        verbose = 0
    if verbose:
        print ('Info: Python Imaging Library module Image '
               'could not be imported')
else:

    _modes = {RGB       : 'RGB',
              GREYSCALE : 'L'}
    _inverse_modes = {'RGB': RGB,
                      'L'  : GREYSCALE}
    ## also boolean mode '1' --- I think this is 8 bits per pixel

    class from_pil(PluginFunction):
        """
        Instantiates a Gamera image from a Python Imaging Library
        image *image*.

        Only RGB or 8-bit greyscale mode PIL images are supported.
        Requires a copying operation;  may fail for very large images.

        This can, e.g., be used to read images from file formats not
        directly supported by gamera, as JPEG images:

        .. code:: Python

          # Beware: name "Image" is already used in Gamera!
          import Image as Pil
          from pil_io import

          # read a JPEG image and convert it to a Gamera image
          pilimg = Pil.open("image.jpg")
          img = from_pil(pilimg)
        """
        self_type = None
        return_type = ImageType([GREYSCALE, RGB, FLOAT])
        args = Args([Class("image")])
        pure_python = True

        def __call__(image, offset=None):
            from gamera.plugins import _string_io
            from gamera.core import Dim, Point
            typecode = image.mode
            if offset is None:
                offset = Point(0, 0)
            if _inverse_modes.has_key(typecode):
                pixel_type = _inverse_modes[typecode]
            else:
                raise ValueError("Only RGB and 8-bit Greyscale 'L' PIL image modes are supported.")
            return _string_io._from_raw_string(
                offset,
                Dim(image.size[0], image.size[1]),
                pixel_type, DENSE,
                image.tostring())
        __call__ = staticmethod(__call__)

    class to_pil(PluginFunction):
        """
        Returns a Python Imaging Library image containing a copy of
        image's data.

        Only RGB and Greyscale images are supported.
        May fail for very large images.
        """
        self_type = ImageType([RGB, GREYSCALE])
        return_type = Class("pil_image")
        def __call__(image):
            from gamera.plugins import _string_io
            pixel_type = image.data.pixel_type
            if _modes.has_key(pixel_type):
                mode = _modes[pixel_type]
            else:
                raise ValueError("Only RGB and GREYSCALE Images are supported.")
            size = (image.ncols, image.nrows)
            return PIL.fromstring(mode, size,
                                  _string_io._to_raw_string(image))
        __call__ = staticmethod(__call__)

        def __doc_example1__(images):
            image = images[RGB]
            array = image.to_pil()
            image0 = from_pil(array)
            return [image, image0]
        def __doc_example2__(images):
            image = images[GREYSCALE]
            array = image.to_pil()
            image0 = from_pil(array)
            return [image, image0]
        doc_examples = [__doc_example1__,
                        __doc_example2__]

    class PilIOModule(PluginModule):
        category = "ExternalLibraries/PIL"
        author = "Alex Cobb"
        functions = [from_pil, to_pil]
        pure_python = True
        url = ('http://www.oeb.harvard.edu/faculty/holbrook/'
               'people/alex/Website/alex.htm')
    module = PilIOModule()

    from_pil = from_pil()
