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
from gamera.gui import gaoptimizer_display, image_menu, var_name
from gamera.gui.gamera_display import *
import string

###############################################################################
# CLASSIFIER DISPLAY
###############################################################################

symbol_table = None

class ClassifierMultiImageDisplay(MultiImageDisplay):
   def __init__(self, toplevel, parent):
      self.toplevel = toplevel
      MultiImageDisplay.__init__(self, parent)
      self.last_image_no = None
      EVT_GRID_RANGE_SELECT(self, self.OnSelect)
      if hasattr(self, "GetGridWindow"):
         EVT_MOTION(self.GetGridWindow(), self.OnMotion)
         EVT_LEAVE_WINDOW(self.GetGridWindow(), self.OnLeave)
      self.tooltip = wxButton(self.GetGridWindow(), -1, "",
                              wxPoint(0, 0), wxSize(175, 24))
      font = self.tooltip.GetFont()
      font.SetPointSize(14)
      font.SetWeight(wxBOLD)
      self.tooltip.SetFont(font)
      self.tooltip.SetBackgroundColour(wxColor(red=247, green=243, blue=192))
      self.tooltip.Show(false)
      self.last_sort = None
      # This is to turn off the display of row labels if a) we have done some classification
      # and b) we have done a sort other than the default. KWM
      self.display_row_labels = 0

   def append_glyphs(self, list):
      wxBeginBusyCursor()
      self.BeginBatch()
      self.list.extend(list)
      self.resize_grid(do_auto_size=0)
      self.EndBatch()
      wxEndBusyCursor()

   ########################################
   # AUTO-MOVE

   def do_auto_move(self, state):
      if state != []:
         found = -1
         firstrow = firstcol = -1
         lastrow = lastcol = 0
         if self.IsSelection():
            for row in range(self.rows):
               for col in range(self.cols):
                  if self.IsInSelection(row, col):
                     lastrow = row
                     lastcol = col
         image_no = self.get_image_no(lastrow, lastcol)
         for i in range(image_no + 1, len(self.list)):
            if (self.list[i] != None and
                self.list[i].classification_state in state):
               found = i
               break
         if found != -1:
            row = found / GRID_NCOLS
            col = found % GRID_NCOLS
            self.SetGridCursor(row, col)
            self.SelectBlock(row, col, row, col, 0)
            self.MakeCellVisible(row, col)
            if not self.list[i].manually_classified():
               id = self.toplevel.classifier.guess_classify_glyph(self.list[i])
               self.toplevel.set_label_display([id])
               self.toplevel.display_label_at_cell(row, col, id)
            return
      if self.IsSelection():
         if config.get_option("need_full_refresh"):
            self.ForceRefresh()
         else:
            cells = []
            for row in range(self.rows):
               for col in range(self.cols):
                  if self.IsInSelection(row, col):
                     cells.append((row, col))
            first = 0
            if len(cells) < 3:
               for row, col in cells:
                  self.SelectBlock(row, col, row, col, first)
                  if first == 0:
                     first = 1
            else:
               self.ForceRefresh()
         

   ########################################
   # DISPLAYING A LABEL BENEATH A CELL

   def display_label_at_cell(self, row, col, label):
      rect = self.CellToRect(row, col)
      origin = self.GetViewStart()
      units = self.GetScrollPixelsPerUnit()
      self.tooltip.Move(wxPoint(rect.GetLeft() - origin[0] * units[0],
                                rect.GetBottom() - origin[1] * units[1] + 3))
      self.tooltip.Show(1)
      self.tooltip.SetLabel(label)

   def find_glyphs_in_rect(self, x1, y1, x2, y2):
      self.BeginBatch()
      matches = []
      if x1 == x2 or y1 == y2:
         point = Point(x1, y1)
         for i in range(len(self.list)):
            g = self.list[i]
            if g != None and g.contains_point(point):
                  if (g.get(y1 - g.page_offset_y(), x1 - g.page_offset_x()) != 0):
                     matches = [i]
                     break
      else:
         matches = []
         r = Rect(y1, x1, y2 - y1 + 1, x2 - x1 + 1)
         for i in range(len(self.list)):
            g = self.list[i]
            if g != None:
               if r.contains_rect(g.m):
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
         for image in self.toplevel.last_cc_displayed:
            self.toplevel.single_iw.clear_highlight_cc(image)
         self.toplevel.single_iw.focus(self.toplevel.last_cc_displayed)
         self.toplevel.last_cc_displayed = []
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
         id_name_cmp = cmp(a.id_name[0], b.id_name[0])
      if id_name_cmp == 0:
         id_name_cmp = cmp(b.classification_state, a.classification_state)
      return id_name_cmp

   def split_classified_from_unclassified(self, list):
      # Find split between classified and unclassified
      for i in range(len(list)):
         if list[i].unclassified():
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
         column = column + 1
         if column >= GRID_NCOLS:
            column = 0
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
         if not item.unclassified():
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
      # Remove any "None"s in the list
      self.last_sort = None
      if function:
         # Turn off the row labels
         self.display_row_labels = 0
      else:
         self.display_row_labels = 1
      global symbol_table
      symbol_table = self.toplevel.classifier.symbol_table
      wxBeginBusyCursor()
      self.BeginBatch()
      orig_len = len(self.list)
      new_list = []
      for item in self.list:
         if item != None:
            new_list.append(item)
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
      for i in range(self.rows):
         try:
            image = self.list[i * GRID_NCOLS]
         except IndexError:
            self.SetRowLabelValue(i, "")
         else:
            if image == None or image.unclassified():
               self.SetRowLabelValue(i, "")
            elif self.display_row_labels:
               label = image.id_name[0]
               max_label = max(max_label, len(label))
               self.SetRowLabelValue(i, label)
            else:
               self.SetRowLabelValue(i, "")
      self.EndBatch()
      return max_label

   ########################################
   # CALLBACKS

   # Display selected items in the context display
   def OnSelectImpl(self):
      if not self.updating:
         image = self.GetSelectedItems()
         if image != []:
            symbol_table = self.toplevel.classifier.symbol_table
            id = image[0].id_name
            all_same = 1
            for x in image:
               if x.id_name != id:
                  all_same = 0
                  break
            if all_same:
               self.toplevel.set_label_display(id)
            else:
               self.toplevel.set_label_display([])
            self.toplevel.display_cc(image)

   def OnSelect(self, event):
      event.Skip()
      self.OnSelectImpl()

   # Display the label in the status bar.
   # Too bad wxPython makes this so hideously complicated
   def OnMotion(self, event):
      origin = self.GetViewStart()
      units = self.GetScrollPixelsPerUnit()
      row = self.YToRow(event.GetY() + origin[1] * units[1])
      col = self.XToCol(event.GetX() + origin[0] * units[0])
      image_no = self.get_image_no(row, col)
      if image_no == None or image_no >= len(self.list) or image_no < 0:
         image = None
      else:
         image = self.list[image_no]
      if image == None or image.unclassified():
         self.tooltip.Show(false)
      else:
         id = image.id_name
         self.tooltip.Show(true)
         self.tooltip.Move(wxPoint(event.GetX() + 14, event.GetY() + 14))
         if self.last_image_no != image_no:
            self.tooltip.SetLabel(id[0])
      self.last_image_no = image_no
      event.Skip()

   def OnLeave(self, event):
      self.tooltip.Show(false)
      event.Skip()

