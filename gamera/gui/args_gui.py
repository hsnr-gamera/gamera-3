# -*- mode: python; indent-tabs-mode: nil; tab-width: 3 -*-
# vim: set tabstop=3 shiftwidth=3 expandtab:
#
# Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom,
#                         and Karl MacMillan
#               2013      Manuel Jeltsch
#               2012-2013 Christoph Dalitz
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
from wx.lib import buttons
import array
import os.path
import string
from gamera import util, enums
from gamera.gui import gui_util
from gamera.core import RGBPixel
from gamera.args import DEFAULT_MAX_ARG_NUMBER, CNoneDefault
import sys

class ArgInvalidException(Exception):
   pass

class Args:
   def _create_controls(self, locals, parent):
      self.controls = []
      # Controls
      if util.is_sequence(self.list[0]):
         notebook = wx.Notebook(parent, -1)
         for page in self.list:
            panel = wx.Panel(notebook, -1)
            gs = self._create_page(locals, panel, page[1:])
            panel.SetSizer(gs)
            notebook.AddPage(panel, page[0])
         return notebook
      else:
         return self._create_page(locals, parent, self.list)

   def _create_page(self, locals, parent, page):
      sw = wx.ScrolledWindow(
         parent, style=wx.SIMPLE_BORDER,
         size=(-1, -1))
      gs = self._create_page_impl(locals, sw, page)
      sw.SetSizer(gs)
      if wx.VERSION < (2, 9):
          gs.SetVirtualSizeHints(sw)
      else:
          gs.FitInside(sw)
      return sw

   def _create_page_impl(self, locals, parent, page):
      gs = wx.FlexGridSizer(len(page), 2, 2, 2)
      for item in page:
         if item.name == None:
            item.name = "ERROR!  No name given!"
         gs.Add(wx.StaticText(parent, -1, item.name),
                0,
                (wx.TOP|wx.LEFT|wx.RIGHT|
                 wx.EXPAND|
                 wx.ALIGN_CENTER_VERTICAL|
                 wx.ALIGN_LEFT), 10)
         control = item.get_control(parent, locals)
         self.controls.append(control)
         gs.Add(control.control,
                0,
                (wx.TOP|wx.LEFT|wx.RIGHT|
                 wx.EXPAND|
                 wx.ALIGN_CENTER_VERTICAL|
                 wx.ALIGN_RIGHT), 10)
      gs.AddGrowableCol(1)
      return gs

   def _create_buttons(self):
      # Buttons
      buttons = wx.BoxSizer(wx.HORIZONTAL)
      ok = wx.Button(self.window, wx.ID_OK, "OK")
      ok.SetDefault()
      buttons.AddStretchSpacer(1)
      buttons.Add(ok, 0, wx.ALL, 5)
      buttons.Add(wx.Button(self.window,
                           wx.ID_CANCEL,
                           "Cancel"),
                  0,
                  wx.ALL,
                  5)
      self.buttons = buttons
      return buttons

   def _create_wizard_buttons(self):
      # Buttons
      buttons = wx.BoxSizer(wx.HORIZONTAL)
      buttons.AddStretchSpacer(1)
      buttons.Add(wx.Button(self.window,
                           wx.ID_CANCEL,
                           "< Back"),
                  0,
                  wx.ALL,
                  5)
      ok = wx.Button(self.window,
                    wx.ID_OK,
                    "Next >")
      ok.SetDefault()
      buttons.Add(ok,
                  0,
                  wx.ALL,
                  5)
      return buttons

   if wx.VERSION >= (2, 5):
      import wx.html
      def _create_help_display(self, docstring):
         try:
            docstring = util.dedent(docstring)
            html = gui_util.docstring_to_html(docstring)
            window = wx.html.HtmlWindow(self.window, -1, size=wx.Size(50, 100))
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
      def _create_help_display(self, docstring):
         docstring = util.dedent(docstring)
         style = (wx.TE_MULTILINE | wx.TE_READONLY | wx.TE_RICH2)
         window = wx.TextCtrl(self.window, -1, style=style, size=wx.Size(50, 100))
         window.SetValue(docstring)
         window.SetBackgroundColour(wx.Colour(255, 255, 232))
         return window

   # generates the dialog box
   def setup(self, parent, locals, docstring = "", function = None):
      if function is not None:
         name = function
      else:
         name = self.name
      self.window = wx.Dialog(parent, -1, name,
                              style=wx.CAPTION|wx.RESIZE_BORDER)
      self.window.SetAutoLayout(1)
      if self.wizard:
         bigbox = wx.BoxSizer(wx.HORIZONTAL)
         from gamera.gui import gamera_icons
         bmp = gamera_icons.getGameraWizardBitmap()
         bitmap = wx.StaticBitmap(self.window, -1, bmp)
         bigbox.Add(bitmap, 0, wx.ALIGN_TOP)
      self.box = wx.BoxSizer(wx.VERTICAL)
      self.border = wx.BoxSizer(wx.HORIZONTAL)
      self.window.SetSizer(self.border)
      self.gs = self._create_controls(locals, self.window)
      if self.wizard:
         buttons = self._create_wizard_buttons()
      else:
         buttons = self._create_buttons()
      # Put it all together
      if self.title != None:
         static_text = wx.StaticText(self.window, 0, self.title)
         font = wx.Font(12, wx.SWISS, wx.NORMAL, wx.BOLD, False, "Helvetica")
         static_text.SetFont(font)
         self.box.Prepend(static_text, 0,
                      wx.EXPAND|wx.BOTTOM, 20)
      self.box.Add(self.gs, 1,
                   wx.EXPAND|wx.ALIGN_RIGHT)
      self.box.Add(wx.Panel(self.window, -1, size=(20,20)), 0,
                   wx.ALIGN_RIGHT)
      if docstring:
         help = self._create_help_display(docstring)
         self.box.Add(help, 1, wx.EXPAND)
      self.box.Add(buttons, 0, wx.ALIGN_RIGHT|wx.EXPAND)
      if self.wizard:
         bigbox.Add(
            self.box, 1,
            wx.EXPAND|wx.ALL|wx.ALIGN_TOP,
            15)
         self.border.Add(
            bigbox, 1, wx.EXPAND|wx.ALL, 10)
      else:
         self.border.Add(
            self.box, 1, wx.EXPAND|wx.ALL, 15)
      self.border.Fit(self.window)
      self.border.Layout()
      size = self.window.GetSize()
      self.window.SetSize((max(450, size[0]), max(200, size[1])))
      min_width = self.border.GetMinSize().width
      #self.border.Layout()
      #self.border.Fit(self.window)
      min_height = self.border.GetMinSize().height
      display = wx.Display(0)
      client_area = display.GetClientArea()
      if min_width < 350:
         min_width = 350
      if min_height < 200:
         min_height = 200
      if min_height < client_area.height/100*98:
         if wx.VERSION < (2, 8):
            self.gs.SetBestFittingSize(wx.Size(0, 0))
            self.window.SetBestFittingSize(wx.Size(min_width,min_height))
            self.gs.SetWindowStyle(wx.BORDER_NONE)
         else:
            height = self.window.GetSize().height
            self.gs.SetInitialSize(wx.Size(0, 0))
            self.window.SetInitialSize(wx.Size(min_width,height))
            self.gs.SetWindowStyle(wx.BORDER_NONE)
      else:
         if wx.VERSION < (2, 8):
            self.gs.SetBestFittingSize(wx.Size(0, 0))
            self.window.SetBestFittingSize(wx.Size(min_width,client_area.height/2))
            self.gs.EnableScrolling(0, 1)
            self.gs.SetScrollRate(0, 20)
         else:
            self.gs.SetInitialSize(wx.Size(0, 0))
            self.window.SetInitialSize(wx.Size(min_width,client_area.height/2))
            self.gs.EnableScrolling(0, 1)
            self.gs.SetScrollRate(0, 20)
      self.border.Layout()
      self.border.Fit(self.window)
      self.window.Centre()

   def get_args_string(self):
      results = [x.get_string() for x in self.controls]
      tuple = '(' + ', '.join(results) + ')'
      return tuple

   def get_args(self):
      return [control.get() for control in self.controls]

   def show(self, parent=None, locals={}, function=None, wizard=0, docstring=""):
      docstring = util.dedent(docstring)
      self.wizard = wizard
      if function != None:
         self.function = function
      self.setup(parent, locals, docstring, function)
      while 1:
         result = wx.Dialog.ShowModal(self.window)
         try:
            if result == wx.ID_CANCEL:
               return None
            elif self.function is None:
               if function is None:
                  return tuple(self.get_args())
               else:
                  return function + self.get_args_string()
            else:
               ret = self.function + self.get_args_string()
               self.window.Destroy()
               return ret 
         except ArgInvalidException, e:
            gui_util.message(str(e))
         #except Exception:
         #   throw
         else:
            break
      self.window.Destroy()

