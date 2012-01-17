# -*- mode: python; indent-tabs-mode: nil; tab-width: 3 -*-
# vim: set tabstop=3 shiftwidth=3 expandtab:
#
# Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom,
#                         and Karl MacMillan
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

import sys

# TODO: needs to be rewritten

import wx
from gamera.gui import gui_util
from gamera.args import *
from gamera.core import *
from gamera.knn import *
from gamera.classify import *
from gamera import plugin
from gamera import gamera_xml
import time, math

SIZER_FLAGS = (wx.TOP|wx.LEFT|wx.RIGHT|
               wx.EXPAND|
               wx.ALIGN_CENTER_VERTICAL|
               wx.ALIGN_LEFT)

MAX_POPULATION = 10000
MAX_K = 300
TIMER_INTERVAL = 2500

class OptimizerFrame(wx.Frame):
   def __init__(self, parent, id, title):
      wx.Frame.__init__(self, parent, id, title, wx.DefaultPosition,
                       (400, 500))

      # we must remember all menu item id's because wx.menu.FindItem
      # is broken in several wxpython versions (e.g. 2.8.7)
      self.menu_item_ids = {}
      self.setup_menus()
      self.classifier = None
      self.notebook = wx.Notebook(self, -1)
      self.notebook.SetAutoLayout(True)
      self.running = False
      self.elapsed_time = 0
      self.start_time = 0
      self.filename = None
      self.settings_filename = None

      self.status = StatusPanel(self.notebook, self.classifier)
      self.notebook.AddPage(self.status, "Status")
      self.status.enable_controls(False)

      self.weights_panel = WeightsPanel(self.notebook)
      self.notebook.AddPage(self.weights_panel, "Best Weights")

      id = wx.NewId()
      self.timer = wx.Timer(self, id)
      wx.EVT_TIMER(self, id, self.timer_cb)
      self.last_generation = 0
      wx.EVT_CLOSE(self, self.close_cb)

   def timer_cb(self, evt):
      self.update_status()

   def setup_menus(self):
      # open
      menu = wx.Menu()
      id = wx.NewId()
      menu.Append(id, "&Open data...", "Open a k-NN database")
      self.menu_item_ids["File|Open data..."] = id
      wx.EVT_MENU(self, id, self.open_cb)

      # save
      id = wx.NewId()
      menu.Append(id, "&Save settings", "Save the current weights")
      self.menu_item_ids["File|Save settings"] = id
      wx.EVT_MENU(self, id, self.save_cb)
      menu.Enable(id, False)

      # save as
      id = wx.NewId()
      menu.Append(id, "&Save settings as...", "Save the current weights")
      self.menu_item_ids["File|Save settings as..."] = id
      wx.EVT_MENU(self, id, self.save_as_cb)
      menu.Enable(id, False)

      # exit
      menu.AppendSeparator()
      id = wx.NewId()
      menu.Append(id, "&Exit", "Exit the application")
      self.menu_item_ids["File|Exit"] = id
      wx.EVT_MENU(self, id, self.close_cb)

      self.file_menu = menu

      menu = wx.Menu()
      # start
      id = wx.NewId()
      menu.Append(id, "&Start", "Start the optimization")
      self.menu_item_ids["Optimizer|Start"] = id
      wx.EVT_MENU(self, id, self.start_cb)
      menu.Enable(id, False)

      # stop
      id = wx.NewId()
      menu.Append(id, "&Stop", "Stop the optimization")
      self.menu_item_ids["Optimizer|Stop"] = id
      wx.EVT_MENU(self, id, self.stop_cb)
      menu.Enable(id, False)

      # Features
      menu.AppendSeparator()
      id = wx.NewId()
      menu.Append(id, "&Features...", "Set the features for the classifier")
      self.menu_item_ids["Optimizer|Features..."] = id
      wx.EVT_MENU(self, id, self.features_cb)
      menu.Enable(id, False)

      self.optimizer_menu = menu

      self.menu_bar = wx.MenuBar()
      self.menu_bar.Append(self.file_menu, "&File")
      self.menu_bar.Append(self.optimizer_menu, "&Optimizer")
      self.SetMenuBar(self.menu_bar)

   def enable_controls(self, enable=True):
      #id = self.optimizer_menu.FindItem("Start") # broken in wxpython 2.8.7
      id = self.menu_item_ids["Optimizer|Start"]
      self.optimizer_menu.Enable(id, enable)
      #id = self.optimizer_menu.FindItem("Features...")
      id = self.menu_item_ids["Optimizer|Features..."]
      self.optimizer_menu.Enable(id, enable)
      #id = self.file_menu.FindItem("Save settings")
      id = self.menu_item_ids["File|Save settings"]
      self.file_menu.Enable(id, True)
      #id = self.file_menu.FindItem("Save settings as...")
      id = self.menu_item_ids["File|Save settings as..."]
      self.file_menu.Enable(id, True)
      self.status.enable_controls(enable)
      self.update_status()
      
   def set_classifier(self, classifier):
      self.classifier = classifier
      self.weights_panel.new_classifier(self.classifier)
      self.status.classifier = classifier

   def stop(self):
      if self.classifier is not None and self.running:
         self.timer.Stop()
         wx.BeginBusyCursor()
         try:
            self.classifier.stop_optimizing()
            self.running = False
            self.enable_controls(True)
            #id = self.optimizer_menu.FindItem("Stop")
            id = self.menu_item_ids["Optimizer|Stop"]
            self.optimizer_menu.Enable(id, False)
         finally:
            wx.EndBusyCursor()

   def start(self):
      if self.running:
         return
      if self.classifier is None:
         gui_util.message("There is no classifier to start.")
         return
      self.classifier.start_optimizing()
      self.enable_controls(False)
      #id = self.optimizer_menu.FindItem("Stop")
      id = self.menu_item_ids["Optimizer|Stop"]
      self.optimizer_menu.Enable(id, True)
      self.timer.Start(TIMER_INTERVAL)
      self.running = True
      self.elapsed_time = 0
      self.start_time = time.time()

   def open_cb(self, evt):
      if self.running:
         gui_util.message("Please stop the optimizer first.")
         return
      import sys
      self.stop_cb(evt)
      filename = gui_util.open_file_dialog(self, gamera.gamera_xml.extensions)
      if filename:
         features = self.features_dialog()
         if features == None:
            return

         self.filename = filename
         glyphs = None
         try:
            wx.BeginBusyCursor()
            try:
               glyphs = gamera_xml.glyphs_from_xml(filename)
               self.set_classifier(kNNNonInteractive(glyphs, features))
            finally:
               wx.EndBusyCursor()
         except Exception, e:
            gui_util.message('Error opening the xml file. The error was:\n\"%s"' %
                             str(e))                
            return
         self.weights_panel.new_classifier(self.classifier)
         self.enable_controls(True)
         #id = self.optimizer_menu.FindItem("Stop")
         id = self.menu_item_ids["Optimizer|Stop"]
         self.optimizer_menu.Enable(id, False)
         return

   def save_cb(self, evt):
      if self.classifier == None:
         gui_util.message("There is no loaded classifier to save.")
      else:
         if self.settings_filename is not None:
            wx.BeginBusyCursor()
            try:
               self.classifier.save_settings(self.settings_filename)
            finally:
               wx.EndBusyCursor()
         else:
            self.save_as_cb(evt)

   def save_as_cb(self, evt):
      if self.classifier == None:
         gui_util.message("There is no loaded classifier to save.")
      else:
         self.settings_filename = gui_util.save_file_dialog(self)
         if self.settings_filename is not None:
            wx.BeginBusyCursor()
            try:
               self.classifier.save_settings(self.settings_filename)
            finally:
               wx.EndBusyCursor()

   def start_cb(self, evt):
      self.start()

   def stop_cb(self, evt):
      if self.running:
         response = wx.MessageDialog(
            self,
            "Are you sure you want to stop the optimizer?  (This may take a few seconds.)",
            "Stop?",
            wx.YES_NO).ShowModal()
         if response == wx.ID_NO:
            return
         self.stop()

   def features_cb(self, evt):
      if self.running:
         gui_util.message("Please stop the optimizer first.")
         return
      if self.classifier == None:
         gui_util.message("Please load some data first.")
         return
      existing_features = self.classifier.features
      selected_features = self.features_dialog(existing_features)
      self.classifier.change_feature_set(selected_features)
      self.weights_panel.new_classifier(self.classifier)

   def features_dialog(self, existing_features=None):
      all_features = [x[0] for x in plugin.methods_flat_category("Features", ONEBIT)]
      all_features.sort()
      if existing_features == None:
         existing_features = all_features[:]
      #else:
      #   existing_features = self.classifier.features
      feature_controls = []
      for x in all_features:
         feature_controls.append(
            Check('', x, default=(x in existing_features)))
      dialog = Args(
         feature_controls,
         name='Feature selection', 
         title='Select the features you want to use for classification')
      result = dialog.show(self)
      if result is None:
         return existing_features
      selected_features = [name for check, name in zip(result, all_features) if check]
      return selected_features
      
   def close_cb(self, evt):
      if self.running:
         self.stop_cb(evt)
      if self.IsTopLevel():
         self.Destroy()
      else:
         self.Show(0)
         evt.Skip()

   def update_status(self):
      if self.running:
         self.elapsed_time = time.time() - self.start_time
      self.status.update_cb(self, self.classifier)
      self.weights_panel.update_cb()


