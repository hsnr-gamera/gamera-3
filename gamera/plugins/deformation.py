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
  from gamera.core import RGBPixel
except:
  def RGBPixel(*args):
    pass

try:
    from gamera.core import *
except:
    pass

import _deformation

class rotate(PluginFunction):
    """Rotates an image by skew method"""
    category = "Deformations"
    self_type = ImageType([ONEBIT, FLOAT, COMPLEX, RGB])    
    return_type = ImageType([ONEBIT, FLOAT, COMPLEX, RGB])
    args = Args([Float("Rotation angle"), Pixel("Background Color")])
    #args.list[0].rng = (-180,180)
    image_types_must_match = True
    #doc_examples = [(RGB, 32.0, RGBPixel(255, 255, 255))]

class wave(PluginFunction):
    """Causes periodic disturbance of user-defined frequency, amplitude, and direction"""
    category = "Deformations"
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])
    return_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])
    args = Args([Int("Amplitude"),\
                 Int("Period"),\
                 Choice('Direction',['Horizontal','Vertical']),\
                 Choice('Waveform type',['Sinusoid','Square','Sawtooth','Triangle','Sinc']),\
                 Int('Waveform Offset')\
                ])
    args.list[0].rng = (0,sys.maxint)
    args.list[1].rng = (0,sys.maxint)
    def __call__(self, amplitude, period, direction, waveform_type=0, offset=0):
        return _deformation.wave(self, amplitude, period, direction, waveform_type, offset)
    __call__ = staticmethod(__call__)
    doc_examples = [(RGB, 5, 10, 0, 0, 0), (RGB, 10, 5, 1, 2, 0)]

class noise(PluginFunction):
    """Causes random shifting of pixels within a user-specified range, in a user-specified direction"""
    category = "Deformations"
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])
    return_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])
    args = Args([Int("Amplitude"),\
                 Choice('Direction',['Horizontal','Vertical']),\
                ])
    args.list[0].rng = (0,500)
    doc_examples = [(RGB, 10, 0)]

class inkrub(PluginFunction):
    """Simulates rubbing off of ink from another page"""
    category = "Deformations"
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])
    return_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])
    args = Args(Int("Transcription Probability 1 in"))
    args.list[0].rng = (0,500)
    doc_examples = [(GREYSCALE, 50)]

class ink_diffuse(PluginFunction):
    """Simulates water-driven diffusion of ink in paper"""
    category = "Deformations"
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])
    return_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])
    args = Args([Choice('Diffusion Type',["Linear Horizontal","Linear Vertical","Brownian"]),\
                Float("Exponential decay constant 1 over")])

class batch_deform(PluginFunction):
    """Performs all possible deformations over given range of arguments"""
    pure_python = 1
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])
    return_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])

class DefModule(PluginModule):
    cpp_headers=["deformations.hpp"]
    category = "Deformations"
    functions = [rotate, noise, inkrub, wave, ink_diffuse]
    author = "Albert Brzeczko"
    url = "http://gamera.dkc.jhu.edu/"
module = DefModule()

