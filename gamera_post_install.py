#!/usr/bin/env python
# -*- mode: python; indent-tabs-mode: nil; tab-width: 3 -*-
# vim: set tabstop=3 shiftwidth=3 expandtab:
#
# Copyright (C) 2003-2008 Michael Droettboom
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
"""Installs an icon on the user's Desktop when Gamera is installed."""

import sys, os.path, os

def install_desktop_icon():
  try:
    try:
      import wx
    except:
      import wxPython.wx
  except ImportError:
    print "wxPython was not found.  Please make sure that wxPython is installed before running Gamera."

  try:
    #desktop_path = get_special_folder_path("CSIDL_COMMON_DESKTOPDIRECTORY")
    desktop_path = get_special_folder_path("CSIDL_DESKTOPDIRECTORY")
  except OSError:
    print "Can't find the desktop!"
    return
  if os.path.exists(desktop_path):
    print "Creating desktop icon..."
    desktop_icon = os.path.join(desktop_path, "Gamera.py")
    fd = open(desktop_icon, 'w')
    fd.write("""
print "Loading GAMERA..."
import sys
try:
   import wxversion
except ImportError:
   print "Please ensure that wxPython is installed before running Gamera."
   print "Press <ENTER> to exit."
   x = raw_input()
   sys.exit()
try:
   from gamera.gui import gui
   gui.run()
except:
   import traceback
   print "Gamera made a fatal error:"
   print
   traceback.print_exc()
   print
   print "Press <ENTER> to exit."
   x = raw_input()
   sys.exit(1)""")
    fd.close()
    file_created(desktop_icon)
    print "done."

if sys.argv[1] == '-install':
  install_desktop_icon()
elif sys.argv[1] == '-remove':
  pass
else:
  print "This script should only be run by the Windows installer."
