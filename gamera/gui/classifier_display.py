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

import types
from wxPython.wx import *
from gamera.core import *
from gamera.args import *
from gamera.symbol_table import SymbolTable
from gamera import gamera_xml, classify, classifier_stats
from gamera.gui import image_menu, var_name, toolbar, gui_util
from gamera.gui.gamera_display import *

###############################################################################
# CLASSIFIER DISPLAY
###############################################################################

class ClassifierMultiImageDisplay(MultiImageDisplay):
   def __init__(self, toplevel, parent):
      self.toplevel = toplevel
      MultiImageDisplay.__init__(self, parent)
      self.last_image_no = None
      EVT_GRID_RANGE_SELECT(self, self.OnSelect)
      self.last_sort = None
      # This is to turn off the display of row labels if a)
      # we have done some classification and b) we have done a
      # sort other than the default. KWM
      self.display_row_labels = 0
      EVT_KEY_DOWN(self, self.OnKey)

   ########################################
   # AUTO-MOVE

   def do_auto_move(self, state):
      # if auto-moving is turned off, just return
      if state == []:
         return

      # We may have multiple glyphs selected, so we need
      # to start looking from the last one.  Since there's
      # no way in wxPython to just get a list of the selected
      # glyphs, we have to look for them, in this case backwards.
      if self.IsSelection():
         found = 0
         image_no = -1
         for col in xrange(self.cols, -1, -1):
            for row in xrange(self.rows, -1, -1):
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
      list = self.GetAllItems()
      for i in range(image_no + 1, len(list)):
         image = list[i]
         if (image != None and
             not hasattr(image, 'dead') and
             image.classification_state in state):
            found = i
            break
      if found != -1:
         row = found / GRID_NCOLS
         col = found % GRID_NCOLS
         self.SetGridCursor(row, col)
         self.SelectBlock(row, col, row, col, 0)
         self.MakeCellVisible(min(row + 1, self.rows - 1), col)
         if image.classification_state == UNCLASSIFIED:
            id = self.toplevel.guess_glyph(image)
         else:
            id = image.id_name
         self.toplevel.set_label_display(id)
         self.toplevel.display_label_at_cell(row, col, id)

   ########################################
   # DISPLAYING A LABEL BENEATH A CELL

   def find_glyphs_in_rect(self, x1, y1, x2, y2, shift):
      self.BeginBatch()
      matches = []
      if x1 == x2 or y1 == y2:
         point = Point(x1, y1)
         for i in range(len(self.list)):
            g = self.list[i]
            if g != None and g.contains_point(point):
               if (g.get(y1 - g.ul_y, x1 - g.ul_x) != 0):
                  matches.append(i)
                  break
      else:
         matches = []
         r = Rect(y1, x1, y2 - y1 + 1, x2 - x1 + 1)
         for i in range(len(self.list)):
            g = self.list[i]
            if g != None:
               if r.contains_rect(g):
                  matches.append(i)
      if matches != []:
         first = 0
         self.updating = 1
         last_index = matches[-1]
         for index in matches:
            row = index / GRID_NCOLS
            col = index % GRID_NCOLS
            if index is last_index:
               self.updating = 0
            self.SelectBlock(row, col, row, col, first or shift)
            if first == 0:
               self.MakeCellVisible(row, col)
               first = 1
      else:
         if not shift:
            self.toplevel.single_iw.id.clear_all_highlights()
            self.ClearSelection()
      self.EndBatch()

   ########################################
   # SORTING

   def sort_by_name_func(self, a, b):
      if a.id_name == [] and b.id_name == []:
         id_name_cmp = 0
      elif a.id_name == []:
         id_name_cmp = 1
      elif b.id_name == []:
         id_name_cmp = -1
      else:
         id_name_cmp = cmp(a.get_main_id(), b.get_main_id())
      if id_name_cmp == 0:
         id_name_cmp = cmp(b.classification_state, a.classification_state)
      return id_name_cmp

   def split_classified_from_unclassified(self, list):
      # Find split between classified and unclassified
      for i in range(len(list)):
         if list[i].classification_state == UNCLASSIFIED:
            break
      return list[:i], list[i:]

   def insert_for_line_breaks(self, list):
      # Make sure each new label begins in a new row
      column = 0
      prev_id = -1
      new_list = []
      for image in list:
         main_id = image.get_main_id()
         if main_id != prev_id and column != 0:
            new_list.extend([None] * (GRID_NCOLS - column))
            column = 0
         new_list.append(image)
         column += 1
         column %= GRID_NCOLS
         prev_id = main_id
      for i in range(column, GRID_NCOLS):
         new_list.append(None)
      return new_list

   def default_sort(self, list):
      # If we've done no classification, use the default sort from
      # MultiImageDisplay
      self.last_sort = "default"
      clean = 1
      for item in list:
         if item.classification_state != UNCLASSIFIED:
            clean = 0
            break
      if clean:
         new_list = MultiImageDisplay.default_sort(self, list)
      else:
         # mark that we want to display row labels
         self.display_row_labels = 1
         # Sort by label
         list.sort(self.sort_by_name_func)
         # Find split between classified and unclassified
         classified, unclassified = self.split_classified_from_unclassified(list)
         # Sort the unclassified by size
         unclassified = MultiImageDisplay.default_sort(self, unclassified)
         # Merge back together
         list = classified + unclassified
         # Make sure each new label begins in a new row
         new_list = self.insert_for_line_breaks(list)
      return new_list

   def sort_images(self, function=None, order=0):
      self.last_sort = None
      self.display_row_labels = not function
      wxBeginBusyCursor()
      self.BeginBatch()
      orig_len = len(self.list)
      new_list = self.GetAllItems()
      if len(new_list) != len(self.list):
         self.list = new_list
      MultiImageDisplay.sort_images(self, function, order)
      if orig_len != len(self.list):
         self.resize_grid()
      self.EndBatch()
      wxEndBusyCursor()

   def set_labels(self):
      if self.last_sort != "default":
         MultiImageDisplay.set_labels(self)
      self.BeginBatch()
      max_label = 1
      for i in range(self.cols):
         self.SetColLabelValue(i, "")
      dc = wxClientDC(self)
      for i in range(self.rows):
         try:
            image = self.list[i * GRID_NCOLS]
         except IndexError:
            self.SetRowLabelValue(i, "")
         else:
            if image == None or image.classification_state == UNCLASSIFIED:
               self.SetRowLabelValue(i, "")
            elif self.display_row_labels:
               label = self.get_label(image)
               label = self.reduce_label_length(
                  dc, GRID_MAX_LABEL_LENGTH * 0.6, label)
               max_label = max(dc.GetTextExtent(label)[0], max_label)
               self.SetRowLabelValue(i, label)
            else:
               max_label = max(dc.GetTextExtent("<>")[0], max_label)
               self.SetRowLabelValue(i, "<>")
      self.EndBatch()
      return min(max_label, GRID_MAX_LABEL_LENGTH)

   def delete_selected(self):
      for item in self.GetSelectedItems():
         if hasattr(item, 'dead'):
            del item.__dict__['dead']
         else:
            item.dead = 1
      self.RefreshSelected()

   ########################################
   # CALLBACKS

   # Display selected items in the context display
   def OnSelectImpl(self):
      if not self.updating:
         images = self.GetSelectedItems()
         if images != []:
            id = images[0].id_name
            all_same = 1
            for x in images:
               if x.id_name != id:
                  all_same = 0
                  break
            if all_same:
               self.toplevel.set_label_display(id)
            else:
               self.toplevel.set_label_display([])
            self.toplevel.display_cc(images)

   def OnSelect(self, event):
      event.Skip()
      self.OnSelectImpl()

   def OnKey(self, event):
      if event.KeyCode() == WXK_F12:
         self.toplevel.do_auto_move()
         

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
      self.auto_move_buttons = []
      self.auto_move_buttons.append(
         self.toolbar.AddSimpleTool(
         200, gamera_icons.getIconNextUnclassBitmap(),
         "Move to next unclassified glyph after each classification.",
         self._OnAutoMoveButton, 1)
         )
      self.auto_move_buttons.append(
         self.toolbar.AddSimpleTool(
         201, gamera_icons.getIconNextAutoclassBitmap(),
         "Move to next auto-classified glyph after each classification.",
         self._OnAutoMoveButton, 1)
         )
      self.auto_move_buttons.append(
      self.toolbar.AddSimpleTool(
         202, gamera_icons.getIconNextHeurclassBitmap(),
         "Move to next heuristically classified glyph after each classification.",
         self._OnAutoMoveButton, 1)
      )
      self.auto_move_buttons.append(
      self.toolbar.AddSimpleTool(
         203, gamera_icons.getIconNextManclassBitmap(),
         "Move to next manually classified glyph after each classification.",
         self._OnAutoMoveButton, 1)
      )
      
   def get_display(self):
      return ClassifierMultiImageDisplay(self.toplevel, self)

   def _OnAutoMoveButton(self, event):
      id = event.GetId() - 200
      if event.GetIsDown():
         self.toplevel.auto_move.append(id)
      else:
         self.toplevel.auto_move.remove(id)

   def _OnDelete(self, event):
      self.toplevel.delete_selected()

