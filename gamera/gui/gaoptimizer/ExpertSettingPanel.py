#
# Copyright (C) 2012 Tobias Bolten
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

import wx

#-------------------------------------------------------------------------------
class ExpertSettingPanel(wx.Panel):
#-------------------------------------------------------------------------------
    #---------------------------------------------------------------------------
    def __init__(self, parent, id):
    #---------------------------------------------------------------------------
        wx.Panel.__init__(self, parent, id)

        # for storing widget category
        self.genericWidgets = []
        self.selectionWidgets = []
        self.weightingWidgets = []

        # for storing the activation dependencies between the widgets
        self.childWidgets = {}

    #---------------------------------------------------------------------------
    def GetAllSettingWidgets(self):
    #---------------------------------------------------------------------------
        return self.genericWidgets + self.selectionWidgets + \
            self.weightingWidgets
        
    #---------------------------------------------------------------------------
    def AddChildToParent(self, parent, child):
    #---------------------------------------------------------------------------
        if parent in self.childWidgets.keys():
            self.childWidgets[parent].append(child)
        else:
            self.childWidgets[parent] = [child]

    #---------------------------------------------------------------------------
    def BindEvent(self, event, function, widgetsList):
    #---------------------------------------------------------------------------
        for widget in widgetsList:
            self.Bind(event, function, widget)

    #---------------------------------------------------------------------------
    def OnCheckBox(self, event):
    #---------------------------------------------------------------------------
        activeWidget = event.GetEventObject()

        if activeWidget.GetValue():
            if activeWidget in self.childWidgets.keys():
                for widget in self.childWidgets[activeWidget]:
                    widget.Enable()
        else:
            if activeWidget in self.childWidgets.keys():
                for widget in self.childWidgets[activeWidget]:
                    widget.Disable()
                
    #---------------------------------------------------------------------------
    def OnRadioButton(self, event):
    #---------------------------------------------------------------------------
        activeRadioButton = event.GetEventObject()
        activeRadioLabel = activeRadioButton.GetLabel()
        
        for widget in self.childWidgets.keys():
            if widget.GetLabel() != activeRadioLabel:
                for child in self.childWidgets[widget]:
                    child.Disable()
            else:
                for child in self.childWidgets[widget]:
                    child.Enable()
