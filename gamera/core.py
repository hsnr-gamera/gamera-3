# vi:set tabsize=3:
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


""" This module contains the core parts of Gamera including all of the image
representation types, the dimension types, abstract mappings of values
to regions on an image (ImageMap), image loading, and feature generation.
 
Everything in this module can be used without the Gui to facilitate
batch processing. It contains the following classes:

ImageBase - common functionality for all of the image types.
Image - images.
SubImage - a view on part of an image.
Cc - a non-rectangular view of part of an image (the view area
     is marked with labels in the image).

Additionally this module contains the following functions:

load_image - load an image from a file.
image_info - get information about an image file.
display_multi - display a list of images in a grid-like window.
init_gamera - parse the gamera options and load all of the plugins.
"""

# Python standard library
from new import instancemethod
from array import array
from types import *

# import the classification states
from gameracore import UNCLASSIFIED, AUTOMATIC, HEURISTIC, MANUAL
# import the pixel types
from gameracore import ONEBIT, GREYSCALE, GREY16, RGB, FLOAT
from enums import ALL
# import the storage types
from gameracore import DENSE, RLE
# import some of the basic types
from gameracore import ImageData, Size, Dimensions, Point, \
     Rect, Region, RegionMap, ImageInfo
# import gamera.gameracore for subclassing
import gameracore
# from classify import InteractiveClassifier, NonInteractiveClassifier

# from gamera.classify import *
import paths, util, config    # Gamera-specific

class SegmentationError(Exception):
   pass

######################################################################

def load_image(filename, compression = DENSE):
   """Load an image from a file optionally using the given type of
   compression for the image object."""
   from gamera.plugins import tiff_support
   image = tiff_support.load_tiff(filename, compression)
   image.name = filename
   return image

def image_info(filename):
   """Retrieve an ImageInfo about about a given filename. The ImageInfo
   object gives information such as the type (color, b&w, onebit),
   the bit-depth, resolution, size, etc."""
   import tiff_support
   return tiff_support.tiff_info(filename)

def display_multi(list):
   """Display a list of images in a grid-like window."""
   gui = config.options.__.gui
   if gui:
      # If it's not a list, we'll get errors, so make it one
      if not util.is_sequence(list):
         list = [list]
      return gui.ShowImages(list)

# Used to cache the list of all features
all_features = None

