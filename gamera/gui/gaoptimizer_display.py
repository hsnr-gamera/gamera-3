from wxPython.wx import *
from wxPython.lib.filebrowsebutton import FileBrowseButton
from gamera.core import *
from gamera import gaoptimizer

import time
import math

MAX_POPULATION = 10000
MAX_K = 300


class OptimizerFrame(wxFrame):
    def __init__(self, parent, id, title):
        wxFrame.__init__(self, parent, id, title, wxDefaultPosition,
                         (400, 500))

        self.setup_menus()
        self.ga = gaoptimizer.GaKnnOptimizer()
        self.classifier = None
        self.notebook = wxNotebook(self, -1)
        self.notebook.SetAutoLayout(true)
        self.running = 0
        self.elapsed_time = 0
        self.start_time = 0
        self.filename = None

        self.status = StatusPanel(self.notebook, self.ga)
        self.notebook.AddPage(self.status, "Status")

        self.weights_panel = WeightsPanel(self.notebook)
        self.notebook.AddPage(self.weights_panel, "Best Weights")

        id = NewId()
        self.timer = wxTimer(self, id)
        EVT_TIMER(self, id, self.timer_cb)
        self.last_generation = 0
        EVT_CLOSE(self, self.close_cb)

    def timer_cb(self, evt):
        self.update_status()

    def setup_menus(self):
        # open
        menu = wxMenu()
        id = NewId()
        menu.Append(id, "&Open", "Open a k-NN database")
        EVT_MENU(self, id, self.open_cb)
        
        # save
        id = NewId()
        menu.Append(id, "&Save", "Save the current weights")
        EVT_MENU(self, id, self.save_cb)
        menu.Enable(id, false)
        
        # save as
        id = NewId()
        menu.Append(id, "&Save as", "Save the current weights")
        EVT_MENU(self, id, self.save_as_cb)
        menu.Enable(id, false)
        menu.AppendSeparator()

        # start
        id = NewId()
        menu.Append(id, "&Start", "Start the optimization")
        EVT_MENU(self, id, self.start_cb)
        menu.Enable(id, false)
        
        # stop
        id = NewId()
        menu.Append(id, "&Stop", "Stop the optimization")
        EVT_MENU(self, id, self.stop_cb)
        menu.AppendSeparator()
        menu.Enable(id, false)
        
        # exit
        id = NewId()
        menu.Append(id, "&Exit", "Exit the application")
        EVT_MENU(self, id, self.close_cb)

        self.file_menu = menu
        self.menu_bar = wxMenuBar()
        self.menu_bar.Append(menu, "&File")
        self.SetMenuBar(self.menu_bar)

    def set_classifier(self, classifier):
        self.classifier = classifier
        self.weights_panel.new_classifier(self.classifier)

    def stop(self):
        self.timer.Stop()
        self.ga.stop_worker()
        id = self.file_menu.FindItem("Start")
        self.file_menu.Enable(id, true)
        id = self.file_menu.FindItem("Stop")
        self.file_menu.Enable(id, false)
        self.running = 0
        self.status.state_display.SetValue("not running")

    def start(self):
        self.ga.lock.acquire()
        self.ga.optimize(self.classifier.knn_database)
        self.ga.lock.release()
        id = self.file_menu.FindItem("Start")
        self.file_menu.Enable(id, false)
        id = self.file_menu.FindItem("Stop")
        self.file_menu.Enable(id, true)
        self.timer.Start(1000)
        self.running = 1
        self.elapsed_time = 0
        self.start_time = time.time()

    def open_cb(self, evt):
        dlg = wxFileDialog(None, "Choose a file", ".", "", "*.*", wxOPEN)
        if dlg.ShowModal() == wxID_OK:
            self.filename = dlg.GetPath()
            wxBeginBusyCursor()
            self.stop()
            self.classifier = Classifier(production_database=self.filename,
                                         gui=1)
            self.weights_panel.new_classifier(self.classifier)
            wxEndBusyCursor()
            id = self.file_menu.FindItem("Start")
            self.file_menu.Enable(id, true)
            id = self.file_menu.FindItem("Save")
            self.file_menu.Enable(id, true)
            id = self.file_menu.FindItem("Save as")
            self.file_menu.Enable(id, true)
            self.ga.best_score = 0
        dlg.Destroy()


    def save_cb(self, evt):
        if self.filename != None:
            wxBeginBusyCursor()
            self.classifier.save_production_database(self.filename)
            wxEndBusyCursor()
        else:
            self.save_as_cb(evt)

    def save_as_cb(self, evt):
        dlg = wxFileDialog(None, "Choose a file", ".", "", "*.*", wxSAVE)
        if dlg.ShowModal() == wxID_OK:
            self.filename = dlg.GetPath()
            dlg.Destroy()
            wxBeginBusyCursor()
            self.classifier.save_production_database(self.filename)
            wxEndBusyCursor()

    def start_cb(self, evt):
        self.start()

    def stop_cb(self, evt):
        self.stop()

    def close_cb(self, evt):
        if self.IsTopLevel():
            self.Destroy()
        else:
            self.Show(0)
            evt.Skip()

    def update_status(self):
        if not self.running:
            self.status.state_display.SetValue("not running")
        else:
            self.status.state_display.SetValue("running")
            self.ga.lock.acquire()
            self.status.initial_rate_display.SetValue(str(self.ga.initial_score))
            self.status.current_rate_display.SetValue(str(self.ga.current_score))
            self.status.generation_display.SetValue(str(self.ga.generation))
            self.status.best_rate_display.SetValue(str(self.ga.best_score))
            self.elapsed_time = time.time() - self.start_time
            hours = int(self.elapsed_time / 3600.0)
            minutes = int((self.elapsed_time - (hours * 3600)) / 60.0)
            secs = int((self.elapsed_time - (hours * 3600) - (minutes * 60)))
            self.status.elapsed_display.SetValue(str(hours) + ":h " + str(minutes) + ":m " + str(secs) + ":s")

            self.classifier.knn_database.set_weights(self.ga.get_weights())
            self.ga.lock.release()
            self.weights_panel.update_cb()
            

