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

(_NO_GUI, _WX_GUI, _CURSES_GUI) = (0, 1, 2)

try:
   import wxPython.wx   # wxPython
   _has_gui = _WX_GUI
except:
   _has_gui = _NO_GUI
import sys, types, string   # Python standard library
import util, paths, gamera  # Gamera specific


######################################################################

# This is a "self-generating" dialog box

if _has_gui == _WX_GUI:
   class _guiArgs(wxPython.wx.wxDialog):
      def _create_controls(self, locals):
         # Controls
         self.gs = wxPython.wx.wxFlexGridSizer(len(self.list), 2, 10, 10)
         self.controls = []
         for item in self.list:
            self.gs.Add(wxPython.wx.wxStaticText(self, -1, item.name),
                        0,
                        (wxPython.wx.wxEXPAND|
                         wxPython.wx.wxALIGN_CENTER_VERTICAL|
                         wxPython.wx.wxALIGN_LEFT))
            control = item.get_control(self, locals)
            self.controls.append(control)
            self.gs.Add(control,
                        0,
                        (wxPython.wx.wxEXPAND|
                         wxPython.wx.wxALIGN_CENTER_VERTICAL|
                         wxPython.wx.wxALIGN_RIGHT))
         # Add some empties at the bottom for padding
         for i in range(2):
            self.gs.Add(wxPython.wx.wxPanel(self, -1))
         self.gs.AddGrowableCol(1)

      def _create_buttons(self):
         # Buttons
         buttons = wxPython.wx.wxBoxSizer(wxPython.wx.wxHORIZONTAL)
         ok = wxPython.wx.wxButton(self, wxPython.wx.wxID_OK, "OK")
         ok.SetDefault()
         buttons.Add(ok, 1, wxPython.wx.wxEXPAND|wxPython.wx.wxALL, 5)
         buttons.Add(wxPython.wx.wxButton(self, wxPython.wx.wxID_CANCEL, "Cancel"),
                     1,
                     wxPython.wx.wxEXPAND|wxPython.wx.wxALL,
                     5)
         help = wxPython.wx.wxButton(self, 300, "Help")
         buttons.Add(help,
                     1,
                     wxPython.wx.wxEXPAND|wxPython.wx.wxALL,
                     5)
         wxPython.wx.EVT_BUTTON(help, 300, self.OnHelp)
         return buttons

      def _create_wizard_buttons(self):
         # Buttons
         buttons = wxPython.wx.wxBoxSizer(wxPython.wx.wxHORIZONTAL)
         buttons.Add(wxPython.wx.wxButton(self, wxPython.wx.wxID_CANCEL, "< Back"),
                     1,
                     wxPython.wx.wxEXPAND|wxPython.wx.wxALL,
                     5)
         ok = wxPython.wx.wxButton(self, wxPython.wx.wxID_OK, "Next >")
         ok.SetDefault()
         buttons.Add(ok,
                     1,
                     wxPython.wx.wxEXPAND|wxPython.wx.wxALL,
                     5)
         return buttons

      # generates the dialog box
      def setup(self, parent, locals, wizard=0):
         wxPython.wx.wxDialog.__init__(self, parent, -1, self.name,
                                       style=wxPython.wx.wxRESIZE_BORDER)
         self.SetAutoLayout(wxPython.wx.true)
         if wizard:
            bigbox = wxPython.wx.wxBoxSizer(wxPython.wx.wxHORIZONTAL)
            bmp = wxPython.wx.wxBitmap(paths.pixmaps + "gamera_wizard.png",
                                       wxPython.wx.wxBITMAP_TYPE_PNG)
            bitmap = wxPython.wx.wxStaticBitmap(self, -1, bmp)
            bigbox.Add(bitmap, 0, wxPython.wx.wxALIGN_TOP)
         self.box = wxPython.wx.wxBoxSizer(wxPython.wx.wxVERTICAL)
         self.border = wxPython.wx.wxBoxSizer(wxPython.wx.wxHORIZONTAL)
         self._create_controls(locals)
         if wizard:
            buttons = self._create_wizard_buttons()
         else:
            buttons = self._create_buttons()
         # Put it all together
         if self.title != None:
            static_text = wxPython.wx.wxStaticText(self, -1, self.title)
            font = wxPython.wx.wxFont(12,
                                      wxPython.wx.wxSWISS,
                                      wxPython.wx.wxNORMAL,
                                      wxPython.wx.wxBOLD,
                                      wxPython.wx.false,
                                      "Helvetica")
            static_text.SetFont(font)
            self.box.Add(static_text, 0, wxPython.wx.wxEXPAND|wxPython.wx.wxBOTTOM, 20)
         self.box.Add(self.gs, 1, wxPython.wx.wxEXPAND|wxPython.wx.wxALIGN_RIGHT)
         self.box.Add(buttons, 0, wxPython.wx.wxALIGN_RIGHT)
         self.box.RecalcSizes()
         self.gs.RecalcSizes()
         if wizard:
            bigbox.Add(self.box,
                       1,
                       wxPython.wx.wxEXPAND|wxPython.wx.wxALL|wxPython.wx.wxALIGN_TOP,
                       15)
            self.border.Add(bigbox, 1, wxPython.wx.wxEXPAND|wxPython.wx.wxALL, 10)
         else:
            self.border.Add(self.box, 1, wxPython.wx.wxEXPAND|wxPython.wx.wxALL, 15)
         self.border.Fit(self)
         self.SetSizer(self.border)
         size = self.GetSize()
         self.SetSize((max(600, size[0]), max(350, size[1])))
         # self.Centre()  # Doesn't work well on dual-headed displays

      def show(self, parent, locals, function=None, wizard=0):
         self.wizard = wizard
         if function != None:
            self.function = function
         self.setup(parent, locals, wizard=wizard)
         result = wxPython.wx.wxDialog.ShowModal(self)
         self.Destroy()
         if result == wxPython.wx.wxID_CANCEL:
            return None
         elif self.function == None:
            return function + self.get_args()
         else:
            return self.function + self.get_args()

      def OnHelp(self, event):
         
         gamera.help(self.function)
