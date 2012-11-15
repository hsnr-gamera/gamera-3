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

from gamera import knnga

#-------------------------------------------------------------------------------
class ResultPanel(wx.ScrolledWindow):
#-------------------------------------------------------------------------------
    #---------------------------------------------------------------------------
    def __init__(self, parent, id, frame):
    #---------------------------------------------------------------------------
        wx.ScrolledWindow.__init__(self, parent, -1)

        self.frame = frame
        self.bars = None

    #---------------------------------------------------------------------------
    def initResultsWidgets(self):
    #---------------------------------------------------------------------------
        if self.frame.classifier == None:
            raise ValueError("ResultPanel.initResultsWidgets: no valid classifier given")

        for child in self.GetChildren():
            child.Destroy()

        features = self.frame.classifier.feature_functions
        feature_names = [x[0] for x in features[0]]
        feature_functions = [x[1] for x in features[0]]

        if self.frame.settingsPanel.featureSelection.GetValue():
            values = self.frame.classifier.get_selections_by_features()
        elif self.frame.settingsPanel.featureWeighting.GetValue():
            values = self.frame.classifier.get_weights_by_features()
        else:
            raise ValueError("ResultPanel.initResultsWidgets: unknown mode of operation")

        gridSizer = wx.FlexGridSizer(0, 2, 5, 15)
        gridSizer.AddGrowableCol(1)

        flags = (wx.TOP|wx.LEFT|wx.RIGHT|wx.EXPAND|wx.ALIGN_CENTER_VERTICAL)

        text = wx.StaticText(self, -1, "Feature Name")
        gridSizer.Add(text, 0, flags, 10)
        text = wx.StaticText(self, -1, "Value")
        gridSizer.Add(text, 0, flags, 10)

        for (name, ff) in zip(feature_names, feature_functions):
            self.bars[name] = []
            for x in range(ff.return_type.length):

                feature_name = wx.StaticText(self, -1, name + " - " + str(x))
                gridSizer.Add(feature_name, 0, flags, 10)

                bar = wx.Gauge(self, -1, 100, style=wx.GA_SMOOTH, size=(-1,10))
                bar.SetValue(values[name][x]*100)
                gridSizer.Add(bar, 0, flags, 10)

                self.bars[name].append(bar)

        self.SetSizer(gridSizer)

        width, height = gridSizer.GetMinSize()
        self.SetScrollbars(10, 10, width/10, height/10)

        self.Layout()

    #---------------------------------------------------------------------------
    def Update(self):
    #---------------------------------------------------------------------------
        wx.CallAfter(self.UpdateResultWidgets)

    #---------------------------------------------------------------------------
    def UpdateResultWidgets(self):
    #---------------------------------------------------------------------------
        if self.frame.classifier == None:
            return

        if self.bars == None:
            self.bars = {}
            self.initResultsWidgets()

        features = self.frame.classifier.feature_functions
        feature_names = [x[0] for x in features[0]]
        feature_functions = [x[1] for x in features[0]]

        if self.frame.settingsPanel.featureSelection.GetValue():
            values = self.frame.classifier.get_selections_by_features()
        elif self.frame.settingsPanel.featureWeighting.GetValue():
            values = self.frame.classifier.get_weights_by_features()
        else:
            raise ValueError("ResultPanel.UpdateResultWidgets: unknown mode of operation")

        if len([x for x in self.bars.keys() if x not in feature_names]) != 0:
            self.initResultsWidgets()
            return

        if len([x for x in feature_names if x not in self.bars.keys()]) != 0:
            self.initResultsWidgets()
            return

        for (name, ff) in zip(feature_names, feature_functions):
            for x in range(ff.return_type.length):
                self.bars[name][x].SetValue(values[name][x]*100)