class _NumericValidator(wx.PyValidator):
   def __init__(self, name="Float entry box ", range=None):
      wx.PyValidator.__init__(self)
      self.rng = range
      self.name = name
      wx.EVT_CHAR(self, self.OnChar)

   def Clone(self):
      return self.__class__(self.name, self.rng)

   def show_error(self, s):
      dlg = wx.MessageDialog(
         self.GetWindow(), s,
         "Dialog Error",
         wx.OK | wx.ICON_ERROR)
      dlg.ShowModal()
      dlg.Destroy()

   def Validate(self, win):
      tc = self.GetWindow()
      val = str(tc.GetValue())
      if val == "None":
          pass
      else:
          for x in val:
             if x not in self._digits:
                self.show_error(self.name + " must be numeric.")
                return False
          try:
             val = self._type(val)
          except:
             self.show_error(self.caption + " is invalid.")
             return False
          if self.rng:
             if val < self.rng[0] or val > self.rng[1]:
                self.show_error(
                   self.name + " must be in the range " + str(self.rng) + ".")
                return False
      return True

   def OnChar(self, event):
      key = event.GetKeyCode()

      if (key < wx.WXK_SPACE or
          key == wx.WXK_DELETE or key > 255):
         event.Skip()
         return
      if chr(key) in self._digits:
         event.Skip()
         return
      if not wx.Validator_IsSilent():
         wx.Bell()

   def TransferToWindow(self):
      return True

   def TransferFromWindow(self):
      return True

