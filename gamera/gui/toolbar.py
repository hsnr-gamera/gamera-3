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

from wxPython.wx import *         # wxPython
from wxPython.lib.buttons import *
from gamera.gui import gamera_icons

# This is our own custom toolbar class.
# We had to implement our own, since the one in wxWindows one supports
# one toolbar per wxFrame (on MSW, at least).  Since we want two
# independent toolbars in the classifier window, we have to create our
# own toolbar using a wxPanel and a wxBoxSizer.

class ToolBar(wxPanel):
    def __init__(self, parent, id=-1, hideable=1):
        self._close_toolbar_bitmap = gamera_icons.getToolbarCloseBitmap()
        self._open_toolbar_bitmap = gamera_icons.getToolbarOpenBitmap()
        self.controls = []
        self.layout_update_controls = []
        self.sizer = wxBoxSizer(wxHORIZONTAL)
        wxPanel.__init__(self, parent, id,
                         style=wxCLIP_CHILDREN|wxNO_FULL_REPAINT_ON_RESIZE)
        if hideable:
            self.close_button = wxBitmapButton(
                self, 1000,
                self._close_toolbar_bitmap,
                size=wxSize(11, 28))
            self.close_button.SetToolTipString("Hide Toolbar")
            self.sizer.Add(self.close_button)
            EVT_BUTTON(self, 1000, self.OnHideToolbar)
            self.close_button.SetCursor(wxStockCursor(wxCURSOR_HAND))
            self.open_button = wxBitmapButton(
                self, 1001,
                self._open_toolbar_bitmap,
                pos=wxPoint(0,0),
                size=wxSize(28, 11))
            self.open_button.Hide()
            self.open_button.SetToolTipString("Show Toolbar")
            EVT_BUTTON(self, 1001, self.OnShowToolbar)
            self.open_button.SetCursor(wxStockCursor(wxCURSOR_HAND))
            self.AddSeparator()
        self.SetSizer(self.sizer)
        self._closed = 0

    def AddSimpleTool(self, id, bitmap, help_string, callback=None, toggle=0):
        if not toggle:
            button = wxGenBitmapButton(self, id, bitmap, size=wxSize(28,28))
        else:
            button = wxGenBitmapToggleButton(self, id, bitmap, size=wxSize(28,28))
        button.SetBezelWidth(1)
        button.SetUseFocusIndicator(false)
        button.SetToolTipString(help_string)
        if callback:
            EVT_BUTTON(self, id, callback)
        self.sizer.Add(button, flag=wxALIGN_CENTER)
        self.sizer.SetSizeHints(self)
        self.controls.append(button)
        return button

    def AddMenuTool(self, id, text, help_string, callback=None, toggle=0):
        if not toggle:
            button = wxGenBitmapTextButton(self, id, None, text, size=wxSize(48, 28))
        else:
            button = wxGenBitmapTextToggleButton(self, id, None, text, size=wxSize(48,28))
        button.SetBitmapLabel(gamera_icons.getToolbarMenuBitmap())
        button.SetBezelWidth(1)
        button.SetUseFocusIndicator(false)
        button.SetToolTipString(help_string)
        if callback:
            EVT_BUTTON(self, id, callback)
        self.sizer.Add(button, flag=wxALIGN_CENTER)
        self.sizer.SetSizeHints(self)
        self.controls.append(button)
        return button

    def AddControl(self, control):
        self.sizer.Add(control, flag=wxALIGN_CENTER)
        self.sizer.SetSizeHints(self)
        self.controls.append(control)

    def AddSeparator(self):
        self.sizer.Add(wxPanel(self, -1, size=wxSize(5, 2)))
        self.sizer.SetSizeHints(self)

    def OnHideToolbar(self, event):
        self.close_button.Hide()
        self.open_button.Show()
        for control in self.controls:
            control.Hide()
        self.sizer.RecalcSizes()
        self.SetSizeHints(-1, -1, -1, -1, -1, -1)
        self.SetSize(wxSize(self.GetSize().x, 12))
        self.Layout()
        self.GetParent().Layout()

    def OnShowToolbar(self, event):
        self.close_button.Show()
        self.open_button.Hide()
        for control in self.controls:
            control.Show()
        self.sizer.SetSizeHints(self)
        self.Layout()
        self.GetParent().Layout()
    
