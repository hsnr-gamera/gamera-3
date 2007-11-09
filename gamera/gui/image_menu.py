# -*- mode: python; indent-tabs-mode: nil; tab-width: 3 -*-
# vim: set tabstop=3 shiftwidth=3 expandtab:
#
# Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom,
#                         and Karl MacMillan
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

import wx
import weakref              # Python standard library
from gamera.core import *   # Gamera-specific
from gamera import util, plugin
from gamera.gui import var_name, gui_util
from gamera.args import *

EXECUTE_MODE = 0
HELP_MODE = 1

def set_shell(sh):
   global shell
   shell = sh

def set_shell_frame(sf):
   global shell_frame
   shell_frame = sf

######################################################################

_members_for_menu = ('pixel_type_name',
                     'storage_format_name',
                     'ul_x', 'ul_y', 'nrows', 'ncols',
                     'resolution', 'memory_size', 'label', 
                     'classification_state', 'properties')
def members_for_menu(self):
   return ["%s: %s" % (x, getattr(self, x))
           for x in _members_for_menu
           if hasattr(self, x)]

def methods_for_menu(self):
   return plugin.plugin_methods[self.data.pixel_type]

class ImageMenu:
   _base_method_id = 10003

   def __init__(self, parent, x, y, images_, name_=None, shell_=None,
                mode=EXECUTE_MODE, extra_methods={}):
      self.shell = shell_
      self.locals = locals
      self.mode = mode
      self.parent = parent
      if not util.is_sequence(images_):
         self.images = [images_]
      else:
         self.images = images_
      if not util.is_homogeneous_image_list(self.images):
         gui_util.message("All selected images are not of the same type.")
      self.image_name = name_

      self._menu_ids = []
      members = members_for_menu(self.images[0])
      methods = methods_for_menu(self.images[0])
      menu = self.create_menu(
        members, methods,
        self.images[0].data.pixel_type,
        self.images[0].pixel_type_name,
        extra_methods)
      self.did_something = 0
      self.parent.PopupMenu(menu, wx.Point(x, y))
      for i in self._menu_ids:
         self.parent.Disconnect(i)
      menu.Destroy()

   def get_id(self):
      id = wx.NewId()
      self._menu_ids.append(id)
      return id

##   def __del__(self):
##     print 'ImageMenu deleted'

   # Given a list of variables and methods, put it all together
   def create_menu(self, members, methods, type, type_name, extra_methods):
      menu = wx.Menu()
      # Top line
      if self.mode == HELP_MODE:
         menu.Append(0, "Help")
      id = self.get_id()
      menu.Append(id, type_name + " Image")
      menu.AppendSeparator()
      id = self.get_id()
      menu.Append(id, "new reference")
      wx.EVT_MENU(self.parent, id, self.OnCreateReference)
      id = self.get_id()
      menu.Append(id, "new copy")
      wx.EVT_MENU(self.parent, id, self.OnCreateCopy)
      id = self.get_id()
      menu.Append(id, "delete image")
      wx.EVT_MENU(self.parent, id, self.OnDeleteImage)
      menu.AppendSeparator()

      info_menu = wx.Menu()
      id = self.get_id()
      menu.AppendMenu(id, "Info", info_menu)
      # Variables
      for member in members:
         id = self.get_id()
         info_menu.Append(id, member)

      # Methods
      menu.AppendSeparator()
      self.functions = {}
      menu = self.create_methods(methods, menu)

      # Extra methods
      if len(extra_methods):
         menu.AppendSeparator()
         menu = self.create_extra_methods(extra_methods, menu)

      return menu

   def create_methods(self, methods, menu):
      items = methods.items()
      items.sort()
      for key, val in items:
         if type(val) == dict:
            item = self.create_methods(val, wx.Menu())
            id = self.get_id()
            menu.AppendMenu(id, key, item)
         else:
            id = self.get_id()
            menu.Append(id, key)
            wx.EVT_MENU(self.parent, id, self.OnPopupFunction)
            self.functions[id] = val
      return menu

   def create_extra_methods(self, methods, menu):
      items = methods.items()
      items.sort()
      for key, val in items:
         if type(val) == dict:
            id = self.get_id()
            item = self.create_extra_methods(val, wx.Menu())
            menu.AppendMenu(id, key, item)
         else:
            id = self.get_id()
            menu.Append(id, key)
            wx.EVT_MENU(self.parent, id, val)
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
            if isinstance(self.images[0], weakref.ProxyTypes):
               sh.locals[name] = self.images[0].image_copy()
            else:
               sh.locals[name] = self.images[0]
         else:
            sh.locals[name] = []
            for image in self.images:
               if isinstance(image, weakref.ProxyTypes):
                  sh.locals[name].append(image.image_copy())
               else:
                  sh.locals[name].append(image)
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

   def OnDeleteImage(self,event):
       sh = self.get_shell()
       sh.run("del " + self.image_name)
       sh.Update()
   
   def get_function_call(self, sh, function):
      # determine if the function requires an argument gui
      if function.args.list in ('', None, (), []):
         # if not, we can just use empty parentheses
         return function.__name__ + "()"
      # else, display the argument gui and use what it returns
      result = function.args.show(
         self.parent, sh.GetLocals(),
         function.__name__, docstring=function.__call__.__doc__)
      return result

   def get_result_name(self, function, dict):
      if function.return_type not in ('', None):
         if function.return_type.name != None:
            name = function.return_type.name
         else:
            name = function.__name__
         return var_name.get(name, dict)
      return ''

   def OnPopupFunction(self, event):
      sh = self.get_shell()
      function = self.functions[event.GetId()]
      if self.images:
         if self.mode == HELP_MODE:
            sh.run("help('" + function.__name__ + "')")
         elif self.mode == EXECUTE_MODE:
            func_call = self.get_function_call(sh, function)
            if func_call == None:  # User pressed cancel, so bail
               return
            result_name = self.get_result_name(function, sh.locals)
            if result_name is None: return
            # If there is no image name, we have to run the code locally (i.e.
            # not in the shell)
            wx.BeginBusyCursor()
            try:
               if self.image_name is None:
                  self._run_locally(sh, result_name, func_call)
               else:
                  self._run_in_shell(sh, result_name, func_call)
            finally:
               wx.EndBusyCursor()
      self.did_something = 1
      sh.update()

   def _run_locally(self, sh, result_name, func_call):
      namespace = {}
      namespace.update(sh.locals)
      if len(self.images) == 1:
         namespace['image'] = self.images[0]
         source = 'image.%s' % func_call
         result = eval(source, namespace)
         if result_name != '':
            sh.locals[result_name] = result
      else:
         namespace['images'] = self.images
         if result_name != '':
            sh.locals[result_name] = []
         progress = util.ProgressFactory(
           'Processing images...', len(self.images))
         try:
            for i in range(len(self.images)):
               source = "images[%d].%s" % (i, func_call)
               result = eval(source, namespace)
               if result_name != '':
                  sh.locals[result_name].append(result)
               progress.step()
         finally:
            progress.kill()
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
         sh.run('for i in range(len(%s)):' % self.image_name)
         source = '%s[i].%s' % (self.image_name, func_call)
         if result_name != '':
            source = '%s.append(%s)' % (result_name, source)
         sh.run('\t' + source)
         sh.run('\n')
