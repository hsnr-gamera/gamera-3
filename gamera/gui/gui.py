#
#
# Copyright (C) 2001 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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

# Gamera specific
import inspect
from gamera.gamera import *
from gamera import paths, config, magic_import
from gamera.gui import gamera_display, image_menu, icon_display, classifier_display, var_name

# wxPython
from wxPython.wx import *
# Handle multiple versions of wxPython
from wxPython.__version__ import ver
# We use PyCrust on everything but 2.3.1 (2.3.1 is our minimum requirement
USE_PYCRUST = 1
if ver == '2.3.1':
   USE_PYCRUST = 0
# Import the correct shell
if USE_PYCRUST:
   from wxPython.lib.PyCrust import shell
else:
   from wxPython.lib.pyshell import PyShellWindow
   from wxPython.lib.pyshell import _stderr_style, _stdout_style
from wxPython.stc import *
from wxPython.lib.splashscreen import SplashScreen

# Python standard library
# import interactive
import sys, types, traceback, os, string, webbrowser, resource

# Set default options
config.add_option_default("shell_font", "face:Courier,size:12")
config.add_option_default("shell_x", "5")
config.add_option_default("shell_y", "5")

main_win = None
app = None

wxLocale(wxLANGUAGE_ENGLISH)

######################################################################

class GameraSplash(SplashScreen):
   def OnPaint(self, event):
      SplashScreen.OnPaint(self, event)
      if self.glow >= 0:
         self.PaintText()

   def PaintText(self):
      g = min(self.glow, 1.0)
      dc = wxPaintDC(self)
      font = wxFont(14, wxSWISS, wxNORMAL, wxBOLD)
      dc.SetFont(font)
      authors = ("Ichiro Fujinaga",
                 "Michael Droettboom",
                 "Karl MacMillan",
                 "",
                 "Developed at the",
                 "Digital Knowledge Center",
                 "Sheridan Libraries",
                 "Johns Hopkins University")
      for i in range(len(authors)):
         for x in (0,1,2):
            dc.SetTextForeground(wxColour(99*g,125*g,119*g))
            for y in (0,1,2):
               dc.DrawText(authors[i], 112 + x, 191 + y + i * 20)
         dc.SetTextForeground(wxColour(180*g,213*g,215*g))
         dc.DrawText(authors[i], 113, 192 + i * 20)

######################################################################

def message(message):
   dlg = wxMessageDialog(main_win, message, "Message",
                         wxOK | wxICON_INFORMATION)
   dlg.ShowModal()
   dlg.Destroy()

######################################################################

class GameraGui:
   browser = None
   def GetImageFilename(self):
      dlg = wxFileDialog(None, "Choose a file", ".", "", "*.*", wxOPEN)
      if dlg.ShowModal() == wxID_OK:
         filename = dlg.GetPath()
         dlg.Destroy()
         return filename

   def ShowImage(self, image, title, function, owner=None):
      wxBeginBusyCursor()
      img = gamera_display.ImageFrame(title = title, owner=owner)
      img.set_image(image, function)
      img.Show(1)
      wxEndBusyCursor()
      return img

   def ShowMatrices(self, list, function):
      wxBeginBusyCursor()
      img = gamera_display.MultiImageFrame(title = "Multiple Matrices")
      img.set_image(list, function)
      img.Show(1)
      wxEndBusyCursor()
      return img

   def ShowHistogram(self, hist, mark=None):
      f = gamera_display.HistogramDisplay(hist, mark=mark)
      f.Show(1)

   def ShowProjections(self, x_data, y_data, image):
      f = gamera_display.ProjectionsDisplay(x_data, y_data, image)
      f.Show(1)

   def DisplayHTML(self, file):
      if self.browser == None:
         self.browser = webbrowser.get()
      print "Showing HTML: ", file
      self.browser.open("file:" + file)

   def Message(self, message):
      message(message)

   def ShowClassifier(self, classifier, image, function):
      wxBeginBusyCursor()
      img = classifier_display.ClassifierFrame(classifier)
      img.set_image(classifier.current_database, image, function)
      img.Show(1)
      wxEndBusyCursor()
      return img

   def ShowSymbolTableEditor(self, symbol_table=None):
      ed = classifier_display.SymbolTableEditorFrame(symbol_table)
      ed.Show(1)
      return ed

   def UpdateIcons(self):
      global main_win
      main_win.icon_display.update_icons()

   def TopLevel(self):
      print main_win
      

