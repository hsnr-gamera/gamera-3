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

from gamera.plugin import *
from gamera.gui import has_gui
import _projections

class projection_rows(PluginFunction):
    """Compute the horizontal projections of an image.  This computes the
number of pixels in each row."""
    self_type = ImageType([ONEBIT])
    return_type = IntVector()
    doc_examples = [(ONEBIT,)]

class projection_cols(PluginFunction):
    """Compute the vertical projections of an image.  This computes the
number of pixels in each column."""
    self_type = ImageType([ONEBIT])
    return_type = IntVector()
    doc_examples = [(ONEBIT,)]

class projections(PluginFunction):
    """Computes the projections in both the *row*- and *column*- directions.
This is returned as a tuple (*rows*, *columns*), where each element is an
``IntVector`` of projections.
(Equivalent to ``(image.projections_rows(), image.projections_cols())``).

If the GUI is being used, the result is displayed in a window:

.. image:: images/projections.png
"""
    self_type = ImageType([ONEBIT])
    return_type = Class()
    pure_python = 1
    def __call__(image):
        rows = _projections.projection_rows(image)
        cols = _projections.projection_cols(image)
        gui = has_gui.gui
        if gui:
            gui.ShowProjections(rows, cols, image)
        return (rows, cols)
    __call__ = staticmethod(__call__)

class projection_skewed_cols(PluginFunction):
    """Computes all vertical projections of an image skewed by a list of
angles. As in rotateShear_, angles are measured clockwise and in degrees.
Thus a rotateShear followed by a projection_cols would be conceptually the
same, albeit considerably slower.

This function is overloaded to work both with a single angle and a list
of angles as input. In the first case a single projection vector is
returned, in the second a list of projections vectors. This is explained
in the following example:

.. code:: Python

   # called twice with a single angle as input
   proj1 = img.projection_skewed_cols(0.5)
   proj1 = img.projection_skewed_cols(1.0)
   
   # the same result with one function call
   projlist = img.projection_skewed_cols([0.5,1.0])
   proj1 = projlist[0]
   proj2 = projlist[1]

Note that unlike rotateShear_ the image size is not extended. Image regions
moved outside the original image size are simply clipped, which restricts
this method to small angles.

.. _rotateShear: deformations.html#rotateshear
"""
    category = "Analysis"
    self_type = ImageType([ONEBIT])
    args = Args([FloatVector("Rotation angles")])
    return_type = Class("nested_list")
    author = "Christoph Dalitz"

    def __call__(self, angles):
        if type(angles) != list:
            return _projections.projection_skewed_cols(self, [angles])[0]
        else:
            return _projections.projection_skewed_cols(self, angles)
    __call__ = staticmethod(__call__)
    doc_examples = [(ONEBIT, 15)]

class projection_skewed_rows(PluginFunction):
    """Computes all horizontal projections of an image skewed by a list of
angles. For more details and an example see projection_skewed_cols_.

Note that unlike rotateShear_ the image size is not extended. Image regions
moved outside the original image size are simply clipped, which restricts
this method to small angles.

.. _rotateShear: deformations.html#rotateshear
"""
    self_type = ImageType([ONEBIT])
    args = Args([FloatVector("Rotation angles")])
    return_type = Class("nested_list")
    author = "Christoph Dalitz"

    def __call__(self, angles):
        if type(angles) != list:
            return _projections.projection_skewed_rows(self, [angles])[0]
        else:
            return _projections.projection_skewed_rows(self, angles)
    __call__ = staticmethod(__call__)
    doc_examples = [(ONEBIT, 15)]

class ProjectionsModule(PluginModule):
    cpp_headers=["projections.hpp"]
    cpp_namespace=["Gamera"]
    category = "Analysis"
    functions = [projection_rows, projection_cols, projections,
                 projection_skewed_rows, projection_skewed_cols]
    author = "Michael Droettboom and Karl MacMillan"
    url = "http://gamera.dkc.jhu.edu/"
module = ProjectionsModule()
