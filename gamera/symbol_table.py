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

class SymbolTable:
   def __init__(self):
      self.categories = {}
      self.symbols = {}
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
      # assert type(symbol) == types.StringType
      if symbol == '':
         return '', []
      for i in '!@#$%^&*()-=+':
         symbol = symbol.replace(i, '_')
      symbol = symbol.lower()
      # Split by '.' delimiters
      tokens = symbol.strip().split('.')
      # Remove internal whitespace
      tokens = map(lambda x: '_'.join(x.split()), tokens)
      symbol = '.'.join(tokens)
      if symbol[-1] == ".":
         symbol = symbol[:-1]
      return symbol, tokens

   def add(self, symbol, id = -1):
      symbol, tokens = self.normalize_symbol(symbol)
      if self.symbols.has_key(symbol):
         return
      self.add_to_tree(symbol, tokens, id)

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
            name = ".".join(tokens[0:i+1])
            self.symbols[name] = None
         category = category[token]
         if category == {}:
            add_to_alert = 0
         if add_to_alert:
            alert.append(token)
      if do_alert:
         self.alert_listeners(".".join(alert))

   def remove(self, symbol):
      symbol, tokens = self.normalize_symbol(symbol)
      if symbol == '':
         self.categories = {}
         self.symbols = {}
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
      removed.append(symbol)
      del old_category[tokens[-1]]
      try:
         del self.symbols[symbol]
      except:
         pass
      self.alert_listeners('.'.join(tokens[:-1]))
      return removed

   def rename(self, a, b):
      a, x = self.normalize_symbol(a)
      b, x = self.normalize_symbol(b)
      for key in self.symbols.keys():
         if key.startswith(a):
            del self.symbols[key]
            self.symbols[b + key[len(a):]] = None
      self.categories = {}
      for key in self.symbols.keys():
         symbol, tokens = self.normalize_symbol(key)
         self.add_to_tree(symbol, tokens, do_alert=0)
      self.alert_rename_listeners(a, b)
      self.alert_listeners(symbol)

   def autocomplete(self, symbol):
      symbol, tokens = self.normalize_symbol(symbol)
      try:
         category = self.get_category_contents('.'.join(tokens[:-1]))
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
         result = ".".join(tokens[:-1] + [found])
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

   def exists(self, symbol):
      return self.symbols.has_key(symbol)
