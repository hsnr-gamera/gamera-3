# vi:set tabsize=3:
#
# Copyright (C) 2001, 2002 Ichiro Fujinaga, Michael Droettboom,
#                          and Karl MacMillan
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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#

# TODO: needs to be rewritten

from wxPython.wx import *
from wxPython.lib.filebrowsebutton import FileBrowseButton
from gamera.gui import gui_util
from gamera.args import *
from gamera.core import *
from gamera.knn import *
from gamera.classify import *
from gamera import gamera_xml
import time, math

MAX_POPULATION = 10000
MAX_K = 300
TIMER_INTERVAL = 2500

class OptimizerFrame(wxFrame):
   def __init__(self, parent, id, title):
      wxFrame.__init__(self, parent, id, title, wxDefaultPosition,
                       (400, 500))

      self.setup_menus()
      self.classifier = None
      self.notebook = wxNotebook(self, -1)
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

      id = wxNewId()
      self.timer = wxTimer(self, id)
      EVT_TIMER(self, id, self.timer_cb)
      self.last_generation = 0
      EVT_CLOSE(self, self.close_cb)

   def timer_cb(self, evt):
      self.update_status()

   def setup_menus(self):
      # open
      menu = wxMenu()
      id = wxNewId()
      menu.Append(id, "&Open", "Open a k-NN database")
      EVT_MENU(self, id, self.open_cb)

      # save
      id = wxNewId()
      menu.Append(id, "&Save", "Save the current weights")
      EVT_MENU(self, id, self.save_cb)
      menu.Enable(id, False)

      # save as
      id = wxNewId()
      menu.Append(id, "&Save as", "Save the current weights")
      EVT_MENU(self, id, self.save_as_cb)
      menu.Enable(id, False)
      menu.AppendSeparator()

      # start
      id = wxNewId()
      menu.Append(id, "&Start", "Start the optimization")
      EVT_MENU(self, id, self.start_cb)
      menu.Enable(id, False)

      # stop
      id = wxNewId()
      menu.Append(id, "&Stop", "Stop the optimization")
      EVT_MENU(self, id, self.stop_cb)
      menu.Enable(id, False)

      # Features
      menu.AppendSeparator()
      id = wxNewId()
      menu.Append(id, "&Features", "Set the features for the classifier")
      EVT_MENU(self, id, self.features_cb)
      menu.Enable(id, False)

      # exit
      menu.AppendSeparator()
      id = wxNewId()
      menu.Append(id, "&Exit", "Exit the application")
      EVT_MENU(self, id, self.close_cb)

      self.file_menu = menu
      self.menu_bar = wxMenuBar()
      self.menu_bar.Append(menu, "&File")
      self.SetMenuBar(self.menu_bar)

   def enable_controls(self, enable=True):
      id = self.file_menu.FindItem("Start")
      self.file_menu.Enable(id, enable)
      id = self.file_menu.FindItem("Features")
      self.file_menu.Enable(id, enable)
      id = self.file_menu.FindItem("Open")
      self.file_menu.Enable(id, enable)
      self.status.enable_controls(True)
      self.update_status()
      
   def set_classifier(self, classifier):
      self.classifier = classifier
      self.weights_panel.new_classifier(self.classifier)

   def stop(self):
      if self.classifier is not None and self.running:
         self.timer.Stop()
         wxBeginBusyCursor()
         self.classifier.stop_optimizing()
         self.running = False
         self.enable_controls(True)
         id = self.file_menu.FindItem("Stop")
         self.file_menu.Enable(id, False)
         wxEndBusyCursor()

   def start(self):
      if self.running:
         return
      if self.classifier is None:
         gui_util.message("There is no classifier to start.")
         return
      self.classifier.start_optimizing()
      self.enable_controls(False)
      id = self.file_menu.FindItem("Stop")
      self.file_menu.Enable(id, True)
      self.timer.Start(TIMER_INTERVAL)
      self.running = True
      self.elapsed_time = 0
      self.start_time = time.time()

   def open_cb(self, evt):
      if self.running:
         gui_util.message("Please stop the optimizer first.")
         return
      import sys
      filename = gui_util.open_file_dialog(self, gamera.gamera_xml.extensions)
      if filename:
         self.filename = filename
         wxBeginBusyCursor()
         self.stop()
         glyphs = None
         try:
            glyphs = gamera_xml.glyphs_from_xml(filename)
         except Exception, e:
            wxEndBusyCursor()
            gui_util.message('Error opening the xml file. The error was:\n\"%s"' %
                             str(e))                
            return

         self.set_classifier(kNNNonInteractive(glyphs))
         self.weights_panel.new_classifier(self.classifier)
         wxEndBusyCursor()
         id = self.file_menu.FindItem("Start")
         self.file_menu.Enable(id, True)
         id = self.file_menu.FindItem("Save")
         self.file_menu.Enable(id, True)
         id = self.file_menu.FindItem("Save as")
         self.file_menu.Enable(id, True)
         id = self.file_menu.FindItem("Features")
         self.file_menu.Enable(id, True)
         self.status.enable_controls(True)
         return

   def save_cb(self, evt):
      if self.classifier == None:
         gui_util.message("There is no loaded classifier to save.")
      else:
         if self.settings_filename != None:
            wxBeginBusyCursor()
            self.classifier.save_settings(self.settings_filename)
            wxEndBusyCursor()
         else:
            self.save_as_cb(evt)

   def save_as_cb(self, evt):
      if self.classifier == None:
         gui_util.message("There is no loaded classifier to save.")
      else:
         self.settings_filename = gui_util.save_file_dialog(self)
         wxBeginBusyCursor()
         self.classifier.save_settings(self.settings_filename)
         wxEndBusyCursor()

   def start_cb(self, evt):
      self.start()

   def stop_cb(self, evt):
      self.stop()

   def features_cb(self, evt):
      if self.running:
         gui_util.message("Please stop the optimizer first.")
      all_features = [x[0] for x in ImageBase.methods_flat_category("Features", ONEBIT)]
      all_features.sort()
      existing_features = [x[0] for x in self.classifier.get_feature_functions()[0]]
      feature_controls = []
      for x in all_features:
         feature_controls.append(
            Check('', x, default=(x in existing_features)))
      dialog = Args(
         feature_controls,
         name='Feature selection', 
         title='Select the features you to use for classification')
      result = dialog.show(self)
      if result is None:
         return
      selected_features = [name for check, name in zip(result, all_features) if check]
      self.classifier.change_feature_set(selected_features)
      self.weights_panel.new_classifier(self.classifier)
      
   def close_cb(self, evt):
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