######################################################################

if USE_PYCRUST:
   class PyCrustGameraShell(shell.Shell):
      def __init__(self, main_win, parent, id, message):
         shell.Shell.__init__(self, parent, id, introText=message)
         self.SetCodePage(1)
         self.history_win = None
         self.update = None
         self.locals = self.interp.locals
         self.main_win = main_win
         self.SetMarginType(1, 0)
         self.SetMarginWidth(1, 0)
         # This is kind of silly, but we need to parse the option string to
         # get separate font and size so that we can use the PyCrust setStyles
         # method. That method takes the faces dictionary to set all of the
         # relevant STC styles (there are a lot - look in the PyCrust shell.py
         # file).
         font = string.split(string.split(config.get_option("shell_font"),
                                          ",")[0], ":")[1]
         size = int(string.split(string.split(config.get_option("shell_font"),
                                              ",")[1], ":")[1])
         faces = { 'times'  : font,
                   'mono'   : font,
                   'helv'   : font,
                   'lucida' : font,
                   'other'  : font,
                   'size'   : size,
                   'lnsize' : size,
                   'backcol': '#FFFFFF',
                   }
         self.setStyles(faces)
         
      def addHistory(self, command):
         if self.history_win:
            self.history_win.add_line(command)
         if self.update:
            self.update()
            shell.Shell.addHistory(self, command)
            
      def GetLocals(self):
         return self.interp.locals
      
      def runsource(self, source):
         self.push(source)
      def push(self, source):
         shell.Shell.push(self, source)
         if source.strip().startswith("import "):
            new_modules = [x.strip() for x in source.strip()[7:].split(",")]
            for module in new_modules:
               if self.interp.locals.has_key(module):
                  for obj in self.interp.locals[module].__dict__.values():
                     if (inspect.isclass(obj)):
                        if hasattr(obj, "is_custom_menu"):
                           self.main_win.add_custom_menu(module, obj)
                        elif hasattr(obj, "is_custom_icon_description"):
                           self.main_win.add_custom_icon_description(obj)
            self.update()

      def OnKeyDown(self, event):
         key = event.KeyCode()
         if self.AutoCompActive():
            event.Skip()
         elif key == WXK_UP:
            self.OnHistoryInsert(step=+1)
         elif key == WXK_DOWN:
            self.OnHistoryInsert(step=-1)
         else:
            shell.Shell.OnKeyDown(self, event)
            
      # Added so we can see progress as it proceeds
      def write(self, source):
         shell.Shell.write(self, source)
         wxYield()
