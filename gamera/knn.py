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

from gamera import core, util
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

# The kNN classifier stores it settings in a simple xml file -
# this class uses the gamera_xml.LoadXML class to load that
# file. After the file is loaded, kNN.load_settings extracts
# the data from the class to set up kNN.
class _KnnLoadXML(gamera.gamera_xml.LoadXML):
    def __init__(self):
        gamera.gamera_xml.LoadXML.__init__(self)

    def _setup_handlers(self):
        self.feature_functions = []
        self.weights = { }
        self.num_k = None
        self.distance_type = None
        self.ga_mutation = None
        self.ga_crossover = None
        self.ga_population = None
        self.add_start_element_handler('gamera-knn-settings', self._ths_knn_settings)
        self.add_start_element_handler('ga', self._ths_ga)
        self.add_start_element_handler('weights', self._ths_weights)
        self.add_end_element_handler('weights', self._the_weights)
        
    def _remove_handlers(self):
        self.remove_start_element_handler('gamera-knn-settings')
        self.remove_start_element_handler('ga')
        self.remove_start_element_handler('weights')

    def _ths_knn_settings(self, a):
        self.num_k = self.try_type_convert(a, 'num-k', int, 'gamera-knn-settings')
        self.distance_type = \
          _distance_type_to_number[self.try_type_convert(a, 'distance-type',
                                                         str, 'gamera-knn-settings')]

    def _ths_ga(self, a):
        self.ga_mutation = self.try_type_convert(a, 'mutation', float, 'ga')
        self.ga_crossover = self.try_type_convert(a, 'crossover', float, 'ga')
        self.ga_population = self.try_type_convert(a, 'population', int, 'ga')

    def _ths_weights(self, a):
        self.add_start_element_handler('weight', self._ths_weight)
        self.add_end_element_handler('weight', self._the_weight)

    def _the_weights(self):
        self.remove_start_element_handler('weight')
        self.remove_end_element_handler('weight')

    def _ths_weight(self, a):
        self._data = u''
        self._weight_name = str(a["name"])
        self._parser.CharacterDataHandler = self._add_weights

    def _the_weight(self):
        self._parser.CharacterDataHandler = None
        self.weights[self._weight_name] = array.array('d')
        nums = str(self._data).split()
        tmp = array.array('d', [float(x) for x in nums])
        self.weights[self._weight_name] = tmp

    def _add_weights(self, data):
        self._data += data

def _get_num_features(features):
    ff = core.ImageBase.get_feature_functions(features)
    features = 0
    for x in ff:
        features += x[1].return_type.length
    return features

