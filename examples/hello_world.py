import sys

from gamera.core import *
init_gamera()

# Load filename specified on the command line.
# sys.argv is the Pythonic way to access commandline arguments.
# load_image is a Gamera function to load a TIFF or PNG file.
image = load_image(sys.argv[-1])

# The variable 'image' now is a reference to the image

# Convert the image to onebit using `otsu_threshold`
onebit = image.otsu_threshold()

# Save the result to a PNG file
onebit.save_PNG("output.png")
