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

from wxPython.wx import *        # wxPython
from os import path

colors = (wxColor(0xbc, 0x2d, 0x2d), wxColor(0xb4, 0x2d, 0xbc),
          wxColor(0x2d, 0x34, 0xbc), wxColor(0x2d, 0xbc, 0x2d),
          wxColor(0x2d, 0xbc, 0xbc), wxColor(0xbc, 0xb7, 0x2d),
          wxColor(0xbc, 0x88, 0x2d), wxColor(0x6e, 0x00, 0xc7))

def get_color(number):
   return colors[number & 0x7]

# Displays a message box
def message(message):
   dlg = wxMessageDialog(None, message, "Message",
                         wxOK | wxICON_INFORMATION)
   dlg.ShowModal()
   dlg.Destroy()

def are_you_sure_dialog(message):
   dlg = wxMessageDialog(None, message, "Are you sure?",
                         wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION)
   result = dlg.ShowModal()
   dlg.Destroy()
   return result == wxID_YES

menu_item_id = 1000
def build_menu(parent, menu_spec):
   global menu_item_id
   menu = wxMenu()
   for name, func in menu_spec:
      if name == None:
         menu.AppendSeparator()
      else:
         menu.Append(menu_item_id, name)
         EVT_MENU(parent, menu_item_id, func)
      menu_item_id += 1
   return menu

last_directory = '.'
def open_file_dialog(extensions="*.*"):
   global last_directory
   dlg = wxFileDialog(None, "Choose a file", last_directory, "", extensions, wxOPEN)
   if dlg.ShowModal() == wxID_OK:
      filename = dlg.GetPath()
      dlg.Destroy()
      last_directory = path.dirname(filename)
      return filename
   return None

def directory_dialog(create=1):
   global last_directory
   if create:
      style = wxDD_NEW_DIR_BUTTON
   else:
      style = 0
   dlg = wxDirDialog(None, "Choose a directory", last_directory, style)
   if dlg.ShowModal() == wxID_OK:
      filename = dlg.GetPath()
      dlg.Destroy()
      last_directory = filename
      return filename
   return None

def save_file_dialog(extensions="*.*"):
   global last_directory
   dlg = wxFileDialog(None, "Choose a file", last_directory, "", extensions, wxSAVE)
   if dlg.ShowModal() == wxID_OK:
      filename = dlg.GetPath()
      dlg.Destroy()
      last_directory = path.dirname(filename)
      return filename
   return ''

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
      self.update(self._num, self._den)

   def update(self, num, den):
      if not self.done:
         self.progress_box.Update((float(num) / float(den)) * 100.0)
         if num >= den:
            self.done = 1
            wxEndBusyCursor()
            self.progress_box.Destroy()

   def kill(self):
      self.update(1,1)
