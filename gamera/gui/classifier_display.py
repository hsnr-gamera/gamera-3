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

from wxPython.wx import *
from gamera.core import *
from gamera.args import *
from gamera.gui import image_menu, var_name, toolbar
from gamera.gui.gamera_display import *
from gamera.symbol_table import SymbolTable
from gamera import gamera_xml, classify

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
      for i in range(image_no + 1, len(self.list)):
         image = self.list[i]
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

   def find_glyphs_in_rect(self, x1, y1, x2, y2):
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
         for index in matches:
            row = index / GRID_NCOLS
            col = index % GRID_NCOLS
            if index is matches[-1]:
               self.updating = 0
            self.SelectBlock(row, col, row, col, first)
            if first == 0:
               self.MakeCellVisible(row, col)
               first = 1
      else:
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
         if image.id_name != prev_id and column != 0:
            for i in range(GRID_NCOLS - column):
               new_list.append(None)
            column = 0
         new_list.append(image)
         column += 1
         if column >= GRID_NCOLS:
            colomn = 0
         prev_id = image.id_name
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
      if function:
         # Turn off the row labels
         self.display_row_labels = 0
      else:
         self.display_row_labels = 1
      wxBeginBusyCursor()
      self.BeginBatch()
      orig_len = len(self.list)
      # Remove "None"s and dead glyphs from the list
      new_list = [x for x in self.list if x != None and not hasattr(x, 'dead')]
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
               label = self.get_label(image.id_name)
               label = self.reduce_label_length(
                  dc, GRID_MAX_LABEL_LENGTH * 0.6, label)
               max_label = max(dc.GetTextExtent(label)[0], max_label)
               self.SetRowLabelValue(i, label)
            else:
               max_label = max(dc.GetTextExtent("<>")[0], max_label)
               self.SetRowLabelValue(i, "<>")
      self.EndBatch()
      return max_label

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
         
   def delete_selected(self):
      for item in self.GetSelectedItems():
         if hasattr(item, 'dead'):
            del item.__dict__['dead']
         else:
            item.dead = 1
      self.RefreshSelected()

class ClassifierMultiImageWindow(MultiImageWindow):
   def __init__(self, toplevel, parent = None, id = -1, size = wxDefaultSize):
      self.toplevel = toplevel
      MultiImageWindow.__init__(self, parent, id, size)
      from gamera.gui import gamera_icons
      self.toolbar.AddSeparator()
      self.toolbar.AddSimpleTool(
         300, gamera_icons.getIconDeleteBitmap(),
         "Delete selected glyphs",
         self.OnDelete)
      self.toolbar.AddSeparator()
      self.auto_move_buttons = []
      self.auto_move_buttons.append(
         self.toolbar.AddSimpleTool(
         200, gamera_icons.getIconNextUnclassBitmap(),
         "Move to next unclassified glyph after each classification.",
         self.OnAutoMove, 1)
         )
      self.auto_move_buttons.append(
         self.toolbar.AddSimpleTool(
         201, gamera_icons.getIconNextAutoclassBitmap(),
         "Move to next auto-classified glyph after each classification.",
         self.OnAutoMove, 1)
         )
      self.auto_move_buttons.append(
      self.toolbar.AddSimpleTool(
         202, gamera_icons.getIconNextHeurclassBitmap(),
         "Move to next heuristically classified glyph after each classification.",
         self.OnAutoMove, 1)
      )
      self.auto_move_buttons.append(
      self.toolbar.AddSimpleTool(
         203, gamera_icons.getIconNextManclassBitmap(),
         "Move to next manually classified glyph after each classification.",
         self.OnAutoMove, 1)
      )
      
   def get_display(self):
      return ClassifierMultiImageDisplay(self.toplevel, self)

   def OnAutoMove(self, event):
      id = event.GetId() - 200
      if event.GetIsDown():
         self.toplevel.auto_move.append(id)
      else:
         self.toplevel.auto_move.remove(id)

   def OnDelete(self, event):
      self.toplevel.delete_selected()

   def find_glyphs_in_rect(self, x1, y1, x2, y2):
      self.id.find_glyphs_in_rect(x1, y1, x2, y2)

