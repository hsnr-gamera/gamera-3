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
import keyword, re

variable_name = re.compile("^[A-Za-z_][A-Za-z0-9_]*$")
def verify_variable_name(name):
   if keyword.iskeyword(name):
      return 0
   if variable_name.match(name):
      return 1
   return 0

def get(default='untitled', dict={}):
   number = 0
   while 1:
      name = '%s%d' % (default, number)
      while dict.has_key(name):
         number = number + 1
         name = '%s%d' % (default, number)
         if number > 1000:
            number = 0
            break
      dlg = wx.TextEntryDialog(
          None,
          'Please enter a variable name for the result',
          'Select a variable name', name,
          style=wx.OK|wx.CANCEL|wx.CENTRE)
      button = dlg.ShowModal()
      if button == wx.ID_OK:
         result = dlg.GetValue()
         dlg.Destroy()
         if not verify_variable_name(result):
            gui_util.message("Invalid variable name: " + result)
            return None
         else:
            return result
      else:
         dlg.Destroy()
         return None
