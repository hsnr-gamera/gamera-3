#
# Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom,
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
import sys, os
from gamera import core, util, config, classify
from gamera.plugins import features as features_module
import gamera.knncore, gamera.gamera_xml
import array

from gamera.knncore import CITY_BLOCK
from gamera.knncore import EUCLIDEAN
from gamera.knncore import FAST_EUCLIDEAN

KNN_XML_FORMAT_VERSION = 1.0

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
      self.knn.ga_initial = self.knn._ga_create()
      self.knn.ga_best = self.knn.ga_initial
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
      self.add_start_element_handler('gamera-knn-settings', self._tag_start_knn_settings)
      self.add_start_element_handler('ga', self._tag_start_ga)
      self.add_start_element_handler('weights', self._tag_start_weights)
      self.add_end_element_handler('weights', self._tag_end_weights)

   def _remove_handlers(self):
      self.remove_start_element_handler('gamera-knn-settings')
      self.remove_start_element_handler('ga')
      self.remove_start_element_handler('weights')

   def _tag_start_knn_settings(self, a):
      version = self.try_type_convert(a, 'version', float, 'gamera-knn-settings')
      if version < KNN_XML_FORMAT_VERSION:
         raise gamera_xml.XMLError(
            "knn-settings XML file is an older version that can not be read by this version of Gamera.")
      self.num_k = self.try_type_convert(a, 'num-k', int, 'gamera-knn-settings')
      self.distance_type = \
        _distance_type_to_number[self.try_type_convert(a, 'distance-type',
                                                       str, 'gamera-knn-settings')]

   def _tag_start_ga(self, a):
      self.ga_mutation = self.try_type_convert(a, 'mutation', float, 'ga')
      self.ga_crossover = self.try_type_convert(a, 'crossover', float, 'ga')
      self.ga_population = self.try_type_convert(a, 'population', int, 'ga')

   def _tag_start_weights(self, a):
      self.add_start_element_handler('weight', self._tag_start_weight)
      self.add_end_element_handler('weight', self._tag_end_weight)

   def _tag_end_weights(self):
      self.remove_start_element_handler('weight')
      self.remove_end_element_handler('weight')

   def _tag_start_weight(self, a):
      self._data = u''
      self._weight_name = str(a["name"])
      self._parser.CharacterDataHandler = self._add_weights

   def _tag_end_weight(self):
      self._parser.CharacterDataHandler = None
      self.weights[self._weight_name] = array.array('d')
      nums = str(self._data).split()
      tmp = array.array('d', [float(x) for x in nums])
      self.weights[self._weight_name] = tmp

   def _add_weights(self, data):
      self._data += data

