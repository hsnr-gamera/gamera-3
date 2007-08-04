
print "Loading GAMERA..."
import sys
try:
   try:
      import wxversion
      wxversion.select(["2.8", "2.6", "2.4"])
      import wx
   except:
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
   sys.exit(1)
