# vi:set tabsize=3:
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

import os.path
from wxPython.wx import *
from gamera.core import *
from gamera.args import *
from gamera.symbol_table import SymbolTable
from gamera import gamera_xml, classifier_stats, util
from gamera.classify import InteractiveClassifier, ClassifierError
from gamera.gui import image_menu, toolbar, gui_util, rule_engine_runner
from gamera.gui.gamera_display import *

###############################################################################
# CLASSIFIER DISPLAY
###############################################################################

class ClassifierMultiImageDisplay(MultiImageDisplay):
   def __init__(self, toplevel, parent):
      self.toplevel = toplevel
      MultiImageDisplay.__init__(self, parent)
      self.last_image_no = None
      self._last_selection = []
      EVT_GRID_RANGE_SELECT(self, self._OnSelect)
      self.last_sort = None
      # This is to turn off the display of row labels if a)
      # we have done some classification and b) we have done a
      # sort other than the default. KWM
      self.display_row_labels = 0
      EVT_KEY_DOWN(self, self._OnKey)

   ########################################
   # AUTO-MOVE

   def do_auto_move(self, state):
      # if auto-moving is turned off, just return
      if state == []:
         return

      if self.IsSelection():
         found = 0
         image_no = -1
         for row in xrange(0, self.cols):
            for col in xrange(0, self.rows):
               if self.IsInSelection(row, col):
                  image_no = self.get_image_no(row, col)
                  found = 1
                  break
            if found:
               break
         if not found:
            return
      else:
         row, col = self.GetGridCursorRow(), self.GetGridCursorCol()
         image_no = self.get_image_no(row, col)

      # Find the next glyph of the right type
      found = -1
      list = self.list
      for i in range(image_no + 1, len(list)):
         image = list[i]
         if (image != None and
             not hasattr(image, 'dead') and
             image.classification_state in state):
            found = i
            break
      if found != -1:
         row = found / self.cols
         col = found % self.cols
         self.SetGridCursor(row, col)
         self.SelectBlock(row, col, row, col, 0)
         self.MakeCellVisible(min(row + 1, self.rows), col)
         if image.classification_state != MANUAL:
            id = self.toplevel.guess_glyph(image)
         else:
            id = image
         self.toplevel.set_label_display(id[0][1])
         self.toplevel.display_label_at_cell(row, col, id[0][1])

   def move_back(self):
      row = self.GetGridCursorRow()
      col = self.GetGridCursorCol()
      image_no = row * self.cols + col
      image_no = max(0, image_no - 1)
      row = image_no / self.cols
      col = image_no % self.cols
      self.SetGridCursor(row, col)
      self.SelectBlock(row, col, row, col, 0)
      self.MakeCellVisible(min(row + 1, self.rows), col)
      image = self.list[image_no]
      if image.classification_state != MANUAL:
         id = self.toplevel.guess_glyph(image)
      else:
         id = image.id_name
      self.toplevel.set_label_display(id[0][1])
      self.toplevel.display_label_at_cell(row, col, id[0][1])

   ########################################
   # DISPLAYING A LABEL BENEATH A CELL

   _search_order = ((0,0),                          # center
                    (-1,0), (0,-1), (1,0), (0,1),   # +
                    (-1,-1), (-1,1), (1,-1), (1,1)) # x
   def find_glyphs_in_rect(self, x1, y1, x2, y2, shift):
      self.BeginBatch()
      if shift:
         selected = []
         if self.IsSelection():
            for row in range(self.rows):
               for col in range(self.cols):
                  if self.IsInSelection(row, col):
                     selected.append(row * self.cols + col)
      matches = []
      if x1 == x2 or y1 == y2:
         point = Point(x1, y1)
         for i in range(len(self.list)):
            g = self.list[i]
            if g != None:
               for x, y in self._search_order:
                  if (g.contains_point(Point(x1 + x, y1 + y)) and
                      (g.get(y1 + y - g.ul_y, x1 + x - g.ul_x) != 0)):
                     matches.append(i)
                     break
      else:
         matches = []
         r = Rect(y1, x1, y2 - y1 + 1, x2 - x1 + 1)
         for i in range(len(self.list)):
            g = self.list[i]
            if g != None and r.contains_rect(g):
                  matches.append(i)
      if matches != []:
         if shift:
            new_matches = ([x for x in matches if x not in selected] +
                           [x for x in selected if x not in matches])
            matches = new_matches
         if not len(matches):
            return
         first = 0
         self.updating = 1
         last_index = matches[-1]
         for index in matches:
            row = index / self.cols
            col = index % self.cols
            if index is last_index:
               self.updating = 0
            self.SelectBlock(row, col, row, col, first)
            if first == 0:
               self.MakeCellVisible(row, col)
               first = 1
      else:
         if not shift:
            self.toplevel.single_iw.id.clear_all_highlights()
            self.ClearSelection()
      self.EndBatch()

   ########################################
   # DELETION

   def delete_selected(self):
      self._delete_selected(self.GetSelectedItems(), 0)
      self.ForceRefresh()

   def _delete_selected(self, glyphs, setting):
      for glyph in glyphs:
         if (setting == 0 and hasattr(glyph, 'dead')) or setting == -1:
            if hasattr(glyph, 'dead'):
               del glyph.__dict__['dead']
            if glyph.classification_state == MANUAL:
               self.toplevel.add_to_database(glyph)
            self._delete_selected(glyph.children_images, -1)
         elif (setting == 0 and not hasattr(glyph, 'dead')) or setting == 1:
            glyph.dead = 1
            if glyph.classification_state == MANUAL:
               self.toplevel.remove_from_database(glyph)
            self._delete_selected(glyph.children_images, 1)

   ########################################
   # CALLBACKS

   # Display selected items in the context display
   def _OnSelectImpl(self):
      images = self.GetSelectedItems()
      if not self.updating:
         if images != self._last_selection:
            self._last_selection = images
         else:
            return
         ids = {}
         if images != []:
            for x in images:
               ids[x.get_main_id()] = None
            if len(ids) == 1:
               self.toplevel.set_label_display(ids.keys()[0])
            else:
               self.toplevel.set_label_display('')
            ids = ids.keys()
            ids.sort()
            self.toplevel.set_glyph_ids_status(ids)
            self.toplevel.display_cc(images)
      self.toplevel.set_number_of_glyphs_selected_status(len(images))
      if len(images) == 0:
         self.toplevel.set_glyph_ids_status([])

   def _OnSelect(self, event):
      event.Skip()
      self._OnSelectImpl()

   def _OnKey(self, event):
      if event.KeyCode() == WXK_F12:
         self.toplevel.do_auto_move()
      elif event.KeyCode() == WXK_F11:
         self.toplevel.move_back()
      event.Skip()

