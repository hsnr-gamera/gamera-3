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

import wx

from gamera import util
from gamera.gui import gui_util

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
      try:
         docstring = util.dedent(docstring)
         html = gui_util.docstring_to_html(docstring)
         window = wx.html.HtmlWindow(parent, -1, size=wx.Size(50, 100))
         if wx.VERSION >= (2, 5) and "gtk2" in wx.PlatformInfo:
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


