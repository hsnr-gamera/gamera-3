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
   def setup(self, *args, **kwargs):
      raise Exception("No GUI environment available.  Cannot display dialog.")
   
   def show(self, *args, **kwargs):
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

   def __init__(self, name):
      self.name = name
      if name is not None and not util.is_string_or_unicode(name):
         raise TypeError("'name' must be a string")
      self.has_default = False

   def __repr__(self):
      return "<" + self.__class__.__name__ + ">"

   def rest_repr(self, name=False):
      result = "``%s``" % self.__class__.__name__
      if name:
         result += " *%s*" % self.name
         if self.has_default:
            result += " = %s" % str(self.default)
      return result
   
class Int(Arg):
   def __init__(self, name=None, range=(-sys.maxint, sys.maxint), default=None):
      Arg.__init__(self, name)
      if not (util.is_sequence(range) and len(range) == 2 and
              type(range[0]) in (int, float) and type(range[1]) in (int, float)):
         raise TypeError("'range' must be a 2-tuple of numbers")
      self.rng = range
      if default is None:
         self.has_default = False
         self.default = 0
      else:
         self.has_default = True
         self.default = default
      if type(self.default) != int:
         raise TypeError("'default' must be an int")

   def rest_repr(self, name=False):
      result = "int"
      if self.rng != (-sys.maxint, sys.maxint):
         result += "(%d, %d)" % tuple([int(x) for x in self.rng])
      if name:
         result += " *%s*" % self.name
         if self.has_default:
            result += " = %d" % self.default
      return result

class Real(Arg):
   def __init__(self, name=None,
                range=(float(-sys.maxint), float(sys.maxint)),
                default=None):
      Arg.__init__(self, name)
      if not (util.is_sequence(range) and len(range) == 2 and
              type(range[0]) in (int, float) and type(range[1]) in (int, float)):
         raise TypeError("'range' must be a 2-tuple of numbers")
      self.rng = range
      if default is None:
         self.has_default = False
         self.default = 0.0
      else:
         self.has_default = True
         self.default = default
      if type(self.default) != float:
         raise TypeError("'default' must be a float")

   def rest_repr(self, name=False):
      result = "float"
      if self.rng != (-sys.maxint, sys.maxint):
         result += "(%.02f, %.02f)" % tuple([float(x) for x in self.rng])
      if name:
         result += " *%s*" % self.name
         if self.has_default:
            result += " = %.02f" % self.default
      return result

Float = Real

class Complex(Arg):
   def __init__(self, name=None, default=None):
      Arg.__init__(self, name)
      if default is None:
         self.has_default = False
         self.default = 0j
      else:
         self.has_default = True
         self.default = default

   def rest_repr(self, name=False):
      result = "complex"
      if name:
         result += " *%s*" % self.name
         if self.has_default:
            result += " = %s" % self.default
      return result

class String(Arg):
   def __init__(self, name=None, default=None):
      Arg.__init__(self, name)
      if default is None:
         self.has_default = False
         self.default = ''
      else:
         self.has_default = True
         self.default = default
      if not util.is_string_or_unicode(self.default):
         raise TypeError("'default' must be an int")

   def rest_repr(self, name=False):
      result = "str"
      if name:
         result += " *%s*" % self.name
         if self.has_default:
            result += " = %d" % self.default
      return result

class Class(Arg):
   def __init__(self, name=None, klass=None, list_of=False):
      Arg.__init__(self, name)
      self.klass = klass
      self.list_of = bool(list_of)

   def rest_repr(self, name=False):
      if self.klass is None:
         result = "``object``"
      else:
         result = "``%s``" % self.klass.__name__
      if name:
         result += " *%s*" % self.name
      return result

