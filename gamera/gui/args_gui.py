# vi:set tabsize=3:
#
# Copyright (C) 2001, 2002 Ichiro Fujinaga, Michael Droettboom,
#                          and Karl MacMillan
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

from wxPython.wx import *
import os.path
import string
from types import *
from gamera import util, enums
from gamera.gui import gui_util
from gamera.core import RGBPixel

class Args:
   def _create_controls(self, locals, parent):
      self.controls = []
      # Controls
      if util.is_sequence(self.list[0]):
         notebook = wxNotebook(parent, -1)
         for page in self.list:
            panel = wxPanel(notebook, -1)
            gs = self._create_page(locals, panel, page[1:])
            panel.SetSizer(gs)
            gs.RecalcSizes()
            notebook.AddPage(panel, page[0])
         return notebook
      else:
         return self._create_page(locals, parent, self.list)

   def _create_page(self, locals, parent, page):
      if len(page) > 15:
         sw = wxScrolledWindow(
            parent, style=wxSIMPLE_BORDER,
            size=(450, 400))
         sw.EnableScrolling(0, 1)
         sw.SetScrollRate(0, 20)
         gs = self._create_page_impl(locals, sw, page)
         sw.SetSizer(gs)
         gs.SetVirtualSizeHints(sw)
         gs.RecalcSizes()
         return sw
      else:
         return self._create_page_impl(locals, parent, page)

   def _create_page_impl(self, locals, parent, page):
      gs = wxFlexGridSizer(len(page), 2, 8, 8)
      for item in page:
         if item.name == None:
            item.name = "ERROR!  No name given!"
         gs.Add(wxStaticText(parent, -1, item.name),
                0,
                (wxTOP|wxLEFT|wxRIGHT|
                 wxEXPAND|
                 wxALIGN_CENTER_VERTICAL|
                 wxALIGN_LEFT), 10)
         control = item.get_control(parent, locals)
         self.controls.append(control)
         gs.Add(control.control,
                0,
                (wxTOP|wxLEFT|wxRIGHT|
                 wxEXPAND|
                 wxALIGN_CENTER_VERTICAL|
                 wxALIGN_RIGHT), 10)
      gs.AddGrowableCol(1)
      gs.RecalcSizes()
      return gs

   def _create_buttons(self):
      # Buttons
      buttons = wxBoxSizer(wxHORIZONTAL)
      ok = wxButton(self.window, wxID_OK, "OK")
      ok.SetDefault()
      buttons.Add(ok, 1, wxEXPAND|wxALL, 5)
      buttons.Add(wxButton(self.window,
                                       wxID_CANCEL,
                                       "Cancel"),
                  1,
                  wxEXPAND|wxALL,
                  5)
      return buttons

   def _create_wizard_buttons(self):
      # Buttons
      buttons = wxBoxSizer(wxHORIZONTAL)
      buttons.Add(wxButton(self.window,
                                       wxID_CANCEL,
                                       "< Back"),
                  1,
                  wxEXPAND|wxALL,
                  5)
      ok = wxButton(self.window,
                                wxID_OK,
                                "Next >")
      ok.SetDefault()
      buttons.Add(ok,
                  1,
                  wxEXPAND|wxALL,
                  5)
      return buttons

   # generates the dialog box
   def setup(self, parent, locals):
      self.window = wxDialog(parent, -1, self.name,
                                         style=wxCAPTION)
      self.window.SetAutoLayout(1)
      if self.wizard:
         bigbox = wxBoxSizer(wxHORIZONTAL)
         from gamera.gui import gamera_icons
         bmp = gamera_icons.getGameraWizardBitmap()
         bitmap = wxStaticBitmap(self.window, -1, bmp)
         bigbox.Add(bitmap, 0, wxALIGN_TOP)
      self.box = wxBoxSizer(wxVERTICAL)
      self.border = wxBoxSizer(wxHORIZONTAL)
      self.window.SetSizer(self.border)
      self.gs = self._create_controls(locals, self.window)
      if self.wizard:
         buttons = self._create_wizard_buttons()
      else:
         buttons = self._create_buttons()
      # Put it all together
      if self.title != None:
         static_text = wxStaticText(self.window, -1, self.title)
         font = wxFont(12,
                                   wxSWISS,
                                   wxNORMAL,
                                   wxBOLD,
                                   False,
                                   "Helvetica")
         static_text.SetFont(font)
         self.box.Add(static_text, 0,
                      wxEXPAND|wxBOTTOM, 20)
      self.box.Add(self.gs, 1,
                   wxEXPAND|wxALIGN_RIGHT)
      self.box.Add(wxPanel(self.window, -1, size=(20,20)), 0,
                   wxALIGN_RIGHT)
      self.box.Add(buttons, 0, wxALIGN_RIGHT)
      if self.wizard:
         bigbox.Add(
            self.box, 1,
            wxEXPAND|wxALL|wxALIGN_TOP,
            15)
         bigbox.RecalcSizes()
         self.border.Add(
            bigbox, 1, wxEXPAND|wxALL, 10)
      else:
         self.border.Add(
            self.box, 1, wxEXPAND|wxALL, 15)
      self.border.RecalcSizes()
      self.box.RecalcSizes()
      # self.gs.RecalcSizes()
      self.border.Layout()
      self.border.Fit(self.window)
      size = self.window.GetSize()
      self.window.SetSize((max(400, size[0]), max(200, size[1])))
      self.window.Centre()

   def show(self, parent=None, locals={}, function=None, wizard=0):
      self.wizard = wizard
      if function != None:
         self.function = function
      self.setup(parent, locals)
      result = wxDialog.ShowModal(self.window)
      self.window.Destroy()
      if result == wxID_CANCEL:
         return None
      elif self.function is None:
         if function is None:
            return tuple(self.get_args())
         else:
            return function + self.get_args_string()
      else:
         return self.function + self.get_args_string()