class ClassifierImageDisplay(ImageDisplay):
   def __init__(self, toplevel, parent):
      self.toplevel = toplevel
      ImageDisplay.__init__(self, parent)

   def OnRubber(self):
      self.toplevel.find_glyphs_in_rect(
         self.rubber_origin_x, self.rubber_origin_y,
         self.rubber_x2, self.rubber_y2)

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
      from gamera.args import Args, Class
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
         raise ValueError("classifier must be instance of type InteractiveClassifier.")
      self.classifier = classifier
      self.production_database_filename = None
      self.current_database_filename = None
      if not isinstance(symbol_table, SymbolTable):
         self.symbol_table = SymbolTable()
         for symbol in symbol_table:
            self.symbol_table.add(symbol)
      else:
         self.symbol_table = symbol_table
      for glyph in classifier.database.iterkeys():
         for id in glyph.id_name:
            self.symbol_table.add(id[1])
      for action in ImageBase.methods_flat_category("Action", ONEBIT):
         self.symbol_table.add("action." + action[0])
      self.auto_move = []
      toplevel = self
      self.image = None
      self.menu = None
      ImageFrameBase.__init__(self, parent, id, title, owner)
      self._frame.SetSize((800, 600))
      self.splitterv = wxSplitterWindow(self._frame, -1)
      self.splitterh = wxSplitterWindow(self.splitterv, -1)
      self.single_iw = ClassifierImageWindow(self, self.splitterh)
      self.multi_iw = ClassifierMultiImageWindow(self, self.splitterh)
      self.splitterh.SetMinimumPaneSize(5)
      self.splitterh.SplitHorizontally(self.multi_iw, self.single_iw, 300)
      self.symbol_editor = SymbolTableEditorPanel(self.symbol_table,
                                                  self, self.splitterv)
      self.splitterv.SetMinimumPaneSize(5)
      self.splitterv.SplitVertically(self.symbol_editor, self.splitterh, 160)
      from gamera.gui import gamera_icons
      icon = wxIconFromBitmap(gamera_icons.getIconClassifyBitmap())
      self._frame.SetIcon(icon)
      self._frame.CreateStatusBar(3)

   def set_image(self, current_database, image=None):
      self.current_database = {}
      for glyph in current_database:
         self.current_database[glyph] = None
         for id in glyph.id_name:
            self.symbol_table.add(id[1])
      self.multi_iw.id.set_image(current_database)
      if image == None:
         image = self.image
      self.image = image
      if self.image == None:
         self.splitterh.SetSashPosition(10000)
         self.image = Image(0, 0, 300, 200, ONEBIT, DENSE)
      self.single_iw.id.set_image(self.image)

   def append_glyphs(self, list):
      self.multi_iw.id.append_glyphs(list)

   def delete_selected(self):
      self.multi_iw.id.delete_selected()

   ########################################
   # DISPLAY

   def display_cc(self, cc):
      if self.image != None:
         self.single_iw.id.highlight_cc(cc)
         self.single_iw.id.focus(cc)

   def find_glyphs_in_rect(self, x1, y1, x2, y2):
      self.multi_iw.find_glyphs_in_rect(x1, y1, x2, y2)

   ########################################
   # CLASSIFICATION FUNCTIONS

   def guess_all(self):
      wxBeginBusyCursor()
      self.classifier.classify_list_automatic(self.current_database.keys())
      self.multi_iw.id.sort_images(None)
      wxEndBusyCursor()

   def guess_glyph(self, glyph):
      return self.classifier.guess_glyph(glyph)

   def confirm_all(self):
      for x in self.classifier.current_database:
         if x.classification_state == AUTOMATIC:
            self.classifier.classify_glyph_manual(x, x.get_main_id())
      self.multi_iw.id.ForceRefresh()
   
   def classify_manual(self, id):
      wxBeginBusyCursor()
      selection = self.multi_iw.id.GetSelectedItems(
         self.multi_iw.id.GetGridCursorRow(),
         self.multi_iw.id.GetGridCursorCol())
      if selection != []:
         self.multi_iw.id.BeginBatch()
         removed_some = 0
         for glyph in selection:
            for child in glyph.children_images:
               if self.current_database.has_key(child):
                  child.dead = 1
                  del self.current_database[child]
                  removed_some = 1
         splits = self.classifier.classify_list_manual(selection, id)
         if len(splits):
            for split in splits:
               self.current_database[split] = None
            self.append_glyphs(splits)
