# -*- mode: python; indent-tabs-mode: nil; tab-width: 3 -*-
# vim: set tabstop=3 shiftwidth=3 expandtab:
#
# Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom,
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

from __future__ import generators

import string, sys, traceback, re, warnings   ## Python standard
from types import *
from math import pow
from gamera.enums import *
from gamera.gui import has_gui
from gamera.config import config
from gamera.backport import sets, textwrap

config.add_option(
   "-p", "--progress-bar", action="store_true",
   help="[console] Display textual progress bars on stdout")

def is_sequence(obj):
   "Check if an object is a sequence."
   return type(obj) in (list, tuple)

def make_sequence(obj):
   "Make an object into a sequence if it isn't one."
   if not is_sequence(obj):
      return [obj]
   return obj

def is_image_list(l):
   from gamera.core import ImageBase
   if not is_sequence(l):
      return False
   for image in l:
      if not isinstance(image, ImageBase):
         return False
   return True

def is_string_or_unicode(s):
   return type(s) in (StringType, UnicodeType)

def is_homogeneous_image_list(l):
   "Determines if a list contains only images of the same pixel type"
   from gamera.core import ImageBase
   if (not is_sequence(l)) or (not isinstance(l[0], ImageBase)):
      return False
   pixel_type = l[0].data.pixel_type
   for image in l[1:]:
      if (not isinstance(image, ImageBase) or
          image.data.pixel_type != pixel_type):
         return False
   return True

def is_homogenous_list(l, t):
   if not is_sequence(l):
      return False
   for e in l:
      if type(e) not in t:
         return False
   return True

def is_string_or_unicode_list(l):
   return is_homogenous_list(l, (StringType, UnicodeType))

def replace_prefix(s, a, b):
   "replaces the prefix a in s with b"
   if s.startswith(a):
      return b + s[len(a):]
   else:
      return s

def fast_cmp(x, y):
   "Fast sorting on pre-cached values"
   return cmp(x.sort_cache, y.sort_cache)

def rangeeq(a, b, range, mod = 0):
   """Returns true if the difference of *a* and *b* is within a given *range*.
If *mod* is given, than the difference is always considered less than *mod*."""
   if mod == 0:
      return abs(b - a) <= range
   else:
      return min((b - a) % mod,
                 (a - b) % mod) <= range

def constains_instance(gl, klass):
   """Returns true if list *gl* contains an instance of *klass*"""
   for g in gl:
      if isinstance(g, klass):
         return True
   return False

def sublists(list):
   l = len(list) - 1
   m = int(pow(2, l - 1))
   for x in range(len(list)):
      for i in range(m):
         set = []
         k = 1
         last_part = []
         set.append(last_part)
         for j in range(l):
            last_part.append(list[j])
            if i & k:
               last_part = []
               set.append(last_part)
            k *= 2
         yield set
      list = [list[-1]] + list[0:-1]

def string2identifier(str):
   """Defines how illegal variable names are converted to legal ones."""
   # TODO: Not very robust.
   if len(str):
      name = re.sub('\-|/|\.|\ ', '_', str, 0)
      if name[0] in string.digits:
         name = "_" + name
      return name
   else:
      return "DEFAULT"

def sign(i):
   "Returns the sign of a number"
   return cmp(i, 0)

class Set(list):
   "A list-like container that contains only unique elements."
   def __init__(self, list=[]):
      list.__init__(self)
      self._dict = {}
      self.extend(list)
   def append(self, item):
      if not self._dict.has_key(item):
         list.append(self, item)
         self._dict[item] = None
   def insert(self, i, item):
      if not self._dict.has_key(item):
         list.insert(self, i, item)
         self._dict[item] = None
   def extend(self, other):
      for item in other:
         if not self._dict.has_key(item):
            list.append(self, item)
            self._dict[item] = None

class SetDictionary(dict):
   def __getitem__(self, key):
      return dict.setdefault(self, key, Set())

_byte_steps = (('Gb', float(1 << 30), float(1 << 30) * 1.1),
               ('Mb', float(1 << 20), float(1 << 20) * 1.1),
               ('kb', float(1 << 10), float(1 << 10) * 1.1),
               ('bytes', 1, -1))
def pretty_print_byte_size(bytes):
   "Prints byte lengths in a nice human-readable way"
   for step in _byte_steps:
      if bytes > step[2]:
         return "%.2f %s" % (float(bytes) / step[1], step[0])
   return 'error (negative!)'

