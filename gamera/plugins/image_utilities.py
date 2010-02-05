#
#
# Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
#               2008-2010 Christoph Dalitz
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

"""The image utilities module contains plugins that do not fit in
any other category, like image copying or computing histograms."""

from gamera.plugin import * 
from gamera.gui import has_gui
from gamera.util import warn_deprecated
import sys
import _image_utilities 

class image_copy(PluginFunction):
    """
    Copies an image along with all of its underlying data.  Since the data is
    copied, changes to the new image do not affect the original image.

    *storage_format*
      specifies the compression type for the returned copy:

    DENSE (0)
      no compression
    RLE (1)
      run-length encoding compression
    """
    category = "Utility"
    self_type = ImageType(ALL)
    return_type = ImageType(ALL)
    args = Args([Choice("storage_format", ["DENSE", "RLE"])])
    def __call__(image, storage_format = 0):
        if image.nrows <= 0 or image.ncols <= 0:
            return image
        return _image_utilities.image_copy(image, storage_format)
    __call__ = staticmethod(__call__)

class image_save(PluginFunction):
    """
    Saves an image to file with specified name and format.
    """
    pure_python = 1
    category = "Utility"
    self_type = ImageType(ALL)
    args = Args([FileSave("image_file_name", "", "*"),
                 Choice("File format", ["TIFF", "PNG"])
                 ])
    def __call__(image, name, format):
        if format == 0 or format.upper() == "TIFF":
            try:
                from gamera.plugins import tiff_support
            except ImportError:
                raise ImportError("Could not load TIFF support.")
            image.save_tiff(name)
        elif format == 1 or format.upper() == "PNG":
            try:
                from gamera.plugins import png_support
            except ImportError:
                raise ImportError("Could not load PNG support.")
            image.save_PNG(name)
    __call__ = staticmethod(__call__)

class histogram(PluginFunction):
    """
    Compute the histogram of the pixel values in the given image.
    Returns a Python array of doubles, with each value being a
    percentage.

    If the GUI is being used, the histogram is displayed.

    .. image:: images/histogram.png
    """
    category = "Analysis"
    self_type = ImageType([GREYSCALE, GREY16])
    return_type = FloatVector()
    doc_examples = [(GREYSCALE,)]
    def __call__(image):
        hist = _image_utilities.histogram(image)
        if has_gui.has_gui == has_gui.WX_GUI:
            has_gui.gui.ShowHistogram(hist, mark=image.otsu_find_threshold())
        return hist
    __call__ = staticmethod(__call__)

class union_images(PluginFunction):
    """
    Returns an image that is the union of the given list of connected
    components.
    """
    category = "Combine"
    self_type = None
    args = Args([ImageList('list_of_images')])
    return_type = ImageType([ONEBIT])

class fill_white(PluginFunction):
    """
    Fills the entire image with white.
    """
    category = "Draw"
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])

class fill(PluginFunction):
    """
    Fills the entire image with value.

    *value*
      A pixel value.  This value may be any value the pixel type can support.
    """
    category = "Draw"
    self_type = ImageType(ALL)
    args = Args([Pixel("value")])

class pad_image_default(PluginFunction):
    # This is only for plugin generation, it will not be added to the image type
    # (since self_type == None)
    category = "Utility"
    self_type = None
    args = Args([ImageType(ALL), Int("top"), Int("right"), Int("bottom"), Int("left")])
    return_type = ImageType(ALL)

class pad_image(PluginFunction):
    """
    Pads an image with any value.

    *top*
      Padding on the top.

    *right*
      Padding on the right.

    *bottom*
      Padding on the bottom.

    *left*
      Padding on the left.

    *value*
      A pixel value.  This value may be any value the pixel type can support.

    """
    category = "Utility"
    self_type = ImageType(ALL)
    args = Args([Int("top"), Int("right"), Int("bottom"), Int("left"), Pixel("value")])
    return_type = ImageType(ALL)
    _pad_image_default = pad_image_default()
    def __call__(self, top, right, bottom, left, value=None):
        if value is None:
            return pad_image._pad_image_default(self, top, right, bottom, left)
    	return _image_utilities.pad_image(self, top, right, bottom, left, value)
    __call__ = staticmethod(__call__)
    doc_examples = [(RGB, 5, 10, 15, 20)]

class invert(PluginFunction):
    """
    Inverts the image.
    """
    category = "Draw"
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, RGB])
    doc_examples = [(RGB,), (GREYSCALE,), (ONEBIT,)]

class clip_image(PluginFunction):
    """
    Crops an image so that the bounding box includes only the
    intersection of it and another image.  Returns a zero-sized image
    if the two images do not intersect.
    """
    category = "Utility"
    return_type = ImageType(ALL)
    self_type = ImageType(ALL)
    args = Args(Rect("other"))