class ImageType(Arg):
   def __init__(self, pixel_types, name=None, list_of=False):
      import core
      Arg.__init__(self, name)
      if not util.is_sequence(pixel_types):
         pixel_types = (pixel_types,)
      if not util.is_homogenous_list(pixel_types, (int,)):
         raise TypeError("'pixel_types' must be a list of integers.")
      if not core is None:
         self.klass = core.ImageBase
      else:
         self.klass = None
      self.pixel_types = pixel_types
      self.list_of = bool(list_of)

   def rest_repr(self, name=False):
      result = '``Image`` [%s]' % '|'.join([util.get_pixel_type_name(x) for x in self.pixel_types])
      if name:
         result += " *%s*" % self.name
      return result

class Rect(Arg):
   def __init__(self, name=None, list_of=False):
      import core
      Arg.__init__(self, name)
      if not core is None:
         self.klass = core.Rect
      self.list_of = bool(list_of)
      
class Choice(Arg):
   def __init__(self, name=None, choices=[], default=None):
      Arg.__init__(self, name)
      if not util.is_string_or_unicode_list(choices):
         raise TypeError("'choices' must be a list of strings.")
      self.choices = choices
      if default is None:
         self.has_default = False
         self.default = 0
      else:
         self.has_default = True
         self.default = default
      if type(self.default) != int:
         raise TypeError("'default' must be an int")

   def rest_repr(self, name = False):
      result = '``Choice`` [%s]' % '|'.join(self.choices)
      if name:
         result += " *%s*" % self.name
         if self.has_default:
            result += " = %s" % self.choices[self.default]
      return result

class _Filename(Arg):
   def __init__(self, name=None, default="", extension="*.*"):
      Arg.__init__(self, name)
      if not util.is_string_or_unicode(default):
         raise TypeError("'default' must be a string")
      self.default = default
      if not util.is_string_or_unicode(extension):
         raise TypeError("'extension' must be a string")
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
   def __init__(self, name=None, check_box='', default=None, enabled=True):
      Arg.__init__(self, name)
      if not util.is_string_or_unicode(check_box):
         raise TypeError("'check_box' must be a string")
      self.check_box = check_box
      if default is None:
         self.has_default = False
         self.default = False
      else:
         self.has_default = True
         self.default = default
      self.default = bool(self.default)
      self.enabled = bool(enabled)

   def rest_repr(self, name=False):
      result = '``bool``'
      if name:
         result += " *%s*" % self.name
         if self.has_default:
            result += " = %s" % str(self.default)
      return result

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

class _Vector(Class):
   def __init__(self, name=None, klass=None, typecode=None, length=-1):
      Class.__init__(self, name, klass)
      self.typecode = typecode
      if type(length) != int:
         raise TypeError("'length' must be an int")
      self.length = length

class FloatVector(_Vector):
   def __init__(self, name=None, length=-1):
      _Vector.__init__(self, name, float, 'd', length)

class IntVector(_Vector):
   def __init__(self, name=None, length=-1):
      _Vector.__init__(self, name, int, 'i', length)

class ComplexVector(Class):
   def __init__(self, name=None, length=-1):
      Class.__init__(self, name, complex, True)
      if type(length) != int:
         raise TypeError("'length' must be an int")
      self.length = length

class ImageList(Class):
   def __init__(self, name=None):
      Class.__init__(self, name, None, True)

class Pixel(Class):
   """Uses these to pass pixel values into a plugin.  The
PixelType must always match the type of the "self" image."""
   def __init__(self, name=None):
      Class.__init__(self, name)

class Info(Arg):
   pass

class PointVector(Class):
   def __init__(self, name=None):
      Class.__init__(self, name, None, True)

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

__all__ = 'Args Int Real Float Complex String Class ImageType Rect Choice FileOpen FileSave Directory Radio Check Region RegionMap ImageInfo FloatVector IntVector ComplexVector ImageList Info Wizard Pixel PointVector _Vector'.split()

___mixin_locals = locals()
def mixin(module, name):
   for cls_name in __all__:
      cls = ___mixin_locals[cls_name]
      if module.has_key(cls_name):
         cls.__bases__ = tuple([module[cls_name]] + list(cls.__bases__))
   sys.stdout.write("\n")
