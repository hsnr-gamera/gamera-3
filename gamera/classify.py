#
# Copyright (C) 2001 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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

import gamera # grab all of the standard gamera modules
import string, sys, random, os, os.path
import util, paths, database
import knn

class SymbolTable:
   def __init__(self):
      self.categories = {}
      self.all_ids = {}
      self.all_names = {}
      self.listeners = []
      self.rename_listeners = []
      self.add("skip", 0)

   def add_listener(self, listener):
      assert hasattr(listener, 'symbol_table_callback')
      self.listeners.append(listener)

   def remove_listener(self, listener):
      self.listeners.remove(listener)

   def alert_listeners(self, symbol):
      for l in self.listeners:
         if hasattr(l, 'symbol_table_callback'):
            l.symbol_table_callback(symbol)

   def add_rename_listener(self, listener):
      assert hasattr(listener, 'symbol_table_rename_callback')
      self.rename_listeners.append(listener)

   def remove_rename_listener(self, listener):
      self.rename_listeners.remove(listener)

   def alert_rename_listeners(self, a, b):
      for l in self.rename_listeners:
         if hasattr(l, 'symbol_table_rename_callback'):
            l.symbol_table_rename_callback(a, b)

   def normalize_symbol(self, symbol):
      assert type(symbol) == type('')
      if symbol == '':
         return '', []
      # Split by '.' delimiters
      tokens = string.split(string.strip(symbol), '.')
      # Remove internal whitespace
      tokens = map(lambda x: string.join(string.split(x), '_'), tokens)
      symbol = string.join(tokens, '.')
      if symbol[-1] == ".":
         symbol = symbol[:-1]
      return symbol, tokens

   def get_new_id(self):
      id = random.randrange(sys.maxint)
      while self.all_ids.has_key(id):
         id = random.randrange(sys.maxint)
      return id

   def add(self, symbol, id = -1):
      symbol, tokens = self.normalize_symbol(symbol)
      if self.all_names.has_key(symbol):
         return self.all_names[symbol]
      id = self.add_to_tree(symbol, tokens, id)
      return id

   def add_if_not_exists(self, symbol, id = -1):
      symbol, tokens = self.normalize_symbol(symbol)
      if self.all_names.has_key(symbol):
         return self.get_id_by_name(symbol)
      else:
         return self.add(symbol, id)

   # When we add to the tree, the alert listeners need to be
   # informed of the stem of the symbol up to where we start adding
   # new braches
   def add_to_tree(self, symbol, tokens, id=-1, do_alert=1):
      category = self.categories
      alert = []
      add_to_alert = 1
      for i in range(len(tokens)):
         token = tokens[i]
         if not category.has_key(token):
            category[token] = { }
            if token != tokens[-1] or id == -1:
               new_id = self.get_new_id()
            else:
               new_id = id
            name = string.join(tokens[0:i+1], ".")
            self.all_ids[new_id] = name
            self.all_names[name] = new_id
         category = category[token]
         if category == {}:
            add_to_alert = 0
         if add_to_alert:
            alert.append(token)
      if do_alert:
         self.alert_listeners(string.join(alert, '.'))
      return self.all_names[symbol]

   def remove(self, symbol):
      symbol, tokens = self.normalize_symbol(symbol)
      if symbol == '':
         self.categories = {}
         self.all_names = {}
         self.all_ids = {}
         self.alert_listeners('')
         return
      old_category = None
      category = self.categories
      for token in tokens:
         old_category = category
         category = category[token]
      removed = []
      for subcat in category.keys():
         removed.extend(self.remove(symbol + "." + subcat))
      removed.append(self.all_ids[symbol])
      del old_category[tokens[-1]]
      try:
         id = self.all_names[symbol]
         del self.all_ids[id]
         del self.all_names[symbol]
      except:
         pass
      self.alert_listeners(string.join(tokens[:-1], '.'))
      return removed

   def rename(self, a, b):
      a, x = self.normalize_symbol(a)
      b, x = self.normalize_symbol(b)
      for key, value in self.all_names.items():
         if key.startswith(a):
            del self.all_names[key]
            self.all_names[b + key[len(a):]] = value
      self.all_ids = {}
      self.categories = {}
      for key, value in self.all_names.items():
         self.all_ids[value] = key
      for key, value in self.all_names.items():
         symbol, tokens = self.normalize_symbol(key)
         self.add_to_tree(symbol, tokens, do_alert=0)
      self.alert_rename_listeners(a, b)
      self.alert_listeners(symbol)

   def autocomplete(self, symbol):
      symbol, tokens = self.normalize_symbol(symbol)
      try:
         category = self.get_category_contents(string.join(tokens[:-1], "."))
      except:
         return symbol
      find = tokens[-1]
      num_found = 0
      for x in category.keys():
         if x.startswith(find):
            found = x
            num_found = num_found + 1
         if num_found >= 2:
            break
      if num_found == 1:
         result = string.join(tokens[:-1] + [found], ".")
         if category[found] != {}:
            result = result + "."
         return result
      else:
         return symbol

   def get_category_contents(self, symbol):
      symbol, tokens = self.normalize_symbol(symbol)
      category = self.categories
      for token in tokens:
         category = category[token]
      return category

   def get_category_contents_list(self, symbol):
      category = self.get_category_contents(symbol)
      result = []
      for key, val in category.items():
         result.append((key, (val != {})))
      result.sort()
      return result

   def get_name_by_id(self, id):
      return self.all_ids[id]

   def exists(self, symbol):
      return self.all_names.has_key(symbol)

   def get_id_by_name(self, name):
      name, tokens = self.normalize_symbol(name)
      return self.all_names[name]


