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

"""The deformations module contains plugins for applying
deformations to images."""

from gamera.plugin import *
try:
    from gamera.core import *
except:
    pass

import _deformation

class rotate(PluginFunction):
    """Rotates an image by skew method"""
    category = "Deformations"
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])    
    return_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])
    args = Args([Float("Rotation angle"), Pixel("Background Color")])
    args.list[0].rng = (-180,180)

class wave(PluginFunction):
    """Causes periodic disturbance of user-defined frequency, amplitude, and direction"""
    category = "Deformations"
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])
    return_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])
    args = Args([Int("Amplitude"),\
                 Float("Period"),\
                 Choice('Direction',['Horizontal','Vertical']),\
                 Choice('Waveform type',['Sinusoid','Square','Sawtooth','Triangle','Sinc']),\
                 Int('Waveform Offset')\
                ])

class noise(PluginFunction):
    """Causes random shifting of pixels within a user specified range, in a user-specified direction"""
    category = "Deformations"
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])
    return_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])
    args = Args([Int("Amplitude"),\
                 Choice('Direction',['Horizontal','Vertical']),\
                ])
    args.list[0].rng = (0,500)

class inkrub(PluginFunction):
    """Simulates rubbing off of ink from another page"""
    category = "Deformations"
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])
    return_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])
    args = Args(Int("Transcription Probability 1 in"))
    args.list[0].rng = (0,500)

class DefModule(PluginModule):
    cpp_headers=["deformations.hpp"]
    category = "Deformations"
    functions = [rotate,noise,inkrub,wave]
    author = "Albert Bzreckzo"
    url = "http://gamera.dkc.jhu.edu/"
module = DefModule()

