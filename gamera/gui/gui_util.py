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

from wxPython.wx import *        # wxPython
from wxPython.lib.dialogs import wxScrolledMessageDialog
from os import path
from types import *
from gamera import util, config
import sys

config.define_option(
   "file", "default_directory", ".",
   help="The default directory when opening files",
   system=1)

colors = (wxColor(0xbc, 0x2d, 0x2d), wxColor(0xb4, 0x2d, 0xbc),
          wxColor(0x2d, 0x34, 0xbc), wxColor(0x2d, 0xbc, 0x2d),
          wxColor(0x2d, 0xbc, 0xbc), wxColor(0xbc, 0xb7, 0x2d),
          wxColor(0xbc, 0x88, 0x2d), wxColor(0x6e, 0x00, 0xc7))

color_number = 0
def get_color(number):
   global color_number
   if isinstance(number, wxColor):
      return number
   if util.is_sequence(number):
      if len(number) == 3:
         return wxColor(*tuple(number))
      else:
         number = None
   if type(number) != IntType:
      number = color_number
      color_number += 1
   return colors[number & 0x7] # mod 8

# Displays a message box
def message(message):
   if "\n" in message:
      dlg = wxScrolledMessageDialog(
         None, message, "Message")
   else:
      dlg = wxMessageDialog(
         None, message, "Message",
         wxOK|wxICON_INFORMATION)
   dlg.ShowModal()
   dlg.Destroy()

def are_you_sure_dialog(parent, message):
   dlg = wxMessageDialog(
      parent, message, "Are you sure?",
      wxYES_NO|wxNO_DEFAULT|wxICON_QUESTION)
   result = dlg.ShowModal()
   dlg.Destroy()
   return result == wxID_YES

menu_item_id = 1000
def build_menu(parent, menu_spec):
   global menu_item_id
   menu = wxMenu()
   for name, func in menu_spec:
      if util.is_sequence(func):
         menu_item_id += 1
         menu.AppendMenu(menu_item_id, name, build_menu(parent, func))
      elif name == None:
         menu.AppendSeparator()
      else:
         menu_item_id += 1
         menu.Append(menu_item_id, name)
         EVT_MENU(parent, menu_item_id, func)
   return menu

NUM_RECENT_FILES = 9
class FileDialog(wxFileDialog):
   def __init__(self, parent, extensions="*.*"):
      self.recent_files_spec = 'recent_files(%s)' % extensions
      config.define_option(
         "file", self.recent_files_spec, [],
         "Recently opened files",
         system=1)
      last_directory = config.options.file.default_directory
      wxFileDialog.__init__(
         self, parent, "Choose a file",
         last_directory, "", extensions, self._flags)
      self.extensions = extensions
      if not sys.platform == 'win32':
         self.button = wxButton(
            self, 10000, "Recent files...", wxPoint(95, 10))
         EVT_BUTTON(self, 10000, self._OnRecentMenu)
         if not len(config.options.file[self.recent_files_spec].get()):
            self.button.Enable(0)

   def show(self):
      result = self.ShowModal()
      self.Destroy()
      if result == wxID_CANCEL:
         return None
      filename = self.GetPath()
      config.options.file.default_directory = path.dirname(filename)
      config.options.file[self.recent_files_spec].set(filename)
      return filename
      
   def _OnRecentMenu(self, event):
      menu = wxMenu()
      for id, file in util.enumerate(config.options.file[self.recent_files_spec].get()):
         menu.Append(id, "&%d %s" % (id + 1, file))
         EVT_MENU(self, id, self._OnRecentMenuItem)
      self.PopupMenu(menu, wxPoint(95, 32))

   def _OnRecentMenuItem(self, event):
      filename = config.options.file[self.recent_files_spec].get()[event.GetId()]
      self.SetPath(filename)
      self.EndModal(wxID_OK)

class OpenFileDialog(FileDialog):
   _flags = wxOPEN

class SaveFileDialog(FileDialog):
   _flags = wxSAVE|wxOVERWRITE_PROMPT

def open_file_dialog(parent, extensions="*.*"):
   return OpenFileDialog(parent, extensions).show()

def save_file_dialog(parent, extensions="*.*"):
   return SaveFileDialog(parent, extensions).show()

def directory_dialog(parent, create=1):
   last_directory = config.options.file.default_directory
   if create:
      style = wxDD_NEW_DIR_BUTTON
   else:
      style = 0
   dlg = wxDirDialog(parent, "Choose a directory", last_directory, style)
   if dlg.ShowModal() == wxID_OK:
      filename = dlg.GetPath()
      dlg.Destroy()
      config.options.file.default_directory.set(filename)
      return filename
   return None

class ProgressBox:
   def __init__(self, message, length=1):
      self.progress_box = wxProgressDialog(
         "Progress", message, 100,
         style=wxPD_APP_MODAL|wxPD_ELAPSED_TIME|wxPD_REMAINING_TIME|wxPD_AUTO_HIDE)
      self.done = 0
      self._num = 0
      self._den = length
      wxBeginBusyCursor()

   def __del__(self):
      if not self.done:
         self.done = 1
         self.progress_box.Destroy()
         wxEndBusyCursor()

   def add_length(self, l):
      self._den += l

   def step(self):
      self._num += 1
      # Note that trying to cut back on the number of calls
      # here is futile.  The testing overhead is greater than
      # the call.
      self.update(self._num, self._den)

   def update(self, num, den):
      if not self.done:
         self.progress_box.Update((float(num) / float(den)) * 100.0)
         if num >= den:
            self.done = 1
            wxEndBusyCursor()
            self.progress_box.Destroy()

   def kill(self):
      self.update(1, 1)