class StatusPanel(wxPanel):
    def __init__(self, parent, ga):
        wxPanel.__init__(self, parent, -1)

        self.ga = ga

        sizer = wxFlexGridSizer(10, 2, 10, 10)
        sizer.AddGrowableCol(1)
        for i in range(9):
            sizer.AddGrowableRow(i)

        TEXT_SIZE=(150,25)

        # state
        sizer.Add(wxStaticText(self, -1, "Status:"), 1)
        self.state_display = wxTextCtrl(self, -1,"not running", style=wxTE_READONLY, size=TEXT_SIZE)
        sizer.Add(self.state_display, 1)
        
        # initial rate
        sizer.Add(wxStaticText(self, -1, "Initial recognition rate:"), 1)
        self.initial_rate_display = wxTextCtrl(self, -1,"0", style=wxTE_READONLY, size=TEXT_SIZE)
        sizer.Add(self.initial_rate_display, 1)

        # current rate
        sizer.Add(wxStaticText(self, -1, "Current Recognition Rate:"), 1)
        self.current_rate_display = wxTextCtrl(self, -1, "0", style=wxTE_READONLY, size=TEXT_SIZE)
        sizer.Add(self.current_rate_display, 1)

        # Best rate
        sizer.Add(wxStaticText(self, -1, "Best Rate:"), 1)
        self.best_rate_display = wxTextCtrl(self, -1, "0", style=wxTE_READONLY, size=TEXT_SIZE)
        sizer.Add(self.best_rate_display, 1)

        # generation
        sizer.Add(wxStaticText(self, -1, "Current Generation:"), 1)
        self.generation_display = wxTextCtrl(self, -1, "0", style=wxTE_READONLY, size=TEXT_SIZE)
        sizer.Add(self.generation_display, 1)

        # elapsed time
        sizer.Add(wxStaticText(self, -1, "Elapsed Time:"), 1)
        self.elapsed_display = wxTextCtrl(self, -1, "0", style=wxTE_READONLY, size=TEXT_SIZE)
        sizer.Add(self.elapsed_display, 1)

        SLIDER_SIZE=(150, 10)
        SLIDER_STYLE=wxSL_LABELS

        # population size
        sizer.Add(wxStaticText(self, -1, "Population Size:"), 1)
        id = NewId()
        self.population_display = wxSlider(self, id, 10, 10, 5000, size=SLIDER_SIZE, style=SLIDER_STYLE)
        sizer.Add(self.population_display, 1)
        EVT_COMMAND_SCROLL(self, id, self.population_cb)
        
        # k
        sizer.Add(wxStaticText(self, -1, "Size of K:"), 1)
        id = NewId()
        self.k_display = wxSlider(self, id, 1, 1, 200, size=SLIDER_SIZE, style=SLIDER_STYLE)
        sizer.Add(self.k_display, 1)
        EVT_COMMAND_SCROLL(self, id, self.k_cb)

        # crossover rate
        sizer.Add(wxStaticText(self, -1, "Crossover Rate:"), 1)
        id = NewId()
        self.crossover_display = wxSlider(self, id, 60, 0, 100, size=SLIDER_SIZE, style=SLIDER_STYLE)
        sizer.Add(self.crossover_display, 1)
        EVT_COMMAND_SCROLL(self, id, self.crossover_cb)

        # mutation rate
        sizer.Add(wxStaticText(self, -1, "Mutation Rate:"), 1)
        id = NewId()
        self.mutation_display = wxSlider(self, id, 5, 0, 100, size=SLIDER_SIZE, style=SLIDER_STYLE)
        sizer.Add(self.mutation_display, 1)
        EVT_COMMAND_SCROLL(self, id, self.mutation_cb)

        self.SetAutoLayout(true)
        self.SetSizer(sizer)
        sizer.Fit(self)

    def population_cb(self, e):
        self.ga.lock.acquire()
        self.ga.population = self.population_display.GetValue()
        self.ga.lock.release()

    def k_cb(self, e):
        self.ga.lock.acquire()
        self.ga.k = self.k_display.GetValue()
        self.ga.lock.release()

    def crossover_cb(self, e):
        self.ga.lock.acquire()
        self.ga.crossover = self.crossover_display.GetValue() / 100.0
        self.ga.lock.release()

    def mutation_cb(self, e):
        self.ga.lock.acquire()
        self.ga.mutation = self.mutation_display.GetValue() / 100.0
        self.ga.lock.release()

