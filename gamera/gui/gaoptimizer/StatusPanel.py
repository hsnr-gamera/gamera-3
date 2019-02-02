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
import time

#-------------------------------------------------------------------------------
class StatusPanel(wx.ScrolledWindow):
#-------------------------------------------------------------------------------
    #---------------------------------------------------------------------------
    def __init__(self, parent, id, frame):
    #---------------------------------------------------------------------------
        wx.ScrolledWindow.__init__(self, parent, id)

        self.frame = frame
        self.starttime = None

        topSizer = wx.BoxSizer(wx.VERTICAL)

        # status box
        statusBox = wx.StaticBox(self, -1, "Status GA optimization")
        statusBoxSizer = wx.StaticBoxSizer(statusBox, wx.HORIZONTAL)
        statusFlexGridSizer = wx.FlexGridSizer(0, 2, 5, 15)

        # status indicator
        text = wx.StaticText(self, -1, "Current Status:")
        statusFlexGridSizer.Add(text, 0, wx.LEFT | wx.TOP | wx.RIGHT, 10)
        self.status = wx.StaticText(self, -1, "not running")
        statusFlexGridSizer.Add(self.status, 0, wx.LEFT | wx.TOP | wx.RIGHT, 10)

        # status indicator
        text = wx.StaticText(self, -1, "Initial leave one out rate:")
        statusFlexGridSizer.Add(text, 0, wx.LEFT | wx.TOP | wx.RIGHT, 10)
        self.initLOO = wx.StaticText(self, -1, "unknown")
        statusFlexGridSizer.Add(self.initLOO, 0, wx.LEFT | wx.TOP | wx.RIGHT, 10)

        # time counter
        text = wx.StaticText(self, -1, "Elapsed time:")
        statusFlexGridSizer.Add(text, 0, wx.LEFT | wx.TOP | wx.RIGHT, 10)
        self.time = wx.StaticText(self, -1, "00 h 00 m 00 s")
        statusFlexGridSizer.Add(self.time, 0, wx.LEFT | wx.TOP | wx.RIGHT, 10)

        # generation counter
        text = wx.StaticText(self, -1, "Current generation:")
        statusFlexGridSizer.Add(text, 0, wx.LEFT | wx.TOP | wx.RIGHT, 10)
        self.curGen = wx.StaticText(self, -1, "unknown")
        statusFlexGridSizer.Add(self.curGen, 0, wx.LEFT | wx.TOP | wx.RIGHT, 10)

        # current rate
        text = wx.StaticText(self, -1, "Best leave one out rate:")
        statusFlexGridSizer.Add(text, 0, wx.LEFT | wx.TOP | wx.RIGHT, 10)
        self.curLOO = wx.StaticText(self, -1, "unknown")
        statusFlexGridSizer.Add(self.curLOO, 0, wx.LEFT | wx.TOP | wx.RIGHT, 10)

        statusBoxSizer.Add(statusFlexGridSizer, 1, wx.ALL | wx.EXPAND, 10)

        # stop button
        self.stopButton = wx.Button(self, -1, "Stop")
        self.stopButton.Disable()
        self.Bind(wx.EVT_BUTTON, self.frame.settingsPanel.OnButton, self.stopButton)
        statusBoxSizer.Add(self.stopButton, 1, wx.ALL | wx.CENTER, 10)

        # statistics box
        statsBox = wx.StaticBox(self, -1, "GA statistics")
        statsBoxSizer = wx.StaticBoxSizer(statsBox, wx.VERTICAL)

        self.statistics = wx.TextCtrl(self, -1, "", style=wx.TE_MULTILINE | wx.TE_READONLY)
        statsBoxSizer.Add(self.statistics, 1, wx.ALL | wx.EXPAND, 10)


        topSizer.Add(statusBoxSizer, 0, wx.LEFT | wx.TOP | wx.RIGHT | wx.EXPAND, 25)
        topSizer.Add(statsBoxSizer, 1, wx.ALL | wx.EXPAND, 25)
        self.SetSizer(topSizer)

        width, height = topSizer.GetMinSize()
        self.SetScrollbars(10, 10, width/10, height/10)

    #---------------------------------------------------------------------------
    def Update(self):
    #---------------------------------------------------------------------------
        # init rate
        if self.frame.settingsPanel.initLOO == None:
            self.initLOO.SetLabel("unknown")
        else:
            rate = self.frame.settingsPanel.initLOO
            rate = (rate[0] / float(rate[1])) * 100
            self.initLOO.SetLabel("%.2lf %%" % rate)

        # consumed time
        if self.starttime != None:
            now = time.time()
            timediff = now - self.starttime

            hours = int(timediff / 3600.0)
            minutes = int((timediff % 3600) / 60.0)
            secs = int((timediff % 60))

            self.time.SetLabel("%d h %d m %d s" % (hours, minutes, secs))

        # status, generation count, current rate and monitor
        if self.frame.settingsPanel.workerThread == None:
            self.status.SetLabel("not running")
        else:
            if self.frame.settingsPanel.workerThread.isAlive():
                self.status.SetLabel("running")
            else:
                self.status.SetLabel("not running")

            self.curGen.SetLabel("%d" % self.frame.settingsPanel.workerThread.GAOptimizer.generation)
            self.curLOO.SetLabel("%.2lf %%" % (self.frame.settingsPanel.workerThread.GAOptimizer.bestFitness * 100))

            wx.CallAfter(self.statistics.SetValue, self.frame.settingsPanel.workerThread.GAOptimizer.monitorString)
            wx.CallAfter(self.statistics.ShowPosition, self.statistics.GetLastPosition())
