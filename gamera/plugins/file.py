#
#
# Copyright (C) 2001 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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
import _save_tiff, _save_impex

class _File(PluginFunction):
  cpp_source = "file.hpp"
  category = "File"

class save_tiff(_File):
  def __call__(self, filename):
    _save_tiff.save_tiff(self.m, filename)
save_tiff = save_tiff()

class save_impex(_File):
  def __call__(self, filename):
    _save_impex.save_impex(self.m, filename)
save_impex = save_impex()

class save_image(_File):
  def __call__(self, filename = None):
    use_impex = 0
    try:
      import _save_impex
      use_impex = 1
    except:
      pass

    if filename != None:
      self.name = filename
    if self.name == None or self.name == "":
      raise ValueError("self.name must be set to a valid filename")
    split = string.split(self.name, '.')
    if len(split) > 1:
      if split[len(split) -1] == "tiff" or split[len(split) -1] == "tif":
        self.save_tiff(self.name)
      else:
        if use_impex:
          self.save_impex(self.name)
    else:
      self.save_tiff(self.name)
save_image = save_image()

class tiff_info(_File):
  self_type = String("filename")
  def __call__(filename):
    info = ImageInfo()
    _tiff_info.tiff_info(info, filename)
    return info
    

plugins = [save_image, save_tiff, save_impex, tiff_info]