class _IntValidator(_NumericValidator):
   _digits = string.digits + "-"
   _type = int

class Int:
   def get_control(self, parent, locals=None):
      self.control = wx.TextCtrl(parent, -1, str(self.default))
      self.control.SetValidator(_IntValidator(name = self.name, range = self.rng))
      return self

   def get(self):
      return int(self.control.GetValue())

   def get_string(self):
      return str(self.control.GetValue())

class _RealValidator(_NumericValidator):
   _digits = string.digits + "-."
   _type = float

class Real:
   def get_control(self, parent, locals=None):
      self.control = wx.TextCtrl(
         parent, -1, str(self.default),
         validator=_RealValidator(name=self.name, range=self.rng))
      return self

   def get(self):
      return float(self.control.GetValue())

   def get_string(self):
      return str(self.control.GetValue())

class String:
   def get_control(self, parent, locals=None):
      self.control = wx.TextCtrl(parent, -1, str(self.default))
      return self

   def get(self):
      return self.control.GetValue()

   def get_string(self):
      return "r'" + self.control.GetValue() + "'"

class Class:
   def determine_choices(self, locals):
      self.locals = locals
      if self.klass is None:
         choices = locals.keys()
      else:
         choices = []
         if self.list_of:
            for key, val in locals.items():
               try:
                  it = iter(val)
               except:
                  pass
               else:
                  good = True
                  try:
                     for x in val:
                        if not isinstance(x, self.klass):
                           good = False
                           break
                  except:
                     pass
                  else:
                     if good:
                        choices.append(key)
         else:
            for key, val in locals.items():
               if isinstance(val, self.klass):
                  choices.append(key)
      if isinstance(self.default, CNoneDefault):
         choices = ["None"] + choices
      return choices

   def get_control(self, parent, locals=None):
      if util.is_string_or_unicode(self.klass):
         self.klass = eval(self.klass)
      self.control = wx.Choice(
         parent, -1, choices = self.determine_choices(locals))
      #self.control.SetSelection(0)
      return self

   def get(self):
      try:
         return self.locals[self.control.GetStringSelection()]
      except:
         if self.list_of:
            return []
         else:
            return None

   def get_string(self):
      try:
         return self.control.GetStringSelection()
      except:
         if self.list_of:
            return []
         else:
            return None

