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

import core # grab all of the standard gamera modules
import util, gamera_xml, config
import re

"""This file defines the Python part of classifiers.  These wrapper classes
contain a reference to a core classifier class (unusally writting in C++), and an
optional GroupingClassifier.  They add functionality for XML loading/saving,
splitting/grouping, and keeping track of a database of glyphs (in the Interactive
case.)"""

class ClassifierError(Exception):
   pass

class _Classifier:
   """The base class for both the interactive and noninteractive classifier."""

   ########################################
   # INFORMATION
   def get_name(self):
      return self.classifier.__class__.__name__

   ########################################
   # GROUPING
   def get_groups(self):
      return self.grouping_classifier.get_groups()

   def set_groups(self, groups):
      self.grouping_classifier.set_groups(groups)

   def merge_groups(self):
      self.grouping_classifier.merge_groups(groups)

   def group_list_automatic(self, glyphs):
      return self.grouping_classifier.search(glyphs)

   ########################################
   # AUTOMATIC CLASSIFICATION
   def classify_glyph_automatic(self, glyph):
      """Classifies a glyph using the core classifier.  Sets the
      classification_state and id_name of the glyph.  (If you don't want it set, use
      guess_glyph_automatic.  Returns a pair of lists: glyphs to be added to the current
      database, and glyphs to be removed from the current database. """
      # Since we only have one glyph to classify, we can't do any grouping
      if (len(self._database) and
          not glyph.classification_state in (core.MANUAL, core.HEURISTIC)):
         glyph.generate_features(self.feature_functions)
         id = self._classify_automatic_impl(glyph)
         glyph.classify_automatic(id)
         splits = self._do_splits(glyph)
         return splits, []
      return [], []

   def classify_list_automatic(self, glyphs, recursion_level=0, progress=None):
      """Classifies a list of glyphs using the core classifier.  Sets the
      classification_state and id_name of the glyph.  (If you don't want it set, use
      guess_glyph_automatic.  Returns a pair of lists: glyphs to be added to the current
      database, and glyphs to be removed from the current database. The keyword arguments
      are for internal use only."""
      if recursion_level == 0:
         progress = util.ProgressFactory("Classifying glyphs...")
      progress.add_length(len(glyphs) * 2)
      try:
         for glyph in glyphs:
            glyph.generate_features(self.feature_functions)
            progress.step()
         if (recursion_level > 10) or len(self._database) == 0:
            return [], []
         added = []
         removed = []
         for glyph in glyphs:
            if glyph.classification_state in (core.UNCLASSIFIED, core.AUTOMATIC):
               id = self._classify_automatic_impl(glyph)
               glyph.classify_automatic(id)
               added.extend(self._do_splits(glyph))
               progress.step()
         if len(added):
            for glyph in added:
               glyph.generate_features(self.feature_functions)
            added_recurse, removed_recurse = self.classify_list_automatic(
               added, recursion_level+1)
            added.extend(added_recurse)
            removed.extend(removed_recurse)
      finally:
         if recursion_level == 0:
            progress.kill()
      return added, removed

   # Since splitting glyphs is optional (when the classifier instance is created)
   # we have two versions of this function, so that there need not be an 'if'
   # statement everywhere.
   def _do_splits_impl(self, glyph):
      id = glyph.get_main_id()
      if (id.startswith('split')):
         parts = id.split('.')
         if (len(parts) != 2 or not hasattr(glyph, parts[1])):
            raise ClassifierError("'%s' is not a known or valid split function.")
         splits = getattr(glyph, id[6:]).__call__(glyph)
         glyph.children_images = splits
         return splits
      return []

   def _do_splits_null(self, glyph):
      pass

   def generate_features(self, glyphs):
      """Generates features for all the given glyphs."""
      progress = util.ProgressFactory("Generating features...", len(list))
      try:
         for glyph in list:
            glyph.generate_features(self.feature_functions)
            progress.step()
      finally:
         progress.kill()

   ########################################
   # XML
   # Note that unclassified glyphs in the XML file are ignored.
   def to_xml(self, stream):
      import gamera_xml
      return gamera_xml.WriteXML(
         glyphs=self.get_glyphs(),
         groups=self.get_groups()).write_stream(stream)

   def to_xml_filename(self, filename):
      import gamera_xml
      return gamera_xml.WriteXMLFile(
         glyphs=self.get_glyphs(),
         groups=self.get_groups()).write_filename(filename)

   def from_xml(self, stream):
      self._from_xml(gamera_xml.LoadXML().parse_stream(stream))

   def from_xml_filename(self, filename):
      self._from_xml(gamera_xml.LoadXML().parse_filename(filename))

   def _from_xml(self, xml):
      database = [x for x in xml.glyphs if x.classification_state != core.UNCLASSIFIED]
      self.generate_features(database)
      self.set_glyphs(database)
      self.set_groups(xml.groups)

   def merge_from_xml(self, stream):
      self._merge_xml(gamera_xml.LoadXML().parse_stream(stream))

   def merge_from_xml_filename(self, filename):
      self._merge_xml(gamera_xml.LoadXML().parse_filename(filename))

   def _merge_xml(self, xml):
      
      database = [x for x in xml.glyphs if x.classification_state != core.UNCLASSIFIED]
      self.generate_features(database)
      self.merge_glyphs(database)
      self.merge_groups(xml.groups)