class ClassifierImageDisplay(ImageDisplay):
   def __init__(self, toplevel, parent):
      self.toplevel = toplevel
      ImageDisplay.__init__(self, parent)

   def OnRubber(self, shift):
      self.toplevel.find_glyphs_in_rect(
         self.rubber_origin_x, self.rubber_origin_y,
         self.rubber_x2, self.rubber_y2, shift)

class ClassifierImageWindow(ImageWindow):
   def __init__(self, toplevel, parent = None, id = -1):
      self.toplevel = toplevel
      ImageWindow.__init__(self, parent, id)
      self.toolbar.AddSeparator()
      from gamera.gui import gamera_icons
      self.toolbar.AddSimpleTool(
         40, gamera_icons.getIconChooseImageBitmap(),
         "Choose new image", self.OnChooseImage)

   def OnChooseImage(self, event):
      dlg = Args([Class("Image for context display", core.ImageBase)])
      function = dlg.show(self, image_menu.shell.locals, "")
      if function != None:
         function = function[1:-1]
         image = image_menu.shell.locals[function]
         self.id.set_image(image)

   def get_display(self):
      return ClassifierImageDisplay(self.toplevel, self)

class ClassifierFrame(ImageFrameBase):
   def __init__(self, classifier, symbol_table=[],
                parent = None, id = -1, title = "Classifier",
                owner=None):
      if not isinstance(classifier, classify.InteractiveClassifier):
         raise ValueError(
            "classifier must be instance of type InteractiveClassifier.")
      self._classifier = classifier

      if isinstance(symbol_table, SymbolTable):
         self._symbol_table = symbol_table
      elif util.is_sequence(symbol_table):
         self._symbol_table = SymbolTable()
         for symbol in symbol_table:
            if type(symbol) == types.StringType:
               self._symbol_table.add(symbol)
            else:
               raise ValueError(
                  "symbol_table must be a SymbolTable instance or a list of strings.")

      else:
         raise ValueError(
            "symbol_table must be a SymbolTable instance or a list of strings.")
      # Add 'actions' to symbol table
      for action in ImageBase.methods_flat_category("Action", ONEBIT):
         self._symbol_table.add("action." + action[0])
      # Add classifier database's symbols to the symbol table
      for glyph in self._classifier.database.iterkeys():
         for id in glyph.id_name:
            self._symbol_table.add(id[1])

      self.is_dirty = 0
      self.production_database_filename = None
      self.current_database_filename = None
      self.auto_move = []
      self.image = None
      self.menu = None
      self.default_segmenter = -1

      ImageFrameBase.__init__(self, parent, id, title, owner)
      from gamera.gui import gamera_icons
      icon = wxIconFromBitmap(gamera_icons.getIconClassifyBitmap())
      self._frame.SetIcon(icon)
      self._frame.CreateStatusBar(3)
      self._frame.SetSize((800, 600))
      self.splitterv = wxSplitterWindow(
         self._frame, -1,
         style=wxSP_FULLSASH|wxSP_3DSASH|wxSP_LIVE_UPDATE)
      self.splitterh = wxSplitterWindow(
         self.splitterv, -1,
         style=wxSP_FULLSASH|wxSP_3DSASH|wxSP_LIVE_UPDATE)
      self.single_iw = ClassifierImageWindow(self, self.splitterh)
      self.multi_iw = ClassifierMultiImageWindow(self, self.splitterh)
      self.splitterh.SetMinimumPaneSize(3)
      self.splitterh.SplitHorizontally(self.multi_iw, self.single_iw, 300)
      self.symbol_editor = SymbolTableEditorPanel(
         self._symbol_table, self, self.splitterv)
      self.splitterv.SetMinimumPaneSize(3)
      self.splitterv.SplitVertically(self.symbol_editor, self.splitterh, 160)
      self.create_menus()

   def create_menus(self):
      image_menu = gui_util.build_menu(
         self._frame,
         (("Open and segment image...", self._OnOpenAndSegmentImage),
          (None, None),
          ("Save current database as separate images...", self._OnSaveCurrentDatabaseAsImages),
          ("Save selected glyphs as separate images...", self._OnSaveSelectedAsImages),
          ("Save production database as separate images...", self._OnSaveProductionDatabaseAsImages)))
      xml_menu = gui_util.build_menu(
         self._frame,
         (("Save by criteria...", self._OnPowerSave),
          (None, None),
          ("Open production database...", self._OnOpenProductionDatabase),
          ("Merge into production database...", self._OnMergeProductionDatabase),
          ("Save production database", self._OnSaveProductionDatabase),
          ("Save production database as...", self._OnSaveProductionDatabaseAs),
          ("Clear production database...", self._OnClearProductionDatabase),
          (None, None),
          ("Open current database...", self._OnOpenCurrentDatabase),
          ("Merge into current database...", self._OnMergeCurrentDatabase),
          ("Save current database", self._OnSaveCurrentDatabase),
          ("Save current database as...", self._OnSaveCurrentDatabaseAs),
          ("Save selected glyphs as...", self._OnSaveSelectedGlyphsAs),
          (None, None),
          ("Import symbol names...", self._OnImportSymbolTable),
          ("Export symbol names...", self._OnExportSymbolTable)))
      classifier_menu = gui_util.build_menu(
         self._frame,
         (("Guess all", self._OnGuessAll),
          ("Guess selected", self._OnGuessSelected),
          (None, None),
          ("Confirm all", self._OnConfirmAll),
          ("Confirm selected", self._OnConfirmSelected),
          (None, None),
          ("Change set of features...", self._OnChangeSetOfFeatures),
          (None, None),
          ("Change classifier properties...", self._OnClassifierProperties),
          (None, None),
          ("Create noninteractive copy...", self._OnCreateNoninteractiveCopy)))
         
      menubar = wxMenuBar()
      menubar.Append(image_menu, "Image")
      menubar.Append(xml_menu, "XML")
      menubar.Append(classifier_menu, "Classifier")
      self._frame.SetMenuBar(menubar)

   def set_image(self, current_database, image=None, weak=1):
      self.set_multi_image(current_database)
      self.set_single_image(image, weak=weak)

   def set_multi_image(self, current_database):
      for glyph in current_database:
         for id in glyph.id_name:
            self._symbol_table.add(id[1])
      self.multi_iw.id.set_image(current_database)
      self.is_dirty = 1

   def set_single_image(self, image=None, weak=1):
      if image == None:
         if self.splitterh.IsSplit():
            self.splitterh.Unsplit()
            self.single_iw.Hide()
            del self.single_iw.id.image
            del self.single_iw.id.original_image
      else:
         self.single_iw.id.set_image(image, weak=weak)
         if not self.splitterh.IsSplit():
            self.splitterh.SplitHorizontally(
               self.multi_iw, self.single_iw, self._frame.GetSize()[1] / 2)
            self.single_iw.Show()
         
   def append_glyphs(self, list):
      self.multi_iw.id.append_glyphs(list)
      for glyph in list:
         for id in glyph.id_name:
            self._symbol_table.add(id[1])
      self.is_dirty = 1

   def delete_selected(self):
      self.multi_iw.id.delete_selected()
      self.is_dirty = 1

   def update_symbol_table(self):
      for glyph in self._classifier.database.iterkeys():
         for id in glyph.id_name:
            self._symbol_table.add(id[1])
      for glyph in self.multi_iw.id.GetAllItems():
         for id in glyph.id_name:
            self._symbol_table.add(id[1])

   ########################################
   # DISPLAY

   def display_cc(self, cc):
      if self.splitterh.IsSplit():
         self.single_iw.id.highlight_cc(cc)
         self.single_iw.id.focus(cc)

   def find_glyphs_in_rect(self, x1, y1, x2, y2, shift):
      self.multi_iw.id.find_glyphs_in_rect(x1, y1, x2, y2, shift)

   ########################################
   # CLASSIFICATION FUNCTIONS
   def guess_glyph(self, glyph):
      return self._classifier.guess_glyph(glyph)

   def classify_manual(self, id):
      assert type(id) == types.StringType
      wxBeginBusyCursor()
      selection = self.multi_iw.id.GetSelectedItems(
         self.multi_iw.id.GetGridCursorRow(),
         self.multi_iw.id.GetGridCursorCol())
      if selection != []:
         self.multi_iw.id.BeginBatch()
         removed_some = 0
         for glyph in selection:
            for child in glyph.children_images:
               child.dead = 1
               removed_some = 1
         splits = self._classifier.classify_list_manual(selection, id)
         if len(splits):
            self.append_glyphs(splits)
         if removed_some or len(splits):
            self.multi_iw.id.Refresh()
         if not self.do_auto_move():
            self.set_label_display([(1.0, id)])
         self.multi_iw.id.EndBatch()
         wxEndBusyCursor()


   ########################################
   # AUTO-MOVE

   def do_auto_move(self):
      self.multi_iw.id.do_auto_move(self.auto_move)
      return self.auto_move != []

   def set_label_display(self, ids):
      if ids != []:
         self.symbol_editor.tree.set_label_display(ids[0][1])
      else:
         self.symbol_editor.tree.set_label_display("")

   def display_label_at_cell(self, row, col, label):
      self.multi_iw.id.display_label_at_cell(row, col, label)

   def symbol_table_rename_callback(self, old, new):
      for glyph in self.multi_iw.id.list:
         new_ids = []
         if glyph != None:
            for id in glyph.id_name:
               if id[1] == old:
                  new_ids.append((id[0], new))
               else:
                  new_ids.append(id)
            glyph.id_name = new_ids
      self._classifier.rename_ids(old, new)

   ########################################
   # FILE MENU

   def _OnOpenAndSegmentImage(self, event):
      segmenters = [x[0] for x in ImageBase.methods_flat_category("Segmentation", ONEBIT)]
      if self.default_segmenter == -1:
         self.default_segmenter = segmenters.index("cc_analysis")
      dialog = Args(
         [FileOpen("Image file", "", "*.*"),
          Choice("Segmentation algorithm", segmenters, self.default_segmenter)])
      results = dialog.show(self._frame)
      if results is None:
         return
      filename, segmenter = results
      self.default_segmenter = segmenter
      if filename == "'None'":
         gui_util.message("You must provide a filename to load.")
      wxBeginBusyCursor()
      image = load_image(filename[1:-1])
      image_ref = image
      if image_ref.data.pixel_type == RGB:
         image_ref = image_ref.to_greyscale()
      if image_ref.data.pixel_type != ONEBIT:
         image_ref = image_ref.otsu_threshold()
      ccs = getattr(image_ref, segmenters[segmenter])()
      self.set_image(ccs, image, weak=0)
      wxEndBusyCursor()

   def _OnSaveCurrentDatabaseAsImages(self, event):
      self._OnSaveAsImages(self.multi_iw.id.GetAllItems())

   def _OnSaveSelectedAsImages(self, event):
      self._OnSaveAsImages(self.multi_iw.id.GetSelectedItems())

   def _OnSaveProductionDatabaseAsImages(self, event):
      self._OnSaveAsImages(self._classifier.database.keys())

   def _OnSaveAsImages(self, list):
      filename = gui_util.directory_dialog()
      if filename:
         classifier_stats.GlyphStats(list).write(filename)

   ########################################
   # CLASSIFIER MENU

   def _OnGuessAll(self, event):
      self._OnGuess(self.multi_iw.id.GetAllItems())
      self.multi_iw.id.sort_images(None)

   def _OnGuessSelected(self, event):
      self._OnGuess(self.multi_iw.id.GetSelectedItems())

   def _OnGuess(self, list):
      wxBeginBusyCursor()
      self._classifier.classify_list_automatic(list)
      wxEndBusyCursor()

   def _OnConfirmAll(self, event):
      self._OnConfirm(self.multi_iw.id.GetAllItems())

   def _OnConfirmSelected(self, event):
      self._OnConfirm(self.multi_iw.id.GetSelectedItems())

   def _OnConfirm(self, list):
      wxBeginBusyCursor()
      for x in list:
         if (x is not None and not hasattr(x, 'dead') and
             x.classification_state == AUTOMATIC):
            self._classifier.classify_glyph_manual(x, x.get_main_id())
      self.multi_iw.id.ForceRefresh()
      wxEndBusyCursor()
      
   def _OnChangeSetOfFeatures(self, event):
      all_features = [x[0] for x in ImageBase.methods_flat_category("Features", ONEBIT)]
      all_features.sort()
      existing_features = [x[0] for x in self._classifier.feature_functions]
      feature_controls = []
      for x in all_features:
         feature_controls.append(
            Check('', x, default=(x in existing_features)))
      dialog = Args(
         feature_controls,
         name='Feature selection', 
         title='Select the features you to use for classification')
      result = dialog.show(self._frame)
      if result is None:
         return
      selected_features = [name for check, name in zip(result, all_features) if check]
      self._classifier.change_feature_set(selected_features)

   def _OnClassifierProperties(self, event):
      gui_util.message("This will open up a classifier-specific dialog box...")

   def _OnCreateNoninteractiveCopy(self, event):
      name = var_name.get("classifier", image_menu.shell.locals)
      image_menu.shell.locals[name] = self._classifier.noninteractive_copy()
      image_menu.shell.update()
      
   ########################################
   # XML MENU

   def _OnPowerSave(self, event):
      dialog = Args(
         [Info('Set of glyphs to save:'),
          Check('', 'Production database', 1),
          Check('', 'Current database', 1),
          Info('Kind of glyphs to save:'),
          Check('', 'Unclassified', 1),
          Check('', 'Automatically classified', 1),
          Check('', 'Heuristically classified', 1),
          Check('', 'Manually classified', 1),
          FileSave('Save glyphs to file', '',
                   extension=gamera_xml.extensions)],
         name = 'Save by criteria...')
      results = dialog.show(self._frame)
      if results is None:
         return
      skip, proddb, currdb, skip, un, auto, heur, man, filename = results
      if ((proddb == 0 and currdb == 0) or
          (un == 0 and auto == 0 and heur == 0 and man == 0)):
         gui_util.message("You selected nothing to save!")
         return
      if filename == 'None':
         gui_util.message("You must select a filename to save into.")
         return
      # We build a dictionary here, since we don't want to save the
      # same glyph twice
      glyphs = {}
      if proddb:
         for glyph in self._classifier.database:
            glyphs[glyph] = None
      if currdb:
         for glyph in self.multi_iw.id.GetAllItems():
            glyphs[glyph] = None
      # One bigass filtering function
      glyphs = [x for x in glyphs.iterkeys()
                if ((x != None and not hasattr(x, 'dead')) and
                    ((x.classification_state == UNCLASSIFIED and un) or
                     (x.classification_state == AUTOMATIC and auto) or
                     (x.classification_state == HEURISTIC and heur) or
                     (x.classification_state == MANUAL and man)))]
      gamera_xml.WriteXMLFile(glyphs=glyphs,
         symbol_table=self._symbol_table).write_filename(
         filename[1:-1])

   def _OnOpenProductionDatabase(self, event):
      filename = gui_util.open_file_dialog(gamera_xml.extensions)
      if filename:
         wxBeginBusyCursor()
         self.production_database_filename = filename
         self._classifier.from_xml_filename(filename)
         self.update_symbol_table()
         wxEndBusyCursor()

   def _OnMergeProductionDatabase(self, event):
      filename = gui_util.open_file_dialog(gamera_xml.extensions)
      if filename:
         wxBeginBusyCursor()
         self._classifier.merge_from_xml_filename(filename)
         self.update_symbol_table()
         wxEndBusyCursor()

   def _OnSaveProductionDatabase(self, event):
      if self.production_database_filename == None:
         self._OnSaveProductionDatabaseAs(event)
      else:
         wxBeginBusyCursor()
         self._classifier.to_xml_filename(
            self.production_database_filename)
         wxEndBusyCursor()

   def _OnSaveProductionDatabaseAs(self, event):
      filename = gui_util.save_file_dialog(gamera_xml.extensions)
      if filename:
         self.production_database_filename = filename
         self._OnSaveProductionDatabase(event)

   def _OnClearProductionDatabase(self, event):
      if self._classifier.is_dirty:
         if gui_util.are_you_sure_dialog(
            "Are you sure you want to clear all glyphs in the production database?"):
            self._classifier.database = []
      else:
         self._classifier.database = []

   def _OnOpenCurrentDatabase(self, event):
      filename = gui_util.open_file_dialog(gamera_xml.extensions)
      if filename:
         wxBeginBusyCursor()
         glyphs = gamera_xml.LoadXMLGlyphs().parse_filename(filename)
         self.set_multi_image(glyphs)
         wxEndBusyCursor()

   def _OnMergeCurrentDatabase(self, event):
      filename = gui_util.open_file_dialog(gamera_xml.extensions)
      if filename:
         wxBeginBusyCursor()
         glyphs = gamera_xml.LoadXMLGlyphs().parse_filename(filename)
         self.append_glyphs(glyphs)
         wxEndBusyCursor()

   def _OnSaveCurrentDatabase(self, event):
      if self.current_database_filename == None:
         self._OnSaveCurrentDatabaseAs(event)
      else:
         wxBeginBusyCursor()
         glyphs = self.multi_iw.id.GetAllItems()
         progress = util.ProgressFactory("Generating features...")
         for i, glyph in util.enumerate(glyphs):
            glyph.generate_features(self._classifier.feature_functions)
            progress.update(i, len(glyphs))
         progress.update(1,1)
         gamera_xml.WriteXMLFile(
            glyphs=glyphs,
            symbol_table=self._symbol_table).write_filename(
            self.current_database_filename)
         wxEndBusyCursor()

   def _OnSaveCurrentDatabaseAs(self, event):
      filename = gui_util.save_file_dialog(gamera_xml.extensions)
      if filename:
         self.current_database_filename = filename
         self._OnSaveCurrentDatabase(event)

   def _OnSaveSelectedGlyphsAs(self, event):
      filename = gui_util.save_file_dialog(gamera_xml.extensions)
      if filename:
         wxBeginBusyCursor()
         glyphs = self.multi_iw.id.GetSelectedItems()
         progress = util.ProgressFactory("Generating features...")
         for i, glyph in util.enumerate(glyphs):
            glyph.generate_features(self._classifier.feature_functions)
            progress.update(i, len(glyphs))
         progress.update(1,1)
         gamera_xml.WriteXMLFile(
            glyphs=glyphs,
            symbol_table=self._symbol_table).write_filename(
            filename)
         wxEndBusyCursor()
         
   def _OnImportSymbolTable(self, event):
      filename = gui_util.open_file_dialog(gamera_xml.extensions)
      if filename:
         wxBeginBusyCursor()
         symbol_table = gamera_xml.LoadXMLSymbolTable().parse_filename(filename)
         for symbol in symbol_table.symbols.keys():
            self._symbol_table.add(symbol)
         wxEndBusyCursor()

   def _OnExportSymbolTable(self, event):
      filename = gui_util.save_file_dialog(gamera_xml.extensions)
      if filename:
         wxBeginBusyCursor()
         gamera_xml.WriteXMLFile(symbol_table=self._symbol_table).write_filename(filename)
         wxEndBusyCursor()

   ########################################
   # CALLBACKS

   def _OnCloseWindow(self, event):
      if self._classifier.is_dirty:
         if not gui_util.are_you_sure_dialog(
            "Are you sure you want to quit without saving?"):
            return
      #self.single_iw.Destroy()
      #self.multi_iw.Destroy()
      if self.owner:
         self.owner.set_display(None)
      self._frame.Destroy()

   def refresh(self):
      self.single_iw.id.refresh(1)


