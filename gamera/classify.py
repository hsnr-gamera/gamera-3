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
contain a reference to a core classifier class (unusally written in C++), and an
optional GroupingClassifier.  They add functionality for XML loading/saving,
splitting/grouping, and keeping track of a database of glyphs (in the
Interactive case.)"""

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

   def merge_groups(self, groups):
      self.grouping_classifier.merge_groups(groups)

   def group_list_automatic(self, glyphs):
      return self.grouping_classifier.search(glyphs)

   ########################################
   # AUTOMATIC CLASSIFICATION
   def classify_glyph_automatic(self, glyph):
      """Classifies a glyph using the core classifier.  Sets the
      classification_state and id_name of the glyph.  (If you don't want it set,
      use guess_glyph_automatic.)  Returns a pair of lists: glyphs to be added
      to the current database, and glyphs to be removed from the current
      database."""
      # Since we only have one glyph to classify, we can't do any grouping
      if (len(self._database) and
          glyph.classification_state in (core.UNCLASSIFIED, core.AUTOMATIC)):
         removed = glyph.children_images
         id = self._classify_automatic_impl(glyph)
         glyph.classify_automatic(id)
         splits = self._do_splits(glyph)
         return splits, removed
      return [], []

   def classify_list_automatic(self, glyphs, recursion_level=0, progress=None):
      """Classifies a list of glyphs using the core classifier.  Sets the
      classification_state and id_name of the glyph.  (If you don't want it set,
      use guess_glyph_automatic.)  Returns a pair of lists: glyphs to be added
      to the current database, and glyphs to be removed from the current
      database. The keyword arguments are for internal use only."""

      # There is a slightly convoluted handling of the progress bar here, since
      # this function is called recursively on split glyphs
      if recursion_level == 0:
         progress = util.ProgressFactory("Classifying glyphs...")
      try:
         if (recursion_level > 10) or len(self._database) == 0:
            return [], []
         progress.add_length(len(glyphs))
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
                  added.extend(self._do_splits(glyph))
            progress.step()
         if len(added):
            added_recurse, removed_recurse = self.classify_list_automatic(
               added, recursion_level+1, progress)
            added.extend(added_recurse)
            for glyph in removed_recurse:
               removed[glyph] = None
      finally:
         if recursion_level == 0:
            progress.kill()
      return added, removed.keys()

   # Since splitting glyphs is optional (when the classifier instance is
   # created) we have two versions of this function, so that there need not
   # be an 'if' statement everywhere.
   def _do_splits_impl(self, glyph):
      id = glyph.get_main_id()
      if (id.startswith('split')):
         parts = id.split('.')
         if (len(parts) != 2 or not hasattr(glyph, parts[1])):
            raise ClassifierError("'%s' is not a known or valid split function." % parts[1])
         splits = getattr(glyph, id[6:]).__call__(glyph)
         glyph.children_images = splits
         return splits
      return []

   def _do_splits_null(self, glyph):
      return []

   ########################################
   # XML
   # Note that unclassified glyphs in the XML file are ignored.
   def to_xml(self, stream):
      self.is_dirty = 0
      return gamera_xml.WriteXML(
         glyphs=self.get_glyphs(),
         groups=self.get_groups()).write_stream(stream)

   def to_xml_filename(self, filename):
      self.is_dirty = 0
      return gamera_xml.WriteXMLFile(
         glyphs=self.get_glyphs(),
         groups=self.get_groups()).write_filename(filename)

   def from_xml(self, stream):
      self._from_xml(gamera_xml.LoadXML().parse_stream(stream))

   def from_xml_filename(self, filename):
      self._from_xml(gamera_xml.LoadXML().parse_filename(filename))

   def _from_xml(self, xml):
      database = [x for x in xml.glyphs
                  if x.classification_state != core.UNCLASSIFIED]
      self.set_glyphs(database)
      self.set_groups(xml.groups)

   def merge_from_xml(self, stream):
      self._merge_xml(gamera_xml.LoadXML().parse_stream(stream))

   def merge_from_xml_filename(self, filename):
      self._merge_xml(gamera_xml.LoadXML().parse_filename(filename))

   def _merge_xml(self, xml):
      database = [x for x in xml.glyphs
                  if x.classification_state != core.UNCLASSIFIED]
      self.merge_glyphs(database)
      self.merge_groups(xml.groups)

   ##############################################
   # Settings
   def load_settings(self, filename):
      ff = self.get_feature_functions()
      self.classifier.load_settings(filename)
      if ff != self.get_feature_functions():
         self.generate_features(self._database)

   def save_settings(self, filename):
      self.classifier.save_settings(filename)

   def supports_settings_dialog(self):
      return self.classifier.supports_settings_dialog()

   def settings_dialog(self, parent):
      self.classifier.settings_dialog(parent)
      
   ##############################################
   # Features
   def get_feature_functions(self):
      return self.classifier.feature_functions

   def change_feature_set(self, features):
      """Change the set of features used in the classifier.  features is a list
      of strings, naming the feature functions to be used."""
      self.is_dirty = 1
      self.classifier.change_feature_set(features)
      if len(self._database):
         self.generate_features(self._database)
      self.grouping_classifier.change_feature_set()
   
   def generate_features(self, glyphs):
      """Generates features for all the given glyphs."""
      progress = util.ProgressFactory("Generating features...", len(glyphs) / 10)
      feature_functions = self.get_feature_functions()
      try:
         for i, glyph in enumerate(glyphs):
            glyph.generate_features(feature_functions)
            if i % 10 == 0:
               progress.step()
      finally:
         progress.kill()   

class NonInteractiveClassifier(_Classifier):
   def __init__(self, classifier=None, database=[], features='all',
                perform_splits=1, grouping_classifier=None):
      """classifier: the core classifier to use.  If None, defaults to kNN
      database: a list of database to initialize the classifier with.
      features: a list of strings naming the features that will be used in the
                classifier.
      perform_splits: (boolean) true if glyphs classified as split.* should be
                      split.
      grouping_classifier: an optional GroupingClassifier instance.  If None,
                           groups will be remembered, but will not be
                           automatically found."""
      if classifier is None:
         from gamera import knn
         classifier = knn.kNN()
      self.classifier = classifier
      self.classifier.change_feature_set(features)
      if grouping_classifier is None:
         from gamera import group
         grouping_classifier = group.GroupingClassifier([], self)
      self.grouping_classifier = grouping_classifier
##       if not util.is_sequence(database) or database == []:
##          raise ValueError(
##             "You can not initialize a NonInteractiveClassifier an empty database.")
      if database != []:
         self._database = database
         self.change_feature_set(features)
         self.classifier.instantiate_from_images(database)
      else:
         self._database = [0]

      if perform_splits:
         self._do_splits = self._do_splits_impl
      else:
         self._do_splits = self._do_splits_null

   def is_interactive():
      return 0
   is_interactive = staticmethod(is_interactive)

   ########################################
   # BASIC DATABASE MANIPULATION FUNCTIONS
   def get_glyphs(self):
      return self._database
      
   def set_glyphs(self, glyphs):
      # This operation can be quite expensive depending on core classifier
      self.generate_features(glyphs)
      self._database = glyphs
      self.classifier.instantiate_from_images(self._database)

   def merge_glyphs(self, glyphs):
      # This operation can be quite expensive depending on core classifier
      self.generate_features(glyphs)
      self._database.extend(glyphs)
      self.classifier.instantiate_from_images(self._database)

   def clear_glyphs(self):
      self._database = []
      self.grouping_classifier.clear_groups()

   def load_settings(self, filename):
      _Classifier.load_settings(filename)
      self.classifier.instantiate_from_images(self._database)      

   ########################################
   # AUTOMATIC CLASSIFICATION
   # (most of this is implemented in the base class, _Classifier)
   def guess_glyph_automatic(self, glyph):
      glyph.generate_features(self.get_feature_functions())
      return self.classifier.classify(glyph)

   def _classify_automatic_impl(self, glyph):
      return self.classifier.classify(glyph)

class InteractiveClassifier(_Classifier):
   _group_regex = re.compile('group\..*')
   
   def __init__(self, classifier=None, database=[], features='all',
                perform_splits=1, grouping_classifier=None):
      """classifier: the core classifier to use.  If None, defaults to kNN
      database: a list of database to initialize the classifier with. (May be []).
      features: a list of strings naming the features that will be used in the
                classifier.
      perform_splits: (boolean) true if glyphs classified as split.* should be
                      split.
      grouping_classifier: an optional GroupingClassifier instance.  If None,
                           groups will be remembered, but will not be
                           automatically found."""
      if classifier == None:
         from gamera import knn
         classifier = knn.kNN()
      if not classifier.supports_interactive():
         raise ClassifierError(
            "InteractiveClassifier must be initialised with a" +
            "classifier that supports interaction.")                   
      self.classifier = classifier

      if grouping_classifier is None:
         from gamera import genericgrouping
         grouping_classifier = genericgrouping.GenericGroupingClassifier([], self)
         
      grouping_classifier.set_parent_classifier(self)
      self.grouping_classifier = grouping_classifier
      self.is_dirty = 0
      self.clear_glyphs()
      for glyph in database:
         self._database[glyph] = None
      self.change_feature_set(features)
      if perform_splits:
         self._do_splits = self._do_splits_impl
      else:
         self._do_splits = self._do_splits_null
      self._display = None

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
      self.is_dirty = 1
      self._database = {}
      self.grouping_classifier.clear_groups()

   ########################################
   # AUTOMATIC CLASSIFICATION
   def _classify_automatic_impl(self, glyph):
      if len(self._database) == 0:
         raise ClassifierError(
            "Cannot classify using an empty production database.")
      for child in glyph.children_images:
         if self._database.has_key(child):
            del self._database[child]
      return self.classifier.classify_with_images(self._database, glyph)

   def guess_glyph_automatic(self, glyph):
      if len(self._database):
         glyph.generate_features(self.get_feature_functions())
         return self.classifier.classify_with_images(
            self._database, glyph)
      else:
         return [(0.0, 'unknown')]

   ########################################
   # MANUAL CLASSIFICATION
   def classify_glyph_manual(self, glyph, id):
      """Classifies the given glyph using name id.  Returns a pair of lists: the
      glyphs that should be added and removed to the current database."""
      self.is_dirty = 1

      # Deal with grouping
      if id.startswith('group'):
         added, removed = self.grouping_classifier.classify_group_manual([glyph], id[6:])
         return added, removed
      else:
         removed = self.grouping_classifier.remove_groups_containing(glyph)

      for child in glyph.children_images:
         removed.append(child)
         if self._database.has_key(child):
            del self._database[child]
      glyph.classify_manual([(1.0, id)])
      glyph.generate_features(self.get_feature_functions())
      self._database[glyph] = None
      return self._do_splits(glyph), removed

   def classify_list_manual(self, glyphs, id):
      if self.grouping_classifier and id.startswith('group'):
         added, removed = self.grouping_classifier.classify_group_manual(glyphs, id[6:])
         return added, removed

      added = []
      removed = {}

      for glyph in glyphs:
         for child in glyph.children_images:
            removed[child] = None
         group_removed = self.grouping_classifier.remove_groups_containing(glyph)
         for g in group_removed:
            removed[g] = None

      feature_functions = self.get_feature_functions()
      for glyph in glyphs:
         # Don't re-insert removed children glyphs
         if not removed.has_key(glyph):
            self.is_dirty = 1
            if not self._database.has_key(glyph):
               glyph.generate_features(feature_functions)
               self._database[glyph] = None
            glyph.classify_manual([(1.0, id)])
            added.extend(self._do_splits(glyph))

      return added, removed.keys()

   def add_to_database(self, glyphs):
      glyphs = util.make_sequence(glyphs)
      feature_functions = self.get_feature_functions()
      for glyph in glyphs:
         if (glyph.classification_state == core.MANUAL and
             not self._database.has_key(glyph)):
            self.is_dirty = 1
            glyph.generate_features(feature_functions)
            self._database[glyph] = None

   def remove_from_database(self, glyphs):
      glyphs = util.make_sequence(glyphs)
      for glyph in glyphs:
         if self._database.has_key(glyph):
            self.is_dirty = 1
            del self._database[glyph]

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
            self.classifier.features, self.perform_splits,
            self.grouping_classifier.noninteractive_copy())
      raise ClassifierError(
         "Cannot create a noninteractive copy of an empty classifier.")

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
