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

from wxPython.wx import *   # wxPython
import keyword              # Python standard library
from gamera.core import *   # Gamera-specific
from gamera import util
from gamera.gui import var_name
from gamera.args import *
import weakref

EXECUTE_MODE = 0
HELP_MODE = 1

def set_shell(sh):
  global shell
  shell = sh

def set_shell_frame(sf):
  global shell_frame
  shell_frame = sf

######################################################################

class ImageMenu:
  _base_method_id = 10003

  def __init__(self, parent, x, y, images_, name_=None, shell_=None,
               mode=EXECUTE_MODE):
    self.shell = shell_
    self.locals = locals
    self.mode = mode
    self.parent = parent
    if not util.is_sequence(images_):
      # self.images = [weakref.proxy(images_)]
      self.images = [images_]
    else:
      # self.images = [weakref.proxy(x) for x in images_]
      self.images = images_
    self.image_name = name_

    members = self.images[0].members_for_menu()
    methods = self.images[0].methods_for_menu()
    menu = self.create_menu(members, methods,
                            self.images[0].data.pixel_type,
                            self.images[0].pixel_type_name)
    self.parent.PopupMenu(menu, wxPoint(x, y))
    for i in range(10000, self._base_method_id + len(self.functions)):
      self.parent.Disconnect(i)
    menu.Destroy()

  def __del__(self):
    print 'ImageMenu deleted'

  # Given a list of variables and methods, put it all together
  def create_menu(self, members, methods, type, type_name):
    menu = wxMenu()
    # Top line
    if self.mode == HELP_MODE:
      menu.Append(0, "Help")
    menu.Append(0, type_name + " Image")
    menu.AppendSeparator()
    menu.Append(10000, "new reference")
    EVT_MENU(self.parent, 10000, self.OnCreateReference)
    menu.Append(10001, "new copy")
    EVT_MENU(self.parent, 10001, self.OnCreateCopy)
    menu.AppendSeparator()

    info_menu = wxMenu()
    menu.AppendMenu(0, "Info", info_menu)
    # Variables
    for member in members:
      info_menu.Append(0, member)

    menu.AppendSeparator()
    # Methods
    self.functions = [None]
    menu = self.create_methods(methods, menu)
    return menu

  def create_methods(self, methods, menu):
    items = methods.items()
    items.sort()
    for key, val in items:
      if type(val) == type({}):
        item = self.create_methods(val, wxMenu())
        menu.AppendMenu(0, key, item)
      else:
        menu.Append(len(self.functions) + self._base_method_id, key)
        EVT_MENU(self.parent, len(self.functions) + self._base_method_id,
                 self.OnPopupFunction)
        self.functions.append(val)
    return menu

  def get_shell(self):
    if self.shell:
      return self.shell
    return shell

  # Creates a new top-level reference to the image
  def OnCreateReference(self, event):
    sh = self.get_shell()
    name = var_name.get("ref", sh.locals)
    if name:
      if len(self.images) == 1:
        sh.locals[name] = self.images[0]
      else:
        sh.locals[name] = []
        for i in range(len(self.images)):
          sh.locals[name].append(self.images[i])
      sh.Update()
    sh.update()

  def OnCreateCopy(self, event):
    sh = self.get_shell()
    name = var_name.get("copy", sh.locals)
    if name:
      if len(self.images) == 1:
        sh.locals[name] = self.images[0].image_copy()
      else:
        sh.locals[name] = []
        for i in range(len(self.images)):
          sh.locals[name].append(self.images[i].image_copy())
      sh.Update()
    sh.update()

  def get_function_call(self, sh, function):
    # determine if the function requires an argument gui
    if function.args.list in ('', None, (), []):
      # if not, we can just use empty parentheses
      return function.__name__ + "()"
    # else, display the argument gui and use what it returns
    return function.args.show(self.parent,
                              sh.GetLocals(),
                              function.__name__)

  def get_result_name(self, function, dict):
    if function.return_type not in ('', None):
      return var_name.get(function.return_type.name, dict)
    return ''

  def OnPopupFunction(self, event):
    sh = self.get_shell()
    function = self.functions[event.GetId() - self._base_method_id]
    if self.images:
      if self.mode == HELP_MODE:
        sh.run("help('" + function.__name__ + "')")
      elif self.mode == EXECUTE_MODE:
        func_call = self.get_function_call(sh, function)
        if func_call == None:  # User pressed cancel, so bail
          return
        result_name = self.get_result_name(function, sh.locals)
        # If there is no image name, we have to run the code locally (i.e.
        # not in the shell)
        wxBeginBusyCursor()
        if self.image_name is None:
          self._run_locally(sh, result_name, func_call)
        else:
          self._run_in_shell(sh, result_name, func_call)
        wxEndBusyCursor()
    sh.update()

  def _run_locally(self, sh, result_name, func_call):
    if len(self.images) == 1:
      namespace = {'image': self.images[0]}
      source = 'image.%s' % func_call
      print source
      result = eval(source, namespace)
      if result_name != '':
        sh.locals[result_name] = result
    else:
      namespace = {'images': self.images}
      if result_name != '':
        sh.locals[result_name] = []
      progress = util.ProgressFactory('Processing images...')
      for i in range(len(self.images)):
        source = "images[%d].%s" % (i, func_call)
        result = eval(source, namespace)
        if result_name != '':
          sh.locals[result_name].append(result)
        progress.update(i, len(self.images))
      progress.update(1, 1)
    if result_name != '':
      sh.run(result_name)

  def _run_in_shell(self, sh, result_name, func_call):
    if len(self.images) == 1:
      if result_name != '':
        source = '%s = ' % result_name
      else:
        source = ''
      source += '.'.join((self.image_name, func_call))
      sh.run(source)
    else:
      if result_name != '':
        sh.run('%s = []' % result_name)
      wxBeginBusyCursor()
      sh.run('for _ in %s:' % self.image_name)
      source = '_.%s' % (func_call)
      if result_name != '':
        source = '%s.append(%s)' % (result_name, source)
      sh.run('\t' + source)
      sh.run('\n')
      del sh.locals['_'] 
      wxEndBusyCursor()
