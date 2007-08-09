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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#

# This stuff must come the very first before any other gui-specific things
# are imported.

# wxPython
try:
   import wxversion
   wxversion.select(["2.8", "2.6", "2.4"])
   from wx import *
except ImportError:
   from wxPython.wx import *
   # Check that the version is correct
   if wxVERSION[:2] < (2, 4) or wxVERSION[:2] > (2, 8):
     raise RuntimeError("""This version of Gamera requires wxPython 2.4.x, 2.6.x or 2.8.x.  However, it seems that you have wxPython %s installed."""
                      % ".".join([str(x) for x in wxVERSION]))

import inspect
from gamera.core import *
from gamera.config import config
from gamera import paths, util
from gamera.gui import gamera_display, image_menu, \
     icon_display, classifier_display, var_name, gui_util, \
     image_browser, has_gui

# wxPython
import wx
import wx.py

# Python standard library
# import interactive
import sys, traceback, os, string, os.path, imp

# Set default options
config.add_option(
   "", "--shell-font-face", default=wx.py.editwindow.FACES['mono'],
   help="[shell] Font face used in the shell")
config.add_option(
   "", "--shell-font-size", default=wx.py.editwindow.FACES['size'],
   type="int",
   help="[shell] Font size used in the shell")
config.add_option(
   "-e", "--execfile", type="string", action="append",
   help="[shell] Run execfile(...) on the given file.  This argument may be given multiple times")
main_win = None
app = None

######################################################################

class GameraGui:
   def GetImageFilename():
      filename = gui_util.open_file_dialog(None, "*.*")
      if filename:
         return filename
   GetImageFilename = staticmethod(GetImageFilename)

   def ShowImage(image, title, view_function=None, owner=None):
      wx.BeginBusyCursor()
      try:
         img = gamera_display.ImageFrame(title=title, owner=owner)
         img.set_image(image, view_function)
         img.Show(True)
      finally:
         wx.EndBusyCursor()
      return img
   ShowImage = staticmethod(ShowImage)

   def ShowImages(list, view_function=None):
      wx.BeginBusyCursor()
      try:
         img = gamera_display.MultiImageFrame(title = "Multiple Images")
         img.set_image(list, view_function)
         img.Show(1)
      finally:
         wx.EndBusyCursor()
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

   def ShowClassifier(classifier=None, current_database=[],
                      image=None, symbol_table=[]):
      if classifier is None:
         from gamera import knn
         classifier = knn.kNNInteractive()
      wx.BeginBusyCursor()
      class_disp = classifier_display.ClassifierFrame(classifier, symbol_table)
      class_disp.set_image(current_database, image)
      class_disp.Show(1)
      wx.EndBusyCursor()
      return class_disp
   ShowClassifier = staticmethod(ShowClassifier)

   def UpdateIcons():
      main_win.icon_display.update_icons()
   UpdateIcons = staticmethod(UpdateIcons)

   def TopLevel():
      return main_win
   TopLevel = staticmethod(TopLevel)

   def ProgressBox(message, length=1):
      return gui_util.ProgressBox(message, length)
   ProgressBox = staticmethod(ProgressBox)

if wx.VERSION >= (2, 5):
   import wx.html
   class Calltip(wx.html.HtmlWindow):
      def __init__(self, parent=None, id=-1):
         wx.html.HtmlWindow.__init__(self, parent, id)
         wx.py.crust.dispatcher.connect(receiver=self.display, signal='Shell.calltip')
         if wx.VERSION >= (2, 5) and "gtk2" in wx.PlatformInfo:
            self.SetStandardFonts()
         self.SetBackgroundColour(wx.Colour(255, 255, 232))
         self.message_displayed = False
         self.cache = {}

      def display(self, calltip):
         """Receiver for Shell.calltip signal."""
         html = gui_util.docstring_to_html(calltip)
         self.SetPage(html)
         self.SetBackgroundColour(wx.Colour(255, 255, 232))

      def OnLinkClicked(self, link):
         if not self.message_displayed:
            gui_util.message("Clicking on links is not supported.")
            self.message_displayed = True
else:
   Calltip = wx.py.crust.Calltip