if float(sys.version[0:3]) < 2.3:
   def enumerate(collection):
      """Backport to 2.2 of Python 2.3's enumerate function."""
      i = 0
      it = iter(collection)
      while 1:
         yield(i, it.next())
         i += 1
   __builtins__['enumerate'] = enumerate
   __builtins__['True'] = 1
   __builtins__['False'] = 0
else:
   enumerate = __builtins__['enumerate']

_pixel_type_names = {ONEBIT:     "OneBit",
                     GREYSCALE:  "GreyScale",
                     GREY16:     "Grey16",
                     RGB:        "RGB",
                     FLOAT:      "Float",
                     COMPLEX:    "Complex"}
def get_pixel_type_name(type_):
   return _pixel_type_names[type_]

def group_list(list, group_size):
   """Groups the list into fixed-size chunks."""
   groups = []
   for i in range(0, len(list), group_size):
      groups.append(list[i:min(i+group_size, len(list))])
   return groups

def permute_list(alist, level=0):
   """Yields all permutations of a given list."""
   index, copy, printing = level, alist[:], level+1 == len(alist)
   while 1:
      if printing:
         yield copy
      else:
         for x in permute_list(copy, level + 1):
            yield x
      if index != 0:
         copy[index-1], copy[index] = copy[index], copy[index-1]
      index -= 1
      if index < 0:
         break

def combinations(seed):
   lengths = [len(x) for x in seed]
   if 0 in lengths:
      return
   length = len(seed)
   count = [0] * len(seed)
   copy = [x[0] for x in seed]
   while 1:
      yield copy
      i = 0
      while 1:
         count[i] += 1
         if count[i] == lengths[i]:
            count[i] = 0
            copy[i] = seed[i][0]
            i += 1
            if i == length:
               return
         else:
            copy[i] = seed[i][count[i]]
            break

def word_wrap(stream, l, indent=0, width=78):
   """Writes to a stream with word wrapping.  indent is the size of the
   indent for every line.  width is the maximum width of the text."""
   indent *= 2
   width -= indent
   indent_spaces = ' ' * (indent)
   if is_sequence(l):
      l = ' '.join([str(x) for x in l])
   i = 0
   p = 0
   while i != -1:
      stream.write(indent_spaces)
      if len(l) - p < width:
         stream.write(l[p:])
         stream.write('\n')
         break
      else:
         i = l.rfind(' ', p, p + width)
         if i == -1:
            stream.write(l[p:])
         stream.write(l[p:i])
      stream.write('\n')
      p = i + 1

def encode_binary(s):
   import zlib, binascii
   return binascii.b2a_base64(zlib.compress(s))

def decode_binary(s):
   import zlib, binascii
   return zlib.decompress(binascii.a2b_base64(s))

class ProgressNothing:
   """A progress bar that actually does nothing (for batch mode.)"""
   def __init__(self, message, length=0):
      pass
   def ___(*args):
      pass
   update = kill = step = add_length = set_length = ___

class ProgressText:
   """A console-based progress bar."""
   width = 64

   def __init__(self, message, length=0):
      self._message = message
      self._starting = 1
      self._done = 0
      self._last_amount = 0
      self._num = 0
      if length == 0:
         self._den = 1
      else:
         self._den = length

   def add_length(self, l):
      self._den += l

   def set_length(self, l):
      self._den = l

   def step(self):
      self._num += 1
      self.update(self._num, self._den)

   def kill(self):
      self.update(1, 1)

   def update(self, num, den):
      if self._starting:
         sys.stdout.write(self._message)
         sys.stdout.write("\n")
         self._starting = 0
      if not self._done:
         progress = int((float(num) / float(den)) * self.width)
         if progress != self._last_amount:
            self._last_amount = progress
            left = self.width - progress
            sys.stdout.write("|")
            sys.stdout.write("=" * progress)
            sys.stdout.write("-" * left)
            sys.stdout.write("|\r")
            sys.stdout.flush()
         if num >= den:
            self._done = 1
            sys.stdout.write("\n")
            sys.stdout.flush()

def ProgressFactory(message, length=1):
   if has_gui.gui != None:
      return has_gui.gui.ProgressBox(message, length)
   elif config.get("progress_bar"):
      return ProgressText(message, length)
   else:
      return ProgressNothing(message, length)