else:
   class _guiArgs:
      def setup(self, parent, locals, wizard=0):
         raise Exception("No GUI environment available.  Cannot display dialog.")

      def show(self, parent, locals, function=None, wizard=0):
         raise Exception("No GUI environment available.  Cannot display dialog.")

class Args(_guiArgs):
   # list is a list of "Args"
   def __init__(self, list, name="Arguments", function=None, title=None):
      if not util.is_sequence(list):
         list = [list]
      for l in list:
         if not isinstance(l, Arg):
            self.valid = 0
            return
      self.valid = 1
      self.list = list
      self.name = name
      self.function = function
      self.title = title

   def __repr__(self):
      return "<" + self.__class__.__name__ + ">"
   

   def get_args(self):
      results = []
      for control in self.controls:
         res = control.get()
         if res == None:
            return ''
         results.append(control.get())
      tuple = '('
      for i in range(len(results)):
         if results[i] != None:
            if (type(results[i]) == type('') and self.wizard and
                results[i][0] != "'"):
               tuple = tuple + "'" + str(results[i]) + "'"
            else:
               tuple = tuple + str(results[i])
            if i == len(results) - 1:
               tuple = tuple + ")"
            else:
               tuple = tuple + ", "
      return tuple

   def __getitem__(self, i):
      return self.list[i]

   index = __getitem__

   def __len__(self, i):
      return len(self.list)

   

######################################################################

# ARGUMENT TYPES

class Arg:
   default = 0

   def __repr__(self):
      return "<" + self.__class__.__name__ + ">"
   
# Integer
if _has_gui == _WX_GUI:
   class _guiInt(wxPython.wx.wxSpinCtrl):
      def get_control(self, parent, locals=None):
         wxPython.wx.wxSpinCtrl.__init__(self, parent, -1, value=str(self.default),
                             min=self.rng[0], max=self.rng[1],
                             initial=self.default)
         return self

      def get(self):
         return self.GetValue()
else:
   class _guiInt:
      pass

class Int(_guiInt, Arg):
   def __init__(self, name, range=(-sys.maxint, sys.maxint), default=0):
      self.name = name
      self.rng = range
      self.default = default

# Real / Float
if _has_gui == _WX_GUI:
   class _RealValidator(wxPython.wx.wxPyValidator):
      def __init__(self, name="Float entry box ", range=None):
         wxPython.wx.wxPyValidator.__init__(self)
         self.rng = range
         self.name = name
         wxPython.wx.EVT_CHAR(self, self.OnChar)

      def Clone(self):
         return _RealValidator(self.name, self.rng)

      def show_error(self, s):
         dlg = wxPython.wx.wxMessageDialog(self.GetWindow(), s,
                               "Dialog Error", wxPython.wx.wxOK | wxPython.wx.wxICON_ERROR)
         dlg.ShowModal()
         dlg.Destroy()

      def Validate(self, win):
         tc = self.GetWindow()
         val = tc.GetValue()
         for x in val:
            if x not in string.digits + "-.":
               self.show_error(self.name + " must be numeric.")
               return wxPython.wx.false
         try:
            val = float(val)
         except:
            self.show_error(self.caption + " is invalid.")
            return wxPython.wx.false
         if self.rng:
            if val < self.rng[0] or val > self.rng[1]:
               self.show_error(self.name + " must be in the range " + str(self.rng) + ".")
               return wxPython.wx.false
         return wxPython.wx.true

      def OnChar(self, event):
         key = event.KeyCode()
         if key < WXPYTHON.WX.WXK_SPACE or key == WXPYTHON.WX.WXK_DELETE or key > 255:
            event.Skip()
            return
         if chr(key) in string.digits + "-.":
            event.Skip()
            return
         if not wxPython.wx.wxValidator_IsSilent():
            wxPython.wx.wxBell()

      def TransferToWindow(self):
         return wxPython.wx.true

      def TransferFromWindow(self):
         return wxPython.wx.true

   class _guiReal(wxPython.wx.wxTextCtrl):
      def get_control(self, parent, locals=None):
         wxPython.wx.wxTextCtrl.__init__(self, parent, -1, str(self.default),
                             validator=_RealValidator(name=self.name, range=self.rng))
         return self

      def get(self):
         return self.GetValue()
