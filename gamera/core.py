#
#
# Copyright (C) 2001 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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

import sys, os, types, os.path, inspect, pydoc     # Python standard library

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

#import classify
import paths, util, config, new  # Gamera-specific

######################################################################

# Reference to the currently active gui
config.add_option_default("__gui", None)

def help(object):
   gui = config.get_option("__gui")
   if gui is None:
      pydoc.help(object)
   else:
      gui.help(object)

######################################################################

def load_image(filename, compression = DENSE):
   import tiff_support
   return tiff_support.load_tiff(filename, compression)

class ImageBase:
   # Stores the categorized set of methods.  Bears no relationship
   # to __methods__
   _methods = {}
   
   def __init__(self):
      self.name = "Untitled"
      self._display = None
      self.last_display = None

   def __del__(self):
      if self._display:
         self._display.close()

   _pixel_type_names = {ONEBIT:     "OneBit",
                        GREYSCALE:  "GreyScale",
                        GREY16:     "Grey16",
                        RGB:        "RGB",
                        FLOAT:      "Float"}
   
   def pixel_type_name(self):
      return self._pixel_type_names[self.data.pixel_type]
   pixel_type_name = property(pixel_type_name)

   _storage_format_names = {DENSE:  "Dense",
                            RLE:    "RLE"}
   
   def storage_format_name(self):
      return self._storage_format_names[self.data.storage_format]
   storage_format_name = property(storage_format_name)
   
   _members_for_menu = ('pixel_type_name',
                        'storage_format_name',
                        'ul_x', 'ul_y', 'nrows', 'ncols',
                        'size')

   def add_plugin_method(cls, plug, func, category=None):
      methods = cls._methods
      if not func is None:
         func = new.instancemethod(func, None, ImageBase)
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

   def members_for_menu(self):
      return ["%s: %s" % (x, getattr(self, x)) for x in self._members_for_menu]

   def methods_for_menu(self):
      methods = {}
      for type in (ALL, self.data.pixel_type):
         if self._methods.has_key(type):
            self._methods_sub(methods, self._methods[type])
      return methods

   def _methods_sub(self, dest, source):
     for key, val in source.items():
       if type(val) == type({}):
         if not dest.has_key(key):
           dest[key] = {}
         self._methods_sub(dest[key], val)
       else:
         dest[key] = val

   def methods_flat_category(self, category):
     methods = self.methods()
     start = methods
     for subcategory in category.split('/'):
       if start.has_key(subcategory):
         start = start[subcategory]
       else:
         raise KeyError
     return self._methods_flatten(start)

   def _methods_flatten(self, mat):
     list = []
     for val in mat.values():
       if type(val) == type({}):
         list.extend(self._methods_flatten(val))
       else:
         list.append(val)
     return list

   def size_mbytes(self):
      return self.data.mbytes
   size_mbytes = property(size_mbytes)

   def size(self):
      return util.pretty_print_bytes(self.data.bytes)
   size = property(size)

   def load_image(filename=None):
      return load_image(filename)
   load_image = staticmethod(load_image)

   def set_display(self, _display):
      self._display = _display

   def set_resolution(self, v):
      self.m.resolution(v)

   def display(self):
      "Displays the image in its own window."
      gui = config.get_option("__gui")
      if gui:
         if self._display:
            self._display.set_image(self, ImageBase.to_string)
         else:
            self.set_display(
               gui.ShowImage(self, self.name,
                             ImageBase.to_string, owner=self))
      self.last_display = "normal"
      return self._display

   # Displays the image in its own window, coloring the connected-
   # component labels
   def display_ccs(self):
      """Displays the image in its own window, coloring the connected
      components."""
      gui = config.get_option("__gui")
      if gui:
         if self._display:
            self._display.set_image(self, Image.cc_mat_to_string)
         else:
            self.set_display(
               gui.ShowImage(self, self.name,
                             Image.cc_mat_to_string, owner=self))
      self.last_display = "ccs"

   # Displays the image in its own window, highlighting the given
   # subimage (or subimages)
   def display_cc(self, cc):
      """display_cc(self, cc)
      Displays the image in its own window, highlighting the given
      subimage (or list of subimages)."""
      gui = config.get_option("__gui")
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
            self._display.highlight_cc(c, Image.to_string)
            self.cc.append(c)
      # This will adjust the scroll bars so the cc will be visible
      self._display.focus(self.cc)
      self.last_display = "cc"

   def display_children(self):
      if hasattr(self, 'children_images') and self.children_images != []:
         display_multi(self.children_images)

   def classification_color(self):
      if self.classification_state == UNCLASSIFIED:
         return None
      elif self.classification_state == AUTOMATIC:
         return (198,145,145)
      elif self.classification_state == HEURISTIC:
         return (240,230,140)
      elif self.classification_state == MANUAL:
         return (180,238,180)

   def set_id_name_manual(self, id_name):
      if id_name[-1] == ".":
         id_name = id_name[:-1]
      if not util.is_sequence(id_name):
         self.id_name = [id_name]
      else:
         self.id_name = list(id_name)
      self.classification_state = CLASS_MANUAL

   def set_id_name_automatic(self, id_name):
      if id_name[-1] == ".":
         id_name = id_name[:-1]
      if not util.is_sequence(id_name):
         self.id_name = [id_name]
      else:
         self.id_name = list(id_name)
      self.classification_state = CLASS_AUTOMATIC

   def set_id_name_heuristic(self, id_name):
      if id_name[-1] == ".":
         id_name = id_name[:-1]
      if not util.is_sequence(id_name):
         self.id_name = [id_name]
      else:
         self.id_name = list(id_name)
      self.classification_state = CLASS_HEURISTIC

   def subimage(self, offset_y, offset_x, nrows, ncols):
      return SubImage(self, offset_y, offset_x, nrows, ncols)