class ClassifierMultiImageWindow(MultiImageWindow):
   def __init__(self, toplevel, parent = None, id = -1, size = wxDefaultSize):
      self.toplevel = toplevel
      MultiImageWindow.__init__(self, parent, id, size)
      from gamera.gui import gamera_icons
      self.toolbar.AddSeparator()
      self.toolbar.AddSimpleTool(
         300, gamera_icons.getIconDeleteBitmap(),
         "Delete selected glyphs",
         self._OnDelete)
      self.toolbar.AddSeparator()
      self.toolbar.AddSimpleTool(
         200, gamera_icons.getIconNextUnclassBitmap(),
         "Auto-move to next UNCLASSIFIED glyph",
         self._OnAutoMoveButton, 1)
      self.toolbar.AddSimpleTool(
         201, gamera_icons.getIconNextAutoclassBitmap(),
         "Auto-move to next AUTOMATIC glyph",
         self._OnAutoMoveButton, 1)
      self.toolbar.AddSimpleTool(
         202, gamera_icons.getIconNextHeurclassBitmap(),
         "Auto-move to next HEURISTIC glyph",
         self._OnAutoMoveButton, 1)
      self.toolbar.AddSimpleTool(
         203, gamera_icons.getIconNextManclassBitmap(),
         "Auto-move to next MANUAL glyph",
         self._OnAutoMoveButton, 1)
      
   def get_display(self):
      return ClassifierMultiImageDisplay(self.toplevel, self)

   def _OnAutoMoveButton(self, event):
      id = event.GetId() - 200
      if event.GetIsDown():
         self.toplevel.auto_move.append(id)
      else:
         self.toplevel.auto_move.remove(id)

   def _OnDelete(self, event):
      self.toplevel.multi_iw.id.delete_selected()
      self.toplevel.is_dirty = 1

class ClassifierImageDisplay(ImageDisplay):
   def __init__(self, toplevel, parent):
      self.toplevel = toplevel
      ImageDisplay.__init__(self, parent)

   def _OnRubber(self, shift):
      self.toplevel.find_glyphs_in_rect(
         self.rubber_origin_x, self.rubber_origin_y,
         self.rubber_x2, self.rubber_y2, shift)

class ClassifierImageWindow(ImageWindow):
   def __init__(self, toplevel, parent = None, id = -1):
      self.toplevel = toplevel
      ImageWindow.__init__(self, parent, id)
      from gamera.gui import gamera_icons
      self.toolbar.AddSeparator()
      self.toolbar.AddSimpleTool(
         25, gamera_icons.getIconMarkHighlightsBitmap(),
         "Box around highlights", self.id._OnBoxHighlightsToggle, 1)
      self.toolbar.AddSeparator()
      self.toolbar.AddSimpleTool(
         40, gamera_icons.getIconChooseImageBitmap(),
         "Choose new image", self._OnChooseImage)

   def _OnChooseImage(self, event):
      dlg = Args([Class("Image for context display", ImageBase)])
      image = dlg.show(self, image_menu.shell.locals, name="Choose image")
      if image != None:
         self.id.set_image(image)

   def get_display(self):
      return ClassifierImageDisplay(self.toplevel, self)

