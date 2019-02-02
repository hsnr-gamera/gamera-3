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

try:
    from agw import floatspin as FS
except ImportError:
    try:
        import wx.lib.agw.floatspin as FS
    except ImportError:
        raise RuntimeError("Biollante requires at least wxPython 2.8.11")

from gamera.gui import compat_wx
from gamera.gui.gaoptimizer.ExpertSettingPanel import *

#-------------------------------------------------------------------------------
class MutationPanel(ExpertSettingPanel):
#-------------------------------------------------------------------------------
    #---------------------------------------------------------------------------
    def __init__(self, parent, id):
    #---------------------------------------------------------------------------
        ExpertSettingPanel.__init__(self, parent, id)

        sizer = wx.BoxSizer(wx.VERTICAL)
        self.SetSizer(sizer)

        # ---------------- generic mutations ----------------
        genericBox = wx.StaticBox(self, -1, "Generic operations", size=(500,-1))
        genericBoxSizer = wx.StaticBoxSizer(genericBox, wx.HORIZONTAL)
        sizer.Add(genericBoxSizer, 0, wx.LEFT | wx.TOP | wx.RIGHT | wx.EXPAND, 10)
        
        # shift mutation
        self.shiftMutation = wx.CheckBox(self, -1, "Shift Mutation", \
            name = "shiftMutation")
        genericBoxSizer.Add(self.shiftMutation, 0, \
            wx.LEFT | wx.TOP | wx.RIGHT | wx.EXPAND, 5)

        self.genericWidgets.append(self.shiftMutation)
        
        # swap mutation
        self.swapMutation = wx.CheckBox(self, -1, "Swap Mutation", \
            name = "swapMuation")
        self.swapMutation.SetValue(True)
        genericBoxSizer.Add(self.swapMutation, 0, \
            wx.LEFT | wx.TOP | wx.RIGHT | wx.EXPAND, 5)

        self.genericWidgets.append(self.swapMutation)
        
        # inversion mutation
        self.inversionOrderMutation = wx.CheckBox(self, -1, "Inversion Order", \
            name = "inversionMutation")
        genericBoxSizer.Add(self.inversionOrderMutation, 0, \
            wx.LEFT | wx.TOP | wx.RIGHT | wx.EXPAND, 5)

        self.genericWidgets.append(self.inversionOrderMutation)
        
        # ---------------- binary mutations ----------------
        binaryBox = wx.StaticBox(self, -1, "Binary operations (affects only feature selection)", size=(500,-1))
        binaryBoxSizer = wx.StaticBoxSizer(binaryBox, wx.HORIZONTAL)
        sizer.Add(binaryBoxSizer, 0, wx.LEFT | wx.TOP | wx.RIGHT | wx.EXPAND, 10)
        
        # bit flip mutation
        self.binaryMutation = wx.CheckBox(self, -1, "Binary Mutation", \
            name = "binaryMutation")
        self.binaryMutation.SetValue(True)
        binaryBoxSizer.Add(self.binaryMutation, 0, \
            wx.LEFT | wx.TOP | wx.RIGHT | wx.EXPAND, 5)

        self.binaryMutationRate = FS.FloatSpin(self, -1, min_val=0.0, max_val=1.0, \
            increment=0.01, value=0.05)
        self.binaryMutationRate.SetFormat("%f")
        self.binaryMutationRate.SetDigits(2)
        compat_wx.set_tool_tip(self.binaryMutationRate, "Rate of mutation")
        binaryBoxSizer.Add(self.binaryMutationRate, 0, wx.LEFT | wx.TOP | wx.RIGHT, 5)
        
        self.selectionWidgets.append(self.binaryMutation)
        self.AddChildToParent(self.binaryMutation, self.binaryMutationRate)
        
        # ---------------- real mutations ----------------
        realBox = wx.StaticBox(self, -1, "Real operations (affects only feature weighting)", size=(500,-1))
        realBoxSizer = wx.StaticBoxSizer(realBox, wx.VERTICAL)
        realBoxFlexGridSizer = wx.FlexGridSizer(0, 3, 5, 5)
        realBoxSizer.Add(realBoxFlexGridSizer, 0)
        sizer.Add(realBoxSizer, 0, wx.ALL | wx.EXPAND, 10)
        
        # gauss mutation
        self.gaussMutation = wx.CheckBox(self, -1, "Gauss Mutation", \
            name = "gaussMutation")
        self.gaussMutation.SetValue(True)
        self.gaussMutation.Disable()
        realBoxFlexGridSizer.Add(self.gaussMutation, 0, \
            wx.LEFT | wx.TOP | wx.RIGHT | wx.EXPAND, 5)
        
        self.gaussMutationSigma = FS.FloatSpin(self, -1, min_val=0.01, max_val=10.0, \
            increment=0.01, value=0.5)
        self.gaussMutationSigma.SetFormat("%f")
        self.gaussMutationSigma.SetDigits(2)
        self.gaussMutationSigma.Disable()
        compat_wx.set_tool_tip(self.gaussMutationSigma, "sigma the range for uniform nutation")
        realBoxFlexGridSizer.Add(self.gaussMutationSigma, 0, wx.LEFT | wx.TOP | wx.RIGHT, 5)
        
        self.gaussMutationPchance = FS.FloatSpin(self, -1, min_val=0.0, max_val=1.0, \
            increment=0.01, value=0.5)
        self.gaussMutationPchance.SetFormat("%f")
        self.gaussMutationPchance.SetDigits(2)
        self.gaussMutationPchance.Disable()
        compat_wx.set_tool_tip(self.gaussMutationPchance, "p_change the probability to "\
            "change a given coordinate")
        realBoxFlexGridSizer.Add(self.gaussMutationPchance, 0, wx.LEFT | wx.TOP | wx.RIGHT, 5)
        
        self.weightingWidgets.append(self.gaussMutation)
        self.AddChildToParent(self.gaussMutation, self.gaussMutationSigma)
        self.AddChildToParent(self.gaussMutation, self.gaussMutationPchance)
            
        # bind the EVT_CHECKBOX to the CheckBoxes
        self.BindEvent(wx.EVT_CHECKBOX, self.OnCheckBox, \
            [self.shiftMutation,
             self.swapMutation,
             self.inversionOrderMutation,
             self.binaryMutation,
             self.gaussMutation])
