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
import threading

try:
    from agw import pycollapsiblepane as PCP
except ImportError:
    try:
        import wx.lib.agw.pycollapsiblepane as PCP
    except ImportError:
        raise RuntimeError("Biollante requires at least wxPython 2.8.11")
        

from gamera.gui.gaoptimizer.ExpertSettingPanel import *
from gamera.gui.gaoptimizer.SelectionPanel import *
from gamera.gui.gaoptimizer.CrossoverPanel import *
from gamera.gui.gaoptimizer.MutationPanel import *
from gamera.gui.gaoptimizer.ReplacementPanel import *
from gamera.gui.gaoptimizer.StopCriteriaPanel import *
from gamera.gui.gaoptimizer.ParallelizationPanel import *

from gamera.gui import gui_util
from gamera import knnga

#-------------------------------------------------------------------------------
class GAWorker(threading.Thread):
#-------------------------------------------------------------------------------
    #---------------------------------------------------------------------------
    def __init__(self, GAOptimizer, frame):
    #---------------------------------------------------------------------------
        threading.Thread.__init__(self)
        self.GAOptimizer = GAOptimizer
        self.frame = frame

    #---------------------------------------------------------------------------
    def run(self):
    #---------------------------------------------------------------------------
        self.GAOptimizer.startCalculation()
        self.frame.timer.Stop()

        wx.CallAfter(self.frame.UpdatePanels)
        self.frame.settingsPanel.stopButton.Disable()
        self.frame.statusPanel.stopButton.Disable()

#-------------------------------------------------------------------------------
class GAInitialLOOWorker(threading.Thread):
#-------------------------------------------------------------------------------
    #---------------------------------------------------------------------------
    def __init__(self, classifier, frame):
    #---------------------------------------------------------------------------
        threading.Thread.__init__(self)
        self.classifier = classifier
        self.frame = frame

    #---------------------------------------------------------------------------
    def run(self):
    #---------------------------------------------------------------------------
        initLOO = self.classifier.leave_one_out()
        self.frame.settingsPanel.initLOO = initLOO

