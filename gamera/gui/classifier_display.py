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
# EXTENDED MULTI DISPLAY
###############################################################################

class ExtendedMultiImageDisplay(MultiImageDisplay):
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
      self.display_row_labels = False
      EVT_KEY_DOWN(self, self._OnKey)
      self._clearing = False
      EVT_SIZE(self.GetGridWindow(), self._OnSize)
      self._last_size_was_showing = True

   def _OnKey(self, event):
      if event.KeyCode() == WXK_F12:
         self.toplevel.do_auto_move()
      elif event.KeyCode() == WXK_F11:
         self.toplevel.move_back()
      event.Skip()

   def delete_selected(self):
      def _delete_selected(glyphs, setting):
         for glyph in glyphs:
            if (setting == 0 and hasattr(glyph, 'dead')) or setting == -1:
               if hasattr(glyph, 'dead'):
                  del glyph.__dict__['dead']
                  self.glyphs.append(glyph)
               if glyph.classification_state == MANUAL:
                  self.toplevel.add_to_database(glyph)
               _delete_selected(glyph.children_images, -1)
            elif (setting == 0 and not hasattr(glyph, 'dead')) or setting == 1:
               glyph.dead = True
               self.glyphs.remove(glyph)
               if glyph.classification_state == MANUAL:
                  self.toplevel.remove_from_database(glyph)
               _delete_selected(glyph.children_images, 1)
      _delete_selected(self.GetSelectedItems(), 0)

   def _OnSelectImpl(self, force=False):
      self.toplevel.get_other_multi(self).SafeClearSelection()
      ids = {}
      if not self.updating:
         images = self.GetSelectedItems()
         for x in images:
            ids[x.get_main_id()] = None
         ids = ids.keys()
         ids.sort()
         self.toplevel.set_number_of_glyphs_selected_status(len(images))
         self.toplevel.set_glyph_ids_status(ids)
         self._OnSelectImplDisplayCcs(images, force)
      if len(ids) == 1:
         self.toplevel.set_label_display(ids[0])
      else:
         self.toplevel.set_label_display('')

   def _OnSelect(self, event):
      if not self._clearing:
         event.Skip()
         self._OnSelectImpl()

   def SafeClearSelection(self):
      self._clearing = True
      self.ClearSelection()
      self._clearing = False

   def _OnSize(self, event):
      if self.frame.IsShown():
         if not self._last_size_was_showing:
            self.resize_grid()
            self.ForceRefresh()
            self._last_size_was_showing = True
      else:
         self._last_size_was_showing = False

   def resize_grid(self, do_auto_size=True):
      if not self.frame.IsShown():
         return
      return MultiImageDisplay.resize_grid(self, do_auto_size)

   def set_labels(self):
      if not self.frame.IsShown():
         return
      return MultiImageDisplay.set_labels(self)

   def RefreshSelected(self):
      if not self.frame.IsShown():
         return
      return MultiImageDisplay.RefreshSelected(self)

class ExtendedMultiImageWindow(MultiImageWindow):
   def __init__(self, toplevel, parent = None, id = -1, size = wxDefaultSize):
      self.toplevel = toplevel
      MultiImageWindow.__init__(self, parent, id, size)

      self.titlebar = wxStaticText(self, -1, self.pane_name)
      self.titlebar.SetBackgroundColour(wxColor(128,128,128))
      self.titlebar.SetForegroundColour(wxColor(255,255,255))
      font = self.titlebar.GetFont()
      font.SetWeight(wxBOLD)
      font.SetPointSize(14)
      self.titlebar.SetFont(font)
      self.titlebar_button = wxButton(self, -1, "+", size=(16, 16))
      EVT_BUTTON(self.titlebar_button, -1, self._OnClose)
      self._split_button_mode = False
      
      lc = wxLayoutConstraints()
      lc.top.SameAs(self, wxTop, 0)
      lc.left.SameAs(self, wxLeft, 0)
      lc.right.SameAs(self, wxRight, 0)
      lc.height.AsIs()
      self.titlebar.SetAutoLayout(True)
      self.titlebar.SetConstraints(lc)
      lc = wxLayoutConstraints()
      lc.top.SameAs(self, wxTop, 0)
      lc.right.SameAs(self, wxRight, 0)
      lc.height.AsIs()
      lc.width.AsIs()
      self.titlebar_button.SetAutoLayout(True)
      self.titlebar_button.SetConstraints(lc)
      lc = wxLayoutConstraints()
      lc.top.Below(self.titlebar, 0)
      lc.left.SameAs(self, wxLeft, 0)
      lc.right.SameAs(self, wxRight, 0)
      lc.height.AsIs()
      self.toolbar.SetAutoLayout(True)
      self.toolbar.SetConstraints(lc)
      self.Layout()

      from gamera.gui import gamera_icons
      self.toolbar.AddSeparator()
      self.toolbar.AddSimpleTool(
         300, gamera_icons.getIconDeleteBitmap(),
         "Delete selected glyphs",
         self.id._OnDelete)

   def set_close_button(self, mode):
      if mode:
         self.titlebar_button.SetLabel("X")
         self.titlebar_button.SetToolTipString(
            "Close this pane")
      else:
         self.titlebar_button.SetLabel("+")
         self.titlebar_button.SetToolTipString(
            "Split this pane to show classifier and page glyphs.")
      self._split_button_mode = mode

   def _OnClose(self, event):
      if self._split_button_mode:
         self.toplevel.unsplit_editors(self)
      else:
         self.toplevel.split_editors()

