#!/usr/bin/env python
"""Installs an icon on the user's Desktop when Gamera is installed."""

import sys, os.path, os

def install_desktop_icon():
  try:
    desktop_path = get_special_folder_path("CSIDL_COMMON_DESKTOPDIRECTORY")
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
   import wxPython.wx
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
