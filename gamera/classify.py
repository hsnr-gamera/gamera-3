#
# Copyright (C) 2001-2004 Ichiro Fujinaga, Michael Droettboom,
#                          and Karl MacMillan
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but ITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#

import core # grab all of the standard gamera modules
import util, gamera_xml, config
import re
from fudge import Fudge

"""This file defines the Python part of classifiers.  These wrapper classes
contain a reference to a core classifier class (unusally written in C++).
They add functionality for XML loading/saving, splitting/grouping, and
keeping track of a database of glyphs (in the Interactive case.)"""

class ClassifierError(Exception):
   pass

class _Classifier:
   """The base class for both the interactive and noninteractive classifier."""

   ########################################
   # INFORMATION
   def get_name(self):
      return self.__class__.__name__

   ########################################
   # GROUPING
   def group_list_automatic(self, glyphs, grouping_function=None,
                            evaluate_function=None):
      if len(glyphs) == 0:
         return
      glyphs = [x for x in glyphs if x.classification_state != 3]
      splits, removed = self.classify_list_automatic(glyphs)
      glyphs = [x for x in glyphs if not x.get_main_id().startswith('split')]
      if grouping_function is None:
         grouping_function = _Classifier._default_grouping_function
      G = self._pregroup(glyphs, grouping_function)
      if evaluate_function is None:
         evaluate_function = self._evaluate_subgroup
      found_unions = self._find_group_unions(G, evaluate_function)
      return found_unions + splits, removed

   def _default_grouping_function(a, b):
      return Fudge(a, 8).intersects(b)
   _default_grouping_function = staticmethod(_default_grouping_function)

   def _pregroup(self, glyphs, function):
      from gamera import graph
      G = graph.Undirected()
      G.add_nodes(glyphs)
      progress = util.ProgressFactory("Pre-grouping glyphs...", len(glyphs))
      try:
         equivalencies = {}
         group_no = 0
         for i in range(len(glyphs)):
            gi = glyphs[i]
            for j in range(i + 1, len(glyphs)):
               gj = glyphs[j]
               if function(gi, gj):
                  G.add_edge(gi, gj)
            progress.step()
      finally:
         progress.kill()
      return G

   def _evaluate_subgroup(self, subgroup):
      import image_utilities
      if len(subgroup) > 1:
         union = image_utilities.union_images(subgroup)
         classification = self.guess_glyph_automatic(union)
         if (classification[0][1].startswith("split") or
             classification[0][1].startswith("skip")):
            return 0
         else:
            return classification[0][0]
      return subgroup[0].id_name[0][0]

   def _find_group_unions(self, G, evaluate_function):
      import image_utilities
      progress = util.ProgressFactory("Grouping glyphs...", G.nsubgraphs)
      try:
         found_unions = []
         for root in G.get_subgraph_roots():
            best_grouping = G.optimize_partitions(
               root, evaluate_function, 3)
            for subgroup in best_grouping:
               if len(subgroup) > 1:
                  union = image_utilities.union_images(subgroup)
                  found_unions.append(union)
                  classification = self.guess_glyph_automatic(union)
                  union.classify_heuristic(classification)
                  part_name = "_group._part." + classification[0][1]
                  for glyph in subgroup:
                     glyph.classify_heuristic(part_name)
            progress.step()
      finally:
         progress.kill()
      print "Number of groups created: %d" % len(found_unions)
      return found_unions

   ########################################
   # AUTOMATIC CLASSIFICATION
   def classify_glyph_automatic(self, glyph):
      """Classifies a glyph and sets its ``classification_state`` and
      ``id_name``.  (If you don't want it set, use
      guess_glyph_automatic.)  Returns a pair of lists: glyphs to be
      added to the current database, and glyphs to be removed from the
      current database."""
      # Since we only have one glyph to classify, we can't do any grouping
      if (len(self._database) and
          glyph.classification_state in (core.UNCLASSIFIED, core.AUTOMATIC)):
         removed = glyph.children_images
         id = self._classify_automatic_impl(glyph)
         glyph.classify_automatic(id)
         splits = self._do_splits(self, glyph)
         return splits, removed
      return [], []

   def _classify_list_automatic(self, glyphs, max_recursion=10, recursion_level=0, progress=None):

      # There is a slightly convoluted handling of the progress bar here, since
      # this function is called recursively on split glyphs
      if recursion_level == 0:
         progress = util.ProgressFactory("Classifying glyphs...", len(glyphs))
      try:
         if (recursion_level > max_recursion) or len(self._database) == 0:
            return [], []
         added = []
         removed = {}
         feature_functions = self.get_feature_functions()
         for glyph in glyphs:
            if glyph.classification_state in (core.UNCLASSIFIED, core.AUTOMATIC):
               for child in glyph.children_images:
                  removed[child] = None
         for glyph in glyphs:
            if not removed.has_key(glyph):
               glyph.generate_features(feature_functions)
               if (glyph.classification_state in
                   (core.UNCLASSIFIED, core.AUTOMATIC)):
                  id = self._classify_automatic_impl(glyph)
                  glyph.classify_automatic(id)
                  adds = self._do_splits(self, glyph)
                  progress.add_length(len(adds))
                  added.extend(adds)
            progress.step()
         if len(added):
            added_recurse, removed_recurse = self._classify_list_automatic(
               added, max_recursion, recursion_level+1, progress)
            added.extend(added_recurse)
            for glyph in removed_recurse:
               removed[glyph] = None
      finally:
         if recursion_level == 0:
            progress.kill()
      return added, removed.keys()

   def classify_list_automatic(self, glyphs, max_recursion=10, progress=None):
      """Classifies a list of glyphs and sets the
      classification_state and id_name of each glyph.  (If you don't want it set,
      use guess_glyph_automatic.)  Returns a pair of lists: glyphs to be added
      to the current database, and glyphs to be removed from the current
      database."""
      return self._classify_list_automatic(glyphs, max_recursion, 0, progress)

   def classify_and_update_list_automatic(self, glyphs, max_recursion=10, progress=None):
      added, removed = self.classify_list_automatic(glyphs, max_recursion, progress)
      result = glyphs + added
      for g in removed:
         if g in result:
            result.remove(g)
      return result

   # Since splitting glyphs is optional (when the classifier instance is
   # created) we have two versions of this function, so that there need not
   # be an 'if' statement everywhere.
   def _do_splits_impl(self, glyph):
      id = glyph.get_main_id()
      if (id.startswith('_split.')):
         parts = id.split('.')
         if (len(parts) != 2 or not hasattr(glyph, parts[1])):
            raise ClassifierError("'%s' is not a known or valid split function." % parts[1])
         try:
            splits = getattr(glyph, parts[1]).__call__(glyph)
         except core.SegmentationError:
            if len(glyph.id_name) >= 2:
               glyph.id_name = glyph.id_name[1:]
            else:
               glyph.id_name = [(0.0, '_ERROR')]
            return []
         else:
            glyph.children_images = splits
            return splits
      return []

   def _do_splits_null(self, glyph):
      return []

   ########################################
   # XML
   # Note that unclassified glyphs in the XML file are ignored.
   def to_xml(self, stream):
      """Saves the training data in XML format to the given stream (which could
be a file handle object)."""
      self.is_dirty = False
      return gamera_xml.WriteXML(
         glyphs=self.get_glyphs()).write_stream(stream)

   def to_xml_filename(self, filename):
      """Saves the training data in XML format to the given filename."""
      self.is_dirty = False
      return gamera_xml.WriteXMLFile(
         glyphs=self.get_glyphs()).write_filename(filename)

   def from_xml(self, stream):
      """Loads the training data from the given stream (which could be a file
handle object)."""
      self._from_xml(gamera_xml.LoadXML().parse_stream(stream))

   def from_xml_filename(self, filename):
      """Loads the training data from the given filename."""
      stream = gamera_xml.LoadXML().parse_filename(filename)
      self._from_xml(stream)

   def _from_xml(self, xml):
      database = [x for x in xml.glyphs
                  if x.classification_state != core.UNCLASSIFIED]
      self.set_glyphs(database)

   def merge_from_xml(self, stream):
      """Loads the training data from the given stream (which could be a file
handle or StringIO object) and adds it to the existing training data."""
      self._merge_xml(gamera_xml.LoadXML().parse_stream(stream))

   def merge_from_xml_filename(self, filename):
      """Loads the training data from the given filename and adds it to the
existing training data."""
      self._merge_xml(gamera_xml.LoadXML().parse_filename(filename))

   def _merge_xml(self, xml):
      database = [x for x in xml.glyphs
                  if x.classification_state != core.UNCLASSIFIED]
      self.merge_glyphs(database)

   ##############################################
   # Features
   def get_feature_functions(self):
      return self.feature_functions
   