# A regular expression used to determine the amount of space to
# remove.  It looks for the first sequence of spaces immediately
# following the first newline, or at the beginning of the string.
_find_dedent_regex = re.compile("(?:(?:\n\r?)|^)( *)\S")
# A cache to hold the regexs that actually remove the indent.
_dedent_regex = {}
def dedent(s):
    # This implementation has a somewhat obtuse use of regular
    # expressions.  However, this function accounted for almost 30% of
    # matplotlib startup time, so it is worthy of optimization at all
    # costs.
    if not s:      # includes case of s is None
        return ''

    match = _find_dedent_regex.match(s)
    if match is None:
        return s

    # This is the number of spaces to remove from the left-hand side.
    nshift = match.end(1) - match.start(1)
    if nshift == 0:
        return s

    # Get a regex that will remove *up to* nshift spaces from the
    # beginning of each line.  If it isn't in the cache, generate it.
    unindent = _dedent_regex.get(nshift, None)
    if unindent is None:
        unindent = re.compile("\n\r? {0,%d}" % nshift)
        _dedent_regex[nshift] = unindent

    result = unindent.sub("\n", s).strip()
    return result

class CallbackObject:
   def __init__(self):
      self._callbacks = {}
      self.is_dirty = False

   def add_callback(self, alert, callback):
      category = self._callbacks.setdefault(alert, [])
      if callback not in category:
         category.append(callback)

   def remove_callback(self, alert, callback):
      category = self._callbacks.get(alert, [])
      if callback in category:
         category.remove(callback)

   def trigger_callback(self, alert, *args):
      category = self._callbacks.get(alert, [])
      for callback in category:
         callback(*args)
      self.is_dirty = True

class CallbackList(list, CallbackObject):
   def __init__(self, initlist=[]):
      list.__init__(self, initlist)
      CallbackObject.__init__(self)

   def add_callback(self, alert, callback):
      CallbackObject.add_callback(self, alert, callback)
      if alert == 'length_change':
         callback(len(self))
      elif alert == 'add':
         callback(self)

   def clear(self):
      CallbackList.__delslice__(self, 0, len(self))

   def __del__(self):
      for i in self:
         self.trigger_callback('remove', [i])
      self.trigger_callback('length_change', len(self))

   def __setitem__(self, i, item):
      if abs(i) < len(self):
         self.trigger_callback('remove', [self[i]])
         self.trigger_callback('add', [item])
      list.__setitem__(self, i, item)

   def __delitem__(self, i):
      if abs(i) < len(self):
         self.trigger_callback('remove', [self[i]])
      list.__delitem__(self, i)
      self.trigger_callback('length_change', len(self))

   def __setslice__(self, i, j, other):
      i = max(i, 0); j = max(j, 0)
      if i < len(self) and j < len(self) + 1:
         self.trigger_callback('remove', self[i:j])
      self.trigger_callback('add', other)
      list.__setslice__(self, i, j, other)

   def __delslice__(self, i, j):
      i = max(i, 0); j = max(j, 0)
      if i < len(self) and j < len(self) + 1:
         self.trigger_callback('remove', self[i:j])
      list.__delslice__(self, i, j)
      self.trigger_callback('length_change', len(self))

   def __iadd__(self, other):
      self.trigger_callback('add', other)
      list.__iadd__(self, other)
      self.trigger_callback('length_change', len(self))

   def append(self, item):
      self.trigger_callback('add', [item])
      list.append(self, item)
      self.trigger_callback('length_change', len(self))

   def insert(self, i, item):
      if abs(i) < len(self):
         self.trigger_callback('add', [item])
      list.insert(self, i, item)
      self.trigger_callback('length_change', len(self))

   def pop(self, i=-1):
      if abs(i) < len(self):
         item = list.pop(self, i)
      self.trigger_callback('remove', [item])
      self.trigger_callback('length_change', len(self))

   def remove(self, item):
      if item in self:
         self.trigger_callback('remove', [item])
      list.remove(self, item)
      self.trigger_callback('length_change', len(self))

   def extend(self, other):
      self.trigger_callback('add', other)
      list.extend(self, other)
      self.trigger_callback('length_change', len(self))

