# -*- mode: python; indent-tabs-mode: nil; tab-width: 3 -*-
# vim: set tabstop=3 shiftwidth=3 expandtab:
#
# Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom,
#                         and Karl MacMillan
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#

import os.path
import wx
from gamera.core import *                    # Gamera specific
from gamera import paths, util, classify, gamera_xml
from gamera.gui import image_menu, var_name, gamera_icons, gui_util, has_gui
from gamera.gui.matplotlib_support import *
import array, inspect

######################################################################

class IconDisplayDropTarget(wx.FileDropTarget, wx.PyDropTarget):
   def __init__(self, parent):
      wx.FileDropTarget.__init__(self)
      self.parent = parent

   def OnDropFiles(self, x, y, filenames):
      for filename in filenames:
         if os.path.exists(filename):
            filename = os.path.normpath(os.path.abspath(filename))
            if filename.endswith('.xml') or filename.endswith('.xml.gz'):
               if open(filename, 'r').read(4) == "<xml":
                  name = var_name.get("glyphs", self.display.shell.locals)
                  if name:
                     self.display.shell.run('from gamera import gamera_xml')
                     self.display.shell.run(
                        '%s = gamera_xml.glyphs_from_xml(r"%s")' % (name, filename))
            else:
               name = var_name.get('image', self.display.shell.locals)
               if name:
                  self.display.shell.run(
                    '%s = load_image(r"%s")' % (name, filename))

######################################################################

