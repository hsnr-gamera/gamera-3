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

import core, util
from util import word_wrap, ProgressFactory
import gzip, os, os.path, cStringIO
from gamera.symbol_table import SymbolTable
from xml.parsers import expat

classification_state_to_name = {
   core.UNCLASSIFIED: "UNCLASSIFIED",
   core.MANUAL:       "MANUAL",
   core.HEURISTIC:    "HEURISTIC",
   core.AUTOMATIC:    "AUTOMATIC" }

classification_state_to_number = {
   "UNCLASSIFIED":    core.UNCLASSIFIED,
   "MANUAL":          core.MANUAL,
   "HEURISTIC":       core.HEURISTIC,
   "AUTOMATIC":       core.AUTOMATIC }

saveable_types = {
   'int': int,
   'str': str,
   'float': float }

extensions = "XML files (*.xml)|*.xml*"

################################################################################
# SAVING
################################################################################

class WriteXML:
   def __init__(self, glyphs=[], symbol_table=[]):
      self.glyphs = glyphs
      self.symbol_table = symbol_table

   def write_filename(self, filename):
      self.filename = os.path.abspath(filename)
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
      progress = util.ProgressFactory("Writing XML...")
      self._write_core(stream, progress)

   def _write_core(self, stream, progress, indent=0):
      self._write_symbol_table(stream, self.symbol_table, indent=indent)
      if isinstance(self.glyphs, core.ImageBase):
         progress.update(1, 1)
         self._write_glyph(stream, self.glyphs, indent=indent)
      else:
         self._write_glyphs(stream, self.glyphs, progress, indent=indent)
      progress.update(1, 1)

   def _write_symbol_table(self, stream, symbol_table, indent=0):
      if (not isinstance(symbol_table, SymbolTable) and
          util.is_sequence(symbol_table)):
         symbols = symbol_table
      else:
         symbols = symbol_table.symbols.keys()
      if len(symbols):
         symbols.sort()
         word_wrap(stream, '<symbols>', indent)
         indent += 1
         for x in symbols:
            word_wrap(stream, '<symbol name="%s"/>' % str(x), indent)
         indent -= 1
         word_wrap(stream, '</symbols>', indent)

   def _write_glyphs(self, stream, glyphs, progress, indent=0):
      if len(glyphs):
         word_wrap(stream, '<glyphs>', indent)
         for i, glyph in util.enumerate(glyphs):
            self._write_glyph(stream, glyph, indent + 1)
            progress.update(i, len(glyphs))
         word_wrap(stream, '</glyphs>', indent)

   def _write_glyph(self, stream,  glyph, indent=0):
      if not isinstance(glyph, core.ImageBase):
         raise ValueError("'%s' is not an Image instance." % str(glyph))
      tag = ('<glyph uly="%s" ulx="%s" nrows="%s" ncols="%s">' %
             (glyph.ul_y, glyph.ul_x, glyph.nrows, glyph.ncols))
      word_wrap(stream, tag, indent)
      indent += 1
      word_wrap(
         stream,
         '<ids state="%s">' %
         classification_state_to_name[glyph.classification_state],
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
   def write_stream(self, stream=None, progress=None):
      if progress is None:
         progress = util.ProgressFactory("Writing XML...")
      if stream == None:
         return self.string()
      self.stream = stream
      self.stream.write('<?xml version="1.0" ?>\n')
      self.stream.write('<gamera-database>\n')
      self._write_core(stream, progress, indent=1)
      self.stream.write('</gamera-database>\n')


################################################################################
# LOADING
################################################################################

class LoadXML:
   def __init__(self):
      self._parser = expat.ParserCreate()
      self._parser.StartElementHandler = self._start_element_handler
      self._parser.EndElementHandler = self._end_element_handler
      self._start_elements = {}
      self._end_elements = {}
      self._start_elements_global = []
      self._end_elements_global = []
      self._stream_length = 0
      self.setup_handlers()
      
   def return_parse(self):
      pass
   
   def setup_handlers(self):
      pass

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
      return self.return_parse()

   def add_start_element_handler(self, name, func):
      self._start_elements[name] = func

   def remove_start_element_handler(self, name):
      del self._start_elements[name]

   def add_global_start_element_handler(self, func):
      self._start_elements_global.append(func)

   def remove_global_start_element_handler(self, func):
      self._start_elements_global.remove(func)
      
   def _start_element_handler(self, name, attributes):
      if self._stream_length:
         self._progress.update(self._stream.tell(), self._stream_length)
      else:
         self._progress.update(self._stream.tell() % 9, 10)
      for x in self._start_elements_global:
         x(name, attributes)
      if self._start_elements.has_key(name):
         self._start_elements[name](attributes)

   def add_end_element_handler(self, name, func):
      self._end_elements[name] = func

   def remove_end_element_handler(self, name):
      del self._end_elements[name]

   def add_global_end_element_handler(self, func):
      self._end_elements_global.append(func)

   def remove_global_end_element_handler(self, func):
      self._end_elements_global.remove(func)

   def _end_element_handler(self, name):
      if self._stream_length:
         self._progress.update(self._stream.tell(), self._stream_length)
      else:
         self._progress.update(self._stream.tell() % 9, 10)
      for x in self._end_elements_global:
         x(name)
      if self._end_elements.has_key(name):
         self._end_elements[name]()

class LoadXMLSymbolTable(LoadXML):
   def setup_handlers(self):
      self.symbol_table = SymbolTable()
      self.add_start_element_handler('symbol', self.ths_symbol)

   def return_parse(self):
      return self.symbol_table
   
   def ths_symbol(self, a):
      self.symbol_table.add(str(a['name']))

class LoadXMLGlyphs(LoadXML):
   def setup_handlers(self):
      self.add_start_element_handler('glyph', self.ths_glyph)
      self.add_end_element_handler('glyph', self.the_glyph)
      self.add_start_element_handler('features', self.ths_features)
      self.add_start_element_handler('ids', self.ths_ids)
      self.add_start_element_handler('id', self.ths_id)
      self.add_start_element_handler('data', self.ths_data)
      self.add_end_element_handler('data', self.the_data)
      self.add_start_element_handler('property', self.ths_property)
      self.add_end_element_handler('property', self.the_property)
      self.glyphs = []

   def return_parse(self):
      return self.glyphs

   def ths_glyph(self, a):
      self.ul_y = int(a['uly'])
      self.ul_x = int(a['ulx'])
      self.nrows = int(a['nrows'])
      self.ncols = int(a['ncols'])
      self.scaling = 1.0
      self.id_name = []
      self.properties = {}

   def the_glyph(self):
      glyph = core.Image(self.ul_y, self.ul_x, self.nrows, self.ncols,
                         core.ONEBIT, core.DENSE)
      glyph.from_rle(str(self.data.strip()))
      glyph.classification_state = self.classification_state
      glyph.id_name = self.id_name
      for key, val in self.properties.items():
         glyph.properties[key] = val
      glyph.scaling = self.scaling
      self.glyphs.append(glyph)

   def ths_ids(self, a):
      if a.has_key('state'):
         self.classification_state = classification_state_to_number[a['state']]
      else:
         self.classification_state = UNCLASSIFIED

   def ths_id(self, a):
      self.id_name.append((float(a['confidence']), str(a['name'])))

   def ths_features(self, a):
      if a.has_key('scaling'):
         self.scaling = float(a['scaling'])

   def ths_data(self, a):
      self.data = ''
      self._parser.CharacterDataHandler = self.add_data

   def the_data(self):
      self._parser.CharacterDataHandler = None

   def add_data(self, data):
      self.data += data

   def ths_property(self, a):
      self.property_name = a['name']
      self.property_type = a['type']
      self.property_value = ''
      self._parser.CharacterDataHandler = self.add_property_value

   def the_property(self):
      if saveable_types.has_key(self.property_type):
         self.properties[self.property_name] = \
            saveable_types[self.property_type](self.property_value)
      else:
         self.properties[self.property_name] = self.data

   def add_property_value(self, data):
      self.property_value += data

