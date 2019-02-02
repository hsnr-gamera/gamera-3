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
class SelectionPanel(ExpertSettingPanel):
#-------------------------------------------------------------------------------
    #---------------------------------------------------------------------------
    def __init__(self, parent, id):
    #---------------------------------------------------------------------------
        ExpertSettingPanel.__init__(self, parent, id)

        sizer = wx.GridBagSizer(hgap=5, vgap=5)
        self.SetSizer(sizer)
        
        # roulette wheel selection
        self.roulettWheel = wx.RadioButton(self, -1, "Roulette wheel", \
            name="rouletteWheel")
        sizer.Add(self.roulettWheel, pos=(0,0), \
            flag = wx.LEFT | wx.RIGHT | wx.TOP | wx.EXPAND, border=10)

        self.genericWidgets.append(self.roulettWheel)

        # linear scaled fitness
        self.roulettWheelScaled = wx.RadioButton(self, -1, "Roulette wheel (lin. scaled)", \
            name="rouletteWheelScaled")
        self.roulettWheelScaled.SetValue(True)
        sizer.Add(self.roulettWheelScaled, pos=(1,0), \
            flag = wx.LEFT | wx.RIGHT | wx.EXPAND, border=10)

        self.roulettWheelPreasure = FS.FloatSpin(self, -1, min_val=0.0, max_val=2.0, \
            increment=0.01, value=2.0)
        self.roulettWheelPreasure.SetFormat("%f")
        self.roulettWheelPreasure.SetDigits(2)
        compat_wx.set_tool_tip(self.roulettWheelPreasure, "The selective pressure")
        sizer.Add(self.roulettWheelPreasure, pos=(1,1), \
            flag = wx.LEFT | wx.RIGHT | wx.EXPAND, border=10)

        self.genericWidgets.append(self.roulettWheelScaled)
        self.AddChildToParent(self.roulettWheelScaled, self.roulettWheelPreasure)

        # stochastic universal sampling
        self.stochUniSampling = wx.RadioButton(self, -1, "Stochastic universal sampling", \
            name="stochUniSampling")
        sizer.Add(self.stochUniSampling, pos=(2,0), \
            flag = wx.LEFT | wx.RIGHT | wx.EXPAND, border=10)

        self.genericWidgets.append(self.stochUniSampling)

        # rank selection
        self.rankSelection = wx.RadioButton(self, -1, "Rank selection", name="rankSelection")
        sizer.Add(self.rankSelection, pos=(3,0), \
            flag = wx.LEFT | wx.RIGHT | wx.EXPAND, border=10)
            
        self.rankSelectionPreasure = FS.FloatSpin(self, -1, min_val=0.0, max_val=2.0, \
            increment=0.01, value=2.0)
        self.rankSelectionPreasure.SetFormat("%f")
        self.rankSelectionPreasure.SetDigits(2)
        self.rankSelectionPreasure.Disable()
        compat_wx.set_tool_tip(self.rankSelectionPreasure, "The selective pressure")
        sizer.Add(self.rankSelectionPreasure, pos=(3,1), \
            flag = wx.LEFT | wx.RIGHT | wx.EXPAND, border=10)
            
        self.rankSelectionExponent = FS.FloatSpin(self, -1, min_val=0.0, max_val=2.0, \
            increment=0.01, value=1.0)
        self.rankSelectionExponent.SetFormat("%f")
        self.rankSelectionExponent.SetDigits(2)
        self.rankSelectionExponent.Disable()
        compat_wx.set_tool_tip(self.rankSelectionExponent, "Exponent")
        sizer.Add(self.rankSelectionExponent, pos=(3,2), \
            flag = wx.LEFT | wx.RIGHT | wx.EXPAND, border=10)

        self.genericWidgets.append(self.rankSelection)
        self.AddChildToParent(self.rankSelection, self.rankSelectionPreasure)
        self.AddChildToParent(self.rankSelection, self.rankSelectionExponent)
            
        # tournament selection
        self.TournamentSelection = wx.RadioButton(self, -1, "Tournament selection", \
            name="tournamentSelection")
        sizer.Add(self.TournamentSelection, pos=(4,0), \
            flag = wx.LEFT | wx.RIGHT | wx.EXPAND, border=10)
            
        self.TournamentSelectionTsize = wx.SpinCtrl(self, -1, size=(100,-1), \
            min=2, max=25, value='3') # TODO: max = popSize
        self.TournamentSelectionTsize.Disable()
        compat_wx.set_tool_tip(self.TournamentSelectionTsize, "Tournament size")
        sizer.Add(self.TournamentSelectionTsize, pos=(4,1), \
            flag = wx.LEFT | wx.RIGHT | wx.EXPAND, border=10)

        self.genericWidgets.append(self.TournamentSelection)
        self.AddChildToParent(self.TournamentSelection, self.TournamentSelectionTsize)

        # random selection
        self.randomSelect = wx.RadioButton(self, -1, "Random selection", \
            name="randomSelection")
        sizer.Add(self.randomSelect, pos=(5,0), \
            flag = wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border=10)

        self.genericWidgets.append(self.randomSelect)
            
        # bind the EVT_RADIOBUTTON to the RadioButtons
        self.BindEvent(wx.EVT_RADIOBUTTON, self.OnRadioButton, \
            [self.roulettWheel,
             self.roulettWheelScaled,
             self.stochUniSampling,
             self.rankSelection,
             self.TournamentSelection,
             self.randomSelect])