class IconDisplay(wx.ListCtrl):
   def __init__(self, parent, main_win):
      if wx.Platform == '__WXMAC__':
         style = wx.LC_ICON|wx.LC_SINGLE_SEL
      else:
         style = wx.LC_LIST|wx.LC_SINGLE_SEL
      if not (wx.VERSION >= (2, 5) and wx.Platform == '__WXGTK__'):
         style |= wx.LC_ALIGN_TOP
      wx.ListCtrl.__init__(self, parent , -1, (0,0), (-1,-1), style)
      self.data = {}
      self.locals = {}
      self.modules = {}
      self.main_win = main_win
      self.currentIcon = None
      self.currentIconName = None
      self.init_events()
      self.SetToolTip(wx.ToolTip(
        'Double-click to display.\n' +
        'Right-click to perform functions.\n'))
      self.help_mode = 0
      self.dt = IconDisplayDropTarget(self)
      self.dt.display = self
      self.SetDropTarget(self.dt)
      self.il = wx.ImageList(32, 32)
      self.classes = []
      for klass in builtin_icon_types:
         self.add_class(klass)
      if wx.Platform == '__WXMAC__':
         self.AssignImageList(self.il, wx.IMAGE_LIST_NORMAL)
      else:
         self.AssignImageList(self.il, wx.IMAGE_LIST_SMALL)

   def add_class(self, icon_description):
      add_it = 1
      for klass in self.classes:
         if klass[0] == icon_description:
            add_it = 0
            break
      if add_it:
         icon = self.il.AddIcon(icon_description.get_icon())
         self.classes.append((icon_description, icon))

   def init_events(self):
      tID = self.GetId()
      wx.EVT_LIST_BEGIN_DRAG(self, tID, self.OnMouseDown)
      wx.EVT_LEFT_DCLICK(self, self.OnDoubleClick)
      wx.EVT_LIST_ITEM_SELECTED(self, tID, self.OnItemSelected)
      wx.EVT_LIST_ITEM_ACTIVATED(self, tID, self.OnItemSelected)
      wx.EVT_LIST_ITEM_RIGHT_CLICK(self, tID, self.OnRightClick)
      wx.EVT_CHAR(self, self.OnKeyPress)

   def add_icon(self, label, data, icon):
      index = self.GetItemCount()
      data.index = index
      self.data[label] = data
      self.InsertImageStringItem(index, label, icon)

   def refresh_icon(self, key, klass, data, icon_num):
      icon = self.data[key]
      index = icon.index
      del icon.data
      if 'extra_methods' in icon.__dict__:
         del icon.extra_methods
      obj = klass(key, data, index)
      self.data[key] = obj
      self.SetStringItem(index, 0, key, icon_num)

   def remove_icon(self, key):
      if self.data.has_key(key):
         icon = self.data[key]
         del icon.data
         if 'extra_methods' in icon.__dict__:
            del icon.extra_methods
         index = icon.index
         self.DeleteItem(index)
         del self.data[key]
         for i in self.data.values():
            if i.index > index:
               i.index = i.index - 1
         if index < self.GetItemCount():
            next = index
         else:
            next = self.GetItemCount() - 1
         if next >= 0:
            self.SetItemState(
               next, wx.LIST_STATE_SELECTED, wx.LIST_STATE_SELECTED)

   def update_icons(self, locals=None):
      if locals != None:
         self.locals = locals
      okay = []
      for key, val in self.locals.items():
         t = None
         for klass, icon in self.classes:
            try:
               if klass.check(val):
                  t = klass
                  break
            except:
               pass
         if t != None:
            if self.data.has_key(key):
               self.refresh_icon(key, t, val, icon)
            else:
               obj = t(key, val, 0)
               self.add_icon(key, obj, icon)
            okay.append(key)
         elif self.data.has_key(key):
            self.remove_icon(key)
      for i in self.data.keys():
         if i not in okay:
            self.remove_icon(i)

   def find_icon(self, index):
      for i in self.data.values():
         if i.index == index:
            return i
      return None

   def OnItemSelected(self, event):
      self.currentIcon = self.find_icon(event.m_itemIndex)

   def OnRightClick(self, event):
      index = event.GetIndex()
      if index < 0 or index >= self.GetItemCount():
         event.Skip()
         return
      for i in range(self.GetItemCount()):
         self.SetItemState(i, 0, wx.LIST_STATE_SELECTED)
      self.SetItemState(index, wx.LIST_STATE_SELECTED, wx.LIST_STATE_SELECTED)
      currentIcon = self.find_icon(index)
      if currentIcon:
         try:
            currentIcon.right_click(self, event, self.shell)
         except Exception, e:
            gui_util.message(str(e))
      event.Skip()

   def OnDoubleClick(self, event):
      if self.currentIcon:
         try:
            source = self.currentIcon.double_click()
         except Exception, e:
            gui_util.message(str(e))
         else:
            if not source is None:
               source = source.split("\n")
               for s in source:
                  self.shell.run(s)

   def OnKeyPress(self,event):
      keyID = event.GetKeyCode()
      if self.currentIcon:
         if keyID in (127, 8):
            try:
               source = self.currentIcon.delete_key()
            except Exception, e:
               gui_util.message(str(e))
            else:
               self.currentIcon = None
         elif(keyID==19):
            try:
               source = self.currentIcon.control_s()
            except Exception, e:
               gui_util.message(str(e))
         else:
            return
         if not source is None:
            self.shell.run(source)

   def OnMouseDown(self, event):
      if self.currentIcon:
         source = self.currentIcon.drag()
         if source is not None:
            typename, source = source
            data = wx.CustomDataObject(wx.CustomDataFormat(typename))
            data.SetData(source)
            icon = self.currentIcon.get_icon()
            drop_source = wx.DropSource(self, icon, icon, icon)
            drop_source.SetData(data)
            result = drop_source.DoDragDrop(True)

######################################################################

# Standard icons for core Gamera

class CustomIcon:
   is_custom_icon_description = True
   extra_methods = {}

   def __init__(self, label_, data_, index_):
      self.label = label_
      self.data = data_
      self.index = index_

   def register(cls):
      if has_gui.gui.TopLevel() != None:
         icon_display = main_win = has_gui.gui.TopLevel().icon_display
         icon_display.add_class(cls)
   register = classmethod(register)

   def to_icon(bitmap):
      if has_gui.has_gui:
         return wx.IconFromBitmap(bitmap)
      else:
         return None
   to_icon = staticmethod(to_icon)

   def get_icon():
      return wx.IconFromBitmap(gamera_icons.getIconImageUnknownBitmap())
   get_icon = staticmethod(get_icon)

   def check(data):
      return 1
   check = staticmethod(check)

   def double_click(self):
      return '%s.display("%s")' % (self.label, self.label)

   def right_click(self, parent, event, shell):
      if not has_gui.has_gui:
         return
      x,y = event.GetPoint()
      self._shell = shell
      image_menu.ImageMenu(
        parent, x, y,
        self.data, self.label,
        shell, extra_methods = self.extra_methods)
      self._shell = None

   def delete_key(self):
      return "del %s" % self.label

   def control_s(self):
      from gamera.plugins import image_utilities
      call = image_utilities.image_save.args.show()
      if call is None: return None

      return self.label + ".image_save(r\'" + call[0] + "\'," + str(call[1]) + ")"

   def drag(self):
      return None

