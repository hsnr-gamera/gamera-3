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

from sys import stderr

try:
   import matplotlib
   if matplotlib.__version__ != "0.73.1":
      print >>stderr, "WARNING: The version of matplotlib you have installed has not been officially"
      print >>stderr, "tested with Gamera.  It may work fine, or you may experience strange"
      print >>stderr, "problems using the matplotlib functionality.  Please include the"
      print >>stderr, "version of your matplotlib (%s) in any bug reports to the Gamera" % (matplotlib.__version__)
      print >>stderr, "developers.\n"
   try:
      matplotlib.use("WXAgg")
      from matplotlib.backends.backend_wxagg import FigureCanvasWxAgg as FigureCanvas
      from matplotlib.backends import backend_wxagg as underlying_backend
   except ImportError:
      matplotlib.use("WX")
      from matplotlib.backends.backend_wx import FigureCanvasWx as FigureCanvas
      from matplotlib.backends import backend_wx as underlying_backend
   from matplotlib.backends import backend_wx
   from matplotlib import backend_bases
   from matplotlib.figure import Figure
   from matplotlib._pylab_helpers import Gcf
   import wx
   from gamera.gui import toolbar, gui_util, gamera_icons
except ImportError:
   print >>stderr, "WARNING: matplotlib could not be imported.  Gamera will still"
   print >>stderr, "work correctly, but plotting functionality will be disabled."
   print >>stderr, "Download and install matplotlib from matplotlib.sourceforge.net,"
   print >>stderr, "then restart Gamera to have plotting support.\n"
   def plot(*args, **kwargs):
      raise RuntimeError("Plotting is not supported because the optional matplotlib library\n"
                       "could not be found.\n\n"
                       "Download and install matplotlib from matplotlib.sourceforge.net,\n"
                       "then restart Gamera to have plotting support.")
   show_figure = plot
   matplotlib_installed = False
