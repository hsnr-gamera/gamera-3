# vi:set tabsize=3:
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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#

import os.path
from wxPython.wx import *                    # wxPython
from gamera.core import *                    # Gamera specific
from gamera import paths, util, classify, gamera_xml
from gamera.gui import image_menu, var_name, gamera_icons, gui_util, has_gui, gamera_display
import array, inspect

######################################################################

class IconDisplayDropTarget(wxFileDropTarget, wxPyDropTarget):
   def __init__(self, parent):
      wxFileDropTarget.__init__(self)
      self.parent = parent

   def OnDropFiles(self, x, y, filenames):
      for filename in filenames:
         filename = os.path.normpath(os.path.abspath(filename))
         if filename.endswith('.xml') or filename.endswith('.xml.gz'):
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

class IconDisplay(wxListCtrl):
   def __init__(self, parent, main_win):
      wxListCtrl.__init__(self, parent , -1, (0,0), (-1,-1),
                          wxLC_LIST|wxLC_SINGLE_SEL|wxLC_ALIGN_TOP)
      self.data = {}
      self.locals = {}
      self.modules = {}
      self.main_win = main_win
      self.currentIcon = None
      self.currentIconName = None
      self.init_events()
      self.SetToolTip(wxToolTip(
        'Double-click to display.\n' + 
        'Right-click to perform functions.\n'))
      self.help_mode = 0
      self.dt = IconDisplayDropTarget(self)
      self.dt.display = self
      self.SetDropTarget(self.dt)
      self.il = wxImageList(32, 32)
      self.classes = []
      for klass in builtin_icon_types:
         self.add_class(klass)
      self.AssignImageList(self.il, wxIMAGE_LIST_SMALL)

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
      EVT_LIST_BEGIN_DRAG(self, tID, self.OnMouseDown)
      EVT_LEFT_DCLICK(self, self.OnDoubleClick)
      EVT_LIST_ITEM_SELECTED(self, tID, self.OnItemSelected)
      EVT_LIST_ITEM_ACTIVATED(self, tID, self.OnItemSelected)
      EVT_LIST_ITEM_RIGHT_CLICK(self, tID, self.OnRightClick)
      EVT_CHAR(self, self.OnKeyPress)
   
   def add_icon(self, label, data, icon):
      index = self.GetItemCount()
      data.index = index
      self.data[label] = data
      self.InsertImageStringItem(index, label, icon)

   def refresh_icon(self, key, klass, data, icon):
      index = self.data[key].index
      obj = klass(key, data, index)
      self.data[key] = obj
      self.SetStringItem(index, 0, key, icon)

   def remove_icon(self, key):
      if self.data.has_key(key):
         icon = self.data[key]
         del icon.data
         if hasattr(icon, 'extra_method'):
            del icon.extra_method
         index = icon.index
         self.DeleteItem(index)
         del self.data[key]
         for i in self.data.values():
            if i.index > index:
               i.index = i.index - 1

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
         self.SetItemState(i, 0, wxLIST_STATE_SELECTED)
      self.SetItemState(index, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED)
      currentIcon = self.find_icon(index)
      if currentIcon:
         currentIcon.right_click(self, event, self.shell)
      event.Skip()

   def OnDoubleClick(self, event):
      if self.currentIcon:
         source = self.currentIcon.double_click()
         if not source is None:
            self.shell.run(source)
   
   def OnKeyPress(self,event):
      keyID = event.GetKeyCode()
      if self.currentIcon:
         if(keyID == 127 or keyID == 8):
            source = self.currentIcon.delete_key()
            self.currentIcon = None
         elif(keyID==19):
            source = self.currentIcon.control_s()
         else: return
         if not source is None:
            self.shell.run(source)

   def OnMouseDown(self, event):
      if self.currentIcon:
         source = self.currentIcon.drag()
         if source is not None:
            typename, source = source
            data = wxCustomDataObject(wxCustomDataFormat(typename))
            data.SetData(source)
            icon = self.currentIcon.get_icon()
            drop_source = wxDropSource(self, icon, icon, icon)
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
         return wxIconFromBitmap(bitmap)
      else:
         return None
   to_icon = staticmethod(to_icon)

   def get_icon():
      return wxIconFromBitmap(gamera_icons.getIconImageUnknownBitmap())
   get_icon = staticmethod(get_icon)

   def check(data):
      return 1
   check = staticmethod(check)

   def double_click(self):
      return "%s.display()" % self.label

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
      return wxIconFromBitmap(gamera_icons.getIconImageComplexBitmap())
   get_icon = staticmethod(get_icon)

   def check(data):
      return isinstance(data,Image) and data.data.pixel_type == COMPLEX
   check = staticmethod(check)

class CIRGBImage(CustomIcon):
   def get_icon():
      return wxIconFromBitmap(gamera_icons.getIconImageRgbBitmap())
   get_icon = staticmethod(get_icon)

   def check(data):
      return isinstance(data, Image) and data.data.pixel_type == RGB
   check = staticmethod(check)

class CIGreyScaleImage(CustomIcon):
   def get_icon():
      return wxIconFromBitmap(gamera_icons.getIconImageGreyBitmap())
   get_icon = staticmethod(get_icon)

   def check(data):
      return isinstance(data, Image) and data.data.pixel_type == GREYSCALE
   check = staticmethod(check)