class ClassifierMultiImageWindow(MultiImageWindow):
   def __init__(self, toplevel, parent = None, id = -1, size = wxDefaultSize):
      self.toplevel = toplevel
      MultiImageWindow.__init__(self, parent, id, size)
      self.toolbar.AddSeparator()
      from gamera.gui import gamera_icons
      self.toolbar.AddTool(20, gamera_icons.getIconNextUnclassBitmap(), isToggle=TRUE,
                           shortHelpString = "Automatically move to next unclassified glyph.")
      self.toolbar.AddTool(21, gamera_icons.getIconNextAutoclassBitmap(), isToggle=TRUE,
                           shortHelpString =
                           "Automatically move to next automatically classified glyph.")
      self.toolbar.AddTool(22, gamera_icons.getIconNextHeurclassBitmap(), isToggle=TRUE,
                           shortHelpString =
                           "Automatically move to next heuristically classified glyph.")
      self.toolbar.AddTool(23, gamera_icons.getIconNextManclassBitmap(), isToggle=TRUE,
                           shortHelpString =
                           "Automatically move to next manually classified glyph.")
      EVT_TOOL_RANGE(self, 20, 23, self.OnAutoMove)

   def get_display(self):
      return ClassifierMultiImageDisplay(self.toplevel, self)

   def OnAutoMove(self, event):
      id = event.GetId()
      if self.toolbar.GetToolState(id):
         self.toplevel.auto_move.append(id - 20)
      else:
         self.toplevel.auto_move.remove(id - 20)

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
      self.toolbar.AddSimpleTool(40, gamera_icons.getIconChooseImageBitmap(),
                                 "Choose new image")
      EVT_TOOL(self, 40, self.OnChooseImage)

   def OnChooseImage(self, event):
      from args import Args, Class
      dlg = Args([Class("Image for context display", "core.Image")])
      function = dlg.show(self, image_menu.shell.locals, "")
      if function != None:
         function = function[1:-1]
         image = image_menu.shell.locals[function]
         self.id.set_image(image, Image.to_string)

   def get_display(self):
      return ClassifierImageDisplay(self.toplevel, self)