###############################################################################
# CLASSIFIER DISPLAY
###############################################################################

class ClassifierMultiImageDisplay(ExtendedMultiImageDisplay):
   def _OnDelete(self, event):
      self.delete_selected()
      self.RefreshSelected()
      self.toplevel.multi_iw.id.ForceRefresh()

   def _OnSelectImplDisplayCcs(self, images, force=False):
      pass

class ClassifierMultiImageWindow(ExtendedMultiImageWindow):
   pane_name = "Classifier glyphs"

   def __init__(self, toplevel, parent = None, id = -1, size = wxDefaultSize):
      ExtendedMultiImageWindow.__init__(self, toplevel, parent, id, size)
      self.toplevel._classifier.database.add_callback(
         'add',
         self.id.append_glyphs)
      self.toplevel._classifier.database.add_callback(
         'remove',
         self.id.remove_glyphs)

   def __del__(self):
      self.toplevel._classifier.database.remove_callback(
         'add',
         self.id.append_glyphs)
      self.toplevel._classifier.database.remove_callback(
         'remove',
         self.id.remove_glyphs)
      
   def get_display(self):
      return ClassifierMultiImageDisplay(self.toplevel, self)

###############################################################################
# PAGE DISPLAY
###############################################################################

class PageMultiImageDisplay(ExtendedMultiImageDisplay):
   ########################################
   # AUTO-MOVE

   def do_auto_move(self, state):
      # if auto-moving is turned off, just return
      if state == []:
         return

      candidate1 = self.GetSelectionBlockTopLeft()
      candidate2 = self.GetSelectedCells()
      if len(candidate1) and len(candidate2):
         row, col = min(candidate1[0], candidate2[0])
      elif len(candidate1):
         row, col = candidate1[0]
      elif len(candidate2):
         row, col = candidate2[0]
      else:
         row, col = self.GetGridCursorRow(), self.GetGridCursorCol()
      del candidate1
      del candidate2
      image_no = self.get_image_no(row, col)

      # Find the next glyph of the right type
      found = None
      list = self.sorted_glyphs
      for i in range(image_no + 1, len(list)):
         image = list[i]
         if (image != None and
             not hasattr(image, 'dead') and
             image.classification_state in state):
            found = i
            break
      if not found is None:
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
      image = self.sorted_glyphs[image_no]
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
      matches = util.sets.Set()
      if x1 == x2 or y1 == y2:
         point = Point(x1, y1)
         for i, g in enumerate(self.sorted_glyphs):
            if g != None and g.contains_point(Point(x1, y1)):
               for x, y in self._search_order:
                  if (g.contains_point(Point(x1 + x, y1 + y)) and
                      (g.get(y1 + y - g.ul_y, x1 + x - g.ul_x) != 0)):
                     matches.add(i)
                     break
      else:
         r = Rect(y1, x1, y2 - y1 + 1, x2 - x1 + 1)
         for i in range(len(self.sorted_glyphs)):
            g = self.sorted_glyphs[i]
            if g != None and r.contains_rect(g):
                  matches.add(i)
      first = True
      if shift:
         selected = util.sets.Set()
         for i in self.GetSelectedIndices():
            selected.add(i)
         new_matches = matches.symmetric_difference(selected)
         if len(new_matches) == len(matches) + len(selected):
            new_matches.difference_update(selected)
            first = False
         else:
            first = True
         matches = new_matches
      matches = list(matches)
      if len(matches):
         self.BeginBatch()
         self.updating = True
         last_index = matches[-1]
         for index in matches:
            row = index / self.cols
            col = index % self.cols
            if index == last_index:
               self.updating = False
            self.SelectBlock(row, col, row, col, not first)
            if first:
               self.MakeCellVisible(row, col)
               first = False
      else:
         if first:
            self.toplevel.single_iw.id.clear_all_highlights()
            self.ClearSelection()
      self.updating = False
      self.EndBatch()
      self.ForceRefresh()

   ########################################
   # DELETION

   def _OnDelete(self, event):
      self.delete_selected()
      self.RefreshSelected()
      self.toplevel.class_iw.id.ForceRefresh()

   ########################################
   # CALLBACKS

   # Display selected items in the context display
   def _OnSelectImplDisplayCcs(self, images, force=False):
      if images != self._last_selection or force:
         self._last_selection = images
         self.toplevel.display_cc(images)


