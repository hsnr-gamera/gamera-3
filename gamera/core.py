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


"""This module contains the core parts of Gamera including all of the image
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
from gameracore import ONEBIT, GREYSCALE, GREY16, RGB, FLOAT, COMPLEX
from enums import ALL, NONIMAGE
# import the storage types
from gameracore import DENSE, RLE
# import some of the basic types
from gameracore import ImageData, Size, Dimensions, Point, \
     Rect, Region, RegionMap, ImageInfo, RGBPixel
# import gamera.gameracore for subclassing
import gameracore
# from classify import InteractiveClassifier, NonInteractiveClassifier
from gamera.gui import has_gui

# from gamera.classify import *
import paths, util    # Gamera-specific
from config import config

class SegmentationError(Exception):
   pass

######################################################################

def load_image(filename, compression = DENSE):
   """**load_image** (FileOpen *filename*, Choice *storage_format* = ``DENSE``)

Load an image from the given filename.  At present, TIFF and PNG files are
supported.

*storage_format*
  The type of `storage format`__ to use for the resulting image.

.. __: image_types.html#storage-formats"""
   from gamera.plugins import _tiff_support, _png_support
   try:
      image = _tiff_support.load_tiff(filename, compression)
   except RuntimeError:
      try:
         image = _png_support.load_PNG(filename, compression)
      except RuntimeError, AttributeError:
         raise RuntimeError("%s is not a TIFF or PNG file." % filename)
   image.name = filename
   return image

def nested_list_to_image(l, t=-1):
   from gamera.plugins import image_utilities
   return image_utilities.nested_list_to_image(l, t)

def image_info(filename):
   """ImageInfo **image_info** (FileOpen *filename*)

Retrieve information about an image file without loading it.

The result is an ImageInfo__ object about a given filename. The ImageInfo
object gives information such as the type (color, greyscale, onebit),
the bit-depth, resolution, size, etc.

.. __: gamera.core.ImageInfo.html"""
   import tiff_support
   return tiff_support.tiff_info(filename)

def display_multi(list):
   """**display_multi** (ImageList *list*)

Displays a list of images in a grid-like window.  This function has
no effect if the `Gamera GUI`__ is not running.

.. __: gui.html"""
   # If it's not a list, we'll get errors, so make it one
   if not util.is_sequence(list):
      list = [list]
   if not len(list):
      raise ValueError("Given list is empty.")
   return has_gui.gui.ShowImages(list)

# Used to cache the list of all features
all_features = None

class ImageBase:
   """Base class for all of the image objects. This class contains
   common functionality for all of the image types including
   the necessary infrastructure to support dynamically added plugins."""
   # Stores the categorized set of methods.  Bears no relationship
   # to __methods__
   methods = {}

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
      self.properties = self.__class__.Properties()
      # Keep a list of tuples of (feature_name, feature_function) that
      # have already been generated for this Image
      self.feature_functions = [[], 0]

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

Internal use only."""
      from gamera import args
      methods = cls.methods
      image_type =  isinstance(plug.self_type, args.ImageType)
      if not func is None and image_type:
         func = instancemethod(func, None, gameracore.Image)
         setattr(cls, plug.__name__, func)
      if not category is None:
        if image_type:
           pixel_types = plug.self_type.pixel_types
        else:
           pixel_types = [NONIMAGE]
        for type in pixel_types:
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
      """String **pixel_type_name** ()

Returns a string representing the pixel type of the image. Intended
primarily for display (GUI) purposes, since using the integer values
in ``Image.data.pixel_type`` is more efficient.

See `pixel types`_ for more information."""
      return util.get_pixel_type_name(self.data.pixel_type)
   pixel_type_name = property(pixel_type_name, doc=pixel_type_name.__doc__)

   _storage_format_names = {DENSE:  "Dense",
                            RLE:    "RLE"}
   
   def storage_format_name(self):
      """String **storage_format_name** ()

Returns a string representing the storage format of the image.
Intended primarily for display (GUI) purposes, since using the integer
values in ``Image.data.storage_format`` is more efficient.

See `storage formats`_ for more information."""
      return self._storage_format_names[self.data.storage_format]
   storage_format_name = property(storage_format_name, doc=storage_format_name.__doc__)
   
   _members_for_menu = ('pixel_type_name',
                        'storage_format_name',
                        'ul_x', 'ul_y', 'nrows', 'ncols',
                        'resolution', 'memory_size', 'label', 
                        'classification_state', 'properties')
   def members_for_menu(self):
      return ["%s: %s" % (x, getattr(self, x))
              for x in self._members_for_menu
              if hasattr(self, x)]

   def methods_for_menu(self):
      return self.methods[self.data.pixel_type]

   def methods_flat_category(cls, category, pixel_type=None):
      methods = cls.methods[pixel_type]
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
      """**load_image** (FileOpen *filename*, Choice *storage_format* = ``DENSE``)

