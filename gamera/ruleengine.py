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

import traceback, sys
from gamera import group, util
from fudge import Fudge

"""This module provides tools for applying graph-rewriting rules to a
set of glyphs.

It would seem nice to invent a completely new language for this task
(for instance, a logic-programming language like Prolog might have
been nice).

However, in this design, each rule is defined by a basic Python
function. This integrates much easier into the rest of the
Python-based Gamera world, since any Python function can be called at
any time, and doesn't require the user to learn a new language.  The
disadvantage is that the pattern matching is single-tiered (no
recursion) and the ordering and application of the rules is left to
the end programmer, since this approach makes it difficult to do any
global optimisation.  There may be ways around these limitations, but
that will require some thinking.

Rule functions take the basic form:

  def rule_function0(a="dot", b="letter*"):      # 1: function signature
    if (Fudge(a.lr_y) == b.lr_y and
        a.ul_x > b.lr_x and
        a.ul_x - b.lr_x < 20):                   # 2: expression
      a.classify_heuristic('punctuation.period') # 3: operation
      return [], []                              # 4: added, removed

1. The function signature itself is used to define which classes of
   glyphs should be applied to the function.  Each argument has a
   Python default value which is a string containing the symbol name
   or an "symbol name regular expression" that will match the glyph to
   be passed in.  Yes, this is a complete bastardisation of what
   default arguments are meant for in Python, since these string
   values will never actually be passed in to the function.  However,
   I think this syntax is convenient and kind of cute.  (Another
   option might be to use some sort of special doc-string syntax, I
   suppose.)

   Rules are indexed by the first argument, so it is a good idea to
   use the least-frequently occuring symbol name (or the class the
   most clearly defines the structure) as the first argument.

   A "symbol name regular expression" is an instance of the
   special-purpose regular expression language for symbol names
   defined in id_name_matching.py.  This syntax is pretty limited
   compared to standard Python regular expression syntax, but is much
   more convenient for the task. (For example, 'upper.*' would be
   'upper\.[^.]*' as a standard Python regular expression, and nobody
   wants to write code like that ;)
   
     Informal syntax definition:
       A|B                          # matches A or B
       A.B|C                        # matches A.B or A.C 
       (A.B)|C                      # matches A.B or C
       *                            # multiple-character wildcard
       ?                            # single character wildcard
       ()                           # grouping can be performed with
                                    # parentheses
       [a-z]                        # matches any character a-z

     Example expressions:
       (upper.x)|(lower.y)          # match either upper.x or lower.y
       upper.*                      # match the 'upper' category
       upper.a|b|c                  # matches upper.a, upper.b or upper.c
       upper.capital_?              # ? is a single character wildcard

2. The body of the function, in a literal sense, is left entirely up
   to the end-user programmer (it's just Python afterall.)  However,
   most rule functions will consist of some sort of test followed by
   some sort of change (side-effect!!!) of the data.

   For performing tests that need to be somewhat "fuzzy", you may want
   to use Fudge objects.  Comparisons on Fudge objects have a built-in
   fuzzy zone around the core int, Point or Rect that make comparing
   things to be "close enough" easier.  (See fudge.py)

3. In the body, the rule function can modify the glyph attributes in
   any way it sees fit.

4. Optionally, the rule function can return a pair of lists.  The
   first list is of glyphs that have been created by the operation
   (for instance, if combining two glyphs into one union glyph.)  The
   second list is a list of glyphs that have been removed by the
   operation (for instance removing noise glyphs.)  (If your function
   returns None, the RuleEngine assumes you meant [], []). These
   results are appended together each time a set of rules is applied
   and returned to the caller. Optionally, the rule-performing
   mechanism can reapply rules to all the newly-created glyphs.

PERFORMING RULES:

Rules are grouped into RuleEngine classes, which then perform the
actually searching and call the rule functions.

  # Initialise a new rule engine with a couple of rules
  rule_engine = RuleEngine([rule_function0, rule_function1])
  # Run the rules on the given list of glyphs
  added, removed = rule_engine.perform_rules(glyphs)

An optional argument, grid_size, specifies the size of the indexing
grid cells.  This will affect how far apart symbols can be matched.

TODO: There may be at some point some kind of Meta-RuleEngine for
performing sets of rules in sequence or parallel or whatever...  """

