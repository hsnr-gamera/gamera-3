#
#
# Copyright (C) 2001-2009 Ichiro Fujinaga, Michael Droettboom,
#                         Karl MacMillan, and Christoph Dalitz
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
from gamera import util
import _projections
from math import pi

class projection_rows(PluginFunction):
    """
    Compute the horizontal projections of an image.  This computes the
    number of pixels in each row.
    """
    self_type = ImageType([ONEBIT])
    return_type = IntVector()
    doc_examples = [(ONEBIT,)]

class projection_cols(PluginFunction):
    """
    Compute the vertical projections of an image.  This computes the
    number of pixels in each column.
    """
    self_type = ImageType([ONEBIT])
    return_type = IntVector()
    doc_examples = [(ONEBIT,)]

class projections(PluginFunction):
    """
    Computes the projections in both the *row*- and *column*-
    directions.  This is returned as a tuple (*rows*, *columns*),
    where each element is an ``IntVector`` of projections.
    (Equivalent to ``(image.projections_rows(),
    image.projections_cols())``).

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
    """
    Computes all vertical projections of an image skewed by a list of
    angles. As in rotate_, angles are measured clockwise and in
    degrees.  Thus a rotate followed by a projection_cols would be
    conceptually the same, albeit considerably slower.

    This function is overloaded to work both with a single angle and a
    list of angles as input. In the first case a single projection
    vector is returned, in the second a list of projections
    vectors. This is explained in the following example:

    .. code:: Python

      # called twice with a single angle as input
      proj1 = img.projection_skewed_cols(0.5)
      proj2 = img.projection_skewed_cols(1.0)
   
      # the same result with one function call
      projlist = img.projection_skewed_cols([0.5,1.0])
      proj1 = projlist[0]
      proj2 = projlist[1]

    Note that unlike rotate_ the image size is not extended. Image
    regions moved outside the original image size are simply clipped,
    which restricts this method to small angles.

    .. _rotate: deformations.html#rotate
    """
    category = "Analysis"
    self_type = ImageType([ONEBIT])
    args = Args([FloatVector("Rotation angles")])
    return_type = Class("nested_list")
    author = "Christoph Dalitz"

    def __call__(self, angles):
        if not util.is_sequence(angles):
            return _projections.projection_skewed_cols(self, [angles])[0]
        else:
            return _projections.projection_skewed_cols(self, angles)
    __call__ = staticmethod(__call__)
    doc_examples = [(ONEBIT, 15.0)]

class projection_skewed_rows(PluginFunction):
    """
    Computes all horizontal projections of an image skewed by a list
    of angles. For more details and an example see
    projection_skewed_cols_.

    Note that unlike rotate_ the image size is not extended. Image
    regions moved outside the original image size are simply clipped,
    which restricts this method to small angles.

    .. _projection_skewed_cols: #projection_skewed_cols
    """
    self_type = ImageType([ONEBIT])
    args = Args([FloatVector("Rotation angles")])
    return_type = Class("nested_list")
    author = "Christoph Dalitz"

    def __call__(self, angles):
        if not util.is_sequence(angles):
            return _projections.projection_skewed_rows(self, [angles])[0]
        else:
            return _projections.projection_skewed_rows(self, angles)
    __call__ = staticmethod(__call__)
    doc_examples = [(ONEBIT, 15.0)]

class rotation_angle_projections(PluginFunction):
    """
    Estimates the rotation angle of a document with the aid of skewed
    projections (see Ha, Bunke: 'Image Processing Methods for Document
    Image Analysis' in 'Handbook of Character Recognition and Document
    Image Analysis' edited by Bunke and Wang, World Scientific 1997).

    This method works for a wide range of documents (text, music,
    forms), but can become slow for large images. This particular
    implementation can be accelerated by reducing the number of black
    pixels in the image, eg. by scaling it down, only considering a
    fraction of the image or by removing 'uninteresting' pixels.

    Arguments:

    *minangle*, *maxangle* (optional):
      angle interval that is searched for the skew angle;
      default values are -2.5 and +2.5

    *accuracy* (optional):
      error bound for the skew angle estimate;
      default value is zero

    When *accuracy* is set to zero, a default value of
    *180\*0.5/(image.ncols\*pi)* is used, which is only a heuristic
    formula for little changes in the projection profile.

    Return Values:

    *rotation angle*:
      The rotation angle necessary to deskew the image.
      Can be used directly as input to rotate_

    *accuracy*:
      Accuracy of the returned angle.

    .. _rotate: deformations.html#rotate
    """
    category = "Analysis"
    self_type = ImageType([ONEBIT])
    args = Args([Float("minangle", default=-2.5), Float("maxangle", default=2.5), Float("accuracy", default=0.0)])
    return_type = FloatVector("rotation_angle_and_accuracy", 2)
    author = "Christoph Dalitz"
    pure_python = 1

    def __call__(self, minangle = -2.5, maxangle = 2.5, accuracy = 0):

        # l2norm: compute L2-Norm of derivative of vec
        def l2norm(vec):
            var = 0
            for i in range(0,len(vec)-1):
                var += (vec[i] - vec[i+1])**2
            return var

        # some arguments checking
        if (accuracy == 0):
            accuracy = 180 * 0.5 / (self.ncols * pi)
        if (maxangle <= minangle):
            raise RuntimeError("maxangle %f must be greater than minangle %f\n" \
                               % (maxangle, minangle))

        #
        # rough guess where the maximum is
        # necessary because l2norm has many local maxima
        #
        roughacc = 0.5
        if ((maxangle - minangle)/4.0) < roughacc:
            # at least five trial points
            roughacc = (maxangle - minangle) / 4.0
        else:
            roughacc = (maxangle-minangle) / round((maxangle-minangle)/roughacc)
        angle = [(minangle + x*roughacc) for x in \
                    range(0,int(round((maxangle - minangle)/roughacc))+1)]
        alist = [l2norm(x) for x in self.projection_skewed_rows(angle)]
        # find maximum in list
        fb = alist[0]; bi = 0
        for i in range(1,len(alist)):
            if alist[i] > fb:
                fb = alist[i]; bi = i
        b = angle[bi]

        #
        # initialize values for golden section search
        #
        if (bi == 0):
            # maximum on lower iterval end: check neighborhood
            c = b + 1.5*accuracy;
            fc = l2norm(self.projection_skewed_rows(c))
            if (fc > fb) and (c < angle[bi+1]):
                a = b; fa = fb;
                b = c; fb = fc;
                c = angle[bi+1]; fc = alist[bi+1];
            else:
                raise RuntimeError("maximum found on interval end %f\n" \
                                   % angle[bi])
        elif (bi == len(angle)-1):
            # maximum on upper iterval end: check neighborhood
            a = b - 1.5*accuracy;
            fa = l2norm(self.projection_skewed_rows(a))
            if (fa > fb) and (a > angle[bi-1]):
                c = b; fc = fb;
                b = a; fb = fa;
                a = angle[bi-1]; fa = alist[bi-1];
            else:
                raise RuntimeError("maximum found on interval end %f\n" \
                                   % angle[bi])
        else:
            # the normal case: maximum somewhere in the middle
            a = angle[bi-1]; fa = alist[bi-1];
            c = angle[bi+1]; fc = alist[bi+1];

        #
        # fine tuning with golden section search
        # see Press at al: "Numerical Recipes",
        # Cambridge University Press (1986)
        #
        golden = 0.38197  # (3 - sqrt(2)) / 2
        x = 500   # dummy value for recognition of first iteration
        while (c-b > accuracy) or (b-a > accuracy):
            if (x == 500):
                # special case first iteration
                if (fc > fa):
                    x = b + golden * (c - b)
                else:
                    x = b - golden * (b - a)
            else:
                # ordinary situation
                if (c-b > b-a):
                    x = b + golden * (c - b)
                else:
                    x = b - golden * (b - a)
            fx = l2norm(self.projection_skewed_rows(x))
            if (x > b):
                if (fx < fb):
                    c = x; fc = fx
                else:
                    a = b; fa = fb;
                    b = x; fb = fx;
            else:
                if (fx < fb):
                    a = x; fa = fx
                else:
                    c = b; fc = fb;
                    b = x; fb = fx;
        return [b, accuracy]

    __call__ = staticmethod(__call__)


class ProjectionsModule(PluginModule):
    cpp_headers=["projections.hpp"]
    category = "Analysis"
    functions = [projection_rows, projection_cols, projections,
                 projection_skewed_rows, projection_skewed_cols,
                 rotation_angle_projections]
    author = "Michael Droettboom and Karl MacMillan"
    url = "http://gamera.dkc.jhu.edu/"
module = ProjectionsModule()