##############################################################################
# SYMBOL TABLE EDITOR
##############################################################################

class SymbolTreeCtrl(wxTreeCtrl):
   def __init__(self, toplevel, parent, id, pos, size, style):
      self.toplevel = toplevel
      self.editing = 0
      wxTreeCtrl.__init__(self, parent, id, pos, size, style)
      self.root = self.AddRoot("Symbols")
      self.SetItemHasChildren(self.root, TRUE)
      self.SetPyData(self.root, "")
      EVT_TREE_ITEM_EXPANDED(self, id, self.OnItemExpanded)
      EVT_TREE_ITEM_COLLAPSED(self, id, self.OnItemCollapsed)
      EVT_KEY_DOWN(self, self.OnKey)
      EVT_LEFT_DOWN(self, self.OnLeftDown)
      EVT_LEFT_DCLICK(self, self.OnLeftDoubleClick)
      self.Expand(self.root)
      self.toplevel._symbol_table.add_listener(self)

   def __del__(self):
      self._symbol_table.remove_listener(self)

   ########################################
   # CALLBACKS

   def set_label_display(self, symbol):
      self.toplevel.text.SetValue(symbol)
      self.toplevel.text.SetSelection(0, len(symbol))
      self.toplevel.OnText(None)
      self.toplevel.text.SetFocus()

   def symbol_table_callback(self, symbol):
      if symbol == '':
         self.CollapseAndReset(self.root)
         self.Expand(self.root)
         return
      item = self.GetFirstVisibleItem()
      while item.IsOk():
         item = self.GetNextVisible(item)
         if item.IsOk() and self.GetPyData(item) == symbol:
            if self.IsExpanded(item):
               self.CollapseAndReset(item)
               self.Expand(item)
               break

   def OnItemExpanded(self, event):
      parent = event.GetItem()
      parent_path = self.GetPyData(parent)
      items = self.toplevel._symbol_table.get_category_contents_list(parent_path)
      for name, is_parent in items:
         item = self.AppendItem(parent, name)
         new_path = '.'.join((parent_path, name))
         if new_path[0] == ".":
            new_path = new_path[1:]
         self.SetPyData(item, new_path)
         if new_path.startswith("action"):
            self.SetItemBackgroundColour(item, wxColor(0xcc, 0xcc, 0xff))
         if is_parent:
            self.SetItemHasChildren(item, TRUE)

   def OnItemCollapsed(self, event):
      self.CollapseAndReset(event.GetItem())

   def OnKey(self, evt):
      if evt.KeyCode() == WXK_DELETE:
         self.toplevel._symbol_table.remove(self.GetPyData(self.GetSelection()))
      else:
         evt.Skip()

   def OnLeftDoubleClick(self, event):
      pt = event.GetPosition()
      item, flags = self.HitTest(pt)
      if flags == 4:
         return
      id = self.GetPyData(item)
      if id != None:
         self.toplevel.toplevel.classify_manual(id)

   def OnLeftDown(self, event):
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
      wxPanel.__init__(self, parent, id)
      self.toplevel = toplevel
      self._symbol_table = symbol_table
      self.SetAutoLayout(true)
      self.box = wxBoxSizer(wxVERTICAL)
      txID = NewId()
      self.text = wxTextCtrl(self, txID)
      EVT_KEY_DOWN(self.text, self.OnKey)
      EVT_TEXT(self, txID, self.OnText)
      self.box.Add(self.text, 0, wxEXPAND|wxBOTTOM, 5)
      tID = NewId()
      self.tree = SymbolTreeCtrl(self, self, tID, wxDefaultPosition,
                                 wxDefaultSize,
                                 wxTR_HAS_BUTTONS)
      self.box.Add(self.tree, 1, wxEXPAND|wxALL)
      self.box.RecalcSizes()
      self.SetSizer(self.box)

   ########################################
   # CALLBACKS

   def OnKey(self, evt):
      find = self.text.GetValue()
      if evt.KeyCode() == WXK_TAB:
         find = self._symbol_table.autocomplete(find)
         self.text.SetValue(find)
         self.text.SetInsertionPointEnd()
      elif evt.KeyCode() == WXK_RETURN:
         self._symbol_table.add(find)
         self.toplevel.classify_manual(find)
      elif evt.KeyCode() == WXK_LEFT and evt.AltDown():
         current = self.text.GetInsertionPoint()
         new = max(self.text.GetValue().rfind(".", 0, current), 0)
         if evt.ShiftDown():
            self.text.SetSelection(new, current)
         self.text.SetInsertionPoint(new)
         return
      elif evt.KeyCode() == WXK_BACK and evt.AltDown():
         current = self.text.GetInsertionPoint()
         value = self.text.GetValue()
         if value == '':
            return
         if value[-1] == '.':
            value = value[:-1]
         new = value.rfind(".", 0, current)
         if new == -1:
            self.text.SetValue("")
         else:
            self.text.SetValue(self.text.GetValue()[0:new+1])
         self.text.SetInsertionPointEnd()
         return
      elif evt.KeyCode() == WXK_RIGHT and evt.AltDown():
         current = self.text.GetInsertionPoint()
         new = self.text.GetValue().find(".", current)
         if new == -1:
            if evt.ShiftDown():
               self.text.SetSelection(current, len(self.text.GetValue()))
            self.text.SetInsertionPointEnd()
         else:
            if evt.ShiftDown():
               self.text.SetSelection(current, new + 2)
            self.text.SetInsertionPoint(new + 1)
         return
      elif evt.KeyCode() == WXK_F12:
         self.toplevel.do_auto_move()
      else:
         evt.Skip()

   def OnText(self, evt):
      symbol, tokens = self._symbol_table.normalize_symbol(
         self.text.GetValue())
      parent = item = self.tree.root
      cookie = 0
      self.tree.UnselectAll()
      self.tree.SelectItem(parent)
      found = 1
      for token in tokens:
         if not found:
            break
         if self.tree.ItemHasChildren(item):
            parent = item
         else:
            break
         found = 0
         for i in range(self.tree.GetChildrenCount(parent)):
            if i == 0:
               item, cookie = self.tree.GetFirstChild(parent, cookie)
            else:
               item, cookie = self.tree.GetNextChild(parent, cookie)
            if not item.IsOk():
               return
            s = self.tree.GetPyData(item)
            last = s.split(".")[-1]
            if s != '' and last.startswith(token):
               if not self.tree.IsExpanded(item) and token != tokens[-1]:
                  self.tree.Expand(item)
               found = 1
               break
      self.tree.UnselectAll()
      if item.IsOk():
         self.tree.SelectItem(item)
         self.tree.ScrollTo(item)
         self.tree.Refresh()

