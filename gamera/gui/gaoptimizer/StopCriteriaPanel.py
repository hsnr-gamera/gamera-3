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

from gamera.gui import compatibility
from gamera.gui.gaoptimizer.ExpertSettingPanel import *

#-------------------------------------------------------------------------------
class StopCriteriaPanel(ExpertSettingPanel):
#-------------------------------------------------------------------------------
    #---------------------------------------------------------------------------
    def __init__(self, parent, id):
    #---------------------------------------------------------------------------
        ExpertSettingPanel.__init__(self, parent, id)

        sizer = wx.GridBagSizer(hgap=5, vgap=5)
        self.SetSizer(sizer)

        # best fitness
        self.bestFitness = wx.CheckBox(self, -1, "Perfect LOO-recognition reached", \
            name = "bestFitnessStop")
        sizer.Add(self.bestFitness, pos=(0,0), \
            flag = wx.LEFT | wx.RIGHT | wx.TOP | wx.EXPAND, border=10)

        self.genericWidgets.append(self.bestFitness)

        # generation counter
        self.maxGeneration = wx.CheckBox(self, -1, "Max. number of generations", \
            name = "maxGenerations")
        sizer.Add(self.maxGeneration, pos=(1,0), \
            flag = wx.LEFT | wx.RIGHT | wx.EXPAND, border = 10)
        self.maxGenerationCount = wx.SpinCtrl(self, -1, size=(100,-1), \
            min=10, max=5000, value='100')
        compatibility.set_tool_tip(self.maxGenerationCount, "Number of generations")
        self.maxGenerationCount.Disable()
        sizer.Add(self.maxGenerationCount, pos=(1,1), \
            flag = wx.LEFT | wx.RIGHT | wx.EXPAND, border=10)

        self.genericWidgets.append(self.maxGeneration)
        self.AddChildToParent(self.maxGeneration, self.maxGenerationCount)

        # fitness counter
        self.maxFitnessEval = wx.CheckBox(self, -1, "Max. number of fitness evals", \
            name = "maxFitnessEvals")
        sizer.Add(self.maxFitnessEval, pos=(2,0), \
            flag = wx.LEFT | wx.RIGHT | wx.EXPAND, border=10)
        self.maxFitnessEvalCount = wx.SpinCtrl(self, -1, size=(100,-1), \
            min=10, max=50000, value='5000')
        compatibility.set_tool_tip(self.maxFitnessEvalCount, "Number of evaluations")
        self.maxFitnessEvalCount.Disable()
        sizer.Add(self.maxFitnessEvalCount, pos=(2,1), \
            flag = wx.LEFT | wx.RIGHT | wx.EXPAND, border=10)

        self.genericWidgets.append(self.maxFitnessEval)
        self.AddChildToParent(self.maxFitnessEval, self.maxFitnessEvalCount)

        # steady state continue
        self.steadyContinue = wx.CheckBox(self, -1, "Steady state continue", \
            name = "steadyStateStop")
        self.steadyContinue.SetValue(True)
        sizer.Add(self.steadyContinue, pos=(3,0), \
            flag = wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border=10)
        self.steadyContinueMin = wx.SpinCtrl(self, -1, size=(100,-1), \
            min=10, max=250000, value='40')
        compatibility.set_tool_tip(self.steadyContinueMin, "Minimum generations")
        sizer.Add(self.steadyContinueMin, pos=(3,1), \
            flag = wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border=10)
        self.steadyContinueNoChange = wx.SpinCtrl(self, -1, size=(100,-1), \
            min=1, max=10000, value='10')
        compatibility.set_tool_tip(self.steadyContinueNoChange, "Generations without improvement")
        sizer.Add(self.steadyContinueNoChange, pos=(3,2), \
            flag = wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border=10)

        self.genericWidgets.append(self.steadyContinue)
        self.AddChildToParent(self.steadyContinue, self.steadyContinueMin)
        self.AddChildToParent(self.steadyContinue, self.steadyContinueNoChange)

        # bind the EVT_CHECKBOX to the CheckBoxes
        self.BindEvent(wx.EVT_CHECKBOX, self.OnCheckBox, \
            [self.bestFitness, self.maxGeneration,
             self.maxFitnessEval, 
             self.steadyContinue])