Load an image from the given filename.  At present, TIFF and PNG files are
supported.

*storage_format*
  The type of `storage format`__ to use for the resulting image.

.. __: image_types.html#storage-formats"""
      return load_image(filename, compression)
   load_image = staticmethod(load_image)

   def memory_size(self):
      """Int **memory_size** ()

Returns the memory used by the underlying data of the image (in
bytes).  Note that this memory may be shared with other image views on
the same data."""
      return self.data.bytes
   memory_size = property(memory_size, doc=memory_size.__doc__)

   def set_display(self, _display):
      self._display = _display

   def display(self):
      """Displays the image in its own window.  (See `Using the Gamera GUI`__).  If the GUI
process is not running, this method has no effect.

.. __: gui.html

.. image:: images/display.png"""
      if self.data.pixel_type in (COMPLEX, FLOAT):
         conversion = "to_greyscale"
      else:
         conversion = None
      if self._display:
         self._display.set_image(self, conversion)
      else:
         self.set_display(
            has_gui.gui.ShowImage(self, self.name, conversion,
                          owner=self))
      self.last_display = "normal"
      return self._display

   def display_ccs(self):
      """Displays the image in its own window.  (See `Using the Gamera GUI`__).
Each connected component is assigned to one of eight colors.  This
display can be used to see how connected component analysis performs
on a given image.  Uses color_ccs_ under the hood.

.. __: gui.html
.. _color_ccs: gui_support.html#color-ccs

.. note: Connected component analysis must already be performed on the image
   (using cc_analysis_, for example) in order for this to work.

.. _cc_analysis: segmentation.html#cc-analysis

.. image:: images/display_ccs.png"""
      if self._display:
         self._display.set_image(self, "color_ccs")
      else:
         self.set_display(
            has_gui.gui.ShowImage(self, self.name, "color_ccs",
                          owner=self))
      self.last_display = "normal"
      return self._display

   def display_false_color(self):
      """Displays the image using false coloring.  (See false_color_).

.. _false_color: color.html#false-color

.. image:: images/display_false_color.png"""
      if self._display:
         self._display.set_image(self, "false_color")
      else:
         self.set_display(
            has_gui.gui.ShowImage(self, self.name, "false_color",
                          owner=self))
      self.last_display = "normal"
      return self._display

   def unclassify(self):
      """Sets the image back to an UNCLASSIFIED state.  Use this
when you are not longer sure of the identity of the image and you
want an automatic classifier to reclassify."""
      self.id_name = []
      self.classification_state = UNCLASSIFIED

   def classify_manual(self, id_name):
      """Classifies the image as the value *id_name* and sets the state
to MANUAL.  Use this method when an end user has classified this glyph.

*id_name*
  Can come in one of two forms:

    **string**
       image is classified using the given ``.``-delimited class name.
    **list of tuples**
       A list of tuples where each tuple is the pair (*confidence*, *class_name*).

       *confidence*
         A value in range (0, 1), where 0 is uncertain and 1 is certain.
       *class_name*
         A ``.``-delimited class name."""
      if util.is_string_or_unicode(id_name):
         id_name = [(1.0, id_name)]
      elif type(id_name) == ListType:
         id_name.sort()
      else:
         raise TypeError("id_name must be a string or a list")
      self.id_name = id_name
      self.classification_state = MANUAL

   def classify_automatic(self, id_name):
      """Classifies the image as the value *id_name* and sets the state
to AUTOMATIC.  Use this method when an automatic classifier has classified this glyph.

*id_name*
  Can come in one of two forms:

    **string**
       image is classified using the given ``.``-delimited class name.
    **list of tuples**
       A list of tuples where each tuple is the pair (*confidence*, *class_name*).

       *confidence*
         A value in range (0, 1), where 0 is uncertain and 1 is certain.
       *class_name*
         A ``.``-delimited class name."""
      if util.is_string_or_unicode(id_name):
         id_name = [(0.0, id_name)]
      elif type(id_name) == ListType:
         id_name.sort()
      else:
         raise TypeError("id_name must be a string or a list")
      self.id_name = id_name
      self.classification_state = AUTOMATIC

   def classify_heuristic(self, id_name):
      """Classifies the image as the value *id_name* and sets the state
to AUTOMATIC.  Use this method when a heuristic process has classified this glyph.

*id_name*
  Can come in one of two forms:

    **string**
       image is classified using the given ``.``-delimited class name.
    **list of tuples**
       A list of tuples where each tuple is the pair (*confidence*, *class_name*).

       *confidence*
         A value in range (0, 1), where 0 is uncertain and 1 is certain.
       *class_name*
         A ``.``-delimited class name."""
      if util.is_string_or_unicode(id_name):
         id_name = [(0.5, id_name)]
      elif type(id_name) == ListType:
         id_name.sort()
      else:
         raise TypeError("id_name must be a string or a list")
      self.id_name = id_name
      self.classification_state = HEURISTIC

   def get_main_id(self):
      """Returns the classification of the image with the highest likelihood.

If the image is unclassified, the result is 'UNCLASSIFIED'.
"""
      if self.classification_state == UNCLASSIFIED or not len(self.id_name):
         return 'UNCLASSIFIED'
      return self.id_name[0][1]

   def get_confidence(self):
      """Returns the confidence of the classification with the highest likelihood.

If the image is unclassified, returns -1.0.
"""
      if self.classification_state == UNCLASSIFIED:
         return -1.0
      return self.id_name[0][0]

   def has_id_name(self, name):
      """Returns ``True`` if the image has the given classification ``name``."""
      for confidence, id_name in self.id_name:
         if name == id_name:
            return True
      return False

   def subimage(self, *args, **kwargs):
      """Creates a new view that is part of an existing image.

There are a number of ways to create a subimage:

  - **subimage** (Int *offset_y*, Int *offset_x*, Int *nrows*, Int *ncols*)

  - **subimage** (Point *upper_left*, Point *lower_right*)

  - **subimage** (Point *upper_left*, Size *size*)

  - **subimage** (Point *upper_left*, Dimensions *dimensions*)

  - **subimage** (Rect *rectangle*)

Changes to subimages will affect all other subimages viewing the same data."""
      if hasattr(self, "label"):
         return Cc(self, self.label, *args, **kwargs)
      else:
         return SubImage(self, *args, **kwargs)

   def _get_feature_vector_size(cls, functions):
      num_features = 0
      for name, function in functions:
          num_features += function.return_type.length
      return num_features
   _get_feature_vector_size = classmethod(_get_feature_vector_size)

   def get_feature_functions(cls, features='all'):
      global all_features
      if all_features is None:
         all_features = cls.methods_flat_category('Features', ONEBIT)
         all_features.sort()
      if features == 'all' or features is None:
         functions = all_features
         return functions, cls._get_feature_vector_size(functions)
      features = util.make_sequence(features)
      all_strings = True
      for feature in features:
         if not util.is_string_or_unicode(feature):
            all_strings = False
            break
      if not all_strings:
         import plugin
         all_functions = False
         if (type(features) == tuple and
             len(features) == 2 and
             type(features[0]) == list and
             type(features[1]) == int):
            all_functions = True
            for feature in features[0]:
               if not (type(feature) == tuple and
                       util.is_string_or_unicode(feature[0]) and
                       issubclass(feature[1], plugin.PluginFunction)):
                  all_functions = False
                  break
         if not all_functions:
            raise ValueError(
               "'%s' is not a valid way to specify a list of features."
               % str(features))
         else:
            return features
      else:
         features.sort()
         functions = []
         for feature in features:
            found = 0
            for i in all_features:
               if feature == i[0]:
                  functions.append(i)
                  found = 1
                  break
            if not found:
               raise ValueError("'%s' is not a known feature function." % feature)
         functions.sort()
         return functions, cls._get_feature_vector_size(functions)
   get_feature_functions = classmethod(get_feature_functions)

   def to_xml(self, stream=None):
      """Returns a string containing the Gamera XML representation of the image.
(See the Gamera XML DTD in ``misc/gamera.dtd`` in the source distribution.)
"""
      import gamera_xml
      return gamera_xml.WriteXML(glyphs=[self]).write_stream(stream)

   def to_xml_filename(self, filename):
      """Saves the Gamera XML representation of the image to the given *filename*.
(See the Gamera XML DTD in ``misc/gamera.dtd`` in the source distribution.)
"""
      import gamera_xml
      return gamera_xml.WriteXML(glyphs=[self]).write_filename(filename)

   def set_property(self, name, value):
      self.property[name] = value

   def get_property(self, name):
      return self.property[name]