class CIComplexImage(CustomIcon):
   def get_icon():
      return wx.IconFromBitmap(gamera_icons.getIconImageComplexBitmap())
   get_icon = staticmethod(get_icon)

   def check(data):
      return isinstance(data,Image) and data.data.pixel_type == COMPLEX
   check = staticmethod(check)

class CIRGBImage(CustomIcon):
   def get_icon():
      return wx.IconFromBitmap(gamera_icons.getIconImageRgbBitmap())
   get_icon = staticmethod(get_icon)

   def check(data):
      return isinstance(data, Image) and data.data.pixel_type == RGB
   check = staticmethod(check)

class CIGreyScaleImage(CustomIcon):
   def get_icon():
      return wx.IconFromBitmap(gamera_icons.getIconImageGreyBitmap())
   get_icon = staticmethod(get_icon)

   def check(data):
      return isinstance(data, Image) and data.data.pixel_type == GREYSCALE
   check = staticmethod(check)

class CIGrey16Image(CustomIcon):
   def get_icon():
      return wx.IconFromBitmap(gamera_icons.getIconImageGrey16Bitmap())
   get_icon = staticmethod(get_icon)

   def check(data):
      return isinstance(data, Image) and data.data.pixel_type == GREY16
   check = staticmethod(check)

class CIFloatImage(CustomIcon):
   def get_icon():
      return wx.IconFromBitmap(gamera_icons.getIconImageFloatBitmap())
   get_icon = staticmethod(get_icon)

   def check(data):
      return isinstance(data, Image) and data.data.pixel_type == FLOAT
   check = staticmethod(check)

class CIOneBitImage(CustomIcon):
   def get_icon():
      return wx.IconFromBitmap(gamera_icons.getIconImageBinaryBitmap())
   get_icon = staticmethod(get_icon)

   def check(data):
      return isinstance(data, Image) and data.data.pixel_type == ONEBIT
   check = staticmethod(check)

class CIRGBSubImage(CustomIcon):
   def get_icon():
      return wx.IconFromBitmap(gamera_icons.getIconSubimageRgbBitmap())
   get_icon = staticmethod(get_icon)

   def check(data):
      return isinstance(data, SubImage) and data.data.pixel_type == RGB
   check = staticmethod(check)

class CIGreyScaleSubImage(CustomIcon):
   def get_icon():
      return wx.IconFromBitmap(gamera_icons.getIconSubimageGreyBitmap())
   get_icon = staticmethod(get_icon)

   def check(data):
      return isinstance(data, SubImage) and data.data.pixel_type == GREYSCALE
   check = staticmethod(check)

class CIGrey16SubImage(CustomIcon):
   def get_icon():
      return wx.IconFromBitmap(gamera_icons.getIconSubimageGrey16Bitmap())
   get_icon = staticmethod(get_icon)

   def check(data):
      return isinstance(data, SubImage) and data.data.pixel_type == GREY16
   check = staticmethod(check)

class CIFloatSubImage(CustomIcon):
   def get_icon():
      return wx.IconFromBitmap(gamera_icons.getIconSubimageFloatBitmap())
   get_icon = staticmethod(get_icon)

   def check(data):
      return isinstance(data, SubImage) and data.data.pixel_type == FLOAT
   check = staticmethod(check)

class CIOneBitSubImage(CustomIcon):
   def get_icon():
      return wx.IconFromBitmap(gamera_icons.getIconSubimageBinaryBitmap())
   get_icon = staticmethod(get_icon)

   def check(data):
      return isinstance(data, SubImage) and data.data.pixel_type == ONEBIT
   check = staticmethod(check)

class CIComplexSubImage(CustomIcon):
   def get_icon():
      return wx.IconFromBitmap(gamera_icons.getIconSubimageComplexBitmap())
   get_icon = staticmethod(get_icon)

   def check(data):
      return isinstance(data, SubImage) and data.data.pixel_type == COMPLEX
   check = staticmethod(check)

class CICC(CustomIcon):
   def get_icon():
      return wx.IconFromBitmap(gamera_icons.getIconCcBitmap())
   get_icon = staticmethod(get_icon)

   def check(data):
      return isinstance(data, Cc)
   check = staticmethod(check)