GAUGE_WIDTH = 300
class WeightsPanel(wxPanel):
    def __init__(self, parent):
        wxPanel.__init__(self, parent, -1)
        
    def new_classifier(self, classifier):
        self.classifier = classifier
        sizer = wxFlexGridSizer(len(self.classifier.features_list), 2, 5, 5)
        sizer.AddGrowableCol(1)
        self.bars = []
        for x in self.classifier.features_list:
            text = wxStaticText(self, -1, x)
            sizer.Add(text, 1)
            bar = wxGauge(self, -1, 100)
            bar.SetAutoLayout(true)
            sizer.Add(bar, option=1, flag=wxEXPAND | wxADJUST_MINSIZE)
            self.bars.append(bar)
        self.SetAutoLayout(true)
        self.SetSizer(sizer)
        sizer.Fit(self)

    def update_cb(self):
        weights = self.classifier.knn_database.get_weights()
        for i in range(len(weights)):
            self.bars[i].SetValue(weights[i] * 100)

class BiollanteApp(wxApp):
    def OnInit(self):
        frame = OptimizerFrame(NULL, -1,
                               "GA Optimization for k-NN")
        frame.Show(true)
        self.SetTopWindow(frame)
        return true

def run_ga_app():
    app = BiollanteApp(0)
    app.MainLoop()

if __name__ == "__main__":
    # This is needed to load all of the gamera plugins
    init_gamera()

    run_ga_app()
    