class PyShellGameraShell(wx.py.shell.Shell):
   def __init__(self, *args, **kwargs):
      self.update = None
      wx.py.shell.Shell.__init__(self, *args, **kwargs)
      self.push("from gamera.gui import gui")
      self.push("from gamera.gui.matplotlib_support import *")
      self.push("from gamera.core import *")
      self.push("init_gamera()")
      self.locals = self.interp.locals
      self.autoCallTip = False

      style = wx.py.editwindow.FACES.copy()
      style['mono'] = config.get("shell_font_face")
      style['size'] = config.get("shell_font_size")
      self.setStyles(style)
      self.ScrollToLine(1)
      if wx.VERSION < (2, 5):
         self.autoComplete = False

   def addHistory(self, command):
      if self.update:
         self.update()
      wx.py.shell.Shell.addHistory(self, command)

   def push(self, source):
      wx.py.shell.Shell.push(self, source)
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
      key = event.GetKeyCode()

      if key in (wx.WXK_UP, wx.WXK_DOWN):
         event.m_controlDown = True
      wx.py.shell.Shell.OnKeyDown(self, event)

   def GetLocals(self):
      return self.locals
      
class PyCrustGameraShell(wx.py.crust.Crust):
   def __init__(self, parent, id=-1, pos=wx.DefaultPosition, 
                size=wx.DefaultSize, style=0,
                name='Crust Window', rootObject=None, rootLabel=None,
                rootIsNamespace=True, intro='', locals=None, 
                InterpClass=None, *args, **kwds):
      wx.SplitterWindow.__init__(self, parent, id, pos, size, style, name)
      self.shell = PyShellGameraShell(parent=self, introText=intro, 
                                      locals=locals, InterpClass=InterpClass, 
                                      *args, **kwds)
      self.editor = self.shell
      if rootObject is None:
         rootObject = self.shell.interp.locals
      self.notebook = wx.Notebook(parent=self, id=-1, style=wx.NB_BOTTOM)
      self.shell.interp.locals['notebook'] = self.notebook
      self.filling = wx.py.filling.Filling(parent=self.notebook, 
                                        rootObject=rootObject, 
                                        rootLabel=rootLabel, 
                                        rootIsNamespace=rootIsNamespace)
      # Add 'filling' to the interpreter's locals.
      self.shell.interp.locals['filling'] = self.filling
      self.calltip = Calltip(parent=self.notebook)
      self.notebook.AddPage(page=self.calltip, text='Documentation', select=True)
      self.notebook.AddPage(page=self.filling, text='Namespace')
      self.sessionlisting = wx.py.crust.SessionListing(parent=self.notebook)
      self.notebook.AddPage(page=self.sessionlisting, text='History')
      self.SplitHorizontally(self.shell, self.notebook, parent.GetClientSize()[1] - 200)
      self.SetMinimumPaneSize(1)
      