class StatusPanel(wx.Panel):
   def __init__(self, parent, classifier):
      wx.Panel.__init__(self, parent, -1)

      self.classifier = classifier

      sizer = wx.FlexGridSizer(9, 2, 8, 8)
      sizer.AddGrowableCol(1)

      TEXT_SIZE=(150,25)

      # state
      sizer.Add(wx.StaticText(self, -1, "Status:"), 0, SIZER_FLAGS, 10)
      self.state_display = wx.StaticText(
          self, -1,"not running")
      sizer.Add(self.state_display, 0, SIZER_FLAGS, 10)

      # initial rate
      sizer.Add(wx.StaticText(self, -1, "Initial recognition rate:"), 0, SIZER_FLAGS, 10)
      self.initial_rate_display = wx.StaticText(
          self, -1,"0")
      sizer.Add(self.initial_rate_display, 0, SIZER_FLAGS, 10)

      # Best rate
      sizer.Add(wx.StaticText(self, -1, "Best rate:"), 0, SIZER_FLAGS, 10)
      self.best_rate_display = wx.StaticText(
          self, -1, "0.0%", style=wx.TE_READONLY, size=TEXT_SIZE)
      sizer.Add(self.best_rate_display, 0, SIZER_FLAGS, 10)

      # generation
      sizer.Add(wx.StaticText(self, -1, "Current generation:"), 0, SIZER_FLAGS, 10)
      self.generation_display = wx.StaticText(
          self, -1, "0.0%", style=wx.TE_READONLY, size=TEXT_SIZE)
      sizer.Add(self.generation_display, 0, SIZER_FLAGS, 10)

      # elapsed time
      sizer.Add(wx.StaticText(self, -1, "Elapsed time:"), 0, SIZER_FLAGS, 10)
      self.elapsed_display = wx.StaticText(
          self, -1, "0.0%", style=wx.TE_READONLY, size=TEXT_SIZE)
      sizer.Add(self.elapsed_display, 0, SIZER_FLAGS, 10)

      SLIDER_SIZE=(150, 40)
      SLIDER_STYLE=wx.SL_LABELS

      # population size
      sizer.Add(wx.StaticText(self, -1, "Population size:"), 0, SIZER_FLAGS, 10)
      id = wx.NewId()
      self.population_display = wx.Slider(
          self, id, 10, 10, 1000, size=SLIDER_SIZE, style=SLIDER_STYLE)
      sizer.Add(self.population_display, 0, SIZER_FLAGS, 10)
      wx.EVT_COMMAND_SCROLL(self, id, self.population_cb)

      # k
      sizer.Add(wx.StaticText(self, -1, "Size of k:"), 0, SIZER_FLAGS, 10)
      id = wx.NewId()
      self.k_display = wx.Slider(
          self, id, 1, 1, 200, size=SLIDER_SIZE, style=SLIDER_STYLE)
      sizer.Add(self.k_display, 0, SIZER_FLAGS, 10)
      wx.EVT_COMMAND_SCROLL(self, id, self.k_cb)

      # crossover rate
      sizer.Add(wx.StaticText(self, -1, "Crossover rate:"), 0, SIZER_FLAGS, 10)
      id = wx.NewId()
      self.crossover_display = wx.Slider(
          self, id, 60, 0, 100, size=SLIDER_SIZE, style=SLIDER_STYLE)
      sizer.Add(self.crossover_display, 0, SIZER_FLAGS, 10)
      wx.EVT_COMMAND_SCROLL(self, id, self.crossover_cb)

      # mutation rate
      sizer.Add(wx.StaticText(self, -1, "Mutation rate:"), 0, SIZER_FLAGS, 10)
      id = wx.NewId()
      self.mutation_display = wx.Slider(
          self, id, 5, 0, 100, size=SLIDER_SIZE, style=SLIDER_STYLE)
      sizer.Add(self.mutation_display, 0, SIZER_FLAGS, 10)
      wx.EVT_COMMAND_SCROLL(self, id, self.mutation_cb)

      self.SetAutoLayout(True)
      self.SetSizer(sizer)
      sizer.Fit(self)

   def enable_controls(self, enable=True):
      self.population_display.Enable(enable)
      self.k_display.Enable(enable)
      self.crossover_display.Enable(enable)
      self.mutation_display.Enable(enable)
      
   def population_cb(self, e):
      self.classifier.ga_population = self.population_display.GetValue()

   def k_cb(self, e):
      self.classifier.num_k = self.k_display.GetValue()

   def crossover_cb(self, e):
      self.classifier.ga_crossover = self.crossover_display.GetValue() / 100.0

   def mutation_cb(self, e):
      self.classifier.ga_mutation = self.mutation_display.GetValue() / 100.0

   def update_cb(self, optimizer, classifier):
      if not optimizer.running:
         self.state_display.SetLabel("not running")
      else:
         self.state_display.SetLabel("running")
         self.initial_rate_display.SetLabel(
            "%.06f%%" % (classifier.ga_initial * 100.0))
         self.best_rate_display.SetLabel(
            "%.06f%%" % (classifier.ga_best * 100.0))
         self.generation_display.SetLabel(
            "%d" % (classifier.ga_generation))

         # It seems funny to have to do this, but before datetime
         # was introduced in Python 2.3, there was no way to built-in
         # module to deal with time lengths, and backporting datetime
         # seems a little heavyweight for this task - MGD
         hours = int(optimizer.elapsed_time / 3600.0)
         minutes = int((optimizer.elapsed_time % 3600) / 60.0)
         secs = int((optimizer.elapsed_time % 60))
         self.elapsed_display.SetLabel(
            "%d:h %d:m %d:s" % (hours, minutes, secs))

