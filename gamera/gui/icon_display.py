#
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

from wxPython.wx import *                    # wxPython
from gamera.core import *                         # Gamera specific
from gamera import paths, util, classify
from gamera.gui import image_menu, var_name, gamera_icons

######################################################################

######################################################################

class IconDisplayDropTarget(wxFileDropTarget, wxPyDropTarget):
  def __init__(self, parent):
    wxFileDropTarget.__init__(self)
    self.parent = parent

  def OnDropFiles(self, x, y, filenames):
    for filename in filenames:
      name = var_name.get("image", self.display.shell.locals)
      self.display.shell.run(name + " = load_image('" + filename + "')")


######################################################################

class IconDisplay(wxListCtrl):
  def __init__(self, parent):
    wxListCtrl.__init__(self, parent , -1, (0,0), (-1,-1),
                        wxLC_LIST|wxLC_SINGLE_SEL|wxLC_ALIGN_TOP)
    self.data = {}
    self.locals = {}
    self.currentIcon = None
    self.currentIconName = None
    self.init_events()
    self.SetToolTip(wxToolTip(
      "Double-click to display.\n" + 
      "Right-click to perform functions.\n"))
    self.help_mode = 0
    self.dt = IconDisplayDropTarget(self)
    self.dt.display = self
    self.SetDropTarget(self.dt)
    self.il = wxImageList(32, 32)
    self.classes = []
    for klass in builtin_icon_types:
      self.add_class(klass)
    self.SetImageList(self.il, wxIMAGE_LIST_SMALL)
    
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
    EVT_LEFT_DCLICK(self, self.OnDoubleClick)
    EVT_LIST_ITEM_SELECTED(self, tID, self.OnItemSelected)
    EVT_LIST_ITEM_ACTIVATED(self, tID, self.OnItemSelected)

    # for wxMSW
    EVT_COMMAND_RIGHT_CLICK(self, tID, self.OnRightClick)
    # for wxGTK
    EVT_RIGHT_UP(self, self.OnRightClick)
    EVT_RIGHT_DOWN(self, self.OnRightDown)

  def add_icon(self, label, data, icon):
    index = self.GetItemCount()
    data.index = index
    self.data[label] = data
    self.InsertImageStringItem(index, label, icon)
    
  def refresh_icon(self, key, klass, data, icon):
    if klass != self.data[key].__class__:
      index = self.data[key].index
      obj = klass(key, data, index)
      self.data[key] = obj
      self.SetStringItem(index, 0, key, icon)

  def remove_icon(self, key):
    if self.data.has_key(key):
      del self.data[key].data
    index = self.data[key].index
    self.DeleteItem(index)
    del self.data[key]
    for i in self.data.values():
      if i.index > index:
        i.index = i.index - 1

  def update_icons(self, locals=None):
    if locals != None:
      self.locals = locals
    okay = []
    for i in self.locals.items():
      t = None
      for klass, icon in self.classes:
        try:
          if klass.check(i[1]):
            t = klass
            break
        except:
          pass
      if t != None:
        if self.data.has_key(i[0]):
          self.refresh_icon(i[0], t, i[1], icon)
        else:
          obj = t(i[0], i[1], 0)
          self.add_icon(i[0], obj, icon)
        okay.append(i[0])
      elif self.data.has_key(i[0]):
        self.remove_icon(i[0])
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

  def OnRightDown(self, event):
    self.x = event.GetX()
    self.y = event.GetY()
    event.Skip()

  def OnRightClick(self, event):
    index = self.HitTest(
      wxPoint(event.GetX(), event.GetY()))[0]
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

# Standard icons for core Gamera
 
class CustomIcon:
  is_custom_icon_description = 1
  def __init__(self, label_, data_, index_):
    self.label = label_
    self.data = data_
    self.index = index_
  
  def get_icon():
    return wxIconFromBitmap(gamera_icons.getIconImageUnknownBitmap())
  get_icon = staticmethod(get_icon)

  def check(data):
    return 1
  check = staticmethod(check)

  def double_click(self):
    return "%s.display()" % self.label

  def right_click(self, parent, event, shell):
    image_menu.ImageMenu(
      parent, event.GetX(), event.GetY(),
      self.data, self.label,
      shell)

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

class CICC(CustomIcon):
  def get_icon():
    return wxIconFromBitmap(gamera_icons.getIconCcBitmap())
  get_icon = staticmethod(get_icon)
    
  def check(data):
    return isinstance(data, Cc)
  check = staticmethod(check)

class CIImageList(CustomIcon):
  def get_icon():
    return wxIconFromBitmap(gamera_icons.getIconImageListBitmap())
  get_icon = staticmethod(get_icon)

  def check(data):
    return util.is_homogeneous_image_list(data)
  check = staticmethod(check)

  def double_click(self):
    return 'display_multi(%s)' % self.label

class CIInteractiveClassifier(CustomIcon):
  def get_icon():
    return wxIconFromBitmap(gamera_icons.getIconClassifyBitmap())
  get_icon = staticmethod(get_icon)

  def check(data):
    return isinstance(data, classify.InteractiveClassifier)
  check = staticmethod(check)

  def right_click(self, *args):
    pass

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

builtin_icon_types = (
  CICC, CIRGBImage, CIGreyScaleImage, CIGrey16Image,
  CIFloatImage, CIOneBitImage, CIRGBSubImage,
  CIGreyScaleSubImage, CIGrey16SubImage, CIFloatSubImage,
  CIOneBitSubImage, CIImageList, CIInteractiveClassifier,
  CINonInteractiveClassifier)
