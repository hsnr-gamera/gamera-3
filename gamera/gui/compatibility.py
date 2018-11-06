#
# Copyright (C) 2018      Jens Dahlmanns
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

from distutils.version import LooseVersion

import wx
import wx.py

from gamera import util

def resize_window_virtual(sizer, window):
   """
   Tell the sizer to resize the *virtual size* of the *window* to match the
   sizer's minimal size. This will not alter the on screen size of the
   window, but may cause the addition/removal/alteration of scrollbars
   required to view the virtual area in windows which manage it.

   :param sizer: Sizer used for resizing
   :param window: Window which is resized
   """
   if wx.VERSION < (2, 9):
       sizer.SetVirtualSizeHints(window)
   else:
       sizer.FitInside(window)


def create_help_display(parent, docstring):
   """
   Creates a help window that contains the information specified by the *docstring*.

   :param parent: Window which should be the parent of the help window
   :param docstring: Content of the help window
   :return: help window
   """
   if wx.VERSION >= (2, 5):
      import wx.html
      from gamera.gui.gui_util import docstring_to_html
      try:
         docstring = util.dedent(docstring)
         html = docstring_to_html(docstring)
         window = wx.html.HtmlWindow(parent, -1, size=wx.Size(50, 100))
         if "gtk2" in wx.PlatformInfo:
            window.SetStandardFonts()
         window.SetPage(html)
         window.SetBackgroundColour(wx.Colour(255, 255, 232))
         if wx.VERSION < (2, 8):
            window.SetBestFittingSize(wx.Size(50, 150))
         else:
            window.SetInitialSize(wx.Size(50, 150))
         return window
      except Exception, e:
         print e
   else:
      docstring = util.dedent(docstring)
      style = (wx.TE_MULTILINE | wx.TE_READONLY | wx.TE_RICH2)
      window = wx.TextCtrl(parent, -1, style=style, size=wx.Size(50, 100))
      window.SetValue(docstring)
      window.SetBackgroundColour(wx.Colour(255, 255, 232))
      return window

def configure_size_small_height(grid_sizer, window, min_width, client_area):
   if wx.VERSION < (2, 8):
      grid_sizer.SetBestFittingSize(wx.Size(0, 0))
      window.SetBestFittingSize(wx.Size(min_width, client_area.height/2))
   else:
      grid_sizer.SetInitialSize(wx.Size(0, 0))
      window.SetInitialSize(wx.Size(min_width, client_area.height/2))

def configure_size_normal_height(grid_sizer, window, min_width, min_height):
   if wx.VERSION < (2, 8):
      grid_sizer.SetBestFittingSize(wx.Size(0, 0))
      window.SetBestFittingSize(wx.Size(min_width, min_height))
   else:
      height = window.GetSize().height
      grid_sizer.SetInitialSize(wx.Size(0, 0))
      window.SetInitialSize(wx.Size(min_width,height))

if wx.VERSION >= (2, 5):
   import wx.html
   from gamera.gui.gui_util import docstring_to_html, message
   class Calltip(wx.html.HtmlWindow):
      def __init__(self, parent=None, id=-1):
         wx.html.HtmlWindow.__init__(self, parent, id)
         wx.py.crust.dispatcher.connect(receiver=self.display, signal='Shell.calltip')
         if "gtk2" in wx.PlatformInfo:
            self.SetStandardFonts()
         self.SetBackgroundColour(wx.Colour(255, 255, 232))
         self.message_displayed = False
         self.cache = {}

      def display(self, calltip):
         """Receiver for Shell.calltip signal."""
         html = docstring_to_html(calltip)
         self.SetPage(html)
         self.SetBackgroundColour(wx.Colour(255, 255, 232))

      def OnLinkClicked(self, link):
         if not self.message_displayed:
            message("Clicking on links is not supported.")
            self.message_displayed = True
else:
   Calltip = wx.py.crust.Calltip

def configure_shell_auto_completion(shell):
   if wx.VERSION < (2, 5):
      shell.autoComplete = False

def set_dialog_style(dialog):
   if wx.VERSION < (2, 8):
      dialog.SetStyle(dialog._flags)

def configure_icon_display_style():
   """
   Creates a display-style for icons based on the OS und wx-Version.

   :return: display-style for icons
   """
   if wx.Platform == '__WXMAC__':
      style = wx.LC_ICON|wx.LC_SINGLE_SEL
   else:
      style = wx.LC_LIST|wx.LC_SINGLE_SEL
   if not (wx.VERSION >= (2, 5) and wx.Platform == '__WXGTK__'):
      style |= wx.LC_ALIGN_TOP
   return style

def configure_print_dialog_data(dialog_data):
   """
   Configures the specified print dialog-data in a way that an upcoming OnPrint
   is called.

   :param dialog_data: PrintDialogData
   """
   # TODO: This should theoretically work with wxPython 2.5 +,
   # but it actually causes OnPrint to never be called.
   if wx.VERSION < (2, 5):
      dialog_data.EnableHelp(False)
      dialog_data.EnablePageNumbers(False)
      dialog_data.EnableSelection(False)

def register_get_first_child(tree_ctrl_class):
   """
   Defines a GetFirstChild(root, cookie)-method the given TreeCtrl-class in case the
   wx-Version is 2.5 or greater.

   :param tree_ctrl_class: TreeCtrl-class
   """
   # This is a stub to provide compatibility with wx2.4 and wx2.5
   if wx.VERSION >= (2, 5):
      def GetFirstChild(self, root, cookie):
         return wx.TreeCtrl.GetFirstChild(self, root)
      tree_ctrl_class.GetFirstChild = GetFirstChild

def register_set_scrollbars(image_display_class):
   """
   Defines a SetScrollbars(x_amount, y_amount, w, h, x, y)-method for the given ImageDisplay-class
   in case the wx-Version is 2.5 or greater.

   :param image_display_class: ImageDisplay-class
   """
   if wx.VERSION >= (2, 5):
      def SetScrollbars(self, x_amount, y_amount, w, h, x, y):
         self.SetVirtualSize((w * x_amount, h * y_amount))
         self.SetScrollRate(x_amount, y_amount)
         self.Scroll(x, y)
      image_display_class.SetScrollbars = SetScrollbars

def set_grid_line_colour(grid):
   """
   Sets a colour of (130,130,254) for the grid lines for the specified grid in case
   the wx-Version is 2.5 or greater.

   :param grid:
   """
   if wx.VERSION >= (2, 5):
      grid.SetGridLineColour(wx.Colour(130,130,254))

def register_renderer_access(multi_image_display_class):
   """
   Registers a renderer-accessor method (get_renderer) for the MultiImageDisplay-class.

   :param multi_image_display_class: MultiImageDisplay-class
   """
   if wx.VERSION >= (2,5):
      def get_renderer(self):
         return self.renderer.Clone()
   else:
      def get_renderer(self):
         return self.GetDefaultRenderer()

   multi_image_display_class.get_renderer = get_renderer

def init_image_handlers():
   """
   Initialization of all image handlers (for versions below 2.9)
   """
   #if int(wx.__version__.split('.')[0]) < 3 and int(wx.__version__.split('.')[1]) < 9:
   if LooseVersion(wx.__version__) < LooseVersion('2.9'):
       wx.InitAllImageHandlers() # deprecated since wxPython 2.9

def set_control_down(key_event):
   """
   Sets the control key down for the specified key-event based on the wx-Version.
   """
   if LooseVersion(wx.__version__) < LooseVersion('3.0'):
       key_event.m_controlDown = True
   else:
       key_event.SetControlDown(True)