class _Vector(Class):
   def get_control(self, parent, locals=None):
      if util.is_string_or_unicode(self.klass):
         self.klass = eval(self.klass)
      self.choices = self.determine_choices(locals)
      self.control = wx.ComboBox( 
        parent, -1, str(self.default), choices=self.choices, style=wx.CB_DROPDOWN)
      return self

   def get(self):
      value = self.control.GetValue()
      if value in self.choices:
         return self.locals[self.control.GetStringSelection()]
      else:
         try:
            x = eval(value)
         except SyntaxError:
            raise ArgInvalidException("Syntax error in '%s'.  Must be a %s" % (value, self.__class__.__name__))
         if not self.is_vector(x):
            raise ArgInvalidException("Argument '%s' must be a %s" % (value, self.__class__.__name__))
         return x

   def get_string(self):
      value = self.control.GetValue()
      if value in self.choices:
         return value
      else:
         try:
            x = eval(value)
         except SyntaxError:
            raise ArgInvalidException("Syntax error in '%s'.  Must be a %s" % (value, self.__class__.__name__))
         if not self.is_vector(x):
            raise ArgInvalidException("Argument '%s' must be a %s" % (value, self.__class__.__name__))
         return value

   def is_vector(self, val):
      try:
         it = iter(val)
      except:
         return False
      else:
         good = True
         try:
            for x in val:
               if not isinstance(x, self.klass):
                  good = False
                  break
         except:
            return False
         else:
            return good

   def determine_choices(self, locals):
      self.locals = locals
      choices = []
      for key, val in locals.items():
         if (isinstance(val, array.array) and
             val.typecode == self.typecode):
            choices.append(key)
         else:
            if self.is_vector(val):
               choices.append(key)
      return choices

class ImageType(Class):
   def determine_choices(self, locals):
      from gamera import core
      choices = []
      self.locals = locals
      if locals:
         if self.list_of:
            for key, val in locals.items():
               try:
                  it = iter(val)
               except:
                  pass
               else:
                  good = True
                  try:
                     for x in val:
                        if (not isinstance(x, core.ImageBase) or
                            not val.data.pixel_type in self.pixel_types):
                           good = False
                           break
                  except:
                     pass
                  else:
                     if good:
                        choices.append(key)
         else:
            for key, val in locals.items():
               if isinstance(val, core.ImageBase) and val.data.pixel_type in self.pixel_types:
                  choices.append(key)
      if self.has_default and isinstance(self.default,CNoneDefault):
          choices = ["None"] + choices
      return choices

class Rect(Class):
   pass

class Choice:
   def get_control(self, parent, locals=None):
      choices = []
      for choice in self.choices:
         if len(choice) == 2 and not util.is_string_or_unicode(choice):
            choices.append(choice[0])
         else:
            choices.append(choice)
      self.control = wx.Choice(parent, -1, choices=choices)
      if self.default < 0:
         self.default = len(choices) + self.default
      if self.default >= 0 and self.default < len(self.choices):
         self.control.SetSelection(self.default)
      return self

   def get_string(self):
      selection = self.control.GetSelection()
      if (len(self.choices[selection]) == 2 and
          not util.is_string_or_unicode(self.choices[selection])):
         return str(self.choices[selection][1])
      else:
         return str(selection)

   def get(self):
      selection = self.control.GetSelection()
      if (len(self.choices[selection]) == 2 and
          not util.is_string_or_unicode(self.choices[selection])):
         return self.choices[selection][1]
      else:
         return int(selection)

class ChoiceString:
   def get_control(self, parent, locals=None):
      if self.strict:
         self.control = wx.Choice(parent, -1, choices=self.choices)
         default_index = self.choices.index(self.default)
         self.control.SetSelection(default_index)
      else:
         if self.has_default:
            default = self.default
         else:
            default = self.choices[0]
         self.control = wx.ComboBox( 
            parent, -1, default, choices=self.choices, style=wx.CB_DROPDOWN)
      return self

   def get_string(self):
      return repr(self.get())

   def get(self):
      if self.strict:
         selection = self.control.GetStringSelection()
      else:
         selection = self.control.GetValue()
      return selection

