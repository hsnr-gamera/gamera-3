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
from gamera.gamera import *                         # Gamera specific
from gamera import paths, util
from gamera.gui import matrix_menu, var_name

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
    name = var_name.get("image", self.display.shell.locals)
    if name == '':
      return
    if len(filenames) == 1:
        self.display.shell.run(name +
                                    " = load_image('" + filenames[0] + "')")
    else:
      i = 0
      for filename in filenames:
        i = i + 1
        self.display.shell.run(name + '_' + str(i) +
                                    " = load_image('" + filename + "')")


######################################################################

class IconDisplay(wxListCtrl):
  def __init__(self, parent):
    wxListCtrl.__init__(self, parent , -1, (0,0), (-1,-1), wxLC_ICON)
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
    self.SetImageList(self.il, wxIMAGE_LIST_NORMAL)
    
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

    EVT_RIGHT_DOWN(self, self.OnRightDown)
    # for wxMSW
    EVT_COMMAND_RIGHT_CLICK(self, tID, self.OnRightClick)
    # for wxGTK
    EVT_RIGHT_UP(self, self.OnRightClick)
    EVT_LEFT_DOWN(self, self.OnLeftDown)
    EVT_LEFT_UP(self, self.OnLeftUp)

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
    if isinstance(self.data[key].data, Matrix):
      # removed custom callback from Matrix in gamera.py
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
    self.currentIcon = self.find_icon(self.HitTest(
      wxPoint(event.GetX(), event.GetY()))[0])
    if self.currentIcon:
      if isinstance(self.currentIcon.data, Matrix):
        if event.ShiftDown():
          mode = matrix_menu.HELP_MODE
        else:
          mode = matrix_menu.EXECUTE_MODE
          menu = matrix_menu.MatrixMenu(self, self.x, self.y,
                                        self.currentIcon.data,
                                        self.currentIcon.label,
                                        self.shell, mode)
          menu.PopupMenu()
    event.Skip()

  def OnDoubleClick(self, event):
    if self.currentIcon:
      if isinstance(self.currentIcon.data, Matrix):
        source = (self.currentIconName + ".display()")
      else:
        source = "display_multi(" + self.currentIconName + ")"
      self.shell.run(source)

  def OnLeftDown(self, event):
    self.SetCursor(wxStockCursor(wxCURSOR_BULLSEYE))
    self.drag = self.find_icon(self.HitTest(
      wxPoint(event.GetX(), event.GetY()))[0])
    event.Skip()

  def OnLeftUp(self, event):
    self.SetCursor(wxSTANDARD_CURSOR)
    target = self.find_icon(self.HitTest(
      wxPoint(event.GetX(), event.GetY()))[0])
    if target == None or self.drag == None:
      return
    if target == self.drag:
      event.Skip()
    source = self.drag.data
    target = target.data
    if (isinstance(target, Matrix)
        and not isinstance(target, SubMatrix)
        and not isinstance(target, CC)):
      if (isinstance(source, CC) or
          isinstance(source, SubMatrix) or
          (util.is_sequence(source) and
           len(source) > 0 and
          (isinstance(source[0], CC) or
           isinstance(source[0], SubMatrix)))):
        target.display_cc(source)
        self.update_icons()
    elif (util.is_sequence(target) and
          util.is_sequence(source)):
      target = target + source
      self.update_icons()


# Standard icons for core Gamera
 
class CustomIcon:
  is_custom_icon_description = 1

  def get_icon(self):
    return wxIcon(paths.pixmaps + "icon_matrix_unknown.png", wxBITMAP_TYPE_PNG)

  def check(self, data):
    return 1

class CIRGBMatrix(CustomIcon):
  def get_icon(self):
    return wxIcon(paths.pixmaps + "icon_matrix_rgb.png", wxBITMAP_TYPE_PNG)

  def check(self, data):
    return isinstance(data, Matrix) and data.get_type() == "RGB"

class CIGreyScaleMatrix(CustomIcon):
  def get_icon(self):
    return wxIcon(paths.pixmaps + "icon_matrix_grey.png", wxBITMAP_TYPE_PNG)

  def check(self, data):
    return isinstance(data, Matrix) and data.get_type() == "GreyScale"

class CIGrey16Matrix(CustomIcon):
  def get_icon(self):
    return wxIcon(paths.pixmaps + "icon_matrix_grey16.png", wxBITMAP_TYPE_PNG)

  def check(self, data):
    return isinstance(data, Matrix) and data.get_type() == "Grey16"

class CIFloatMatrix(CustomIcon):
  def get_icon(self):
    return wxIcon(paths.pixmaps + "icon_matrix_float.png", wxBITMAP_TYPE_PNG)

  def check(self, data):
    return isinstance(data, Matrix) and data.get_type() == "Float"

class CIOneBitMatrix(CustomIcon):
  def get_icon(self):
    return wxIcon(paths.pixmaps + "icon_matrix_binary.png", wxBITMAP_TYPE_PNG)

  def check(self, data):
    return isinstance(data, Matrix) and data.get_type() == "OneBit"

class CIRGBSubMatrix(CustomIcon):
  def get_icon(self):
    return wxIcon(paths.pixmaps + "icon_submatrix_rgb.png", wxBITMAP_TYPE_PNG)

  def check(self, data):
    return isinstance(data, SubMatrix) and data.get_type() == "RGB"

class CIGreyScaleSubMatrix(CustomIcon):
  def get_icon(self):
    return wxIcon(paths.pixmaps + "icon_submatrix_grey.png", wxBITMAP_TYPE_PNG)

  def check(self, data):
    return isinstance(data, SubMatrix) and data.get_type() == "GreyScale"

class CIGrey16SubMatrix(CustomIcon):
  def get_icon(self):
    return wxIcon(paths.pixmaps + "icon_submatrix_grey16.png", wxBITMAP_TYPE_PNG)

  def check(self, data):
    return isinstance(data, SubMatrix) and data.get_type() == "Grey16"

class CIFloatSubMatrix(CustomIcon):
  def get_icon(self):
    return wxIcon(paths.pixmaps + "icon_submatrix_float.png", wxBITMAP_TYPE_PNG)

  def check(self, data):
    return isinstance(data, SubMatrix) and data.get_type() == "Float"

class CIOneBitSubMatrix(CustomIcon):
  def get_icon(self):
    return wxIcon(paths.pixmaps + "icon_submatrix_binary.png", wxBITMAP_TYPE_PNG)

  def check(self, data):
    return isinstance(data, SubMatrix) and data.get_type() == "OneBit"

class CICC(CustomIcon):
  def get_icon(self):
    return wxIcon(paths.pixmaps + "icon_cc.png", wxBITMAP_TYPE_PNG)
    
  def check(self, data):
    return isinstance(data, CC)

class CIMatrixList(CustomIcon):
  def get_icon(self):
    return wxIcon(paths.pixmaps + "icon_matrix_list.png", wxBITMAP_TYPE_PNG)
  
  def check(self, data):
    return util.is_sequence(data)

builtin_icon_types = (CICC, CIRGBMatrix, CIGreyScaleMatrix, CIGrey16Matrix,
                      CIFloatMatrix, CIOneBitMatrix,
                      CIRGBSubMatrix, CIGreyScaleSubMatrix, CIGrey16SubMatrix,
                      CIFloatSubMatrix, CIOneBitSubMatrix,
                      CIMatrixList)
