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

# This is a compatibility wrapper to enable parallel support of wxPython 3 and 4

from distutils.version import LooseVersion

import wx
IS_WXP4 = wx.VERSION >= (4,0)

import wx.py
import wx.grid
if IS_WXP4:
   import wx.adv


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
   import wx
   if wx.VERSION >= (2, 5):
      import wx.html
      from gamera import util
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
      from gamera import util

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
         from gamera.gui.gui_util import docstring_to_html
         """Receiver for Shell.calltip signal."""
         html = docstring_to_html(calltip)
         self.SetPage(html)
         self.SetBackgroundColour(wx.Colour(255, 255, 232))

      def OnLinkClicked(self, link):
         from gamera.gui.gui_util import message
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


#
# wxPython 4 (Phoenix)
#

DropTarget = wx.DropTarget if IS_WXP4 else wx.PyDropTarget
Validator = wx.Validator if IS_WXP4 else wx.PyValidator
GridCellRenderer = wx.grid.GridCellRenderer if IS_WXP4 else wx.grid.PyGridCellRenderer
SplashScreen = wx.adv.SplashScreen if IS_WXP4 else wx.SplashScreen
SPLASH_CENTRE_ON_SCREEN = wx.adv.SPLASH_CENTRE_ON_SCREEN if IS_WXP4 else wx.SPLASH_CENTRE_ON_SCREEN
SPLASH_NO_TIMEOUT = wx.adv.SPLASH_NO_TIMEOUT if IS_WXP4 else wx.SPLASH_NO_TIMEOUT
ASSERT_SUPPRESS = wx.APP_ASSERT_SUPPRESS if IS_WXP4 else wx.PYAPP_ASSERT_SUPPRESS
FD_SAVE = wx.FD_SAVE if IS_WXP4 else wx.SAVE
# TODO: or wx.grid.EVT_GRID_CELL_CHANGING?
EVT_GRID_CELL_CHANGED = wx.grid.EVT_GRID_CELL_CHANGED if IS_WXP4 else wx.grid.EVT_GRID_CELL_CHANGE
FILE_DROP_DONE = True if IS_WXP4 else None

def __get_version():
   if IS_WXP4:
      return wx.VERSION[:2]
   else:
      from wxPython.wx import wxVERSION
      return wxVERSION[:2]

def select_version():
   # This function is no longer called
   try:
      import wxversion
      wxversion.select(["3.0", "2.9", "2.8", "2.6", "2.5", "2.4"])
   except ImportError as e:
      version = __get_version()
      # Check that the version is correct
      if version < (2, 4) or version > (4, 0):
         raise RuntimeError("""This version of Gamera requires wxPython 2.4.x, 2.6.x, 2.8.x, 2.9.x, 3.0.x or 4.0.x.  
         However, it seems that you have wxPython %s installed.""" % ".".join([str(x) for x in version]))

def create_empty_image(width, height, clear=True):
   if IS_WXP4:
      return wx.Image(width, height, clear)
   else:
      return wx.EmptyImage(width, height, clear)

def create_empty_bitmap(width, height, depth=wx.BITMAP_SCREEN_DEPTH):
   if IS_WXP4:
      return wx.Bitmap(width, height, depth)
   else:
      return wx.EmptyBitmap(width, height, depth)

def create_icon_from_bitmap(bmp):
   if IS_WXP4:
      return wx.Icon(bmp)
   else:
      return wx.IconFromBitmap(bmp)

def create_image_from_stream(stream):
   if IS_WXP4:
      return wx.Image(stream)
   else:
      return wx.ImageFromStream(stream)

def create_bitmap_from_image(image):
   if IS_WXP4:
      return wx.Bitmap(image)
   else:
      return wx.BitmapFromImage(image)

def create_stock_cursor(id):
   if IS_WXP4:
      return wx.Cursor(id)
   else:
      return wx.StockCursor(id)

def set_tool_tip(window, tooltip_string):
   """
   Sets the tooltip string for the given window.
   """
   if IS_WXP4:
      window.SetToolTip(tooltip_string)
   else:
      window.SetToolTipString(tooltip_string)

