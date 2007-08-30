# -*- coding: utf-8 -*-
#
# Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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
import _thinning

class Thinning(PluginFunction):
    self_type = ImageType([ONEBIT])
    return_type = ImageType([ONEBIT])
    doc_examples = [(ONEBIT,)]

class thin_zs(Thinning):
    """
    Thins (skeletonizes) a ONEBIT image using the Zhang and Suen
    algorithm.

    The resulting skeleton is not a medial axis transformation, and
    the ends of the skeleton will not extend to the edges of the
    original image.

    T. Y. Zhang and C. Y. Suen. 1984.  A Fast Parallel Algorithm for
    Thinning Digital Patterns., *Communications of ACM*, 2(3).
  
    R. C. Gonzalez and P. Wintz. 1987 *Digital Image Processing.*,
    2. edition. 398-402.
    """
    pass

_hs_common_docstring = """
Derives the medial axis transformation from a ONEBIT image using the
Haralick and Shapiro algorithm.

%s

Unlike thin_zs_ and thin_lc_, this function performs a medial axis
transformation, and the ends of the skeleton extend to the corners of
the original image.

Consider using thin_hs_large_image_ instead, for faster performance on
large images with a lot of connected components.

R. M. Haralick and L. G. Shapiro. 1992.  *Computer and Robot Vision*,
Vol. 1, Chapter 5 (especially 5.10.1).  Reading, MA: Addison-Wesley.
"""
    
class thin_hs(Thinning):
    _hs_common_docstring % "This function is an alias for medial_axis_transform_hs_."
    pass

class medial_axis_transform_hs(Thinning):
    _hs_common_docstring % "This function is an alias for thin_hs_."
    pure_python = True
    def __call__(self):
        return _thinning.thin_hs(self)

class thin_hs_large_image(Thinning):
    """
    Thins (skeletonizes) a ONEBIT image using the Haralick and Shapiro
    algorithm.

    Unlike thin_hs_, this algorithm performs skeletonization on one
    connected component at a time.  On large images with a lot of
    connected components, this can be significantly faster.  However,
    for small images with a single connected component, this has
    unnecessary overhead, which is why both versions are included.
    Please note cc_analysis results in a labelled image, which you can
    reset afterwards with reset_onebit_image().
    """
    pure_python = True
    def __call__(self):
        copy = self.image_copy()
        ccs = copy.cc_analysis()
        for cc in ccs:
            thin_cc = cc.thin_hs()
            cc.and_image(thin_cc, in_place=True)
        return copy
    __call__ = staticmethod(__call__)

class medial_axis_transform_large_image_hs(Thinning):
    """
    This function is an alias for thin_hs_large_image_.
    """
    pure_python = True
    def __call__(self):
        return thin_hs_large_image()(self)

class thin_lc(Thinning):
    """
    Thins (skeletonizes) a ONEBIT image using the Lee and Chen
    algorithm.

    This function is a simple extension to the Zhang and Suen
    algorithm in thin_zs_ that ensure that no two pixels are more than
    4-connected.
    
    The resulting skeleton is not a medial axis transformation, and
    the ends of the skeleton will not extend to the edges of the
    original image.

    H.-J. Lee and B. Chen. 1992.  Recognition of handwritten chinese
    characters via short line segments. *Pattern Recognition*. 25(5)
    543-552.
    """
    pass

class ThinningModule(PluginModule):
    category = "Filter/Thinning"
    functions = [thin_zs, thin_hs, medial_axis_transform_hs,
                 thin_hs_large_image, medial_axis_transform_large_image_hs,
                 thin_lc]
    cpp_headers = ["thinning.hpp"]
    author = u"Michael Droettboom and Karl MacMillan (based on code by \u00d8ivind Due Trier and Qian Huang)"
    url = "http://gamera.dkc.jhu.edu/"
module = ThinningModule()

del Thinning
del _hs_common_docstring