class ImageBase:
   """Base class for all of the image objects. This class contains
   common functionality for all of the image types including
   the necessary infrastructure to support dynamically added plugins."""
   # Stores the categorized set of methods.  Bears no relationship
   # to __methods__
   _methods = {}

   class Properties(dict):
      def __getitem__(self, attr):
         return dict.get(self, attr, None)
      def __getattr__(self, attr):
         return self.__getitem__(attr)
      def __setattr__(self, attr, value):
         return dict.__setitem__(self, attr, value)
   
   def __init__(self):
      self.name = "Untitled"
      self._display = None
      self.last_display = None
      self.properties = self.Properties()
      # Keep a list of tuples of (feature_name, feature_function) that
      # have already been generated for this Image
      self.feature_functions = []

   def __del__(self):
      if self._display:
         self._display.close()

   def __getstate__(self):
      """Extremely basic pickling support for use in testing.
      Note that there is no unpickling support."""
      dict = {}
      for key in self._members_for_menu:
         dict[key] = getattr(self, key)
      dict['encoded_data'] = util.encode_binary(
         self.to_string())
      return dict

   def add_plugin_method(cls, plug, func, category=None):
      """Add a plugin method to all Image instances.
      plug -- subclass of PluginFunction describing the function.
      func -- raw function pointer.
      category -- menu category that the method should be placed under.
        Categories may be nested using '/' (i.e. "General/Specific")
      """
      methods = cls._methods
      if not func is None:
         func = instancemethod(func, None, gameracore.Image)
         setattr(cls, plug.__name__, func)
      if not category is None:
        for type in plug.self_type.pixel_types:
          if not methods.has_key(type):
            methods[type] = {}
          start = methods[type]
          for subcategory in category.split('/'):
            if not start.has_key(subcategory):
              start[subcategory] = {}
            start = start[subcategory]
          start[plug.__name__] = plug
   add_plugin_method = classmethod(add_plugin_method)

   def pixel_type_name(self):
      """Returns a string representing the pixle type of the image. Can be
      used for display purposes. Pixel types determine the type of data
      stored in an image - i.e. RGB, GreyScale, Onebit."""
      return util.get_pixel_type_name(self.data.pixel_type)
   pixel_type_name = property(pixel_type_name)

   _storage_format_names = {DENSE:  "Dense",
                            RLE:    "RLE"}
   
   def storage_format_name(self):
      """Returns a string representing the storage type of the image. Can
      be used for display purposes. Storage formats determing the way
      image data is stored in an image - i.e. Dense (uncompressed) or
      RLE (runlength-encoded)."""
      return self._storage_format_names[self.data.storage_format]
   storage_format_name = property(storage_format_name)
   
   _members_for_menu = ('pixel_type_name',
                        'storage_format_name',
                        'ul_x', 'ul_y', 'nrows', 'ncols',
                        'resolution', 'memory_size', 'label', 
                        'classification_state', 'properties')
   def members_for_menu(self):
      """Returns a list of members (and their values) for convenient feedback
      for the user."""
      return ["%s: %s" % (x, getattr(self, x))
              for x in self._members_for_menu
              if hasattr(self, x)]

   def methods_for_menu(self):
      """Returns a list of methods (in nested dictionaries by categories) for
      building user interfaces."""
      methods = {}
      for type in (ALL, self.data.pixel_type):
         if self._methods.has_key(type):
            self._methods_sub(methods, self._methods[type])
      return methods

   def _methods_sub(cls, dest, source):
     for key, val in source.items():
       if type(val) == DictType:
         if not dest.has_key(key):
           dest[key] = {}
         cls._methods_sub(dest[key], val)
       else:
         dest[key] = val
   _methods_sub = classmethod(_methods_sub)

   def methods_flat_category(cls, category, pixel_type=None):
      methods = {}
      for type in (ALL, pixel_type):
         if cls._methods.has_key(type):
            cls._methods_sub(methods, cls._methods[type])
      if methods.has_key(category):
         return cls._methods_flatten(methods[category])
      else:
         return []
   methods_flat_category = classmethod(methods_flat_category)

   def _methods_flatten(cls, mat):
     list = []
     for key, val in mat.items():
       if type(val) == DictType:
         list.extend(cls._methods_flatten(val))
       else:
         list.append((key, val))
     return list
   _methods_flatten = classmethod(_methods_flatten)

   def load_image(filename, compression=DENSE):
      return load_image(filename, compression)
   load_image = staticmethod(load_image)

   def memory_size(self):
      return util.pretty_print_byte_size(self.data.bytes)
   memory_size = property(memory_size)

   def set_display(self, _display):
      self._display = _display

   def display(self):
      "Displays the image in its own window."
      gui = config.options.__.gui
      if gui:
         if self._display:
            self._display.set_image(self)
         else:
            self.set_display(
               gui.ShowImage(self, self.name,
                             owner=self))
      self.last_display = "normal"
      return self._display

   def display_ccs(self):
      gui = config.options.__.gui
      if gui:
         if self._display:
            self._display.set_image(self, "color_ccs")
         else:
            self.set_display(
               gui.ShowImage(self, self.name, "color_ccs",
                             owner=self))
      self.last_display = "normal"
      return self._display

   # Displays the image in its own window, coloring the connected-
   # component labels
   def display_cc(self, cc):
      """Displays the image in its own window, coloring the connected
      components."""
      gui = config.options.__.gui
      # If the last thing displayed was something other than a cc
      # do a normal display to clear and refresh the window
      if self.last_display != "cc" or not gui or not self._display:
         self.display()
      # If the last thing displayed was a cc, clear only those ccs
      # This is much faster than clearing the whole image
      if self.last_display == "cc":
         for c in self.cc:
            self._display.clear_highlight_cc(c)
      if not util.is_sequence(cc):
         cc = [cc]
      self.cc = []
      for c in cc:
         if isinstance(c, Cc) or isinstance(c, SubImage):
            self._display.highlight_cc(c)
            self.cc.append(c)
      # This will adjust the scroll bars so the cc will be visible
      self._display.focus(self.cc)
      self.last_display = "cc"

   def display_children(self):
      if hasattr(self, 'children_images') and self.children_images != []:
         display_multi(self.children_images)

   def display_false_color(self):
      gui = config.options.__.gui
      if gui:
         if self._display:
            self._display.set_image(self, "to_false_color")
         else:
            self.set_display(
               gui.ShowImage(self, self.name, "to_false_color",
                             owner=self))
      self.last_display = "normal"
      return self._display

   def unclassify(self):
      self.id_name = []
      self.classification_state = UNCLASSIFIED

   def classify_manual(self, id_name):
      if type(id_name) == StringType:
         id_name = [(1.0, id_name)]
      elif type(id_name) == ListType:
         id_name.sort()
      else:
         raise TypeError("id_name must be a string or a list")
      self.id_name = id_name
      self.classification_state = MANUAL

   def classify_automatic(self, id_name):
      if type(id_name) == StringType:
         id_name = [(0.0, id_name)]
      elif type(id_name) == ListType:
         id_name.sort()
      else:
         raise TypeError("id_name must be a string or a list")
      self.id_name = id_name
      self.classification_state = AUTOMATIC

   def classify_heuristic(self, id_name):
      if type(id_name) == StringType:
         id_name = [(0.5, id_name)]
      elif type(id_name) == ListType:
         id_name.sort()
      else:
         raise TypeError("id_name must be a string or a list")
      self.id_name = id_name
      self.classification_state = HEURISTIC

   def get_main_id(self):
      if self.classification_state == UNCLASSIFIED or not len(self.id_name):
         return 'UNCLASSIFIED'
      return self.id_name[0][1]

   def get_confidence(self):
      """Returns the confidence of main id"""
      if self.classification_state == UNCLASSIFIED:
         return -1.0
      return self.id_name[0][0]

   def has_id_name(self, name):
      for confidence, id_name in self.id_name:
         if name == id_name:
            return 1
      return 0

   def subimage(self, offset_y, offset_x, nrows, ncols):
      """Create a SubImage from this Image (or SubImage)."""
      if hasattr(self, "label"):
         return Cc(self, self.label, offset_y, offset_x, nrows, ncols)
      else:
         return SubImage(self, offset_y, offset_x, nrows, ncols)

   def get_feature_functions(cls, features='all'):
      global all_features
      if all_features is None:
         all_features = cls.methods_flat_category('Features', ONEBIT)
         all_features.sort()
      if features == 'all' or features is None:
         functions = all_features
         return functions
      features = util.make_sequence(features)
      features.sort()
      all_strings = 1
      for feature in features:
         if not type(feature) == StringType:
            all_strings = 0
            break
      if not all_strings:
         import plugin
         all_functions = 1
         for feature in features:
            if not (type(feature) == TupleType and
                    type(feature[0]) == StringType and
                    issubclass(feature[1], plugin.PluginFunction)):
               all_functions = 0
               break
         if not all_functions:
            raise ValueError(
               "'%s' is not a valid way to specify a list of features."
               % features)
         else:
            return features
      else:
         functions = []
         for feature in features:
            found = 0
            for i in all_features:
               if feature == i[0]:
                  functions.append(i)
                  found = 1
                  break
            if not found:
               raise ValueError("'%s' is not a known feature function.")
         functions.sort()
         return functions
   get_feature_functions = classmethod(get_feature_functions)

   def to_xml(self, stream=None):
      import gamera_xml
      return gamera_xml.WriteXML(glyphs=self).write_stream(stream)

   def to_xml_filename(self, filename):
      import gamera_xml
      return gamera_xml.WriteXML(glyphs=self).write_filename(filename)

   def set_property(self, name, value):
      self.property[name] = value

   def get_property(self, name):
      return self.property[name]