class PageMultiImageWindow(ExtendedMultiImageWindow):
   pane_name = "Page glyphs"
   
   def __init__(self, toplevel, parent = None, id = -1, size = wxDefaultSize):
      ExtendedMultiImageWindow.__init__(self, toplevel, parent, id, size)
      from gamera.gui import gamera_icons
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
      return PageMultiImageDisplay(self.toplevel, self)

   def _OnAutoMoveButton(self, event):
      id = event.GetId() - 200
      if event.GetIsDown():
         self.toplevel.auto_move.append(id)
      else:
         self.toplevel.auto_move.remove(id)

class ClassifierImageDisplay(ImageDisplay):
   def __init__(self, toplevel, parent):
      self.toplevel = toplevel
      ImageDisplay.__init__(self, parent)
      self.SetToolTipString("Click or drag to select connected components.")

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
   status_bar_description = [
      ('menu_bar', "", -1),
      ('num_glyphs_classifier', "0 glyphs in classifier", -1),
      ('num_glyphs_page', "0 glyphs in page", -1),
      ('num_selected', "0 glyphs selected", -1),
      ('selected_ids', "", -1)]
   
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

      self.classifier_collection_filename = None
      self.page_collection_filename = None
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
      self._frame.CreateStatusBar(len(self.status_bar_description))
      status_bar = self._frame.GetStatusBar()
      self.status_bar_mapping = {}
      for i, (id, default, width) in enumerate(self.status_bar_description):
         status_bar.SetStatusText(default, i)
         self.status_bar_mapping[id] = i
      status_bar.SetStatusWidths([x[2] for x in self.status_bar_description])
      
      self._frame.SetSize((800, 600))
      self.splitterv = wxSplitterWindow(
         self._frame, -1,
         style=wxSP_FULLSASH|wxSP_3DSASH|wxCLIP_CHILDREN|
         wxNO_FULL_REPAINT_ON_RESIZE|wxSP_LIVE_UPDATE)
      self.splitterhr0 = wxSplitterWindow(
         self.splitterv, -1,
         style=wxSP_FULLSASH|wxSP_3DSASH|wxCLIP_CHILDREN|
         wxNO_FULL_REPAINT_ON_RESIZE|wxSP_LIVE_UPDATE)
      self.splitterhr1 = wxSplitterWindow(
         self.splitterhr0, -1,
         style=wxSP_FULLSASH|wxSP_3DSASH|wxCLIP_CHILDREN|
         wxNO_FULL_REPAINT_ON_RESIZE|wxSP_LIVE_UPDATE)
      self.splitterhl = wxSplitterWindow(
         self.splitterv, -1,
         style=wxSP_FULLSASH|wxSP_3DSASH|wxCLIP_CHILDREN|
         wxNO_FULL_REPAINT_ON_RESIZE|wxSP_LIVE_UPDATE)
      self.single_iw = ClassifierImageWindow(self, self.splitterhr0)
      self.multi_iw = PageMultiImageWindow(self, self.splitterhr1, id=2001)
      self.class_iw = ClassifierMultiImageWindow(self, self.splitterhr1, id=2000)
      self.splitterhr1.SetMinimumPaneSize(3)
      self.split_editors()
      self.unsplit_editors(self.class_iw)
      self.splitterhr0.SetMinimumPaneSize(3)
      self.splitterhr0.SplitHorizontally(self.splitterhr1, self.single_iw, 300)
      self.symbol_editor = SymbolTableEditorPanel(
         self._symbol_table, self, self.splitterhl)
      self.rule_engine_runner = rule_engine_runner.RuleEngineRunnerPanel(
         self, self.splitterhl)
      self.splitterhl.SetMinimumPaneSize(3)
      self.splitterhl.SplitHorizontally(
         self.symbol_editor, self.rule_engine_runner, 300)
      self.splitterhl.Unsplit()
      self.splitterv.SetMinimumPaneSize(3)
      self.splitterv.SplitVertically(self.splitterhl, self.splitterhr0, 160)
      self.create_menus()
      self._classifier.database.add_callback(
         'length_change',
         self.classifier_collection_length_change_callback)
      self.multi_iw.id.glyphs.add_callback(
         'length_change',
         self.page_collection_length_change_callback)
      self.class_iw.id.set_image([])

   def __del__(self):
      self._classifier.database.remove_callback(
         'length_change',
         self.classifier_collection_length_change_callback)
      self.multi_iw.id.glyphs.remove_callback(
         'length_change',
         self.page_collection_length_change_callback)

   def create_menus(self):
      file_menu = gui_util.build_menu(
         self._frame,
         (("&Open all...", self._OnOpenAll),
          ("&Save all...", self._OnSaveAll),
          (None, None),
          ("Save &by criteria...", self._OnSaveByCriteria),
          (None, None),
          ("&Classifier glyphs",
           (("Open glyphs into classifier...", self._OnOpenClassifierCollection),
            ("Merge glyphs into classifier...", self._OnMergeClassifierCollection),
            ("Save glyphs in classifier", self._OnSaveClassifierCollection),
            ("Save glyphs in classifier as...", self._OnSaveClassifierCollectionAs),
            ("Clear glyphs in classifier", self._OnClearClassifierCollection))),
          ("&Page glyphs",
           (("Open glyphs into page editor...", self._OnOpenPageCollection),
            ("Merge glyphs into page editor...", self._OnMergePageCollection),
            ("Save glyphs in page editor", self._OnSavePageCollection),
            ("Save glyphs in page editor as...", self._OnSavePageCollectionAs))),
          ("Save selected glyphs &as...", self._OnSaveSelectedGlyphs),
          (None, None),
          ("&Symbol names",
           (("&Import...", self._OnImportSymbolTable),
            ("&Export...", self._OnExportSymbolTable)))))
      image_menu = gui_util.build_menu(
         self._frame,
         (("&Open and segment image...", self._OnOpenAndSegmentImage),
          ("Se&lect and segment image...", self._OnSelectAndSegmentImage),
          ("Se&lect image...", self._OnSelectImage),
          (None, None),
          ("&Save glyphs into separate files",
           (("&Classifier glyphs...", self._OnSaveClassifierCollectionAsImages),
            ("&Page glyphs...", self._OnSavePageCollectionAsImages),
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
         ("Display/Hide classifier glyphs", self._OnDisplayContents),
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

   def is_dirty(self):
      return self.multi_iw.id.is_dirty or self._classifier.database.is_dirty
   is_dirty = property(is_dirty)

   def get_other_multi(self, id):
      if id == self.multi_iw.id:
         return self.class_iw.id
      else:
         return self.multi_iw.id

   def set_image(self, page_collection, image=None, weak=True):
      self.set_multi_image(page_collection)
      self.set_single_image(image, weak=weak)

   def set_multi_image(self, page_collection):
      wxBeginBusyCursor()
      try:
         for glyph in page_collection:
            for id in glyph.id_name:
               self._symbol_table.add(id[1])
         self.multi_iw.id.set_image(page_collection)
      finally:
         wxEndBusyCursor()

   def set_single_image(self, image=None, weak=True):
      if image == None:
         if self.splitterhr0.IsSplit():
            self.splitterhr0.Unsplit()
            self.single_iw.Hide()
            del self.single_iw.id.image
            del self.single_iw.id.original_image
      else:
         self.single_iw.id.set_image(image, weak=weak)
         if not self.splitterhr0.IsSplit():
            self.splitterhr0.SplitHorizontally(
               self.splitterhr1, self.single_iw, self._frame.GetSize()[1] / 2)
            if self.splitterhr1.IsSplit():
               self.splitterhr1.SetSashPosition(self._frame.GetSize()[1] / 4)
            self.single_iw.Show()

   def unsplit_editors(self, display):
      self.splitterhr1.Unsplit(display)
      display.Show(False)
      for id in (self.multi_iw, self.class_iw):
         id.set_close_button(False)

   def split_editors(self):
      self.splitterhr1.SplitHorizontally(
         self.class_iw, self.multi_iw,
         self.splitterhr1.GetSize()[1] / 2)
      self.class_iw.Show(True)
      self.multi_iw.Show(True)
      for id in (self.multi_iw, self.class_iw):
         id.set_close_button(True)
         
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

   def get_all_selected(self):
      for display in (self.multi_iw.id, self.class_iw.id):
         if display.IsSelection():
            selection = display.GetSelectedItems(display.GetGridCursorRow(),
                                            display.GetGridCursorCol())
            active_id = display
            inactive_id = self.get_other_multi(display)
            break
      return selection, active_id, inactive_id

   def display_cc(self, cc):
      if self.splitterhr0.IsSplit():
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
      self._frame.GetStatusBar().SetStatusText(text, self.status_bar_mapping['num_selected'])

   def set_glyph_ids_status(self, ids):
      self._frame.GetStatusBar().SetStatusText(", ".join(ids), self.status_bar_mapping['selected_ids'])

   def classifier_collection_length_change_callback(self, length):
      self._frame.GetStatusBar().SetStatusText(
         "%d glyphs in classifier" % length,
         self.status_bar_mapping['num_glyphs_classifier'])

   def page_collection_length_change_callback(self, length):
      self._frame.GetStatusBar().SetStatusText(
         "%d glyphs in page" % length,
         self.status_bar_mapping['num_glyphs_page'])

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
      selection, active_id, inactive_id = self.get_all_selected()
      if len(selection):
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
         self.multi_iw.id.is_dirty = True
         active_id.RefreshSelected()
         inactive_id.ForceRefresh()
         if active_id == self.multi_iw.id:
            if not self.do_auto_move():
               self.multi_iw.id._OnSelectImpl(True)

   def set_glyph_ids_status(self, ids):
      self._frame.GetStatusBar().SetStatusText(
         ", ".join(ids), self.status_bar_mapping['selected_ids'])


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
          Check('', 'Page glyphs', self._save_state_dialog[1]),
          Check('', 'Classifier glyphs', self._save_state_dialog[2]),
          Check('', 'Symbol table', self._save_state_dialog[3]),
          Check('', 'Source image', self._save_state_dialog[4]),
          Directory('Save directory')], name="Open classifier window")
      results = dialog.show(self._frame)
      if results == None:
         return
      self._save_state_dialog = results
      settings, page, classifier, symbols, source, directory = results
      if directory == None:
         gui_util.message("You must provide a directory to load.")
         return
      if settings:
         self._OpenClassifierSettings(
            os.path.join(directory, "classifier_settings.xml"))
      if page:
         if os.path.exists(os.path.join(directory, "current_database.xml")):
            self._OpenPageCollection(
               [os.path.join(directory, "current_database.xml")])
         else:
            self._OpenPageCollection(
               [os.path.join(directory, "page_glyphs.xml")])
      if classifier:
         if os.path.exists(os.path.join(directory, "production_database.xml")):
            self._OpenClassifierCollection(
               [os.path.join(directory, "production_database.xml")])
         else:
            self._OpenClassifierCollection(
               [os.path.join(directory, "classifier_glyphs.xml")])
      if symbols:
         self._ImportSymbolTable(
            os.path.join(directory, "symbol_table.xml"))
      if source:
         try:
            self.set_single_image(
               load_image(os.path.join(directory, "source_image.tiff")), False)
         except Exception, e:
            gui_util.message("Loading image: " + str(e))

   def _OnSaveAll(self, event):
      dialog = Args(
         [Check('', 'Classifier settings', self._save_state_dialog[0]),
          Check('', 'Page glyphs', self._save_state_dialog[1]),
          Check('', 'Classifier glyphs', self._save_state_dialog[2]),
          Check('', 'Symbol table', self._save_state_dialog[3]),
          Check('', 'Source image', self._save_state_dialog[4],
                enabled=self.splitterhr0.IsSplit()),
          Directory('Save directory')], name="Save classifier window")
      results = dialog.show(self._frame)
      if results == None:
         return
      self._save_state_dialog = results
      settings, page, classifier, symbols, source, directory = results
      if directory == None:
         gui_util.message("You must provide a directory to load.")
         return
      error_messages = util.sets.Set()
      if settings:
         try:
            self._SaveClassifierSettings(
               os.path.join(directory, "classifier_settings.xml"))
         except Exception, e:
            error_messages.add(str(e))
      if page:
         try:
            self._SavePageCollection(
               os.path.join(directory, "page_glyphs.xml"))
         except:
            error_messages.add(str(e))
         self.multi_iw.id.is_dirty = False
      if classifier:
         try:
            self._SaveClassifierCollection(
               os.path.join(directory, "classifier_glyphs.xml"))
         except:
            error_messages.add(str(e))
         self._classifier.is_dirty = False
      if symbols:
         try:
            self._ExportSymbolTable(os.path.join(directory, "symbol_table.xml"))
         except:
            error_messages.add(str(e))
      if source and self.splitterhr0.IsSplit():
         try:
            self.single_iw.id.image.save_tiff(
               os.path.join(directory, "source_image.tiff"))
         except Exception, e:
            gui_util.message("Saving image: " + str(e))
      if len(error_messages):
         gui_util.message("There were errors during the save.\n\n" +
                          "\n".join(list(error_messages)))

   def _OnSaveByCriteria(self, event):
      dialog = Args(
         [Info('Set of glyphs to save:'),
          Check('', 'Classifier glyphs', self._save_by_criteria_dialog[1]),
          Check('', 'Page glyphs', self._save_by_criteria_dialog[2]),
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
         skip, classifier, page, skip, un, auto, heur, man, filename = results
         if ((classifier == 0 and page == 0) or
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

      glyphs = util.sets.Set()
      if classifier:
         glyphs.update(self._classifier.get_glyphs())
      if page:
         glyphs.update(self.multi_iw.id.GetAllItems())

      # One big filtering list comprehension
      glyphs = [x for x in glyphs
                if ((x != None and not hasattr(x, 'dead')) and
                    ((x.classification_state == UNCLASSIFIED and un) or
                     (x.classification_state == AUTOMATIC and auto) or
                     (x.classification_state == HEURISTIC and heur) or
                     (x.classification_state == MANUAL and man)))]
      if gui_util.are_you_sure_dialog(
         ("By the given filtering criteria, %d glyphs will be saved.\n" +
          "Are you sure you want to continue?") % len(glyphs)):
         try:
            gamera_xml.WriteXMLFile(
               glyphs=glyphs,
               symbol_table=self._symbol_table).write_filename(
                  filename)
         except gamera_xml.XMLError, e:
            gui_util.message("Saving by criteria: " + str(e))
         
   def _OnOpenClassifierCollection(self, event):
      if self._classifier.is_dirty:
         if not gui_util.are_you_sure_dialog(
            ("Classifier glyphs have not been saved and will be replaced.\n" +
             "Are you sure you want to load glyphs into the classifier?")):
            return
      filenames = gui_util.open_file_dialog(self._frame, gamera_xml.extensions,
                                           multiple=1)
      if not filenames is None:
         if len(filenames) == 1:
            self.classifier_collection_filename = filenames[0]
         else:
            self.classifier_collection_filename = None
         self._OpenClassifierCollection(filenames)

   def _OpenClassifierCollection(self, filenames):
      self.class_iw.id.set_image([])
      try:
         self._classifier.from_xml_filename(filenames[0])
         if len(filenames) > 1:
            for f in filenames[1:]:
               self._classifier.merge_from_xml_filename(f)
      except gamera_xml.XMLError, e:
         gui_util.message("Opening classifier glyphs: " + str(e))
         return
      self.update_symbol_table()
      self._classifier.is_dirty = False
      self.class_iw.id.resize_grid()

   def _OnMergeClassifierCollection(self, event):
      filenames = gui_util.open_file_dialog(self._frame, gamera_xml.extensions,
                                            multiple=1)
      if not filenames is None:
         try:
            for f in filenames:
               self._classifier.merge_from_xml_filename(f)
         except gamera_xml.XMLError, e:
            gui_util.message("Merging classifier glyphs: " + str(e))
            return
         self.update_symbol_table()

   def _OnSaveClassifierCollection(self, event):
      if self.classifier_collection_filename == None:
         self._OnSaveClassifierCollectionAs(event)
      else:
         if gui_util.are_you_sure_dialog(
            ("There are %d glyphs in the classifier.\n" +
             "Are you sure you want to save?") % len(self._classifier.database)):
            self._SaveClassifierCollection(self.classifier_collection_filename)

   def _OnSaveClassifierCollectionAs(self, event):
      if gui_util.are_you_sure_dialog(
         ("There are %d glyphs in the classifier.\n" +
          "Are you sure you want to save?") % len(self._classifier.database)):
         filename = gui_util.save_file_dialog(self._frame, gamera_xml.extensions)
         if filename:
            self.classifier_collection_filename = filename
            self._SaveClassifierCollection(filename)
         
   def _SaveClassifierCollection(self, filename):
         try:
            self._classifier.to_xml_filename(filename)
         except gamera_xml.XMLError, e:
            gui_util.message("Saving classifier glyphs: " + str(e))

   def _OnClearClassifierCollection(self, event):
      if self._classifier.is_dirty:
         if gui_util.are_you_sure_dialog(self._frame,
            "Are you sure you want to clear all glyphs in the classifier?"):
            self._classifier.clear_glyphs()
      else:
         self._classifier.clear_glyphs()
      self._classifier.is_dirty = False

   def _OnOpenPageCollection(self, event):
      if self.multi_iw.id.is_dirty:
         if not gui_util.are_you_sure_dialog(
            ("Page glyphs have not been saved and will be replaced.\n" +
             "Are you sure you want to load glyphs into the page glyphs pane?")):
            return
      filenames = gui_util.open_file_dialog(self._frame, gamera_xml.extensions,
                                            multiple=1)
      if not filenames is None:
         if len(filenames) == 1:
            self.page_collection_filename = filenames[0]
         else:
            self.page_collection_filename = None
         self._OpenPageCollection(filenames)

   def _OpenPageCollection(self, filenames):
      try:
         glyphs = gamera_xml.LoadXML().parse_filename(filenames[0]).glyphs
         if len(filenames) > 1:
            for f in filenames[1:]:
               glyphs.extend(gamera_xml.LoadXML().parse_filename(f).glyphs)
      except gamera_xml.XMLError, e:
         gui_util.message("Opening page glyphs: " + str(e))
         return
      self.set_multi_image(glyphs)
      self.multi_iw.id.is_dirty = False

   def _OnMergePageCollection(self, event):
      filenames = gui_util.open_file_dialog(self._frame, gamera_xml.extensions,
                                            multiple=1)
      if not filenames is None:
         glyphs = []
         try:
            for f in filenames:
               glyphs.extend(gamera_xml.LoadXML().parse_filename(f).glyphs)
         except gamera_xml.XMLError, e:
            gui_util.message("Merging page glyphs: " + str(e))
            return
         self.multi_iw.id.append_glyphs(glyphs)

   def _OnSavePageCollection(self, event):
      glyphs = self.multi_iw.id.GetAllItems()
      if self.page_collection_filename == None:
         self._OnSavePageCollectionAs(event)
      else:
         if gui_util.are_you_sure_dialog(
            ("There are %d glyphs in the page glyphs pane.\n" +
             "Are you sure you want to save?") % len(glyphs)):
            self._SavePageCollection(self.page_collection_filename, glyphs)

   def _OnSavePageCollectionAs(self, event):
      glyphs = self.multi_iw.id.GetAllItems()
      if gui_util.are_you_sure_dialog(
         ("There are %d glyphs in the page glyphs pane.\n" +
          "Are you sure you want to save?") % len(glyphs)):
         filename = gui_util.save_file_dialog(self._frame, gamera_xml.extensions)
         if filename:
            self.page_collection_filename = filename
            self._SavePageCollection(filename, glyphs)

   def _SavePageCollection(self, filename, glyphs=None):
      if glyphs is None:
         glyphs = self.multi_iw.id.GetAllItems()
      self._classifier.generate_features(glyphs)
      try:
         gamera_xml.WriteXMLFile(
            glyphs=glyphs,
            symbol_table=self._symbol_table).write_filename(
            filename)
      except gamera_xml.XMLError, e:
         gui_util.message("Saving page glyphs: " + str(e))

   def _OnSaveSelectedGlyphs(self, event):
      glyphs = self.get_all_selected()[0]
      if len(glyphs) == 0:
         gui_util.message("There are no selected glyphs.")
         return
      if gui_util.are_you_sure_dialog(
         ("There are %d selected glyphs.\n" +
          "Are you sure you want to save?") % len(glyphs)):
         filename = gui_util.save_file_dialog(self._frame, gamera_xml.extensions)
         if filename:
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
   # IMAGE MENU

   def _OnOpenAndSegmentImage(self, event):
      if self.multi_iw.id.is_dirty:
         if not gui_util.are_you_sure_dialog("Editing glyphs have not been saved.  Are you sure you wish to proceed?"):
            return
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
      self.multi_iw.id.is_dirty = False
      wxEndBusyCursor()

   def _OnSelectAndSegmentImage(self, event):
      if self.multi_iw.id.is_dirty:
         if not gui_util.are_you_sure_dialog("Editing glyphs have not been saved.  Are you sure you wish to proceed?"):
            return
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
      self.multi_iw.id.is_dirty = False
      wxEndBusyCursor()

   def _OnSelectImage(self, event):
      dialog = Args(
         [Class("Image", ImageBase)],
         name="Select image...")
      results = dialog.show(self._frame, image_menu.shell.locals)
      if results is None:
         return
      (image,) = results
      self.set_single_image(image, weak=False)
      
   def _segment_image(self, image, segmenter):
      image_ref = image
      image_ref = image_ref.to_onebit()
      ccs = getattr(image_ref, segmenter)()
      self.set_image(ccs, image, weak=False)

   def _OnSavePageCollectionAsImages(self, event):
      self._OnSaveAsImages(self.multi_iw.id.GetAllItems())

   def _OnSaveSelectedAsImages(self, event):
      self._OnSaveAsImages(self.get_all_selected()[0])

   def _OnSaveClassifierCollectionAsImages(self, event):
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
      selected = list(self.multi_iw.id.GetSelectedItems())
      if len(selected) == 0:
         gui_util.message("No glyphs are selected in the page glyphs pane.")
         return
      self._OnGuess(selected)

   def _OnGuess(self, list):
      try:
         added, removed = self._classifier.classify_list_automatic(list)
      except ClassifierError, e:
         gui_util.message(str(e))
      self._AdjustAfterGuess(added, removed)

   def _OnGroupAndGuessAll(self, event):
      self._OnGroupAndGuess(self.multi_iw.id.GetAllItems())

   def _OnGroupAndGuessSelected(self, event):
      selected = list(self.multi_iw.id.GetSelectedItems())
      if len(selected) == 0:
         gui_util.message("No glyphs are selected in the page glyphs pane.")
         return
      self._OnGroupAndGuess(selected)

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
      if gui_util.are_you_sure_dialog("This function will treat all automatically classified glyphs \n" +
                                      "on the page as manually classified and add them to the classifier.\n" +
                                      "Proceed?"):
         self._OnConfirm(self.multi_iw.id.GetAllItems())

   def _OnConfirmSelected(self, event):
      selected = list(self.multi_iw.id.GetSelectedItems())
      if len(selected) == 0:
         gui_util.message("No glyphs are selected in the page glyphs pane.")
         return
      if gui_util.are_you_sure_dialog("This function will treat all selected automatically\n" +
                                      "classified glyphs in the page glyphs pane as manually\n" +
                                      "classified and add them to the classifier.  Proceed?"):
         self._OnConfirm(selected)

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
      self.class_iw.id.resize_grid()
      
   def _OnChangeSetOfFeatures(self, event):
      all_features = [x[0] for x in
                      ImageBase.methods_flat_category("Features", ONEBIT)]
      all_features.sort()
      existing_features = [x[0] for x in
                           self._classifier.get_feature_functions()[0]]
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
      selected_features = [name for check, name in
                           zip(result, all_features) if check]
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

   def _OnDisplayContents(self, event):
      if self.splitterhr1.IsSplit():
         self.unsplit_editors(self.class_iw)
      else:
         self.split_editors()
         
   def _SaveClassifierCollection(self, filename):
         try:
            self._classifier.to_xml_filename(filename)
         except gamera_xml.XMLError, e:
            gui_util.message("Saving classifier glyphs: " + str(e))

   def _OnClearClassifierCollection(self, event):
      if self._classifier.is_dirty:
         if gui_util.are_you_sure_dialog(self._frame,
            "Are you sure you want to clear all glyphs in the classifier?"):
            self._classifier.clear_glyphs()
      else:
         self._classifier.clear_glyphs()
      self._classifier.is_dirty = False

   def _OnOpenEditorCollection(self, event):
      if self.multi_iw.id.is_dirty:
         if not gui_util.are_you_sure_dialog("Editor glyphs have not been saved and will be replaced.\nAre you sure you want to load glyphs into the classifier?"):
            return
      filenames = gui_util.open_file_dialog(self._frame, gamera_xml.extensions,
                                            multiple=1)
      if not filenames is None:
         if len(filenames) == 1:
            self.editor_collection_filename = filenames[0]
         else:
            self.editor_collection_filename = None
         self._OpenEditorCollection(filenames)

   def _OpenEditorCollection(self, filenames):
      try:
         glyphs = gamera_xml.LoadXML().parse_filename(filenames[0]).glyphs
         if len(filenames) > 1:
            for f in filenames[1:]:
               glyphs.extend(gamera_xml.LoadXML().parse_filename(f).glyphs)
      except gamera_xml.XMLError, e:
         gui_util.message("Opening editor glyphs: " + str(e))
         return
      self.set_multi_image(glyphs)
      self.multi_iw.id.is_dirty = False

   def _OnMergeEditorCollection(self, event):
      filenames = gui_util.open_file_dialog(self._frame, gamera_xml.extensions,
                                            multiple=1)
      if not filenames is None:
         glyphs = []
         try:
            for f in filenames:
               glyphs.extend(gamera_xml.LoadXML().parse_filename(f).glyphs)
         except gamera_xml.XMLError, e:
            gui_util.message("Merging editor glyphs: " + str(e))
            return
         self.multi_iw.id.append_glyphs(glyphs)

   def _OnSaveEditorCollection(self, event):
      if self.editor_collection_filename == None:
         self._OnSaveEditorCollectionAs(event)
      else:
         glyphs = self.multi_iw.id.GetAllItems()
         if gui_util.are_you_sure_dialog("There are %d glyphs in the editor.\nAre you sure you want to save?" % len(glyphs)):
            self._SaveEditorCollection(self.editor_collection_filename)

   def _OnSaveEditorCollectionAs(self, event):
      glyphs = self.multi_iw.id.GetAllItems()
      if gui_util.are_you_sure_dialog("There are %d glyphs in the editor.\nAre you sure you want to save?" % len(glyphs)):
         filename = gui_util.save_file_dialog(self._frame, gamera_xml.extensions)
         if filename:
            self.editor_collection_filename = filename
            self._SaveEditorCollection(self.editor_collection_filename)

   def _SaveEditorCollection(self, filename, force=False):
      glyphs = self.multi_iw.id.GetAllItems()
      self._classifier.generate_features(glyphs)
      try:
         gamera_xml.WriteXMLFile(
            glyphs=glyphs,
            symbol_table=self._symbol_table).write_filename(
            filename)
      except gamera_xml.XMLError, e:
         gui_util.message("Saving editor glyphs: " + str(e))

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
      if self.is_dirty:
         if not gui_util.are_you_sure_dialog(
            "Are you sure you want to quit without saving?",
            self._frame):
            event.Veto()
            return
      self._classifier.set_display(None)
      self.multi_iw.Destroy()
      self.single_iw.Destroy()
      self.splitterhr1.Destroy()
      self.splitterhr0.Destroy()
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
      self.toplevel._symbol_table.add_callback(
         'add', self.symbol_table_add_callback)
      self.toplevel._symbol_table.add_callback(
         'remove', self.symbol_table_remove_callback)
      self.Expand(self.root)
      self.SelectItem(self.root)

   def __del__(self):
      self._symbol_table.remove_callback(
         'add', self.symbol_table_add_callback)
      self._symbol_table.remove_callback(
         'remove', self.symbol_table_remove_callback)

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

   def symbol_table_add_callback(self, tokens):
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
