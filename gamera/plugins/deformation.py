#
#
# Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, Karl MacMillan
#               2010      Christoph Dalitz
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

class wave(PluginFunction):
    """
    Causes periodic disturbance of user-defined frequency, amplitude,
    and direction.  Turbulence specifies the amount of random
    variation from that line.
    """
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
    """
    Causes random shifting of pixels within a user-specified range, in
    a user-specified direction.
    """
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
    """
    Simulates rubbing off of ink from another page.
    """
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])
    return_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])
    args = Args([Int("transcription_prob", range=(0, 500)),
                 Int("random_seed", default=-1)])
    doc_examples = [(GREYSCALE, 50)]
    def __call__(self, transcription_prob, random_seed=0):
      return _deformation.inkrub(self, transcription_prob, random_seed)
    __call__ = staticmethod(__call__)

class ink_diffuse(PluginFunction):
    """
    Simulates water-driven diffusion of ink in paper.
    """
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])
    return_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])
    args = Args([Choice('diffusion_type',
                        ["Linear Horizontal","Linear Vertical","Brownian"]),
                 Float("exponential_decay_constant"),
                 Int("random_seed", default=-1)])
    doc_examples = [(GREYSCALE, 0, 20)]
    def __call__(self, diffusion_type, exponential_decay_constant, random_seed=0):
      return _deformation.ink_diffuse(self, diffusion_type, exponential_decay_constant, random_seed)
    __call__ = staticmethod(__call__)

class degrade_kanungo(PluginFunction):
    """Degrades an image due to a scheme proposed by Kanungo et al.
(see the reference below). This is supposed to emulate image defects
introduced through printing and scanning.

The degradation scheme depends on six parameters *(eta,a0,a,b0,b,k)* with
the following meaning:

  - each foreground pixel (black) is flipped with probability
    `a0*exp(-a*d^2) + eta`, where d is the distance to the closest
    background pixel

  - each background pixel (white) is flipped with probability
    `b0*exp(-b*d^2) + eta`, where d is the distance to the closest
    foreground pixel

  - eventuall a morphological closing operation is performed with a disk
    of diameter *k*. If you want to skip this step set *k=0*; in that
    case you should do your own smoothing afterwards.

The random generator is initialized with *random_seed* for allowing
reproducible results.

References:

  T. Kanungo, R.M. Haralick, H.S. Baird, et al.:
  *A statistical, nonparametric methodology for document
  degradation model validation.*
  IEEE Transactions on Pattern Analysis and Machine Intelligence
  22, pp. 1209-1223 (2000)
"""
    self_type = ImageType([ONEBIT])
    args = Args([Float('eta', range=(0.0,1.0)),
                 Float('a0', range=(0.0,1.0)),
                 Float('a'),
                 Float('b0', range=(0.0,1.0)),
                 Float('b'),
                 Int('k', default=2),
                 Int('random_seed', default=0)])
    return_type = ImageType([ONEBIT])
    author = "Christoph Dalitz"
    doc_examples = [(ONEBIT, 0.0, 0.5, 0.5, 0.5, 0.5, 2, 0)]

class white_speckles(PluginFunction):
    """Adds white speckles to an image. This is supposed to emulate
image defects introduced through printing, scanning and thresholding.

The degradation scheme depends on three parameters *(p,n,k)* with
the following meaning:

 - Each black pixel in the input image is taken with probability *p*
   as a starting point for a random walk of length *n*.
   Consequently *p* can be interpreted as the speckle frequency and *n*
   as a measure for the speckle size.

 - An image containing the random walk is smoothed by a closing operation
   with a rectangle of size *k*.

 - Eventually the image with the random walks is subtracted from the input
   image, which results in white speckles at the random walk positions

Input arguments:

 *p, n, k*:
   speckle frequency, random walk length and closing disc size

 *connectivity*:
   effects the connectivity of adjacent random walk points as shown
   in the following figure (in case of bishop moves the final closing
   operation whitens the neighbouring 4-connected pixels too):

   .. image:: images/randomwalk_connectivity.png

References:

  C. Dalitz, M. Droettboom, B. Pranzas, I. Fujinaga:
  *A Comparative Study of Staff Removal Algorithms.*
  IEEE Transactions on Pattern Analysis and Machine Intelligence 30,
  pp. 753-766 (2008)
"""
    self_type = ImageType([ONEBIT])
    args = Args([Float('p', range=(0.0,1.0)),
                 Int('n'),
                 Int('k', default=2),
                 Choice('connectivity', ['rook','bishop','king'], default=2),
                 Int('random_seed', default=0)])
    return_type = ImageType([ONEBIT], 'white_speckles')
    author = "Christoph Dalitz"
    doc_examples = [(ONEBIT, 0.05, 5, 2, 2, 0)]

# class batch_deform(PluginFunction):
#     """
#     Performs all possible deformations over given range of arguments.
#     """
#     pure_python = 1
#     self_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])
#     return_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])


class DefModule(PluginModule):
    cpp_headers=["deformations.hpp"]
    category = "Deformations"
    functions = [noise, inkrub, wave, ink_diffuse,
                 degrade_kanungo, white_speckles]
    author = "Albert Brzeczko"
    url = "http://gamera.sourceforge.net/"
module = DefModule()