class CIImageList(CustomIcon):
   def __init__(self, *args):
      CustomIcon.__init__(self, *args)
      self.extra_methods = {
         'List': {'XML': {'glyphs_to_xml': self.glyphs_to_xml},
                  'Features': {'generate_features_list' : self.generate_features}}}

   def get_icon():
      return wx.IconFromBitmap(gamera_icons.getIconImageListBitmap())
   get_icon = staticmethod(get_icon)

   def check(data):
      return util.is_homogeneous_image_list(data)
   check = staticmethod(check)

   def glyphs_to_xml(self, event):
      from gamera import gamera_xml
      filename = gui_util.save_file_dialog(None, gamera_xml.extensions)
      if filename != None:
         gamera_xml.glyphs_to_xml(filename, self.data)

   def generate_features(self, event):
      import gamera.core
      ff = gamera.core.Image.get_feature_functions()
      progress = gamera.util.ProgressFactory(
        "Generating features...", len(self.data))
      try:
         for glyph in self.data:
            glyph.generate_features(ff)
            progress.step()
      finally:
         progress.kill()

   def double_click(self):
      return 'display_multi(%s)' % self.label

   def control_s(self): pass

class CIInteractiveClassifier(CustomIcon):
   def get_icon():
      return wx.IconFromBitmap(gamera_icons.getIconClassifyBitmap())
   get_icon = staticmethod(get_icon)

   def check(data):
      return isinstance(data, classify.InteractiveClassifier)
   check = staticmethod(check)

   def double_click(self):
      return '%s.display()' % (self.label)

   def right_click(self, *args):
      pass

   def control_s(self): pass

class CINonInteractiveClassifier(CustomIcon):
   def get_icon():
      return wx.IconFromBitmap(gamera_icons.getIconNoninterClassifyBitmap())
   get_icon = staticmethod(get_icon)

   def check(data):
      return isinstance(data, classify.NonInteractiveClassifier)
   check = staticmethod(check)

   def double_click(self):
      pass

   def right_click(self, *args):
      pass

   def control_s(self): pass

class _CIVector(CustomIcon):
   def check(cls, data):
      if data is ALL:
         return False
      if (isinstance(data, array.array) and
          data.typecode == cls.typecode):
         return True
      try:
         it = iter(data)
      except Exception, e:
         return False
      else:
         if not len(data):
            return false
         good = True
         try:
            for x in data:
               if not isinstance(x, cls.klass):
                  good = False
                  break
         except Exception, e:
            print e
            return False
         else:
            return good
   check = classmethod(check)

   def double_click(self):
      from gamera.gui import matplotlib_support
      if matplotlib_support.matplotlib_installed:
         name = var_name.get("figure")
         if name != None:
            return "%s = plot(%s)" % (name, self.label)
      else:
         gui_util.message("Plotting is not supported because the optional matplotlib library\n"
                       "could not be found.\n\n"
                       "Download and install matplotlib from matplotlib.sourceforge.net,\n"
                       "then restart Gamera to have plotting support.")

   def right_click(self, *args):
      pass

   def control_s(self):
      pass

   def drag(self):
      return ("Vector", str(list(self.data)))

class CIIntVector(_CIVector):
   typecode = 'i'
   klass = int

   def get_icon():
      return wx.IconFromBitmap(gamera_icons.getIntVectorBitmap())
   get_icon = staticmethod(get_icon)

class CIFloatVector(_CIVector):
   typecode = 'd'
   klass = float

   def get_icon():
      return wx.IconFromBitmap(gamera_icons.getFloatVectorBitmap())
   get_icon = staticmethod(get_icon)

class CIComplexVector(_CIVector):
   typecode = None
   klass = complex

   def get_icon():
      return wx.IconFromBitmap(gamera_icons.getComplexVectorBitmap())
   get_icon = staticmethod(get_icon)

builtin_icon_types = (
  CICC, CIRGBImage, CIComplexImage, CIGreyScaleImage, CIGrey16Image,
  CIFloatImage, CIOneBitImage, CIRGBSubImage,
  CIGreyScaleSubImage, CIGrey16SubImage, CIFloatSubImage,
  CIOneBitSubImage, CIComplexSubImage, CIImageList, CIInteractiveClassifier,
  CINonInteractiveClassifier, CIIntVector, CIFloatVector, CIComplexVector)
