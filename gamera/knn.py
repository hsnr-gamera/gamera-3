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

from threading import *

import gamera.knncore
import gamera.gamera_xml
import array

from gamera.knncore import CITY_BLOCK
from gamera.knncore import EUCLIDEAN
from gamera.knncore import FAST_EUCLIDEAN

_distance_type_to_name = {
    CITY_BLOCK: "CITY-BLOCK",
    EUCLIDEAN: "EUCLIDEAN",
    FAST_EUCLIDEAN: "FAST-EUCLIDEAN" }

_distance_type_to_number = {
    "CITY-BLOCK": CITY_BLOCK,
    "EUCLIDEAN": EUCLIDEAN,
    "FAST-EUCLIDEAN": FAST_EUCLIDEAN }


class GaWorker(Thread):
    def __init__(self, knn):
        Thread.__init__(self)
        self.knn = knn

    def run(self):
        self.ga_initial = self.knn._ga_create()
        self.ga_best = self.ga_initial
        while(1):
            if self.knn.ga_worker_stop:
                return
            self.knn.ga_best = self.knn._ga_step()
            self.knn.ga_generation += 1
            for x in self.knn.ga_callbacks:
                x(self.knn)

class _KnnLoadXML(gamera.gamera_xml.LoadXML):
    def __init__(self):
        gamera.gamera_xml.LoadXML.__init__(self)

    def _setup_handlers(self):
        self.feature_functions = []
        self.weights = array.array('d')
        self.num_k = None
        self.distance_type = None
        self.ga_mutation = None
        self.ga_crossover = None
        self.ga_population = None
        self.add_start_element_handler('num-k', self._ths_num_k)
        self.add_start_element_handler('distance-type', self._ths_distance_type)
        self.add_start_element_handler('ga-mutation', self._ths_ga_mutation)
        self.add_start_element_handler('ga-crossover', self._ths_ga_crossover)
        self.add_start_element_handler('ga-population', self._ths_ga_population)
        
    def _remove_handlers(self):
        self.remove_start_element_handler('num-k')
        self.remove_start_element_handler('distance-type')
        self.remove_start_element_handler('ga-mutation')
        self.remove_start_element_handler('ga-crossover')
        self.remove_start_element_handler('ga-population')

    def _ths_num_k(self, a):
        self.num_k = self.try_type_convert(a, 'value', int, 'num-k')

    def _ths_distance_type(self, a):
        self.distance_type = \
          _distance_type_to_number[self.try_type_convert(a, 'value', unicode, 'distance-type')]

    def _ths_ga_mutation(self, a):
        self.ga_mutation = self.try_type_convert(a, 'value', float, 'ga-mutation')

    def _ths_ga_crossover(self, a):
        self.ga_crossover = self.try_type_convert(a, 'value', float, 'ga-crossover')

    def _ths_ga_population(self, a):
        self.ga_population = self.try_type_convert(a, 'value', int, 'ga-population')


class kNN(gamera.knncore.kNN):
    def __init__(self):
        gamera.knncore.kNN.__init__(self)
        self.ga_initial = 0.0
        self.ga_best = 0.0
        self.ga_worker_thread = None
        self.ga_worker_stop = 0
        self.ga_generation = 0
        self.ga_callbacks = []
        self.feature_functions = None

    def instantiate_from_images(self, images):
        """Create a k-NN database from a list of images"""
        assert(len(images) > 0)
        assert(images[0].feature_functions != [])
        self.feature_functions = images[0].feature_functions
        self._instantiate_from_images(images)

    def evaluate(self):
        """Evaluate the performance of the kNN classifier using
        leave-one-out cross-validation. The return value is a
        floating-point number between 0.0 and 1.0"""
        return self.leave_one_out()

    def supports_interactive(self):
        """Flag indicating that this classifier supports interactive
        classification."""
        return 1

    def supports_optimization(self):
        """Flag indicating that this classifier supports optimization."""
        return 1

    def start_optimizing(self):
        """Start optizing the classifier using a Genetic Algorithm"""
        self.ga_worker_stop = 0
        self.ga_worker_thread = GaWorker(self)
        self.ga_worker_thread.setDaemon(1)
        self.ga_worker_thread.start()
        return

    def stop_optimizing(self):
        """Stop optimization with the Genetic Algorithm. WARNING: this function
        has to wait for the current GA generation to finish before returning, which
        could take several secongs."""
        if not self.ga_worker_thread:
            return
        self.ga_worker_stop = 1
        self.ga_worker_thread.join()
        self.ga_worker_thread = None
        self._ga_destroy()
        
    def add_optimization_callback(self, func):
        """Add a function to be called everytime time the optimization updates the
        classifier."""
        self.ga_callbacks.append(func)

    def remove_optimization_callback(self, func):
        """Remove a function that is called everytime time the optimization updates the
        classifier."""
        try:
            self.ga_callbacks.remove(func)
        except:
            pass

    def supports_settings_dialog(self):
        """Flag to indicate that this classifier has a settings dialog"""
        return 1
    
    def settings_dialog(self):
        """Display a settings dialog for k-NN settings"""
        from gamera import args
        from wxPython.wx import NULL
        dlg = args.Args([args.Int('k', range=(0, 100), default=self.num_k),
                         args.Choice('Distance Function',
                                     ['City Block', 'Euclidean', 'Fast Euclidean'],
                                     default = self.distance_type)
                         ])
        k,distance = dlg.show(NULL)
        self.num_k = k
        self.distance_type = distance

    def save_settings(self, filename):
        """Save the k-NN settings to filename."""
        from util import word_wrap
        file = open(filename, "w")
        indent = 0
        word_wrap(file, '<?xml version="1.0" encoding="utf-8"?>', indent)
        word_wrap(file, '<gamera-knn-database version="1.0">', indent)
        indent += 1
        word_wrap(file, '<num-k value="%s"/>' % self.num_k, indent)
        word_wrap(file,'<distance-type value="%s"/>' % _distance_type_to_name[self.distance_type],
                  indent)
        word_wrap(file, '<ga-mutation value="%s"/>' % self.ga_mutation, indent)
        word_wrap(file, '<ga-crossover value="%s"/>' % self.ga_crossover, indent)
        word_wrap(file, '<ga-population value="%s"/>' % self.ga_population, indent)
        if self.feature_functions != None:
            word_wrap(file, '<weights>', indent)
            indent += 1
            feature_no = 0
            for name, function in self.feature_functions:
                word_wrap(file, '<weight name="%s">' % name, indent)
                length = function.return_type.length
                word_wrap(file,
                          [str(x) for x in
                           self.weights[feature_no:feature_no+length]],
                          indent + 1)
                word_wrap(file, '</weight>', indent)
                feature_no += length
            indent -= 1
            word_wrap(file, '</weights>', indent)
        indent -= 1
        word_wrap(file, '</gamera-knn-database>', indent)
        file.close()

    def load_settings(self, filename):
        loader = _KnnLoadXML()
        loader.parse_filename(filename)
        self.num_k = loader.num_k
        self.distance_type = loader.distance_type
        self.ga_mutation = loader.ga_mutation
        self.ga_crossover = loader.ga_crossover
        self.ga_population = loader.ga_population
        

        
        
                    
