# vi:set tabsize=3:
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

from gamera import core, config
from gamera.gui import gamera_display, gui_util, gamera_icons
from wxPython.wx import *
from wxPython.lib.mixins.listctrl import wxListCtrlAutoWidthMixin
import glob, string, os.path

class FileList(wxGenericDirCtrl):
   def __init__(self, parent, ID, image_display):
      wxGenericDirCtrl.__init__(
          self, parent, ID,
          filter="All files (*.*)|*.*|TIFF files (*.tiff,*.tif)|*.tiff,*.tif",
          style=wxDIRCTRL_SHOW_FILTERS)
      # self.SetDefaultPath(config.options.file.default_directory)
      EVT_TREE_ITEM_ACTIVATED(self.GetTreeCtrl(), -1, self.OnItemSelected)
      self.image_display = image_display

   def OnItemSelected(self, e):
      filename = self.GetFilePath()
      if filename == '':
         return
      wxBeginBusyCursor()
      try:
         try:
            image = core.load_image(filename)
         except Exception, e:
            gui_util.message("Loading image %s failed. \n\nThe error was:\n%s"
                             % (filename, str(e)))
            return
      finally:
         wxEndBusyCursor()
      width, height = self.image_display.id.GetSize()
      scale = max(float(width) / float(image.width),
                  (float(height) / float(image.height)))
      self.image_display.id.set_image(image, weak=0)
      self.image_display.id.scale(scale)

class ImageBrowserFrame(wxFrame):
   def __init__(self):
      wxFrame.__init__(self, NULL, -1, "Image File Browser",
                       wxDefaultPosition,(600, 400))
      icon = wxIconFromBitmap(gamera_icons.getIconImageBrowserBitmap())
      self.SetIcon(icon)
      self.splitter = wxSplitterWindow(self, -1)
      self.image = gamera_display.ImageWindow(self.splitter, -1)
      self.file = FileList(self.splitter, -1, self.image)
      self.splitter.SetMinimumPaneSize(20)
      self.splitter.SplitVertically(self.file, self.image)
      self.splitter.Show(1)
      self.image.id.RefreshAll()