GA_TIMER_ID = 400
class ClassifierFrame(ImageFrameBase):
   def __init__(self, classifier, parent = None, id = -1, title = "Classifier",
                owner=None):
      global toplevel
      toplevel = self
      self.image = None
      self.menu = None
      ImageFrameBase.__init__(self, parent, id, title, owner)
      self.SetSize((800, 600))
      self.classifier = classifier
      self.last_id = []
      self.last_cc_displayed = []
      self.auto_move = []
      self.splitterv = wxSplitterWindow(self, -1)
      self.splitterh = wxSplitterWindow(self.splitterv, -1)
      self.single_iw = ClassifierImageWindow(self, self.splitterh)
      self.multi_iw = ClassifierMultiImageWindow(self, self.splitterh)
      self.splitterh.SetMinimumPaneSize(5)
      self.splitterh.SplitHorizontally(self.multi_iw, self.single_iw, 300)
      self.symbol_editor = SymbolTableEditorPanel(classifier.symbol_table,
                                                  self, self.splitterv)
      self.splitterv.SetMinimumPaneSize(5)
      self.splitterv.SplitVertically(self.symbol_editor, self.splitterh, 160)
      from gamera.gui import gamera_icons
      icon = wxIconFromBitmap(gamera_icons.getIconClassifyBitmap())
      self.SetIcon(icon)
      EVT_SIZE(self, self.OnSizeImpl)
      
      # GA optimizer
      self.ga_running = 0
      self.ga = gaoptimizer_display.OptimizerFrame(self, -1, "GA Optimization Settings")
      self.ga.set_classifier(self.classifier)
      self.ga_timer = wxTimer(self, GA_TIMER_ID)
      self.best_weights = None
      self.best_weights_score = 0
      EVT_TIMER(self, GA_TIMER_ID, self.update_from_ga)

      self.CreateStatusBar(3)
      self.check_optimizer = wxCheckBox(self.GetStatusBar(), 500, "Optimize Database")
      EVT_CHECKBOX(self, 500, self.OnCheckOptimizer)

      id = NewId()
      self.check_optimizer_settings = wxButton(self.GetStatusBar(), id, "Optimization Settings")
      EVT_BUTTON(self, id, self.OnCheckOptimizerSettings)

      self.meter = wxGauge(self.GetStatusBar(), -1, 100, size=(100, 20))
      self.meter_text = wxTextCtrl(self.GetStatusBar(), -1, "", style=wxTE_READONLY)
      self.SetStatusBarPositions()

   def OnCheckOptimizerSettings(self, e):
      self.ga.Show(1)

   def OnCheckOptimizer(self, event):
      wxBeginBusyCursor()
      self.ga_timer.Stop()
      self.ga.stop()
      if self.check_optimizer.GetValue():
         self.ga.start()
         self.ga_running = 1
         self.ga_timer.Start(1000)
         self.ga.ga.lock.acquire()
         self.meter.SetValue(int(self.ga.ga.best_score))
         self.ga.ga.lock.release()
      else:
         self.meter.SetValue(0)
      wxEndBusyCursor()

   #######################################
   ## Status Bar

   def SetStatusBarPositions(self):
      # Settings
      rect = self.GetStatusBar().GetFieldRect(0)
      self.check_optimizer_settings.SetPosition(wxPoint(rect.x + 2, rect.y))
      # Status
      rect = self.GetStatusBar().GetFieldRect(1)
      self.check_optimizer.SetPosition(wxPoint(rect.x + 2, rect.y))
      # Meter Text
      rect = self.GetStatusBar().GetFieldRect(2)
      self.meter_text.SetPosition(wxPoint(rect.x + 2, rect.y))
      # Meter
      self.meter.SetDimensions(rect.x + self.meter_text.GetSizeTuple()[0] + 2, rect.y,
                               rect.width - self.meter_text.GetSizeTuple()[0], 20)
      

   def OnSizeImpl(self, event):
      self.SetStatusBarPositions()
      event.Skip()

   def set_image(self, ccs, image=None, function=None):
      self.to_string_function = function
      self.ccs = ccs
      self.multi_iw.set_image(ccs, function)
      if image != None:
         self.image = image
         if self.image == None:
            self.splitterh.SetSashPosition(10000)
            self.image = Image(300, 200)
            self.to_string_function = Image.to_string
         self.single_iw.set_image(self.image, self.to_string_function)

   def append_glyphs(self, list):
      self.multi_iw.id.append_glyphs(list)

   ########################################
   # DISPLAY

   def display_cc(self, cc):
      if self.image != None:
         for image in self.last_cc_displayed:
            if image not in cc:
               self.single_iw.clear_highlight_cc(image)
         for image in cc:
            if image not in self.last_cc_displayed:
               self.single_iw.highlight_cc(image, Image.to_string)
         self.single_iw.focus(cc)
         self.last_cc_displayed = cc

   def find_glyphs_in_rect(self, x1, y1, x2, y2):
      self.multi_iw.find_glyphs_in_rect(x1, y1, x2, y2)

   ########################################
   # CLASSIFICATION FUNCTIONS

   def guess(self):
      wxBeginBusyCursor()
      self.classifier.auto_classify_all()
      self.multi_iw.id.sort_images(None)
      wxEndBusyCursor()

   def guess_list(self, list):
      for x in list:
         self.classifier.auto_classify_glyph(x)

   def confirm_all(self):
      for x in self.classifier.current_database:
         if x.automatically_classified():
            self.classifier.manual_classify_glyph(x, x.id_name)
      self.multi_iw.id.ForceRefresh()
   
   def manual_classify(self, id):
      wxBeginBusyCursor()
      selection = self.multi_iw.id.GetSelectedItems(
         self.multi_iw.id.GetGridCursorRow(),
         self.multi_iw.id.GetGridCursorCol())
      if selection != []:
         self.multi_iw.id.BeginBatch()
         self.manual_classify_list(selection, id)
         if not self.do_auto_move():
            self.set_label_display([id])
         self.multi_iw.id.EndBatch()
         wxEndBusyCursor()

   # To avoid multiple refreshes of the grid, this batches manually
   # classifying the list and refreshes the grid at the end.
   def manual_classify_list(self, list, id):
      removed = 0
      did_splits = 0
      for item in list:
         if isinstance(item, core.Image):
            a, b = self.classifier.manual_classify_glyph(
               item, id, update_display=0)
         did_splits = did_splits + a
         removed = removed or b
      if removed:
         self.set_image(self.classifier.current_database)
      elif did_splits:
         self.append_glyphs(self.classifier.current_database[-did_splits:])

   ####################################################
   # GA
   def update_from_ga(self, event):
      if self.ga_running:
         self.ga.ga.lock.acquire()
         weights = self.ga.ga.get_weights()
         if weights != None:
            current_score = self.ga.ga.best_score
            # We only want to replace our wieghts if the current score is better than
            # the overall best. This stays across restarting the GA
            if current_score > self.best_weights_score or self.best_weights == None:
               self.best_weights = self.ga.ga.get_weights()
               self.best_weights_score = current_score
               self.classifier.knn_database.set_weights(self.best_weights)
         self.ga.ga.lock.release()
         self.meter.SetValue(int(self.best_weights_score))
         self.meter_text.SetValue(str(self.best_weights_score) + '%')

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
         self.symbol_editor.tree.set_label_display(ids[0])
      else:
         self.symbol_editor.tree.set_label_display("")

   def display_label_at_cell(self, row, col, label):
      self.multi_iw.id.display_label_at_cell(row, col, label)

   def file_menu(self):
      self.menu = wxMenu()
      self.menu.Append(10, "Save production database")
      self.menu.Append(11, "Save production database as...")
      self.menu.Append(16, "Save production database with images")
      self.menu.Append(17, "Save production database with images as...")
      self.menu.Append(14, "Merge into production database...")
      self.menu.AppendSeparator()
      self.menu.Append(12, "Save current database")
      self.menu.Append(13, "Save current database as...")
      self.menu.Append(18, "Save current database with images")
      self.menu.Append(19, "Save current database with images as...")
      self.menu.AppendSeparator()
      self.menu.Append(21, "Import weights and features...")
      self.menu.Append(20, "Export weights and features...")
      self.menu.AppendSeparator()
      self.menu.Append(15, "Export symbol table...")
      EVT_MENU(self, 10, self.OnSaveProductionDatabase)
      EVT_MENU(self, 11, self.OnSaveProductionDatabaseAs)
      EVT_MENU(self, 16, self.OnSaveProductionDatabaseWithImages)
      EVT_MENU(self, 17, self.OnSaveProductionDatabaseWithImagesAs)
      EVT_MENU(self, 14, self.OnMergeProductionDatabase)
      EVT_MENU(self, 12, self.OnSaveCurrentDatabase)
      EVT_MENU(self, 13, self.OnSaveCurrentDatabaseAs)
      EVT_MENU(self, 18, self.OnSaveCurrentDatabaseWithImages)
      EVT_MENU(self, 19, self.OnSaveCurrentDatabaseWithImagesAs)
      EVT_MENU(self, 21, self.OnImportWeights)
      EVT_MENU(self, 20, self.OnExportWeights)
      EVT_MENU(self, 15, self.OnSaveSymbolTable)
      self.PopupMenu(self.menu, wxPoint(5, 22))

   def file_menu_destroy(self):
      if self.menu != None:
         self.menu.Destroy()
         self.menu = None

   ########################################
   # CALLBACKS

   def OnCloseWindow(self, event):
      if self.classifier.is_dirty:
         message = wxMessageDialog(self,
                                   "Are you sure you want to quit without saving?",
                                   "Classifier",
                                   wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION )
         result = message.ShowModal()
         if result != wxID_YES:
            return
      del self.single_iw
      del self.multi_iw
      if self.owner:
         self.owner.set_display(None)
      self.ga_timer.Stop()
      self.ga.stop()
      self.Destroy()

   def OnSaveProductionDatabase(self, event):
      if self.classifier.production_database_filename == None:
         self.OnSaveProductionDatabaseAs(event)
      else:
         wxBeginBusyCursor()
         self.classifier.save_production_database(
            self.classifier.production_database_filename)
         wxEndBusyCursor()

   def OnSaveProductionDatabaseAs(self, event):
      dlg = wxFileDialog(None, "Choose a file", ".", "", "*.*", wxSAVE)
      if dlg.ShowModal() == wxID_OK:
         self.classifier.production_database_filename = dlg.GetPath()
         dlg.Destroy()
         self.OnSaveProductionDatabase(event)

   def OnSaveProductionDatabaseWithImages(self, event):
      if self.classifier.production_database_filename == None:
         self.OnSaveProductionDatabaseAs(event)
      else:
         wxBeginBusyCursor()
         self.classifier.save_production_database(
            self.classifier.production_database_filename, 1)
         wxEndBusyCursor()

   def OnSaveProductionDatabaseWithImagesAs(self, event):
      dlg = wxFileDialog(None, "Choose a file", ".", "", "*.*", wxSAVE)
      if dlg.ShowModal() == wxID_OK:
         self.classifier.production_database_filename = dlg.GetPath()
         dlg.Destroy()
         self.OnSaveProductionDatabaseWithImages(event)

   def OnMergeProductionDatabase(self, event):
      dlg = wxFileDialog(None, "Choose a file", ".", "", "*.*", wxOPEN)
      if dlg.ShowModal() == wxID_OK:
         filename = dlg.GetPath()
         dlg.Destroy()
         wxBeginBusyCursor()
         self.classifier.merge_production_database(filename)
         wxEndBusyCursor()
      
   def OnSaveCurrentDatabase(self, event):
      if self.classifier.current_database_filename == None:
         self.OnSaveCurrentDatabaseAs(event)
      else:
         wxBeginBusyCursor()
         self.classifier.save_current_database(
            self.classifier.current_database_filename)
         wxEndBusyCursor()

   def OnSaveCurrentDatabaseAs(self, event):
      dlg = wxFileDialog(None, "Choose a file", ".", "", "*.*", wxSAVE)
      if dlg.ShowModal() == wxID_OK:
         self.classifier.current_database_filename = dlg.GetPath()
         dlg.Destroy()
         self.OnSaveCurrentDatabase(event)

   def OnSaveCurrentDatabaseWithImages(self, event):
      if self.classifier.current_database_filename == None:
         self.OnSaveCurrentDatabaseAs(event)
      else:
         wxBeginBusyCursor()
         self.classifier.save_current_database(self.classifier.current_database_filename)
         wxEndBusyCursor()

   def OnSaveCurrentDatabaseWithImagesAs(self, event):
      dlg = wxFileDialog(None, "Choose a file", ".", "", "*.*", wxSAVE)
      if dlg.ShowModal() == wxID_OK:
         self.classifier.current_database_filename = dlg.GetPath()
         dlg.Destroy()
         self.OnSaveCurrentDatabaseWithImages(event)

   def OnSaveSymbolTable(self, event):
      dlg = wxFileDialog(None, "Choose a file", ".", "", "*.*", wxSAVE)
      if dlg.ShowModal() == wxID_OK:
         filename = dlg.GetPath()
         dlg.Destroy()
         wxBeginBusyCursor()
         self.classifier.save_symbol_table(filename)
         wxEndBusyCursor()

   def OnImportWeights(self, event):
      dlg = wxFileDialog(None, "Choose a file", ".", "", "*.*", wxOPEN)
      if dlg.ShowModal() == wxID_OK:
         filename = dlg.GetPath()
         dlg.Destroy()
         wxBeginBusyCursor()
         self.classifier.load_weights(filename)
         wxEndBusyCursor()

   def OnExportWeights(self, event):
      dlg = wxFileDialog(None, "Choose a file", ".", "", "*.*", wxOPEN)
      if dlg.ShowModal() == wxID_OK:
         filename = dlg.GetPath()
         dlg.Destroy()
         wxBeginBusyCursor()
         self.classifier.save_weights(filename)
         wxEndBusyCursor()

   def close(self):
      self.single_iw.Destroy()
      self.multi_iw.Destroy()
      self.Destroy()

   def refresh(self):
      self.single_iw.refresh(1)


