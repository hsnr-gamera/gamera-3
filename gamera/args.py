# vi:set tabsize=3:
#
# Copyright (C) 2001-2004 Ichiro Fujinaga, Michael Droettboom,
#                          and Karl MacMillan
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

from gamera.gui import has_gui
import sys, os.path   # Python standard library
from types import *
import util, paths            # Gamera specific

######################################################################

# This is a "self-generating" dialog box

class NoGUIArgs:
   def setup(self, parent, locals):
      raise Exception("No GUI environment available.  Cannot display dialog.")
   
   def show(self, parent, locals, function=None, wizard=0):
      raise Exception("No GUI environment available.  Cannot display dialog.")

class Args(NoGUIArgs):
   # list is a list of "Arg s"
   def __init__(self, list=[], name="Arguments", function=None, title=None):
      if not util.is_sequence(list):
         list = [list]
      self.valid = 1
      self.list = list
      self.name = name
      self.function = function
      self.title = title

   def __repr__(self):
      return "<" + self.__class__.__name__ + ">"

   def get_args_string(self):
      results = [x.get_string() for x in self.controls]
      tuple = '(' + ', '.join(results) + ')'
      return tuple

   def get_args(self):
      return [control.get() for control in self.controls]

   def __getitem__(self, i):
      return self.list[i]
   index = __getitem__

   def __len__(self, i):
      return len(self.list)
   

######################################################################

# ARGUMENT TYPES

class Arg:
   default = 0
   length = 1

   def __init__(self, name=None):
##       if name is None:
##          name = "_" + str(hash(self))
      self.name = name

   def __repr__(self):
      return "<" + self.__class__.__name__ + ">"

   def rest_repr(self):
      return self.__class__.__name__
   
class Int(Arg):
   def __init__(self, name=None, range=(-sys.maxint, sys.maxint), default=0):
      Arg.__init__(self, name)
      self.rng = range
      self.default = default

   def rest_repr(self):
      result = "int"
      if self.rng != (-sys.maxint, sys.maxint):
         result += str(self.rng)
      return result

class Real(Arg):
   def __init__(self, name=None, range=(-sys.maxint, sys.maxint), default=0):
      Arg.__init__(self, name)
      self.rng = range
      self.default = default

   def rest_repr(self):
      result = "double"
      if self.rng != (-sys.maxint, sys.maxint):
         result += str(self.rng)
      return result

Float = Real

class String(Arg):
   def __init__(self, name=None, default=''):
      Arg.__init__(self, name)
      self.default = default

class Class(Arg):
   def __init__(self, name=None, klass=None, list_of=False):
      Arg.__init__(self, name)
      self.klass = klass
      self.list_of = list_of

   def rest_rept(self):
      return self.klass.__name__

class ImageType(Arg):
   def __init__(self, pixel_types, name=None, list_of = 0):
      import core
      Arg.__init__(self, name)
      if not core is None:
         self.klass = core.ImageBase
      if not util.is_sequence(pixel_types):
         pixel_types = (pixel_types,)
      self.pixel_types = pixel_types
      self.list_of = list_of

   def rest_repr(self):
      return 'Image [%s]' % '|'.join([util.get_pixel_type_name(x) for x in self.pixel_types])

class Choice(Arg):
   def __init__(self, name=None, choices=[], default=0):
      Arg.__init__(self, name)
      self.choices = choices
      self.default = default

   def rest_repr(self):
      return 'Choice[%s]' % '|'.join(self.choices)

class _Filename(Arg):
   def __init__(self, name=None, default="", extension="*.*"):
      Arg.__init__(self, name)
      self.default = default
      self.extension = extension

class FileOpen(_Filename):
   pass

class FileSave(_Filename):
   pass

class Directory(_Filename):
   pass

class Radio(Arg):
   def __init__(self, name=None, radio_button=''):
      Arg.__init__(self, name)
      self.radio_button = radio_button

class Check(Arg):
   def __init__(self, name=None, check_box='', default=False, enabled=True):
      Arg.__init__(self, name)
      self.check_box = check_box
      self.default = default
      self.enabled = enabled

   def rest_repr(self):
      return 'bool'

Bool = Check

class Region(Class):
   def __init__(self, name=None):
      Class.__init__(self, name, None)

class RegionMap(Class):
   def __init__(self, name=None):
      Class.__init__(self, name, None)

class ImageInfo(Class):
   def __init__(self, name=None):
      Class.__init__(self, name, None)

# These can only be used as return values
class FloatVector(Class):
   def __init__(self, name=None, length=-1):
      import array
      Class.__init__(self, name, type(array.array('d')))
      self.length = length

# These can only be used as return values
class IntVector(Class):
   def __init__(self, name=None, length=-1):
      import array
      Class.__init__(self, name, type(array.array('i')))
      self.length = length

class ImageList(Class):
   def __init__(self, name=None):
      Class.__init__(self, name, None, True)

class Info(Arg):
   pass

class Wizard:
   def show(self, dialog):
      dialog_history = ['start', dialog]
      next_dialog = dialog
      while next_dialog != None:
         if next_dialog == 'start':
            return
         result = next_dialog.show(self.parent, self.locals, wizard=1)
         if not result is None:
            next_dialog = getattr(self, next_dialog.function)(*next_dialog.get_args())
            if next_dialog != dialog_history[-1]:
               dialog_history.append(next_dialog)
         else:
            next_dialog = dialog_history[-2]
            dialog_history = dialog_history[0:-1]
      self.done()

__all__ = 'Args Int Real Float String Class ImageType Choice FileOpen FileSave Directory Radio Check Region RegionMap ImageInfo FloatVector IntVector ImageList Info Wizard'.split()

mixin_modules = []
if has_gui.has_gui:
   from gamera.gui import args_gui
   mixin_modules.append(args_gui)
from gamera import args_wrappers
mixin_modules.append(args_wrappers)

for module in mixin_modules:
   for cls_name in __all__:
      if hasattr(module, cls_name):
         cls = locals()[cls_name]
         cls.__bases__ = tuple([getattr(module, cls_name)] + list(cls.__bases__))
         