class NonInteractiveClassifier(_Classifier):
   def __init__(self, database=[], features='all', perform_splits=True):
      """
      database: a list of database to initialize the classifier with.
      features: a list of strings naming the features that will be used in the
                classifier.
      perform_splits: (boolean) true if glyphs classified as split.* should be
                split."""
      self.is_dirty = False
      self._database = database
      self.features = features
      self.change_feature_set(features)
      self.instantiate_from_images(database)

      if perform_splits:
         self._do_splits = self.__class__._do_splits_impl
      else:
         self._do_splits = self.__class__._do_splits_null
      self._perform_splits = perform_splits

   def __del__(self):
      del self._database

   def is_interactive():
      return False
   is_interactive = staticmethod(is_interactive)

   ########################################
   # BASIC DATABASE MANIPULATION FUNCTIONS
   def get_glyphs(self):
      return self._database
      
   def set_glyphs(self, glyphs):
      # This operation can be quite expensive depending on core classifier
      self.generate_features(glyphs)
      self._database = glyphs
      self.instantiate_from_images(self._database)

   def merge_glyphs(self, glyphs):
      # This operation can be quite expensive depending on core classifier
      self.generate_features(glyphs)
      self._database.extend(glyphs)
      self.instantiate_from_images(self._database)

   def clear_glyphs(self):
      self._database = []

   def load_settings(self, filename):
      _Classifier.load_settings(self, filename)
      self.instantiate_from_images(self._database)      

   ########################################
   # AUTOMATIC CLASSIFICATION
   # (most of this is implemented in the base class, _Classifier)
   def guess_glyph_automatic(self, glyph):
      glyph.generate_features(self.get_feature_functions())
      return self.classify(glyph)

   def _classify_automatic_impl(self, glyph):
      return self.classify(glyph)

   #########################################
   # Features
   def change_feature_set(self, features):
      if len(self._database):
         self.instantiate_from_images(self._database)