######################################################################
      
class Image(gameracore.Image, ImageBase):
   def __init__(self, page_offset_y, page_offset_x, nrows, ncols,
                pixel_format, storage_type):
      ImageBase.__init__(self)
      gameracore.Image.__init__(self, page_offset_y, page_offset_x,
                                nrows, ncols, pixel_format, storage_type)


######################################################################

class SubImage(gameracore.SubImage, ImageBase):
   def __init__(self, image, offset_y, offset_x, nrows, ncols):
      ImageBase.__init__(self)
      gameracore.SubImage.__init__(self, image, offset_y, offset_x,
                                   nrows, ncols)


######################################################################

class Cc(gameracore.Cc, ImageBase):
   def __init__(self, image, label, offset_y, offset_x, nrows, ncols):
      ImageBase.__init__(self)
      gameracore.Cc.__init__(self, image, label, offset_y, offset_x,
                             nrows, ncols)
   
   # Displays this cc in context
   def display_context(self):
      gui = config.get_option("__gui")
      if not gui:
         return
      image = self.parent()
      if self._display:

         self._display.set_image(image,
                                  Image.to_string)
      else:
         self.set_display(
            gui.ShowImage(image,
                          "Connected Component",
                          Image.to_string,
                          owner=self))
      self._display.highlight_cc(self,
                                 Image.to_string)
      self._display.focus(self)
      self.last_display = "context"


#####################################################################
#
# Copy Utilities
#
# The Python builtin copy utilities break when trying to
# copy Gamera images. These utility functions allow you
# to easily copy the portions of a Gamera image that you
# want with the correct semantics (i.e. things like._display
# are never copied, but they need to be properly initialized.
#
######################################################################

def copy_image_classification(source, dest):
   assert isinstance(source, Image)
   assert isinstance(dest, Image)
   from copy import copy
   if source.features:
      dest.features = type(source.features)()
      for i in source.features:
         dest.features.append(i)
      if hasattr(source.features, 'id'):
         dest.features.id(source.features.id())
   if source.id_name:
      dest.id_name = copy(source.id_name)
   dest.unique_id = source.unique_id
   dest.children_images = []
   dest.action_depth = 0

def copy_image_regions(source, dest):
   assert isinstance(source, Image)
   assert isinstance(dest, Image)
   dest.region_maps = source.region_maps
   dest.region_map = source.region_map
   dest.scaling = source.scaling

def copy_image_misc(source, dest):
   assert isinstance(source, Image)
   assert isinstance(dest, Image)
   dest.name = source.name

def copy_image_display(source, dest):
   assert isinstance(source, Image)
   assert isinstance(dest, Image)
   dest._display = None
   dest.last_display = None

def copy_image_all(source, dest):
   copy_image_classification(source, dest)
   copy_image_regions(source, dest)
   copy_image_misc(source, dest)
   copy_image_display(source, dest)

# this is a convenience function for using in a console
_gamera_initialised = 0
def init_gamera():
   global _gamera_initialised
   if not _gamera_initialised:
      import plugin
      # Create the default functions for the menu
      for method in (
         plugin.PluginFactory("load_image", None, "File",
                              plugin.ImageType([], "image"),
                              plugin.ImageType([ALL]),
                              (plugin.FileOpen("filename"))),
         plugin.PluginFactory("display", None, "Display",
                              None,
                              plugin.ImageType([ALL]),
                              None),
         plugin.PluginFactory("display_children", None, "Display",
                              None,
                              plugin.ImageType([ALL]),
                              None),
         plugin.PluginFactory("display_ccs", None, "Display",
                              None,
                              plugin.ImageType([ONEBIT]),
                              None),
         plugin.PluginFactory("display_cc", None, "Display",
                              None,
                              plugin.ImageType([ONEBIT]),
                              plugin.ImageType([ONEBIT], "cc"))
         ):
         method.register()
      paths.import_directory(paths.plugins, globals(), locals(), 1)
      _gamera_initialised = 1