######################################################################
      
class Image(gameracore.Image, ImageBase):
   def __init__(self, page_offset_y, page_offset_x, nrows, ncols,
                pixel_format, storage_type):
      ImageBase.__init__(self)
      gameracore.Image.__init__(self, page_offset_y, page_offset_x,
                                nrows, ncols, pixel_format, storage_type)
   __getstate__ = ImageBase.__getstate__


######################################################################

class SubImage(gameracore.SubImage, ImageBase):
   def __init__(self, image, offset_y, offset_x, nrows, ncols):
      ImageBase.__init__(self)
      gameracore.SubImage.__init__(self, image, offset_y, offset_x,
                                   nrows, ncols)
   __getstate__ = ImageBase.__getstate__


######################################################################

class Cc(gameracore.Cc, ImageBase):
   def __init__(self, image, label, offset_y, offset_x, nrows, ncols):
      ImageBase.__init__(self)
      gameracore.Cc.__init__(self, image, label, offset_y, offset_x,
                             nrows, ncols)
   __getstate__ = ImageBase.__getstate__
   
   # Displays this cc in context
   def display_context(self):
      gui = config.options.__.gui
      if not gui:
         return
      image = self.parent()
      if self._display:
         self._display.set_image(image)
      else:
         self.set_display(
            gui.ShowImage(image,
                          "Connected Component",
                          owner=self))
      self._display.highlight_cc(self)
      self._display.focus(self)
      self.last_display = "context"

