#
# Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, Karl MacMillan
#               2009-2010 Christoph Dalitz
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

import array
from gamera.plugin import *
import _features

class Feature(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = FloatVector(length=1)
    feature_function = True
    doc_examples = [(ONEBIT,)]

class black_area(Feature):
    """
    The simplest of all feature-generating functions, ``black_area``
    simply returns the number of black pixels.

    +---------------------------+
    | **Invariant to:**         |  
    +-------+----------+--------+
    | scale | rotation | mirror | 
    +-------+----------+--------+
    |       |     X    |   X    |
    +-------+----------+--------+
    
    .. warning:: This feature is not scale invariant.
    """
    pass

class moments(Feature):
    """
    Returns *moments* of the image.

    The first two elements of the returned ``FloatVector`` are the 
    center of gravity on *x* and *y* axis normalized by width and height,
    respectively. The following seven entries are the 
    *normalized central moments* (*u20,u02,u11,u30,u12,u21,u03*). For their
    definition, see Gonzalez, Woods: \"Digital Image Processing\",
    Prentice Hall, second edition (2002).

    +---------------------------+
    | **Invariant to:**         |  
    +-------+----------+--------+
    | scale | rotation | mirror | 
    +-------+----------+--------+
    |   X   |          |        |
    +-------+----------+--------+
    """
    return_type = FloatVector(length=9)

class nholes(Feature):
    """
    Computes for each row or column the average number of white runs not
	touching the border. From these values, the average over all rows and
	all columns is returned.

    The elements of the returned ``FloatVector`` are:

    0. vertical
    1. horizontal

    +---------------------------+
    | **Invariant to:**         |  
    +-------+----------+--------+
    | scale | rotation | mirror | 
    +-------+----------+--------+
    |   X   |          |   X    |
    +-------+----------+--------+
    """
    return_type = FloatVector(length=2)

class nholes_extended(Feature):
    """
    Divides the image into four strips and then does a nholes_
    analysis on each of those strips. This is first done vertically
    and then horizontally, resulting in a total of eight feature values.

    The elements of the returned ``FloatVector`` are:

    0 - 3
      vertical ``nholes`` for each of the strips in order left to right.
    4 - 7
      horizonal ``nholes`` for each of the strips in order top to bottom.

    +---------------------------+
    | **Invariant to:**         |  
    +-------+----------+--------+
    | scale | rotation | mirror | 
    +-------+----------+--------+
    |   X   |          |        |
    +-------+----------+--------+
    """
    return_type = FloatVector(length=8)

class volume(Feature):
    """
    The percentage of black pixels within the rectangular bounding box
    of the image.  Result in range (0, 1].

    +---------------------------+
    | **Invariant to:**         |  
    +-------+----------+--------+
    | scale | rotation | mirror | 
    +-------+----------+--------+
    |   X   |          |   X    |
    +-------+----------+--------+
    """
    pass

class area(Feature):
    """
    The area of the bounding box (i.e. *nrows* * *ncols*).

    +---------------------------+
    | **Invariant to:**         |  
    +-------+----------+--------+
    | scale | rotation | mirror | 
    +-------+----------+--------+
    |       |          |   X    |
    +-------+----------+--------+
    """
    pass

class aspect_ratio(Feature):
    """
    The aspect ratio of the bounding box (i.e. *ncols* / *nrows*).

    This feature is scale invariant.

    +---------------------------+
    | **Invariant to:**         |  
    +-------+----------+--------+
    | scale | rotation | mirror | 
    +-------+----------+--------+
    |   X   |          |   X    |
    +-------+----------+--------+
    """
    pass
    
class nrows_feature(Feature):
    """
    Simply the number of rows. As this feature is *not* scale
    invariant, it is helpful for distinguishing similar symbols of
    different size.

    +---------------------------+
    | **Invariant to:**         |  
    +-------+----------+--------+
    | scale | rotation | mirror | 
    +-------+----------+--------+
    |       |          |   X    |
    +-------+----------+--------+
    """
    pass

class ncols_feature(Feature):
    """
    Simply the number of cols. As this feature is *not* scale
    invariant, it is helpful for distinguishing similar symbols of
    different size.

    +---------------------------+
    | **Invariant to:**         |  
    +-------+----------+--------+
    | scale | rotation | mirror | 
    +-------+----------+--------+
    |       |          |   X    |
    +-------+----------+--------+
    """
    pass

class compactness(Feature):
    """
    Compactness is the volume to surface ratio. Highly ornate connected
    components have a low compactness, whereas a perfect circle has a 
    high compactness. The present implementation of this feature in
    Gamera does however not return the compactness, but its *inverse*,
    i.e. the surface to volume ratio.

    Since this function requires allocation and deallocation of
    memory, it is relatively slow.  However, it has proven to be a
    very useful feature in many cases.

    This feature is not scale invariant, because, as
    the image is scaled by *a*, the surface increases proportional to *a*
    while the volume grows with *a^2*.
    This is currently not corrected for.

    +---------------------------+
    | **Invariant to:**         |  
    +-------+----------+--------+
    | scale | rotation | mirror | 
    +-------+----------+--------+
    |       |    X     |   X    |
    +-------+----------+--------+
    """
    pass
    
class volume16regions(Feature):
    """
    Divides the image into a 4 x 4 grid of 16 regions and calculates
    the volume within each.

    +---------------------------+
    | **Invariant to:**         |  
    +-------+----------+--------+
    | scale | rotation | mirror | 
    +-------+----------+--------+
    |   X   |          |        |
    +-------+----------+--------+
    """
    return_type = FloatVector(length=16)

class volume64regions(Feature):
    """
    Divides the image into a 8 x 8 grid of 64 regions and calculates
    the volume within each.

    +---------------------------+
    | **Invariant to:**         |  
    +-------+----------+--------+
    | scale | rotation | mirror | 
    +-------+----------+--------+
    |   X   |          |        |
    +-------+----------+--------+
    """
    return_type = FloatVector(length=64)

class zernike_moments(Feature):
    """
    I can't say I understand much about Zernike moments, except that
    they are well known for all kinds of invariance, and are often detailed
    enough to reconstruct many shapes in a reasonable way.

    A. Khotanzad and Y. Hong. Invariant image recognition by Zernike
    moments.  *IEEE Transactions on Pattern Analysis and Machine
    Intelligence*, 12(5), 1990.

    +---------------------------+
    | **Invariant to:**         |  
    +-------+----------+--------+
    | scale | rotation | mirror | 
    +-------+----------+--------+
    |   X   |    X     |   X    |
    +-------+----------+--------+
    """
    return_type = FloatVector(length=26)

class skeleton_features(Feature):
    """
    Generates a number of features based on the skeleton of an image.
    First, the image in skeletonized using the Lee and Chen algorithm,
    which guarantees that the pixels of the resulting skeleton are
    never more than 4-connected.  Then, this skeleton is analysed for
    a number of properties:
    
    0. Number of X joints (4-connected pixels)
    1. Number of T joints (3-connected pixels)
    2. Average number of bend points (pixels which do not form a horizontal or
       vertical line with its neighbors)
    3. Number of end points (1-connected pixels)
    4. Number of *x*-axis crossings with respect to the *x*-axis through the center of mass
    5. Number of *y*-axis crossings with respect to the *y*-axis through the center of mass

    +---------------------------+
    | **Invariant to:**         |  
    +-------+----------+--------+
    | scale | rotation | mirror | 
    +-------+----------+--------+
    |   X   |          |   X    |
    +-------+----------+--------+
    """
    return_type = FloatVector(length=6)

class top_bottom(Feature):
    """
    Features useful only for segmentation-free analysis.  Currently,
    the first feature is the first row containing a black pixel, and
    the second feature is the last row containing a black pixel.

    +---------------------------+
    | **Invariant to:**         |  
    +-------+----------+--------+
    | scale | rotation | mirror | 
    +-------+----------+--------+
    |   X   |          |   X    |
    +-------+----------+--------+
    """
    return_type = FloatVector(length=2)

class generate_features(PluginFunction):
    """
    Generates features for the image by calling a number of feature
    functions and storing the results in the image's ``features``
    member variable (a Python ``array``).
    
    *features*
      Optional.  A list of feature function names.  If not given, the
      previously set feature functions will be used.  If none were
      previously given, all available feature functions will be used.
      Using all feature functions can also be forced by passing
      ``'all'``.

    .. warning:: For efficiency, if the given feature functions match
       those that have been already generated for the image, the
       features are *not* recalculated.  If you want to force
       recalculation, pass the optional argument ``force=True``.
    """
    category = "Utility"
    pure_python = True
    self_type = ImageType([ONEBIT])
    args = Args([Class('features', list), Check('force')])
    return_type = None
    cache = {}
    def __call__(self, features=None, force=False):
      if features is None:
         features = self.get_feature_functions()
      if self.feature_functions == features and not force:
         return
      self.feature_functions = features
      features, num_features = features
      if len(self.features) != num_features:
          if not generate_features.cache.has_key(num_features):
              generate_features.cache[num_features] = [0] * num_features
          self.features = array.array('d', generate_features.cache[num_features])
      offset = 0
      for name, function in features:
          function.__call__(self, offset)
          offset += function.return_type.length
    __call__ = staticmethod(__call__)

class FeaturesModule(PluginModule):
    category = "Features"
    cpp_headers=["features.hpp"]
    functions = [black_area, moments, nholes,
                 nholes_extended, volume, area,
                 aspect_ratio, nrows_feature, ncols_feature, compactness,
                 volume16regions, volume64regions,
                 generate_features, zernike_moments,
                 skeleton_features, top_bottom]
    author = "Michael Droettboom and Karl MacMillan"
    url = "http://gamera.sourceforge.net/"
module = FeaturesModule()

def get_features_length(features):
    """
    Given a list of feature functions return the number of features
    that will be generated. This function is necessary because each
    features 'function' can return multiple individual float values.
    """
    from gamera import core
    ff = core.ImageBase.get_feature_functions(features)
    return ff[1]

def generate_features_list(list, features='all'):
   """
   Generate features on a list of images.

   *features*
     Follows the same rules as for generate_features_.
   """
   from gamera import core, util
   ff = core.Image.get_feature_functions(features)
   progress = util.ProgressFactory("Generating features...", len(list) / 10)
   try:
      for i, glyph in enumerate(list):
         glyph.generate_features(ff)
         if i % 10 == 0:
             progress.step()
   finally:
       progress.kill()

generate_features = generate_features()

del Feature