class CIGrey16Image(CustomIcon):
   def get_icon():
      return wxIconFromBitmap(gamera_icons.getIconImageGrey16Bitmap())
   get_icon = staticmethod(get_icon)

   def check(data):
      return isinstance(data, Image) and data.data.pixel_type == GREY16
   check = staticmethod(check)

class CIFloatImage(CustomIcon):
   def get_icon():
      return wxIconFromBitmap(gamera_icons.getIconImageFloatBitmap())
   get_icon = staticmethod(get_icon)

   def check(data):
      return isinstance(data, Image) and data.data.pixel_type == FLOAT
   check = staticmethod(check)

class CIOneBitImage(CustomIcon):
   def get_icon():
      return wxIconFromBitmap(gamera_icons.getIconImageBinaryBitmap())
   get_icon = staticmethod(get_icon)

   def check(data):
      return isinstance(data, Image) and data.data.pixel_type == ONEBIT
   check = staticmethod(check)

class CIRGBSubImage(CustomIcon):
   def get_icon():
      return wxIconFromBitmap(gamera_icons.getIconSubimageRgbBitmap())
   get_icon = staticmethod(get_icon)

   def check(data):
      return isinstance(data, SubImage) and data.data.pixel_type == RGB
   check = staticmethod(check)

class CIGreyScaleSubImage(CustomIcon):
   def get_icon():
      return wxIconFromBitmap(gamera_icons.getIconSubimageGreyBitmap())
   get_icon = staticmethod(get_icon)

   def check(data):
      return isinstance(data, SubImage) and data.data.pixel_type == GREYSCALE
   check = staticmethod(check)

class CIGrey16SubImage(CustomIcon):
   def get_icon():
      return wxIconFromBitmap(gamera_icons.getIconSubimageGrey16Bitmap())
   get_icon = staticmethod(get_icon)

   def check(data):
      return isinstance(data, SubImage) and data.data.pixel_type == GREY16
   check = staticmethod(check)

class CIFloatSubImage(CustomIcon):
   def get_icon():
      return wxIconFromBitmap(gamera_icons.getIconSubimageFloatBitmap())
   get_icon = staticmethod(get_icon)

   def check(data):
      return isinstance(data, SubImage) and data.data.pixel_type == FLOAT
   check = staticmethod(check)

class CIOneBitSubImage(CustomIcon):
   def get_icon():
      return wxIconFromBitmap(gamera_icons.getIconSubimageBinaryBitmap())
   get_icon = staticmethod(get_icon)

   def check(data):
      return isinstance(data, SubImage) and data.data.pixel_type == ONEBIT
   check = staticmethod(check)

class CIComplexSubImage(CustomIcon):
   def get_icon():
      return wxIconFromBitmap(gamera_icons.getIconSubimageComplexBitmap())
   get_icon = staticmethod(get_icon)

   def check(data):
      return isinstance(data, SubImage) and data.data.pixel_type == COMPLEX
   check = staticmethod(check)

class CICC(CustomIcon):
   def get_icon():
      return wxIconFromBitmap(gamera_icons.getIconCcBitmap())
   get_icon = staticmethod(get_icon)

   def check(data):
      return isinstance(data, Cc)
   check = staticmethod(check)

class CIImageList(CustomIcon):
   def __init__(self, *args):
      CustomIcon.__init__(self, *args)
      self.extra_method = {
        'List': {'XML': {'glyphs_to_xml': self.glyphs_to_xml},
                'Features': {'generate_features_list' : self.generate_features}}}

   def get_icon():
      return wxIconFromBitmap(gamera_icons.getIconImageListBitmap())
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
      return wxIconFromBitmap(gamera_icons.getIconClassifyBitmap())
   get_icon = staticmethod(get_icon)

   def check(data):
      return isinstance(data, classify.InteractiveClassifier)
   check = staticmethod(check)

   def right_click(self, *args):
      pass

   def control_s(self): pass

class CINonInteractiveClassifier(CustomIcon):
   def get_icon():
      return wxIconFromBitmap(gamera_icons.getIconNoninterClassifyBitmap())
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
      f = gamera_display.GraphDisplay(self.data)
      f.Show(1)

   def right_click(self, *args):
      pass

   def control_s(self):
      pass

   def drag(self):
      return ("Vector", str(self.data))

class CIIntVector(_CIVector):
   typecode = 'i'
   klass = int
   
   def get_icon():
      return wxIconFromBitmap(gamera_icons.getIntVectorBitmap())
   get_icon = staticmethod(get_icon)

class CIFloatVector(_CIVector):
   typecode = 'd'
   klass = float
   
   def get_icon():
      return wxIconFromBitmap(gamera_icons.getFloatVectorBitmap())
   get_icon = staticmethod(get_icon)

class CIComplexVector(_CIVector):
   typecode = None
   klass = complex
   
   def get_icon():
      return wxIconFromBitmap(gamera_icons.getComplexVectorBitmap())
   get_icon = staticmethod(get_icon)

builtin_icon_types = (
  CICC, CIRGBImage, CIComplexImage, CIGreyScaleImage, CIGrey16Image,
  CIFloatImage, CIOneBitImage, CIRGBSubImage,
  CIGreyScaleSubImage, CIGrey16SubImage, CIFloatSubImage,
  CIOneBitSubImage, CIComplexSubImage, CIImageList, CIInteractiveClassifier,
  CINonInteractiveClassifier, CIIntVector, CIFloatVector, CIComplexVector)