class _kNNBase(gamera.knncore.kNN):
   """k-NN classifier that supports optimization using
   a Genetic Algorithm. This classifier supports all of
   the Gamera interactive/non-interactive classifier interface."""

   def __init__(self, num_features=1, num_k=1):
      """Constructor for knn object. Features is a list
      of feature names to use for classification. If features
      is none then the default settings will be loaded from a
      file specified in the user config file. If there is no
      settings file specified then all of the features will be
      used."""
      gamera.knncore.kNN.__init__(self)
      self.num_features = num_features
      self.num_k = num_k
      self.ga_initial = 0.0
      self.ga_best = 0.0
      self.ga_worker_thread = None
      self.ga_worker_stop = 0
      self.ga_generation = 0
      self.ga_callbacks = []

   def __del__(self):
      pass

   def distance_from_images(self, images, glyph, max=None):
      """**distance_from_images** (ImageList *glyphs*, Image *glyph*, Float *max* = ``None``)

Compute a list of distances between a list of glyphs and a single glyph. Distances
greater than *max* are not included in the output.  The return value is a list of
floating-point distances.
"""
      self.generate_features(glyph)
      self.generate_features_on_glyphs(images)
      if max is None:
         return self._distance_from_images(iter(images), glyph)
      else:
         return self._distance_from_images(iter(images), glyph, max)

   def distance_between_images(self, imagea, imageb):
      """**distance_between_images** (Image *imagea*, Image *imageb*)

Compute the distance between two images using the settings
for the kNN object (distance_type, features, weights, etc). This
can be used when more control over the distance computations are
needed than with any of the other methods that work on multiple
images at once."""
      self.generate_features(imagea)
      self.generate_features(imageb)
      return self._distance_between_images(imagea, imageb)

   def distance_matrix(self, images, normalize=True):
      """**distance_matrix** (ImageList *images*, Bool *normalize* = ``True``)

Create a symmetric FloatImage containing all of the
distances between the images in the list passed in. This is useful
because it allows you to find the distance between any two pairs
of images regardless of the order of the pairs.

*normalize*
  When true, the features are normalized before performing the distance
  calculations."""
      self.generate_features_on_glyphs(images)
      l = len(images)
      progress = util.ProgressFactory("Generating unique distances...", l)
      m = self._distance_matrix(images, progress.step, normalize)
      #m = self._distance_matrix(images)
      progress.kill()
      return m

   def unique_distances(self, images, normalize=True):
      """**unique_distances** (ImageList *images*, Bool *normalize* = ``True``)

Return a list of the unique pairs of images in the passed in list
and the distances between them. The return list is a list of tuples
of (distance, imagea, imageb) so that it easy to sort.

*normalize*
  When true, the features are normalized before performing the distance
  calculations."""
      self.generate_features_on_glyphs(images)
      l = len(images)
      progress = util.ProgressFactory("Generating unique distances...", l)
      dists = self._unique_distances(images, progress.step, normalize)
      #dists = self._unique_distances(images)
      progress.kill()
      return dists

   def evaluate(self):
      """Float **evaluate** ()

Evaluate the performance of the kNN classifier using
leave-one-out cross-validation. The return value is a
floating-point number between 0.0 (0% correct) and 1.0 (100%
correct).
"""
      ans = self.leave_one_out()
      return float(ans[0]) / float(ans[1])

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
      """**save_settings** (FileSave *filename*)

Save the kNN settings to the given filename. This settings file (which is XML)
includes k, distance type, GA mutation rate, GA crossover rate, GA population size,
and the current floating point weights. This file is different from the one produced
by serialize in that it contains only the settings and no data."""
      from util import word_wrap
      file = open(filename, "w")
      indent = 0
      word_wrap(file, '<?xml version="1.0" encoding="utf-8"?>', indent)
      word_wrap(file,
                '<gamera-knn-settings version="%s" num-k="%s" distance-type="%s">'
                % (KNN_XML_FORMAT_VERSION,
                   self.num_k,
                   _distance_type_to_name[self.distance_type]), indent)
      indent += 1
      word_wrap(file, '<ga mutation="%s" crossover="%s" population="%s"/>' %
                (self.ga_mutation, self.ga_crossover, self.ga_population), indent)
      if self.feature_functions != None:
         word_wrap(file, '<weights>', indent)
         indent += 1
         feature_no = 0
         weights = self.get_weights()
         for name, function in self.feature_functions[0]:
            word_wrap(file, '<weight name="%s">' % name, indent)
            length = function.return_type.length
            word_wrap(file,
                      [x for x in
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
      """**load_settings** (FileOpen *filename*)

Load the kNN settings from an XML file.  See save_settings_."""
      from gamera import core

      loader = _KnnLoadXML()
      loader.parse_filename(filename)
      self.num_k = loader.num_k
      self.distance_type = loader.distance_type
      self.ga_mutation = loader.ga_mutation
      self.ga_crossover = loader.ga_crossover
      self.ga_population = loader.ga_population
      functions = loader.weights.keys()
      functions.sort()
      self.change_feature_set(functions)
      # Create the weights array with the weights in the correct order
      weights = array.array('d')
      for x in self.feature_functions[0]:
         weights.extend(loader.weights[x[0]])
      self.set_weights(weights)

   def serialize(self, filename):
      """**serialize** (FileSave *filename*)

Saves the classifier-specific settings *and* data in an optimized and
classifer-specific format.  

.. note:: 
   It is good practice to retain the XML
   file, since it is portable across platforms and to future versions of
   Gamera.  The binary format is not guaranteed to be portable."""
      if self.features == 'all':
         gamera.knncore.kNN.serialize(self, filename,['all'])
      else:
         gamera.knncore.kNN.serialize(self, filename,self.features)

   def unserialize(self, filename):
      """**unserialize** (FileOpen *filename*)

Opens the classifier-specific settings *and* data from an optimized and
classifer-specific format."""
      features = gamera.knncore.kNN.unserialize(self, filename)
      if len(features) == 1 and features[0] == 'all':
         self.change_feature_set('all')
      else:
         self.change_feature_set(features)

   def generate_features(self, glyph):
      """**generate_features** (Image *glyph*)

Generates features for the given glyph.
"""
      glyph.generate_features(self.feature_functions)

class kNNInteractive(_kNNBase, classify.InteractiveClassifier):
   def __init__(self, database=[], features='all', perform_splits=1, num_k=1):
      """**kNNInteractive** (ImageList *database* = ``[]``, *features* = 'all', bool *perform_splits* = ``True``)

Creates a new kNN interactive classifier instance.

*database*
        Must be a list (or Python interable) containing glyphs to use
        as training data for the classifier.

        Any images in the list that were manually classified (have
	classification_state == MANUAL) will be used as training data
	for the classifier.  Any UNCLASSIFIED or AUTOMATICALLY
	classified images will be ignored.

	When initializing a noninteractive classifier, the database
	*must* be non-empty.

*features*
	A list of feature function names to use for classification.
	These feature names
	correspond to the `feature plugin methods`__.  To use all
	available feature functions, pass in ``'all'``.

.. __: plugins.html#features

*perform_splits*
	  If ``perform_splits`` is ``True``, glyphs trained with names
	  beginning with ``_split.`` are run through a given splitting
	  algorithm.  For instance, glyphs that need to be broken into
	  upper and lower halves for further classification of those
	  parts would be trained as ``_split.splity``.  When the
	  automatic classifier encounters glyphs that most closely
	  match those trained as ``_split``, it will perform the
	  splitting algorithm and then continue to recursively
	  classify its parts.

	  The `splitting algorithms`__ are documented in the plugin documentation.

.. __: plugins.html#segmentation

          New splitting algorithms can be created by `writing plugin`__ methods
          in the category ``Segmentation``.  

.. __: writing_plugins.html

      """
      self.features = features
      self.feature_functions = core.ImageBase.get_feature_functions(features)
      num_features = features_module.get_features_length(features)
      _kNNBase.__init__(self, num_features)
      classify.InteractiveClassifier.__init__(self, database, perform_splits)

   def __del__(self):
      _kNNBase.__del__(self)
      classify.InteractiveClassifier.__del__(self)

   def noninteractive_copy(self):
      """**noninteractive_copy** ()

Creates a non-interactive copy of the interactive classifier."""
      return kNNNonInteractive(
         list(self.get_glyphs()), self.features, self._perform_splits, num_k=self.num_k)

   def supports_optimization(self):
      """Flag indicating that this classifier supports optimization."""
      return False

   def change_feature_set(self, f):
      """**change_feature_set** (*features*)

Changes the set of features used in the classifier to the given list of feature names.

*features*
  These feature names correspond to the `feature plugin methods`__.
  To use all available feature functions, pass in ``'all'``.

.. __: plugins.html#features"""
      self.features = f
      self.feature_functions = core.ImageBase.get_feature_functions(f)
      self.num_features = features_module.get_features_length(f)
      if len(self.database):
         self.is_dirty = True
         self.generate_features_on_glyphs(self.database)

class kNNNonInteractive(_kNNBase, classify.NonInteractiveClassifier):
   def __init__(self, database=[], features='all', perform_splits=True, num_k=1):
      """**kNNNonInteractive** (ImageList *database* = ``[]``, *features* = ``'all'``,
bool *perform_splits* = ``True``)

Creates a new kNN classifier instance.

*database*
        Can be in one of two forms:

           - When a list (or Python iterable) each element is a glyph
             to use as training data for the classifier.  (For
             non-interactive classifiers, this list must be
             non-empty).

           - For non-interactive classifiers, *database* may be a
             filename, in which case the classifier will be
             "unserialized" from the given file.

        Any images in the list that were manually classified (have
	classification_state == MANUAL) will be used as training data
	for the classifier.  Any UNCLASSIFIED or AUTOMATICALLY
	classified images will be ignored.

	When initializing a noninteractive classifier, the database
	*must* be non-empty.

*features*
	A list of feature function names to use for classification.
	These feature names
	correspond to the `feature plugin methods`__.  To use all
	available feature functions, pass in ``'all'``.

.. __: plugins.html#features

*perform_splits*
	  If ``perform_splits`` is ``True``, glyphs trained with names
	  beginning with ``_split.`` are run through a given splitting
	  algorithm.  For instance, glyphs that need to be broken into
	  upper and lower halves for further classification of those
	  parts would be trained as ``_split.splity``.  When the
	  automatic classifier encounters glyphs that most closely
	  match those trained as ``_split``, it will perform the
	  splitting algorithm and then continue to recursively
	  classify its parts.

	  The `splitting algorithms`__ are documented in the plugin documentation.

.. __: plugins.html#segmentation

          New splitting algorithms can be created by `writing plugin`__ methods
          in the category ``Segmentation``.  

.. __: writing_plugins.html

      """
      self.features = features
      self.feature_functions = core.ImageBase.get_feature_functions(features)
      num_features = features_module.get_features_length(features)
      _kNNBase.__init__(self, num_features)
      classify.NonInteractiveClassifier.__init__(self, database, perform_splits)

   def __del__(self):
      _kNNBase.__del__(self)
      classify.NonInteractiveClassifier.__del__(self)

   def change_feature_set(self, f):
      """**change_feature_set** (*features*)

Changes the set of features used in the classifier to the given list of feature names.

*features*
  These feature names correspond to the `feature plugin methods`__.
  To use all available feature functions, pass in ``'all'``.

.. __: plugins.html#features"""
      self.features = f
      self.feature_functions = core.ImageBase.get_feature_functions(f)
      self.num_features = features_module.get_features_length(f)
      if len(self.database):
         self.is_dirty = True
         self.generate_features_on_glyphs(self.database)
         self.instantiate_from_images(self.database)

   def supports_optimization(self):
      """Flag indicating that this classifier supports optimization."""
      return True

   def start_optimizing(self):
      """**start_optimizing** ()

Starts the genetic algorithm optimization of the weights of the
features.  The optimization is run in a background thread.

For information about genetic algorithms, see [Holland1975]_.

In the genetic algorithm, the population consists of vectors of
feature weights.  The vectors are evaluated using the
leave_one_out algorithm.  The vectors that perform well are
allowed to reproduce to producing offspring using combination at a
randomly chosen split point.

For a user-friendly way to perform GA optimization, consider the Biollante_
tool in the Gamera GUI.

.. _Biollante: gui.html#classifier-optimization-biollante"""
      self.ga_worker_stop = False
      self.ga_worker_thread = GaWorker(self)
      self.ga_worker_thread.setDaemon(1)
      self.ga_worker_thread.start()

   def stop_optimizing(self):
      """**stop_optimizing** ()

Stops the background optimization thread.

NOTE: This method has to wait for the current GA generation to finish before returning, which
could take several seconds."""
      if not self.ga_worker_thread:
         return
      self.ga_worker_stop = 1
      self.ga_worker_thread.join()
      self.ga_worker_thread = None
      self._ga_destroy()
      return self.ga_best

   def add_optimization_callback(self, func):
      """**add_optimization_callback** (*function*)

Adds a function that will be called everytime the optimization process
improves the performance of the classifier.  This callback function must take
one argument which is an instance of the kNN classifier."""
      self.ga_callbacks.append(func)

   def remove_optimization_callback(self, func):
      """**remove_optimization_callback** (*function*)

Removes an optimization callback function added using
add_optimization_callback_."""
      try:
         self.ga_callbacks.remove(func)
      except:
         pass

def simple_feature_selector(glyphs):
   """simple_feature_selector does a brute-force search through all
   possible combinations of features and returns a sorted list of
   tuples (accuracy, features). WARNING: this function should take
   a long time to complete."""

   if len(glyphs) <= 1:
      raise RuntimeError("Lenght of list must be greater than 1")
   
   c = classify.kNNNonInteractive()

   # For efficiency we calculate all of the features and pass in the
   # indexes of the features vector that we want to use for the distance
   # calculation. This is more efficient than using the features weights
   # or recalculating the features.
   all_features = []
   feature_indexes = {}
   offset = 0
   for x in glyphs[0].get_feature_functions()[0]:
      all_features.append(x[0])
      feature_indexes[x[0]] = range(offset, offset + x[1].return_type.length)
      offset += x[1].return_type.length
   # First do the easy ones = single features and all features
   answers = []
   c.change_feature_set(all_features)
   c.set_glyphs(glyphs)
   ans = c.classifier.leave_one_out()
   # Because we are only interested in top score, we can stop the evaluation
   # after we have missed too many answers to possibly beat the top score.
   # Therefore, we store the number missed for the top score here and pass
   # it into leave_one_out.
   stop_threshold = ans[1] - ans[0]
   answer = (float(ans[0]) / float(ans[1]), all_features)
   for x in all_features:
      ans = c.classifier.leave_one_out(feature_indexes[x], stop_threshold)
      num_wrong = ans[1] - ans[0]
      print num_wrong, ans[1], ans[0]
      if num_wrong < stop_threshold:
         stop_threshold = num_wrong
         answer = (float(ans[0]) / float(ans[1]), x)
   # Now do the remaining combinations using the CombGen object for each
   # size subset of all of the features.
   for i in range(2, len(all_features) - 1):
      for x in CombGen(all_features, i):
         indexes = []
         for y in x:
            indexes.extend(feature_indexes[y])
         ans = c.classifier.leave_one_out(indexes, stop_threshold)
         num_wrong = ans[1] - ans[0]
         print num_wrong, ans[1], ans[0]
         if num_wrong < stop_threshold:
            stop_threshold = num_wrong
            answer = (float(ans[0]) / float(ans[1]), x)
   return answer


class CombGen:
   """Generate the k-combinations of a sequence. This is a iterator
   that generates the combinations on the fly. This code was adapted
   from a posting by Tim Peters"""
   def __init__(self, seq, k):
      n = self.n = len(seq)
      if not 1 <= k <= n:
         raise ValueError("k must be in 1.." + `n` + ": " + `k`)
      self.k = k
      self.seq = seq
      self.indices = range(k)
      # Trickery to make the first .next() call work.
      self.indices[-1] = self.indices[-1] - 1

   def __iter__(self):
      return self

   def next(self):
      n, k, indices = self.n, self.k, self.indices
      lasti, limit = k-1, n-1
      while lasti >= 0 and indices[lasti] == limit:
         lasti = lasti - 1
         limit = limit - 1
      if lasti < 0:
         raise StopIteration
      newroot = indices[lasti] + 1
      indices[lasti:] = range(newroot, newroot + k - lasti)
      # Build the result.
      result = []
      seq = self.seq
      for i in indices:
         result.append(seq[i])
      return result

def _get_id_stats(glyphs, k=None):
   import stats
   if len(glyphs) < 3:
      return (len(glyphs),1.0, 1.0, 1.0)
   if k is None:
      k = kNN()
   distances = k.unique_distances(glyphs)
   return (len(glyphs),stats.lmean(distances), stats.lstdev(distances), stats.lmedian(distances))

def get_glyphs_stats(glyphs):
   k = kNN()
   klasses = {}
   for x in glyphs:
      id = x.get_main_id()
      if not klasses.has_key(id):
         klasses[id] = []
      klasses[id].append(x)
   stats = {}
   for x in klasses.iteritems():
      stats[x[0]] = _get_id_stats(x[1], k)
   return stats

def comma_delim_stats(glyphs, filename):
   file = open(filename, "w")
   stats = get_glyphs_stats(glyphs)
   for x in stats.iteritems():
      file.write(x[0])
      file.write(',')
      file.write(str(x[1][0]))
      file.write(',')
      file.write(str(x[1][1]))
      file.write(',')
      file.write(str(x[1][2]))
      file.write(',')
      file.write(str(x[1][3]))
      file.write('\n')
   file.close()

def glyphs_by_category(glyphs):
   klasses = {}
   for x in glyphs:
      id = x.get_main_id()
      if not klasses.has_key(id):
         klasses[id] = []
      klasses[id].append(x)
   return klasses