class ShellFrame(wx.Frame):
   def __init__(self, parent, id, title):
      global shell
      wx.Frame.__init__(
         self, parent, id, title, (100, 100),
         # Win32 change
         [600, 550],
         style=wx.DEFAULT_FRAME_STYLE|wx.CLIP_CHILDREN|wx.NO_FULL_REPAINT_ON_RESIZE)
      wx.EVT_CLOSE(self, self._OnCloseWindow)

      self.known_modules = {}
      self.menu = self.make_menu()
      self.SetMenuBar(self.menu)

      self.splitter = wx.SplitterWindow(
         self, -1,
         style=wx.SP_3DSASH|wx.CLIP_CHILDREN|
         wx.NO_FULL_REPAINT_ON_RESIZE|wx.SP_LIVE_UPDATE)

      self.icon_display = icon_display.IconDisplay(self.splitter, self)
      self.crust = PyCrustGameraShell(self.splitter, -1)
      self.shell = self.crust.shell
      self.shell.main_win = self
      self.shell.update = self.Update
      image_menu.set_shell(self.shell)
      image_menu.set_shell_frame(self)
      self.shell.push("from gamera.gui import gui")
      self.shell.push("from gamera.gui.matplotlib_support import *")
      self.shell.push("from gamera.core import *")
      self.shell.push("init_gamera()")

      self.Update()

      self.shell.update = self.Update
      self.icon_display.shell = self.shell
      self.icon_display.main = self
      self.Update()
      self.shell.SetFocus()

      self.splitter.SetMinimumPaneSize(20)
      self.splitter.SplitVertically(self.icon_display, self.crust, 120)
      self.splitter.SetSashPosition(120)

      self.status = StatusBar(self)
      self.SetStatusBar(self.status)
      from gamera.gui import gamera_icons
      icon = wx.IconFromBitmap(gamera_icons.getIconBitmap())
      self.SetIcon(icon)
      self.Move(wx.Point(int(30), int(30)))
      wx.Yield()

   def import_command_line_modules(self):
      sys.argv = config.get_free_args()
      if len(sys.argv):
         file = sys.argv[0]
         try:
            name = os.path.basename(file)[:-3]
            module = imp.load_source(name, file)
            self.shell.locals[name] = module
            self.shell.push(name)
            imported = True
         except Exception, e:
            exc_type, exc_value, exc_traceback = sys.exc_info()
            gui_util.message("Error importing file '%s':\n%s" %
                             (file, "".join(
               traceback.format_exception(exc_type, exc_value, exc_traceback))))

      execfiles = config.get("execfile")
      if execfiles is not None:
         for file in execfiles:
            try:
               self.shell.run("execfile(%s)" % repr(file))
            except Exception, e:
               exc_type, exc_value, exc_traceback = sys.exc_info()
               gui_util.message("Error importing file '%s':\n%s" %
                                (file, "".join(traceback.format_exception(
                  exc_type, exc_value, exc_traceback))))

   def make_menu(self):
      self.custom_menus = {}
      file_menu = gui_util.build_menu(
         self,
         (("&Open image...", self._OnFileOpen),
          ("&Image browser...", self._OnImageBrowser),
          (None, None),
          ("Open &XML...", self._OnLoadXML),
          (None, None),
          ("Execute &code...", self._OnExecFile),
          ("&Save history...", self._OnSaveHistory),
          (None, None),
          ("&Biollante...", self._OnBiollante),
          (None, None),
          ("E&xit...", self._OnCloseWindow)))
      classify_menu = gui_util.build_menu(
         self,
         (("&Interactive classifier", self._OnClassifier),))
      toolkits = paths.get_toolkit_names(paths.toolkits)
      self.import_toolkits = {}
      self.reload_toolkits = {}
      self.toolkit_menus = {}
      toolkits_menu = wx.Menu()
      if len(toolkits):
         for toolkit in toolkits:
            toolkitID = wx.NewId()
            toolkit_menu = wx.Menu() #style=wxMENU_TEAROFF)
            toolkit_menu.Append(toolkitID, "Import '%s' toolkit" % toolkit,
                                "Import %s toolkit" % toolkit)
            wx.EVT_MENU(self, toolkitID, self._OnImportToolkit)
            self.import_toolkits[toolkitID] = toolkit
            toolkitID = wx.NewId()
            toolkit_menu.Append(toolkitID, "Reload '%s' toolkit" % toolkit,
                                "Reload %s toolkit" % toolkit)
            wx.EVT_MENU(self, toolkitID, self._OnReloadToolkit)
            self.reload_toolkits[toolkitID] = toolkit
            toolkits_menu.AppendMenu(wx.NewId(), toolkit, toolkit_menu)
            self.toolkit_menus[toolkit] = toolkit_menu
      else:
         toolkits_menu.Append(wx.NewId(), "No toolkits installed.")
      menubar = wx.MenuBar()
      menubar.Append(file_menu, "&File")
      menubar.Append(classify_menu, "&Classify")
      menubar.Append(toolkits_menu, "&Toolkits")
      return menubar

   def add_custom_menu(self, name, menu):
      if name not in self.custom_menus:
         self.toolkit_menus[name].AppendSeparator()
         menu(self.toolkit_menus[name], self.shell, self.shell.GetLocals())
         self.custom_menus.append(name)

   def add_custom_icon_description(self, icon_description):
      self.icon_display.add_class(icon_description)

   def _OnFileOpen(self, event):
      filename = gui_util.open_file_dialog(
         self, util.get_file_extensions("load"))
      if filename:
         name = var_name.get("image", self.shell.locals)
         if name:
            try:
               wx.BeginBusyCursor()
               self.shell.run('%s = load_image(r"%s")' % (name, filename))
            finally:
               wx.EndBusyCursor()

   def _OnImageBrowser(self, event):
      browser = image_browser.ImageBrowserFrame()
      browser.Show(1)

   def _OnLoadXML(self, event):
      from gamera import gamera_xml
      filename = gui_util.open_file_dialog(self, gamera_xml.extensions)
      if filename:
         name = var_name.get("glyphs", self.shell.locals)
         if name:
            wx.BeginBusyCursor()
            try:
               self.shell.run("from gamera import gamera_xml")
               self.shell.run('%s = gamera_xml.glyphs_from_xml(r"%s")' %
                              (name, filename))
            finally:
               wx.EndBusyCursor()

   def _OnExecFile(self, event):
      filename = gui_util.open_file_dialog(self, "Python files (*py)|*.py")
      if filename:
         self.shell.run("execfile(%s)" % (repr(filename)))

   def _OnSaveHistory(self, event):
      filename = gui_util.save_file_dialog(self, "Python files (*.py)|*.py")
      if filename:
         fd = open(filename, "w")
         fd.write(self.history.GetText())
         fd.close()

   def _OnBiollante(self, event):
      from gamera.gui import gaoptimizer_display
      from gamera import knn
      frame = gaoptimizer_display.OptimizerFrame(None, -1, "GA Optimization for k-NN")
      frame.Show(True)

   def _OnClassifier(self, event):
      name = var_name.get("classifier", self.shell.locals)
      if name:
         self.shell.run("from gamera import knn")
         self.shell.run("%s = knn.kNNInteractive()" % name)
         self.shell.run("%s.display()" % name)

   def _OnImportToolkit(self, event):
      self.shell.run("from gamera.toolkits import %s\n" %
                     self.import_toolkits[event.GetId()])

   def _OnReloadToolkit(self, event):
      self.shell.run("reload(%s)\n" %
                     self.reload_toolkits[event.GetId()])

   def _OnCloseWindow(self, event):
      for window in self.GetChildren():
         if isinstance(window, wx.Frame):
            if not window.Close():
               return
      self.Destroy()
      app.ExitMainLoop()

   def Update(self):
      self.icon_display.update_icons(self.shell.interp.locals)

