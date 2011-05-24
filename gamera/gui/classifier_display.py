# -*- mode: python; indent-tabs-mode: nil; tab-width: 3 -*-
# vim: set tabstop=3 shiftwidth=3 expandtab:
#
# Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom,
#                         and Karl MacMillan
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
import wx
from wx import grid
from wx.lib import buttons
try:
   from wx import aui
   # AUI has too many problems for now
   aui = None
except ImportError:
   aui = None
from gamera.core import *
from gamera.args import *
from gamera.symbol_table import SymbolTable
from gamera import gamera_xml, util, plugin
from gamera.classify import InteractiveClassifier, ClassifierError, BoundingBoxGroupingFunction, ShapedGroupingFunction
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
      grid.EVT_GRID_RANGE_SELECT(self, self._OnSelect)
      self.last_sort = None
      # This is to turn off the display of row labels if a)
      # we have done some classification and b) we have done a
      # sort other than the default. KWM
      self.display_row_labels = False
      wx.EVT_KEY_DOWN(self, self._OnKey)
      self._clearing = False
      wx.EVT_SIZE(self.GetGridWindow(), self._OnSize)
      self._last_size_was_showing = True

   def _OnKey(self, event):
      keycode = event.GetKeyCode()

      if keycode == wx.WXK_F12:
         self.toplevel.do_auto_move()
      elif keycode == wx.WXK_F11:
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
         self.updating = True
         images = self.GetSelectedItems()
         for x in images:
            id = x.get_main_id()
            if not ids.has_key(id):
               ids[id] = 1
            else:
               ids[id] += 1
         ids = [(val, key) for (key, val) in ids.items()]
         ids.sort()
         ids.reverse()
         ids = [x[1] for x in ids]
         self.toplevel.set_number_of_glyphs_selected_status(len(images))
         self.toplevel.set_glyph_ids_status(ids)
         self._OnSelectImplDisplayCcs(images, force)
         self.updating = False
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
            self._last_size_was_showing = True
         # self.ForceRefresh()
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
   def __init__(self, toplevel, parent = None, id = -1, size = wx.DefaultSize):
      from gamera.gui import gamera_icons

      self.toplevel = toplevel
      MultiImageWindow.__init__(self, parent, id, size)

      self.toolbar.AddSeparator()
      self.toolbar.AddSimpleTool(
         300, gamera_icons.getIconDeleteBitmap(),
         "Delete selected glyphs",
         self.id._OnDelete)

      if not aui:
         self.titlebar_text = wx.StaticText(self, -1, self.pane_name,
                                            style = wx.ALIGN_CENTRE)
         font = self.titlebar_text.GetFont()
         font.SetWeight(wx.BOLD)
         self.titlebar_text.SetFont(font)
         if wx.Platform != '__WXGTK__':
            self.titlebar_text.SetForegroundColour(wx.Colour(255,255,255))
            self.titlebar_text.SetBackgroundColour(wx.Colour(128,128,128))
         if hasattr(buttons, 'ThemedGenBitmapButton'):
            TitleBarButtonClass = buttons.ThemedGenBitmapButton
         else:
            TitleBarButtonClass = wx.BitmapButton
         self.titlebar_button = TitleBarButtonClass(
            self, -1,
            gamera_icons.getPlusBitmap())
         wx.EVT_BUTTON(self.titlebar_button, -1, self._OnClose)
         self._split_button_mode = False

         title_sizer = wx.BoxSizer(wx.HORIZONTAL)
         title_sizer.Add(self.titlebar_text, 1, wx.EXPAND)
         title_sizer.Add(self.titlebar_button, 0, wx.EXPAND)
         self.box_sizer.Insert(0, title_sizer, 0, wx.EXPAND)

   if not aui:
      def set_close_button(self, mode):
         from gamera.gui import gamera_icons

         if mode:
            bitmap = gamera_icons.getXBitmap()
            self.titlebar_button.SetToolTipString(
               "Close this pane")
         else:
            bitmap = gamera_icons.getPlusBitmap()
            self.titlebar_button.SetToolTipString(
               "Split this pane to show classifier and page glyphs.")
         self.titlebar_button.SetBitmapLabel(bitmap)
         self.titlebar_button.SetBitmapDisabled(bitmap)
         self.titlebar_button.SetBitmapFocus(bitmap)
         self.titlebar_button.SetBitmapSelected(bitmap)
         self.titlebar_button.Refresh()
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
      # self.toplevel.multi_iw.id.ForceRefresh()

   def _OnSelectImplDisplayCcs(self, images, force=False):
      pass