##############################################################################
# SYMBOL TABLE EDITOR
##############################################################################

class SymbolTreeCtrl(wxTreeCtrl):
   def __init__(self, toplevel, parent, id, pos, size, style):
      self.toplevel = toplevel
      self.editing = 0
      wxTreeCtrl.__init__(self, parent, id, pos, size, style)
      self.SetWindowStyle(wxSUNKEN_BORDER)
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
      # show how to reject edit, we'll not allow any digits
      previous = self.GetPyData(event.GetItem())
      previous_stub = string.join(string.split(previous, ".")[:-1], ".")
      if previous_stub == "":
         new = event.GetLabel()
      else:
         new = previous_stub + "." + event.GetLabel()
      self.SetPyData(event.GetItem(), new)

      self.toplevel.symbol_table.rename(previous, new)
      self.editing = 0

   def OnItemExpanded(self, event):
      parent = event.GetItem()
      parent_path = self.GetPyData(parent)
      items = self.toplevel.symbol_table.get_category_contents_list(parent_path)
      for name, is_parent in items:
         item = self.AppendItem(parent, name)
         new_path = string.join((parent_path, name), ".")
         if new_path[0] == ".":
            new_path = new_path[1:]
         self.SetPyData(item, new_path)
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
      id = self.GetPyData(item)
      if id != None:
         self.toplevel.toplevel.manual_classify(id)

   def OnRightDown(self, event):
      self.editing = 1
      pt = event.GetPosition()
      item, flags = self.HitTest(pt)
      self.EditLabel(item)

   def OnRightUp(self, event):
      self.editing = 0

   def OnLeftDown(self, event):
      pt = event.GetPosition()
      item, flags = self.HitTest(pt)
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
      self.toolbar = wxToolBar(self, -1, style=wxTB_HORIZONTAL)
      from gamera.gui import gamera_icons
      self.toolbar.AddSimpleTool(5, gamera_icons.getIconFileBitmap(),
                                 "File Menu")
      EVT_TOOL(self, 5, self.OnFileClick)
      EVT_LEFT_UP(self.toolbar, self.OnFileLeftUp)
      self.toolbar.AddSeparator()
      self.toolbar.AddSimpleTool(10, gamera_icons.getIconGuessBitmap(),
                                 "Classify all glyphs")
      EVT_TOOL(self, 10, self.OnGuessClick)
      self.toolbar.AddSimpleTool(11, gamera_icons.getIconConfirmBitmap(),
                                 "Confirm all guesses")
      EVT_TOOL(self, 11, self.OnConfirmClick)
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
      self.toplevel.guess()

   def OnConfirmClick(self, event):
      self.toplevel.confirm_all()

   def OnKey(self, evt):
      find = self.text.GetValue()
      if evt.KeyCode() == WXK_TAB:
         find = self.symbol_table.autocomplete(find)
         self.text.SetValue(find)
         self.text.SetInsertionPointEnd()
      elif evt.KeyCode() == WXK_RETURN:
         self.symbol_table.add_if_not_exists(find)
         self.toplevel.manual_classify(find)
      elif evt.KeyCode() == WXK_LEFT and evt.AltDown():
         current = self.text.GetInsertionPoint()
         new = max(string.rfind(self.text.GetValue(), ".", 0, current), 0)
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
         new = string.rfind(value, ".", 0, current)
         if new == -1:
            self.text.SetValue("")
         else:
            self.text.SetValue(self.text.GetValue()[0:new+1])
         self.text.SetInsertionPointEnd()
         return
      elif evt.KeyCode() == WXK_RIGHT and evt.AltDown():
         current = self.text.GetInsertionPoint()
         new = string.find(self.text.GetValue(), ".", current)
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
            last = string.split(s, ".")[-1]
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
      if (self.locals[self.context_image].get_type() == "GreyScale"):
         self.shell.run(self.context_image + ".threshold()")
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
         self.current_database = "'" + filename + "'"
      return self.dlg_select_production_database

   def cb_select_production_database(self, empty, load, production_database):
      if load:
         if production_database == 'None':
            dlg = wxFileDialog(None, "Choose a Gamera XML file",
                               ".", "", "*.xml", wxOPEN)
            if dlg.ShowModal() == wxID_OK:
               production_database = dlg.GetPath()
               dlg.Destroy()
         self.production_database = "'" + production_database + "'"
      if self.production_database == 'None' and self.ccs != 'None':
         self.locals[self.ccs][0].methods_flat_category("Features")
         ff.sort()
         feature_controls = []
         self.feature_list = []
         for x in ff:
            feature_controls.append(Check('', str(x.__name__), default=1))
            self.feature_list.append(x.__name__)
         self.dlg_features = Args(
            feature_controls,
            name=name, function='cb_features',
            title='Select the features you would like to use for classification')
         return self.dlg_features
      return None

   def cb_features(self, *args):
      self.features = []
      for i in range(len(args)):
         if args[i]:
            self.features.append(self.feature_list[i])
      if self.current_database == 'None':
         return self.dlg_select_symbol_table
      return None

   def cb_select_symbol_table(self, symbol_table):
      if symbol_table != "None":
         self.symbol_table = "'" + symbol_table + "'"
      return None

   def done(self):
      classifier = var_name.get("classifier", self.shell.locals)
      try:
         self.shell.run("%s = Classifier(%s, %s, %s, %s, %s, %s)" %
                        (classifier, self.ccs, self.production_database,
                         self.current_database,
                         self.symbol_table, self.context_image,
                         self.features))
         
         self.shell.run("%s.display(%s)" % (classifier, self.context_image))
      except e:
         pass

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