class Int:
   def get_control(self, parent, locals=None):
      self.control = wxSpinCtrl(
         parent, -1,
         value=str(self.default),
         min=self.rng[0], max=self.rng[1],
         initial=self.default)
      return self

   def get(self):
      return int(self.control.GetValue())

   def get_string(self):
      return str(self.control.GetValue())

class _RealValidator(wxPyValidator):
   def __init__(self, name="Float entry box ", range=None):
      wxPyValidator.__init__(self)
      self.rng = range
      self.name = name
      EVT_CHAR(self, self.OnChar)

   def Clone(self):
      return _RealValidator(self.name, self.rng)

   def show_error(self, s):
      dlg = wxMessageDialog(
         self.GetWindow(), s,
         "Dialog Error",
         wxOK | wxICON_ERROR)
      dlg.ShowModal()
      dlg.Destroy()

   def Validate(self, win):
      tc = self.GetWindow()
      val = tc.GetValue()
      for x in val:
         if x not in string.digits + "-.":
            self.show_error(self.name + " must be numeric.")
            return False
      try:
         val = float(val)
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
      key = event.KeyCode()
      if (key < WXK_SPACE or
          key == WXK_DELETE or key > 255):
         event.Skip()
         return
      if chr(key) in string.digits + "-.":
         event.Skip()
         return
      if not wxValidator_IsSilent():
         wxBell()

   def TransferToWindow(self):
      return True

   def TransferFromWindow(self):
      return True

class Real:
   def get_control(self, parent, locals=None):
      self.control = wxTextCtrl(
         parent, -1, str(self.default),
         validator=_RealValidator(name=self.name, range=self.rng))
      return self

   def get(self):
      return float(self.control.GetValue())

   def get_string(self):
      return str(self.control.GetValue())

class String:
   def get_control(self, parent, locals=None):
      self.control = wxTextCtrl(parent, -1, str(self.default))
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
         for i in locals.items():
            if ((self.list_of and isinstance(i[1], ListType) and
                 len(i[1]) and isinstance(i[1][0], self.klass)) or
                (not self.list_of and isinstance(i[1], self.klass))):
               choices.append(i[0])
      return choices

   def get_control(self, parent, locals=None):
      if util.is_string_or_unicode(self.klass):
         self.klass = eval(self.klass)
      self.control = wxChoice(
         parent, -1, choices = self.determine_choices(locals))
      return self

   def get(self):
      if self.control.Number() > 0:
         return self.locals[self.control.GetStringSelection()]
      else:
         return None

   def get_string(self):
      if self.control.Number() > 0:
         return self.control.GetStringSelection()
      else:
         return 'None'

class ImageType(Class):
   def determine_choices(self, locals):
      choices = []
      self.locals = locals
      if locals:
         for key, val in locals.items():
            if ((self.list_of and isinstance(val, ListType) and
                 len(val) and isinstance(val[0], self.klass)) or
                (not self.list_of and isinstance(val, self.klass))):
               if (self.pixel_types in enums.ALL or
                   val.data.pixel_type in self.pixel_types):
                  choices.append(key)
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
      self.control = wxChoice(parent, -1, choices=choices)
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

class _Filename:
   def get_control(self, parent, locals=None):
      self.control = wxBoxSizer(wxHORIZONTAL)
      self.text = wxTextCtrl(parent, -1, str(self.default), size=wxSize(200, 24))
      browseID = wxNewId()
      browse = wxButton(
         parent, browseID, "...", size=wxSize(24, 24))
      EVT_BUTTON(browse, browseID, self.OnBrowse)
      self.control.Add(self.text, 1, wxEXPAND)
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
      if text == "":
         return None
      else:
         return str(text)

class FileOpen(_Filename):
   def OnBrowse(self, event):
      
      filename = gui_util.open_file_dialog(self.text, self.extension)
      if filename:
         self.text.SetValue(filename)
      self.text.GetParent().Raise()

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
      filename = gui_util.save_file_dialog(self.text, self.extension)
      if filename:
         self.text.SetValue(filename)
      self.text.GetParent().Raise()

class Directory(_Filename):
   def OnBrowse(self, event):
      
      filename = gui_util.directory_dialog(self.text)
      if filename:
         self.text.SetValue(filename)
      self.text.GetParent().Raise()

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
      self.control = wxRadioButton(parent, -1, self.radio_button)
      return self

   def get_string(self):
      return str(self.control.GetValue())

   def get(self):
      return self.control.GetValue()

class Check:
   def get_control(self, parent, locals=None):
      self.control = wxCheckBox(parent, -1, self.check_box)
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
      return _guiClass.determine_choices(self, locals)

class RegionMap(Class):
   def determine_choices(self, locals):
      from gamera import core
      self.klass = core.RegionMap
      return _guiClass.determine_choices(self, locals)

class ImageInfo(Class):
   def determine_choices(self, locals):
      from gamera import core
      self.klass = core.ImageInfo
      return _guiClass.determine_choices(self, locals)

class ImageList(Class):
   def determine_choices(self, locals):
      from gamera import core
      self.klass = core.ImageBase
      return _guiClass.determine_choices(self, locals)

class Info:
   def get_control(self, parent, locals=None):
      self.control = wxStaticText(parent, -1, "")
      return self

   def get_string(self):
      return None
   get = get_string

class Pixel(_Filename):
   def OnBrowse(self, event):
      dialog = wxColourDialog(None)
      if dialog.ShowModal() == wxID_OK:
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

from gamera import args
args.mixin(locals(), "GUI")