class NonInteractiveClassifier(_Classifier):
   def __init__(self, classifier=None, database=[], features='all',
                perform_splits=1, grouping_classifier=None):
      """classifier: the core classifier to use.  If None, defaults to kNN
      database: a list of database to initialize the classifier with.
      features: a list of strings naming the features that will be used in the classifier.
      perform_splits: (boolean) true if glyphs classified as split.* should be split.
      grouping_classifier: an optional GroupingClassifier instance.  If None, groups
                           will be remembered, but will not be automatically found."""
      
      if classifier is None:
         from gamera import knn
         classifier = knn.kNN()
      self.classifier = classifier
      if grouping_classifier is None:
##          from gamera import polargrouping
##          grouping_classifier = polargrouping.PolarGroupingClassifier([], self)
         from gamera import group
         grouping_classifier = group.GroupingClassifier([], self)
      self.grouping_classifier = grouping_classifier
      if not util.is_sequence(database) or database == []:
         raise ValueError("You can not initialize a NonInteractiveClassifier an empty database.")
      self._database = database
      self.features = features
      self.feature_functions = core.ImageBase.get_feature_functions(self.features)
      if perform_splits:
         self._do_splits = self._do_splits_impl
      else:
         self._do_splits = self._do_splits_null
      self.generate_features(database)
      self.classifier.instantiate_from_images(database)

   ########################################
   # BASIC DATABASE MANIPULATION FUNCTIONS
   def get_glyphs(self):
      return self._database
      
   def set_glyphs(self, glyphs):
      # This operation can be quite expensive depending on core classifier
      self._database = glyphs
      self.classifier.instantiate_from_images(self._database)

   def merge_glyphs(self, glyphs):
      # This operation can be quite expensive depending on core classifier
      self._database.extend(glyphs)
      self.classifier.instantiate_from_images(self._database)

   def clear_glyphs(self):
      self._database = []
      self.grouping_classifier.clear_groups()

   ########################################
   # AUTOMATIC CLASSIFICATION
   # (most of this is implemented in the base class, _Classifier)
   def guess_glyph_automatic(self, glyph):
      glyph.generate_features(self.feature_functions)
      return self.classifier.classify(glyph)

   def _classify_automatic_impl(self, glyph):
      return self.classifier.classify(glyph)

class InteractiveClassifier(_Classifier):
   _group_regex = re.compile('group\..*')
   
   def __init__(self, classifier=None, database=[], features='all',
                perform_splits=1, grouping_classifier=None):
      """classifier: the core classifier to use.  If None, defaults to kNN
      database: a list of database to initialize the classifier with. (May be []).
      features: a list of strings naming the features that will be used in the classifier.
      perform_splits: (boolean) true if glyphs classified as split.* should be split.
      grouping_classifier: an optional GroupingClassifier instance.  If None, groups
                           will be remembered, but will not be automatically found."""
      if classifier == None:
         from gamera import knn
         classifier = knn.kNN()
      self.classifier = classifier

      if grouping_classifier is None:
         from gamera import polargrouping
         grouping_classifier = polargrouping.PolarGroupingClassifier([], self)
