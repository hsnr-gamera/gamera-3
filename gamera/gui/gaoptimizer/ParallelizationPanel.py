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

from gamera.gui import compat_wx
from gamera.gui.gaoptimizer.ExpertSettingPanel import *

#-------------------------------------------------------------------------------
class ParallelizationPanel(ExpertSettingPanel):
#-------------------------------------------------------------------------------
    #---------------------------------------------------------------------------
    def __init__(self, parent, id):
    #---------------------------------------------------------------------------
        ExpertSettingPanel.__init__(self, parent, id)

        # sizer for widget setting
        sizer = wx.FlexGridSizer(0, 2, 5, 5)
        sizer.AddGrowableCol(1)
        self.SetSizer(sizer)

        # enable or disable the parallelization
        text = wx.StaticText(self, -1, "Status:")
        sizer.Add(text, 0, wx.ALIGN_BOTTOM | wx.LEFT | wx.RIGHT | wx.TOP, 10)
        self.parallelEnabled = wx.CheckBox(self, -1, "Enable Parallelization", \
            name = "parallelization")
        self.parallelEnabled.SetValue(True)
        compat_wx.set_tool_tip(self.parallelEnabled, "Only available if compiled with OpenMP")
        sizer.Add(self.parallelEnabled, 0, wx.LEFT | wx.RIGHT | wx.TOP | wx.EXPAND, 10)

        # if enabled choose the number of used threads
        threadText = wx.StaticText(self, -1, "Number of Threads:")
        sizer.Add(threadText, 0, wx.ALIGN_BOTTOM | wx.LEFT | wx.RIGHT | wx.TOP, 10)

        try:
            # check available hardware
            import multiprocessing
            cpu_count = multiprocessing.cpu_count()
        except Exception:
            # when no multiprocessing module is available (python < 2.6)
            # assume 2 cores
            cpu_count = 2

        # by default use all instead of one core
        cores_to_use = cpu_count - 1
        if cores_to_use < 1:
            cores_to_use = 1

        self.threadNum = wx.Slider(self, -1, cores_to_use, 1, cpu_count, style= wx.SL_LABELS)

        sizer.Add(self.threadNum, 0, wx.LEFT | wx.RIGHT | wx.TOP | wx.EXPAND, 10)

        # save the logical dependencies
        self.genericWidgets.append(self.parallelEnabled)
        #self.AddChildToParent(self.parallelEnabled, threadText)
        self.AddChildToParent(self.parallelEnabled, self.threadNum)

        # bind the EVT_CHECKBOX to the CheckBox
        self.BindEvent(wx.EVT_CHECKBOX, self.OnCheckBox, [self.parallelEnabled])
