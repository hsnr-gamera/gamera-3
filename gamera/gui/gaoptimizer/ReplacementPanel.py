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

from gamera.gui import compatibility
from gamera.gui.gaoptimizer.ExpertSettingPanel import *

#-------------------------------------------------------------------------------
class ReplacementPanel(ExpertSettingPanel):
#-------------------------------------------------------------------------------
    #---------------------------------------------------------------------------
    def __init__(self, parent, id):
    #---------------------------------------------------------------------------
        ExpertSettingPanel.__init__(self, parent, id)

        sizer = wx.GridBagSizer(hgap=5, vgap=5)
        self.SetSizer(sizer)

        # generational replacement
        self.genReplacement = wx.RadioButton(self, -1, "Generational Replacement", \
            name = "generationalReplacement")
        sizer.Add(self.genReplacement, pos=(0,0), \
            flag = wx.LEFT | wx.RIGHT | wx.TOP | wx.EXPAND, border=10)

        self.genericWidgets.append(self.genReplacement)

        # SSGA - replace worst
        self.ssgaWorse = wx.RadioButton(self, -1, "SSGA Worse Replacement", \
            name = "SSGAworse")
        sizer.Add(self.ssgaWorse, pos=(1,0), \
            flag = wx.LEFT | wx.RIGHT | wx.EXPAND, border=10)

        self.genericWidgets.append(self.ssgaWorse)

        # SSGA deterministic tournament
        self.ssgaDetTour = wx.RadioButton(self, -1, "SSGA det. tournament", \
            name = "SSGAdetTournament")
        self.ssgaDetTour.SetValue(True)
        sizer.Add(self.ssgaDetTour, pos=(2,0), \
            flag = wx.LEFT | wx.RIGHT | wx.EXPAND, border=10)
        self.ssgaDetTourTsize = wx.SpinCtrl(self, -1, size=(100,-1), \
            min=2, max=10, value='3') # TODO: max = popSize
        compatibility.set_tool_tip(self.ssgaDetTourTsize, "Tournament size")
        sizer.Add(self.ssgaDetTourTsize, pos=(2,1), \
            flag = wx.LEFT | wx.RIGHT | wx.EXPAND, border=10)

        self.genericWidgets.append(self.ssgaDetTour)
        self.AddChildToParent(self.ssgaDetTour, self.ssgaDetTourTsize)

        # bind the EVT_RADIOBUTTON to the RadioButtons
        self.BindEvent(wx.EVT_RADIOBUTTON, self.OnRadioButton, \
            [self.genReplacement, 
             self.ssgaWorse,
             self.ssgaDetTour])