class kNN(gamera.knncore.kNN):
    """k-NN classifier that supports optimization using
    a Genetic Algorithm. This classifier supports all of
    the Gamera interactive/non-interactive classifier interface."""

    def __init__(self, features='all'):
        gamera.knncore.kNN.__init__(self)
        self.change_feature_set(features)
        self.ga_initial = 0.0
        self.ga_best = 0.0
        self.ga_worker_thread = None
        self.ga_worker_stop = 0
        self.ga_generation = 0
        self.ga_callbacks = []
        self.features = features

    def change_feature_set(self, features):
        """Change the set of features used in the classifier.  features is a list of
        strings, naming the feature functions to be used."""
        self.features = features
        self.feature_functions = core.ImageBase.get_feature_functions(self.features)
        self.num_features = _get_num_features(self.features)

    def distance_from_images(self, images, glyph, max):
        glyph.generate_features(self.feature_functions)
        progress = None
        for x in images:
            if progress == None and \
               x.feature_functions != self.feature_functions:
                progress = util.ProgressFactory("Generating Features . . .", len(images))
            x.generate_features(self.feature_functions)
            if progress:
                progress.step()
        if progress:
            progress.kill()
        return self._distance_from_images(iter(images), glyph, max)

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
        """Start optizing the classifier using a Genetic Algorithm. This
        function will not block. Instead it starts a background (daemon) thread that
        will perform the optimization in the background. While the classifier is
        performing classification no other methods should be called until stop_optimizing
        has been called."""
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
    
    def settings_dialog(self, parent):
        """Display a settings dialog for k-NN settings"""
        from gamera import args
        dlg = args.Args([args.Int('k', range=(0, 100), default=self.num_k),
                         args.Choice('Distance Function',
                                     ['City block', 'Euclidean', 'Fast Euclidean'],
                                     default = self.distance_type)
                         ], name="kNN settings")
        results = dlg.show(parent)
        if results is None:
            return
        self.num_k, self.distance_type = results

    def save_settings(self, filename):
        """Save the k-NN settings to filename. This settings file (which is xml)
        includes k, distance type, GA mutation rate, GA crossover rate, GA population size,
        and the current floating point weights. This file is different from the one produced
        by serialize in that it contains only the settings and no data."""
        from util import word_wrap
        file = open(filename, "w")
        indent = 0
        word_wrap(file, '<?xml version="1.0" encoding="utf-8"?>', indent)
        word_wrap(file, '<gamera-knn-settings version="1.0" num-k="%s" distance-type="%s">'
                  % (self.num_k, _distance_type_to_name[self.distance_type]), indent)
        indent += 1
        word_wrap(file, '<ga mutation="%s" crossover="%s" population="%s"/>' %
                  (self.ga_mutation, self.ga_crossover, self.ga_population), indent)
        if self.feature_functions != None:
            word_wrap(file, '<weights>', indent)
            indent += 1
            feature_no = 0
            weights = self.get_weights()
            for name, function in self.feature_functions:
                word_wrap(file, '<weight name="%s">' % name, indent)
                length = function.return_type.length
                word_wrap(file,
                          [str(x) for x in
                           weights[feature_no:feature_no+length]],
                          indent + 1)
                word_wrap(file, '</weight>', indent)
                feature_no += length
            indent -= 1
            word_wrap(file, '</weights>', indent)
        indent -= 1
        word_wrap(file, '</gamera-knn-settings>', indent)
        file.close()

    def load_settings(self, filename):
        """Load the k-NN settings from an xml file. If settings file contains
        weights they are loading. Loading the weights can potentially change the
        feature functions of which the classifier is aware."""
        from gamera import core
        
        loader = _KnnLoadXML()
        loader.parse_filename(filename)
        self.num_k = loader.num_k
        self.distance_type = loader.distance_type
        self.ga_mutation = loader.ga_mutation
        self.ga_crossover = loader.ga_crossover
        self.ga_population = loader.ga_population
        # In order to correctly set the weights we need to look
        # up all of the feature functions for the individual weights
        # so that we can set self.feature_functions. This allows the
        # classification methods to know whether the weights should
        # be used or not (according to whether the features on the
        # image passed in match the features we currently know about).
        functions = []
        for key in loader.weights:
            try:
                func = core.ImageBase.get_feature_functions(key)
            except Exception, e:
                raise gamera.gamera_xml.XMLError("While loading the weights " +
                "an unknown feature function '%s' was found." % key)
            functions.append(func[0])
        functions.sort()
        self.change_feature_set(functions)
        # Create the weights array with the weights in the correct order
        weights = array.array('d')
        for x in self.feature_functions:
            weights.extend(loader.weights[x[0]])
        self.set_weights(weights)
            
    def serialize(self, filename):
        """Save the k-NN settings and data into an optimized, k-NN specific
        file format."""
        if self.features == 'all':
            gamera.knncore.kNN.serialize(self, filename,['all'])
        else:
            gamera.knncore.kNN.serialize(self, filename,self.features)
            

    def unserialize(self, filename):
        """Load the k-NN settings and data from an optimized, k-NN specific
        file format"""
        features = gamera.knncore.kNN.unserialize(self, filename)
        if len(features) == 1 and features[0] == 'all':
            self.change_feature_set('all')
        else:
            self.change_feature_set(features)

        

        
        
                    