class Classifier:
   def __init__(self, glyphs=[], production_database=None,
                current_database=None, symbol_table=None, parent_image=None,
                features=None, gui=0):
      self._display = None
      self.gui = gamera.gui
      self.parent_image = None
      self.is_dirty = 0
      self.production_database_filename = production_database
      self.current_database_filename = current_database
      self.symbol_table = SymbolTable()

      #####################################################
      # These will either be set to defaults later or set by
      # _load_production_database
      # the number of k
      self.num_k = 1
      # The feature gathering functions
      self.feature_functions = []
      self.features_list = []
      # The number of features - this is not necessarily the same as the
      # len of feature_functions because the functions can return lists
      self.num_features = None

      if self.gui != None or gui:
         import args
         self.progress_dialog = args.ProgressDialog("Completing task...")
         self.progress = self.progress_dialog.Update
      else:
         self.progress = self.dev_null

      try:
         if (symbol_table):
            self.load_symbol_table(symbol_table)
         self.progress(10)

         #######################################################################
         # For now, the current_database is just a filename for saving. The real
         # current database is the glyphs that were passed in
         if (current_database):
            self.load_current_database(self.current_database_filename)
         else:
            self.current_database = glyphs
         self.progress(30)

         ###################################################################
         # Load the databases - both the production database and the current
         # database are just a list of CC
         self.knn_database = None
         if (production_database):
            self.load_production_database(self.production_database_filename)
         else:
            self.production_database = []
            # because we didn't have a production database file we need to set
            # up some additional things

         # Find feature functions, if we haven't already loaded them from
         # a file
         if self.feature_functions == []:
            self.num_features = 0
            if features == None:
               import find_members # this is to find all of the feature functions
               ff = find_members.find_methods_flat_category(
                  self.current_database[0], "Features")
               assert(ff != [])
               for x in ff:
                  self.feature_functions.append([x[0], 0])
            else:
               for f in features:
                  self.feature_functions.append([f, 0])
         self.progress(50)

         if self.num_features == 0:
            self.find_number_of_features_in_each_function()
            self.num_features = len(self.features_list)

         if self.current_database != []:
            prog = 30.0 / len(self.current_database)
            if not current_database:
               for i in range(len(self.current_database)):
                  current = self.current_database[i]
                  self.set_features(current)
                  self.progress(int(50 + i * prog))
         self.progress(80)

         # get the number of features
         if self.knn_database == None:
            self.knn_database = knn.FloatDatabase(self.num_k,
                                                  self.num_features)

         if self.current_database != []:
            prog = 20.0 / len(self.current_database)
            for i in range(len(self.current_database)):
               g = self.current_database[i]
               if not g.unclassified():
                  try:
                     self.symbol_table.get_id_by_name(g.id_name[0])
                  except:
                     g.id_name = []
                     g.classification_state = 0
                     continue
                  if g.manually_classified():
                     self.add_glyph_to_database(x)
               self.progress(int(80 + i * prog))

         self.progress(99)
         import find_members
         sf = find_members.find_methods_flat_category(self.current_database[0],
                                                      "Action")
         for f in sf:
            self.symbol_table.add_if_not_exists("action." + f[0])

         self.progress(100)
         self.symbol_table.add_rename_listener(self)
      except Exception, e:
         if self.gui != None:
            self.progress_dialog.Destroy()
            self.progress = None
            self.progress_dialog = None
         raise e


   def __del__(self):
      self.symbol_table.remove_rename_listener(self)

   # This will change the list of features that the database is currently
   # using, regenerate the features for all of the glyphs in the current
   # and production database, and recreate the knn database. This is a
   # fairly intrusive change to a running classifier so care must be taken.
   # Also this is _very_ expensive, especially if we are dealing with a lot
   # of glyphs. It is possible that we could try and see if the passed in
   # list of features is the same as the ones that we are currently using,
   # but that doesn't seem fool-proof. If nothing else, this method can be
   # used to force the regeneration of the features with the feature-editor
   # wizard, which is usefule. Detecting an unchanged list of features would
   # break that.
   def update_features(self, features):
      # Set up all of the members that hold information about the feature
      # functions that we are using.
      for f in features:
         self.feature_functions.append([f, 0])
      self.find_number_of_features_in_each_function()
      self._regenerate_features()

   def update_features_enumerated(self, feature_functions):
      self.feature_functions = feature_functions
      self.set_extended_feature_names()
      self._regenerate_features()

   def find_number_of_features_in_each_function(self):
      tmp_glyph = gamera.Image(10, 10, "OneBit")
      for f in self.feature_functions:
         features = getattr(tmp_glyph, f[0])()
         try:
            f[1] = len(features)
         except:
            f[1] = 1
      self.set_extended_feature_names()

   def set_extended_feature_names(self):
      self.features_list = []
      for x in self.feature_functions:
         for y in range(x[1]):
            self.features_list.append("%s-%02d" % (x[0], y))

   def _regenerate_features(self):
      # Re-generate the features for the current and production database.
      # This is an expensive thing to do, but it is necessary to make
      # certain that everything is in sync.
      for i in range(len(self.current_database)):
         current = self.current_database[i]
         self.set_features(current)
      for i in range(len(self.production_database)):
         current = self.production_database[i]
         self.set_features(current)
         # set the id on the features
         current.features.id(self.symbol_table.get_id_by_name(current.id_name[0]))

      self.num_features = 0
      if self.current_database != []:
         self.num_features = len(self.current_database[0].features)
      elif self.production_database != []:
         self.num_features = len(self.production_database[0].features)
      assert(self.num_features > 0)
      
      # Finally, we have to clean out the knn database and recreate it. This
      # is possible because self.production database should hold all
      # of the data that the knn database has.
      self.knn_database = knn.FloatDatabase(self.num_k, self.num_features)
      if self.production_database != []:
         for x in self.production_database:
            x.unique_id = self.knn_database.add(x.features)

   def symbol_table_rename_callback(self, a, b):
      for x in self.current_database + self.production_database:
         for name in x.id_name:
            if x.id_name.startswith(a):
               x.id_name = util.replace_prefix(x.id_name, a, b)

   def dev_null(self, ignore):
      pass

   def display(self, image=None):
      assert image == None or isinstance(image, gamera.Image)
      self.parent_image = image
      if self.gui:
         if image:
            self._display = self.gui.ShowClassifier(self, image,
                                                    gamera.Image.to_string)
         else:
            self._display = self.gui.ShowClassifier(self, None, None)

   def set_features(self, glyph):
      assert isinstance(glyph, gamera.Image)
      glyph.feature_names = self.features_list
      glyph.features = knn.FeatureVector(self.num_features)
      i = 0
      for f in self.feature_functions:
         features = getattr(glyph, f[0])()
         try:
            for x in features:
               glyph.features[i] = x
               i = i + 1
         except:
            glyph.features[i] = features
            i = i + 1

   def set_id_automatic(self, glyph, id):
      id_name = self.symbol_table.get_name_by_id(id)
      if not glyph.id_name or glyph.id_name[0] != id_name:
         glyph.set_id_name_automatic(id_name)
         return 1
      else:
         return 0

   def auto_classify_all(self):
      if self.production_database == []:
         return
      # flag for updating the display if the current_database has changed
      added_to_current = 0
      removed = 0
      for x in self.current_database:
         if x.features == None:
            self.set_features(x)
         if x.unclassified() or x.automatically_classified():
            id = self.knn_database.classify(x.features)
            if self.set_id_automatic(x, id):
               removed = removed or self._remove_children(x)
               added_to_current = added_to_current + self._do_splits(x)
      if self._display and (added_to_current or removed):
         self._display.set_image(self.current_database)

   def auto_classify_glyph(self, glyph):
      assert isinstance(glyph, gamera.Image)
      if self.production_database == []:
         return
      if glyph.features == None:
         self.set_features(glyph)
      if not glyph.manually_classified() or not glyph.heuristically_classified():
         id = self.knn_database.classify(glyph.features)
         if self.set_id_automatic(glyph, id):
            removed = self._remove_children(glyph)
            did_splits = self._do_splits(glyph)
            if self._display and removed:
               self._display.set_image(self.current_database)
            elif self._display and did_splits:
               self._display.append_glyphs(self.current_database[-did_splits:])

   def guess_classify_glyph(self, glyph):
      if self.production_database == []:
         return
      if glyph.features == None:
         self.set_features(glyph)
      return self.symbol_table.get_name_by_id(self.knn_database.classify(glyph.features))

   def manual_classify_glyph(self, glyph, id, update_display=1):
      assert isinstance(glyph, gamera.Image)
      self.is_dirty = 1
      glyph.set_id_name_manual(id)
      removed = self._remove_children(glyph)
      did_splits = self._do_splits(glyph)
      self.add_glyph_to_database(glyph)
      if update_display:
         if self._display and removed:
            self._display.set_image(self.current_database)
         elif self._display and did_splits:
            self._display.append_glyphs(self.current_database[-did_splits:])
      return (did_splits, removed)

   def heuristic_classify_glyph(self, glyph, id):
      glyph.set_id_heuristic(id)

   ###########################################################################
   # This method handles the special split category - returns true if splits
   # were done, false otherwise
   def _do_splits(self, glyph):
      assert isinstance(glyph, gamera.Image)
      if glyph.action_depth > 10:
         glyph.id_name = ['action.terminated']
         return 0
      glyph_name = string.split(glyph.id_name[0], '.')
      if (len(glyph_name) > 1):
         if (glyph_name[0] == 'action'):
            new_glyphs = getattr(glyph, glyph_name[1])()
            for x in new_glyphs:
               x.action_depth = glyph.action_depth + 1
               self.auto_classify_glyph(x)
            # set the children of the image to the split images
            glyph.children_images = new_glyphs
            self.current_database.extend(new_glyphs)
            return len(new_glyphs)
      return 0

   #########################################################
   # Remove children from a glyph recursively
   def _remove_children(self, glyph):
      assert isinstance(glyph, gamera.Image)
      self.is_dirty = 1
      if glyph.children_images != []:
         todo_list = glyph.children_images
         glyph.children_images = []
         remove_from_production = []
         remove_from_current = []
         i = 0
         while i < len(todo_list):
            x = todo_list[i]
            if x.children_images != []:
               todo_list.extend(x.children_images)
               x.children_images = []
            if x.manually_classified():
               remove_from_production.append(x)
            i = i + 1
            
         i = 0
         while i < len(self.production_database):
            x = self.production_database[i]
            if x in remove_from_production:
               self.production_database = (self.production_database[:i] +
                                           self.production_database[i+1:])
               remove_from_production.remove(x)
               self.knn_database.remove_by_unique_id(x.unique_id)
               continue
            i = i + 1
            
         i = 0
         while i < len(self.current_database):
            x = self.current_database[i]
            if x in todo_list:
               self.current_database = (self.current_database[:i] +
                                        self.current_database[i+1:])
               todo_list.remove(x)
               continue
            i = i + 1
         return 1
      else:
         return 0

   def add_glyph_to_database(self, glyph):
      assert isinstance(glyph, gamera.Image)
      if glyph.id_name == []:
         return
      self.is_dirty = 1
      # We have to add it to both the glyph list and the knn database
      if glyph.features == None:
         self.set_features(glyph)
      # For now there is no modification of items so we have to delete
      # things that are in the database
      if (glyph.unique_id != None):
         self.delete_glyph_from_database(glyph)
      glyph.features.id(self.symbol_table.get_id_by_name(glyph.id_name[0]))
      id = self.knn_database.add(glyph.features)
      glyph.unique_id = id
      self.production_database.append(glyph)

   def delete_glyph_from_database(self, glyph):
      assert isinstance(glyph, gamera.Image)
      self.is_dirty = 1
      self.knn_database.remove_by_unique_id(glyph.unique_id)
      if glyph in self.production_database:
         self.production_database.remove(glyph)

   ############################################################
   # SAVING AND LOADING
   ############################################################

   def save_current_database(self, filename = None, save_images=0):
      if filename != None:
         self.current_database_filename = filename
      filename = self.current_database_filename
      # This will make certain that all of the glyphs have all of the
      # data that we need to save them (i.e. features). This fixes
      # a bug when things are classified as an action and the current
      # database is saved without guessing. KWM
      self.auto_classify_all()
      WriteXMLCurrentDatabase(self, save_images).write(filename)

   def save_production_database(self, filename = None, save_images=0):
      if filename != None:
         self.production_database_filename = filename
      filename = self.production_database_filename
      WriteXMLProductionDatabase(self, save_images).write(filename)
      self.is_dirty = 0

   def save_symbol_table(self, filename = None):
      database.WriteXMLSymbolTable(self.symbol_table).write(filename)

   def save_weights(self, filename = None):
      database.WriteXMLFeatures(self).write(filename)

   def load_current_database(self, filename=None, load_images=1):
      if filename != None:
         self.current_database_filename = filename
      self.current_database = []
      LoadXMLCurrentDatabase(self).ParseFilename(self.current_database_filename)
      
   def load_production_database(self, filename=None):
      if filename != None:
         self.production_database_filename = filename
      self.production_database = []
      LoadXMLProductionDatabase(self).ParseFilename(self.production_database_filename)

   def merge_production_database(self, filename = None):
      if filename == None:
         return
      LoadXMLProductionDatabase(self).ParseFilename(filename)

   def load_symbol_table(self, filename):
      database.LoadXMLSymbolTable(self).ParseFilename(filename)

   def load_weights(self, filename):
      database.LoadXMLFeatures(self).ParseFilename(filename)

   def print_report(self, filename):
      symbols = {}
      for x in self.production_database:
         symbol = self.symbol_table.get_name_by_id(x.id_name[0])
         if symbols.has_key(symbol):
            symbols[symbol] = symbols[symbol] + 1
         else:
            symbols[symbol] = 1
      items = symbols.items()
      items.sort()
      fd = open(filename, 'w')
      for x in items:
         fd.write("%06d\t%s\n" % (x[1], x[0]))
      fd.close()