GAUGE_WIDTH = 300
class WeightsPanel(wx.ScrolledWindow):
   def __init__(self, parent):
      wx.ScrolledWindow.__init__(self, parent, -1)

   def new_classifier(self, classifier):
      glyphs = classifier.get_glyphs()
      ff = glyphs[0].feature_functions[0]
      feature_functions = []
      for x in ff:
         tmp = x[1].__call__(glyphs[0])
         if isinstance(tmp, type(glyphs[0].features)):
            for i in range(len(tmp)):
               feature_functions.append(x[0] + " - " + str(i))
         else:
            feature_functions.append(x[0])
      self.DestroyChildren()
      self.classifier = classifier
      sizer = wx.FlexGridSizer(len(feature_functions), 2, 4, 8)
      self.bars = []
      for x in feature_functions:
         text = wx.StaticText(self, -1, x)
         sizer.Add(text, 0, SIZER_FLAGS, 10)
         bar = wx.Gauge(self, -1, 100, style=wx.GA_SMOOTH, size=(200, 10))
         bar.SetAutoLayout(True)
         sizer.Add(bar, 0, SIZER_FLAGS, 10)
         self.bars.append(bar)
      sizer.Layout()
      self.SetSizer(sizer, True)
      width, height = sizer.GetMinSize()
      self.SetScrollbars(10, 10, width / 10, height / 10)

   def update_cb(self):
      weights = self.classifier.get_weights()
      for bar, weight in zip(self.bars, weights):
         value = int(weight * 100)
         if value != bar.GetValue():
            bar.SetValue(int(weight * 100))

class BiollanteApp(wx.App):
   def OnInit(self):
      frame = OptimizerFrame(None, -1,
                             "GA Optimization for k-NN")
      frame.Show(True)
      self.SetTopWindow(frame)
      return True

def run_ga_app():
   app = BiollanteApp(0)
   app.MainLoop()


