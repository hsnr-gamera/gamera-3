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

# Mixin classes to add wrapper-generation support for arguments list

import re
from enums import *
import util

class WrapperArg:
   arg_format = 'O'
   convert_from_PyObject = False
   multiple = False
   delete_cpp = False
   uid = 0

   def __getitem__(self, attr):
      return getattr(self, attr)
   
   def _get_symbol(self):
      return re.sub(r"\s", "_", self.name) + '_arg'
   symbol = property(_get_symbol)

   def _get_pysymbol(self):
      if self.convert_from_PyObject:
         return re.sub(r"\s", "_", self.name) + '_pyarg'
      else:
         return self.symbol
   pysymbol = property(_get_pysymbol)

   def declare(self):
      if self.name == None:
         self.name = "_%08d" % WrapperArg.uid
         WrapperArg.uid += 1
      if self.name == 'return' and hasattr(self, 'return_type'):
         result = "%s %s;\n" % (self.return_type, self.symbol)
      else:
         result = "%s %s;\n" % (self.c_type, self.symbol)
      if self.convert_from_PyObject:
         result += "PyObject* %s;\n" % self.pysymbol
      return result

   def from_python(self):
      return ""

   def _do_call(self, function, output_args):
      if function.feature_function:
         return "%s(%s, feature_buffer);" % (function.__name__, ", ".join(output_args))
      else:
         if function.return_type != None:
            result = function.return_type.symbol + " = "
         else:
            result = ""
         result += "%s(%s);\n" % (function.__name__, ", ".join(output_args))
         return result

   def call(self, function, args, output_args, limit_choices=None):
      if len(args):
         return args[0].call(function, args[1:], output_args + [self.symbol], limit_choices)
      else:
         return self._do_call(function, output_args + [self.symbol])

   def delete(self):
      if self.delete_cpp:
         return "delete %(symbol)s;" % self
      else:
         return ""
      
class Int(WrapperArg):
   arg_format = 'i'
   c_type = 'int'

   def to_python(self):
      return '%(pysymbol)s = PyInt_FromLong((long)%(symbol)s);' % self

Choice = Check = Int

class Float(WrapperArg):
   arg_format = 'd'
   c_type = 'double'

   def to_python(self):
      return '%(pysymbol)s = PyFloat_FromDouble((double)%(symbol)s);' % self

class String(WrapperArg):
   arg_format = 's'
   c_type = 'char*'
   return_type = 'std::string'

   def to_python(self):
      return "%(pysymbol)s = PyString_FromStringAndSize(%(symbol)s.data(), %(symbol)s.size());" % self

FileOpen = FileSave = Directory = String

class ImageType(WrapperArg):
   c_type = 'Image*'
   convert_from_PyObject = True
   multiple = True

   def from_python(self):
      return """if (!is_ImageObject(%(pysymbol)s)) {
          PyErr_SetString(PyExc_TypeError, "Argument '%(name)s' is not an image");
          return 0;
        }
        %(symbol)s = ((Image*)((RectObject*)%(pysymbol)s)->m_x);
        image_get_fv(%(pysymbol)s, &%(symbol)s->features, &%(symbol)s->features_len);
        """ % self

   def to_python(self):
      return "%(pysymbol)s = create_ImageObject(%(symbol)s);" % self

   def call(self, function, args, output_args, limit_choices=None):
      if function.image_types_must_match and limit_choices is not None:
         choices = self._get_choices_for_pixel_type(limit_choices)
      else:
         choices = self._get_choices()
      result = "switch(get_image_combination(%(pysymbol)s)) {\n" % self
      for choice, pixel_type in choices:
         result += "case %s:\n" % choice.upper()
         new_output_args = output_args + ["*((%s*)%s)" % (choice, self.symbol)]
         if len(args) == 0:
            result += self._do_call(function, new_output_args)
         else:
            if limit_choices is None:
               result += args[0].call(function, args[1:], new_output_args, pixel_type)
            else:
               result += args[0].call(function, args[1:], new_output_args, limit_choices)
         result += "break;\n"
      result += "default:\n"
      result += ('PyErr_Format(PyExc_TypeError, "The \'%s\' argument of \'%s\' can not have pixel type \'%%s\'.", get_pixel_type_name(%s));\nreturn 0;\n' %
                 (self.name, function.__name__, self.pysymbol))
      result += "}\n"
      return result

   def _get_choices(self):
      result = []
      pixel_types = list(self.pixel_types[:])
      pixel_types.sort()
      for type in pixel_types:
         result.extend(self._get_choices_for_pixel_type(type))
      return result

   def _get_choices_for_pixel_type(self, pixel_type):
      if pixel_type == ONEBIT:
         result = ["OneBitImageView", "Cc", "OneBitRleImageView", "RleCc"]
      else:
         result = [util.get_pixel_type_name(pixel_type) + "ImageView"]
      return [(x, pixel_type) for x in result]

class Rect(WrapperArg):
   c_type = 'Rect*'
   convert_from_PyObject = True

   def from_python(self):
      return """
      if (!is_RectObject(%(pysymbol)s)) {
        PyErr_SetString(PyExc_TypeError, "Argument '%(name)s' is not a Rect");
        return 0;
      }
      %(symbol)s = (((RectObject*)%(pysymbol)s)->m_x);
      """ % self

   def to_python(self):
      return "%(pysymbol)s = create_RectObject(%(symbol)s);" % self
            