class ClassifierMultiImageWindow(ExtendedMultiImageWindow):
   pane_name = "Classifier glyphs"

   def __init__(self, toplevel, parent = None, id = -1, size = wx.DefaultSize):
      ExtendedMultiImageWindow.__init__(self, toplevel, parent, id, size)
      self.toplevel._classifier.database.add_callback(
         'add',
         self.id.append_glyphs)
      self.toplevel._classifier.database.add_callback(
         'remove',
         self.id.remove_glyphs)
      wx.EVT_WINDOW_DESTROY(self, self._OnDestroy)

   def _OnDestroy(self, event):
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
            id = self.toplevel.guess_glyph(image)[0][0][1]
         else:
            id = image.id_name[0][1]
         self.toplevel.set_label_display(id)
         self.toplevel.display_label_at_cell(row, col, id)

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
         id = self.toplevel.guess_glyph(image)[0][0][1]
      else:
         id = image.id_name[0][1]
      self.toplevel.set_label_display(id)
      self.toplevel.display_label_at_cell(row, col, id)

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
                      (g.get((x1 + x - g.ul_x, y1 + y - g.ul_y)) != 0)):
                     matches.add(i)
                     break
      else:
         r = Rect((x1, y1), (x2, y2))
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
      # self.ForceRefresh()

   ########################################
   # DELETION

   def _OnDelete(self, event):
      self.delete_selected()
      self.RefreshSelected()
      # self.toplevel.class_iw.id.ForceRefresh()

   ########################################
   # CALLBACKS

   # Display selected items in the context display
   def _OnSelectImplDisplayCcs(self, images, force=False):
      if images != self._last_selection or force:
         self._last_selection = images
         self.toplevel.display_cc(images)