# TODO: Add dialog to select kind of classifier

extensions = "XML files (*.xml)|*.xml*"
name = "Classifier Wizard"
class ClassifierWizard(Wizard):
   dlg_select_image = Args([
      Radio('Use an image that is already loaded', 'From memory'),
      Class('    Image variable name', Image),
      Radio('Load an image from disk', 'From disk'),
      FileOpen('    Image filename', '', extension="*.*")],
      name=name,
      function = 'cb_select_image',
      title = '1. Select an image to use in the classifier.')

   dlg_select_ccs = Args([
      Radio('Generate connected components from image', 'Generate'),
      Radio('Use connected components in memory', 'From memory'),
      Class('    Connected components list', Cc, list_of=1),
      Radio('Load connected components (current database)', 'From disk'),
      FileOpen('    Current database', extension=extensions),
      Check('Apply scaling', '', 0),
      Int('Scaling factor', (0, 100), 1)],
      name=name,
      function='cb_select_ccs',
      title = '2. How should the individual symbols (connected components) be obtained?')

   dlg_select_production_database = Args([
      Radio('Start with an empty database', 'Empty database'),
      Radio('Use an existing database for the classifier', 'Load database'),
      FileOpen('    Production database', extension=extensions)],
      name=name,
      function='cb_select_production_database',
      title='3. Optionally load the data used to classify symbols.')
   
   dlg_select_symbol_table = Args([
      FileOpen('Symbol table', extension=extensions)],
      name=name,
      function='cb_select_symbol_table',
      title=('You have chosen not to load any existing databases.\n' +
             'You may optionally load just the symbol names below.'))
   
   def __init__(self, shell, locals):
      self.shell = shell
      self.parent = None
      self.locals = locals
      self.ccs = 'None'
      self.production_database = 'None'
      self.current_database = 'None'
      self.symbol_table = 'None'
      self.context_image = 'None'
      self.features = 'None'
      self.show(self.dlg_select_image)

   def cb_select_image(self, memory, image_var, file, image_filename):
      if memory:
         self.context_image = image_var
      elif file:
         self.context_image = var_name.get("image", self.shell.locals)
         if image_filename == 'None':
            dlg = wxFileDialog(None, "Choose an image", ".", "", "*.*", wxOPEN)
            if dlg.ShowModal() == wxID_OK:
               image_filename = dlg.GetPath()
               dlg.Destroy()
         self.shell.run(self.context_image + " = load_image('" +
                             image_filename + "')")
      if self.context_image == None or self.context_image == 'None':
         self.dlg_select_image.title = ("That image is not valid." +
                                        "Please try again.")
         return self.dlg_select_image
      if (self.locals[self.context_image].data.pixel_type == GREYSCALE):
         self.shell.run("%s = %s.otsu_threshold()" %
                        (self.context_image, self.context_image))
      return self.dlg_select_ccs

   def cb_select_ccs(self, generate, use, list, file, filename, scaling, factor):
      if generate:
         ## Generate some ccs
         self.ccs = var_name.get("ccs", self.shell.locals)
         if scaling:
            self.shell.run(self.ccs + " = " + self.context_image + ".cc_analysis(%d)" % factor)
         else:
            self.shell.run(self.ccs + " = " + self.context_image + ".cc_analysis()")
      elif use:
         self.ccs = list
         if scaling:
            self.shell.run("for x in " + self.ccs + ":")
            self.shell.run("\tx.scaling = " + str(factor))
            self.shell.run("")
      else:
         if filename == 'None':
            dlg = wxFileDialog(None, "Choose a Gamera XML file",
                               ".", "", "*.xml", wxOPEN)
            if dlg.ShowModal() == wxID_OK:
               filename = dlg.GetPath()
               dlg.Destroy()
         self.ccs = var_name.get("ccs", self.shell.locals)
         self.shell.run(self.ccs + " = gamera_xml.LoadXMLGlyphs().parse_filename('" + filename + "')")
      return self.dlg_select_production_database

   def cb_select_production_database(self, empty, load, production_database):
      self.prod_db = ''
      if load:
         if production_database == 'None':
            dlg = wxFileDialog(None, "Choose a Gamera XML file",
                               ".", "", "*.xml", wxOPEN)
            if dlg.ShowModal() == wxID_OK:
               production_database = dlg.GetPath()
               dlg.Destroy()
         self.prod_db = var_name.get("production_db", self.shell.locals)
         self.shell.run(self.prod_db + " = gamera_xml.LoadXMLGlyphs().parse_filename('" + filename + "')")
      self.feature_list = core.ImageBase.methods_flat_category("Features", ONEBIT)
      self.feature_list = [x[0] for x in self.feature_list]
      self.feature_list.sort()
      feature_controls = []
      for key in self.feature_list:
         feature_controls.append(Check('', key, default=1))
      self.dlg_features = Args(
         feature_controls,
         name=name, function='cb_features',
         title='Select the features you would like to use for classification')
      return self.dlg_features

   def cb_features(self, *args):
      self.features = []
      for i in range(len(args)):
         if args[i]:
            self.features.append(self.feature_list[i])
      return self.dlg_select_symbol_table

   def cb_select_symbol_table(self, symbol_table):
      if symbol_table != "None":
         self.symbol_table = "'" + symbol_table + "'"
      return None

   def done(self):
      classifier = var_name.get("classifier", self.shell.locals)
      try:
         self.shell.run("from gamera import classify")
         self.shell.run("from gamera import knn")
         if self.prod_db != '':
            prod_db = "database=%s," % self.prod_db
         else:
            prod_db = ''
         self.shell.run("%s = classify.InteractiveClassifier(knn.kNN(), %s features=%s)" %
                        (classifier, prod_db, self.features))
         if self.symbol_table != 'None':
            symbol_table = ", symbol_table=%s" % self.symbol_table
         else:
            symbol_table = ''
         self.shell.run("%s.display(%s, context_image=%s%s)" %
                        (classifier, self.ccs, self.context_image, symbol_table))
      except Exception, e:
         print e
