# vi:set tabsize=3:
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

import gzip, os, os.path, cStringIO
import warnings
from weakref import proxy
from xml.parsers import expat

import core, util
from util import word_wrap, ProgressFactory, is_image_list
from gamera.symbol_table import SymbolTable
from config import config

config.add_option(
   "", "--xml-encoding", action="store", default="utf-8",
   help='[xml] Character encoding to use when saving XML files')

GAMERA_XML_FORMAT_VERSION = 2.0

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

extensions = "XML files (*.xml;*.xml.gz)|*.xml;*.xml.gz"

class XMLError(Exception):
   pass

################################################################################
# SAVING
################################################################################

class WriteXML:
   def __init__(self, glyphs=[], symbol_table=[], with_features=True):
      self.glyphs = glyphs
      if (not (isinstance(symbol_table, SymbolTable) or
               util.is_string_or_unicode_list(symbol_table))):
         raise XMLError(
            "symbol_table argument to WriteXML must be of type SymbolTable or a list of strings.")
      self.symbol_table = symbol_table
      self.with_features = with_features

   def write_filename(self, filename, with_features=None):
      if not with_features is None:
         self.with_features = with_features
      if not os.path.exists(os.path.split(os.path.abspath(filename))[0]):
         raise XMLError(
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
      progress = util.ProgressFactory("Saving XML...", 1)
      try:
         self._write_symbol_table(stream, self.symbol_table, indent=indent)
         if isinstance(self.glyphs, core.ImageBase):
            progress.add_length(1)
            self._write_glyph(stream, self.glyphs, indent=indent)
            progress.step()
         else:
            progress.add_length(len(self.glyphs))
            self._write_glyphs(stream, self.glyphs, progress, indent=indent)
      finally:
         progress.kill()

   def _write_symbol_table(self, stream, symbol_table, indent=0):
      encoding = config.get("xml_encoding")
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

   def _write_glyphs(self, stream, glyphs, progress, indent=0):
      if len(glyphs):
         word_wrap(stream, '<glyphs>', indent)
         indent += 1
         for i, glyph in util.enumerate(glyphs):
            self._write_glyph(stream, glyph, indent)
            progress.step()
         indent -= 1
         word_wrap(stream, '</glyphs>', indent)

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
      feature_functions = glyph.feature_functions[0]
      if self.with_features and len(feature_functions):
         word_wrap(stream,
                   '<features scaling="%s">' % str(glyph.scaling),
                   indent)
         indent += 1
         feature_no = 0
         for name, function in feature_functions:
            word_wrap(stream,
                      '<feature name="%s">' % name,
                      indent)
            length = function.return_type.length
            word_wrap(stream,
                      [x for x in
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
      encoding = config.get("xml_encoding")
      self.stream.write('<?xml version="1.0" encoding="%s"?>\n' % encoding)
      self.stream.write('<gamera-database version="%s">\n' %
                        str(GAMERA_XML_FORMAT_VERSION))
      self._write_core(stream, indent=1)
      self.stream.write('</gamera-database>\n')


################################################################################
# LOADING
################################################################################

class LoadXML:
   def __init__(self, parts = ['symbol_table', 'glyphs']):
      self._start_elements = {}
      self._end_elements = {}
      self._stream_length = 0
      self._parts = parts
      self._progress_value = 0

   def try_type_convert(self, dictionary, key, typename, tagname):
      try:
         return typename(dictionary[key])
      except KeyError:
         raise XMLError(
            "XML ValueError: <%s> tag does not have a required attribute '%s'." %
            (tagname, key))
      except TypeError:
         raise XMLError(
            'XML ValueError: <%s %s="%s" ... is not of the correct type' %
            (tagname, key, dictionary[key]))
      
   def parse_filename(self, filename):
      try:
         self._stream_length = os.stat(filename).st_size
      except OSError, e:
         raise XMLError(str(e))
      if filename.endswith('gz'):
         fd = gzip.open(filename, 'r')
      else:
         fd = open(filename, 'r')
      try:
         return self.parse_stream(fd)
      except Exception, e:
         raise XMLError(str(e))

   def parse_string(self, s):
      self._stream_length = len(s)
      stream = cStringIO.StringIO(s)
      return self.parse_stream(stream)

   def parse_stream(self, stream):
      self._setup_handlers()
      self._parser = expat.ParserCreate()
      self._parser.StartElementHandler = self._start_element_handler
      self._parser.EndElementHandler = self._end_element_handler
      self._stream = stream
      self._progress = util.ProgressFactory("Loading XML...", self._stream_length)
      try:
         try:
            self._parser.ParseFile(stream)
         except expat.ExpatError, e:
            raise
      finally:
         self._progress.kill()
         self._remove_handlers()
         self._parser.StartElementHandler = None
         self._parser.EndElementHandler = None
         del self._parser
      return self
   
   def add_start_element_handler(self, name, func):
      self._start_elements[name] = func

   def remove_start_element_handler(self, name):
      del self._start_elements[name]

   def _start_element_handler(self, name, attributes):
      try:
         self._start_elements[name](attributes)
      except KeyError:
         pass

   def add_end_element_handler(self, name, func):
      self._end_elements[name] = func

   def remove_end_element_handler(self, name):
      del self._end_elements[name]

   def _end_element_handler(self, name):
      try:
         self._end_elements[name]()
      except KeyError:
         pass

   def _update_progress(self):
      if self._stream_length:
         self._progress.update(self._stream.tell(), self._stream_length)
      else:
         self._progress_value += 1
         self._progress_value %= 50
         self._progress.update(self._progress_value, 50)

   def _append_glyph_to_glyphs(self, glyph):
      self.glyphs.append(glyph)

   def _setup_handlers(self):
      self.symbol_table = SymbolTable()
      self.glyphs = []
      self.add_start_element_handler('gamera-database', self._tag_start_gamera_database)
      self.add_end_element_handler('gamera-database', self._tag_end_gamera_database)

   def _remove_handlers(self):
      self.remove_start_element_handler('gamera-database')
      self.remove_end_element_handler('gamera-database')

   def _tag_start_gamera_database(self, a):
      try:
         version = self.try_type_convert(a, 'version', float, 'gamera-database')
      except:
         version = 1.0
      if version < GAMERA_XML_FORMAT_VERSION:
         raise XMLError(
            "The XML file is an old version, which can not be read " +
            "by this version of Gamera.")
      if 'symbol_table' in self._parts:
         self.add_start_element_handler('symbols', self._tag_start_symbols)
         self.add_end_element_handler('symbols', self._tag_end_symbols)
      if 'glyphs' in self._parts:
         self.add_start_element_handler('glyphs', self._tag_start_glyphs)
         self.add_end_element_handler('glyphs', self._tag_end_glyphs)

   def _tag_end_gamera_database(self):
      if 'symbol_table' in self._parts:
         self.remove_start_element_handler('symbols')
         self.remove_end_element_handler('symbols')
      if 'glyphs' in self._parts:
         self.remove_start_element_handler('glyphs')
         self.remove_end_element_handler('glyphs')

   def _tag_start_symbols(self, a):
      self.add_start_element_handler('symbol', self._tag_start_symbol)

   def _tag_end_symbols(self):
      self.remove_start_element_handler('symbol')
   
   def _tag_start_symbol(self, a):
      self.symbol_table.add(str(a['name']))
      self._update_progress()

   def _tag_start_glyphs(self, a):
      self._append_glyph = self._append_glyph_to_glyphs
      self.add_start_element_handler('glyph', self._tag_start_glyph)
      self.add_end_element_handler('glyph', self._tag_end_glyph)
      self.add_start_element_handler('features', self._tag_start_features)
      self.add_start_element_handler('ids', self._tag_start_ids)
      self.add_start_element_handler('id', self._tag_start_id)
      self.add_start_element_handler('data', self._tag_start_data)
      self.add_end_element_handler('data', self._tag_end_data)
      self.add_start_element_handler('property', self._tag_start_property)
      self.add_end_element_handler('property', self._tag_end_property)

   def _tag_end_glyphs(self):
      self._append_glyph = None
      for element in 'glyph features ids id data property'.split():
         self.remove_start_element_handler(element)
      for element in 'glyph data property'.split():
         self.remove_end_element_handler(element)

   def _tag_start_glyph(self, a):
      self._ul_y = self.try_type_convert(a, 'uly', int, 'glyph')
      self._ul_x = self.try_type_convert(a, 'ulx', int, 'glyph')
      self._nrows = self.try_type_convert(a, 'nrows', int, 'glyph')
      self._ncols = self.try_type_convert(a, 'ncols', int, 'glyph')
      self._scaling = 1.0
      self._id_name = []
      self._properties = {}
      self._data = None
      self._classification_state = core.UNCLASSIFIED

   def _tag_end_glyph(self):
      glyph = core.Image(core.Point(self._ul_x, self._ul_y),
                         core.Dim(self._ncols, self._nrows),
                         core.ONEBIT, core.DENSE)
      if not self._data is None:
         glyph.from_rle(str(u''.join(self._data)))
      glyph.classification_state = self._classification_state
      self._id_name.sort()
      glyph.id_name = self._id_name
      for key, val in self._properties.items():
         glyph.properties[key] = val
      glyph.scaling = self._scaling
      self._append_glyph(glyph)
      if not len(self.glyphs) & 0xf:
         self._update_progress()

   def _tag_start_ids(self, a):
      self._classification_state = self.try_type_convert(
         a, 'state', classification_state_to_number, 'ids')

   def _tag_start_id(self, a):
      confidence = self.try_type_convert(
         a, 'confidence', float, 'id')
      name = self.try_type_convert(
         a, 'name', unicode, 'id')
      self._id_name.append((confidence, name.encode()))

   def _tag_start_features(self, a):
      self._scaling = self.try_type_convert(
         a, 'scaling', float, 'features')

   def _tag_start_data(self, a):
      self._data = []
      self._parser.CharacterDataHandler = self.add_data

   def _tag_end_data(self):
      self._parser.CharacterDataHandler = None

   def add_data(self, data):
      self._data.append(data)

   def _tag_start_property(self, a):
      self._property_name = self.try_type_convert(
         a, 'name', str, 'property')
      self._property_type = self.try_type_convert(
         a, 'type', str, 'property')
      self._property_value = []
      self._parser.CharacterDataHandler = self.add_property_value

   def _tag_end_property(self):
      data = u''.join(self._property_value)
      if _saveable_types.has_key(self._property_type):
         self._properties[self._property_name] = \
            _saveable_types[self._property_type](data)
      else:
         self._properties[self._property_name] = data
      self._parser.CharacterDataHandler = None

   def add_property_value(self, data):
      self._property_value.append(data)

def glyphs_from_xml(filename, feature_functions = None):
   """**glyphs_from_xml** (FileOpen *filename*, Features *feature_functions* = ``None``)

Return a list of glyphs from a Gamera XML file."""
   glyphs = LoadXML().parse_filename(filename).glyphs
   if not feature_functions is None:
      from gamera.plugins import features
      features.generate_features_list(glyphs, feature_functions)
   return glyphs

def glyphs_with_features_from_xml(filename, feature_functions = None):
   """**glyphs_with_features_from_xml** (FileOpen *filename*, Features *feature_functions* = ``None``)

Loads glyphs from a Gamera XML file, and then generates features
for all of those glyphs.  The set of features can be specified with the
*feature_functions* argument (which defaults to all features)."""
   warnings.warn("Use glyphs_from_xml with a feature descriptor instead of glyphs_with_features_from_xml.", DeprecationWarning)
   if feature_functions == None:
      feature_functions = 'all'
   return glyphs_from_xml(filename, feature_functions)

def glyphs_to_xml(filename, glyphs, with_features=True):
   """**glyphs_to_xml** (FileSave *filename*, *with_features* = ``True``)

Saves the given list of glyphs to a Gamera XML file.

*with_features*
  When set to ``True``, features generated on the image are saved to the XML file.
"""
   WriteXMLFile(glyphs, with_features=with_features).write_filename(filename) 	 
 
class StripTag:
   # This is a ridiculous implementation that probably deserves some
   # explanation.  I want to use a full-fledged XML parser, (even though this
   # could probably be done with Regex's or something), for robustness.
   # However, when parsing a file with Expat, there's no way of getting the
   # original XML back, and .GetInputContext() seems to be broken in Py2.3.3
   # AFAICT.  Therefore, what this does is two passes:
   #   1) go through storing the indices of start and end tags
   #   2) use that information to generate an excerpted file
   def __init__(self, input_filename, output_filename, tag_name):
      if input_filename == output_filename:
         raise ValueError("Input and output filenames must be different.")
      self._input_filename = input_filename
      self._output_filename = output_filename
      self._tag_name = tag_name
      self.parse()
      self.excerpt()

   def open_input_file(self):
      try:
         if self._input_filename.endswith('gz'):
            self._input = gzip.open(self._input_filename, 'r')
         else:
            self._input = open(self._input_filename, 'r')
      except Exception, e:
         raise XMLError("Couldn't open input file '%s': %s" %
                        (self._input_filename, str(e)))

   def open_output_file(self):
      try:
         if self._output_filename.endswith('gz'):
            self._output = gzip.open(self._output_filename, 'w')
         else:
            self._output = open(self._output_filename, 'w')
      except Exception, e:
         raise XMLError("Couldn't open output file '%s': %s" %
                        (self._output_filename, str(e)))

   def parse(self):
      self.open_input_file()
      self._parser = expat.ParserCreate()
      self._parser.StartElementHandler = self.start_element_handler
      self._parser.EndElementHandler = self.end_element_handler
      self._parser.DefaultHandler = self.default_handler
      self._last_was_end = False
      self._indices = []
      try:
         try:
            self._parser.ParseFile(self._input)
         except expat.ExpatError, e:
            raise
      finally:
         self._parser.StartElementHandler = None
         self._parser.EndElementHandler = None
         del self._parser
      self._input.close()

   def start_element_handler(self, name, attributes):
      if self._last_was_end:
         self._indices.append(self._parser.ErrorByteIndex)
      self._last_was_end = False
      if name == self._tag_name:
         self._indices.append(self._parser.ErrorByteIndex)

   def end_element_handler(self, name):
      if self._last_was_end:
         self._indices.append(self._parser.ErrorByteIndex)
      if name == self._tag_name:
         self._last_was_end = True
      else:
         self._last_was_end = False

   def default_handler(self, data):
      if self._last_was_end:
         self._indices.append(self._parser.ErrorByteIndex)
      self._last_was_end = False

   def excerpt(self):
      self.open_input_file()
      self.open_output_file()
      pos = 0
      for i in range(0, len(self._indices), 2):
         start, end = self._indices[i], self._indices[i+1]
         self._output.write(self._input.read(start - pos))
         pos = end
         self._input.seek(pos)
      self._output.write(self._input.read())
      self._input.close()
      self._output.close()

def strip_features(input_filename, output_filename):
   """**strip_features** (FileOpen *input_filename*, FileSave *output_filename*)

Strips the features from a Gamera XML file.  Provided mainly to reduce filesizes for
files created before saving features was an option.

*input_filename*
  The input Gamera XML filename

*output_filename*
  The output Gamera XML filename"""
   StripTag(input_filename, output_filename, 'features')
   
   
