#
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
    self_type = ImageType(ALL)    
    return_type = ImageType(ALL)
    args = Args([Float("angle"), Pixel("bgcolor")])
    args.list[0].rng = (-180,180)
    doc_examples = [(RGB, 32.0, RGBPixel(255, 255, 255)), (COMPLEX, 15.0, 0.0j)]

class wave(PluginFunction):
    """Causes periodic disturbance of user-defined frequency, amplitude, and direction.
Turbulence specifies the amount of random variation from that line."""
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])
    return_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])
    args = Args([Int("amplitude"),
                 Int("period"),
                 Choice('direction', ['Horizontal','Vertical']),
                 Choice('waveform', ['Sinusoid','Square','Sawtooth','Triangle','Sinc']),
                 Int('offset'),
                 Float('turbulence', default=0.0),
                 Int('random_seed', default=-1)
                ])
    args.list[0].rng = (0,sys.maxint)
    args.list[1].rng = (0,sys.maxint)
    def __call__(self, amplitude, period, direction, waveform_type=0, offset=0, turbulence=0.0, random_seed=0):
        return _deformation.wave(self, amplitude, period, direction, waveform_type,
                                 offset, turbulence, random_seed)
    __call__ = staticmethod(__call__)
    doc_examples = [(RGB, 5, 10, 0, 0, 0), (RGB, 10, 5, 1, 2, 0)]

class noise(PluginFunction):
    """Causes random shifting of pixels within a user-specified range, in a user-specified direction"""
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])
    return_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])
    args = Args([Int("amplitude", range=(0, 500)),
                 Choice('direction',['Horizontal','Vertical']),
                 Int("random_seed", default=-1)
                ])
    doc_examples = [(RGB, 10, 0)]
    def __call__(self, amplitude, direction, random_seed=0):
      return _deformation.noise(self, amplitude, direction, random_seed)
    __call__ = staticmethod(__call__)

class inkrub(PluginFunction):
    """Simulates rubbing off of ink from another page"""
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])
    return_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])
    args = Args([Int("transcription_prob", range=(0, 500)),
                 Int("random_seed", default=-1)])
    doc_examples = [(GREYSCALE, 50)]
    def __call__(self, transcription_prob, random_seed=0):
      return _deformation.inkrub(self, transcription_prob, random_seed)
    __call__ = staticmethod(__call__)

class ink_diffuse(PluginFunction):
    """Simulates water-driven diffusion of ink in paper"""
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])
    return_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])
    args = Args([Choice('diffusion_type',["Linear Horizontal","Linear Vertical","Brownian"]),
                 Float("exponential_decay_constant"),
                 Int("random_seed", default=-1)])
    doc_examples = [(GREYSCALE, 0, 20)]
    def __call__(self, diffusion_type, exponential_decay_constant, random_seed=0):
      return _deformation.ink_diffuse(self, diffusion_type, exponential_decay_constant, random_seed)
    __call__ = staticmethod(__call__)

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