# this is a convenience function for using in a console
_gamera_initialised = 0
def init_gamera():
   global _gamera_initialised
   if not _gamera_initialised:
      import plugin, gamera_xml
      config.parse_options()
      # Create the default functions for the menu
      for method in (
         plugin.PluginFactory(
            "load_image", None, "File", plugin.ImageType([], "image"),
            plugin.ImageType([ALL]), (plugin.FileOpen("filename"),)),
         plugin.PluginFactory(
            "display", None, "Display", None, plugin.ImageType([ALL]), None),
         plugin.PluginFactory(
            "display_children", None, "Display", None, plugin.ImageType([ALL]),
            None),
         plugin.PluginFactory(
            "display_ccs", None, "Display", None, plugin.ImageType([ONEBIT]),
            None),
         plugin.PluginFactory(
            "display_cc", None, "Display", None, plugin.ImageType([ONEBIT]),
            plugin.ImageType([ONEBIT], "cc")),
         plugin.PluginFactory(
            "display_false_color", None, "Display", None, plugin.ImageType([GREYSCALE, FLOAT]),
            None),
         plugin.PluginFactory(
            "classify_manual", None, "Classification", None,
            plugin.ImageType([ONEBIT]), plugin.String("id")),
         plugin.PluginFactory(
            "classify_heuristic", None, "Classification", None,
            plugin.ImageType([ONEBIT]), plugin.String("id")),
         plugin.PluginFactory(
            "classify_automatic", None, "Classification", None,
            plugin.ImageType([ONEBIT]), plugin.String("id")),
         plugin.PluginFactory(
            "unclassify", None, "Classification", None,
            plugin.ImageType([ONEBIT]), None),
         plugin.PluginFactory(
            "to_xml", None, "XML", plugin.String('xml'),
            plugin.ImageType([ONEBIT]), None),
         plugin.PluginFactory(
            "to_xml_filename", None, "XML", None, plugin.ImageType([ONEBIT]),
            (plugin.FileSave("filename", extension=gamera_xml.extensions),))
         ):
         method.register()
      paths.import_directory(paths.plugins, globals(), locals(), 1)
      import sys
      sys.path.append(".")
      _gamera_initialised = 1

if __name__ == "__main__":
   init_gamera()