else:
   class _guiReal:
      pass

class Real(_guiReal, Arg):
   def __init__(self, name, range=(-sys.maxint, sys.maxint), default=0):
      self.name = name
      self.rng = range
      self.default = default

Float = Real

# String
if _has_gui == _WX_GUI:
   class _guiString(wxPython.wx.wxTextCtrl):
      def get_control(self, parent, locals=None):
         wxPython.wx.wxTextCtrl.__init__(self, parent, -1, str(self.default))
         return self

      def get(self):
         return "'" + self.GetString() + "'"
else:
   class _guiString:
      pass

class String(_guiString, Arg):
   def __init__(self, name, default=''):
      self.name = name
      self.default = default

# Class (a drop-down list of instances of a given class in a given namespace)
if _has_gui == _WX_GUI:
   class _guiClass(wxPython.wx.wxChoice):
      def determine_choices(self, locals):
         choices = []
         self.locals = locals
         if locals:
            for i in locals.items():
               if ((self.list_of and isinstance(i[1], type([])) and
                    len(i[1]) and isinstance(i[1][0], self.klass)) or
                   (not self.list_of and isinstance(i[1], self.klass))):
                  choices.append(i[0])
         return choices

      def get_control(self, parent, locals=None):
         if type(self.klass) == type(''):
            self.klass = eval(self.klass)
         wxPython.wx.wxChoice.__init__(self, parent, -1, choices = self.determine_choices(locals))
         return self

      def get(self):
         if self.Number() > 0:
            return self.GetStringSelection()
         else:
            return 'None'
else:
   class _guiClass:
      pass

class Class(_guiClass, Arg):
   def __init__(self, name, klass, list_of = 0):
      self.name = name
      self.klass = klass
      self.list_of = list_of

# Image (a drop-down list of instances of a given class in a given namespace)
if _has_gui == _WX_GUI:
   class _guiImage(_guiClass):
      def determine_choices(self, locals):
         choices = []
         self.locals = locals
         if locals:
            for i in locals.items():
               if ((self.list_of and isinstance(i[1], type([])) and
                    len(i[1]) and isinstance(i[1][0], self.klass)) or
                   (not self.list_of and isinstance(i[1], self.klass))):
                  if i.get_type() in self.pixel_types:
                     choices.append(i[0])
         return choices
else:
   class _guiImage(_guiClass):
      pass

class ImageType(_guiImage, Arg):
   def __init__(self, pixel_types, name="self", list_of = 0):
      import gamera
      self.name = name
      # self.klass = gamera.Image
      if not util.is_sequence(pixel_types):
         pixel_types = (pixel_types,)
      self.pixel_types = pixel_types
      self.list_of = list_of

# Choice
if _has_gui == _WX_GUI:
   class _guiChoice(wxPython.wx.wxChoice):
      def get_control(self, parent, locals=None):
         choices = []
         for choice in self.choices:
            if len(choice) == 2 and type(choice) != type(''):
               choices.append(choice[0])
            else:
               choices.append(choice)
         wxPython.wx.wxChoice.__init__(self, parent, -1, choices=choices)
         if self.default < 0:
            self.default = len(choices) + self.default
         if self.default >= 0 and self.default < len(self.choices):
            self.SetSelection(self.default)
         return self

      def get(self):
         if self.Number() > 0:
            selection = self.GetSelection()
            if (len(self.choices[selection]) == 2 and
                type(self.choices[selection]) != type('')):
               return self.choices[selection][1]
            else:
               return selection
         else:
            return 'None'
else:
   class _guiChoice:
      pass

class Choice(_guiChoice, Arg):
   def __init__(self, name, choices, default=0):
      self.name = name
      self.choices = choices
      self.default = default