#-------------------------------------------------------------------------------
class SettingsPanel(wx.ScrolledWindow):
#-------------------------------------------------------------------------------
    #---------------------------------------------------------------------------
    def __init__(self, parent, id, frame):
    #---------------------------------------------------------------------------
        wx.ScrolledWindow.__init__(self, parent, id)

        self.frame = frame
        self.workerThread = None
        self.initLOO = None
        
        topSizer = wx.BoxSizer(wx.VERTICAL)
        
        # mode box
        modeBox = wx.StaticBox(self, -1, "Mode of operation")
        modeBoxSizer = wx.StaticBoxSizer(modeBox, wx.HORIZONTAL)
        modeBoxGridSizer = wx.GridBagSizer(hgap=5, vgap=5)
        modeBoxSizer.Add(modeBoxGridSizer, 1, wx.EXPAND)

        self.selectingLabel = "Feature selection"
        self.weightingLabel = "Feature weighting"

        self.featureSelection = wx.RadioButton(self, -1, self.selectingLabel)
        self.featureSelection.SetValue(True)
        self.featureWeighting = wx.RadioButton(self, -1, self.weightingLabel)
        self.normalization = wx.CheckBox(self, -1, "Normalization")

        modeBoxGridSizer.Add(self.featureSelection, pos=(0,0), \
            flag=wx.LEFT | wx.RIGHT | wx.TOP | wx.EXPAND, border=10)
        modeBoxGridSizer.Add(self.normalization, pos=(0,1), \
            flag=wx.LEFT | wx.RIGHT | wx.TOP | wx.EXPAND, border=10)
        modeBoxGridSizer.Add(self.featureWeighting, pos=(1,0), \
            flag=wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border=10)

        self.Bind(wx.EVT_RADIOBUTTON, self.OnOpModeChange, self.featureSelection)
        self.Bind(wx.EVT_RADIOBUTTON, self.OnOpModeChange, self.featureWeighting)

        # basic settings box
        settingsBox = wx.StaticBox(self, -1, "Basic GA settings")
        settingsBoxSizer = wx.StaticBoxSizer(settingsBox, wx.VERTICAL)
        settingsBoxFlexSizer = wx.FlexGridSizer(0, 2, 5, 15)
        settingsBoxFlexSizer.AddGrowableCol(1)

        text = wx.StaticText(self, -1, "Population size:")
        settingsBoxFlexSizer.Add(text, 0, wx.ALIGN_CENTER_VERTICAL |wx.LEFT | wx.RIGHT | wx.TOP, 0)
        self.popSize = wx.SpinCtrl(self, -1, size=(100,-1), min=2, max=1000, value='75')
        settingsBoxFlexSizer.Add(self.popSize, 0, wx.ALIGN_LEFT | wx.LEFT | wx.RIGHT | wx.TOP, 0)

        text = wx.StaticText(self, -1, "Crossover rate:")
        settingsBoxFlexSizer.Add(text, 0, wx.ALIGN_BOTTOM | wx.LEFT | wx.RIGHT | wx.TOP, 0)
        self.crossoverRate = wx.Slider(self, -1, 95, 0, 100, style= wx.SL_LABELS)
        settingsBoxFlexSizer.Add(self.crossoverRate, 0,  wx.LEFT | wx.RIGHT | wx.TOP | wx.EXPAND, 0)

        text = wx.StaticText(self, -1, "Mutation rate:")
        settingsBoxFlexSizer.Add(text, 0, wx.ALIGN_BOTTOM | wx.LEFT | wx.RIGHT | wx.TOP, 0)
        self.mutationRate = wx.Slider(self, -1, 5, 0, 100, style= wx.SL_LABELS)
        settingsBoxFlexSizer.Add(self.mutationRate, 0,  wx.LEFT | wx.RIGHT | wx.TOP | wx.EXPAND, 0)

        settingsBoxSizer.Add(settingsBoxFlexSizer, 0, wx.ALL | wx.EXPAND, 10)

        # expert settings box
        expertPane = PCP.PyCollapsiblePane(self, -1, "Expert GA settings", \
            agwStyle=wx.CP_USE_STATICBOX)
        self.Bind(wx.EVT_COLLAPSIBLEPANE_CHANGED, self.OnPaneChanged, expertPane)
        self.MakeExpertPane(expertPane.GetPane())

        # buttons
        buttonSizer = wx.BoxSizer(wx.HORIZONTAL)

        self.startButton = wx.Button(self, -1, "Start")
        self.startButton.Disable()
        buttonSizer.Add(self.startButton, 0, wx.ALL | wx.EXPAND, 10)
        self.Bind(wx.EVT_BUTTON, self.OnButton, self.startButton)

        self.stopButton = wx.Button(self, -1, "Stop")
        self.stopButton.Disable()
        buttonSizer.Add(self.stopButton, 0, wx.ALL | wx.EXPAND, 10)
        self.Bind(wx.EVT_BUTTON, self.OnButton, self.stopButton)


        topSizer.Add(modeBoxSizer, 0, wx.LEFT | wx.TOP | wx.RIGHT | wx.EXPAND, 25)
        topSizer.Add(settingsBoxSizer, 0, wx.LEFT | wx.TOP | wx.RIGHT | wx.EXPAND, 25)
        topSizer.Add(expertPane, 0, wx.LEFT | wx.TOP | wx.RIGHT | wx.EXPAND, 25)
        topSizer.Add(buttonSizer, 0, wx.ALL | wx.CENTER, 25)
        self.SetSizer(topSizer)

        width, height = topSizer.GetMinSize()
        self.SetScrollbars(10, 10, width/10, height/10)

    #---------------------------------------------------------------------------
    def MakeExpertPane(self, pane):
    #---------------------------------------------------------------------------
        sizer = wx.BoxSizer(wx.VERTICAL)

        notebook = wx.Notebook(pane, -1)

        self.selectionPanel = SelectionPanel(notebook, -1)
        self.crossoverPanel = CrossoverPanel(notebook, -1)
        self.mutationPanel = MutationPanel(notebook, -1)
        self.replacementPanel = ReplacementPanel(notebook, -1)
        self.stopCriteriaPanel = StopCriteriaPanel(notebook, -1)
        self.parallelizationPanel = ParallelizationPanel(notebook, -1)

        notebook.AddPage(self.selectionPanel, "Selection")
        notebook.AddPage(self.crossoverPanel, "Crossover")
        notebook.AddPage(self.mutationPanel, "Mutation")
        notebook.AddPage(self.replacementPanel, "Replacement")
        notebook.AddPage(self.stopCriteriaPanel, "Stop Criteria")
        from gamera.__compiletime_config__ import has_openmp
        if has_openmp:
            notebook.AddPage(self.parallelizationPanel, "Parallelization")

        sizer.Add(notebook, 1, wx.ALL | wx.EXPAND, 0)
        pane.SetSizer(sizer)

    #---------------------------------------------------------------------------
    def OnPaneChanged(self, event):
    #---------------------------------------------------------------------------
        # redo the layout ... necessary!
        self.Layout()
        
    #---------------------------------------------------------------------------
    def OnOpModeChange(self, event):
    #---------------------------------------------------------------------------
        opMode = event.GetEventObject().GetLabel()

        # crossover settings for selection and weighting
        # TODO: implement a more generic way to set the 'defaults'
        if opMode == self.selectingLabel:
            self.crossoverPanel.uniformCrossover.SetValue(True)
        if opMode == self.weightingLabel:
            for widget in self.crossoverPanel.genericWidgets:
                widget.SetValue(False)
                if widget in self.crossoverPanel.childWidgets:
                    for child in self.crossoverPanel.childWidgets[widget]:
                        child.Disable()


        panels = [self.crossoverPanel, self.mutationPanel]

        if opMode == self.selectingLabel:
            for panel in panels:
                for selectionWidget in panel.selectionWidgets:
                    self.TriggerWidgetsInPanel(panel, selectionWidget, True)
                for weightingWidget in panel.weightingWidgets:
                    self.TriggerWidgetsInPanel(panel, weightingWidget, False)
        elif opMode == self.weightingLabel:
            for panel in panels:
                for selectionWidget in panel.selectionWidgets:
                    self.TriggerWidgetsInPanel(panel, selectionWidget, False)
                for weightingWidget in panel.weightingWidgets:
                    self.TriggerWidgetsInPanel(panel, weightingWidget, True)

    #---------------------------------------------------------------------------
    def TriggerWidgetsInPanel(self, panel, widget, state):
    #---------------------------------------------------------------------------
        widget.Enable(state)
        if widget.GetValue():
            for child in panel.childWidgets[widget]:
                child.Enable(state)

    #---------------------------------------------------------------------------
    def OnButton(self, event):
    #---------------------------------------------------------------------------
        eventObject = event.GetEventObject()
        eventObjectLabel = eventObject.GetLabel()

        if eventObjectLabel == "Start":
            self.startCalculation()
        elif eventObjectLabel == "Stop":
            self.stopCalculation()

    #---------------------------------------------------------------------------
    def startCalculation(self):
    #---------------------------------------------------------------------------

        if self.frame.classifier == None:
            gui_util.message("No classifier loaded")
            return

        # Base GA settings
        base = knnga.GABaseSetting()

        if self.featureSelection.GetValue():
            base.opMode = knnga.GA_SELECTION
        elif self.featureWeighting.GetValue():
            base.opMode = knnga.GA_WEIGHTING

        # Set the normalization state in the used classifier
        if self.classifier.get_normalization_state() != self.normalization.GetValue():
            self.classifier.set_normalization_state(self.normalization.GetValue())

        base.popSize = self.popSize.GetValue()
        base.crossRate = self.crossoverRate.GetValue() / 100.0
        base.mutRate = self.mutationRate.GetValue() / 100.0

        # calculate initial leave one out rate for status display
        self.initLOO = None
        initLOOThread = GAInitialLOOWorker(self.frame.classifier, self.frame)
        initLOOThread.setDaemon(1)
        initLOOThread.start()

        # Selection GA settings
        selection = knnga.GASelection()

        for expertWidget in self.selectionPanel.GetAllSettingWidgets():
            if expertWidget.IsEnabled() and expertWidget.GetValue():
                if expertWidget.GetName() == "rouletteWheel":
                    selection.setRoulettWheel()
                elif expertWidget.GetName() == "rouletteWheelScaled":
                    selection.setRoulettWheelScaled(\
                        self.selectionPanel.roulettWheelPreasure.GetValue())
                elif expertWidget.GetName() == "stochUniSampling":
                    selection.setStochUniSampling()
                elif expertWidget.GetName() == "rankSelection":
                    selection.setRankSelection(\
                        self.selectionPanel.rankSelectionPreasure.GetValue(),\
                        self.selectionPanel.rankSelectionExponent.GetValue())
                elif expertWidget.GetName() == "tournamentSelection":
                    selection.setTournamentSelection(\
                        self.selectionPanel.TournamentSelectionTsize.GetValue())
                elif expertWidget.GetName() == "randomSelection":
                    selection.setRandomSelection()
                else:
                    raise RuntimeError("Unknown selection method choosen")

        # Crossover GA settings
        cross = knnga.GACrossover()

        crossOpSet = False
        for expertWidget in self.crossoverPanel.GetAllSettingWidgets():
            if expertWidget.IsEnabled() and expertWidget.GetValue():
                if expertWidget.GetName() == "nPointCrossover":
                    cross.setNPointCrossover(self.crossoverPanel.nPointCrossoverN.GetValue())
                elif expertWidget.GetName() == "uniformCrossover":
                    cross.setUniformCrossover(self.crossoverPanel.uniformCrossoverPref.GetValue())
                elif expertWidget.GetName() == "SBXcrossover":
                    cross.setSBXcrossover(self.frame.classifier.num_features, 0.0, 1.0, \
                        self.crossoverPanel.sbxCrossoverEta.GetValue())
                elif expertWidget.GetName() == "segmentCrossover":
                    cross.setSegmentCrossover(self.frame.classifier.num_features, 0.0, 1.0, \
                        self.crossoverPanel.segmentCrossoverAlpha.GetValue())
                elif expertWidget.GetName() == "hypercubeCrossover":
                    cross.setHypercubeCrossover(self.frame.classifier.num_features, 0.0, 1.0, \
                        self.crossoverPanel.hypercubeCrossoverAlpha.GetValue())
                else:
                    raise RuntimeError("Unknown crossover method choosen")
                crossOpSet = True
        if not crossOpSet:
            raise RuntimeError("At least one crossover operator must be choosen")

        # Mutation GA settings
        muta = knnga.GAMutation()

        mutOpSet = False
        for expertWidget in self.mutationPanel.GetAllSettingWidgets():
            if expertWidget.IsEnabled() and expertWidget.GetValue():
                if expertWidget.GetName() == "shiftMutation":
                    muta.setShiftMutation()
                elif expertWidget.GetName() == "swapMuation":
                    muta.setSwapMutation()
                elif expertWidget.GetName() == "inversionMutation":
                    muta.setInversionMutation()
                elif expertWidget.GetName() == "binaryMutation":
                    muta.setBinaryMutation(self.mutationPanel.binaryMutationRate.GetValue(), False)
                elif expertWidget.GetName() == "gaussMutation":
                    muta.setGaussMutation(self.frame.classifier.num_features, 0.0, 1.0, \
                        self.mutationPanel.gaussMutationSigma.GetValue(), \
                        self.mutationPanel.gaussMutationPchance.GetValue())
                else:
                    raise RuntimeError("Unknown mutation method choosen")
                mutOpSet = True
        if not mutOpSet:
            raise RuntimeError("At least one mutation operator must be choosen")

        # Replacement GA settings
        replacement = knnga.GAReplacement()

        for expertWidget in self.replacementPanel.GetAllSettingWidgets():
            if expertWidget.IsEnabled() and expertWidget.GetValue():
                if expertWidget.GetName() == "generationalReplacement":
                    replacement.setGenerationalReplacement()
                elif expertWidget.GetName() == "SSGAworse":
                    replacement.setSSGAworse()
                elif expertWidget.GetName() == "SSGAdetTournament":
                    replacement.setSSGAdetTournament(self.replacementPanel.ssgaDetTourTsize.GetValue())
                else:
                    raise RuntimeError("Unknown replacement method choosen")

        # Stop Criteria GA settings
        stop = knnga.GAStopCriteria()

        stopCritSet = False
        for expertWidget in self.stopCriteriaPanel.GetAllSettingWidgets():
            if expertWidget.IsEnabled() and expertWidget.GetValue():
                if expertWidget.GetName() == "bestFitnessStop":
                    stop.setBestFitnessStop(1.0)
                elif expertWidget.GetName() == "maxGenerations":
                    stop.setMaxGenerations(self.stopCriteriaPanel.maxGenerationCount.GetValue())
                elif expertWidget.GetName() == "maxFitnessEvals":
                    stop.setMaxFitnessEvals(self.stopCriteriaPanel.maxFitnessEvalCount.GetValue())
                elif expertWidget.GetName() == "steadyStateStop":
                    stop.setSteadyStateStop(self.stopCriteriaPanel.steadyContinueMin.GetValue(), \
                        self.stopCriteriaPanel.steadyContinueNoChange.GetValue())
                else:
                    raise RuntimeError("Unknown stop criteria choosen")
                stopCritSet = True
        if not stopCritSet:
            raise RuntimeError("At least one stop criteria must be choosen")

        # Parallelization GA settings
        para = knnga.GAParallelization()

        para.mode = self.parallelizationPanel.parallelEnabled.GetValue()
        para.thredNum = self.parallelizationPanel.threadNum.GetValue()

        # encapsulate all settings to the final algorithm
        ga = knnga.GAOptimization(self.frame.classifier, base, selection, cross, \
                                  muta, replacement, stop, para)

        self.workerThread = GAWorker(ga, self.frame)
        self.workerThread.setDaemon(1)

        self.frame.statusPanel.starttime = time.time()
        self.frame.timer.Start(1000)
        self.workerThread.start()

        self.frame.UpdatePanels()
        self.stopButton.Enable()
        self.frame.statusPanel.stopButton.Enable()

        # switch to status panel
        self.frame.notebook.SetSelection(1)

    #---------------------------------------------------------------------------
    def stopCalculation(self):
    #---------------------------------------------------------------------------
        if self.workerThread == None:
            return True

        if self.workerThread.GAOptimizer.status:
            response = wx.MessageDialog(self, "Are you sure you want to stop the "\
                "optimization? This may take a long time.\n(The current generation"\
                " will be finished)", "Stop?", wx.YES_NO).ShowModal()

            if response == wx.ID_NO:
                return False

        # TODO: thread for displaying status indicator?
        # ... gui still responsible?
        self.workerThread.GAOptimizer.stopCalculation()
        self.workerThread.join()
        self.workerThread = None


        self.frame.timer.Stop()
        self.frame.UpdatePanels()
        self.stopButton.Disable()
        self.frame.statusPanel.stopButton.Disable()

        return True