def is_validator_silent():
   """
   Checks whether the wx.Validator is currently silent.

   See also:
   - wxp4 : wx.Validator#IsSilent()
   - older: wx.Validator_IsSilent()
   """
   if IS_WXP4:
      return wx.Validator.IsSilent()
   else:
      return wx.Validator_IsSilent()

def begin_drawing(dc):
   if not IS_WXP4:
      dc.BeginDrawing()

def end_drawing(dc):
   if not IS_WXP4:
      dc.EndDrawing()

def set_size(window, x, y, width, height, sizeFlags=wx.SIZE_AUTO):
   """
   Sets the size of the wx.Window in pixels.
   """
   if IS_WXP4:
      window.SetSize(x, y, width, height, sizeFlags)
   else:
      window.SetDimensions(x, y, width, height, sizeFlags)

def create_data_format(format):
   if IS_WXP4:
      return wx.DataFormat(format)
   else:
      return wx.CustomDataFormat(format)

def get_window_size(window):
   """
   Returns the size of the entire window in pixels, including title bar, border, scrollbars, etc.
   """
   if IS_WXP4:
      return window.GetSize()
   else:
      return window.GetSizeTuple()

def add_img_list_icon(image_list, icon):
   """
   Adds a new image using an icon to the wx.ImageList.
   """
   if IS_WXP4:
      return image_list.Add(icon)
   else:
      return image_list.AddIcon(icon)

def insert_list_img_string_item(list_ctrl, index, label, icon):
   """
   Inserts an image/string item to the wx.ListCtrl.
   """
   if IS_WXP4:
      list_ctrl.InsertItem(index, label, icon)
   else:
      list_ctrl.InsertImageStringItem(index, label, icon)

def set_list_string_item(list_ctrl, index, column, label, image_id):
   """
   Sets an item string field at a particular column for a wx.ListCtrl.
   """
   if IS_WXP4:
      list_ctrl.SetItem(index, column, label, image_id)
   else:
      list_ctrl.SetStringItem(index, column, label, image_id)

def get_list_event_item_index(list_event):
   """
   Returns the item index of the wx.ListEvent.
   """
   if IS_WXP4:
      return list_event.GetIndex()
   else:
      return list_event.m_itemIndex

def get_tree_item_data(tree_ctrl, item):
   """
   Returns the wx.TreeCtrl item data associated with the item.
   """
   if IS_WXP4:
      return tree_ctrl.GetItemData(item)
   else:
      return tree_ctrl.GetPyData(item)

def set_tree_item_data(tree_ctrl, item, data):
   """
   Sets item client data for the specified wx.TreeCtrl.
   """
   if IS_WXP4:
      tree_ctrl.SetItemData(item, data)
   else:
      tree_ctrl.SetPyData(item, data)

def extend_menu(menu, menu_item_id, item, sub_menu):
   """
   Extends the specified wx.Menu with a sub-menu.
   """
   if IS_WXP4:
      menu.Append(menu_item_id, item, sub_menu)
   else:
      menu.AppendMenu(menu_item_id, item, sub_menu)

def handle_event_0(event_handler, event, callable):
   """
   Registers an event handler for the specified event.
   """
   if IS_WXP4:
      event_handler.Bind(event, callable)
   else:
      event(event_handler, callable)

def handle_event_1(event_handler, event, callable, id1=wx.ID_ANY):
   """
   Registers an event handler for the specified event that requires a single ID.
   """
   if IS_WXP4:
      event_handler.Bind(event, callable, id=id1)
   else:
      event(event_handler, id1, callable)

def handle_event_2(event_handler, event, callable, id1=wx.ID_ANY, id2=wx.ID_ANY):
   """
   Registers an event handler for the specified event that requires two IDs.
   """
   if IS_WXP4:
      event_handler.Bind(event, callable, id=id1, id2=id2)
   else:
      event(event_handler, id1, id2, callable)

def handle_timer_event(event_handler, callable, timer_id):
   """
   Registers an event handler for the wx.EVT_TIMER-event.
   """
   if IS_WXP4:
      event_handler.Bind(wx.EVT_TIMER, callable)
   else:
      wx.EVT_TIMER(event_handler, timer_id, callable)
