#!/usr/bin/env python

import sys, os.path

if sys.platform == "win32":
  desktop = os.path.expanduser("~/../Desktop")
  if os.path.exists(desktop):
    print desktop
    desktop_icon = os.path.join(desktop, "Gamera.py")
    if not os.path.exists(desktop_icon):
      print desktop_icon
      fd = open(desktop_icon, 'w')
      fd.write("from gamera.gui import gui\n")
      fd.write("gui.run()\n")
      fd.close()
