import sys

def my_application():
   image = load_image(sys.argv[-1])
   onebit = image.otsu_threshold()

   # Display the image in a window
   onebit.display()	       

   # Assign it do a global variable so the window doesn't
   # disappear
   global onebit_global
   onebit_global = onebit

# Import the Gamera core and initialise it
from gamera.core import *
init_gamera()

# Import the Gamera GUI and start it   
from gamera.gui import gui
gui.run(my_application)
