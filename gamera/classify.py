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

class ClassifierError(Exception):
   pass

class _Classifier:
   def get_name(self):
      return self.classifier.__class__.__name__

   def classify_glyph_automatic(self, glyph):
      # Since we only have one glyph to classify, we can't do any grouping
      if (len(self.database) and
          not glyph.classification_state in (core.MANUAL, core.HEURISTIC)):
         glyph.generate_features(self.feature_functions)
         id = self._classify_automatic_impl(glyph)
         glyph.classify_automatic(id)
         splits = self._do_splits(glyph)
         return splits
      return []

   def classify_list_automatic(self, glyphs, recursion_level=0, progress=None,
                               progress_i=0, progress_len=0):
      if (len(self.database) == 0 or recursion_level > 10):
         return []
      splits = []
      if recursion_level == 0:
         progress = util.ProgressFactory("Classifying glyphs...")
      #try: # Big try block so the progress display goes away if there is an error
      if 1:
         progress_len += len(glyphs)
         for glyph in glyphs:
            if not glyph.classification_state == core.MANUAL:
               glyph.generate_features(self.feature_functions)
               id = self._classify_automatic_impl(glyph)
               glyph.classify_automatic(id)
               splits.extend(self._do_splits(glyph))
               progress.update(progress_i, progress_len + len(splits))
               progress_i += 1
         if len(splits):
            splits.extend(self.classify_list_automatic(
               splits, recursion_level+1, progress, progress_i, progress_len))
         if recursion_level == 0:
            progress.update(1, 1)
            # TODO: should we group before or after splitting?
            if self.grouping_classifier:
               groups = self.grouping_classifier.search(glyphs + splits)
            return splits + groups
         return splits
##       except Exception, e:
##          progress.update(1, 1)
##          raise e

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
      progress = util.ProgressFactory("Generating features...")
      for i, glyph in util.enumerate(glyphs):
         glyph.generate_features(self.feature_functions)
         progress.update(i, len(glyphs))
      progress.update(1,1)

   def to_xml(self, stream):
      import gamera_xml
      return gamera_xml.WriteXML(
         glyphs=self.get_glyphs(),
         groups=self.grouping_classifier.get_groups()).write_stream(stream)

   def to_xml_filename(self, filename):
      import gamera_xml
      return gamera_xml.WriteXMLFile(
         glyphs=self.get_glyphs(),
         groups=self.grouping_classifier.get_groups()).write_filename(filename)

   def from_xml(self, stream):
      self._from_xml(gamera_xml.LoadXML().parse_stream(stream))

   def from_xml_filename(self, filename):
      self._from_xml(gamera_xml.LoadXML().parse_filename(filename))

   def _from_xml(self, xml):
      database = [x for x in xml.glyphs if x.classification_state != core.UNCLASSIFIED]
      self.generate_features(database)
      self.set_glyphs(database)
      self.grouping_classifier.set_groups(xml.groups)

   def merge_from_xml(self, stream):
      self._merge_xml(gamera_xml.LoadXML().parse_stream(stream))

   def merge_from_xml_filename(self, filename):
      self._merge_xml(gamera_xml.LoadXML().parse_filename(filename))

   def _merge_xml(self, xml):
      database = [x for x in xml.glyphs if x.classification_state != core.UNCLASSIFIED]
      self.generate_features(database)
      self.merge_glyphs(database)
      self.grouping_classifier.merge_groups(xml.groups)

class _NullGroupingClassifier:
   def __init__(self, groups=[]):
      pass

   def get_groups(self):
      return []

   def set_groups(self, groups):
      pass

   def merge_groups(self, groups):
      pass

   def clear_groups(self):
      pass

class NonInteractiveClassifier(_Classifier):
   def __init__(self, classifier=None, database=[], features='all', perform_splits=1, grouping_classifier=None):
      if classifier is None:
         from gamera import knn
         classifier = knn.kNN()
      self.classifier = classifier
      if database == []:
         raise ValueError("You must initialize a NonInteractiveClassifier with a non-zero length database.")
      self.database = database
      self.features = features
      self.feature_functions = core.ImageBase.get_feature_functions(self.features)
      if perform_splits:
         self._do_splits = self._do_splits_impl
      else:
         self._do_splits = self._do_splits_null
      if grouping_classifier is None:
         from gamera import structure
         grouping_classifier = structure.GroupingClassifier()
      self.grouping_classifier = grouping_classifier
      for i, glyph in enumerate(database):
         glyph.generate_features(self.feature_functions)
      self.classifier.instantiate_from_images(database)

   def get_glyphs(self):
      return self.database
      
   def set_glyphs(self, glyphs):
      self.database = glyphs
      self.classifier.instantiate_from_images(self.database)

   def merge_glyphs(self, glyphs):
      self.database.extend(glyphs)
      self.classifier.instantiate_from_images(self.database)

   def clear_glyphs(self):
      self.database = []
      self.grouping_classifier.clear_groups()

   def _classify_automatic_impl(self, glyph):
      return self.classifier.classify(glyph)

