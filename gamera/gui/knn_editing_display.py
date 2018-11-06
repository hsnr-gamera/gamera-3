#
# GUI dialog to execute editing algorithms on a kNN Classifier
# Copyright (C) 2007, 2008 Colin Baumgarten
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

from gamera.knn_editing import AlgoRegistry
import wx
from gamera.gui.compatibility import create_help_display

class EditingDialog(object):
    """Dialog to apply any of the editing algorithms known to the 
*gamera.knn_editing.AlgoRegistry* to a given *kNNInteractive* Classifier. Lets 
the user select one of the algorithms, tune its parameters and then returns the 
edited result"""
    def __init__(self):
        self.selection = 0
        self.panel = None
        self._createDialog()

    def _createDialog(self):
        self.dlg = wx.Dialog(None, wx.NewId(), "Create edited Classifier")
        self.sizer = wx.FlexGridSizer(rows = 0, cols = 1)
        self.dlg.SetSizer(self.sizer)

        choiceSizer = wx.FlexGridSizer(rows = 1, cols = 0)
        statText = wx.StaticText(self.dlg, wx.NewId(), "Select an Editing Algorithm")
        choiceSizer.Add(statText, flag = wx.ALIGN_CENTER_VERTICAL | wx.RIGHT, border = 10)
        choiceId = wx.NewId()
        choiceControl = wx.Choice(self.dlg, choiceId, choices = self._algoNames())
        wx.EVT_CHOICE(self.dlg, choiceId, self._choiceCallback)
        choiceSizer.Add(choiceControl, flag = wx.ALIGN_RIGHT)
        choiceSizer.AddGrowableCol(0)

        self.sizer.Add(choiceSizer, flag = wx.EXPAND | wx.ALL, border = 10)
        self.sizer.Add(wx.StaticLine(self.dlg), flag = wx.EXPAND)

        self._replaceArgs()
        
        buttons = self.dlg.CreateButtonSizer(wx.OK | wx.CANCEL)
        #buttons.Realize() # does not work on Windows
        self.sizer.Add(buttons, flag = wx.ALL, border = 10)

        self.sizer.Fit(self.dlg)
        self.sizer.SetSizeHints(self.dlg)

    def _choiceCallback(self, event):
        """called, if the user selects an algorithm from the dropdown, so that 
the list of parameters can be updated""" 
        if event.GetSelection() != self.selection:
            self.selection = event.GetSelection()
            self._replaceArgs()
            self.sizer.Fit(self.dlg)
            self.sizer.SetSizeHints(self.dlg)
            
    def _replaceArgs(self):
        """replace (or initially create) the parameter input controls, according
to the just selected algorithm"""
        panel = wx.Panel(self.dlg)

        help = create_help_display(panel, self._selected().doc)
        help.SetInitialSize(wx.Size(50, 200))
        panelSizer = wx.FlexGridSizer(rows = 0, cols = 1)
        panelSizer.AddGrowableCol(0)
        panel.SetSizer(panelSizer)
        
        argControls = self._selected().args._create_controls(None, panel)
        panelSizer.Add(argControls, flag = wx.EXPAND)
        
        panelSizer.Add(help, flag = wx.EXPAND | wx.ALL, border = 10)
        panel.Layout()

        if self.panel == None:
            self.sizer.Add(panel, flag = wx.EXPAND)
        else:
            self.sizer.Replace(self.panel, panel)
            self.panel.Destroy()
            self.dlg.Layout()

        self.panel = panel

    def _algoNames(self):
        """Returns a list of all editing algorithm names as known to the 
*AlgoRegistry*"""        
        return [alg.name for alg in AlgoRegistry.algorithms]

    def _selected(self):
        """ Returns the metadata of the currently selected algorithm as a
*AlgoData* object"""
        return AlgoRegistry.algorithms[self.selection]

    def show(self, classifier):
        """Shows the dialog. In case the user clicks *OK*, applies the 
selected algorithm to the given *classifier* and returns the resulting one. 
Otherwise returns *None*""" 
        if self.dlg.ShowModal() == wx.ID_OK:
            args = self._selected().args.get_args()
            args.insert(0, classifier)
            args = tuple(args)
            return self._selected().callable(*args)
        else:
            return None