class CallbackSet(sets.Set, CallbackObject):
   def __init__(self, initset=None):
      sets.Set.__init__(self, initset)
      CallbackObject.__init__(self)

   def __del__(self):
      self.trigger_callback('remove', self)
      self.trigger_callback('length_change', len(self))

   def add_callback(self, alert, callback):
      CallbackObject.add_callback(self, alert, callback)
      if alert == 'length_change':
         callback(len(self))
      elif alert == 'add':
         callback(self)

   def add(self, element):
      alert = element not in self
      sets.Set.add(self, element)
      if alert:
         self.trigger_callback('add', [element])
         self.trigger_callback('length_change', len(self))
   append = add

   def remove(self, element):
      alert = element in self
      sets.Set.remove(self, element)
      if alert:
         self.trigger_callback('remove', [element])
         self.trigger_callback('length_change', len(self))

   def discard(self, element):
      if element in self:
         sets.Set.remove(self, element)
         self.trigger_callback('remove', [element])
         self.trigger_callback('length_change', len(self))

   def pop(self):
      result = sets.Set.pop(self)
      self.trigger_callback('remove', [result])
      self.trigger_callback('length_change', len(self))

   def clear(self):
      self.trigger_callback('remove', self)
      sets.Set.clear(self)
      self.trigger_callback('length_change', len(self))

   def update(self, iterable):
      def iter():
         for i in iterable:
            if i not in self:
               yield i
      self.trigger_callback('add', iter())
      sets.Set.update(self, iterable)
      self.trigger_callback('length_change', len(self))

   def difference_update(self, iterable):
      def iter():
         for i in iterable:
            if i in self:
               yield i
      self.trigger_callback('remove', iter())
      sets.Set.difference_update(self, iterable)
      self.trigger_callback('length_change', len(self))

   def symmetric_difference_update(self, iterable):
      def remove_iter():
         for i in iterable:
            if i in self:
               yield i
      def add_iter():
         for i in iterable:
            if not i in self:
               yield i
      self.trigger_callback('remove', remove_iter())
      self.trigger_callback('add', add_iter())
      self.Set.symmetric_difference_update(self, iterable)
      self.trigger_callback('length_change', len(self))

   def intersection_update(self, iterable):
      def iter():
         for i in self:
            if not i in iterable:
               yield i
      self.trigger_callback('remove', iter())
      self.Set.intersection_update(self, iterable)
      self.trigger_callback('length_change', len(self))

   def union_update(self, other):
      self.update(other)

   extend = update

   def __setstate__(self, data):
      CallbackObject.__init__(self)
      self.trigger_callback('remove', self)
      sets.Set.__setstate__(self, data)
      self.trigger_callback('add', self)
      self.trigger_callback('length_change', len(self))

def get_file_extensions(mode):
   from gamera import plugin
   import os.path
   from gamera.backport import sets
   methods = plugin.methods_flat_category("File")
   methods = [y for x, y in methods if x.startswith(mode) and not x.endswith("image")]

   if len(methods) == 0:
      raise RuntimeError("There don't seem to be any imported plugins that can %s files.  Try running init_gamera() or explictly loading file i/o plugins such as tiff_support and png_support." % mode)
   extensions = sets.Set()
   types = []
   for method in methods:
      wildcards = ";".join(["*.%s;*.%s" %
                            (ext.lower(), ext.upper()) for ext in method.exts])
      type = "%s Files (%s)|%s" % (method.exts[0].upper(), wildcards, wildcards)
      types.append(type)
      # We have to cast the lists to sets here to make Python 2.3.0 happy.
      extensions.update(sets.Set(method.exts))
      extensions.update(sets.Set([x.upper() for x in method.exts]))
   all_extensions = ";".join(["*.%s" % x for x in extensions])
   types.insert(0, "All images (%s)|%s" % (all_extensions, all_extensions))
   types.append("All files (*.*)|*.*")
   return "|".join(types)

class _ImageFileExtensionFinder:
   def __init__(self, mode):
      self.mode = mode
   def __str__(self):
      return get_file_extensions(self.mode)
load_image_file_extension_finder = _ImageFileExtensionFinder("load")
save_image_file_extension_finder = _ImageFileExtensionFinder("save")

_warnings_history = {}
def __warn_deprecated__(message, other_filename=None, other_lineno=None,
                        from_cpp=False):
   stack = traceback.extract_stack()
   if not from_cpp:
      filename, lineno, scope, call = stack[-3]
   else:
      filename, lineno, scope, call = stack[-2]
   if other_filename is None:
      other_filename, other_lineno, scope, call = stack[-2]
   key = "%s:%d:%s:%d" % (filename, lineno, other_filename, other_lineno)
   if filename.endswith("code.py"):
      filename = "<shell>"
      lineno = 0
   if not _warnings_history.has_key(key):
      _warnings_history[key] = None
      warnings.warn_explicit("\n" + message,
                             DeprecationWarning, filename, lineno)

warn_deprecated = __warn_deprecated__
