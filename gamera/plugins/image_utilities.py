#
#
# Copyright (C) 2001-2004 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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

"""The image utilities module contains plugins for copy, rotating, resizing,
and computing histograms."""

from gamera.plugin import * 
from gamera.gui import has_gui
import _image_utilities 

class image_copy(PluginFunction):
    """Copies an image along with all of its underlying data.  Since the data is
copied, changes to the new image do not affect the original image.

*storage_format*
  specifies the compression type for the returned copy:

  DENSE (0)
    no compression
  RLE (1)
    run-length encoding compression"""
    category = "Utility"
    self_type = ImageType(ALL)
    return_type = ImageType(ALL)
    args = Args([Choice("storage_format", ["DENSE", "RLE"])])
    def __call__(image, storage_format = 0):
        return _image_utilities.image_copy(image, storage_format)
    __call__ = staticmethod(__call__)

class resize(PluginFunction):
    """Returns a resized copy of an image. In addition to size, the type of
interpolation can be specified, with a tradeoff between speed
and quality.

If you need to maintain the aspect ratio of the original image,
consider using scale_ instead.

*nrows*
   The height of the resulting image.
*ncols*
   The width of the resulting image.
*interp_type* [None|Linear|Spline]
   The type of interpolation used to resize the image.  Each option is
   progressively higher quality, yet slower.
"""
    category = "Utility"
    self_type = ImageType(ALL)
    args= Args([Int("nrows"), Int("ncols"),
                Choice("interp_type", ["None", "Linear", "Spline"])])
    return_type = ImageType(ALL)
    doc_examples = [(RGB, 96, 32, 3)]

class scale(PluginFunction):
    """Returns a scaled copy of the image. In addition to scale, the type of
interpolation can be specified, with a tradeoff between speed
and quality.

If you need to change the aspect ratio of the original image,
consider using resize_ instead.

*scale*
   A scaling factor.  Values greater than 1 will result in a larger image.
   Values less than 1 will result in a smaller image.
*interp_type* [None|Linear|Spline]
   The type of interpolation used to resize the image.  Each option is
   progressively higher quality, yet slower.
"""
    category = "Utility"
    self_type = ImageType(ALL)
    args= Args([Real("scaling"),
                Choice("interp_type", ["None", "Linear", "Spline"])])
    return_type = ImageType(ALL)
    doc_examples = [(RGB, 0.5, 3), (RGB, 2.0, 3)]

class histogram(PluginFunction):
    """Compute the histogram of the pixel values in the given image.
Returns a Python array of doubles, with each value being a percentage.

If the GUI is being used, the histogram is displayed.

.. image:: images/histogram.png"""
    category = "Analysis"
    self_type = ImageType([GREYSCALE, GREY16])
    return_type = FloatVector()
    def __call__(image):
        hist = _image_utilities.histogram(image)
        gui = has_gui.gui
        if gui:
            gui.ShowHistogram(hist, mark=image.otsu_find_threshold())
        return hist
    __call__ = staticmethod(__call__)

class union_images(PluginFunction):
    """Returns an image that is the union of the given list of connected components."""
    category = "Combine"
    self_type = None
    args = Args([ImageList('list_of_images')])
    return_type = ImageType([ONEBIT])

class fill_white(PluginFunction):
    """Fills the entire image with white."""
    category = "Draw"
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])

class invert(PluginFunction):
    """Inverts the image."""
    category = "Draw"
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, RGB])
    doc_examples = [(RGB,), (GREYSCALE,), (ONEBIT,)]

class clip_image(PluginFunction):
    """Crops an image so that the bounding box includes only the intersection of
it and another image.  Returns a zero-sized image if the two images do
not intersect."""
    category = "Utility"
    return_type = ImageType(ALL)
    self_type = ImageType(ALL)
    args = Args(Rect("other"))

class mask(PluginFunction):
    """Masks an image using the given ONEBIT image.  Parts of the ONEBIT
image that are white will be changed to white in the resulting image.

The images must be the same size.
"""
    category = "Combine"
    return_type = ImageType([GREYSCALE, RGB])
    self_type = ImageType([GREYSCALE, RGB])
    args = Args(ImageType([ONEBIT], "mask"))

class corelation(PluginFunction):
    """Returns a floating-point value for how well an image is corelated
to another image placed at a given (*x*, *y*).  Uses the sum of squares
method.  A greater result implies more corelation.

*template*
   The template image.
*y*, *x*
   The displacement of the template on the image.
"""
    category = "Utility"
    return_type = Float("correlation")
    self_type = ImageType([ONEBIT])
    args = Args([ImageType([ONEBIT], "template"), Int("y_offset"), Int("x_offset")])

class to_nested_list(PluginFunction):
    """Converts an image to a nested Python list.
This method is the inverse of ``nested_list_to_image``.

The following table describes how each image type is converted to
Python types:

  - ONEBIT -> int
  - GREYSCALE -> int
  - GREY16 -> int
  - RGB -> RGBPixel
  - FLOAT -> float

NOTE: This will not scale very well and should only be used
for small images, such as convolution kernels.
"""
    category = "Utility/NestedLists"
    self_type = ImageType(ALL)
    return_type = Class("nested_list")
    doc_examples = [(ONEBIT,)]

class nested_list_to_image(PluginFunction):
    """Converts a nested Python list to an Image.  Is the inverse of ``to_nested_list``.

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

TODO: Handle Python arrays.  This is difficult without a proper C API.
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

class shear_row(PluginFunction):
    """Shears a given row by a given amount.

*row*
   The row number to shear.

*distance*
   The number of pixels to move the row.  Positive values
   move the row to the right.  Negative values move the row
   to the left.
   """
    category = "Shearing"
    self_type = ImageType(ALL)
    args = Args([Int('row'), Int('distance')])

class shear_column(PluginFunction):
    """Shears a given column by a given amount.

*column*
   The column number to shear.

*distance*
   The number of pixels to move the column.  Positive values
   move the column downward.  Negative values move the column
   upward.
   """
    category = "Shearing"
    self_type = ImageType(ALL)
    args = Args([Int('column'), Int('distance')])

class UtilModule(PluginModule):
    cpp_headers=["image_utilities.hpp"]
    cpp_namespace=["Gamera"]
    category = "Utility"
    functions = [image_copy, resize, scale,
                 histogram, union_images,
                 fill_white, invert, clip_image, mask,
                 corelation, nested_list_to_image,
                 to_nested_list, shear_row, shear_column]
    author = "Michael Droettboom and Karl MacMillan"
    url = "http://gamera.dkc.jhu.edu/"
module = UtilModule()

union_images = union_images()
nested_list_to_image = nested_list_to_image()