class RuleEngineError(Exception):
  pass

def example_rule(a="dot", b="letter*"):
  if (Fudge(a.lr_y) == b.lr_y and
      a.ul_x > b.lr_x and a.ul_x - b.lr_x < 20):
    a.classify_heuristic('punctuation.period')
  return [], []

class RuleEngine:
  _class_rules = []
  
  def __init__(self, rules=[], reapply=0):
    self.rules = {}
    self._regexs = {}
    self._rules_by_regex = {}
    for rule in rules + self._class_rules:
      self.add_rule(rule)
    self._reapply = reapply

  def add_rule(self, rule):
    import id_name_matching
    assert len(rule.func_defaults)
    self.rules[rule] = None
    regex = rule.func_defaults[0]
    if not self._rules_by_regex.has_key(regex):
      self._rules_by_regex[regex] = []
    self._rules_by_regex[regex].append(rule)
    for regex in rule.func_defaults:
      if not self._regexs.has_key(regex):
        self._regexs[regex] = id_name_matching.build_id_regex(regex)
      if not self._rules_by_regex.has_key(regex):
        self._rules_by_regex[regex] = []

  def get_rules(self):
    return self.rules.keys()

  def _deal_with_result(self, rule, glyphs, added, removed):
    try:
      current_stack = traceback.extract_stack()
      result = rule(*glyphs)
    except Exception, e:
      lines = traceback.format_exception(*sys.exc_info())
      del lines[1]
      self._exceptions[''.join(lines)] = None 
      return 0
    if result is None or result == ([],[]):
      return 0
    for a in result[0]:
      added[a] = None
    for a in result[1]:
      removed[a] = None
    return 1

  def perform_rules(self, glyphs, grid_size=100, recurse=0,
                    progress=None, _recursion_level=0):
    self._exceptions = {}
    if _recursion_level > 10:
      return [], []
    elif _recursion_level == 0:
      progress = util.ProgressFactory("Performing rules...")

    try:
      grid_index = group.GridIndexWithKeys(glyphs, grid_size, grid_size)
      found_regexs = {}
      for regex_string, compiled in self._regexs.items():
        for glyph in glyphs:
          if glyph.match_id_name(compiled):
            grid_index.add_glyph_by_key(glyph, regex_string)
            found_regexs[regex_string] = None

      # This loop is only so the progress bar can do something useful.
      for regex in found_regexs.iterkeys():
        progress.add_length(
          len(self._rules_by_regex[regex]) *
          len(grid_index.get_glyphs_by_key(regex)))
        
      added = {}
      removed = {}
      for regex in found_regexs.iterkeys():
        for rule in self._rules_by_regex[regex]:
          glyph_specs = rule.func_defaults
          for glyph in grid_index.get_glyphs_by_key(regex):
            if len(glyph_specs) == 1:
              self._deal_with_result(rule, (glyph,), added, removed)
            elif len(glyph_specs) == 2:
              for glyph2 in grid_index.get_glyphs_around_glyph_by_key(
                glyph, glyph_specs[1]):
                stop = self._deal_with_result(rule, (glyph, glyph2), added, removed)
                if not self._reapply and stop:
                  break
            else:
              seed = [list(grid_index.get_glyphs_around_glyph_by_key(glyph, x))
                      for x in glyph_specs[1:]]
              for combination in util.combinations(seed):
                stop = self._deal_with_result(rule, [glyph] + combination, added, removed)
                if not self._reapply and stop:
                  break
            progress.step()
    finally:
      if _recursion_level == 0:
        progress.kill()
    print
    if recurse and len(added):
      self._deal_with_result(
        self.perform_rules(added.keys(), 1, progress, _recursion_level + 1))

    if len(self._exceptions):
      s = ("One or more of the rule functions caused an exception.\n(Each exception listed only once:)\n\n" +
           "\n".join(self._exceptions.keys()))
      raise RuleEngineError(s)

    return added.keys(), removed.keys()
