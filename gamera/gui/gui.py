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

# Gamera specific
import inspect
from gamera.core import *
from gamera import paths, config
from gamera.gui import gamera_display, image_menu, \
     icon_display, classifier_display, var_name

# wxPython
from wxPython.wx import *
# Handle multiple versions of wxPython
from wxPython.lib.PyCrust import shell
from wxPython.stc import *
from wxPython.lib.splashscreen import SplashScreen

# Python standard library
# import interactive
import sys, types, traceback, os, string

# Set default options
config.add_option_default("shell_style_face", "Helvetica")
config.add_option_default("shell_style_size", 12)
config.add_option_default("shell_x", "5")
config.add_option_default("shell_y", "5")

main_win = None
app = None

######################################################################

class GameraGui:
   def GetImageFilename():
      dlg = wxFileDialog(None, "Choose a file", ".", "", "*.*", wxOPEN)
      if dlg.ShowModal() == wxID_OK:
         filename = dlg.GetPath()
         dlg.Destroy()
         return filename
   GetImageFilename = staticmethod(GetImageFilename)

   def ShowImage(image, title, view_function=None, owner=None):
      wxBeginBusyCursor()
      img = gamera_display.ImageFrame(title = title, owner=owner)
      img.set_image(image, view_function)
      img.Show(1)
      wxEndBusyCursor()
      return img
   ShowImage = staticmethod(ShowImage)

   def ShowImages(list, view_function=None):
      wxBeginBusyCursor()
      img = gamera_display.MultiImageFrame(title = "Multiple Images")
      img.set_image(list, view_function)
      img.Show(1)
      wxEndBusyCursor()
      return img
   ShowImages = staticmethod(ShowImages)

   def ShowHistogram(hist, mark=None):

      f = gamera_display.HistogramDisplay(hist, mark=mark)
      f.Show(1)
   ShowHistogram = staticmethod(ShowHistogram)

   def ShowProjections(x_data, y_data, image):
      f = gamera_display.ProjectionsDisplay(x_data, y_data, image)
      f.Show(1)
   ShowProjections = staticmethod(ShowProjections)

   def ShowClassifier(classifier, current_database, image, symbol_table):
      wxBeginBusyCursor()
      img = classifier_display.ClassifierFrame(classifier, symbol_table)
      img.set_image(current_database, image)
      img.Show(1)
      wxEndBusyCursor()
      return img
   ShowClassifier = staticmethod(ShowClassifier)

   def UpdateIcons():
      global main_win
      main_win.icon_display.update_icons()
   UpdateIcons = staticmethod(UpdateIcons)

   def TopLevel():
      return main_win
   TopLevel = staticmethod(TopLevel)

######################################################################

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
      faces = shell.faces
      options = config.get_options_by_prefix("shell_style_")
      for face in ('times', 'mono', 'helv', 'other'):
         faces[face] = options['face']
      faces.update(options)
      faces['lnsize'] = int(faces['size']) - 2
      self.setStyles(faces)

   def addHistory(self, command):
      if self.history_win:
         self.history_win.add_line(command)
      if self.update:
         self.update()
      shell.Shell.addHistory(self, command)

   def GetLocals(self):
      return self.interp.locals

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

   # The below existed to show progress in the shell window as output was
   # printed.  However, this seems to be no longer necessary with wxPython
   # 2.3.3.1, in fact, in can cause "recursive calls to wxYield" which is
   # bad.
##    def write(self, source):
##       shell.Shell.write(self, source)
##       # wxYield()

######################################################################

class History(wxStyledTextCtrl):
   def __init__(self, parent):
      wxStyledTextCtrl.__init__(self, parent, -1,
                                wxDefaultPosition, wxDefaultSize)
      style = "face:%(face)s,size:%(size)d" % config.get_options_by_prefix("shell_style_")
      self.StyleSetSpec(wxSTC_STYLE_DEFAULT,
                        style)
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
      
      self.history = History(self.hsplitter)
      self.shell = PyCrustGameraShell(self, self.splitter, -1,
                                      "Welcome to Gamera")
      self.shell.history_win = self.history

      self.history.shell = self.shell
      image_menu.set_shell(self.shell)
      image_menu.set_shell_frame(self)
      self.shell.push("from gamera.gui import gui")
      self.shell.push("from gamera.core import *")
      self.shell.push("config.add_option('__gui', gui.GameraGui)")
      self.shell.push("init_gamera()")

      self.Update()

      self.shell.update = self.Update
      self.icon_display.shell = self.shell
      self.icon_display.main = self
      self.shell.SetFocus()

      self.splitter.SetMinimumPaneSize(20)
      self.splitter.SplitVertically(self.icon_display, self.shell, 120)

      self.hsplitter.SetMinimumPaneSize(20)
      self.hsplitter.SplitHorizontally(self.splitter, self.history, 380)
      self.hsplitter.SetSashPosition(380)
      self.splitter.SetSashPosition(120)

      self.status = StatusBar(self)
      self.SetStatusBar(self.status)
      from gamera.gui import gamera_icons
      icon = wxIconFromBitmap(gamera_icons.getIconBitmap())
      self.SetIcon(icon)
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
      toolkits = paths.get_toolkit_names(paths.toolkits)
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
      self.icon_display.update_icons(self.shell.interp.locals)

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

class GameraSplash(wxSplashScreen):
   def __init__(self):
      from gamera.gui import gamera_icons
      wxSplashScreen.__init__(self, gamera_icons.getGameraSplashBitmap(),
                              wxSPLASH_CENTRE_ON_SCREEN|wxSPLASH_TIMEOUT,
                              1000, None, -1,
                              style = (wxSIMPLE_BORDER|
                                       wxFRAME_NO_TASKBAR|wxSTAY_ON_TOP))
      EVT_CLOSE(self, self.OnClose)
         
   def OnClose(self, evt):
      global main_win
      main_win = ShellFrame(NULL, -1, "Gamera")
      main_win.Show(true)
      evt.Skip()

def run():
   global app
   wxInitAllImageHandlers()

   class MyApp(wxApp):
      def __init__(self, parent):
         wxApp.__init__(self, parent)
      
      # wxWindows calls this method to initialize the application
      def OnInit(self):
         self.SetAppName("Gamera")
         self.splash = GameraSplash()
         self.splash.Show()
         return true

      def RunScript(self, script):
         self.win.shell.run("import " + script)

      def OnExit(self):
         pass

   app = MyApp(0)
   
   script = config.get_option("script")
   if script != None:
      app.RunScript(script)

   app.MainLoop()

if __name__ == "__main__":
   run()