######################################################################

class Image(gameracore.Image, ImageBase):
   def __init__(self, *args, **kwargs):
      ImageBase.__init__(self)
      gameracore.Image.__init__(self, *args, **kwargs)
   __init__.__doc__ = gameracore.Image.__doc__

   def __del__(self):
      if self._display:
         self._display.close()
   __getstate__ = ImageBase.__getstate__

######################################################################

class SubImage(gameracore.SubImage, ImageBase):
   def __init__(self, *args, **kwargs):
      ImageBase.__init__(self)
      gameracore.SubImage.__init__(self, *args, **kwargs)
   __init__.__doc__ = gameracore.SubImage.__doc__

   def __del__(self):
      if self._display:
         self._display.close()

   __getstate__ = ImageBase.__getstate__

######################################################################

class Cc(gameracore.Cc, ImageBase):
   def __init__(self, *args, **kwargs):
      ImageBase.__init__(self)
      gameracore.Cc.__init__(self, *args, **kwargs)
   __init__.__doc__ = gameracore.Cc.__doc__

   __getstate__ = ImageBase.__getstate__

   def __del__(self):
      if self._display:
         self._display.close()
   
   def display_context(self):
      """**display_context** ()

Displays the Cc in context, by opening a window displaying the underlying
image with the given Cc highlighted.  This method has no effect if the GUI
is not running.
"""
      image = self.parent()
      if self._display:
         self._display.set_image(image)
      else:
         self.set_display(
            has_gui.gui.ShowImage(image,
                          "Connected Component",
                          owner=self))
      self._display.highlight_cc(self)
      self._display.focus(self)
      self.last_display = "context"

