from sys import stdout

# Import the Gamera core and initialise it
from gamera.core import *
init_gamera()

# Import the classifier module and the XML module
from gamera import knn, gamera_xml

# Since noninteractive classifiers can not be added to after
# they are created, we need a set of glyphs to initialize the
# classifier with.
glyphs = gamera_xml.glyphs_from_xml("training.xml")
classifier = knn.kNNNonInteractive(glyphs)

# OPTION:
# You can also create a noninteractive classifier from an
# interactive one:
#
# classifier = knn.kNNInteractive()
# classifier.from_xml("training.xml")
# classifier = classifier.noninteractive_copy()

# Set up a function that displays the results of the
# genetic algorithm as it progresses
def optimizer_callback(classifier):
   stdout.write("Leave one out performance: %02f%%\r" % (classifier.ga_best * 100.0))
   stdout.flush()
classifier.add_optimization_callback(optimizer_callback)

print "Starting optimization (press Ctrl+C to stop)..."

# Optimize until the user presses "Ctrl+C"
classifier.start_optimizing()
try:
   while 1:
      # We have to do something do get around a bug
      # in the Python optimizer
      x = 0
except KeyboardInterrupt:
   print "\nCtrl+C pressed"
except:
   pass

# When the user presses Ctrl+C *or* any other sort of
# error happens, stop optimizing and then save to disk.
print "Stopping optimization (please wait...)"
final = classifier.stop_optimizing()
print "Final leave one out performance: %.02f" % (final * 100.0)
print "Saving to disk"
classifier.serialize("training.knn")
