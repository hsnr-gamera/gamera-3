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

from inspect import isfunction, ismodule
from os import path
import wx
from gamera.gui import toolbar, gui_util

class RuleEngineRunnerTree(wx.TreeCtrl):
   def __init__(self, toplevel, parent):
      self.toplevel = toplevel
      self.modules = []
      self.undo_history = []
      self.added = []
      self.removed = []
      id = wx.NewId()
      wx.TreeCtrl.__init__(self, parent, id)
      self.root = self.AddRoot("RuleSets")
      self.SetItemHasChildren(self.root, True)
      self.SetPyData(self.root, "")
      wx.EVT_TREE_ITEM_ACTIVATED(self, id, self._OnActivated)
      self.Expand(self.root)

   def _OnKey(self, event):
      pass

   def _OnActivated(self, event):
      from gamera import ruleengine
      item = event.GetItem()
      data = self.GetPyData(item)
      multi_display = self.toplevel.multi_iw.id
      single_display = self.toplevel.single_iw.id
      if isfunction(data):
         engine = ruleengine.RuleEngine([data])
      elif isinstance(data, ruleengine.RuleEngine):
         engine = data
      elif ismodule(data):
         engine = ruleengine.RuleEngine(
           [val for val in data.__dict__.values()
            if isinstance(val, ruleengine.RuleEngine)])
      added = {}
      removed = {}
      try:
         added, removed = engine.perform_rules(multi_display.GetAllItems())
      except ruleengine.RuleEngineError, e:
         gui_util.message(str(e))
         return
      if len(added) == 0 and len(removed) == 0:
         gui_util.message("No rules matched the data set.")
      multi_display.ClearSelection()
      multi_display.append_and_remove_glyphs(added, removed)
      single_display.highlight_cc(added, 0)
      # single_display.add_highlight_cc(removed, 1)
      self.undo_history.append((added, removed))
      self.added, self.removed = added, removed

   def _OnUndo(self, event):
      multi_display = self.toplevel.multi_iw.id
      single_display = self.toplevel.single_iw.id
      if len(self.undo_history):
         added, removed = self.undo_history.pop()
         multi_display.ClearSelection()
         for glyph in removed:
            del glyph.__dict__['dead']
         multi_display.append_and_remove_glyphs([], added)
         if len(self.undo_history):
            self.added, self.removed = self.undo_history[-1]
            single_display.highlight_cc(self.added, 0)
            single_display.add_highlight_cc(self.removed, 1)

   def _OnReloadCode(self, event):
      for module in self.modules:
         reload(module)
      self.CollapseAndReset(self.root)
      old_modules = self.modules[:]
      self.modules = []
      for module in old_modules:
         self.add_module(module)

   def _OnSelectAdded(self, event):
      self.toplevel.multi_iw.id.SelectGlyphs(self.added)

   def _OnSelectRemoved(self, event):
      self.toplevel.multi_iw.id.SelectGlyphs(self.removed)

   def add_module(self, module):
      from gamera import ruleengine
      self.modules.append(module)
      module_node = self.AppendItem(self.root, path.split(module.__file__)[1])
      self.SetPyData(module_node, module)
      for key, val in module.__dict__.items():
         self.SetItemHasChildren(module_node, True)
         if isinstance(val, ruleengine.RuleEngine):
            rule_engine_node = self.AppendItem(module_node, key)
            self.Expand(module_node)
            self.SetPyData(rule_engine_node, val)
            for rule in val.get_rules():
               self.SetItemHasChildren(rule_engine_node, True)
               rule_node = self.AppendItem(rule_engine_node, rule.__name__)
               self.SetPyData(rule_node, rule)

class RuleEngineRunnerPanel(wx.Panel):
   def __init__(self, toplevel, parent = None, id = -1):
      wx.Panel.__init__(
        self, parent, id,
        style=wx.WANTS_CHARS|wx.CLIP_CHILDREN|wx.NO_FULL_REPAINT_ON_RESIZE)
      self.toplevel = toplevel
      self.SetAutoLayout(True)
      self.toolbar = toolbar.ToolBar(self, -1)
      from gamera.gui import gamera_icons
      self.tree = RuleEngineRunnerTree(toplevel, self)
      self.toolbar.AddSimpleTool(
        10, gamera_icons.getIconUndoBitmap(),
        "Undo", self.tree._OnUndo)
      self.toolbar.AddSeparator()
      self.toolbar.AddSimpleTool(
        11, gamera_icons.getIconRefreshBitmap(),
        "Reload code", self.tree._OnReloadCode)
      self.toolbar.AddSeparator()
      self.toolbar.AddSimpleTool(
        12, gamera_icons.getIconSelectAddedBitmap(),
        "Select added glyphs", self.tree._OnSelectAdded)
      self.toolbar.AddSimpleTool(
        13, gamera_icons.getIconSelectRemovedBitmap(),
        "Select removed glyphs", self.tree._OnSelectRemoved)

      lc = wx.LayoutConstraints()
      lc.top.SameAs(self, wx.Top, 0)
      lc.left.SameAs(self, wx.Left, 0)
      lc.right.SameAs(self, wx.Right, 0)
      lc.height.AsIs()
      self.toolbar.SetAutoLayout(True)
      self.toolbar.SetConstraints(lc)
      lc = wx.LayoutConstraints()
      lc.top.Below(self.toolbar, 0)
      lc.left.SameAs(self, wx.Left, 0)
      lc.right.SameAs(self, wx.Right, 0)
      lc.bottom.SameAs(self, wx.Bottom, 0)
      self.tree.SetAutoLayout(True)
      self.tree.SetConstraints(lc)
      self.Layout()

   def open_module(self, filename):
      import imp
      fd = open(filename, 'r')
      module_name = path.split(filename)[1]
      module_name = module_name[:module_name.rfind('.')]
      module = imp.load_module(
        module_name, fd, filename, ('py', 'r', imp.PY_SOURCE))
      self.tree.add_module(module)
