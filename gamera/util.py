#
# Copyright (C) 2001, 2002 Ichiro Fujinaga, Michael Droettboom,
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

import string, UserList, sys, re   ## Python standard
from types import *
from math import pow
from gamera.enums import *
from gamera import config

def is_sequence(obj):
  "Check if an object is a sequence."
  return type(obj) in (ListType, TupleType)

def make_sequence(obj):
  "Make an object into a sequence if it isn't one."
  if not type(obj) in (ListType, TupleType):
    return (obj,)
  return obj

def is_image_list(l):
  # TODO: This doesn't seem to work correctly
  ##   if not is_sequence(l):
  ##     return 0
  ##   for image in l:
  ##     if not isinstance(l, image_type):
  ##       return 0
  return 1

def is_string_or_unicode_list(l):
  if not is_sequence(l):
    return 0
  for s in l:
    if s not in (StringType, UnicodeType):
      return 0
  return 1

def is_homogeneous_image_list(l):
  "Determines if a list contains only images of the same pixel type"
  from gamera.core import ImageBase
  if not is_sequence(l) or not isinstance(l[0], ImageBase):
    return 0
  pixel_type = l[0].data.pixel_type
  for image in l[1:]:
    if (not isinstance(image, ImageBase) or
        image.data.pixel_type != pixel_type):
      return 0
  return 1
    
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
      return 1
  return 0  

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
  if i < 0:
    return -1
  return 1

class Set(list):
    "A list-like container that contains only unique elements."
    def append(self, item):
      if item not in self:
        list.append(self, item)
    def insert(self, i, item):
      if item not in self:
        list.insert(self, i, item)
    def extend(self, other):
      for item in other:
        self.append(item)

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
  enumerate = __builtins__.enumerate

_pixel_type_names = {ONEBIT:     "OneBit",
                     GREYSCALE:  "GreyScale",
                     GREY16:     "Grey16",
                     RGB:        "RGB",
                     FLOAT:      "Float"}
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
  update = kill = step = add_length = ___

class ProgressText:
  """A console-based progress bar."""
  width = 70
  
  def __init__(self, message, length=0):
    self._message = message
    self._starting = 1
    self._done = 0
    self._last_amount = 0
    self._num = 0
    self._den = length

  def add_length(self, l):
    self._den += l

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
        sys.stdout.write("#" * progress)
        sys.stdout.write("=" * left)
        sys.stdout.write("|\r")
        sys.stdout.flush()
      if num >= den:
        self._done = 1
        sys.stdout.write("\n")
        sys.stdout.flush()

def ProgressFactory(message, length=1):
  gui = config.get_option('__gui')
  if gui:
    return gui.ProgressBox(message, length)
  else:
    return ProgressText(message, length)