else:
   class PyShellGameraShell(PyShellWindow):
      def Update(self):
         if hasattr(self, 'update'):
            self.update()

      def insert_cached_line(self):
         self.SetSelection(self.GetTextLength() - len(self.GetCurLine()[0]),
                           self.GetTextLength())
         self.ReplaceSelection('')
         self.Prompt()
         if self.lastUsedLine < len(self.lines):
            self.AddText(self.lines[self.lastUsedLine])

      def OnKey(self, evt):
         key = evt.KeyCode()
         if key == WXK_RETURN:
            self.SetCurrentPos(self.GetTextLength())
            PyShellWindow.OnKey(self, evt)
            self.update()
         elif key == WXK_UP:
            self.lastUsedLine = self.lastUsedLine - 1
            if self.lastUsedLine < 0:
               self.lastUsedLine = 0
            self.insert_cached_line()
         elif key == WXK_DOWN:
            self.lastUsedLine = self.lastUsedLine + 1
            if self.lastUsedLine > len(self.lines):
               self.lastUsedLine = len(self.lines)
            self.insert_cached_line()
         elif key == WXK_PAGEUP or key == 312:
            self.lastUsedLine = 0
            self.insert_cached_line()
         elif key == WXK_PAGEDOWN or key == 313:
            self.lastUsedLine = len(self.lines)
            self.insert_cached_line()
         elif key == WXK_BACK or key == WXK_LEFT:
            if self.GetCurrentPos() > self.lastPromptPos:
               evt.Skip()
         elif key == WXK_HOME:
            self.SetCurrentPos(self.lastPromptPos)
         else:
            evt.Skip()

      def PushLine(self, text):
         if ((len(self.lines) > 0 and self.lines[-1] != text) or
             len(self.lines) == 0):
            self.lines.append(text)
            self.history.add_line(text)
         if len(self.lines) > 50:
            self.lines = self.lines[1:]
         self.lastUsedLine = len(self.lines)
         self.SetFocus()

      def Prompt(self):
         # is the current line non-empty?
         text, pos = self.GetCurLine()
         if pos != 0:
            self.AddText('\n')
         self.AddText(self.props['ps1'])
         self.lastPromptPos = self.GetCurrentPos()
         self.EnsureCaretVisible()
         self.ScrollToColumn(0)

      def chunk(self, s):
         if len(s) > 40:
            words = string.split(s)
            s = ''
            l = 0
            for w in words:
               if l + len(w) > 40:
                  s = s + "\n"
                  l = 0
               s = s + w + " "
               l = l + len(w)
         return s

      def pushcode(self, source):
         self.write(source + "\n", wxSTC_STYLE_DEFAULT)
         self.PushLine(source)
         if not self.runsource(source):
            self.Prompt()
            self.SetFocus()
         self.Update()

      # for compatibility with pycrust
      def run(self, source):
         self.pushcode(source)

      def pushresult(self, source):
         self.write("\n" + source)
         self.Prompt()
         self.SetFocus()
         self.Update()


######################################################################

class History(wxStyledTextCtrl):
   def __init__(self, parent):
      wxStyledTextCtrl.__init__(self, parent, -1,
                                wxDefaultPosition, wxDefaultSize)
      self.StyleSetSpec(wxSTC_STYLE_DEFAULT,
                        config.get_option("shell_font"))
      self.SetTabWidth(2)
      EVT_KEY_DOWN(self, self.OnKey)
      EVT_LEFT_DCLICK(self, self.OnDoubleClick)

   def OnKey(self, evt):
      evt.Skip()

   def OnDoubleClick(self, evt):
      text = self.GetCurLine()[0]
      if text != '':
         for i in range(self.GetCurrentLine() + 1, self.GetLineCount()):
            text2 = self.GetLine(i)
            if text2 != '':
               if text2[0] in (' ', '\t'):
                  text = text + string.split(text2, "\n")[0] + "\n"
         self.shell.run(string.split(text, "\n")[0])

   def add_line(self, text):
      self.GotoPos(self.GetTextLength())
      self.AddText(text + "\n")

######################################################################