class CustomMenu:
   is_custom_menu = 1
   _items = []
   def __init__(self):
      if not has_gui.has_gui:
         return
      main_win = has_gui.gui.TopLevel()
      if not main_win:
         return
      name = self.__class__.__module__.split('.')[-1]
      if not main_win.custom_menus.has_key(name):
         main_win.custom_menus[name] = None
         self.shell = main_win.shell
         self.locals = main_win.shell.locals
         menu = main_win.toolkit_menus[name]
         menu.AppendSeparator()
         if self._items == []:
            menu.Append(wx.NewId(), "--- empty ---")
         else:
            for item in self._items:
               if item == "-":
                  menu.Break()
               else:
                  menuID = wx.NewId()
                  menu.Append(menuID, item)
                  wx.EVT_MENU(main_win, menuID,
                           getattr(self, "_On" +
                                   util.string2identifier(item)))

class StatusBar(wx.StatusBar):
   def __init__(self, parent):
      wx.StatusBar.__init__(self, parent, -1)
      self.SetFieldsCount(3)
      self.SetStatusText("Gamera", 0)

class GameraSplash(wx.SplashScreen):
   def __init__(self):
      from gamera.gui import gamera_icons
      wx.SplashScreen.__init__(self, gamera_icons.getGameraSplashBitmap(),
                              wx.SPLASH_CENTRE_ON_SCREEN|wx.SPLASH_NO_TIMEOUT,
                              1000, None, -1,
                              style = (wx.SIMPLE_BORDER|
                                       wx.FRAME_NO_TASKBAR|wx.STAY_ON_TOP))

def _show_shell():
   global main_win
   main_win = ShellFrame(None, -1, "Gamera")
   main_win.Show(True)
   main_win.import_command_line_modules()

app = None
def run(startup=_show_shell):
   global app
   has_gui.has_gui = has_gui.WX_GUI
   has_gui.gui = GameraGui
   from gamera.gui import args_gui

   class MyApp(wx.App):
      def __init__(self, startup, parent):
         self._startup = startup
         wx.App.__init__(self, 0)
         self.SetExitOnFrameDelete(1)

      # wxWindows calls this method to initialize the application
      def OnInit(self):
         self.SetAppName("Gamera")
         self.splash = GameraSplash()
         self.splash.Show()
         wx.Yield()
         init_gamera()
         self._startup()
         self.splash.Show(0)
         del self.splash
         return True

      def OnExit(self):
         pass

   app = MyApp(startup, 0)
   app.MainLoop()

if __name__ == "__main__":
   run()

