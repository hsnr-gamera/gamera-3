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

import gzip, os, os.path, cStringIO
from xml.parsers import expat

import core, util, config
from util import word_wrap, ProgressFactory, is_image_list
from gamera.symbol_table import SymbolTable

config.add_option_default('encoding', 'utf-8')

_classification_state_to_name = {
   core.UNCLASSIFIED: "UNCLASSIFIED",
   core.MANUAL:       "MANUAL",
   core.HEURISTIC:    "HEURISTIC",
   core.AUTOMATIC:    "AUTOMATIC" }
def classification_state_to_name(state):
   return _classification_state_to_name[state]

_classification_state_to_number = {
   "UNCLASSIFIED":    core.UNCLASSIFIED,
   "MANUAL":          core.MANUAL,
   "HEURISTIC":       core.HEURISTIC,
   "AUTOMATIC":       core.AUTOMATIC }
def classification_state_to_number(state):
   return _classification_state_to_number[state]

_saveable_types = {
   'int':   int,
   'str':   str,
   'float': float }

extensions = "XML files (*.xml)|*.xml*"

################################################################################
# SAVING
################################################################################

class WriteXML:
   def __init__(self, glyphs=[], symbol_table=[], groups=[]):
      if not is_image_list(glyphs):
         raise TypeError(
            "glyphs argument to WriteXML must be a list of images.")
      self.glyphs = glyphs
      if (not isinstance(symbol_table, SymbolTable) and
          not util.is_string_or_unicode_list(symbol_table)):
         raise TypeError(
            "symbol_table argument to WriteXML must be of type SymbolTable or a list of strings.")
      self.symbol_table = symbol_table
      self.groups = groups

   def write_filename(self, filename):
      if not os.path.exists(os.path.split(os.path.abspath(filename))[0]):
         raise ValueError(
            "Cannot create a file at '%s'." %
            os.path.split(os.path.abspath(filename))[0])
      if filename.endswith('gz'):
         fd = gzip.open(filename, 'w')
      else:
         fd = open(filename, 'w')
      self.write_stream(fd)

   def string(self):
      stream = cStringIO.StringIO()
      self.write_stream(stream)
      return stream.getvalue()

   def write_stream(self, stream=None):
      if stream == None:
         return self.string()
      self._write_core(stream)

   def _write_core(self, stream, indent=0):
      self._write_symbol_table(stream, self.symbol_table, indent=indent)
      if isinstance(self.glyphs, core.ImageBase):
         self._write_glyph(stream, self.glyphs, indent=indent)
      else:
         self._write_glyphs(stream, self.glyphs, indent=indent)
      self._write_groups(stream, self.groups, indent=indent)

   def _write_symbol_table(self, stream, symbol_table, indent=0):
      encoding = config.get_option('encoding')
      if (not isinstance(symbol_table, SymbolTable) and
          util.is_string_or_unicode_list(symbol_table)):
         symbols = symbol_table
      else:
         symbols = symbol_table.symbols.keys()
      if len(symbols):
         symbols.sort()
         word_wrap(stream, '<symbols>', indent)
         indent += 1
         for x in symbols:
            word_wrap(stream, '<symbol name="%s"/>' % x.encode(encoding), indent)
         indent -= 1
         word_wrap(stream, '</symbols>', indent)

   def _write_glyphs(self, stream, glyphs, indent=0):
      if len(glyphs):
         progress = ProgressFactory("Writing glyphs to XML...")
         try:
            word_wrap(stream, '<glyphs>', indent)
            indent += 1
            for i, glyph in util.enumerate(glyphs):
               self._write_glyph(stream, glyph, indent)
               progress.update(i, len(glyphs))
            indent -= 1
            word_wrap(stream, '</glyphs>', indent)
            progress.update(1, 1)
         except Exception, e:
            progress.update(1, 1)
            raise e

   def _write_groups(self, stream, groups, indent=0):
      if len(groups):
         progress = ProgressFactory("Writing groups to XML...")
         try:
            word_wrap(stream, '<groups>', indent)
            indent += 1
            for i, group in enumerate(groups):
               word_wrap(stream, '<group id="%s">' % group[0], indent)
               indent += 1
               for glyph in group[1]:
                  self._write_glyph(stream, glyph, indent)
               indent -= 1
               word_wrap(stream, '</group>', indent)
               progress.update(i, len(groups))
            indent -= 1
            word_wrap(stream, '</groups>', indent)
         except Exception, e:
            progress.update(1, 1)
            raise e

   def _write_glyph(self, stream,  glyph, indent=0):
      tag = ('<glyph uly="%s" ulx="%s" nrows="%s" ncols="%s">' %
             (glyph.ul_y, glyph.ul_x, glyph.nrows, glyph.ncols))
      word_wrap(stream, tag, indent)
      indent += 1
      word_wrap(
         stream,
         '<ids state="%s">' %
         classification_state_to_name(glyph.classification_state),
         indent)
      indent += 1
      for confidence, id in glyph.id_name:
         word_wrap(stream, '<id name="%s" confidence="%f"/>' %
                   (id, confidence), indent)
      indent -= 1
      word_wrap(stream, '</ids>', indent)
      word_wrap(stream, '<data>', indent)
      word_wrap(stream, glyph.to_rle(), indent+1)
      word_wrap(stream, '</data>', indent)
      if len(glyph.feature_functions):
         word_wrap(stream,
                   '<features scaling="%s">' % str(glyph.scaling),
                   indent)
         indent += 1
         feature_no = 0
         for name, function in glyph.feature_functions:
            word_wrap(stream,
                      '<feature name="%s">' % name,
                      indent)
            length = function.return_type.length
            word_wrap(stream,
                      [str(x) for x in
                       glyph.features[feature_no:feature_no+length]],
                      indent + 1)
            feature_no += length
            word_wrap(stream,
                      '</feature>',
                      indent)
         indent -= 1
         word_wrap(stream, '</features>', indent)
      properties = glyph.properties.items()
      properties.sort()
      for key, val in properties:
         if not val is None:
            word_wrap(stream, '<property name="%s" type="%s">%s</property>' %
                      (key, type(val).__name__, str(val)), indent)
      indent -= 1
      word_wrap(stream, '</glyph>', indent)