class ShellFrame(wxFrame):
   def __init__(self, parent, id, title):
      global shell
      wxFrame.__init__(self, parent, id, title, wxDefaultPosition,
                       [600, 550])
      EVT_CLOSE(self, self.OnCloseWindow)

      self.known_modules = {}
      self.menu = self.make_menu()
      self.SetMenuBar(self.menu)

      self.hsplitter = wxSplitterWindow(self, -1)
      self.splitter = wxSplitterWindow(self.hsplitter, -1)
      self.icon_display = icon_display.IconDisplay(self.splitter)
      self.icon_display
      
      font = config.get_option('shell_font')

      self.history = History(self.hsplitter)
      if USE_PYCRUST:
         self.shell = PyCrustGameraShell(self, self.splitter, -1,
                                         "Welcome to Gamera")
         self.shell.history_win = self.history
      else:
         self.shell = PyShellGameraShell(self.splitter, -1,
                                         banner="Welcome to Gamera",
                                         properties={'default': font})
         self.shell.history = self.history
      self.history.shell = self.shell
      image_menu.set_shell(self.shell)
      image_menu.set_shell_frame(self)
      #self.shell.run("")
      self.shell.runsource("from gamera.gui import gui")
      self.shell.runsource("from gamera.gamera import *")
      #self.shell.runsource("set_interactive(gui.GameraGui())")
      self.shell.runsource("init_gamera()")
      self.shell.update = self.Update
      self.icon_display.shell = self.shell
      self.icon_display.main = self
      self.shell.SetFocus()

      self.splitter.SetMinimumPaneSize(20)
      self.splitter.SplitVertically(self.icon_display, self.shell, 80)

      self.hsplitter.SetMinimumPaneSize(20)
      self.hsplitter.SplitHorizontally(self.splitter, self.history, 380)
      self.hsplitter.SetSashPosition(380)
      self.splitter.SetSashPosition(80)

      self.status = StatusBar(self)
      self.SetStatusBar(self.status)

      self.SetIcon(wxIcon(paths.pixmaps + "icon.png", wxBITMAP_TYPE_PNG))
      self.Move(wxPoint(int(config.get_option("shell_x")),
                        int(config.get_option("shell_y"))))

   def make_menu(self):
      self.custom_menus = []
      menubar = wxMenuBar()
      menu = wxMenu()
      openID = wxNewId()
      menu.Append(openID, "&Open", "Open an image")
      EVT_MENU(self, openID, self.OnFileOpen)
      menubar.Append(menu, "&File")
      menu = wxMenu()
      classifyID = wxNewId()
      menu.Append(classifyID, "&Classifier", "Start the classifier")
      EVT_MENU(self, classifyID, self.OnClassifier)
      classifyID = wxNewId()
      menu.Append(classifyID, "&Feature Editor", "Start the feature editor")
      EVT_MENU(self, classifyID, self.OnFeatureEditor)
      classifyID = wxNewId()
      menu.Append(classifyID, "&Process Wizard", "Start the process wizard")
      EVT_MENU(self, classifyID, self.OnProcess)
      menubar.Append(menu, "&Classifier")
      self.toolkit_menu = wxMenu()
      toolkits = magic_import.get_toolkit_names(paths.toolkits)
      self.import_toolkits = {}
      self.reload_toolkits = {}
      self.toolkit_menus = {}
      for toolkit in toolkits:
         toolkitID = wxNewId()
         toolkit_menu = wxMenu(style=wxMENU_TEAROFF)
         toolkit_menu.Append(toolkitID, "Import '%s' toolkit" % toolkit,
                             "Import %s toolkit" % toolkit)
         EVT_MENU(self, toolkitID, self.OnImportToolkit)
         self.import_toolkits[toolkitID] = toolkit
         toolkitID = wxNewId()
         toolkit_menu.Append(toolkitID, "Reload '%s' toolkit" % toolkit,
                             "Reload %s toolkit" % toolkit)
         EVT_MENU(self, toolkitID, self.OnReloadToolkit)
         self.reload_toolkits[toolkitID] = toolkit
         self.toolkit_menu.AppendMenu(wxNewId(), toolkit, toolkit_menu)
         self.toolkit_menus[toolkit] = toolkit_menu
      menubar.Append(self.toolkit_menu, "&Toolkits")
      return menubar

   def add_custom_menu(self, name, menu):
      if name not in self.custom_menus:
         self.toolkit_menus[name].AppendSeparator()
         menu(self.toolkit_menus[name], self.shell, self.shell.GetLocals())
         self.custom_menus.append(name)

   def add_custom_icon_description(self, icon_description):
      self.icon_display.add_class(icon_description)

   def OnFileOpen(self, event):
      dlg = wxFileDialog(self, "Choose a file", ".", "", "*.*", wxOPEN)
      path = None
      if dlg.ShowModal() == wxID_OK:
         path = dlg.GetPath()
      dlg.Destroy()
      if (path):
         name = var_name.get("image", self.shell.locals)
         if name:
            wxBeginBusyCursor()
            self.shell.run(name + " = load_image(\""
                                + path + "\")")
            wxEndBusyCursor()

   def OnClassifier(self, event):
      classifier_display.ClassifierWizard(self.shell, self.shell.GetLocals())

   def OnFeatureEditor(self, event):
      classifier_display.FeatureEditorWizard(self.shell, self.shell.GetLocals())

   def OnProcess(self, event):
      import omr, process
      process.ProcessWizard(self.shell, self.shell.GetLocals(), omr.OMR)

   def OnImportToolkit(self, event):
      self.shell.run("import %s\n" % self.import_toolkits[event.GetId()])

   def OnReloadToolkit(self, event):
      self.shell.run("reload(%s)\n" % self.reload_toolkits[event.GetId()])

   def OnCloseWindow(self, event):
      self.Destroy()
      app.ExitMainLoop()

   def Update(self):
      try:
         if USE_PYCRUST:
            self.icon_display.update_icons(self.shell.interp.locals)
         else:
            self.icon_display.update_icons(self.shell.GetLocals())
      except AttributeError:
         pass