class ClassifierFrame(ImageFrameBase):
   def __init__(self, classifier, symbol_table=[],
                parent = None, id = -1, title = "Classifier",
                owner=None):
      if not isinstance(classifier, InteractiveClassifier):
         raise ValueError(
            "classifier must be instance of type InteractiveClassifier.")
      self._classifier = classifier

      if isinstance(symbol_table, SymbolTable):
         self._symbol_table = symbol_table
      elif util.is_string_or_unicode_list(symbol_table):
         self._symbol_table = SymbolTable()
         for symbol in symbol_table:
            self._symbol_table.add(symbol)
      else:
         raise ValueError(
            "symbol_table must be a SymbolTable instance or a list of strings.")
      # Add 'splits' to symbol table
      for split in ImageBase.methods_flat_category("Segmentation", ONEBIT):
         self._symbol_table.add("_split." + split[0])
      self._symbol_table.add("_group")
      self._symbol_table.add("_group._part")
      # Add classifier database's symbols to the symbol table
      for glyph in self._classifier.get_glyphs():
         for id in glyph.id_name:
            self._symbol_table.add(id[1])

      self.is_dirty = 0
      self.production_database_filename = None
      self.current_database_filename = None
      self.state_directory = None
      self._save_state_dialog = [1] * 10
      self.auto_move = []
      self.image = None
      self.menu = None
      self.default_segmenter = -1
      self._save_by_criteria_dialog = [1] * 10

      ImageFrameBase.__init__(
         self, parent, id,
         self._classifier.get_name() + " Classifier", owner)
      from gamera.gui import gamera_icons
      icon = wxIconFromBitmap(gamera_icons.getIconClassifyBitmap())
      self._frame.SetIcon(icon)
      self._frame.CreateStatusBar(3)
      self._frame.SetSize((800, 600))
      self.splitterv = wxSplitterWindow(
         self._frame, -1,
         style=wxSP_FULLSASH|wxSP_3DSASH|wxCLIP_CHILDREN|
         wxNO_FULL_REPAINT_ON_RESIZE|wxSP_LIVE_UPDATE)
      self.splitterhr = wxSplitterWindow(
         self.splitterv, -1,
         style=wxSP_FULLSASH|wxSP_3DSASH|wxCLIP_CHILDREN|
         wxNO_FULL_REPAINT_ON_RESIZE|wxSP_LIVE_UPDATE)
      self.splitterhl = wxSplitterWindow(
         self.splitterv, -1,
         style=wxSP_FULLSASH|wxSP_3DSASH|wxCLIP_CHILDREN|
         wxNO_FULL_REPAINT_ON_RESIZE|wxSP_LIVE_UPDATE)
      self.single_iw = ClassifierImageWindow(self, self.splitterhr)
      self.multi_iw = ClassifierMultiImageWindow(self, self.splitterhr)
      self.splitterhr.SetMinimumPaneSize(3)
      self.splitterhr.SplitHorizontally(self.multi_iw, self.single_iw, 300)
      self.symbol_editor = SymbolTableEditorPanel(
         self._symbol_table, self, self.splitterhl)
      self.rule_engine_runner = rule_engine_runner.RuleEngineRunnerPanel(
         self, self.splitterhl)
      self.splitterhl.SetMinimumPaneSize(3)
      self.splitterhl.SplitHorizontally(
         self.symbol_editor, self.rule_engine_runner, 300)
      self.splitterhl.Unsplit()
      self.splitterv.SetMinimumPaneSize(3)
      self.splitterv.SplitVertically(self.splitterhl, self.splitterhr, 160)
      self.create_menus()

   def create_menus(self):
      file_menu = gui_util.build_menu(
         self._frame,
         (("&Open all...", self._OnOpenAll),
          ("&Save all...", self._OnSaveAll),
          (None, None),
          ("Save &by criteria...", self._OnSaveByCriteria),
          (None, None),
          ("&Production database", 
           (("&Open...", self._OnOpenProductionDatabase),
            ("&Merge...", self._OnMergeProductionDatabase),
            ("&Save", self._OnSaveProductionDatabase),
            ("Save &as...", self._OnSaveProductionDatabaseAs),
            ("&Clear...", self._OnClearProductionDatabase))),
          ("&Current database",
           (("&Open...", self._OnOpenCurrentDatabase),
            ("&Merge...", self._OnMergeCurrentDatabase),
            ("&Save", self._OnSaveCurrentDatabase),
            ("Save &as...", self._OnSaveCurrentDatabaseAs),
            ("Save se&lection as...", self._OnSaveSelectedGlyphsAs))),
          ("&Symbol names",
           (("&Import...", self._OnImportSymbolTable),
            ("&Export...", self._OnExportSymbolTable)))))
      image_menu = gui_util.build_menu(
         self._frame,
         (("&Open and segment image...", self._OnOpenAndSegmentImage),
          ("Se&lect and segment image...", self._OnSelectAndSegmentImage),
          ("Se&lect image...", self._OnSelectImage),
          (None, None),
          ("&Save glyphs separately",
           (("&Production database...", self._OnSaveProductionDatabaseAsImages),
            ("&Current database...", self._OnSaveCurrentDatabaseAsImages),
            ("&Selected glyphs...", self._OnSaveSelectedAsImages)))
          ))
      classifier_settings = []
      if hasattr(self._classifier, "settings_dialog"):
         classifier_settings.append(("&Edit...", self._OnClassifierSettingsEdit))
      if hasattr(self._classifier, "load_settings"):
         classifier_settings.append(("&Open...", self._OnClassifierSettingsOpen))
      if hasattr(self._classifier, "save_settings"):
         classifier_settings.append(("&Save...", self._OnClassifierSettingsSave))
      classifier_menu_spec = [
         ("Guess all", self._OnGuessAll),
         ("&Guess selected", self._OnGuessSelected),
         (None, None),
         ("Group and guess all", self._OnGroupAndGuessAll),
         ("Group &and guess selected", self._OnGroupAndGuessSelected),
         (None, None),
         ("Confirm all", self._OnConfirmAll),
         ("&Confirm selected", self._OnConfirmSelected),
         (None, None),
         ("Change set of &features...", self._OnChangeSetOfFeatures)]
      if classifier_settings != []:
         classifier_menu_spec.extend([
            (None, None),
            ("Classifier &settings", classifier_settings)])
      if hasattr(self._classifier, 'noninteractive_copy'):
         classifier_menu_spec.extend([
            (None, None),
            ("Create &noninteractive copy...", self._OnCreateNoninteractiveCopy)])
      classifier_menu = gui_util.build_menu(
         self._frame,
         classifier_menu_spec)

      rules_menu = gui_util.build_menu(
         self._frame,
         (("Show rule testing panel", self._OnShowRuleTestingPanel),
          (None, None),
          ("Open rule module", self._OnOpenRuleModule)))
         
      menubar = wxMenuBar()
      menubar.Append(file_menu, "&File")
      menubar.Append(image_menu, "&Image")
      menubar.Append(classifier_menu, "&Classifier")
      menubar.Append(rules_menu, "&Rules")
      self._frame.SetMenuBar(menubar)

   def set_image(self, current_database, image=None, weak=1):
      self.set_multi_image(current_database)
      self.set_single_image(image, weak=weak)

   def set_multi_image(self, current_database):
      wxBeginBusyCursor()
      try:
         for glyph in current_database:
            for id in glyph.id_name:
               self._symbol_table.add(id[1])
         self.multi_iw.id.set_image(current_database)
      finally:
         wxEndBusyCursor()
         self.is_dirty = 1

   def set_single_image(self, image=None, weak=1):
      if image == None:
         if self.splitterhr.IsSplit():
            self.splitterhr.Unsplit()
            self.single_iw.Hide()
            del self.single_iw.id.image
            del self.single_iw.id.original_image
      else:
         self.single_iw.id.set_image(image, weak=weak)
         if not self.splitterhr.IsSplit():
            self.splitterhr.SplitHorizontally(
               self.multi_iw, self.single_iw, self._frame.GetSize()[1] / 2)
            self.single_iw.Show()
         
   def update_symbol_table(self):
      for glyph in self._classifier.get_glyphs():
         for id in glyph.id_name:
            self._symbol_table.add(id[1])
      for glyph in self.multi_iw.id.GetAllItems():
         for id in glyph.id_name:
            self._symbol_table.add(id[1])

   def add_to_database(self, glyphs):
      if hasattr(self._classifier, 'add_to_database'):
         self._classifier.add_to_database(glyphs)

   def remove_from_database(self, glyphs):
      if hasattr(self._classifier, 'remove_from_database'):
         self._classifier.remove_from_database(glyphs)

   ########################################
   # DISPLAY

   def display_cc(self, cc):
      if self.splitterhr.IsSplit():
         self.single_iw.id.highlight_cc(cc)
         self.single_iw.id.focus_glyphs(cc)

   def find_glyphs_in_rect(self, x1, y1, x2, y2, shift):
      self.multi_iw.id.find_glyphs_in_rect(x1, y1, x2, y2, shift)

   def set_number_of_glyphs_selected_status(self, number):
      if number < 0:
         text = "Error!"
      elif number == 1:
         text = "1 selected glyph"
      else:
         text = "%d selected glyphs" % number
      self._frame.GetStatusBar().SetStatusText(text, 0)

   def set_glyph_ids_status(self, ids):
      self._frame.GetStatusBar().SetStatusText(", ".join(ids), 1)

   ########################################
   # CLASSIFICATION FUNCTIONS
   def guess_glyph(self, glyph):
      try:
         return self._classifier.guess_glyph_automatic(glyph)
      except ClassifierError:
         return [(0.0, 'unknown')]

   def classify_manual(self, id):
      # if type(id) == types.StringType
      # id = id.encode('utf8')
      if not hasattr(self._classifier, 'classify_list_manual'):
         gui_util.message("NonInteractive classifiers can not be trained.")
         return
      selection = self.multi_iw.id.GetSelectedItems(
         self.multi_iw.id.GetGridCursorRow(),
         self.multi_iw.id.GetGridCursorCol())
      if selection != []:
         try:
            added, removed = self._classifier.classify_list_manual(selection, id)
         except ClassifierError, e:
            gui_util.message(str(e))
            return
         if len(added) or len(removed):
            wxBeginBusyCursor()
            self.multi_iw.id.BeginBatch()
            try:
               self.multi_iw.id.append_and_remove_glyphs(added, removed)
            finally:
               self.multi_iw.id.EndBatch()
               wxEndBusyCursor()
         self.multi_iw.id.RefreshSelected()
         if not self.do_auto_move():
            self.set_label_display(id)

   ########################################
   # AUTO-MOVE

   def do_auto_move(self):
      self.multi_iw.id.do_auto_move(self.auto_move)
      return self.auto_move != []

   def move_back(self):
      self.multi_iw.id.move_back()

   def set_label_display(self, symbol):
      self.symbol_editor.tree.set_label_display(symbol)

   def display_label_at_cell(self, row, col, label):
      self.multi_iw.id.display_label_at_cell(row, col, label)

   ########################################
   # FILE MENU

   def _OnOpenAll(self, event):
      dialog = Args(
         [Check('', 'Classifier settings', self._save_state_dialog[0]),
          Check('', 'Current database', self._save_state_dialog[1]),
          Check('', 'Production database', self._save_state_dialog[2]),
          Check('', 'Symbol table', self._save_state_dialog[3]),
          Check('', 'Source image', self._save_state_dialog[4]),
          Directory('Save directory')], name="Save classifier window")
      results = dialog.show(self._frame)
      if results == None:
         return
      self._save_state_dialog = results
      settings, current, production, symbols, source, directory = results
      if directory == None:
         gui_util.message("You must provide a directory to load.")
         return
      if settings:
         self._OpenClassifierSettings(
            os.path.join(directory, "classifier_settings.xml"))
      if current:
         self._OpenCurrentDatabase(
            os.path.join(directory, "current_database.xml"))
      if production:
         self._OpenProductionDatabase(
            os.path.join(directory, "production_database.xml"))
      if symbols:
         self._ImportSymbolTable(
            os.path.join(directory, "symbol_table.xml"))
      if source:
         try:
            self.set_single_image(
               load_image(os.path.join(directory, "source_image.tiff")))
         except Exception, e:
            gui_util.message("Loading image: " + str(e))

   def _OnSaveAll(self, event):
      dialog = Args(
         [Check('', 'Classifier settings', self._save_state_dialog[0]),
          Check('', 'Current database', self._save_state_dialog[1]),
          Check('', 'Production database', self._save_state_dialog[2]),
          Check('', 'Symbol table', self._save_state_dialog[3]),
          Check('', 'Source image', self._save_state_dialog[4],
                enabled=self.splitterhr.IsSplit()),
          Directory('Save directory')], name="Save classifier window")
      results = dialog.show(self._frame)
      if results == None:
         return
      self._save_state_dialog = results
      settings, current, production, symbols, source, directory = results
      if directory == None:
         gui_util.message("You must provide a directory to load.")
         return
      if settings:
         try:
            self._SaveClassifierSettings(
               os.path.join(directory, "classifier_settings.xml"))
         except:
            pass
      if current:
         try:
            self._SaveCurrentDatabase(
               os.path.join(directory, "current_database.xml"))
         except:
            pass
      if production:
         try:
            self._SaveProductionDatabase(
               os.path.join(directory, "production_database.xml"))
         except:
            pass
      if symbols:
         try:
            self._ExportSymbolTable(os.path.join(directory, "symbol_table.xml"))
         except:
            pass
      if source and self.splitterhr.IsSplit():
         try:
            self.single_iw.id.image.save_tiff(
               os.path.join(directory, "source_image.tiff"))
         except Exception, e:
            gui_util.message("Saving image: " + str(e))


   ########################################
   # IMAGE MENU

   def _OnOpenAndSegmentImage(self, event):
      segmenters = [x[0] for x in
                    ImageBase.methods_flat_category("Segmentation", ONEBIT)]
      if self.default_segmenter == -1:
         self.default_segmenter = segmenters.index("cc_analysis")
      dialog = Args(
         [FileOpen("Image file", "", "*.*"),
          Choice("Segmentation algorithm", segmenters, self.default_segmenter)],
         name="Open and segment image...")
      filename = None
      while filename is None:
         results = dialog.show(self._frame)
         if results is None:
            return
         filename, segmenter = results
         self.default_segmenter = segmenter
         if filename == None:
            gui_util.message("You must provide a filename to load.")
      
      wxBeginBusyCursor()
      try:
         image = load_image(filename)
         self._segment_image(image, segmenters[segmenter])
      except Exception, e:
         gui_util.message(str(e))
         wxEndBusyCursor()
         return
      wxEndBusyCursor()

   def _OnSelectAndSegmentImage(self, event):
      segmenters = [x[0] for x in
                    ImageBase.methods_flat_category("Segmentation", ONEBIT)]
      if self.default_segmenter == -1:
         self.default_segmenter = segmenters.index("cc_analysis")
      dialog = Args(
         [Class("Image", ImageBase),
          Choice("Segmentation algorithm", segmenters, self.default_segmenter)],
         name="Select and segment image...")
      results = dialog.show(self._frame, image_menu.shell.locals)
      if results is None:
         return
      image, segmenter = results
      wxBeginBusyCursor()
      try:
         self._segment_image(image, segmenters[segmenter])
      except Exception, e:
         gui_util.message(str(e))
         wxEndBusyCursor()
         return
      wxEndBusyCursor()

   def _OnSelectImage(self, event):
      dialog = Args(
         [Class("Image", ImageBase)],
         name="Select image...")
      results = dialog.show(self._frame, image_menu.shell.locals)
      if results is None:
         return
      (image,) = results
      self.set_single_image(image, weak=0)
      
   def _segment_image(self, image, segmenter):
      image_ref = image
      image_ref = image_ref.to_onebit()
      ccs = getattr(image_ref, segmenter)()
      self.set_image(ccs, image, weak=0)

   def _OnSaveCurrentDatabaseAsImages(self, event):
      self._OnSaveAsImages(self.multi_iw.id.GetAllItems())

   def _OnSaveSelectedAsImages(self, event):
      self._OnSaveAsImages(self.multi_iw.id.GetSelectedItems())

   def _OnSaveProductionDatabaseAsImages(self, event):
      self._OnSaveAsImages(self._classifier.get_glyphs())

   def _OnSaveAsImages(self, list):
      filename = gui_util.directory_dialog(self._frame)
      if filename:
         classifier_stats.GlyphStats(list).write(filename)

   ########################################
   # CLASSIFIER MENU

   def _OnGuessAll(self, event):
      self._OnGuess(self.multi_iw.id.GetAllItems())

   def _OnGuessSelected(self, event):
      self._OnGuess(self.multi_iw.id.GetSelectedItems())

   def _OnGuess(self, list):
      try:
         added, removed = self._classifier.classify_list_automatic(list)
      except ClassifierError, e:
         gui_util.message(str(e))
      self._AdjustAfterGuess(added, removed)

   def _OnGroupAndGuessAll(self, event):
      self._OnGroupAndGuess(self.multi_iw.id.GetAllItems())

   def _OnGroupAndGuessSelected(self, event):
      self._OnGroupAndGuess(self.multi_iw.id.GetSelectedItems())

   def _OnGroupAndGuess(self, list):
      try:
         added, removed = self._classifier.group_list_automatic(list)
      except ClassifierError, e:
         gui_util.message(str(e))
      self._AdjustAfterGuess(added, removed)

   def _AdjustAfterGuess(self, added, removed):
      wxBeginBusyCursor()
      self.multi_iw.id.BeginBatch()
      try:
         if len(added) or len(removed):
            self.multi_iw.id.append_and_remove_glyphs(added, removed)
         else:
            self.multi_iw.id.RefreshSelected()
         self.multi_iw.id.sort_images()
      finally:
         self.multi_iw.id.ForceRefresh()
         self.multi_iw.id.EndBatch()
         wxEndBusyCursor()

   def _OnConfirmAll(self, event):
      self._OnConfirm(self.multi_iw.id.GetAllItems())

   def _OnConfirmSelected(self, event):
      self._OnConfirm(self.multi_iw.id.GetSelectedItems())

   def _OnConfirm(self, list):
      if not hasattr(self._classifier, 'classify_glyph_manual'):
         gui_util.message("This classifier can not be trained.")
         return
      wxBeginBusyCursor()
      try:
         for x in list:
            if (x is not None and not hasattr(x, 'dead') and
                x.classification_state == AUTOMATIC):
               try:
                  self._classifier.classify_glyph_manual(x, x.get_main_id())
               except ClassifierError, e:
                  gui_util.message(str(e))
                  return
      finally:
         self.multi_iw.id.ForceRefresh()
         wxEndBusyCursor()
      
   def _OnChangeSetOfFeatures(self, event):
      all_features = [x[0] for x in ImageBase.methods_flat_category("Features", ONEBIT)]
      all_features.sort()
      existing_features = [x[0] for x in self._classifier.get_feature_functions()[0]]
      feature_controls = []
      for x in all_features:
         feature_controls.append(
            Check('', x, default=(x in existing_features)))
      dialog = Args(
         feature_controls,
         name='Feature selection', 
         title='Select the features you want to use for classification')
      result = dialog.show(self._frame)
      if result is None:
         return
      selected_features = [name for check, name in zip(result, all_features) if check]
      self._classifier.change_feature_set(selected_features)

   def _OnClassifierSettingsEdit(self, event):
      if hasattr(self._classifier, 'settings_dialog'):
         self._classifier.settings_dialog(self._frame)
      else:
         gui_util.message("This classifier doesn't have a settings dialog.")

   def _OnClassifierSettingsOpen(self, event):
      filename = gui_util.open_file_dialog(self._frame, gamera_xml.extensions)
      if filename:
         self._OpenClassifierSettings(filename)
         
   def _OpenClassifierSettings(self, filename):
      try:
         self._classifier.load_settings(filename)
      except gamera_xml.XMLError, e:
         gui_util.message("Opening classifier settings: " + str(e))

   def _OnClassifierSettingsSave(self, event):
      filename = gui_util.save_file_dialog(self._frame, gamera_xml.extensions)
      if filename:
         self._SaveClassifierSettings(filename)

   def _SaveClassifierSettings(self, filename):
      try:
         self._classifier.save_settings(filename)
      except gamera_xml.XMLError, e:
         gui_util.message("Saving classifier settings: " + str(e))

   def _OnCreateNoninteractiveCopy(self, event):
      from gamera.gui import var_name
      name = var_name.get("classifier", image_menu.shell.locals)
      if name is None: return
      try:
         result = self._classifier.noninteractive_copy()
         image_menu.shell.locals[name] = result
         image_menu.shell.update()
      except ClassifierError, e:
         gui_util.message(str(e))
         
   ########################################
   # XML MENU

   def _OnSaveByCriteria(self, event):
      dialog = Args(
         [Info('Set of glyphs to save:'),
          Check('', 'Production database', self._save_by_criteria_dialog[1]),
          Check('', 'Current database', self._save_by_criteria_dialog[2]),
          Info('Kind of glyphs to save:'),
          Check('', 'Unclassified', self._save_by_criteria_dialog[4]),
          Check('', 'Automatically classified', self._save_by_criteria_dialog[5]),
          Check('', 'Heuristically classified', self._save_by_criteria_dialog[6]),
          Check('', 'Manually classified', self._save_by_criteria_dialog[6]),
          FileSave('Save glyphs to file:', '',
                   extension=gamera_xml.extensions)],
         name = 'Save by criteria...')
      verified = False
      while not verified:
         results = dialog.show(self._frame)
         if results is None:
            return
         skip, proddb, currdb, skip, un, auto, heur, man, filename = results
         if ((proddb == 0 and currdb == 0) or
             (un == 0 and auto == 0 and heur == 0 and man == 0)):
            gui_util.message(
               "You didn't select anything to save!" +
               "\n(You must check at least one box per category.)")
            continue
         if filename is None:
            gui_util.message("You must select a filename to save into.")
            continue
         verified = True

      self._save_by_criteria_dialog = results

      glyphs = []
      if proddb:
         glyphs = util.combine_unique_elements(glyphs, self._classifier.get_glyphs())
      if currdb:
         elements = util.combine_unique_elements(glyphs, self.multi_iw.id.GetAllItems())

      # One big-ass filtering list comprehension
      glyphs = [x for x in glyphs.iterkeys()
                if ((x != None and not hasattr(x, 'dead')) and
                    ((x.classification_state == UNCLASSIFIED and un) or
                     (x.classification_state == AUTOMATIC and auto) or
                     (x.classification_state == HEURISTIC and heur) or
                     (x.classification_state == MANUAL and man)))]
      try:
         gamera_xml.WriteXMLFile(
            glyphs=glyphs,
            symbol_table=self._symbol_table).write_filename(
               filename)
      except gamera_xml.XMLError, e:
         gui_util.message("Saving by criteria: " + str(e))
         
   def _OnOpenProductionDatabase(self, event):
      filenames = gui_util.open_file_dialog(self._frame, gamera_xml.extensions,
                                           multiple=1)
      if not filenames is None:
         if len(filenames) == 1:
            self.production_database_filename = filenames[0]
         else:
            self.production_database_filename = None
         self._OpenProductionDatabase(filenames)

   def _OpenProductionDatabase(self, filenames):
      try:
         self._classifier.from_xml_filename(filenames[0])
         if len(filenames) > 1:
            for f in filenames[1:]:
               self._classifier.merge_from_xml_filename(f)
      except gamera_xml.XMLError, e:
         gui_util.message("Opening production database: " + str(e))
         return
      self.update_symbol_table()

   def _OnMergeProductionDatabase(self, event):
      filenames = gui_util.open_file_dialog(self._frame, gamera_xml.extensions,
                                            multiple=1)
      if not filenames is None:
         try:
            for f in filenames:
               self._classifier.merge_from_xml_filename(f)
         except gamera_xml.XMLError, e:
            gui_util.message("Merging production database: " + str(e))
            return
         self.update_symbol_table()

   def _OnSaveProductionDatabase(self, event):
      if self.production_database_filename == None:
         self._OnSaveProductionDatabaseAs(event)
      else:
         self._SaveProductionDatabase(self.production_database_filename)

   def _OnSaveProductionDatabaseAs(self, event):
      filename = gui_util.save_file_dialog(self._frame, gamera_xml.extensions)
      if filename:
         self.production_database_filename = filename
         self._OnSaveProductionDatabase(event)

   def _SaveProductionDatabase(self, filename):
      try:
         self._classifier.to_xml_filename(filename)
      except gamera_xml.XMLError, e:
         gui_util.message("Saving production database: " + str(e))

   def _OnClearProductionDatabase(self, event):
      if self._classifier.is_dirty:
         if gui_util.are_you_sure_dialog(self._frame,
            "Are you sure you want to clear all glyphs in the production database?"):
            self._classifier.clear_glyphs()
      else:
         self._classifier.clear_glyphs()

   def _OnOpenCurrentDatabase(self, event):
      filenames = gui_util.open_file_dialog(self._frame, gamera_xml.extensions,
                                            multiple=1)
      if not filenames is None:
         if len(filenames) == 1:
            self.current_database_filename = filenames[0]
         else:
            self.current_database_filename = None
         self._OpenCurrentDatabase(filenames)

   def _OpenCurrentDatabase(self, filenames):
      try:
         glyphs = gamera_xml.LoadXML().parse_filename(filenames[0]).glyphs
         if len(filenames) > 1:
            for f in filenames[1:]:
               glyphs.extend(gamera_xml.LoadXML().parse_filename(f).glyphs)
      except gamera_xml.XMLError, e:
         gui_util.message("Opening current database: " + str(e))
         return
      self.set_multi_image(glyphs)

   def _OnMergeCurrentDatabase(self, event):
      filenames = gui_util.open_file_dialog(self._frame, gamera_xml.extensions,
                                            multiple=1)
      if not filenames is None:
         glyphs = []
         try:
            for f in filenames:
               glyphs.extend(gamera_xml.LoadXML().parse_filename(f).glyphs)
         except gamera_xml.XMLError, e:
            gui_util.message("Merging current database" + str(e))
            return
         self.multi_iw.id.append_glyphs(glyphs)

   def _OnSaveCurrentDatabase(self, event):
      if self.current_database_filename == None:
         self._OnSaveCurrentDatabaseAs(event)
      else:
         self._SaveCurrentDatabase(self.current_database_filename)

   def _OnSaveCurrentDatabaseAs(self, event):
      filename = gui_util.save_file_dialog(self._frame, gamera_xml.extensions)
      if filename:
         self.current_database_filename = filename
         self._OnSaveCurrentDatabase(event)

   def _SaveCurrentDatabase(self, filename):
      glyphs = self.multi_iw.id.GetAllItems()
      self._classifier.generate_features(glyphs)
      try:
         gamera_xml.WriteXMLFile(
            glyphs=glyphs,
            symbol_table=self._symbol_table).write_filename(
            filename)
      except gamera_xml.XMLError, e:
         gui_util.message("Saving current database: " + str(e))

   def _OnSaveSelectedGlyphsAs(self, event):
      filename = gui_util.save_file_dialog(self._frame, gamera_xml.extensions)
      if filename:
         glyphs = self.multi_iw.id.GetSelectedItems()
         self._classifier.generate_features(glyphs)
         try:
            gamera_xml.WriteXMLFile(
               glyphs=glyphs,
               symbol_table=self._symbol_table).write_filename(
               filename)
         except gamera_xml.XMLError, e:
            gui_util.message("Saving selected glyphs: " + str(e))
         
   def _OnImportSymbolTable(self, event):
      filename = gui_util.open_file_dialog(self._frame, gamera_xml.extensions)
      if filename:
         self._ImportSymbolTable(filename)

   def _ImportSymbolTable(self, filename):
      wxBeginBusyCursor()
      try:
         try:
            symbol_table = gamera_xml.LoadXML(
               parts=['symbol_table']).parse_filename(filename).symbol_table
         except gamera_xml.XMLError, e:
            gui_util.message("Importing symbol table: " + str(e))
            return
         for symbol in symbol_table.symbols.keys():
            self._symbol_table.add(symbol)
      finally:
         wxEndBusyCursor()

   def _OnExportSymbolTable(self, event):
      filename = gui_util.save_file_dialog(self._frame, gamera_xml.extensions)
      if filename:
         self._ExportSymbolTable(filename)

   def _ExportSymbolTable(self, filename):
      wxBeginBusyCursor()
      try:
         try:
            gamera_xml.WriteXMLFile(
               symbol_table=self._symbol_table).write_filename(filename)
         except gamera_xml.XMLError, e:
            gui_util.message("Exporting symbol table: " + str(e))
      finally:
         wxEndBusyCursor()

   ########################################
   # RULES MENU

   def _OnShowRuleTestingPanel(self, event, show=1):
      if self.splitterhl.IsSplit():
         self.splitterhl.Unsplit()
         self.rule_engine_runner.Hide()
      else:
         self.splitterhl.SplitHorizontally(
            self.symbol_editor, self.rule_engine_runner,
            self._frame.GetSize()[1] / 2)
         self.rule_engine_runner.Show()

   def _OnOpenRuleModule(self, event):
      filename = gui_util.open_file_dialog(self._frame, "*.py")
      if not filename is None:
         self.rule_engine_runner.open_module(filename)
      
      if not self.splitterhl.IsSplit():
         self.splitterhl.SplitHorizontally(
            self.symbol_editor, self.rule_engine_runner,
            self._frame.GetSize()[1] / 2)
         self.rule_engine_runner.Show()

   ########################################
   # CALLBACKS

   def _OnCloseWindow(self, event):
      if self._classifier.is_dirty:
         if not gui_util.are_you_sure_dialog(
            self._frame,
            "Are you sure you want to quit without saving?"):
            event.Veto()
            return
      self._classifier.set_display(None)
      self.multi_iw.Destroy()
      self.single_iw.Destroy()
      self.splitterhr.Destroy()
      self.splitterhl.Destroy()
      self.splitterv.Destroy()
      self._frame.Destroy()
      del self._frame

   def refresh(self):
      self.single_iw.id.refresh(1)