class PageMultiImageWindow(ExtendedMultiImageWindow):
   pane_name = "Page glyphs"

   def __init__(self, toplevel, parent = None, id = -1, size = wx.DefaultSize):
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
      self.add_callback("rubber", self._OnRubber)
      wx.EVT_WINDOW_DESTROY(self, self._OnDestroy)

   def _OnDestroy(self, event):
      self.remove_callback("rubber", self._OnRubber)

   def _OnRubber(self, y1, x1, y2, x2, shift, ctrl):
      self.toplevel.find_glyphs_in_rect(int(x1), int(y1), int(x2), int(y2), shift or ctrl)

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
      image = dlg.show(self, image_menu.shell.locals,
                       docstring="""Choose an image to display in the context (bottom) pane.""")
      if image != None:
         self.id.set_image(image[0])

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
      for split in plugin.methods_flat_category("Segmentation", ONEBIT):
         self._symbol_table.add("_split." + split[0])
      self._symbol_table.add("_group")
      self._symbol_table.add("_group._part")
      # Add classifier database's symbols to the symbol table
      for glyph in self._classifier.get_glyphs():
         for idx in glyph.id_name:
            self._symbol_table.add(idx[1])

      self.classifier_collection_filename = None
      self.page_collection_filename = None
      self.state_directory = None
      self._save_state_dialog = [1] * 10
      self.auto_move = []
      self.image = None
      self.menu = None
      self.default_segmenter = -1
      self._save_by_criteria_dialog = [1] * 10
      self._group_and_guess_dialog = [0, 4, 4, 16, 0]

      ImageFrameBase.__init__(
         self, parent, id,
         self._classifier.get_name() + " Classifier", owner)
      from gamera.gui import gamera_icons
      icon = wx.IconFromBitmap(gamera_icons.getIconClassifyBitmap())
      self._frame.SetIcon(icon)
      self._frame.CreateStatusBar(len(self.status_bar_description))
      status_bar = self._frame.GetStatusBar()
      self.status_bar_mapping = {}
      for i, (id, default, width) in enumerate(self.status_bar_description):
         status_bar.SetStatusText(default, i)
         self.status_bar_mapping[id] = i
      status_bar.SetStatusWidths([x[2] for x in self.status_bar_description])

      self._frame.SetSize((800, 600))
      if aui:
         self._aui = aui.AuiManager(self._frame)
         nb_style = (aui.AUI_NB_TAB_SPLIT|aui.AUI_NB_TAB_MOVE|
                     aui.AUI_NB_SCROLL_BUTTONS)

         splitterhl_parent = splitterhr0_parent = \
             splitterhr1_parent = splitterv_parent = self.nb = \
             aui.AuiNotebook(self._frame, style=nb_style)
      else:
         self.splitterv = wx.SplitterWindow(
            self._frame, -1,
            style=wx.SP_3DSASH|wx.CLIP_CHILDREN|
            wx.NO_FULL_REPAINT_ON_RESIZE|wx.SP_LIVE_UPDATE)
         splitterv_parent = self.splitterv
         self.splitterhr0 = wx.SplitterWindow(
            self.splitterv, -1,
            style=wx.SP_3DSASH|wx.CLIP_CHILDREN|
            wx.NO_FULL_REPAINT_ON_RESIZE|wx.SP_LIVE_UPDATE)
         splitterhr0_parent = self.splitterhr0
         self.splitterhr1 = wx.SplitterWindow(
            self.splitterhr0, -1,
            style=wx.SP_3DSASH|wx.CLIP_CHILDREN|
            wx.NO_FULL_REPAINT_ON_RESIZE|wx.SP_LIVE_UPDATE)
         splitterhr1_parent = self.splitterhr1
         self.splitterhl = wx.SplitterWindow(
            self.splitterv, -1,
            style=wx.SP_3DSASH|wx.CLIP_CHILDREN|
            wx.NO_FULL_REPAINT_ON_RESIZE|wx.SP_LIVE_UPDATE)
         splitterhl_parent = self.splitterhl

      self.single_iw = ClassifierImageWindow(self, splitterhr0_parent)
      self.multi_iw = PageMultiImageWindow(self, splitterhr1_parent, id=2001)
      self.class_iw = ClassifierMultiImageWindow(self, splitterhr1_parent, id=2000)
      self.class_iw.id.sort_images()
      self.symbol_editor = SymbolTableEditorPanel(
         self._symbol_table, self, splitterhl_parent)
      self.rule_engine_runner = rule_engine_runner.RuleEngineRunnerPanel(
         self, splitterhl_parent)

      if aui:
         self._aui.AddPane(self.nb)
         self.nb.AddPage(self.symbol_editor, "Symbol Name Editor")
         self.nb.AddPage(self.multi_iw, "Page glyphs")
         self.nb.AddPage(self.class_iw, "Classifier glyphs")
         self.nb.AddPage(self.single_iw, "Source image display")
         self.nb.Split(3, wx.BOTTOM)
         self.nb.Split(0, wx.LEFT)
         self.nb.InsertPage(1, self.rule_engine_runner, "Rule Engine Runner")
         for pane in self.nb.GetAuiManager().GetAllPanes():
            pane.MinSize(wx.Size(150, 200))
         self.nb.GetAuiManager().Update()
      else:
         self.splitterhr1.SetMinimumPaneSize(3)
         self.split_editors()
         self.unsplit_editors(self.class_iw)
         self.splitterhr0.SetMinimumPaneSize(3)
         self.splitterhr0.SplitHorizontally(self.splitterhr1, self.single_iw, 300)
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
      self.class_iw.id.set_image(self._classifier.get_glyphs())

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
            ("Save glyphs in classifier with features as...", self._OnSaveClassifierCollectionWithFeaturesAs),
            ("Clear glyphs in classifier", self._OnClearClassifierCollection))),
          ("&Page glyphs",
           (("Open glyphs into page editor...", self._OnOpenPageCollection),
            ("Merge glyphs into page editor...", self._OnMergePageCollection),
            ("Save glyphs in page", self._OnSavePageCollection),
            ("Save glyphs in page as...", self._OnSavePageCollectionAs),
            ("Save glyphs in page with features as...", self._OnSavePageCollectionWithFeaturesAs),
            ),
           ),
          ("Save selected glyphs &as...", self._OnSaveSelectedGlyphs),
          (None, None),
          ("&Symbol names",
           (("&Import...", self._OnImportSymbolTable),
            ("&Export...", self._OnExportSymbolTable)))
         ))
      image_menu = gui_util.build_menu(
         self._frame,
         (("&Open and segment image...", self._OnOpenAndSegmentImage),
          ("Se&lect and segment image...", self._OnSelectAndSegmentImage),
          ("Select already &segmented image...", self._OnSelectSegmentedImage),
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
         ("Group and guess all...", self._OnGroupAndGuessAll),
         ("Group &and guess selected...", self._OnGroupAndGuessSelected),
         (None, None),
         ("Confirm all", self._OnConfirmAll),
         ("&Confirm selected", self._OnConfirmSelected),
         (None, None),
         ("Change set of &features...", self._OnChangeSetOfFeatures)]
      if not aui:
         classifer_menu_spec = (
            classifier_menu_spec[:-1] +
            [("Display/Hide classifier glyphs", self._OnDisplayContents),
             (None, None),
             classifier_menu_spec[-1]]
            )

      if classifier_settings != []:
         classifier_menu_spec.extend([
            (None, None),
            ("Classifier &settings", classifier_settings)])
      if hasattr(self._classifier, 'noninteractive_copy'):
         classifier_menu_spec.extend([
            (None, None),
            ("Create &noninteractive copy...", self._OnCreateNoninteractiveCopy)])
      classifier_menu_spec.extend([
         ("Create &edited classifier...", self._OnCreateEditedClassifier),
         (None, None),
         ("Generate classifier stats...", self._OnGenerateClassifierStats)])
      classifier_menu = gui_util.build_menu(
         self._frame,
         classifier_menu_spec)

      rules_menu_spec = []
      if not aui:
         rules_menu_spec.extend(
            [("Show rule testing panel", self._OnShowRuleTestingPanel),
             (None, None)])
      rules_menu_spec.append(("Open rule module", self._OnOpenRuleModule))
      rules_menu = gui_util.build_menu(
         self._frame,
         rules_menu_spec)

      menubar = wx.MenuBar()
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

   def set_image(self, page_collection, image=None, weak=False):
      self.set_multi_image(page_collection)
      self.set_single_image(image, weak=weak)

   def set_multi_image(self, page_collection):
      wx.BeginBusyCursor()
      try:
         for glyph in page_collection:
            for id in glyph.id_name:
               self._symbol_table.add(id[1])
         self.multi_iw.id.set_image(page_collection)
      finally:
         wx.EndBusyCursor()

   def set_single_image(self, image=None, weak=False):
      if image == None:
         if not aui:
            if self.splitterhr0.IsSplit():
               self.splitterhr0.Unsplit()
               self.single_iw.Hide()
         self.single_iw.id.image = None
         self.single_iw.id.original_image = None
      else:
         self.single_iw.id.set_image(image, weak=False)
         if not aui:
            if not self.splitterhr0.IsSplit():
               self.splitterhr0.SplitHorizontally(
                  self.splitterhr1, self.single_iw, self._frame.GetSize()[1] / 2)
               if self.splitterhr1.IsSplit():
                  self.splitterhr1.SetSashPosition(self._frame.GetSize()[1] / 4)
               self.single_iw.Show()

   if not aui:
      def unsplit_editors(self, display):
         wx.BeginBusyCursor()
         try:
            self.splitterhr1.Unsplit(display)
            display.Show(False)
            for id in (self.multi_iw, self.class_iw):
               id.set_close_button(False)
         finally:
            wx.EndBusyCursor()

      def split_editors(self):
         wx.BeginBusyCursor()
         try:
            self.splitterhr1.SplitHorizontally(
               self.class_iw, self.multi_iw,
               self.splitterhr1.GetSize()[1] / 2)
            self.class_iw.Show(True)
            self.class_iw.id.sort_images()
            self.multi_iw.Show(True)
            for id in (self.multi_iw, self.class_iw):
               id.set_close_button(True)
         finally:
            wx.EndBusyCursor()

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
            return selection, active_id, inactive_id
      return [], self.multi_iw.id, self.class_iw.id

   def display_cc(self, cc):
      if self.single_iw.id.image is not None:
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
            cursorbusy = False
            if wx.IsBusy():
               cursorbusy = True
               wx.EndBusyCursor()
            gui_util.message(str(e))
            if cursorbusy:
               wx.BeginBusyCursor()
            return
         if len(added) or len(removed):
            wx.BeginBusyCursor()
            self.multi_iw.id.BeginBatch()
            try:
               self.multi_iw.id.append_and_remove_glyphs(added, removed)
            finally:
               self.multi_iw.id.EndBatch()
               wx.EndBusyCursor()
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
          Directory('Open directory')], name="Open classifier window")
      results = dialog.show(
         self._frame,
         docstring = """
           This dialog opens a special directory of files containing
           an original image, and contents of the editor and the
           classifier.  This directory should be one created by **Save
           all...**

           *Classifier settings*
             When checked, load the classifier settings.  This is
             things like the number of *k* and distance function.

           *Page glyphs*
             When checked, load the page glyphs from the directory.

           *Classifier glyphs*
             When checked, load the classifier glyphs from the
             directory.

           *Symbol table*
             When checked, load the table of symbol names from the
             directory.

           *Source image*
             When checked, load the source image from the directory
             into the context pane.
           """)
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
                enabled=hasattr(self.single_iw.id, 'image') and
                self.single_iw.id.image is not None),
          Check('', 'With features', self._save_state_dialog[5]),
          Directory('Save directory')], name="Save classifier window")
      results = dialog.show(
         self._frame,
         docstring = """
           This dialog saves all of the data necessary to restore the
           classifier's state into a special directory.  This includes
           the original image and the contents of the editor and the
           classifier.  This special directory can be reloaded using
           **Open all...**.

           *Classifier settings*
             When checked, save the classifier settings.  This is
             things like the number of *k* and distance function.

           *Page glyphs*
             When checked, save the page glyphs.

           *Classifier glyphs*
             When checked, save the classifier glyphs.

           *Symbol table*
             When checked, save the table of symbol names.

           *Source image*
             When checked, save the source image.
           """)
      if results == None:
         return
      self._save_state_dialog = results
      settings, page, classifier, symbols, source, with_features, directory = results
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
               os.path.join(directory, "page_glyphs.xml"), with_features=with_features)
         except:
            error_messages.add(str(e))
         self.multi_iw.id.is_dirty = False
      if classifier:
         try:
            self._SaveClassifierCollection(
               os.path.join(directory, "classifier_glyphs.xml"), with_features=with_features)
         except:
            error_messages.add(str(e))
         self._classifier.is_dirty = False
      if symbols:
         try:
            self._ExportSymbolTable(os.path.join(directory, "symbol_table.xml"))
         except:
            error_messages.add(str(e))
      if source and self.single_iw.id is not None:
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
          Check('', 'Manually classified', self._save_by_criteria_dialog[7]),
          Info('Options'),
          Check('', 'Save with features', self._save_by_criteria_dialog[9]),
          FileSave('Save glyphs to file:', '',
                   extension=gamera_xml.extensions)],
         name = 'Save by criteria...')
      verified = False
      while not verified:
         results = dialog.show(
            self._frame,
            docstring = """
              Choose a set of glyphs to save to an XML file.

              *Classifier glyphs*
                Include glyphs from the classifier glyphs database.

              *Page glyphs*
                Include glyphs from the current page.

              *Unclassified*
                Include glyphs that have not yet be classified.

              *Automatically classified*
                Include glyphs that have been automatically classified.

              *Heuristically classified*
                Include glyphs that have been heuristically classified.

              *Manually classified*
                Include glyphs that have been manually classified.

              *Save with features*
                Include the actual computed features of the glyphs in the
                XML file (instead of only the image data.)

              *Save glyphs to file*
                Choose an XML to save the glyphs to.
            """)
         if results is None:
            return
         skip, classifier, page, skip, un, auto, heur, man, skip2, with_features, filename = results
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
               symbol_table=self._symbol_table,
               with_features=with_features).write_filename(filename)
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
            self._SaveClassifierCollection(
               self.classifier_collection_filename, with_features=False)

   def _OnSaveClassifierCollectionAs(self, event):
      self._SaveClassifierCollectionAs(event, False)

   def _OnSaveClassifierCollectionWithFeaturesAs(self, event):
      self._SaveClassifierCollectionAs(event, True)

   def _SaveClassifierCollectionAs(self, event, with_features=False):
      if gui_util.are_you_sure_dialog(
         ("There are %d glyphs in the classifier.\n" +
          "Are you sure you want to save?") % len(self._classifier.database)):
         filename = gui_util.save_file_dialog(self._frame, gamera_xml.extensions)
         if filename:
            self.classifier_collection_filename = filename
            self._SaveClassifierCollection(filename, with_features=with_features)

   def _SaveClassifierCollection(self, filename, with_features=True):
      try:
         self._classifier.to_xml_filename(filename, with_features=with_features)
      except gamera_xml.XMLError, e:
         gui_util.message("Saving classifier glyphs: " + str(e))

   def _OnClearClassifierCollection(self, event):
      if self._classifier.is_dirty:
         if gui_util.are_you_sure_dialog(
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
            self._SavePageCollection(
               self.page_collection_filename, glyphs, with_features=False)

   def _OnSavePageCollectionAs(self, event):
      self._SavePageCollectionAs(event, with_features=False)

   def _OnSavePageCollectionWithFeaturesAs(self, event):
      self._SavePageCollectionAs(event, with_features=True)

   def _SavePageCollectionAs(self, event, with_features=False):
      glyphs = self.multi_iw.id.GetAllItems()
      if gui_util.are_you_sure_dialog(
         ("There are %d glyphs in the page glyphs pane.\n" +
          "Are you sure you want to save?") % len(glyphs)):
         filename = gui_util.save_file_dialog(self._frame, gamera_xml.extensions)
         if filename:
            self.page_collection_filename = filename
            self._SavePageCollection(filename, glyphs, with_features=with_features)

   def _SavePageCollection(self, filename, glyphs=None, with_features=True):
      if glyphs is None:
         glyphs = self.multi_iw.id.GetAllItems()
      if with_features:
         self._classifier.generate_features_on_glyphs(glyphs)
      try:
         gamera_xml.WriteXMLFile(
            glyphs=glyphs,
            symbol_table=self._symbol_table,
            with_features=with_features).write_filename(filename)
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
            self._classifier.generate_features_on_glyphs(glyphs)
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
      wx.BeginBusyCursor()
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
         wx.EndBusyCursor()

   def _OnExportSymbolTable(self, event):
      filename = gui_util.save_file_dialog(self._frame, gamera_xml.extensions)
      if filename:
         self._ExportSymbolTable(filename)

   def _ExportSymbolTable(self, filename):
      wx.BeginBusyCursor()
      try:
         try:
            gamera_xml.WriteXMLFile(
               symbol_table=self._symbol_table).write_filename(filename)
         except gamera_xml.XMLError, e:
            gui_util.message("Exporting symbol table: " + str(e))
      finally:
         wx.EndBusyCursor()

   ########################################
   # IMAGE MENU

   def _OnOpenAndSegmentImage(self, event):
      if self.multi_iw.id.is_dirty:
         if not gui_util.are_you_sure_dialog("Page glyphs have not been saved.  Are you sure you wish to proceed?"):
            return
      segmenters = [x[0] for x in
                    plugin.methods_flat_category("Segmentation", ONEBIT)]
      if self.default_segmenter == -1:
         self.default_segmenter = segmenters.index("cc_analysis")
      dialog = Args(
         [FileOpen("Image file", "", util.get_file_extensions("load")),
          Choice("Segmentation algorithm", segmenters, self.default_segmenter)],
         name="Open and segment image...")
      filename = None
      while filename is None:
         results = dialog.show(
            self._frame,
            docstring="""Choose a file to open and a segmentation method.""")
         if results is None:
            return
         filename, segmenter = results
         self.default_segmenter = segmenter
         if filename == None:
            gui_util.message("You must provide a filename to load.")

      wx.BeginBusyCursor()
      try:
         image = load_image(filename)
         self._segment_image(image, segmenters[segmenter])
         self.multi_iw.id.is_dirty = False
      except Exception, e:
         wx.EndBusyCursor()
         gui_util.message(str(e))
         return
      wx.EndBusyCursor()

   def _OnSelectAndSegmentImage(self, event):
      if self.multi_iw.id.is_dirty:
         if not gui_util.are_you_sure_dialog("Page glyphs have not been saved.  Are you sure you wish to proceed?"):
            return
      segmenters = [x[0] for x in
                    plugin.methods_flat_category("Segmentation", ONEBIT)]
      if self.default_segmenter == -1:
         self.default_segmenter = segmenters.index("cc_analysis")
      dialog = Args(
         [Class("Image", ImageBase),
          Choice("Segmentation algorithm", segmenters, self.default_segmenter)],
         name="Select and segment image...")
      results = dialog.show(
         self._frame, image_menu.shell.locals,
         docstring="""Choose an already opened image to use, and a segmentation method.""")
      if results is None:
         return
      image, segmenter = results
      wx.BeginBusyCursor()
      try:
         self._segment_image(image, segmenters[segmenter])
         self.multi_iw.id.is_dirty = False
      except Exception, e:
         wx.EndBusyCursor()
         gui_util.message(str(e))
         return
      wx.EndBusyCursor()

   def _OnSelectSegmentedImage(self, event):
      if self.multi_iw.id.is_dirty:
         if not gui_util.are_you_sure_dialog("Page glyphs have not been saved.  Are you sure you wish to proceed?"):
            return
      dialog = Args(
         [Class("Image", ImageBase),
          Class("Ccs", ImageBase, list_of=True)],
         name="Select image and ccs...")
      results = dialog.show(
         self._frame, image_menu.shell.locals,
         docstring = "Select an image and its segmented glyphs")
      if results is None:
         return
      (image, ccs) = results
      self.set_image(ccs, image, weak=False)

   def _OnSelectImage(self, event):
      dialog = Args(
         [Class("Image", ImageBase)],
         name="Select image...")
      results = dialog.show(
         self._frame, image_menu.shell.locals,
         docstring = "Select an image to display in the context pane")
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
      list = self.multi_iw.id.GetAllItems()
      if len(list) == 0:
         gui_util.message("No glyphs in page.")
         return
      self._OnSaveAsImages(list)

   def _OnSaveSelectedAsImages(self, event):
      list = self.get_all_selected()[0]
      if len(list) == 0:
         gui_util.message("No glyphs selected.")
         return
      self._OnSaveAsImages(list)

   def _OnSaveClassifierCollectionAsImages(self, event):
      list = self._classifier.get_glyphs()
      if len(list) == 0:
         gui_util.message("No glyphs in classifier.")
         return
      self._OnSaveAsImages(list)

   def _OnSaveAsImages(self, list):
      dirname = gui_util.directory_dialog(self._frame)
      if dirname:
         glyphs = [g for g in list]
         glyphs.sort(lambda g1,g2: cmp(g1.get_main_id().lower(),g2.get_main_id().lower()))
         lastid = ""
         nr = 0
         for glyph in glyphs:
            thisid = glyph.get_main_id().lower()
            if thisid != lastid:
               nr = 0
               lastid = thisid
            nr += 1
            glyph.save_PNG(os.path.join(dirname, "%s-%03d.png" % (thisid,nr)))

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
      else:
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
      dialog = Args(
         [Choice('Grouping function', ['Bounding Box', 'Shaped'], self._group_and_guess_dialog[0]),
          Int('Distance threshold', range=(0, 100), default=self._group_and_guess_dialog[1]),
          Int('Maximum number of parts per group', default=self._group_and_guess_dialog[2]),
          Int('Maximum solveable subgraph size', default=self._group_and_guess_dialog[3]),
          Choice('Grouping criterion', ['min', 'avg'], self._group_and_guess_dialog[4])],
         name = 'Group and guess...')
      results = dialog.show(self._frame)
      if results is None:
         return
      function, threshold, max_parts_per_group, max_graph_size, criterion = results
      self._group_and_guess_dialog = results

      if function == 0:
         func = BoundingBoxGroupingFunction(threshold)
      elif function == 1:
         func = ShapedGroupingFunction(threshold)

      try:
         try:
            wx.BeginBusyCursor()
            added, removed = self._classifier.group_list_automatic(
               list,
               grouping_function=func,
               max_parts_per_group=max_parts_per_group,
               max_graph_size=max_graph_size)
         finally:
            wx.EndBusyCursor()
      except ClassifierError, e:
         gui_util.message(str(e))
      else:
         self._AdjustAfterGuess(added, removed)

   def _AdjustAfterGuess(self, added, removed):
      try:
         wx.BeginBusyCursor()
         try:
            self.multi_iw.id.BeginBatch()
            if len(added) or len(removed):
               self.multi_iw.id.append_and_remove_glyphs(added, removed)
            else:
               self.multi_iw.id.RefreshSelected()
            self.multi_iw.id.sort_images()
         finally:
            self.multi_iw.id.ForceRefresh()
            self.multi_iw.id.EndBatch()
      finally:
         wx.EndBusyCursor()

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
      wx.BeginBusyCursor()
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
         wx.EndBusyCursor()
      self.class_iw.id.resize_grid()

   def _OnChangeSetOfFeatures(self, event):
      all_features = [x[0] for x in
                      plugin.methods_flat_category("Features", ONEBIT)]
      all_features.sort()
      existing_features = [x[0] for x in
                           ImageBase.get_feature_functions(self._classifier.features)[0]]
      feature_controls = []
      for x in all_features:
         feature_controls.append(
            Check('', x, default=(x in existing_features)))
      dialog = Args(
         feature_controls,
         name='Feature selection',
         title='Select the features you want to use for classification')
      result = dialog.show(
         self._frame)
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

   def _OnCreateEditedClassifier(self, event):
      from gamera.gui.knn_editing_display import EditingDialog
      editedClassifier = EditingDialog().show(self._classifier)

      if not editedClassifier is None:
         from gamera.gui import var_name
         classifierName = var_name.get("classifier", image_menu.shell.locals)
         image_menu.shell.locals[classifierName] = editedClassifier
         image_menu.shell.update()

   if not aui:
      def _OnDisplayContents(self, event):
         if self.splitterhr1.IsSplit():
            self.unsplit_editors(self.class_iw)
         else:
            self.split_editors()

   def _OnGenerateClassifierStats(self, event):
      from gamera import classifier_stats
      all_stats = [(x.title, x) for x in classifier_stats.all_stat_pages]
      all_stats.sort()
      stats_controls = [Check('', x[0], default=True) for x in all_stats]
      stats_controls.append(Directory('Stats directory'))
      dialog = Args(
         stats_controls,
         name='Select statistics',
         title='Select the statistics to generate and the directory to save them to.\n(Experimental feature).')
      result = dialog.show(self._frame)
      if result is None or result[-1] is None:
         return
      pages = []
      for on, (name, page) in zip(result, all_stats):
         if on:
            pages.append(page)
      wx.BeginBusyCursor()
      try:
         classifier_stats.make_stat_pages(self._classifier, result[-1], pages)
      except Exception, e:
         wx.EndBusyCursor()
         gui_util.message(e)
      else:
         wx.EndBusyCursor()

   ########################################
   # RULES MENU

   if not aui:
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

      if not aui and not self.splitterhl.IsSplit():
         self.splitterhl.SplitHorizontally(
            self.symbol_editor, self.rule_engine_runner,
            self._frame.GetSize()[1] / 2)
         self.rule_engine_runner.Show()

   ########################################
   # CALLBACKS

   def _OnCloseWindow(self, event):
      if self.is_dirty:
         if not gui_util.are_you_sure_dialog(
            "Are you sure you want to quit without saving?"):
            event.Veto()
            return
      self._classifier.database.remove_callback(
         'length_change',
         self.classifier_collection_length_change_callback)
      self.multi_iw.id.glyphs.remove_callback(
         'length_change',
         self.page_collection_length_change_callback)
      self._classifier.set_display(None)
      self._frame.Destroy()
      self.multi_iw.Destroy()
      self.single_iw.Destroy()
      if not aui:
         self.splitterhr1.Destroy()
         self.splitterhr0.Destroy()
         self.splitterhl.Destroy()
         self.splitterv.Destroy()
      del self._frame

   def refresh(self):
      self.single_iw.id.refresh(1)


##############################################################################
# SYMBOL TABLE EDITOR
##############################################################################

class SymbolTreeCtrl(wx.TreeCtrl):
   def __init__(self, toplevel, parent, id, pos, size, style):
      self.toplevel = toplevel
      self.editing = 0
      wx.TreeCtrl.__init__(self, parent, id, pos, size,
                          style=style|wx.NO_FULL_REPAINT_ON_RESIZE)
      self.root = self.AddRoot("Symbols")
      self.SetItemHasChildren(self.root, True)
      self.SetPyData(self.root, "")
      wx.EVT_KEY_DOWN(self, self._OnKey)
      wx.EVT_TREE_SEL_CHANGED(self, id, self._OnChanged)
      wx.EVT_TREE_ITEM_ACTIVATED(self, id, self._OnActivated)
      self.toplevel._symbol_table.add_callback(
         'add', self.symbol_table_add_callback)
      self.toplevel._symbol_table.add_callback(
         'remove', self.symbol_table_remove_callback)
      self.Expand(self.root)
      self.SelectItem(self.root)
      wx.EVT_WINDOW_DESTROY(self, self._OnDestroy)

   def _OnDestroy(self, event):
      self.toplevel._symbol_table.remove_callback(
         'add', self.symbol_table_add_callback)
      self.toplevel._symbol_table.remove_callback(
         'remove', self.symbol_table_remove_callback)

   # This is a stub to provide compatibility with wx2.4 and wx2.5
   if wx.VERSION >= (2, 5):
      def GetFirstChild(self, root, cookie):
         return wx.TreeCtrl.GetFirstChild(self, root)

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
      # self.toplevel._OnText(None)
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
      keycode = evt.GetKeyCode()

      if keycode == wx.WXK_DELETE:
         self.toplevel._symbol_table.remove(
            self.GetPyData(self.GetSelection()))
      else:
         evt.Skip()

   def _OnActivated(self, event):
      symbol = self.GetPyData(event.GetItem())
      if symbol != None:
         self.toplevel.toplevel.classify_manual(symbol)

   def _OnChanged(self, event):
      item = self.GetSelection()
      try:
         data = self.GetPyData(item)
      except:
         event.Skip()
         return
      text = self.toplevel.text.GetValue()
      if data != None and text != data and text and text[-1] != ".":
         self.toplevel.text.SetValue(data)
         self.toplevel.text.SetInsertionPointEnd()
      event.Skip()

class SymbolTableEditorPanel(wx.Panel):
   def __init__(self, symbol_table, toplevel, parent = None, id = -1):
      wx.Panel.__init__(
         self, parent, id,
         style=wx.WANTS_CHARS|wx.CLIP_CHILDREN|wx.NO_FULL_REPAINT_ON_RESIZE)
      self.toplevel = toplevel
      self._symbol_table = symbol_table
      self.SetAutoLayout(True)
      self.box = wx.BoxSizer(wx.VERTICAL)
      txID = wx.NewId()
      self.text = wx.TextCtrl(self, txID,
                             style=wx.TE_PROCESS_ENTER|wx.TE_PROCESS_TAB)
      self.box.Add(self.text, 0, wx.EXPAND|wx.BOTTOM, 5)
      tID = wx.NewId()
      self.tree = SymbolTreeCtrl(self, self, tID, wx.DefaultPosition,
                                 wx.DefaultSize,
                                 wx.TR_HAS_BUTTONS | wx.TR_DEFAULT_STYLE)
      self.box.Add(self.tree, 1, wx.EXPAND|wx.ALL)
      self.SetSizer(self.box)

      wx.EVT_KEY_DOWN(self.text, self._OnKey)
      wx.EVT_TEXT(self, txID, self._OnText)
      # On win32, the enter key is only caught by the EVT_TEXT_ENTER
      # On GTK, the enter key is sent directly to EVT_KEY_DOWN
      if wx.Platform == '__WXMSW__':
         wx.EVT_TEXT_ENTER(self, txID, self._OnEnter)

   ########################################
   # CALLBACKS

   def _OnEnter(self, evt):
      wx.BeginBusyCursor()
      try:
         find = self.text.GetValue()
         normalized_symbol = self._symbol_table.add(find)
         if normalized_symbol != '':
            self.toplevel.classify_manual(normalized_symbol)
      finally:
         wx.EndBusyCursor()

   def _OnKey(self, evt):
      keycode = evt.GetKeyCode()
      ctrldown = evt.ControlDown()

      find = self.text.GetValue()
      if keycode == wx.WXK_TAB:
         find = self._symbol_table.autocomplete(find)
         self.text.SetValue(find)
         self.text.SetInsertionPointEnd()
      elif keycode == wx.WXK_RETURN:
         self._OnEnter(evt)
      elif keycode == wx.WXK_F12:
         self.toplevel.do_auto_move()
      elif keycode == wx.WXK_F11:
         self.toplevel.move_back()
      # This behavior works automatically in GTK+
      elif wx.Platform == '__WXMSW__' and ctrldown:
         if keycode == wx.WXK_LEFT:
            current = self.text.GetInsertionPoint()
            if current != 0:
               new = self.text.GetValue().rfind(".", 0, current - 1)
               self.text.SetInsertionPoint(new + 1)
            return
         elif keycode == wx.WXK_BACK:
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
         elif keycode == wx.WXK_RIGHT:
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
