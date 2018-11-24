# -*- mode: python; indent-tabs-mode: nil; tab-width: 3 -*-
# vim: set tabstop=3 shiftwidth=3 expandtab:
#
# Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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
from gamera import core, config
from gamera.gui import gamera_display, gui_util, gamera_icons, compatibility
import glob, string, os.path

class FileList(wx.GenericDirCtrl):
   def __init__(self, parent, ID, image_display):
      wx.GenericDirCtrl.__init__(
          self, parent, ID,
          filter="All files (*.*)|*.*|TIFF files (*.tiff,*.tif)|*.tiff,*.tif",
          style=wx.DIRCTRL_SHOW_FILTERS)
      # self.SetDefaultPath(config.options.file.default_directory)
      wx.EVT_TREE_ITEM_ACTIVATED(self.GetTreeCtrl(), -1, self.OnItemSelected)
      self.image_display = image_display

   def OnItemSelected(self, e):
      filename = self.GetFilePath()
      if filename == '':
         return
      wx.BeginBusyCursor()
      try:
         filename = filename.encode('utf8')
      except Exception:
         pass
      try:
         try:
            image = core.load_image(filename)
         except Exception, e:
            gui_util.message("Loading image %s failed. \n\nThe error was:\n%s"
                             % (filename, str(e)))
            return
      finally:
         wx.EndBusyCursor()
      width, height = self.image_display.id.GetSize()
      scale = max(float(width) / float(image.width),
                  (float(height) / float(image.height)))
      self.image_display.id.set_image(image, weak=0)
      self.image_display.id.scale(scale)

class ImageBrowserFrame(wx.Frame):
   def __init__(self):
      wx.Frame.__init__(self, None, -1, "Image File Browser",
                       wx.DefaultPosition,(600, 400))
      icon = compatibility.create_icon_from_bitmap(gamera_icons.getIconImageBrowserBitmap())
      self.SetIcon(icon)
      self.splitter = wx.SplitterWindow(self, -1)
      self.image = gamera_display.ImageWindow(self.splitter, -1)
      self.file = FileList(self.splitter, -1, self.image)
      self.splitter.SetMinimumPaneSize(20)
      self.splitter.SplitVertically(self.file, self.image)
      self.splitter.Show(1)
      self.image.id.RefreshAll()