##############################################################################
# SYMBOL TABLE EDITOR
##############################################################################

class SymbolTreeCtrl(wxTreeCtrl):
   def __init__(self, toplevel, parent, id, pos, size, style):
      self.toplevel = toplevel
      self.editing = 0
      wxTreeCtrl.__init__(self, parent, id, pos, size,
                          style=style|wxNO_FULL_REPAINT_ON_RESIZE)
      self.root = self.AddRoot("Symbols")
      self.SetItemHasChildren(self.root, True)
      self.SetPyData(self.root, "")
      EVT_KEY_DOWN(self, self._OnKey)
      EVT_LEFT_DOWN(self, self._OnLeftDown)
      EVT_TREE_ITEM_ACTIVATED(self, id, self._OnActivated)
      self.toplevel._symbol_table.evt_add(self.symbol_table_add_callback)
      self.toplevel._symbol_table.evt_remove(self.symbol_table_remove_callback)
      self.Expand(self.root)
      self.SelectItem(self.root)

   def __del__(self):
      self._symbol_table.evt_del_add(self.symbol_table_add_callback)
      self._symbol_table.evt_del_remove(self.symbol_table_remove_callback)

   ########################################
   # CALLBACKS

   def OnCompareItems(self, item1, item2):
      t1 = self.GetItemText(item1)
      t2 = self.GetItemText(item2)
      if t1 < t2: return -1
      if t1 == t2: return 0
      return 1
     
   def set_label_display(self, symbol):
      self.toplevel.text.SetValue(symbol)
      self.toplevel.text.SetSelection(0, len(symbol))
      self.toplevel._OnText(None)
      self.toplevel.text.SetFocus()

   def symbol_table_callback(self, tokens):
      root = self.root
      expand_list = []
      for i in range(len(tokens)):
         token = tokens[i]
         found = 0
         cookie = 0
         item, cookie = self.GetFirstChild(root, cookie)
         while item.IsOk():
            text = self.GetItemText(item)
            if text == token:
               found = 1
               break
            elif text > token:
               found = 0
               break
            item, cookie = self.GetNextChild(root, cookie)
         if not found:
            item = self.AppendItem(root, token)
            self.SetPyData(item, '.'.join(tokens[0:i+1]))
            self.SortChildren(root)
            if token.startswith('_'):
               self.SetItemBold(item)
         root = item
         if token != tokens[-1]:
            self.SetItemHasChildren(root)
            self.Expand(root)

   def symbol_table_remove_callback(self, tokens):
      root = self.root
      expand_list = []
      for i in range(len(tokens)):
         token = tokens[i]
         found = 0
         cookie = 0
         item, cookie = self.GetFirstChild(root, cookie)
         while item.IsOk():
            text = self.GetItemText(item)
            if text == token:
               found = 1
               break
            elif text > token:
               found = 0
               break
            item, cookie = self.GetNextChild(root, cookie)
         if not found:
            break
         root = item
      if found:
         self.Delete(item)

   def _OnKey(self, evt):
      if evt.KeyCode() == WXK_DELETE:
         self.toplevel._symbol_table.remove(
            self.GetPyData(self.GetSelection()))
      else:
         evt.Skip()

   def _OnActivated(self, event):
      symbol = self.GetPyData(event.GetItem())
      if symbol != None:
         self.toplevel.toplevel.classify_manual(symbol)

   def _OnLeftDown(self, event):
      pt = event.GetPosition()
      item, flags = self.HitTest(pt)
      if flags == 4:
         return
      data = self.GetPyData(item)
      if data != None:
         self.toplevel.text.SetValue(data)
         self.toplevel.text.SetInsertionPointEnd()
      event.Skip()

