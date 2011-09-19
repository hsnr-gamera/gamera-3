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
from wx.lib import dialogs
from os import path
from types import *
from gamera import util
from gamera.config import config
import sys

config.add_option(
   "", "--default-dir", default=".",
   help="[gui] The default directory when opening files")

colors = (wx.Colour(0xbc, 0x2d, 0x2d), wx.Colour(0xb4, 0x2d, 0xbc),
          wx.Colour(0x2d, 0x34, 0xbc), wx.Colour(0x2d, 0xbc, 0x2d),
          wx.Colour(0x2d, 0xbc, 0xbc), wx.Colour(0xbc, 0xb7, 0x2d),
          wx.Colour(0xbc, 0x88, 0x2d), wx.Colour(0x6e, 0x00, 0xc7))

color_number = 0
def get_color(number):
   global color_number
   if isinstance(number, wx.Colour):
      return number
   if util.is_sequence(number):
      if len(number) == 3:
         return wx.Colour(*tuple(number))
      else:
         number = None
   if type(number) != IntType:
      number = color_number
      color_number += 1
   return colors[number & 0x7] # mod 8

# Displays a message box
def message(message):
   if message.count("\n") > 4:
      dlg = dialogs.ScrolledMessageDialog(
         None, message, "Message")
   else:
      dlg = wx.MessageDialog(
         None, message, "Message",
         wx.OK|wx.ICON_INFORMATION)
   dlg.ShowModal()
   dlg.Destroy()

def are_you_sure_dialog(message, parent=None):
   dlg = wx.MessageDialog(
      parent, message, "Are you sure?",
      wx.YES_NO|wx.NO_DEFAULT|wx.ICON_QUESTION)
   result = dlg.ShowModal()
   dlg.Destroy()
   return result == wx.ID_YES

def build_menu(parent, menu_spec):
   global menu_item_id
   menu = wx.Menu()
   for name, func in menu_spec:
      if util.is_sequence(func):
         menu_item_id = wx.NewId()
         menu.AppendMenu(menu_item_id, name, build_menu(parent, func))
      elif name == None:
         menu.AppendSeparator()
      else:
         menu_item_id = wx.NewId()
         menu.Append(menu_item_id, name)
         wx.EVT_MENU(parent, menu_item_id, func)
   return menu

NUM_RECENT_FILES = 9
class FileDialog(wx.FileDialog):
   last_directory = None
   
   def __init__(self, parent, extensions="*.*", multiple=0):
      cls = self.__class__
      if cls.last_directory is None:
         cls.last_directory = path.abspath(config.get("default_dir"))
      if multiple:
         self._flags |= wx.MULTIPLE
         self._multiple = True
      else:
         self._multiple = False
      wx.FileDialog.__init__(
         self, parent, "Choose a file",
         cls.last_directory, "", str(extensions), style=self._flags)
      self.extensions = extensions

   def show(self):
      if wx.VERSION < (2, 8):
         self.SetStyle(self._flags)
      cls = self.__class__
      result = self.ShowModal()
      self.Destroy()
      if result == wx.ID_CANCEL:
         return None
      if self._multiple:
         filenames = self.GetPaths()
         cls.last_directory = path.dirname(filenames[0])
         return filenames
      else:
         filename = self.GetPath()
         cls.last_directory = path.dirname(filename)
         return filename

class OpenFileDialog(FileDialog):
   _flags = wx.OPEN

class SaveFileDialog(FileDialog):
   _flags = wx.SAVE|wx.OVERWRITE_PROMPT

def open_file_dialog(parent, extensions="*.*", multiple=0):
   return OpenFileDialog(parent, extensions, multiple).show()

def save_file_dialog(parent, extensions="*.*"):
   return SaveFileDialog(parent, extensions).show()

def directory_dialog(parent, create=1):
   last_directory = config.get("default_dir")
   if create:
      style = wx.DD_NEW_DIR_BUTTON
   else:
      style = 0
   dlg = wx.DirDialog(parent, "Choose a directory", last_directory, style)
   if dlg.ShowModal() == wx.ID_OK:
      filename = dlg.GetPath()
      dlg.Destroy()
      return filename
   return None

class ProgressBox:
   def __init__(self, message, length=1, numsteps=0):
      assert util.is_string_or_unicode(message)
      self.progress_box = wx.ProgressDialog(
         "Progress", message, 100,
         style=wx.PD_APP_MODAL|wx.PD_ELAPSED_TIME|wx.PD_REMAINING_TIME|wx.PD_AUTO_HIDE)
      self.done = 0
      self._num = 0
      if length == 0:
         self._den = 1
      else:
         self._den = length
      self._numsteps = numsteps
      self._lastupdate = 0
      wx.BeginBusyCursor()

   def __del__(self):
      if not self.done:
         self.done = 1
         self.progress_box.Destroy()
         wx.EndBusyCursor()

   def add_length(self, l):
      self._den += l

   def set_length(self, l):
      self._den = l

   def step(self):
      self._num += 1
      # Note that trying to cut back on the number of calls
      # here is futile.  The testing overhead is greater than
      # the call.
      self.update(self._num, self._den)

   def update(self, num, den):
      if not self.done:
         if num >= den or num == self._lastupdate:
            self.done = True
            wx.EndBusyCursor()
            self.progress_box.Destroy()
         elif 0 == self._numsteps or \
                (den/(num-self._lastupdate) <= self._numsteps):
            self.progress_box.Update(min(100, int((float(num) / float(den)) * 100.0)))
            self._lastupdate = num

   def kill(self):
      self.update(1, 1)

######################################################################
# DOCUMENTATION DISPLAY
#
# If available, we use docutils to format the doc strings from
# reStructuredText into HTML, and display it with a wx.html.HtmlWindow.
# Otherwise, we just default to displaying it as text.
# Since reStructuredText -> HTML conversion is quite slow, we
# cache the HTML chunks based on the md5sum of the docstring.
try:
   import docutils.core
   import docutils.parsers.rst
except ImportError, e:
   # If we don't have docutils, we just wrap the docstring
   # in <pre> tags
   def docstring_to_html(docstring):
      return "<pre>%s</pre>" % docstring
else:
   # Some docstrings may contain SilverCity ".. code::" blocks.
   # Since wx.html.HtmlWindow does not support CSS, it therefore
   # can not properly handle SilverCity's syntax coloring.  So instead,
   # we create a "dummy" code block handler that simply uses <pre>
   def code_block(name, arguments, options, content, lineno,
                  content_offset, block_text, state, state_machine ):
      html = '\n<pre>%s</pre>\n' % "\n".join(content)
      raw = docutils.nodes.raw('', html, format = 'html')
      return [raw]
   code_block.arguments = (1,0,0)
   code_block.options = {'language' : docutils.parsers.rst.directives.unchanged }
   code_block.content = 1
   docutils.parsers.rst.directives.register_directive( 'code', code_block )

   def docstring_to_html(docstring):
      try:
         corrected = docstring.replace("*args", "\*args")
         corrected = corrected.replace("**kwargs", "\*\*kwargs")
         html = docutils.core.publish_string(corrected, writer_name="html")
      except Exception, e:
         html = '''<pre>%s</pre><br/<br/><font size=1><pre>%s</pre></font>''' % (docstring, str(e))
      return html.decode("utf-8")
