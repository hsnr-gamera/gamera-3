#
#
# Copyright (C) 2001 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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
import gamera.config
import _image_utilities 

class image_copy(PluginFunction):
    """Copies an image, with all of its underlying data."""
    category = "Utility/Copy"
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])
    return_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])
    args = Args([Choice("storage format", ["DENSE", "RLE"])])
    def __call__(image, storage_format = 0):
        return _image_utilities.image_copy(image, storage_format)
    __call__ = staticmethod(__call__)
image_copy = image_copy()

class rotate_copy(PluginFunction):
    """Copies and rotates an image"""
    category = "Utility/Copy"
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])
    return_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])
    args = Args(Float("angle"))
rotate_copy = rotate_copy()

class resize_copy(PluginFunction):
    """Copies and resizes an image. In addition to size the type of
    interpolation can be specified to allow tradeoffs between speed
    and quality."""
    category = "Utility/Copy"
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])
    args= Args([Int("nrows"), Int("ncols"),
                Choice("Interpolation Type", ["None", "Linear", "Spline"])])
    return_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])
resize_copy = resize_copy()

class scale_copy(PluginFunction):
    """Copies and scales an image. In addition to size the type of
    interpolation can be specified to allow tradeoffs between speed
    and quality."""
    category = "Utility/Copy"
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])
    args= Args([Real("scaling"),
                Choice("Interpolation Type", ["None", "Linear", "Spline"])])
    return_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])
scale_copy = scale_copy()

class histogram(PluginFunction):
    """Compute the histogram of an image. The return type is a
    Python array of doubles. The values are percentages. If a
    gui is being used the histogram is displayed."""
    self_type = ImageType([GREYSCALE, GREY16])
    return_type = FloatVector("histogram")
    def __call__(image):
        hist = _image_utilities.histogram(image)
        gui = gamera.config.get_option("__gui")
        if gui:
            gui.ShowHistogram(hist, mark=image.otsu_find_threshold())
        return hist
    __call__ = staticmethod(__call__)
histogram = histogram()

class union_images(PluginFunction):
    self_type = None
    args = Args([ImageList('list_of_images')])
    return_type = ImageType([ONEBIT])
union_images = union_images() 

class projections_rows(PluginFunction):
    """Compute the projections of an image.  The computes the
    number of pixels in each row."""
    self_type = ImageType([ONEBIT])
    return_type = IntVector("rows")
projections_rows = projections_rows()

class projections_cols(PluginFunction):
    """Compute the projections of an image.  The computes the
    number of pixels in each row."""
    self_type = ImageType([ONEBIT])
    return_type = IntVector("cols")
projections_cols = projections_cols()

class projections(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = Class("projections")
    pure_python = 1
    def __call__(image):
        rows = _image_utilities.projections_rows(image)
        cols = _image_utilities.projections_cols(image)
        gui = gamera.config.get_option("__gui")
        if gui:
            gui.ShowProjections(rows, cols, image)
        return (rows, cols)
    __call__ = staticmethod(__call__)
projections = projections()

class fill_white(PluginFunction):
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])
fill_white = fill_white()

class UtilModule(PluginModule):
    cpp_headers=["image_utilities.hpp"]
    cpp_namespace=["Gamera"]
    category = "Utility"
    functions = [image_copy, rotate_copy, resize_copy, scale_copy,
                 histogram, union_images, projections_rows, projections_cols,
                 projections, fill_white]
    author = "Michael Droettboom and Karl MacMillan"
    url = "http://gamera.dkc.jhu.edu/"

module = UtilModule()
