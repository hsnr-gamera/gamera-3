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

class _Classifier:
   def _do_action(self, glyph):
      id = glyph.get_main_id()
      if (self.perform_actions and
          id.startswith('action') and
          len(id) > 7):
         splits = getattr(glyph, id[7:]).__call__(glyph)
         glyph.children_images = splits
         return splits
      return []

   def to_xml(self, stream):
      import gamera_xml
      return gamera_xml.WriteXML(glyphs=self.database).write_stream(stream)

   def to_xml_filename(self, filename):
      import gamera_xml
      return gamera_xml.WriteXMLFile(glyphs=self.database).write_filename(filename)

   def from_xml(self, stream):
      self.database = gamera_xml.LoadXMLGlyphs().parse_stream(stream)
      for glyph in self.database:
         glyph.generate_features(self.feature_functions)

   def from_xml_filename(self, filename):
      self.database = gamera_xml.LoadXMLGlyphs().parse_filename(filename)
      for glyph in self.database:
         glyph.generate_features(self.feature_functions)

   def merge_from_xml(self, stream):
      self.database.extend(gamera_xml.LoadXMLGlyphs().parse_stream(stream))
      for glyph in self.database:
         glyph.generate_features(self.feature_functions)

   def merge_from_xml_filename(self, filename):
      self.database.extend(gamera_xml.LoadXMLGlyphs().parse_filename(filename))
      for glyph in self.database:
         glyph.generate_features(self.feature_functions)

class NonInteractiveClassifier(_Classifier):
   def __init__(self, classifier, database=[], features='all', perform_actions=1):
      self.classifier = classifier
      self.features = features
      self.feature_functions = None
      if database == []:
         raise ValueError("You must initialize a NonInteractiveClassifier with a non-zero length database.")
      self.database = database
      self.feature_functions = core.ImageBase.get_feature_functions(self.features)
      self.perform_actions = perform_actions
      self.determine_feature_functions()
      for i, glyph in enumerate(database):
         glyph.generate_features(self.feature_functions)
      self.classifier.instantiate_from_images(database)

   def classify_glyph_automatic(self, glyph):
      if (not glyph.classification_state in (core.MANUAL, core.HEURISTIC)):
         glyph.generate_features(self.feature_functions)
         id = self.classifier.classify(glyph)
         glyph.classify_automatic(id)
         return self._do_action(glyph)
      return []

   def classify_list_automatic(self, glyphs, recursion_level=0):
      if recursion_level > 10:
         return []
      splits = []
      for glyph in glyphs:
         if (not glyph.classification_state in (core.MANUAL, core.HEURISTIC)):
            glyph.generate_features(self.feature_functions)
            id = self.classifier.classify(glyph)
            glyph.classify_automatic(id)
            splits.extend(self._do_action(glyph))
      if len(splits):
         splits.extend(self.classify_list_automatic(splits, recursion_level+1))
      return splits

   def from_xml(self, stream):
      _Classifier.from_xml(self, stream)
      self.classifier.instantiate_from_images(self.database)

   def from_xml_filename(self, filename):
      _Classifier.from_xml_filename(self, filename)
      self.classifier.instantiate_from_images(self.database)

   def merge_from_xml(self, stream):
      _Classifier.merge_from_xml(self, stream)
      self.classifier.instantiate_from_images(self.database)

   def merge_from_xml_filename(self, filename):
      _Classifier.merge_from_xml(self, filename)
      self.classifier.instantiate_from_images(self.database)

class InteractiveClassifier(_Classifier):
   def __init__(self, classifier=None, database=[], features='all', perform_actions=1):
      if classifier == None:
         from gamera import knn
         classifier = knn.kNN()
      self.classifier = classifier
      self.is_dirty = 0
      self.database = {}
      for key in database:
         self.database[key] = None
      self.change_feature_set(features)
      self.perform_actions = perform_actions

   def change_feature_set(self, features):
      self.features = features
      self.feature_functions = core.ImageBase.get_feature_functions(self.features)
      progress = util.ProgressFactory("Generating features...")
      for i, glyph in enumerate(self.database.iterkeys()):
         glyph.generate_features(self.feature_functions)
         progress.update(i, len(self.database))
      progress.update(1, 1)
                         
   def guess_glyph(self, glyph):
      if len(self.database):
         glyph.generate_features(self.feature_functions)
         return self.classifier.classify_with_images(
            self.database.iterkeys(), glyph)
      else:
         return [(0.0, 'unknown')]

   def classify_glyph_manual(self, glyph, id):
      self.is_dirty = 1
      if self.database.has_key(glyph):
         del self.database[glyph]
      glyph.generate_features(self.feature_functions)
      for child in glyph.children_images:
         if self.database.has_key(child):
            del self.database[child]
      glyph.classify_manual([(1.0, id)])
      self.database[glyph] = None

   def classify_list_manual(self, glyphs, id):
      self.is_dirty = 1
      splits = []
      for glyph in glyphs:
         if not self.database.has_key(glyph):
            self.database[glyph] = None
            glyph.generate_features(self.feature_functions)
         for child in glyph.children_images:
            if self.database.has_key(child):
               del self.database[child] 
         glyph.classify_manual([(1.0, id)])
         splits.extend(self._do_action(glyph))
      return splits

   def classify_glyph_automatic(self, glyph):
      if not glyph.classification_state in (core.MANUAL, core.HEURISTIC):
         glyph.generate_features(self.feature_functions)
         id = self.classifier.classify_with_images(self.database.iterkeys(), glyph)
         glyph.classify_automatic(id)
         return self._do_action(glyph)
      return []

   def classify_list_automatic(self, glyphs, recursion_level=0, progress=None,
                               progress_i=0, progress_len=0):
      if self.database == []:
         return []
      if recursion_level > 10:
         return []
      splits = []
      if recursion_level == 0:
         progress = util.ProgressFactory("Classifying glyphs...")
      progress_len += len(glyphs)
      for glyph in glyphs:
         if not glyph.classification_state in (core.MANUAL, core.HEURISTIC):
            glyph.generate_features(self.feature_functions)
            id = self.classifier.classify_with_images(self.database.iterkeys(), glyph)
            glyph.classify_automatic(id)
            splits.extend(self._do_action(glyph))
            progress.update(progress_i, progress_len + len(splits))
            progress_i += 1
      if len(splits):
         splits.extend(self.classify_list_automatic(
            splits, recursion_level+1, progress, progress_i, progress_len))
      if recursion_level == 0:
         progress.update(1, 1)
      return splits

   def add_to_database(self, glyphs):
      if not util.is_sequence(glyphs):
         glyphs = [glyphs]
      for glyph in glyphs:
         if glyph.classification_state == MANUAL:
            self.is_dirty = 1
            self.database.append(glyph)

   def rename_ids(self, old, new):
      for glyph in self.database:
         new_ids = []
         for id in glyph.id_name:
            if id[1] == old:
               new_ids.append((id[0], new))
            else:
               new_ids.append(id)
         glyph.id_name = new_ids

   def display(self, current_database, context_image=None, symbol_table=[]):
      gui = config.get_option("__gui")
      display = gui.ShowClassifier(self, current_database, context_image, symbol_table)