class _Filename:
   def get_control(self, parent, locals=None, text=None):
      if text is None:
         text = self.default
      self.control = wx.BoxSizer(wx.HORIZONTAL)
      self.text = wx.TextCtrl(parent, -1, text, size=wx.Size(200, 24))
      browseID = wx.NewId()
      if wx.Platform == '__WXMAC__' and hasattr(buttons, 'ThemedGenButton'):
         browse = buttons.ThemedGenButton(
            parent, browseID, "...", size=wx.Size(24, 24))
      else:
         browse = wx.Button(
            parent, browseID, "...", size=wx.Size(24, 24))
      wx.EVT_BUTTON(browse, browseID, self.OnBrowse)
      self.control.Add(self.text, 1, wx.EXPAND)
      self.control.Add((4, 4), 0)
      self.control.Add(browse, 0)
      return self

   def get_string(self):
      text = self.text.GetValue()
      if text == "":
         return "None"
      else:
         return "r'" + text + "'"

   def get(self):
      text = self.text.GetValue()
      try:
         textutf8 = text.encode('utf8')
         text = textutf8
      except:
         pass
      if text == "":
         return None
      else:
         return str(text)

class FileOpen(_Filename):
   def OnBrowse(self, event):
      parent = self.text.GetParent()
      filename = gui_util.open_file_dialog(parent, self.extension)
      if filename:
         self.text.SetValue(filename)
      parent.Raise()
      while wx.IsBusy():
         wx.EndBusyCursor()

   def get(self):
      while 1:
         text = self.text.GetValue()
         if not os.path.exists(os.path.abspath(text)):
            gui_util.message("File '%s' does not exist." % text)
            self.OnBrowse(None)
         else:
            break
      return _Filename.get(self)

   def get_string(self):
      while 1:
         text = self.text.GetValue()
         if not os.path.exists(os.path.abspath(text)):
            gui_util.message("File '%s' does not exist." % text)
            self.OnBrowse(None)
         else:
            break
      return _Filename.get_string(self)

class FileSave(_Filename):
   def OnBrowse(self, event):
      parent = self.text.GetParent()
      filename = gui_util.save_file_dialog(parent, self.extension)
      if filename:
         self.text.SetValue(filename)
      parent.Raise()
      while wx.IsBusy():
         wx.EndBusyCursor()

class Directory(_Filename):
   def OnBrowse(self, event):
      parent = self.text.GetParent()
      filename = gui_util.directory_dialog(self.text)
      if filename:
         self.text.SetValue(filename)
      parent.Raise()
      while wx.IsBusy():
         wx.EndBusyCursor()

   def get(self):
      while 1:
         text = self.text.GetValue()
         if not os.path.exists(os.path.abspath(text)):
            gui_util.message("File '%s' does not exist." % text)
            self.OnBrowse(None)
         else:
            break
      return _Filename.get(self)

   def get_string(self):
      while 1:
         text = self.text.GetValue()
         if not os.path.exists(os.path.abspath(text)):
            gui_util.message("File '%s' does not exist." % text)
            self.OnBrowse(None)
         else:
            break
      return _Filename.get_string(self)

class Radio:
   def get_control(self, parent, locals=None):
      self.control = wx.RadioButton(parent, -1, self.radio_button)
      return self

   def get_string(self):
      return str(self.control.GetValue())

   def get(self):
      return self.control.GetValue()

class Check:
   def get_control(self, parent, locals=None):
      self.control = wx.CheckBox(parent, -1, self.check_box)
      self.control.Enable(self.enabled)
      self.control.SetValue(self.default)
      return self

   def get_string(self):
      return str(self.control.GetValue())

   def get(self):
      return self.control.GetValue()

class Region(Class):
   def determine_choices(self, locals):
      from gamera import core
      self.klass = core.Region
      return Class.determine_choices(self, locals)

class RegionMap(Class):
   def determine_choices(self, locals):
      from gamera import core
      self.klass = core.RegionMap
      return Class.determine_choices(self, locals)