class StatusPanel(wxPanel):
   def __init__(self, parent, classifier):
      wxPanel.__init__(self, parent, -1)

      self.classifier = classifier

      sizer = wxFlexGridSizer(10, 2, 10, 10)
      sizer.AddGrowableCol(1)
      for i in range(9):
         sizer.AddGrowableRow(i)

      TEXT_SIZE=(150,25)

      # state
      sizer.Add(wxStaticText(self, -1, "Status:"), 1)
      self.state_display = wxTextCtrl(
          self, -1,"not running", style=wxTE_READONLY, size=TEXT_SIZE)
      sizer.Add(self.state_display, 1)

      # initial rate
      sizer.Add(wxStaticText(self, -1, "Initial recognition rate:"), 1)
      self.initial_rate_display = wxTextCtrl(
          self, -1,"0", style=wxTE_READONLY, size=TEXT_SIZE)
      sizer.Add(self.initial_rate_display, 1)

      # current rate
      sizer.Add(wxStaticText(self, -1, "Current Recognition Rate:"), 1)
      self.current_rate_display = wxTextCtrl(
          self, -1, "0", style=wxTE_READONLY, size=TEXT_SIZE)
      sizer.Add(self.current_rate_display, 1)

      # Best rate
      sizer.Add(wxStaticText(self, -1, "Best Rate:"), 1)
      self.best_rate_display = wxTextCtrl(
          self, -1, "0", style=wxTE_READONLY, size=TEXT_SIZE)
      sizer.Add(self.best_rate_display, 1)

      # generation
      sizer.Add(wxStaticText(self, -1, "Current Generation:"), 1)
      self.generation_display = wxTextCtrl(
          self, -1, "0", style=wxTE_READONLY, size=TEXT_SIZE)
      sizer.Add(self.generation_display, 1)

      # elapsed time
      sizer.Add(wxStaticText(self, -1, "Elapsed Time:"), 1)
      self.elapsed_display = wxTextCtrl(
          self, -1, "0", style=wxTE_READONLY, size=TEXT_SIZE)
      sizer.Add(self.elapsed_display, 1)

      SLIDER_SIZE=(150, 10)
      SLIDER_STYLE=wxSL_LABELS

      # population size
      sizer.Add(wxStaticText(self, -1, "Population Size:"), 1)
      id = wxNewId()
      self.population_display = wxSlider(
          self, id, 10, 10, 5000, size=SLIDER_SIZE, style=SLIDER_STYLE)
      sizer.Add(self.population_display, 1)
      EVT_COMMAND_SCROLL(self, id, self.population_cb)

      # k
      sizer.Add(wxStaticText(self, -1, "Size of K:"), 1)
      id = wxNewId()
      self.k_display = wxSlider(
          self, id, 1, 1, 200, size=SLIDER_SIZE, style=SLIDER_STYLE)
      sizer.Add(self.k_display, 1)
      EVT_COMMAND_SCROLL(self, id, self.k_cb)

      # crossover rate
      sizer.Add(wxStaticText(self, -1, "Crossover Rate:"), 1)
      id = wxNewId()
      self.crossover_display = wxSlider(
          self, id, 60, 0, 100, size=SLIDER_SIZE, style=SLIDER_STYLE)
      sizer.Add(self.crossover_display, 1)
      EVT_COMMAND_SCROLL(self, id, self.crossover_cb)

      # mutation rate
      sizer.Add(wxStaticText(self, -1, "Mutation Rate:"), 1)
      id = wxNewId()
      self.mutation_display = wxSlider(
          self, id, 5, 0, 100, size=SLIDER_SIZE, style=SLIDER_STYLE)
      sizer.Add(self.mutation_display, 1)
      EVT_COMMAND_SCROLL(self, id, self.mutation_cb)

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
         self.state_display.SetValue("not running")
      else:
         self.state_display.SetValue("running")
         self.initial_rate_display.SetValue(
            str(classifier.ga_initial * 100.0))
         self.current_rate_display.SetValue(
            str(classifier.ga_best * 100.0))
         self.generation_display.SetValue(
            str(classifier.ga_generation))
         self.best_rate_display.SetValue(
            str(classifier.ga_best * 100.0))

         # It seems funny to have to do this, but before datetime
         # was introduced in Python 2.3, there was no way to built-in
         # module to deal with time lengths, and backporting datetime
         # seems a little heavyweight for this task - MGD
         hours = int(optimizer.elapsed_time / 3600.0)
         minutes = int((optimizer.elapsed_time - (hours * 3600)) / 60.0)
         secs = int((optimizer.elapsed_time - (hours * 3600) - (minutes * 60)))
         self.elapsed_display.SetValue(
            "%d:h %d:m %d:s" % (hours, minutes, secs))

GAUGE_WIDTH = 300
class WeightsPanel(wxScrolledWindow):
   def __init__(self, parent):
      wxScrolledWindow.__init__(self, parent, -1)

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
      sizer = wxFlexGridSizer(len(feature_functions), 2, 5, 5)
      self.bars = []
      for x in feature_functions:
         text = wxStaticText(self, -1, x)
         sizer.Add(text, 1)
         bar = wxGauge(self, -1, 100, style=wxGA_SMOOTH, size=(200, 10))
         bar.SetAutoLayout(True)
         sizer.Add(bar, 1, flag=wxGROW)
         self.bars.append(bar)
      sizer.Layout()
      self.SetSizer(sizer, True)
      width, height = sizer.GetMinSize()
      self.SetScrollbars(10, 10, width / 10, height / 10)

   def update_cb(self):
      weights = self.classifier.get_weights()
      for i in range(len(weights)):
         self.bars[i].SetValue(int(weights[i] * 100))

class BiollanteApp(wxApp):
   def OnInit(self):
      frame = OptimizerFrame(NULL, -1,
                             "GA Optimization for k-NN")
      frame.Show(True)
      self.SetTopWindow(frame)
      return True

def run_ga_app():
   app = BiollanteApp(0)
   app.MainLoop()