# Filename
if _has_gui == _WX_GUI:
   class _guiFilename(wxPython.wx.wxBoxSizer):
      def get_control(self, parent, locals=None):
         wxPython.wx.wxBoxSizer.__init__(self, wxPython.wx.wxHORIZONTAL)
         self.text = wxPython.wx.wxTextCtrl(parent,
                                            -1,
                                            str(self.default),
                                            size=wxPython.wx.wxSize(200, 20))
         browseID = wxPython.wx.wxNewId()
         browse = wxPython.wx.wxButton(parent, browseID, "...", size=wxPython.wx.wxSize(20, 20))
         wxPython.wx.EVT_BUTTON(browse, browseID, self.OnBrowse)
         self.Add(self.text, 1, wxPython.wx.wxEXPAND)
         self.Add(browse, 0)
         return self

      def get(self):
         text = self.text.GetValue()
         if text == "":
            return "None"
         else:
            return "'" + self.text.GetValue() + "'"
else:
   class _guiFilename:
      pass

class _Filename(_guiFilename, Arg):
   def __init__(self, name, default="", extension="*.*"):
      self.name = name
      self.default = default
      self.extension = extension

if _has_gui == _WX_GUI:
   class FileOpen(_Filename):
      def OnBrowse(self, event):
         dlg = wxPython.wx.wxFileDialog(None,
                                        "Choose a file",
                                        ".", "",
                                        self.extension,
                                        wxPython.wx.wxOPEN)
         if dlg.ShowModal() == wxPython.wx.wxID_OK:
            filename = dlg.GetPath()
            dlg.Destroy()
            self.text.SetValue(filename)
         self.text.GetParent().Raise()

   class FileSave(_Filename):
      def OnBrowse(self, event):
         dlg = wxPython.wx.wxFileDialog(None,
                                        "Choose a file",
                                        ".", "",
                                        self.extension,
                                        wxPython.wx.wxSAVE)
         if dlg.ShowModal() == wxPython.wx.wxID_OK:
            filename = dlg.GetPath()
            dlg.Destroy()
            self.text.SetValue(filename)
         self.text.GetParent().Raise()
else:
   class FileOpen(Filename):
      pass
   class FileSave(Filename):
      pass

# Radio Buttons
if _has_gui == _WX_GUI:
   class _guiRadio(wxPython.wx.wxRadioButton):
      def get_control(self, parent, locals=None):
         wxPython.wx.wxRadioButton.__init__(self, parent, -1, self.radio_button)
         return self

      def get(self):
         return self.GetValue()
else:
   class _guiRadio:
      pass

class Radio(_guiRadio, Arg):
   def __init__(self, name, radio_button):
      self.name = name
      self.radio_button = radio_button

# Check Buttons
if _has_gui == _WX_GUI:
   class _guiCheck(wxPython.wx.wxCheckBox):
      def get_control(self, parent, locals=None):
         wxPython.wx.wxCheckBox.__init__(self, parent, -1, self.check_box)
         self.SetValue(self.default)
         return self

      def get(self):
         return self.GetValue()
else:
   class _guiCheck:
      pass
      
class Check(_guiCheck, Arg):
   def __init__(self, name, check_box, default=0):
      self.name = name
      self.check_box = check_box
      self.default = default

# Info
if _has_gui == _WX_GUI:
   class _guiInfo(wxPython.wx.wxStaticText):
      def get_control(self, parent, locals=None):
         wxPython.wx.wxStaticText.__init__(self, parent, -1, "")
         return self

      def get(self):
         return None
else:
   class _guiInfo:
      pass

class Info(_guiInfo, Arg):
   def __init__(self, name):
      self.name = name

class Wizard:
   def show(self, dialog):
      dialog_history = ['start']
      next_dialog = dialog
      while next_dialog != None:
         if next_dialog == 'start':
            return
         result = next_dialog.show(self.parent, self.locals, wizard=1)
         if result != None:
            dialog_history.append(next_dialog)
            self.locals['self'] = self
            next_dialog = eval("self." + result, {}, self.locals)
         else:
            next_dialog = dialog_history[-1]
            dialog_history = dialog_history[0:-1]
      self.done()

class ProgressDialog:
   def __init__(self, caption):
      self.caption = caption
      self.dialog = wxPython.wx.wxProgressDialog("Progress", caption, 100)
   
   def Update(self, value):
      if self.dialog == None:
         self.dialog = wxPython.wx.wxProgressDialog("Progress", self.caption, 100)
      self.dialog.Update(value)
      if value == 100 and self.dialog != None:
         self.dialog.Destroy()
         self.dialog = None

   def Destroy(self):
      self.dialog.Destroy()
      self.dialog = None
         
