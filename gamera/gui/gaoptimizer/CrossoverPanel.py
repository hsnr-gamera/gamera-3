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

from gamera.gui.gaoptimizer.ExpertSettingPanel import *

#-------------------------------------------------------------------------------
class CrossoverPanel(ExpertSettingPanel):
#-------------------------------------------------------------------------------
    #---------------------------------------------------------------------------
    def __init__(self, parent, id):
    #---------------------------------------------------------------------------
        ExpertSettingPanel.__init__(self, parent, id)

        sizer = wx.BoxSizer(wx.VERTICAL)
        self.SetSizer(sizer)

        # ---------------- generic crossovers ----------------
        genericBox = wx.StaticBox(self, -1, "Generic operations", size=(500,-1))
        genericBoxSizer = wx.StaticBoxSizer(genericBox, wx.VERTICAL)
        genericFlexGridSizer = wx.FlexGridSizer(0, 2, 5, 5)
        genericBoxSizer.Add(genericFlexGridSizer, 0)
        sizer.Add(genericBoxSizer, 0, wx.ALL | wx.EXPAND, 10)

        # n point crossover (default: one point crossover)
        nPointSizer = wx.BoxSizer(wx.HORIZONTAL)
        genericBoxSizer.Add(nPointSizer, 0, wx.EXPAND)
        
        self.nPointCrossover = wx.CheckBox(self, -1, "N-Point crossover", \
            name = "nPointCrossover")
        genericFlexGridSizer.Add(self.nPointCrossover, 0, \
            wx.LEFT | wx.TOP | wx.RIGHT | wx.EXPAND, 5)
        
        self.nPointCrossoverN = wx.SpinCtrl(self, -1, size=(100,-1), \
            min=1, max=10, value='1') # TODO: max = vector dimension
        self.nPointCrossoverN.Disable()
        self.nPointCrossoverN.SetToolTipString("num crossover points")
        genericFlexGridSizer.Add(self.nPointCrossoverN, 0, wx.LEFT | wx.TOP | wx.RIGHT, 5)

        self.genericWidgets.append(self.nPointCrossover)
        self.AddChildToParent(self.nPointCrossover, self.nPointCrossoverN)

        # uniform crossover
        self.uniformCrossover = wx.CheckBox(self, -1, "Uniform crossover", \
            name = "uniformCrossover")
        self.uniformCrossover.SetValue(True)
        genericFlexGridSizer.Add(self.uniformCrossover, 0, \
            wx.LEFT | wx.TOP | wx.RIGHT | wx.EXPAND, 5)

        self.uniformCrossoverPref = FS.FloatSpin(self, -1, min_val=0.0, max_val=1.0, \
            increment=0.01, value=0.5)
        self.uniformCrossoverPref.SetFormat("%f")
        self.uniformCrossoverPref.SetDigits(2)
        self.uniformCrossoverPref.SetToolTipString("Preference")
        genericFlexGridSizer.Add(self.uniformCrossoverPref, 0, wx.LEFT | wx.TOP | wx.RIGHT, 5)

        self.genericWidgets.append(self.uniformCrossover)
        self.AddChildToParent(self.uniformCrossover, self.uniformCrossoverPref)

        # ---------------- real crossovers ----------------
        realBox = wx.StaticBox(self, -1, "Real operations (affects only feature weighting)", size=(500,-1))
        realBoxSizer = wx.StaticBoxSizer(realBox, wx.VERTICAL)
        realBoxFlexGridSizer = wx.FlexGridSizer(0, 2, 5, 5)
        realBoxSizer.Add(realBoxFlexGridSizer, 0)
        sizer.Add(realBoxSizer, 0, wx.ALL | wx.EXPAND, 10)
        
        # simulated binary crossover
        self.sbxCrossover = wx.CheckBox(self, -1, "SBX Crossover", \
            name = "SBXcrossover")
        self.sbxCrossover.SetValue(True)
        self.sbxCrossover.Disable()
        realBoxFlexGridSizer.Add(self.sbxCrossover, 0, wx.LEFT | wx.TOP | wx.RIGHT | wx.EXPAND, 5)
        
        self.sbxCrossoverEta = FS.FloatSpin(self, -1, min_val=0.01, max_val=10.0, \
            increment=0.01, value=1.0)
        self.sbxCrossoverEta.SetFormat("%f")
        self.sbxCrossoverEta.SetDigits(2)
        self.sbxCrossoverEta.Disable()
        self.sbxCrossoverEta.SetToolTipString("eta the amount of exploration "\
            "OUTSIDE the parents in BLX-alpha notation")
        realBoxFlexGridSizer.Add(self.sbxCrossoverEta, 0, wx.LEFT | wx.TOP | wx.RIGHT, 5)

        self.weightingWidgets.append(self.sbxCrossover)
        self.AddChildToParent(self.sbxCrossover, self.sbxCrossoverEta)
        
        # segment crossover
        self.segmentCrossover = wx.CheckBox(self, -1, "Segment Crossover", \
            name = "segmentCrossover")
        self.segmentCrossover.Disable()
        realBoxFlexGridSizer.Add(self.segmentCrossover, 0, wx.LEFT | wx.TOP | wx.RIGHT | wx.EXPAND, 5)

        self.segmentCrossoverAlpha = FS.FloatSpin(self, -1, min_val=0.01, max_val=10.0, \
            increment=0.01, value=1.0)
        self.segmentCrossoverAlpha.SetFormat("%f")
        self.segmentCrossoverAlpha.SetDigits(2)
        self.segmentCrossoverAlpha.Disable()
        self.segmentCrossoverAlpha.SetToolTipString("alpha the amount of exploration "\
            "OUTSIDE the parents in BLX-alpha notation")
        realBoxFlexGridSizer.Add(self.segmentCrossoverAlpha, 0, wx.LEFT | wx.TOP | wx.RIGHT, 5)
        
        self.weightingWidgets.append(self.segmentCrossover)
        self.AddChildToParent(self.segmentCrossover, self.segmentCrossoverAlpha)
        
        # hypercube crossover
        self.hypercubeCrossover = wx.CheckBox(self, -1, "Hypercube Crossover", \
            name = "hypercubeCrossover")
        self.hypercubeCrossover.Disable()
        realBoxFlexGridSizer.Add(self.hypercubeCrossover, 0, wx.LEFT | wx.TOP | wx.RIGHT | wx.EXPAND, 5)
        
        self.hypercubeCrossoverAlpha = FS.FloatSpin(self, -1, min_val=0.01, max_val=10.0, \
            increment=0.01, value=1.0)
        self.hypercubeCrossoverAlpha.SetFormat("%f")
        self.hypercubeCrossoverAlpha.SetDigits(2)
        self.hypercubeCrossoverAlpha.Disable()
        self.hypercubeCrossoverAlpha.SetToolTipString("alpha the amount of exploration "\
            "OUTSIDE the parents in BLX-alpha notation")
        realBoxFlexGridSizer.Add(self.hypercubeCrossoverAlpha, 0, wx.LEFT | wx.TOP | wx.RIGHT, 5)
        
        self.weightingWidgets.append(self.hypercubeCrossover)
        self.AddChildToParent(self.hypercubeCrossover, self.hypercubeCrossoverAlpha)
        
        # bind the EVT_CHECKBOX to the CheckBoxes
        self.BindEvent(wx.EVT_CHECKBOX, self.OnCheckBox, \
            [self.nPointCrossover,
             self.uniformCrossover,
             self.sbxCrossover,
             self.segmentCrossover,
             self.hypercubeCrossover])