class Region(WrapperArg):
   c_type = 'Region*'
   convert_from_PyObject = True

   def from_python(self):
      return """
      if (!is_RegionObject(%(pysymbol)s)) {
        PyErr_SetString(PyExc_TypeError, "Argument '%(name)s' is not a Region");
        return 0;
      }
      %(symbol)s = (Region*)((RectObect*)%(pysymbol)s)->m_x;""" % self

   def to_python(self):
      return """
      return_pyarg = create_RegionObject(*%(symbol)s);
      delete %(symbol)s;""" % self

class RegionMap(WrapperArg):
   c_type = 'RegionMap*'
   convert_from_PyObject = True

   def from_python(self):
      return """
      if (!is_RegionMapObject(%(pysymbol)s)) {
        PyErr_SetString(PyExc_TypeError, "Argument '%(name)s' is not a RegionMap.");
        return 0;
      }
      %(symbol)s = (RegionMap*)((RegionMapObect*)%(pysymbol)s)->m_x;""" % self

   def to_python(self):
      return """
      return_pyarg = create_RegionMapObject(*%(symbol)s);
      delete %(symbol)s;""" % self

class ImageList(WrapperArg):
   c_type = 'std::vector<Image*>'
   return_type = 'std::list<Image*>*'
   convert_from_PyObject = True
   
   def from_python(self):
      return """
          const char* type_error_%(name)s = "Argument '%(name)s' is not a list of images.";
          if (!PyList_Check(%(pysymbol)s)) {
            PyErr_SetString(PyExc_TypeError, type_error_%(name)s);
            return 0;
          }
          int %(symbol)s_size = PyList_GET_SIZE(%(pysymbol)s);
          %(symbol)s.resize(%(symbol)s_size);
          for (int i=0; i < %(symbol)s_size; ++i) {
            PyObject *element = PyList_GET_ITEM(%(pysymbol)s, i);
            if (!is_ImageObject(element)) {
              PyErr_SetString(PyExc_TypeError, type_error_%(name)s);
              return 0;
            }
            %(symbol)s[i] = (Image*)(((RectObject*)element)->m_x);
            image_get_fv(element, &%(symbol)s[i]->features,
                         &%(symbol)s[i]->features_len);
          }""" % self

   def to_python(self):
      return """
      %(pysymbol)s = PyList_New(%(symbol)s->size());
      std::list<Image*>::iterator it = %(symbol)s->begin();
      for (size_t i = 0; i < %(symbol)s->size(); ++i, ++it) {
        PyObject *item = create_ImageObject(*it);
        PyList_SetItem(%(pysymbol)s, i, item);
      }
      delete %(symbol)s;
      """ % self

class Class(WrapperArg):
   arg_format = 'O'
   c_type = 'PyObject*'
   convert_from_PyObject = True

   def from_python(self):
      return "%(symbol)s = %(pysymbol)s;" % self

   def to_python(self):
      return "%(pysymbol)s = %(symbol)s;" % self

class IntVector(WrapperArg):
   arg_format = 'O'
   convert_from_PyObject = True
   c_type = 'IntVector*'
   delete_cpp = True

   def from_python(self):
      return """
      %(symbol)s = IntVector_from_python(%(pysymbol)s);
      """ % self

   def to_python(self):
      return """
      %(pysymbol)s = IntVector_to_python(%(symbol)s);
      delete %(symbol)s;
      """ % self

class FloatVector(WrapperArg):
   arg_format = 'O'
   convert_from_PyObject = True
   c_type = 'FloatVector*'
   delete_cpp = True

   def from_python(self):
      return """
      %(symbol)s = FloatVector_from_python(%(pysymbol)s);
      """ % self

   def to_python(self):
      return """
      %(pysymbol)s = FloatVector_to_python(%(symbol)s);
      delete %(symbol)s;
      """ % self

class Pixel(WrapperArg):
   def from_python(self):
      return ''

   def call(self, function, args, output_args, limit_choices=None):
      if limit_choices is None:
         raise RuntimeError("You can not create a plugin that takes a Pixel argument that does not have a self type")
      pixel_type = limit_choices
      new_output_args = (output_args +
                         ["pixel_from_python<%sPixel>::convert(%s)" %
                          (util.get_pixel_type_name(pixel_type), self.pysymbol)])
      if len(args) == 0:
         return self._do_call(function, new_output_args)
      else:
         return args[0].call(function, args[1:], new_output_args, limit_choices)

class PointVector(WrapperArg):
   arg_format = 'O'
   convert_from_PyObject = True
   c_type = 'PointVector*'
   delete_cpp = True

   def from_python(self):
      return """
      %(symbol)s = PointVector_from_python(%(pysymbol)s);
      """ % self

   def to_python(self):
      return """
      %(pysymbol)s = PointVector_to_python(%(symbol)s);
      delete %(symbol)s;
      """ % self

class ImageInfo(WrapperArg):
   arg_format = "O";
   c_type = 'ImageInfo*'
   convert_from_PyObject = True

   def to_python(self):
      return "%(pysymbol)s = create_ImageInfoObject(%(symbol)s);""" % self

from gamera import args
args.mixin(locals(), "C++ Wrappers")