##          from gamera import group
##          grouping_classifier = group.GroupingClassifier([])
      grouping_classifier.set_parent_classifier(self)
      self.grouping_classifier = grouping_classifier
      self.is_dirty = 0
      self._database = {}
      for key in database:
         self._database[key] = None
      self.change_feature_set(features)
      if perform_splits:
         self._do_splits = self._do_splits_impl
      else:
         self._do_splits = self._do_splits_null
      self._display = None

   ########################################
   # BASIC DATABASE MANIPULATION FUNCTIONS
   def get_glyphs(self):
      return self._database.keys()
      
   def set_glyphs(self, glyphs):
      glyphs = util.make_sequence(glyphs)
      self.is_dirty = len(glyphs) > 0
      self.clear_glyphs()
      for glyph in glyphs:
         self._database[glyph] = None

   def merge_glyphs(self, glyphs):
      glyphs = util.make_sequence(glyphs)
      self.is_dirty = len(glyphs) > 0
      for glyph in glyphs:
         self.is_dirty = 1
         self._database[glyph] = None

   def clear_glyphs(self):
      self.is_dirty = 1
      self._database = {}
      self.grouping_classifier.clear_groups()

   def change_feature_set(self, features):
      """Change the set of features used in the classifier.  features is a list of
      strings, naming the feature functions to be used."""
      self.is_dirty = 1
      self.features = features
      self.feature_functions = core.ImageBase.get_feature_functions(self.features)
      if len(self._database):
         self.generate_features(self._database.keys())
         self.classifier.instantiate_from_images(self._database.keys())
      self.grouping_classifier.change_feature_set()

   ########################################
   # AUTOMATIC CLASSIFICATION
   def _classify_automatic_impl(self, glyph):
      for child in glyph.children_images:
         if self._database.has_key(child):
            del self._database[child]
      return self.classifier.classify_with_images(self._database.iterkeys(), glyph)

   def guess_glyph_automatic(self, glyph):
      if len(self._database):
         glyph.generate_features(self.feature_functions)
         return self.classifier.classify_with_images(
            self._database.iterkeys(), glyph)
      else:
         return [(0.0, 'unknown')]

   ########################################
   # MANUAL CLASSIFICATION
   def classify_glyph_manual(self, glyph, id):
      """Classifies the given glyph using name id.  Returns a pair of lists: the glyphs
      that should be added and removed to the current database."""
      self.is_dirty = 1
      removed = []

      # Deal with grouping
      if id.startswith('group'):
         added, removed = self.grouping_classifier.classify_group_manual([glyph], id[6:])
         return added, removed
      else:
         removed = self.grouping_classifier.remove_groups_containing(glyph)

      glyph.generate_features(self.feature_functions)
      for child in glyph.children_images:
         removed.append(child)
         if self._database.has_key(child):
            del self._database[child]
      glyph.classify_manual([(0.0, id)])
      self._database[glyph] = None
      return self._do_splits(glyph), removed

   def classify_list_manual(self, glyphs, id):
      splits = []
      removed = {}
      if self.grouping_classifier and id.startswith('group'):
         added, removed = self.grouping_classifier.classify_group_manual(glyphs, id[6:])
         return added, removed

      for glyph in glyphs:
         for child in glyph.children_images:
            removed[child] = None
         group_removed = self.grouping_classifier.remove_groups_containing(glyph)
         for g in group_removed:
            removed[g] = None

      for glyph in glyphs:
         # Don't re-insert removed children glyphs
         if removed.has_key(glyph):
            continue
         self.is_dirty = 1
         if not self._database.has_key(glyph):
            self._database[glyph] = None
            glyph.generate_features(self.feature_functions)
         glyph.classify_manual([(0.0, id)])
         splits.extend(self._do_splits(glyph))

      return splits, removed.keys()

   def add_to_database(self, glyphs):
      glyphs = util.make_sequence(glyphs)
      for glyph in glyphs:
         if glyph.classification_state == MANUAL:
            self.is_dirty = 1
            self._database.append(glyph)

   def rename_ids(self, old, new):
      for glyph in self._database.iterkeys():
         self.is_dirty = 1
         new_ids = []
         for id in glyph.id_name:
            if id[1] == old:
               new_ids.append((id[0], new))
            else:
               new_ids.append(id)
         glyph.id_name = new_ids

   def noninteractive_copy(self, classifier=None):
      """Creates a noninteractive version of this classifier."""
      if len(self._database):
         if classifier is None:
            classifier = self.classifier
         return NonInteractiveClassifier(
            classifier, self.get_glyphs(),
            self.features, self.perform_splits,
            self.grouping_classifier.noninteractive_copy())
      return None

   def display(self, current_database=[],
               context_image=None, symbol_table=[]):
      gui = config.get_option("__gui")
      if gui and self._display is None:
         self._display = gui.ShowClassifier(
            self, current_database, context_image, symbol_table)
      else:
         self._display.Show(1)

   def set_display(self, display):
      self._display = display