class SymbolTableEditorPanel(wxPanel):
   def __init__(self, symbol_table, toplevel, parent = None, id = -1):
      wxPanel.__init__(
         self, parent, id,
         style=wxWANTS_CHARS|wxCLIP_CHILDREN|wxNO_FULL_REPAINT_ON_RESIZE)
      self.toplevel = toplevel
      self._symbol_table = symbol_table
      self.SetAutoLayout(True)
      self.box = wxBoxSizer(wxVERTICAL)
      txID = wxNewId()
      self.text = wxTextCtrl(self, txID,
                             style=wxTE_PROCESS_ENTER|wxTE_PROCESS_TAB)
      EVT_KEY_DOWN(self.text, self._OnKey)
      EVT_TEXT(self, txID, self._OnText)
      # On win32, the enter key is only caught by the EVT_TEXT_ENTER
      # On GTK, the enter key is sent directly to EVT_KEY_DOWN
      if wxPlatform == '__WXMSW__':
         EVT_TEXT_ENTER(self, txID, self._OnEnter)
      self.box.Add(self.text, 0, wxEXPAND|wxBOTTOM, 5)
      tID = wxNewId()
      self.tree = SymbolTreeCtrl(self, self, tID, wxDefaultPosition,
                                 wxDefaultSize,
                                 wxTR_HAS_BUTTONS | wxTR_DEFAULT_STYLE)
      self.box.Add(self.tree, 1, wxEXPAND|wxALL)
      self.box.RecalcSizes()
      self.SetSizer(self.box)

   ########################################
   # CALLBACKS

   def _OnEnter(self, evt):
      find = self.text.GetValue()
      normalized_symbol = self._symbol_table.add(find)
      if normalized_symbol != '':
         self.toplevel.classify_manual(normalized_symbol)

   def _OnKey(self, evt):
      find = self.text.GetValue()
      if evt.KeyCode() == WXK_TAB:
         find = self._symbol_table.autocomplete(find)
         self.text.SetValue(find)
         self.text.SetInsertionPointEnd()
      elif evt.KeyCode() == WXK_RETURN:
         self._OnEnter(evt)
      elif evt.KeyCode() == WXK_F12:
         self.toplevel.do_auto_move()
      elif evt.KeyCode() == WXK_F11:
         self.toplevel.move_back()
      # This behavior works automatically in GTK+
      elif wxPlatform == '__WXMSW__' and evt.ControlDown():
         if evt.KeyCode() == WXK_LEFT:
            current = self.text.GetInsertionPoint()
            if current != 0:
               new = self.text.GetValue().rfind(".", 0, current - 1)
               self.text.SetInsertionPoint(new + 1)
            return
         elif evt.KeyCode() == WXK_BACK:
            current = self.text.GetInsertionPoint()
            value = self.text.GetValue()
            if current != 0:
               new = value.rfind(".", 0, current - 1)
               if new == -1:
                  self.text.SetValue("")
               else:
                  self.text.SetValue(self.text.GetValue()[0:new + 1])
               self.text.SetInsertionPointEnd()
            return
         elif evt.KeyCode() == WXK_RIGHT:
            current = self.text.GetInsertionPoint()
            new = self.text.GetValue().find(".", current)
            if new == -1:
               self.text.SetInsertionPointEnd()
            else:
               self.text.SetInsertionPoint(new + 1)
            return
      else:
         evt.Skip()

   def _OnText(self, evt):
      symbol, tokens = self._symbol_table.normalize_symbol(
         self.text.GetValue())
      root = self.tree.root
      expand_list = []
      found = None
      for i in range(len(tokens)):
         token = tokens[i]
         found = None
         cookie = 0
         item, cookie = self.tree.GetFirstChild(root, cookie)
         while item.IsOk():
            text = self.tree.GetItemText(item)
            if text == token:
               found = item
               break
            elif text > token:
               break
            item, cookie = self.tree.GetNextChild(root, cookie)
         if not found:
            break
         elif token != tokens[-1] and not self.tree.IsExpanded(item):
            self.tree.Expand(item)
         root = item
      if not found is None:
         self.tree.SelectItem(found)
         self.tree.ScrollTo(found)
      else:
         self.tree.UnselectAll()
         
      if not evt is None:
         evt.Skip()
