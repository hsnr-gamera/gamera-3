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

   def call(self, function, args, output_args):
      if len(args):
         return args[0].call(function, args[1:], output_args + [self.symbol])
      else:
         return self._do_call(function, output_args + [self.symbol])
      
class Int(WrapperArg):
   arg_format = 'i'
   c_type = 'int'

   def to_python(self):
      return '%(pysymbol)s = Py_BuildValue("i", %(symbol)s);' % self

Choice = Check = Int

class Float(WrapperArg):
   arg_format = 'd'
   c_type = 'double'

   def to_python(self):
      return '%(pysymbol)s = Py_BuildValue("f", %(symbol)s);' % self

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
          PyErr_SetString(PyExc_TypeError, "Object is not an image as expected!");
          return 0;
        }
        %(symbol)s = ((Image*)((RectObject*)%(pysymbol)s)->m_x);
        image_get_fv(%(pysymbol)s, &%(symbol)s->features, &%(symbol)s->features_len);
        """ % self

   def to_python(self):
      return "%(pysymbol)s = create_ImageObject(%(symbol)s);" % self

   def call(self, function, args, output_args):
      choices = self._get_choices()
      result = "switch(get_image_combination(%(pysymbol)s)) {\n" % self
      for choice in choices:
         result += "case %s:\n" % choice.upper()
         new_output_args = output_args[:] + ["*((%s*)%s)" % (choice, self.symbol)]
         if len(args) == 0:
            result += self._do_call(function, new_output_args)
         else:
            result += args[0].call(function, args[1:], new_output_args)
         result += "break;\n"
      result += "default:\n"
      result += 'PyErr_SetString(PyExc_TypeError, "Image types do not match function signature.");\nreturn 0;\n'
      result += "}\n"
      return result

   def _get_choices(self):
      result = []
      for type in self.pixel_types:
         if type == ONEBIT:
            result.extend(["OneBitImageView", "OneBitRleImageView", "RleCc", "Cc"])
         else:
            result.append(util.get_pixel_type_name(type) + "ImageView")
      return result
            
class Region(WrapperArg):
   c_type = 'Region*'
   convert_from_PyObject = True

   def from_python(self):
      return """
      if (!is_RegionObject(%(pysymbol)s)) {
        PyErr_SetString(PyExc_TypeError, "Object is not a Region.");
        return 0;
      }
      %(symbol)s = (Region*)((RectObect*)%(pysymbol)s)->m_x;""" % self

   def to_python(self):
      return """
      result_pyarg = create_RegionObject(*%(symbol)s);
      delete %(symbol)s;""" % self

class RegionMap(WrapperArg):
   c_type = 'RegionMap*'
   convert_from_PyObject = True

   def from_python(self):
      return """
      if (!is_RegionMapObject(%(pysymbol)s)) {
        PyErr_SetString(PyExc_TypeError, "Object is not a RegionMap.");
        return 0;
      }
      %(symbol)s = (RegionMap*)((RegionMapObect*)%(pysymbol)s)->m_x;""" % self

   def to_python(self):
      return """
      result_pyarg = create_RegionMapObject(*%(symbol)s);
      delete %(symbol)s;""" % self

class ImageList(WrapperArg):
   c_type = 'std::vector<Image*>'
   return_type = 'std::list<Image*>*'
   convert_from_PyObject = True
   
   def from_python(self):
      return """if (!PyList_Check(%(pysymbol)s)) {
            PyErr_SetString(PyExc_TypeError, "Expected a list of images.");
            return 0;
          }
          int %(symbol)s_size = PyList_GET_SIZE(%(pysymbol)s);
          %(symbol)s.resize(%(symbol)s_size);
          for (int i=0; i < %(symbol)s_size; ++i) {
            PyObject *element = PyList_GET_ITEM(%(pysymbol)s, i);
            if (!is_ImageObject(element)) {
              PyErr_SetString(PyExc_TypeError, "Expected a list of images.");
              return 0;
            }
            %(symbol)s[i] = ((Image*)((RectObject*)element)->m_x);
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
   c_type = 'IntVector*'
   convert_from_PyObject = True;

   def to_python(self):
      return """
      PyObject* array_init = get_ArrayInit();
      if (array_init == 0)
        return 0;
      PyObject* str = PyString_FromStringAndSize(
        (char*)(&((*%(symbol)s)[0])),
        %(symbol)s->size() * sizeof(int));
      %(pysymbol)s = PyObject_CallFunction(
        array_init, "sO", "i", str);
      Py_DECREF(str);
      delete %(symbol)s;""" % self

class FloatVector(WrapperArg):
   arg_format = 'O'
   c_type = 'FloatVector*'
   convert_from_PyObject = True;
   
   def to_python(self):
      return """
      PyObject* array_init = get_ArrayInit();
      if (array_init == 0)
        return 0;
      PyObject* str = PyString_FromStringAndSize(
        (char*)(&((*%(symbol)s)[0])),
        %(symbol)s->size() * sizeof(double));
      %(pysymbol)s = PyObject_CallFunction(
        array_init, "sO", "d", str);
      Py_DECREF(str);
      delete %(symbol)s;""" % self

class ImageInfo(WrapperArg):
   arg_format = "O";
   c_type = 'ImageInfo*'
   convert_from_PyObject = True

   def to_python(self):
      return "%(pysymbol)s = create_ImageInfoObject(%(symbol)s);""" % self