##          if removed_some or len(splits):
##             self.multi_iw.id.Refresh()
         if not self.do_auto_move():
            self.set_label_display([(1.0, id)])
         self.multi_iw.id.EndBatch()
         wxEndBusyCursor()

   ########################################
   # AUTO-MOVE

   def do_auto_move(self):
      self.multi_iw.id.do_auto_move(self.auto_move)
      if self.auto_move != []:
         return 1
      else:
         return 0

   def set_label_display(self, ids):
      if ids != []:
         self.symbol_editor.tree.set_label_display(ids[0][1])
      else:
         self.symbol_editor.tree.set_label_display("")

   def display_label_at_cell(self, row, col, label):
      self.multi_iw.id.display_label_at_cell(row, col, label)

   ########################################
   # FILE MENU

   def file_menu(self):
      self.menu = wxMenu()
      self.menu.Append(10, "Smart saving...")
      EVT_MENU(self._frame, 10, self.OnSmartSave)
      self.menu.AppendSeparator()
      self.menu.Append(11, "Open production database...")
      EVT_MENU(self._frame, 11, self.OnOpenProductionDatabase)
      self.menu.Append(12, "Merge into production database...")
      EVT_MENU(self._frame, 12, self.OnMergeProductionDatabase)
      self.menu.Append(13, "Save production database")
      EVT_MENU(self._frame, 13, self.OnSaveProductionDatabase)
      self.menu.Append(14, "Save production database as...")
      EVT_MENU(self._frame, 14, self.OnSaveProductionDatabaseAs)
      self.menu.Append(140, "Clear production database")
      EVT_MENU(self._frame, 140, self.OnClearProductionDatabase)
      self.menu.AppendSeparator()
      self.menu.Append(15, "Open current database...")
      EVT_MENU(self._frame, 15, self.OnOpenCurrentDatabase)
      self.menu.Append(16, "Merge into current database...")
      EVT_MENU(self._frame, 16, self.OnMergeCurrentDatabase)
      self.menu.Append(17, "Save current database")
      EVT_MENU(self._frame, 17, self.OnSaveCurrentDatabase)
      self.menu.Append(18, "Save current database as...")
      EVT_MENU(self._frame, 18, self.OnSaveCurrentDatabaseAs)
      self.menu.AppendSeparator()
      self.menu.Append(19, "Import symbol names...")
      EVT_MENU(self._frame, 19, self.OnImportSymbolTable)
      self.menu.Append(20, "Export symbol names...")
      EVT_MENU(self._frame, 20, self.OnExportSymbolTable)
      self._frame.PopupMenu(self.menu, wxPoint(5, 22))

   def file_menu_destroy(self):
      if self.menu != None:
         self.menu.Destroy()
         self.menu = None

   ########################################
   # CALLBACKS

   def _OnCloseWindow(self, event):
      if self.classifier.is_dirty:
         message = wxMessageDialog(
            self._frame,
            "Are you sure you want to quit without saving?",
            "Classifier",
            wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION )
         result = message.ShowModal()
         if result != wxID_YES:
            return
      self.single_iw.Destroy()
      self.multi_iw.Destroy()
      if self.owner:
         self.owner.set_display(None)
      self._frame.Destroy()

   def OnSmartSave(self, event):
      print "To be implemented."

   def OnOpenProductionDatabase(self, event):
      dlg = wxFileDialog(None, "Choose a file", ".", "", "*.*", wxOPEN)
      if dlg.ShowModal() == wxID_OK:
         filename = dlg.GetPath()
         dlg.Destroy()
         wxBeginBusyCursor()
         self.production_database_filename = filename
         self.classifier.from_xml_filename(filename)
         wxEndBusyCursor()

   def OnMergeProductionDatabase(self, event):
      dlg = wxFileDialog(None, "Choose a file", ".", "", "*.*", wxOPEN)
      if dlg.ShowModal() == wxID_OK:
         filename = dlg.GetPath()
         dlg.Destroy()
         wxBeginBusyCursor()
         self.classifier.merge_from_xml_filename(filename)
         wxEndBusyCursor()

   def OnSaveProductionDatabase(self, event):
      if self.production_database_filename == None:
         self.OnSaveProductionDatabaseAs(event)
      else:
         wxBeginBusyCursor()
         self.classifier.to_xml_filename(
            self.production_database_filename)
         wxEndBusyCursor()

   def OnSaveProductionDatabaseAs(self, event):
      dlg = wxFileDialog(None, "Choose a file", ".", "", "*.*", wxSAVE)
      if dlg.ShowModal() == wxID_OK:
         self.production_database_filename = dlg.GetPath()
         dlg.Destroy()
         self.OnSaveProductionDatabase(event)

   def OnClearProductionDatabase(self, event):
      # TODO: An are you sure dialog
      self.classifier.database = []

   def OnOpenCurrentDatabase(self, event):
      dlg = wxFileDialog(None, "Choose a file", ".", "", "*.*", wxOPEN)
      if dlg.ShowModal() == wxID_OK:
         filename = dlg.GetPath()
         dlg.Destroy()
         wxBeginBusyCursor()
         glyphs = gamera_xml.LoadXMLGlyphs().parse_filename(filename)
         self.set_image(glyphs)
         wxEndBusyCursor()

   def OnMergeCurrentDatabase(self, event):
      dlg = wxFileDialog(None, "Choose a file", ".", "", "*.*", wxOPEN)
      if dlg.ShowModal() == wxID_OK:
         filename = dlg.GetPath()
         dlg.Destroy()
         wxBeginBusyCursor()
         glyphs = gamera_xml.LoadXMLGlyphs().parse_filename(filename)
         for glyph in glyphs:
            self.current_database[glyph] = None
         self.append_glyphs(glyphs)
         wxEndBusyCursor()

   def OnSaveCurrentDatabase(self, event):
      if self.current_database_filename == None:
         self.OnSaveCurrentDatabaseAs(event)
      else:
         wxBeginBusyCursor()
         self.current_database = {}
         for i in self.multi_iw.id.list:
            if not i is None and not hasattr(i, 'dead'):
               self.current_database[i] = i
         gamera_xml.WriteXMLFile(
            glyphs=self.current_database.keys(),
            symbol_table=self.symbol_table).write_filename(
            self.current_database_filename)
         wxEndBusyCursor()

   def OnSaveCurrentDatabaseAs(self, event):
      dlg = wxFileDialog(None, "Choose a file", ".", "", "*.*", wxSAVE)
      if dlg.ShowModal() == wxID_OK:
         self.current_database_filename = dlg.GetPath()
         dlg.Destroy()
         self.OnSaveCurrentDatabase(event)

   def OnImportSymbolTable(self, event):
      dlg = wxFileDialog(None, "Choose a file", ".", "", "*.*", wxOPEN)
      if dlg.ShowModal() == wxID_OK:
         filename = dlg.GetPath()
         dlg.Destroy()
         wxBeginBusyCursor()
         symbol_table = gamera_xml.LoadXMLSymbolTable().parse_filename(filename)
         for symbol in symbol_table.symbols.keys():
            self.symbol_table.add(symbol)
         wxEndBusyCursor()

   def OnExportSymbolTable(self, event):
      dlg = wxFileDialog(None, "Choose a file", ".", "", "*.*", wxSAVE)
      if dlg.ShowModal() == wxID_OK:
         filename = dlg.GetPath()
         dlg.Destroy()
         wxBeginBusyCursor()
         gamera_xml.WriteXMLFile(symbol_table=self.symbol_table).write_filename(filename)
         wxEndBusyCursor()

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
      EVT_TREE_BEGIN_LABEL_EDIT(self, id, self.OnBeginEdit)
      EVT_TREE_END_LABEL_EDIT(self, id, self.OnEndEdit)
      EVT_KEY_DOWN(self, self.OnKey)
      EVT_LEFT_DOWN(self, self.OnLeftDown)
      EVT_RIGHT_DOWN(self, self.OnRightDown)
      EVT_RIGHT_UP(self, self.OnRightUp)
      EVT_LEFT_DCLICK(self, self.OnLeftDoubleClick)
      self.Expand(self.root)
      self.toplevel.symbol_table.add_listener(self)

   def __del__(self):
      self.symbol_table.remove_listener(self)

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
         if self.GetPyData(item) == symbol:
            if self.IsExpanded(item):
               self.CollapseAndReset(item)
               self.Expand(item)
               break

   def OnBeginEdit(self, event):
      if not self.editing:
         event.Veto()

   def OnEndEdit(self, event):
      previous = self.GetPyData(event.GetItem())
      previous_stub = '.'.join(previous.split('.')[:-1])
      if event.GetLabel() == '':
         self.SetItemText(event.GetItem(), self.last_value)
         self.editing = 0
         return
      else:
         label = event.GetLabel()
      if previous_stub == "":
         new = label
      else:
         new = previous_stub + "." + label
      self.SetPyData(event.GetItem(), new)
      self.toplevel.symbol_table.rename(previous, new)
      self.editing = 0

   def OnItemExpanded(self, event):
      parent = event.GetItem()
      parent_path = self.GetPyData(parent)
      items = self.toplevel.symbol_table.get_category_contents_list(parent_path)
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
         self.toplevel.symbol_table.remove(self.GetPyData(self.GetSelection()))
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

   def OnRightDown(self, event):
      self.editing = 1
      pt = event.GetPosition()
      item, flags = self.HitTest(pt)
      if (flags == 4):
         return
      self.last_value = self.GetItemText(item)
      print self.last_value
      self.EditLabel(item)

   def OnRightUp(self, event):
      self.editing = 0

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
      self.symbol_table = symbol_table
      self.SetAutoLayout(true)
      self.toolbar = toolbar.ToolBar(self, -1, hideable=0)
      from gamera.gui import gamera_icons
      file_menu = self.toolbar.AddSimpleTool(5, gamera_icons.getIconFileBitmap(),
                                 "File Menu", self.OnFileClick)
      EVT_LEFT_UP(file_menu, self.OnFileLeftUp)
      self.toolbar.AddSeparator()
      self.toolbar.AddSimpleTool(10, gamera_icons.getIconGuessBitmap(),
                                 "Classify all glyphs", self.OnGuessClick)
      self.toolbar.AddSimpleTool(11, gamera_icons.getIconConfirmBitmap(),
                                 "Confirm all guesses", self.OnConfirmClick)
      self.toolbar.AddSeparator()
      self.box = wxBoxSizer(wxVERTICAL)
      self.box.Add(self.toolbar, 0, wxEXPAND|wxBOTTOM, 5)
      txID = NewId()
      self.text = wxTextCtrl(self, txID)
      EVT_KEY_DOWN(self.text, self.OnKey)
      EVT_TEXT(self, txID, self.OnText)
      self.box.Add(self.text, 0, wxEXPAND|wxBOTTOM, 5)
      tID = NewId()
      self.tree = SymbolTreeCtrl(self, self, tID, wxDefaultPosition,
                                 wxDefaultSize,
                                 wxTR_HAS_BUTTONS | wxTR_EDIT_LABELS)
      self.box.Add(self.tree, 1, wxEXPAND|wxALL)
      self.box.RecalcSizes()
      self.SetSizer(self.box)

   ########################################
   # CALLBACKS

   def OnFileClick(self, event):
      self.toplevel.file_menu()

   def OnFileLeftUp(self, event):
      self.toplevel.file_menu_destroy()

   def OnGuessClick(self, event):
      self.toplevel.guess_all()

   def OnConfirmClick(self, event):
      self.toplevel.confirm_all()

   def OnKey(self, evt):
      find = self.text.GetValue()
      if evt.KeyCode() == WXK_TAB:
         find = self.symbol_table.autocomplete(find)
         self.text.SetValue(find)
         self.text.SetInsertionPointEnd()
      elif evt.KeyCode() == WXK_RETURN:
         self.symbol_table.add(find)
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
      symbol, tokens = self.symbol_table.normalize_symbol(self.text.GetValue())
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

class FeatureEditorWizard(Wizard):
   dlg_select_database = Args([
      FileOpen('Database filename', '', extension="*.*")],
      name=name,
      function = 'cb_select_database',
      title = '1. Select a database to add and remove features.')

   def __init__(self, shell, locals):
      self.shell = shell
      self.parent = None
      self.locals = locals
      self.database = 'None'
      self.show(self.dlg_select_database)

   def cb_select_database(self, database):
      self.database = database
      self.classifier = Classifier(None, None, database, None, None, [])
      self.classifier.current_database[0].members_flat_category("Features")
      ff.sort()
      existing_features = map(lambda x: x[0], self.classifier.feature_functions)
      feature_controls = []
      self.feature_list = []
      for x in ff:
         feature_controls.append(Check('', str(x[0]),
                                       default=(x[0] in existing_features)))
         self.feature_list.append(x[0])
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
      dlg_save_as = Args([
         FileSave('Database filename', self.database, extension="*.*")],
                  name=name,
                  function = 'cb_save_as',
      title = '3. Where would you like to save the new database?')
      return dlg_save_as

   def cb_save_as(self, file):
      self.save_database = file
      return None

   def done(self):
      self.classifier.update_features(self.features)
      self.classifier.save_current_database(self.save_database)
