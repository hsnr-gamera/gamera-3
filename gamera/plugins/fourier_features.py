#
# Copyright (C) 2013      Christian Brandt and Christoph Dalitz
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

from gamera.plugin import *
import _fourier_features

class fourier_broken(PluginFunction):
	"""Fourier descriptor for arbitrary (not necessarily connected) shapes
according to equations (18) and (19) in 

  C. Dalitz, C. Brandt, S. Goebbels, D. Kolanus:
  `Fourier Descriptors for Broken Shapes`__.
  EURASIP Journal on Advances in Signal Processing 2013:161, 2013
 
The coefficient c_0 is used for scale normalisation. The 
absolute values of the coefficients A(0), A(N-1) A(1), A(N-2), ... are 
returned.

    +---------------------------+
    | **Invariant to:**         |  
    +-------+----------+--------+
    | scale | rotation | mirror | 
    +-------+----------+--------+
    |   X   |     X    |        |
    +-------+----------+--------+

.. __: http://dx.doi.org/10.1186/1687-6180-2013-161
"""
	category = "Features"
	self_type = ImageType([ONEBIT])
	return_type = FloatVector(length=48)
	feature_function = True
        doc_examples = [(ONEBIT,)]

class FourierFeaturesModule(PluginModule):
	cpp_headers = ["fourier_features.hpp"]
	category = "Features"
	functions = [fourier_broken]
	cpp_sources = ["src/geostructs/kdtree.cpp", "src/geostructs/delaunaytree.cpp"]
	extra_compile_args = ["-DFDLENGTH=48"]
	author = "Christian Brandt and Christoph Dalitz"
	url = "http://gamera.sourceforge.net/"

module = FourierFeaturesModule()