class InteractiveClassifier(_Classifier):
   def __init__(self, database=[], features='all',
                perform_splits=1):
      """classifier: the core classifier to use.  If None, defaults to kNN
      database: a list of database to initialize the classifier with. (May be []).
      features: a list of strings naming the features that will be used in the
                classifier.
      perform_splits: (boolean) true if glyphs classified as split.* should be
                      split."""
      self.is_dirty = False
      self.clear_glyphs()
      for glyph in database:
         self._database[glyph] = None
      self.features = features
      self.change_feature_set(features)
      if perform_splits:
         self._do_splits = self.__class__._do_splits_impl
      else:
         self._do_splits = self.__class__._do_splits_null
      self._perform_splits = perform_splits
      self._display = None

   def __del__(self):
      del self._database

   def is_interactive():
      return 1
   is_interactive = staticmethod(is_interactive)

   ########################################
   # BASIC DATABASE MANIPULATION FUNCTIONS
   def get_glyphs(self):
      return self._database.keys()
      
   def set_glyphs(self, glyphs):
      glyphs = util.make_sequence(glyphs)
      self.is_dirty = len(glyphs) > 0
      self.clear_glyphs()
      self.generate_features(glyphs)
      for glyph in glyphs:
         self._database[glyph] = None

   def merge_glyphs(self, glyphs):
      glyphs = util.make_sequence(glyphs)
      self.is_dirty = len(glyphs) > 0
      self.generate_features(glyphs)
      for glyph in glyphs:
         self.is_dirty = 1
         self._database[glyph] = None

   def clear_glyphs(self):
      self.is_dirty = True
      self._database = {}

   ########################################
   # AUTOMATIC CLASSIFICATION
   def _classify_automatic_impl(self, glyph):
      if len(self._database) == 0:
         raise ClassifierError(
            "Cannot classify using an empty production database.")
      for child in glyph.children_images:
         if self._database.has_key(child):
            del self._database[child]
      return self.classify_with_images(self._database, glyph)

   def guess_glyph_automatic(self, glyph):
      if len(self._database):
         glyph.generate_features(self.get_feature_functions())
         return self.classify_with_images(
            self._database, glyph)
      else:
         return [(0.0, 'unknown')]

   def change_feature_set(self, features):
      """Change the set of features used in the classifier.  features is a list
      of strings, naming the feature functions to be used."""
      self.is_dirty = True
      if len(self._database):
         self.generate_features(self._database)
   
   def generate_features(self, glyphs):
      """Generates features for all the given glyphs."""
      import time
      t = time.clock()
      progress = util.ProgressFactory("Generating features...",
                                      len(glyphs) / 16)
      feature_functions = self.get_feature_functions()
      try:
         for i, glyph in enumerate(glyphs):
            glyph.generate_features(feature_functions)
            if i & 0xf == 0:
               progress.step()
      finally:
         progress.kill()
      print "generate features time:", time.clock() - t

   ########################################
   # MANUAL CLASSIFICATION
   def classify_glyph_manual(self, glyph, id):
      """Classifies the given glyph using name id.  Returns a pair of lists: the
      glyphs that should be added and removed to the current database."""
      self.is_dirty = True

      # Deal with grouping
      if id.startswith('_group'):
         raise ClassifierError(
            "You must select more than one connected component " +
            "to create a group")

      removed = {}
      for child in glyph.children_images:
         removed[child] = None
         if self._database.has_key(child):
            del self._database[child]
      glyph.classify_manual([(1.0, id)])
      glyph.generate_features(self.get_feature_functions())
      self._database[glyph] = None
      return self._do_splits(self, glyph), removed.keys()

   def classify_list_manual(self, glyphs, id):
      if id.startswith('_group'):
         import image_utilities
         parts = id.split('.')
         sub = '.'.join(parts[1:])
         union = image_utilities.union_images(glyphs)
         for glyph in glyphs:
            glyph.classify_heuristic('_group._part.' + sub)
         added, removed = self.classify_glyph_manual(union, sub)
         added.append(union)
         return added, removed

      added = []
      removed = {}
      for glyph in glyphs:
         for child in glyph.children_images:
            removed[child] = None

      feature_functions = self.get_feature_functions()
      for glyph in glyphs:
         # Don't re-insert removed children glyphs
         if not removed.has_key(glyph):
            self.is_dirty = True
            if not self._database.has_key(glyph):
               glyph.generate_features(feature_functions)
               self._database[glyph] = None
            glyph.classify_manual([(1.0, id)])
            added.extend(self._do_splits(self, glyph))
      return added, removed.keys()

   def add_to_database(self, glyphs):
      glyphs = util.make_sequence(glyphs)
      feature_functions = self.get_feature_functions()
      for glyph in glyphs:
         if (glyph.classification_state == core.MANUAL and
             not self._database.has_key(glyph)):
            self.is_dirty = True
            glyph.generate_features(feature_functions)
            self._database[glyph] = None

   def remove_from_database(self, glyphs):
      glyphs = util.make_sequence(glyphs)
      for glyph in glyphs:
         if self._database.has_key(glyph):
            self.is_dirty = True
            del self._database[glyph]

   def display(self, current_database=[],
               context_image=None, symbol_table=[]):
      gui = config.options.__.gui
      if gui and self._display is None:
         self._display = gui.ShowClassifier(
            self, current_database, context_image, symbol_table)
      else:
         self._display.Show(1)

   def set_display(self, display):
      self._display = display


