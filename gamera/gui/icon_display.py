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

from wxPython.wx import *                    # wxPython
from gamera.core import *                         # Gamera specific
from gamera import paths, util
from gamera.gui import image_menu, var_name

######################################################################

class Icon:
  data = None
  index = None
  label = None
  type = None

  def __init__(self, label_, data_, index_, type_):
    self.label = label_
    self.data = data_
    self.index = index_
    self.type = type_

######################################################################

class IconDisplayDropTarget(wxFileDropTarget, wxPyDropTarget):
  def __init__(self, parent):
    wxFileDropTarget.__init__(self)
    self.parent = parent

  def OnDropFiles(self, x, y, filenames):
    for filename in filenames:
      name = var_name.get("image", self.display.shell.locals)
      self.display.shell.run(name + '_' + str(i) +
                             " = load_image('" + filename + "')")


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
      "Right-click to perform functions.\n" +
      "Shift-right-click for help."))
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
      if klass[0].__class__ == icon_description:
        add_it = 0
        break
    if add_it:
      id = icon_description()
      icon = self.il.AddIcon(id.get_icon())
      self.classes.append((id, icon))
    
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
    
  def refresh_icon(self, key, data, icon):
    if data.type != self.data[key].type:
      index = self.data[key].index
      data.index = index
      self.data[key] = data
      self.SetStringItem(index, 0, key, icon)

  def remove_icon(self, key):
    if isinstance(self.data[key].data, Image):
      # removed custom callback from Image in gamera.py
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
        obj = Icon(i[0], i[1], 0, t)
        if self.data.has_key(i[0]):
          self.refresh_icon(i[0], obj, icon)
        else:
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
    self.currentIconName = self.currentIcon.label

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
    self.currentIcon = self.find_icon(index)
    if self.currentIcon:
      if isinstance(self.currentIcon.data, Image):
        if event.ShiftDown():
          mode = image_menu.HELP_MODE
        else:
          mode = image_menu.EXECUTE_MODE
          menu = image_menu.ImageMenu(self, self.x, self.y,
                                        self.currentIcon.data,
                                        self.currentIcon.label,
                                        self.shell, mode)
          menu.PopupMenu()
    event.Skip()

  def OnDoubleClick(self, event):
    if self.currentIcon:
      if isinstance(self.currentIcon.data, Image):
        source = (self.currentIconName + ".display()")
      else:
        source = "display_multi(" + self.currentIconName + ")"
      self.shell.run(source)

# Standard icons for core Gamera
 
class CustomIcon:
  is_custom_icon_description = 1

  def get_icon(self):
    from gamera.gui import gamera_icons
    icon = wxIconFromBitmap(gamera_icons.getIconImageUnknownBitmap())
    return icon

  def check(self, data):
    return 1

class CIRGBImage(CustomIcon):
  def get_icon(self):
    from gamera.gui import gamera_icons
    icon = wxIconFromBitmap(gamera_icons.getIconImageRgbBitmap())
    return icon

  def check(self, data):
    return isinstance(data, Image) and data.data.pixel_type == RGB

class CIGreyScaleImage(CustomIcon):
  def get_icon(self):
    from gamera.gui import gamera_icons
    icon = wxIconFromBitmap(gamera_icons.getIconImageGreyBitmap())
    return icon

  def check(self, data):
    return isinstance(data, Image) and data.data.pixel_type == GREYSCALE

class CIGrey16Image(CustomIcon):
  def get_icon(self):
    from gamera.gui import gamera_icons
    icon = wxIconFromBitmap(gamera_icons.getIconImageGrey16Bitmap())
    return icon

  def check(self, data):
    return isinstance(data, Image) and data.data.pixel_type == GREY16

class CIFloatImage(CustomIcon):
  def get_icon(self):
    from gamera.gui import gamera_icons
    icon = wxIconFromBitmap(gamera_icons.getIconImageFloatBitmap())
    return icon

  def check(self, data):
    return isinstance(data, Image) and data.data.pixel_type == FLOAT

class CIOneBitImage(CustomIcon):
  def get_icon(self):
    from gamera.gui import gamera_icons
    icon = wxIconFromBitmap(gamera_icons.getIconImageBinaryBitmap())
    return icon

  def check(self, data):
    return isinstance(data, Image) and data.data.pixel_type == ONEBIT

class CIRGBSubImage(CustomIcon):
  def get_icon(self):
    from gamera.gui import gamera_icons
    icon = wxIconFromBitmap(gamera_icons.getIconSubimageRgbBitmap())
    return icon

  def check(self, data):
    return isinstance(data, SubImage) and data.data.pixel_type == RGB

class CIGreyScaleSubImage(CustomIcon):
  def get_icon(self):
    from gamera.gui import gamera_icons
    icon = wxIconFromBitmap(gamera_icons.getIconSubimageGreyBitmap())
    return icon

  def check(self, data):
    return isinstance(data, SubImage) and data.data.pixel_type == GREYSCALE

class CIGrey16SubImage(CustomIcon):
  def get_icon(self):
    from gamera.gui import gamera_icons
    icon = wxIconFromBitmap(gamera_icons.getIconSubimageGrey16Bitmap())
    return icon

  def check(self, data):
    return isinstance(data, SubImage) and data.data.pixel_type == GREY16

class CIFloatSubImage(CustomIcon):
  def get_icon(self):
    from gamera.gui import gamera_icons
    icon = wxIconFromBitmap(gamera_icons.getIconSubimageFloatBitmap())
    return icon

  def check(self, data):
    return isinstance(data, SubImage) and data.data.pixel_type == FLOAT

class CIOneBitSubImage(CustomIcon):
  def get_icon(self):
    from gamera.gui import gamera_icons
    icon = wxIconFromBitmap(gamera_icons.getIconSubimageBinaryBitmap())
    return icon

  def check(self, data):
    return isinstance(data, SubImage) and data.data.pixel_type == ONEBIT

class CICC(CustomIcon):
  def get_icon(self):
    from gamera.gui import gamera_icons
    icon = wxIconFromBitmap(gamera_icons.getIconCcBitmap())
    return icon
    
  def check(self, data):
    return isinstance(data, Cc)

class CIImageList(CustomIcon):
  def get_icon(self):
    from gamera.gui import gamera_icons
    icon = wxIconFromBitmap(gamera_icons.getIconImageListBitmap())
    return icon
  
  def check(self, data):
    return util.is_sequence(data)

builtin_icon_types = (CICC, CIRGBImage, CIGreyScaleImage, CIGrey16Image,
                      CIFloatImage, CIOneBitImage,
                      CIRGBSubImage, CIGreyScaleSubImage, CIGrey16SubImage,
                      CIFloatSubImage, CIOneBitSubImage,
                      CIImageList)
