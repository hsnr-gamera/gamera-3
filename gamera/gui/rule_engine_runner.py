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

from inspect import isfunction
from os import path
from wxPython.wx import *
from gamera import ruleengine
from gamera.gui import toolbar

class RuleEngineRunnerTree(wxTreeCtrl):
  def __init__(self, toplevel, parent):
    self.toplevel = toplevel
    self.modules = []
    self.undo_history = []
    self.added = []
    self.removed = []
    id = NewId()
    wxTreeCtrl.__init__(self, parent, id)
    self.root = self.AddRoot("RuleSets")
    self.SetItemHasChildren(self.root, TRUE)
    self.SetPyData(self.root, "")
    EVT_TREE_ITEM_ACTIVATED(self, id, self._OnActivated)
    self.Expand(self.root)

  def _OnKey(self, event):
    pass

  def _OnActivated(self, event):
    item = event.GetItem()
    data = self.GetPyData(item)
    multi_display = self.toplevel.multi_iw.id
    single_display = self.toplevel.single_iw.id
    if isfunction(data):
      engine = ruleengine.RuleEngine([data])
    elif isinstance(data, ruleengine.RuleEngine):
      engine = data
    else:
      # TODO: Do some module-level thing here
      return
    added, removed = engine.perform_rules(multi_display.GetAllItems())
    multi_display.ClearSelection()
    multi_display.append_and_remove_glyphs(added, removed)
    single_display.highlight_cc(added, 0)
    single_display.add_highlight_cc(removed, 1)
    self.undo_history.append((added, removed))
    self.added, self.removed = added, removed

  def _OnUndo(self, event):
    multi_display = self.toplevel.multi_iw.id
    single_display = self.toplevel.single_iw.id
    if len(self.undo_history):
      self.added, self.removed = self.undo_history.pop()
      multi_display.ClearSelection()
      for glyph in self.removed:
        del glyph.__dict__['dead']
      multi_display.append_and_remove_glyphs([], self.added)
      single_display.clear_all_highlights()
      
  def _OnReloadCode(self, event):
    for module in self.modules:
      reload(module)
    self.CollapseAndReset(self.root)
    old_modules = self.modules[:]
    self.modules = []
    for module in old_modules:
      self.add_module(module)

  def _OnSelectAdded(self, event):
    multi_display = self.toplevel.multi_iw.id
    multi_display.SelectGlyphs(self.added)

  def _OnSelectRemoved(self, event):
    multi_display = self.toplevel.multi_iw.id
    multi_display.SelectGlyphs(self.removed)

  def add_module(self, module):
    self.modules.append(module)
    module_node = self.AppendItem(self.root, module.__name__)
    self.SetPyData(module_node, module)
    for key, val in module.__dict__.items():
      self.SetItemHasChildren(module_node, TRUE)
      if isinstance(val, ruleengine.RuleEngine):
        rule_engine_node = self.AppendItem(module_node, key)
        self.SetPyData(rule_engine_node, val)
        for rule in val.get_rules():
          self.SetItemHasChildren(rule_engine_node, TRUE)
          rule_node = self.AppendItem(rule_engine_node, rule.__name__)
          self.SetPyData(rule_node, rule)

class RuleEngineRunnerPanel(wxPanel):
  def __init__(self, toplevel, parent = None, id = -1):
    wxPanel.__init__(
      self, parent, id,
      style=wxWANTS_CHARS|wxCLIP_CHILDREN|wxNO_FULL_REPAINT_ON_RESIZE)
    self.toplevel = toplevel
    self.SetAutoLayout(true)
    self.toolbar = toolbar.ToolBar(self, -1)
    from gamera.gui import gamera_icons
    self.tree = RuleEngineRunnerTree(toplevel, self)
    self.toolbar.AddSimpleTool(10, gamera_icons.getIconUndoBitmap(),
                               "Undo", self.tree._OnUndo)
    self.toolbar.AddSeparator()
    self.toolbar.AddSimpleTool(11, gamera_icons.getIconRefreshBitmap(),
                               "Reload code", self.tree._OnReloadCode)
    self.toolbar.AddSeparator()
    self.toolbar.AddSimpleTool(12, gamera_icons.getIconSelectBitmap(),
                               "Select added glyphs", self.tree._OnSelectAdded)
    self.toolbar.AddSimpleTool(13, gamera_icons.getIconSelectBitmap(),
                               "Select removed glyphs", self.tree._OnSelectRemoved)

    lc = wxLayoutConstraints()
    lc.top.SameAs(self, wxTop, 0)
    lc.left.SameAs(self, wxLeft, 0)
    lc.right.SameAs(self, wxRight, 0)
    lc.height.AsIs()
    self.toolbar.SetAutoLayout(true)
    self.toolbar.SetConstraints(lc)
    lc = wxLayoutConstraints()
    lc.top.Below(self.toolbar, 0)
    lc.left.SameAs(self, wxLeft, 0)
    lc.right.SameAs(self, wxRight, 0)
    lc.bottom.SameAs(self, wxBottom, 0)
    self.tree.SetAutoLayout(true)
    self.tree.SetConstraints(lc)
    self.Layout()
    
  def open_module(self, filename):
    import imp
    fd = open(filename, 'r')
    module_name = path.split(filename)[1]
    module_name = module_name[:module_name.rfind('.')]
    module = imp.load_module(module_name, fd, filename, ('py', 'r', imp.PY_SOURCE))
    self.tree.add_module(module)