class WriteXMLFile(WriteXML):
   def write_stream(self, stream=None):
      if stream == None:
         return self.string()
      self.stream = stream
      encoding = config.get_option('encoding')
      self.stream.write('<?xml version="1.0" encoding="%s"?>\n' % encoding)
      self.stream.write('<gamera-database>\n')
      self._write_core(stream, indent=1)
      self.stream.write('</gamera-database>\n')


################################################################################
# LOADING
################################################################################

class LoadXML:
   def __init__(self, parts = ['symbol_table', 'glyphs', 'groups']):
      self._parser = expat.ParserCreate()
      self._parser.StartElementHandler = self._start_element_handler
      self._parser.EndElementHandler = self._end_element_handler
      self._start_elements = {}
      self._end_elements = {}
      self._start_elements_global = []
      self._end_elements_global = []
      self._stream_length = 0
      self._parts = parts
      self._setup_handlers()

   def try_type_convert(self, dictionary, key, typename, tagname):
      try:
         return typename(dictionary[key])
      except KeyError:
         raise ValueError(
            "XML ValueError: <%s> tag does not have a required element '%s'." %
            (tagname, key))
      except ValueError:
         raise ValueError(
            'XML ValueError: <%s %s="%s" ... is not of valid type' %
            (tagname, key, dictionary[key]))
      
   def parse_filename(self, filename):
      self._stream_length = os.stat(filename).st_size
      try:
         fd = gzip.open(filename, 'r')
         return self.parse_stream(fd)
      except IOError:
         del self._progress
         fd = open(filename, 'r')
         return self.parse_stream(fd)

   def parse_string(self, s):
      self._stream_length = len(s)
      stream = cStringIO.StringIO(s)
      return self.parse_stream(stream)

   def parse_stream(self, stream):
      self._stream = stream
      self._progress = util.ProgressFactory("Loading XML...")
      self._parser.ParseFile(stream)
      self._progress.update(1, 1)
      return self
   
   def add_start_element_handler(self, name, func):
      self._start_elements[name] = func

   def remove_start_element_handler(self, name):
      del self._start_elements[name]

   def add_global_start_element_handler(self, func):
      self._start_elements_global.append(func)

   def remove_global_start_element_handler(self, func):
      self._start_elements_global.remove(func)
      
   def _start_element_handler(self, name, attributes):
      for x in self._start_elements_global:
         x(name, attributes)
      try:
         self._start_elements[name](attributes)
      except KeyError:
         pass

   def add_end_element_handler(self, name, func):
      self._end_elements[name] = func

   def remove_end_element_handler(self, name):
      del self._end_elements[name]

   def add_global_end_element_handler(self, func):
      self._end_elements_global.append(func)

   def remove_global_end_element_handler(self, func):
      self._end_elements_global.remove(func)

   def _end_element_handler(self, name):
      for x in self._end_elements_global:
         x(name)
      try:
         self._end_elements[name]()
      except KeyError:
         pass
      if self._stream_length:
         self._progress.update(self._stream.tell(), self._stream_length)
      else:
         self._progress.update(self._stream.tell() % 49, 50)

   def _append_glyph_to_glyphs(self, glyph):
      self.glyphs.append(glyph)

   def _append_glyph_to_group(self, glyph):
      self._group.append(glyph)

   def _setup_handlers(self):
      self.symbol_table = SymbolTable()
      self.glyphs = []
      self.groups = []
      if 'symbol_table' in self._parts:
         self.add_start_element_handler('symbols', self._ths_symbols)
         self.add_end_element_handler('symbols', self._the_symbols)
      if 'glyphs' in self._parts:
         self.add_start_element_handler('glyphs', self._ths_glyphs)
         self.add_end_element_handler('glyphs', self._the_glyphs)
      if 'groups' in self._parts:
         self.add_start_element_handler('groups', self._ths_groups)
         self.add_end_element_handler('groups', self._the_groups)

   def _ths_symbols(self, a):
      self.add_start_element_handler('symbol', self._ths_symbol)

   def _the_symbols(self):
      self.remove_start_element_handler('symbol')
   
   def _ths_symbol(self, a):
      self.symbol_table.add(str(a['name']))

   def _ths_glyphs(self, a):
      self._append_glyph = self._append_glyph_to_glyphs
      self.add_start_element_handler('glyph', self._ths_glyph)
      self.add_end_element_handler('glyph', self._the_glyph)
      self.add_start_element_handler('features', self._ths_features)
      self.add_start_element_handler('ids', self._ths_ids)
      self.add_start_element_handler('id', self._ths_id)
      self.add_start_element_handler('data', self._ths_data)
      self.add_end_element_handler('data', self._the_data)
      self.add_start_element_handler('property', self._ths_property)
      self.add_end_element_handler('property', self._the_property)

   def _the_glyphs(self):
      self.remove_start_element_handler('glyph')
      self.remove_end_element_handler('glyph')
      self.remove_start_element_handler('features')
      self.remove_start_element_handler('ids')
      self.remove_start_element_handler('id')
      self.remove_start_element_handler('data')
      self.remove_end_element_handler('data')
      self.remove_start_element_handler('property')
      self.remove_end_element_handler('property')

   def _ths_glyph(self, a):
      self._ul_y = self.try_type_convert(a, 'uly', int, 'glyph')
      self._ul_x = self.try_type_convert(a, 'ulx', int, 'glyph')
      self._nrows = self.try_type_convert(a, 'nrows', int, 'glyph')
      self._ncols = self.try_type_convert(a, 'ncols', int, 'glyph')
      self._scaling = 1.0
      self._id_name = []
      self._properties = {}

   def _the_glyph(self):
      glyph = core.Image(self._ul_y, self._ul_x, self._nrows, self._ncols,
                         core.ONEBIT, core.DENSE)
      glyph.from_rle(str(self._data.strip()))
      glyph.classification_state = self._classification_state
      self._id_name.sort()
      glyph.id_name = self._id_name
      for key, val in self._properties.items():
         glyph.properties[key] = val
      glyph.scaling = self._scaling
      self._append_glyph(glyph)

   def _ths_ids(self, a):
      self._classification_state = self.try_type_convert(
         a, 'state', classification_state_to_number, 'ids')

   def _ths_id(self, a):
      confidence = self.try_type_convert(
         a, 'confidence', float, 'id')
      name = self.try_type_convert(
         a, 'name', unicode, 'id')
      self._id_name.append((confidence, name.encode()))

   def _ths_features(self, a):
      self._scaling = self.try_type_convert(
         a, 'scaling', float, 'features')

   def _ths_data(self, a):
      self._data = u''
      self._parser.CharacterDataHandler = self.add_data

   def _the_data(self):
      self._parser.CharacterDataHandler = None

   def add_data(self, data):
      self._data += data

   def _ths_property(self, a):
      self._property_name = self.try_type_convert(
         a, 'name', str, 'property')
      self._property_type = self.try_type_convert(
         a, 'type', str, 'property')
      self._property_value = ''
      self._parser.CharacterDataHandler = self.add_property_value

   def _the_property(self):
      if saveable_types.has_key(self._property_type):
         self._properties[self._property_name] = \
            saveable_types[self._property_type](self._property_value)
      else:
         self._properties[self._property_name] = self._data.encode()

   def add_property_value(self, data):
      self._property_value += data

   def setup_handlers(self):
      self.add_start_element_handler('groups', self._ths_groups)
      self.add_end_element_handler('groups', self._the_groups)
      self.groups = []

   def _ths_groups(self, a):
      self._append_glyph = self._append_glyph_to_group
      self.add_start_element_handler('group', self._ths_group)
      self.add_end_element_handler('group', self._the_group)
      self.add_start_element_handler('glyph', self._ths_glyph)
      self.add_end_element_handler('glyph', self._the_glyph)
      self.add_start_element_handler('features', self._ths_features)
      self.add_start_element_handler('ids', self._ths_ids)
      self.add_start_element_handler('id', self._ths_id)
      self.add_start_element_handler('data', self._ths_data)
      self.add_end_element_handler('data', self._the_data)
      self.add_start_element_handler('property', self._ths_property)
      self.add_end_element_handler('property', self._the_property)

   def _the_groups(self):
      self.remove_start_element_handler('group')
      self.remove_end_element_handler('group')
      self.remove_start_element_handler('glyph')
      self.remove_end_element_handler('glyph')
      self.remove_start_element_handler('features')
      self.remove_start_element_handler('ids')
      self.remove_start_element_handler('id')
      self.remove_start_element_handler('data')
      self.remove_end_element_handler('data')
      self.remove_start_element_handler('property')
      self.remove_end_element_handler('property')

   def _ths_group(self, a):
      self._group = []
      self.group_id = self.try_type_convert(a, 'id', str, 'group')

   def _the_group(self):
      self.groups.append((self.group_id, self._group))

def glyphs_from_xml(filename):
   """Return a list of glyphs from an xml file"""
   return LoadXML().parse_filename(filename).glyphs

def glyphs_with_features_from_xml(filename, feature_functions = None):
   """Return a list of glyphs with features from an xml file"""
   import features
   glyphs = LoadXML().parse_filename(filename).glyphs
   features.generate_features_list(glyphs, feature_functions)
   return glyphs

def glyphs_to_xml(filename, glyphs):
   """Save a list of glyphs to an xml file"""
   WriteXMLFile(glyphs).write_filename(filename)