# this is a convenience function for using in a console
_gamera_initialised = False
def _init_gamera():
   global _gamera_initialised
   if _gamera_initialised:
      return
   _gamera_initialised = True
   import plugin, gamera_xml, sys
   # Create the default functions for the menupl
   for method in (
      plugin.PluginFactory(
         "load_image", "File", plugin.ImageType(ALL, "image"),
         plugin.ImageType(ALL), plugin.Args([plugin.FileOpen("filename")])),
      plugin.PluginFactory(
         "display", "Displaying", None, plugin.ImageType(ALL), None),
      plugin.PluginFactory(
         "display_ccs", "Displaying", None, plugin.ImageType([ONEBIT]),
         None),
      plugin.PluginFactory(
         "display_false_color", "Displaying", None,
         plugin.ImageType([GREYSCALE, FLOAT]),
         None),
      plugin.PluginFactory(
         "classify_manual", "Classification", None,
         plugin.ImageType([ONEBIT]), plugin.Args([plugin.String("id")])),
      plugin.PluginFactory(
         "classify_heuristic", "Classification", None,
         plugin.ImageType([ONEBIT]), plugin.Args([plugin.String("id")])),
      plugin.PluginFactory(
         "classify_automatic", "Classification", None,
         plugin.ImageType([ONEBIT]), plugin.Args([plugin.String("id")])),
      plugin.PluginFactory(
         "unclassify", "Classification", None,
         plugin.ImageType([ONEBIT]), None),
      plugin.PluginFactory(
         "get_main_id", "Classification", plugin.String("id"),
         plugin.ImageType([ONEBIT]), None),
      plugin.PluginFactory(
         "get_confidence", "Classification", plugin.Float("confidence"),
         plugin.ImageType([ONEBIT]), None),
      plugin.PluginFactory(
         "has_id_name", "Classification", plugin.Check("result"),
         plugin.ImageType([ONEBIT]), plugin.Args([plugin.String("id")])),
      plugin.PluginFactory(
         "subimage", "Utility", plugin.Check("result"),
         plugin.ImageType(ALL), plugin.Args([plugin.Int("offset_y"), plugin.Int("offset_x"),
                                             plugin.Int("nrows"), plugin.Int("ncols")])),
      plugin.PluginFactory(
         "to_xml", "XML", plugin.String('xml'),
         plugin.ImageType([ONEBIT]), None),
      plugin.PluginFactory(
         "to_xml_filename", "XML", None, plugin.ImageType([ONEBIT]),
         plugin.Args([
      plugin.FileSave("filename", extension=gamera_xml.extensions)]))
      ):
      method.register()
   paths.import_directory(paths.plugins, globals(), locals(), 1)
   sys.path.append(".")

import sys
if sys.platform == 'win32':
   # Windows doesn't generally keep the console window open, making it difficult to
   # diagnose fatal errors.  This catch-all should help.
   def init_gamera():
      try:
         _init_gamera()
      except Exception, e:
         print type(e)
         if not isinstance(e, SystemExit):
            import traceback
            print "Gamera made a fatal error:"
            print
            traceback.print_exc()
            print
            print "Press <ENTER> to exit."
            x = raw_input()
            sys.exit(1)
else:
   init_gamera = _init_gamera

if __name__ == "__main__":
   init_gamera()

__all__ = ("init_gamera UNCLASSIFIED AUTOMATIC HEURISTIC MANUAL "
           "ONEBIT GREYSCALE GREY16 RGB FLOAT COMPLEX ALL DENSE RLE "
           "ImageData Size Dimensions Point Rect Region RegionMap "
           "ImageInfo Image SubImage Cc load_image image_info "
           "display_multi ImageBase nested_list_to_image RGBPixel").split()