else:
   cursord = backend_wx.cursord
   
   class GameraPlotToolbar(backend_bases.NavigationToolbar2, toolbar.ToolBar):
      def __init__(self, parent, canvas):
         self.canvas = canvas
         self._idle = True
         self.statbar = None
         toolbar.ToolBar.__init__(self, parent)
         backend_bases.NavigationToolbar2.__init__(self, canvas)
         
      def _init_toolbar(self):
         load_bitmap = backend_wx._load_bitmap
         self.AddSimpleTool(10, gamera_icons.getIconHomeBitmap(),
                            'Reset original view', self.home)
         self.AddSimpleTool(20, gamera_icons.getIconBackBitmap(),
                            'Back navigational view', self.back)
         self.AddSimpleTool(30, gamera_icons.getIconForwardBitmap(),
                            'Forward navigational view', self.forward)
         self.AddSeparator()
         self.pan_button = self.AddSimpleTool(40, gamera_icons.getIconMoveBitmap(),
                                              'Pan/zoom mode', self.pan, True)
         self.zoom_button = self.AddSimpleTool(50, gamera_icons.getIconZoomViewBitmap(),
                                               'Zoom to rectangle', self.zoom, True)
         self.AddSeparator()
         self.AddSimpleTool(60, gamera_icons.getIconSaveBitmap(),
                            'Save plot contents to file', self.save)
         self.AddSimpleTool(70, gamera_icons.getIconPrinterBitmap(),
                            'Print', self.print_plot)
         
      def save(self, evt):
         filename = gui_util.save_file_dialog(self, self.canvas._get_imagesave_wildcards())
         if filename is not None:
            self.canvas.print_figure(filename)

      def print_plot(self, evt):
         printout = backend_wx.PrintoutWx(self.canvas)
         dialog_data = wx.PrintDialogData()
         if wx.VERSION < (2, 5):
            dialog_data.EnableHelp(False)
            dialog_data.EnablePageNumbers(False)
            dialog_data.EnableSelection(False)
         printer = wx.Printer(dialog_data)
         printer.Print(self, printout, True)

      def zoom(self, evt):
         if evt.GetIsDown():
            self.pan_button.SetValue(False)
         else:
            self.zoom_button.SetValue(True)
         backend_bases.NavigationToolbar2.zoom(self, evt)
         
      def pan(self, evt):
         if evt.GetIsDown():
            self.zoom_button.SetValue(False)
         else:
            self.pan_button.SetValue(True)
         backend_bases.NavigationToolbar2.pan(self, evt)


      # This is all verbatim from backend_wx.py, which for various
      # multiple-ineheritance-related reasons can not just be directly
      # imported

      def set_cursor(self, cursor):
         cursor = wx.StockCursor(cursord[cursor])
         self.canvas.SetCursor( cursor )

      def release(self, event):
         try: del self.lastrect
         except AttributeError: pass

      def dynamic_update(self):
         d = self._idle
         self._idle = False
         if d:
            self.canvas.draw()
            self._idle = True

      def draw_rubberband(self, event, x0, y0, x1, y1):
         'adapted from http://aspn.activestate.com/ASPN/Cookbook/Python/Recipe/189744'
         canvas = self.canvas
         dc = wx.ClientDC(canvas)
         
         # Set logical function to XOR for rubberbanding
         dc.SetLogicalFunction(wx.XOR)
         
         # Set dc brush and pen
         # Here I set brush and pen to white and grey respectively
         # You can set it to your own choices
         
         # The brush setting is not really needed since we
         # dont do any filling of the dc. It is set just for 
         # the sake of completion.
         
         wbrush = wx.Brush(wx.Colour(255,255,255), wx.TRANSPARENT)
         wpen = wx.Pen(wx.Colour(200, 200, 200), 1, wx.SOLID)
         dc.SetBrush(wbrush)
         dc.SetPen(wpen)
         
         dc.ResetBoundingBox()
         dc.BeginDrawing()
         height = self.canvas.figure.bbox.height()
         y1 = height - y1
         y0 = height - y0
         
         if y1<y0: y0, y1 = y1, y0
         if x1<y0: x0, x1 = x1, x0
         
         w = x1 - x0
         h = y1 - y0
         
         rect = int(x0), int(y0), int(w), int(h)
         try: lastrect = self.lastrect
         except AttributeError: pass
         else: dc.DrawRectangle(*lastrect)  #erase last
         self.lastrect = rect
         dc.DrawRectangle(*rect)
         dc.EndDrawing()

      def set_status_bar(self, statbar):
         self.statbar = statbar

      def set_message(self, s):
         if self.statbar is not None: self.statbar.set_function(s)

   class GameraPlotDropTarget(wx.PyDropTarget):
      def __init__(self, figure):
         wx.PyDropTarget.__init__(self)
         self.df = wx.CustomDataFormat("Vector")
         self.data = wx.CustomDataObject(self.df)
         self.SetDataObject(self.data)
         self.figure = figure

      def OnEnter(self, *args):
         return wx.DragCopy
      
      def OnDrop(self, *args):
         return True
      
      def OnDragOver(self, *args):
         return wx.DragCopy
   
      def OnData(self, x, y, d):
         if self.GetData():
            data = eval(self.data.GetData())
            self.figure.axes[0].plot(data)
         return d

   class GameraPlotFrame(wx.Frame):
      def __init__(self, num, figure):
         self.num = num
         wx.Frame.__init__(self, None, -1, 'matplotlib Plot', size=(550, 350))
         self.figure = figure
         self.canvas = FigureCanvas(self, -1, self.figure)
         self.canvas.SetDropTarget(GameraPlotDropTarget(self.figure))
         statbar = backend_wx.StatusBarWx(self)
         self.SetStatusBar(statbar)
         self.toolbar = GameraPlotToolbar(self, self.canvas)
         self.toolbar.set_status_bar(statbar)

         box = wx.BoxSizer(wx.VERTICAL)
         box.Add(self.toolbar, 0, wx.EXPAND)
         box.Add(self.canvas, 1, wx.EXPAND)
         self.SetSizer(box)
         self.Fit()
         self.figmgr = GameraFigureManager(self.canvas, num, self)

      def GetToolBar(self):
         return self.toolbar

      def get_figure_manager(self):
         return self.figmgr

   _plot_num = 0
   def plot(*args):
      figure = Figure()
      axis = figure.add_subplot(111)
      axis.plot(*args)
      show_figure(figure)
      return figure

   def show_figure(figure):
      display = GameraPlotFrame(0, figure)
      display.Show()
      return display

   # Everything below here is just to support pylab mode
   def show():
       for figwin in Gcf.get_all_fig_managers():
           figwin.frame.Show()
           figwin.canvas.realize()
           figwin.canvas.draw()

   def new_figure_manager(num, *args, **kwargs):
       # in order to expose the Figure constructor to the pylab
       # interface we need to create the figure here
       fig = Figure(*args, **kwargs)
       frame = GameraPlotFrame(num, fig)
       figmgr = frame.get_figure_manager()
       figmgr.canvas.realize()
       figmgr.frame.Show()
       return figmgr

   class GameraFigureManager(backend_bases.FigureManagerBase):
      def __init__(self, canvas, num, frame):
         backend_bases.FigureManagerBase.__init__(self, canvas, num)
         self.frame = frame
         self.window = frame
         self.tb = frame.GetToolBar()
         
         def notify_axes_change(fig):
            'this will be called whenever the current axes is changed'        
            if self.tb != None: self.tb.update()
         self.canvas.figure.add_axobserver(notify_axes_change)
        
      def destroy(self, *args):
         self.frame.Destroy()
         self.canvas.Destroy()        
         import wx
         wx.WakeUpIdle()

   from matplotlib import backends
   backends.show = show
   backends.new_figure_manager = new_figure_manager

   matplotlib_installed = True
   
__all__ = "plot show_figure".split()