class ImageInfo(Class):
   def determine_choices(self, locals):
      from gamera import core
      self.klass = core.ImageInfo
      return Class.determine_choices(self, locals)

class ImageList(Class):
   def determine_choices(self, locals):
      from gamera import core
      self.klass = core.ImageBase
      return Class.determine_choices(self, locals)

class Info:
   def get_control(self, parent, locals=None):
      self.control = wx.StaticText(parent, -1, "")
      return self

   def get_string(self):
      return None
   get = get_string

class Pixel(_Filename):
   def get_control(self, parent, locals=None):
      if type(self.default) == RGBPixel:
         text = "RGBPixel(%d, %d, %d)" % (self.default.red,
                                          self.default.green,
                                          self.default.blue)
      else:
         text = str(self.default)
      return _Filename.get_control(self, parent, locals, text)

   def OnBrowse(self, event):
      dialog = wx.ColourDialog(None)
      if dialog.ShowModal() == wx.ID_OK:
         color = dialog.GetColourData().GetColour()
         self.text.SetValue("RGBPixel(%d, %d, %d)" % (color.Red(), color.Green(), color.Blue()))
      self.text.GetParent().Raise()

   def get_string(self):
      text = self.text.GetValue()
      if text == "":
         return "None"
      else:
         return text

   def get(self):
      text = self.text.GetValue()
      if text == "":
         return None
      else:
         return eval(text, {'RGBPixel': RGBPixel})

class PointVector:
   def determine_choices(self, locals):
      from gamera import core
      self.klass = core.Point
      return Class.determine_choices(self, locals)

class Point:
   def get_control(self, parent, locals=None):
      from gamera.core import Point
      default = Point(self.default)
      self.control = wx.BoxSizer(wx.HORIZONTAL)
      self.control.Add(wx.StaticText(parent, -1, "x:"))
      self.control_x = wx.SpinCtrl(
         parent, -1, value=str(default.x),
         min=-DEFAULT_MAX_ARG_NUMBER, max=DEFAULT_MAX_ARG_NUMBER,
         initial=default.x)
      self.control_x.SetValidator(_IntValidator(name=self.name))
      self.control.Add(self.control_x, 1, wx.EXPAND | wx.LEFT | wx.RIGHT, 5)
      self.control.Add(wx.StaticText(parent, -1, "y:"))
      self.control_y = wx.SpinCtrl(
         parent, -1, value=str(default.y),
         min=-DEFAULT_MAX_ARG_NUMBER, max=DEFAULT_MAX_ARG_NUMBER,
         initial=default.y)
      self.control_y.SetValidator(_IntValidator(name=self.name))
      self.control.Add(self.control_y, 1, wx.EXPAND | wx.LEFT, 5)
      return self

   def get(self):
      from gamera.core import Point
      return Point(int(self.control_x.GetValue()),
                   int(self.control_y.GetValue()))
   
   def get_string(self):
      return str(self.get())

class Dim(Point):
   def get(self):
      from gamera.core import Dim
      return Dim(int(self.control_x.GetValue()),
                   int(self.control_y.GetValue()))
   
   def get_string(self):
      return str(self.get())

class FloatPoint:
   def get_control(self, parent, locals=None):
      from gamera.core import FloatPoint
      default = FloatPoint(self.default)
      self.control = wx.BoxSizer(wx.HORIZONTAL)
      self.control.Add(wx.StaticText(parent, -1, "x:"))
      self.control_x = wx.TextCtrl(
         parent, -1, str(default.x),
         validator = _RealValidator(name=self.name))
      self.control.Add(self.control_x, 1, wx.EXPAND | wx.LEFT | wx.RIGHT, 5)
      self.control.Add(wx.StaticText(parent, -1, "y:"))
      self.control_y = wx.TextCtrl(
         parent, -1, str(default.y),
         validator = _RealValidator(name=self.name))
      self.control.Add(self.control_y, 1, wx.EXPAND | wx.LEFT, 5)
      return self

   def get(self):
      from gamera.core import FloatPoint
      return FloatPoint(float(self.control_x.GetValue()),
                        float(self.control_y.GetValue()))

   def get_string(self):
      return str(self.get())


from gamera import args
args.mixin(locals(), "GUI")