class GameraSplitter(wxSplitterWindow):
   def __init__(self, parent=None, id=-1):
      wxSplitterWindow.__init__(self, parent, id)

class StatusBar(wxStatusBar):
   def __init__(self, parent):
      wxStatusBar.__init__(self, parent, -1)
      self.SetFieldsCount(3)
      self.SetStatusText("Gamera", 0)

class CustomMenu:
   is_custom_menu = 1
   items = []
   def __init__(self, menu, shell, locals):
      self.shell = shell
      self.locals = locals
      if self.items == []:
         menu.Append(wxNewId(), "--- empty ---")
      else:
         for item in self.items:
            if item == "-":
               menu.Break()
            else:
               menuID = wxNewId()
               EVT_MENU(menu, menuID, getattr(self, item))
               menu.Append(menuID, item)

CustomIcon = icon_display.CustomIcon

def run():
   global app
   wxInitAllImageHandlers()
   # Every wxWindows application must have a class derived from wxApp
   class MyApp(wxApp):

      # wxWindows calls this method to initialize the application
      def OnInit(self):
         global main_win
         self.splash = GameraSplash(None,
                                    bitmapfile = paths.pixmaps +
                                    'gamera_splash.png',
                                    duration=2000,callback=self.AfterSplash)
         self.splash.glow = -1
         wxBeginBusyCursor()
         self.splash.Show(true)
         wxYield()
         main_win = self.win = ShellFrame(NULL, -1, "Gamera")
         self.win.Show(true)
         self.SetTopWindow(self.win)
         self.SetAppName("Gamera")
         self.splash.Raise()
         return true

      def AfterSplash(self):
         wxEndBusyCursor()
         import time
         glow = 0.01
         start = time.time() + 2.5
         while time.time() < start:
            if glow < 1.0:
               glow += 0.01
               self.splash.glow = glow
               self.splash.PaintText()
            wxYield()
         self.splash.Close(true)

      def RunScript(self, script):
         self.win.shell.run("import " + script)

   app = MyApp(0)     # Create an instance of the application class
   
   script = config.get_option("script")
   if script != None:
      app.RunScript(script)
            
   app.MainLoop()

if __name__ == "__main__":
   run()
