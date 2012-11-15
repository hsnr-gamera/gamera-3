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

from gamera.gui.gaoptimizer.SettingsPanel import *
from gamera.gui.gaoptimizer.StatusPanel import *
from gamera.gui.gaoptimizer.ResultPanel import *

from gamera import knnga
from gamera.gui import gui_util

#-------------------------------------------------------------------------------
class OptimizerFrame(wx.Frame):
#-------------------------------------------------------------------------------
    #---------------------------------------------------------------------------
    def __init__(self, parent, id, title):
    #---------------------------------------------------------------------------
        wx.Frame.__init__(self, parent, id, title, wx.DefaultPosition, \
                          wx.Size(800,800))

        self.classifier = None
        self.settingsFilename = None

        # Status Bar
        statusBar = self.CreateStatusBar()

        # Menu    
        menuBar = wx.MenuBar()
        menuBar.Append(self.CreateFileMenu(), "&File")
        menuBar.Append(self.CreateClassifierMenu(), "&Classifier")
        self.SetMenuBar(menuBar)

        # Notebook with tabs
        self.notebook = wx.Notebook(self, -1)
        self.settingsPanel = SettingsPanel(self.notebook, -1, self)
        self.statusPanel = StatusPanel(self.notebook, -1, self)
        self.resultPanel = ResultPanel(self.notebook, -1, self)
        self.notebook.AddPage(self.settingsPanel, "Settings")
        self.notebook.AddPage(self.statusPanel, "Status")
        self.notebook.AddPage(self.resultPanel, "Results")

        # timer for status updates
        self.timer = wx.Timer(self, -1)
        self.Bind(wx.EVT_TIMER, self.OnTimerUpdate, self.timer)

    #---------------------------------------------------------------------------
    def CreateFileMenu(self):
    #---------------------------------------------------------------------------
        file_menu = wx.Menu()

        # load menu entries
        open_xml = file_menu.Append(wx.NewId(), "Open from &XML", \
            "classifier data from an existing XML file")
        open_classifier = file_menu.Append(wx.NewId(), "Open existing &classifier",
            "Use an existing classiefier from gamera")
        self.Bind(wx.EVT_MENU, self.OnOpenXML, open_xml)
        self.Bind(wx.EVT_MENU, self.OnOpenClassifier, open_classifier)

        file_menu.AppendSeparator()

        # save menu entries
        save_xml = file_menu.Append(wx.NewId(), "&Save to XML", \
            "Save the current settings to XML")
        save_xml.Enable(False)
        save_xml_as = file_menu.Append(wx.NewId(), "Save to XML &as...", \
            "Save the current settings to XML")
        save_xml_as.Enable(False)
        self.Bind(wx.EVT_MENU, self.OnSaveXML, save_xml)
        self.Bind(wx.EVT_MENU, self.OnSaveXMLAs, save_xml_as)

        file_menu.AppendSeparator()

        # exit menu entry
        exit = file_menu.Append(wx.NewId(), "&Exit", "Exit the application")
        self.Bind(wx.EVT_MENU, self.OnExit, exit)
        self.Bind(wx.EVT_CLOSE, self.OnExit)

        return file_menu

    #---------------------------------------------------------------------------
    def CreateClassifierMenu(self):
    #---------------------------------------------------------------------------
        classifier_menu = wx.Menu()

        # copy existing classifier
        copy_classifier = classifier_menu.Append(wx.NewId(), "&Copy classifier", \
            "Create a copy from classifier and use this")
        copy_classifier.Enable(False)
        self.Bind(wx.EVT_MENU, self.OnCopyClassifier, copy_classifier)

        # insert classifier into gamera gui
        into_gui = classifier_menu.Append(wx.NewId(), "Classifier to &Gamera-GUI", \
            "Insert classifier into gamera gui window")
        into_gui.Enable(False)
        self.Bind(wx.EVT_MENU, self.OnIntoGameraGUI, into_gui)

        # load kNN settings xml (selections, weights, num_k, distance type)
        load_settings = classifier_menu.Append(wx.NewId(), "Load &settings from XML", \
            "Load kNN settings XML into current classifier")
        load_settings.Enable(False)
        self.Bind(wx.EVT_MENU, self.OnLoadSettings, load_settings)

        classifier_menu.AppendSeparator()

        # feature set
        change_features = classifier_menu.Append(wx.NewId(), "Change &feature set", \
            "Set the feature set for the classifier")
        change_features.Enable(False)
        self.Bind(wx.EVT_MENU, self.OnChangeFeatures, change_features)

        # reset selection settings
        reset_selections = classifier_menu.Append(wx.NewId(), "Reset se&lection", \
            "Reset the feature selection values from classifier")
        reset_selections.Enable(False)
        self.Bind(wx.EVT_MENU, self.OnResetSelections, reset_selections)

        # reset weight settings
        reset_weights = classifier_menu.Append(wx.NewId(), "Reset &weights", \
            "Reset the feature weights values from classifier")
        reset_weights.Enable(False)
        self.Bind(wx.EVT_MENU, self.OnResetWeights, reset_weights)

        return classifier_menu

    #---------------------------------------------------------------------------
    def EnableControls(self, flag=True):
    #---------------------------------------------------------------------------
        menuBar = self.GetMenuBar()

        id = menuBar.FindMenuItem("&File", "&Save to XML")
        menuBar.FindItemById(id).Enable(flag)

        id = menuBar.FindMenuItem("&File", "Save to XML &as...")
        menuBar.FindItemById(id).Enable(flag)

        id = menuBar.FindMenuItem("&Classifier", "&Copy classifier")
        menuBar.FindItemById(id).Enable(flag)

        id = menuBar.FindMenuItem("&Classifier", "Classifier to &Gamera-GUI")
        menuBar.FindItemById(id).Enable(flag)

        id = menuBar.FindMenuItem("&Classifier", "Load &settings from XML")
        menuBar.FindItemById(id).Enable(flag)

        id = menuBar.FindMenuItem("&Classifier", "Change &feature set")
        menuBar.FindItemById(id).Enable(flag)

        id = menuBar.FindMenuItem("&Classifier", "Reset se&lection")
        menuBar.FindItemById(id).Enable(flag)

        id = menuBar.FindMenuItem("&Classifier", "Reset &weights")
        menuBar.FindItemById(id).Enable(flag)

        self.settingsPanel.startButton.Enable()

    #---------------------------------------------------------------------------
    def OnTimerUpdate(self, event):
    #---------------------------------------------------------------------------
        self.UpdatePanels()

    #---------------------------------------------------------------------------
    def UpdatePanels(self):
    #---------------------------------------------------------------------------
        self.statusPanel.Update()
        self.resultPanel.Update()

    #---------------------------------------------------------------------------
    def SetClassifiers(self):
    #---------------------------------------------------------------------------
        self.settingsPanel.classifier = self.classifier
        self.statusPanel.classifier = self.classifier
        self.resultPanel.classifier = self.classifier

    #---------------------------------------------------------------------------
    def CopyClassifier(self, classifier):
    #---------------------------------------------------------------------------
        from gamera import knn

        copiedDatabase = []
        for img in self.classifier.get_database():
            imgCopy = img.image_copy()
            imgCopy.id_name = img.id_name
            imgCopy.classification_state = img.classification_state
            copiedDatabase.append(imgCopy)

        copiedClassifier = knn.kNNNonInteractive(
            copiedDatabase,
            self.classifier.features,
            self.classifier._perform_splits,
            self.classifier.num_k,
            self.classifier.normalize)
        copiedClassifier.set_weights(self.classifier.get_weights())
        copiedClassifier.set_selections(self.classifier.get_selections())

        return copiedClassifier

    #---------------------------------------------------------------------------
    def OnOpenXML(self, event):
    #---------------------------------------------------------------------------
        from gamera import gamera_xml
        from gamera import knn

        filename = gui_util.open_file_dialog(self, gamera_xml.extensions)
        if filename == None:
            gui_util.message("Can't open classifier-xml file")
            return

        classifier = knn.kNNNonInteractive(filename)
        self.classifier = classifier
        self.SetClassifiers()

        self.UpdatePanels()
        self.EnableControls(True)

    #---------------------------------------------------------------------------
    def OnOpenClassifier(self, event):
    #---------------------------------------------------------------------------
        from gamera.args import Args, Class
        from gamera.gui import gui
        from gamera import knn

        dialog = Args(
            [Class("Classifier", knn._kNNBase)],
             name="Select an existing classifier...")
        results = dialog.show(
            self, gui.main_win.shell.locals,
            docstring="""Choose an already created classifier for optimization.""")

        if results == None:
            return

        self.classifier = results[0]

        if isinstance(self.classifier, knn.kNNInteractive):
            gui_util.message("Given interactive classifier will be converted"\
                " to noninteractive classifier for optimization (internal)")
            self.classifier = self.CopyClassifier(self.classifier)

        self.SetClassifiers()
        self.UpdatePanels()
        self.EnableControls(True)

    #---------------------------------------------------------------------------
    def OnSaveXML(self, event):
    #---------------------------------------------------------------------------
        if self.classifier == None:
            gui_util.message("No classifier loaded")
            return

        if self.settingsFilename == None:
            self.OnSaveXMLAs(event)

        # save the current settings into file
        wx.BeginBusyCursor()
        try:
            self.classifier.save_settings(self.settingsFilename)
        finally:
            wx.EndBusyCursor()

    #---------------------------------------------------------------------------
    def OnSaveXMLAs(self, event):
    #---------------------------------------------------------------------------
        if self.classifier == None:
            gui_util.message("No classifier loaded")
            return

        filename = gui_util.save_file_dialog(self)
        self.settingsFilename = filename

        # save the current settings into file
        wx.BeginBusyCursor()
        try:
            self.classifier.save_settings(self.settingsFilename)
        finally:
            wx.EndBusyCursor()

    #---------------------------------------------------------------------------
    def OnExit(self, event):
    #---------------------------------------------------------------------------
        response = wx.MessageDialog(self, "Are you sure you want to quit the "\
            "optimization?\n(Make sure you have saved your results)", \
            "Exit?", wx.YES_NO).ShowModal()

        if response == wx.ID_NO:
            return False

        if self.settingsPanel.stopCalculation():
            self.Destroy()
        else:
            return

    #---------------------------------------------------------------------------
    def OnCopyClassifier(self, event):
    #---------------------------------------------------------------------------
        from gamera import knn

        if self.classifier == None:
            gui.message("No classifier loaded")
            return

        if self.classifier.is_interactive():
            self.classifier = self.classifier.noninteractive_copy()
        else:
            self.classifier = self.CopyClassifier(self.classifier)

    #---------------------------------------------------------------------------
    def OnIntoGameraGUI(self, event):
    #---------------------------------------------------------------------------
        from gamera import knn
        from gamera.gui import gui
        from gamera.gui import var_name

        name = var_name.get("classifier", gui.main_win.shell.locals)

        if name == None:
            gui_util.message("No valid variable name for classifier entered")
            return

        copiedClassifier = self.CopyClassifier(self.classifier)

        gui.main_win.shell.locals[name] = copiedClassifier
        gui.main_win.shell.run('\n')

    #---------------------------------------------------------------------------
    def OnLoadSettings(self, event):
    #---------------------------------------------------------------------------
        from gamera import gamera_xml

        if self.classifier == None:
            gui.message("No classifier loaded")
            return

        filename = gui_util.open_file_dialog(self, gamera_xml.extensions)
        if filename == None:
            gui_util.message("Can't open classifier settings xml file")
            return

        self.classifier.load_settings(filename)

        self.UpdatePanels()

    #---------------------------------------------------------------------------
    def OnChangeFeatures(self, event):
    #---------------------------------------------------------------------------
        from gamera import plugin
        from gamera.core import ONEBIT
        from gamera.core import ImageBase
        from gamera.args import Args, Check

        if self.classifier == None:
            gui.message("No classifier loaded")
            return

        allFeatures = [x[0] for x in plugin.methods_flat_category("Features", ONEBIT)]
        allFeatures.sort()

        existingFeatures = [x[0] for x in ImageBase.get_feature_functions(self.classifier.features)[0]]

        featureControls = []
        for f in allFeatures:
            featureControls.append(Check('', f, default=(f in existingFeatures)))

        dialog = Args(featureControls, name = "Feature selection",
            title="Select the features you want to use for optimization")

        result = dialog.show(self)

        if result == None:
            gui_util.message("No change applied")
            return

        selectedFeatures = [name for check, name in zip(result, allFeatures) if check]
        self.classifier.change_feature_set(selectedFeatures)

        self.UpdatePanels()

    #---------------------------------------------------------------------------
    def OnResetSelections(self, event):
    #---------------------------------------------------------------------------
        if self.classifier == None:
            gui.message("No classifier loaded")
            return

        selections = self.classifier.get_selections()

        for sIndex in range(len(selections)):
            selections[sIndex] = 1

        self.classifier.set_selections(selections)

        self.SetClassifiers()
        self.UpdatePanels()

    #---------------------------------------------------------------------------
    def OnResetWeights(self, event):
    #---------------------------------------------------------------------------
        if self.classifier == None:
            gui.message("No classifier loaded")
            return

        weights = self.classifier.get_weights()

        for wIndex in range(len(weights)):
            weights[wIndex] = 1.0

        self.classifier.set_weights(weights)

        self.SetClassifiers()
        self.UpdatePanels()