class mask(PluginFunction):
    """
    Masks an image using the given ONEBIT image.  Parts of the ONEBIT
    image that are white will be changed to white in the resulting
    image.

    The images must be the same size.
    """
    category = "Combine"
    return_type = ImageType([GREYSCALE, RGB])
    self_type = ImageType([GREYSCALE, RGB])
    args = Args(ImageType([ONEBIT], "mask"))

class to_nested_list(PluginFunction):
    """
    Converts an image to a nested Python list.
    This method is the inverse of ``nested_list_to_image``.

    The following table describes how each image type is converted to
    Python types:

      - ONEBIT -> int
      - GREYSCALE -> int
      - GREY16 -> int
      - RGB -> RGBPixel
      - FLOAT -> float

    NOTE: This will not scale very well and should only be used for
    small images, such as convolution kernels.
    """
    category = "Utility/NestedLists"
    self_type = ImageType(ALL)
    return_type = Class("nested_list")
    doc_examples = [(ONEBIT,)]

class nested_list_to_image(PluginFunction):
    """
    Converts a nested Python list to an Image.  Is the inverse of
    ``to_nested_list``.
    
    *nested_list*
      A nested Python list in row-major order.  If the list is a flat list,
      an image with a single row will be created.
  
    *image_type*
      The resulting image type.  Should be one of the integer Image type
      constants (ONEBIT, GREYSCALE, GREY16, RGB, FLOAT).  If image_type
      is not provided or less than 0, the image type will be determined
      by auto-detection from the list.  The following list shows the mapping
      from Python type to image type:

      - int -> GREYSCALE
      - float -> FLOAT
      - RGBPixel -> RGB

    To obtain other image types, the type number must be explicitly passed.

    NOTE: This will not scale very well and should only be used
    for small images, such as convolution kernels.

    Examples:

    .. code:: Python

      # Sobel kernel (implicitly will be a FLOAT image)
      kernel = nested_list_to_image([[0.125, 0.0, -0.125],
                                     [0.25 , 0.0, -0.25 ],
                                     [0.125, 0.0, -0.125]])

      # Single row image (note that nesting is optional)
      image = nested_list_to_image([RGBPixel(255, 0, 0),
                                    RGBPixel(0, 255, 0),
                                    RGBPixel(0, 0, 255)])

    """
    category = "Utility/NestedLists"
    self_type = None
    args = Args([Class('nested_list'),
                 Choice('image_type', ['ONEBIT', 'GREYSCALE',
                                       'GREY16', 'RGB', 'FLOAT'])])
    return_type = ImageType(ALL)
    def __call__(l, t=-1):
        return _image_utilities.nested_list_to_image(l, t)
    __call__ = staticmethod(__call__)

class diff_images(PluginFunction):
    """
    Returns a color image representing the difference of two images
    following the conventions of a number of Unix diff visualization
    tools, such as CVS web.  Pixels in both images are black.  Pixels
    in 'self' but not in the given image (\"deleted\" pixels) are red.
    Pixels in the given image but not in self (\"inserted\" pixels)
    are green.
    """
    category = "Combine"
    self_type = ImageType(ONEBIT)
    args = Args([ImageType(ONEBIT, 'other')])
    return_type = ImageType(RGB)
    pure_python = True
    def __call__(self, other):
        from gamera.core import RGBPixel
        result = self.to_rgb()
        diff = self.subtract_images(other)
        result.highlight(diff, RGBPixel(255, 64, 64))
        diff = other.subtract_images(self)
        result.highlight(diff, RGBPixel(64, 255, 64))
        return result
    __call__ = staticmethod(__call__)

class mse(PluginFunction):
    """
    Calculates the mean square error between two images.
    """
    category = "Utility"
    self_type = ImageType([RGB])
    args = Args([ImageType([RGB])])
    return_type = Float()

class reset_onebit_image(PluginFunction):
    """
    Resets all black pixel values in a onebit image to one.  This
    can be necessary e.g. after a CC analysis which sets black
    pixels to some other label value.
    """
    category="Utility"
    self_type = ImageType([ONEBIT])
    author = "Christoph Dalitz"

class UtilModule(PluginModule):
    cpp_headers=["image_utilities.hpp"]
    category = "Utility"
    functions = [image_save, image_copy,
                 histogram, union_images,
                 fill_white, fill, pad_image, pad_image_default,
		 invert, clip_image, mask,
                 nested_list_to_image, to_nested_list,
                 diff_images, mse, reset_onebit_image]
    author = "Michael Droettboom and Karl MacMillan"
    url = "http://gamera.sourceforge.net/"
module = UtilModule()

union_images = union_images()
nested_list_to_image = nested_list_to_image()

del pad_image_default