class InteractiveClassifier(_Classifier):
   def __init__(self, classifier=None, database=[], features='all', perform_splits=1, grouping_classifier=None):
      if classifier == None:
         from gamera import knn
         classifier = knn.kNN()
      self.classifier = classifier
      self.is_dirty = 0
      self.database = {}
      for key in database:
         self.database[key] = None
      self.change_feature_set(features)
      if perform_splits:
         self._do_splits = self._do_splits_impl
      else:
         self._do_splits = self._do_splits_null
      if grouping_classifier is None:
         from gamera import structure_by_knn
         grouping_classifier = structure_by_knn.GroupingClassifier()
      self.grouping_classifier = grouping_classifier
      self._display = None

   def get_glyphs(self):
      return self.database.keys()
      
   def set_glyphs(self, glyphs):
      glyphs = util.make_sequence(glyphs)
      self.is_dirty = len(glyphs) > 0
      self.database = {}
      for glyph in glyphs:
         self.is_dirty = 1
         self.database[glyph] = None

   def merge_glyphs(self, glyphs):
      glyphs = util.make_sequence(glyphs)
      self.is_dirty = len(glyphs) > 0
      for glyph in glyphs:
         self.is_dirty = 1
         self.database[glyph] = None

   def clear_glyphs(self):
      self.database = {}
      self.grouping_classifier.clear_groups()

   def change_feature_set(self, features):
      self.features = features
      self.feature_functions = core.ImageBase.get_feature_functions(self.features)
      if len(self.database):
         progress = util.ProgressFactory("Generating features...")
         for i, glyph in enumerate(self.database.iterkeys()):
            glyph.generate_features(self.feature_functions)
            progress.update(i, len(self.database))
         progress.update(1, 1)

   def _classify_automatic_impl(self, glyph):
      return self.classifier.classify_with_images(self.database.iterkeys(), glyph)
                         
   def guess_glyph(self, glyph):
      if len(self.database):
         glyph.generate_features(self.feature_functions)
         return self.classifier.classify_with_images(
            self.database.iterkeys(), glyph)
      else:
         return [(0.0, 'unknown')]

   def classify_glyph_manual(self, glyph, id):
      self.is_dirty = 1
      if id.startswith('group'):
         raise ClassifierError('You can not train a single glyph as a group.')
      if self.database.has_key(glyph):
         del self.database[glyph]
      glyph.generate_features(self.feature_functions)
      removed = []
      for child in glyph.children_images:
         removed.append(child)
         if self.database.has_key(child):
            del self.database[child]
      glyph.classify_manual([(0.0, id)])
      self.database[glyph] = None
      return self._do_splits(glyph), removed

   def classify_list_manual(self, glyphs, id):
      if id.startswith('group') and self.grouping_classifier:
         if len(glyphs) <= 1:
            raise ClassifierError('You can not train a single glyph as a group.')
         groups = self.grouping_classifier.classify_group_manual(glyphs, id)
         return groups, []
      splits = []
      removed = {}
      for glyph in glyphs:
         for child in glyph.children_images:
            removed[child] = None
            if self.database.has_key(child):
               del self.database[child] 
      for glyph in glyphs:
         # Don't re-insert removed children glyphs
         if removed.has_key(glyph):
            continue
         self.is_dirty = 1
         if not self.database.has_key(glyph):
            self.database[glyph] = None
            glyph.generate_features(self.feature_functions)
         glyph.classify_manual([(0.0, id)])
         splits.extend(self._do_splits(glyph))
      return splits, removed.keys()

   def add_to_database(self, glyphs):
      glyphs = util.make_sequence(glyphs)
      for glyph in glyphs:
         if glyph.classification_state == MANUAL:
            self.is_dirty = 1
            self.database.append(glyph)

   def rename_ids(self, old, new):
      for glyph in self.database.iterkeys():
         self.is_dirty = 1
         new_ids = []
         for id in glyph.id_name:
            if id[1] == old:
               new_ids.append((id[0], new))
            else:
               new_ids.append(id)
         glyph.id_name = new_ids

   def noninteractive_copy(self, classifier=None):
      if len(self.database):
         if classifier is None:
            classifier = self.classifier.__class__()
         return NonInteractiveClassifier(
            classifier, self.get_glyphs(),
            self.features, self.perform_splits)
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
