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

class convolve(PluginFunction):
    """Convolves an image."""
    image_types_must_match = True
    self_type = ImageType(ALL)
    args = Args([ImageType([FLOAT], 'kernel'),
                 Choice('border_treatment',
                        ['avoid', 'clip', 'repeat', 'reflect', 'wrap'],
                        default=1)])
    return_type = ImageType(ALL)

class ConvolutionModule(PluginModule):
    cpp_headers=["convolution.hpp"]
    cpp_namespace=["Gamera"]
    category = "Convolution"
    functions = [convolve]
    author = "Michael Droettboom"
    url = "http://gamera.dkc.jhu.edu/"
module = ConvolutionModule()

