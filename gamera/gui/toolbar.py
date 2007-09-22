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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#

import wx
from wx.lib import buttons
from gamera.gui import gamera_icons

# This is our own custom toolbar class.
# We had to implement our own, since the one in wxWindows one supports
# one toolbar per wxFrame (on MSW, at least).  Since we want two
# independent toolbars in the classifier window, we have to create our
# own toolbar using a wxPanel and a wxBoxSizer.

if hasattr(buttons, 'ThemedGenBitmapButton'):
   ButtonClass = buttons.ThemedGenBitmapButton
   ToggleButtonClass = buttons.ThemedGenBitmapToggleButton
else:
   ButtonClass = buttons.GenBitmapButton
   ToggleButtonClass = buttons.GenBitmapToggleButton

class ToolBar(wx.Panel):
   def __init__(self, parent, id=-1, hideable=1):
      self._close_toolbar_bitmap = gamera_icons.getToolbarCloseBitmap()
      self._open_toolbar_bitmap = gamera_icons.getToolbarOpenBitmap()
      self.controls = []
      self.layout_update_controls = []
      self.sizer = wx.BoxSizer(wx.HORIZONTAL)
      wx.Panel.__init__(
          self, parent, id,
          style=wx.CLIP_CHILDREN|wx.NO_FULL_REPAINT_ON_RESIZE)
      self.SetSizer(self.sizer)
      self._closed = 0

   def AddSimpleTool(self, id, bitmap, help_string, callback=None, toggle=0):
      if not toggle:
         button = ButtonClass(self, id, bitmap, size=wx.Size(30,30))
      else:
         button = ToggleButtonClass(self, id, bitmap, size=wx.Size(30,30))
      button.SetBezelWidth(1)
      button.SetUseFocusIndicator(False)
      button.SetToolTipString(help_string)
      if callback:
         wx.EVT_BUTTON(self, id, callback)
      self.sizer.Add(button, flag=wx.ALIGN_CENTER)
      self.sizer.SetSizeHints(self)
      self.controls.append(button)
      return button

   def AddMenuTool(self, id, text, help_string, callback=None, toggle=0):
      if not toggle:
         button = buttons.GenBitmapTextButton(
             self, id, None, text, size=wx.Size(48, 28))
      else:
         button = buttons.GenBitmapTextToggleButton(
             self, id, None, text, size=wx.Size(48,28))
      button.SetBitmapLabel(gamera_icons.getToolbarMenuBitmap())
      button.SetBezelWidth(1)
      button.SetUseFocusIndicator(False)
      button.SetToolTipString(help_string)
      if callback:
         wx.EVT_BUTTON(self, id, callback)
      self.sizer.Add(button, flag=wx.ALIGN_CENTER)
      self.sizer.SetSizeHints(self)
      self.controls.append(button)
      return button

   def AddControl(self, control):
      self.sizer.Add(control, flag=wx.ALIGN_CENTER)
      self.sizer.SetSizeHints(self)
      self.controls.append(control)

   def AddSeparator(self):
      self.sizer.Add(wx.Panel(self, -1, size=wx.Size(5, 2)))
      self.sizer.SetSizeHints(self)

   def OnHideToolbar(self, event):
      self.close_button.Hide()
      self.open_button.Show()
      for control in self.controls:
         control.Hide()
      self.SetSizeHints(-1, -1, -1, -1, -1, -1)
      self.SetSize(wx.Size(self.GetSize().x, 12))
      self.Layout()
      self.GetParent().Layout()
      self.GetParent().Refresh()

   def OnShowToolbar(self, event):
      self.close_button.Show()
      self.open_button.Hide()
      for control in self.controls:
         control.Show()
      self.sizer.SetSizeHints(self)
      self.Layout()
      self.GetParent().Layout()
      self.GetParent().Refresh()