################################################################################
# SAVING AND LOADING      

class WriteXMLDatabaseBase(database.WriteXML):
   def __init__(self, classifier, save_images=0):
      database.WriteXML.__init__(self, classifier)
      self.save_images = save_images

   def write_core_base(self, db):
      self.dirname = string.split(os.path.split(self.filename)[1], '.')[0]
      self.dirpath = os.path.join(os.path.split(self.filename)[0],
                                  self.dirname)
      if self.save_images:
         if not os.path.exists(self.dirpath):
            os.mkdir(self.dirpath)
      self.write_features_definition(self.classifier.features_list,
                                     self.classifier.knn_database.get_weights())
      self.write_symbol_table(self.classifier.symbol_table.all_names)
      self.write_glyphs(db,
                        self.save_images)

class WriteXMLCurrentDatabase(WriteXMLDatabaseBase):
   def write_core(self):
      self.write_core_base(self.classifier.current_database)

class WriteXMLProductionDatabase(WriteXMLDatabaseBase):
   def write_core(self):
      self.write_core_base(self.classifier.production_database)

class LoadXMLProductionGlyphs(database.LoadXMLGlyphsBase):
   def setup_handlers(self):
      self.add_start_element_handler('glyphs', self.ths_glyphs)
      database.LoadXMLGlyphsBase.setup_handlers(self)

   def ths_glyphs(self, a):
      self.knn_database = knn.FloatDatabase(self.classifier.num_k,
                                            self.classifier.num_features)

   def append_glyph(self, cc):
      self.classifier.add_glyph_to_database(cc)
   
class LoadXMLProductionDatabase(database.LoadXMLSymbolTable,
                                database.LoadXMLFeatures,
                                LoadXMLProductionGlyphs):
   def setup_handlers(self):
      database.LoadXMLSymbolTable.setup_handlers(self)
      database.LoadXMLFeatures.setup_handlers(self)
      LoadXMLProductionGlyphs.setup_handlers(self)

class LoadXMLCurrentGlyphs(database.LoadXMLGlyphsBase):
   def append_glyph(self, cc):
      self.classifier.current_database.append(cc)

class LoadXMLCurrentDatabase(database.LoadXMLSymbolTable,
                             database.LoadXMLFeatures,
                             LoadXMLCurrentGlyphs):
   def setup_handlers(self):
      database.LoadXMLSymbolTable.setup_handlers(self)
      database.LoadXMLFeatures.setup_handlers(self)
      LoadXMLCurrentGlyphs.setup_handlers(self)

