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
from util import word_wrap
import gzip, os, os.path, string, cStringIO
from gamera.symbol_table import SymbolTable
from xml.parsers import expat
import knn

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
      self.write_core(stream)

   def write_core(self, stream):
      self.write_symbol_table(stream, self.symbol_table)
      if isinstance(self.glyphs, core.ImageBase):
         self.write_glyph(stream, self.glyphs)
      else:
         self.write_glyphs(stream, self.glyphs)

   def write_symbol_table(self, stream, symbol_table, indent=0):
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

   def write_glyphs(self, stream, glyphs, indent=0):
      if len(glyphs):
         word_wrap(stream, '<glyphs>', indent)
         for glyph in glyphs:
            self.write_glyph(stream, glyph, indent + 1)
         word_wrap(stream, '</glyphs>', indent)

   def write_glyph(self, stream,  glyph, indent=0):
      if not isinstance(glyph, core.ImageBase):
         raise ValueError("'%s' is not an Image instance." % str(glyph))
      tag = ('<glyph ul_y="%s" ul_x="%s" nrows="%s" ncols="%s" classification-state="%s"' %
             (glyph.ul_y, glyph.ul_x, glyph.nrows, glyph.ncols,
              classification_state_to_name[glyph.classification_state]))
      if hasattr(glyph, 'source_image_name'):
         tag = tag + (' source_image_name="%s"' % glyph.source_image_name)
      tag = tag + ">"
      word_wrap(stream, tag, indent)
      indent += 1
      if len(glyph.id_name):
         for confidence, id in glyph.id_name:
            word_wrap(stream, '<id name="%s" confidence="%f"/>' %
                      (id, confidence), indent)
      word_wrap(stream, '<data>', indent)
      stream.write(glyph.to_rle())
      stream.write('\n')
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
      for key, val in glyph.properties.items():
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
      self.stream.write('<?xml version="1.0" ?>\n')
      self.stream.write('<gamera-database>\n')
      self.write_core(stream)
      self.stream.write('</gamera-database>\n')


################################################################################
# LOADING
################################################################################

class LoadXML:
   def __init__(self, classifier=None):
      self.classifier = classifier
      self.parser = expat.ParserCreate()
      self.parser.StartElementHandler = self.start_element_handler
      self.parser.EndElementHandler = self.end_element_handler
      self.start_elements = {}
      self.end_elements = {}
      self.start_elements_global = []
      self.end_elements_global = []
      self.setup_handlers()
      
   def return_parse(self):
      pass
   
   def setup_handlers(self):
      pass

   def parse_filename(self, filename):
      self.path = os.path.dirname(os.path.abspath(filename))
      try:
         fd = gzip.open(filename, 'r')
         return self.parse_stream(fd)
      except IOError:
         fd = open(filename, 'r')
         return self.parse_stream(fd)

   def parse_stream(self, stream):
      self.parser.ParseFile(stream)
      return self.return_parse()

   def parse_string(self, s):
      stream = cStringIO.StringIO(s)
      return self.parse_stream(stream)

   def add_start_element_handler(self, name, func):
      self.start_elements[name] = func

   def remove_start_element_handler(self, name):
      del self.start_elements[name]

   def add_global_start_element_handler(self, func):
      self.start_elements_global.append(func)

   def remove_global_start_element_handler(self, func):
      self.start_elements_global.remove(func)
      
   def start_element_handler(self, name, attributes):
      if self.start_elements_global != []:
         for x in self.start_elements_global:
            x(name, attributes)
      if self.start_elements.has_key(name):
         self.start_elements[name](attributes)

   def add_end_element_handler(self, name, func):
      self.end_elements[name] = func

   def remove_end_element_handler(self, name):
      del self.end_elements[name]

   def add_global_end_element_handler(self, func):
      self.end_elements_global.append(func)

   def remove_global_end_element_handler(self, func):
      self.end_elements_global.remove(func)

   def end_element_handler(self, name):
      if self.end_elements_global != []:
         for x in self.end_elements_global:
            x(name)
      if self.end_elements.has_key(name):
         self.end_elements[name]()

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
      self.add_start_element_handler('id', self.ths_id)
      self.add_start_element_handler('data', self.ths_data)
      self.add_end_element_handler('data', self.the_data)
      self.add_start_element_handler('property', self.ths_property)
      self.add_end_element_handler('property', self.the_property)
      self.glyphs = []

   def return_parse(self):
      return self.glyphs

   def ths_glyph(self, a):
      self.ul_y = int(a['ul_y'])
      self.ul_x = int(a['ul_x'])
      self.nrows = int(a['nrows'])
      self.ncols = int(a['ncols'])
      self.scaling = 1.0
      self.id_name = []
      self.properties = {}
      if a.has_key('classification-state'):
         self.classification_state = classification_state_to_number[a['classification-state']]
      else:
         self.classification_state = UNCLASSIFIED

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

   def ths_id(self, a):
      self.id_name.append((a['name'], float(a['confidence'])))

   def ths_features(self, a):
      if a.has_key('scaling'):
         self.scaling = eval(a['scaling'])

   def ths_data(self, a):
      self.data = ''
      self.parser.CharacterDataHandler = self.add_data

   def the_data(self):
      self.parser.CharacterDataHandler = None

   def add_data(self, data):
      self.data += data

   def ths_property(self, a):
      self.property_name = a['name']
      self.property_type = a['type']
      self.property_value = ''
      self.parser.CharacterDataHandler = self.add_property_value

   def the_property(self):
      if saveable_types.has_key(self.property_type):
         self.properties[self.property_name] = \
            saveable_types[self.property_type](self.property_value)
      else:
         self.properties[self.property_name] = self.data

   def add_property_value(self, data):
      self.property_value += data

