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

import types, string, keyword

class SymbolTable:
   def __init__(self):
      self.symbols = {}
      self.listeners = []
      self.rename_listeners = []
      self.remove_listeners = []
      self.add("skip", 0)

   def add_listener(self, listener):
      assert hasattr(listener, 'symbol_table_callback')
      self.listeners.append(listener)
      # Get the listener up-to-date
      for symbol in self.symbols.keys():
         self.alert_add(self.normalize_symbol(symbol)[1])

   def remove_listener(self, listener):
      self.listeners.remove(listener)

   def alert_add(self, symbol):
      for l in self.listeners:
         if hasattr(l, 'symbol_table_callback'):
            l.symbol_table_callback(symbol)

   def alert_remove(self, a):
      for l in self.listeners:
         if hasattr(l, 'symbol_table_remove_callback'):
            l.symbol_table_remove_callback(a)

   def normalize_symbol(self, symbol):
      # assert type(symbol) == types.StringType
      while len(symbol) and symbol[0] == '.':
         symbol = symbol[1:]
      if symbol == '':
         return '', []
      for i in ' !@#$%^&*()-=+~`|\{}[];:"\',<>?/':
         symbol = symbol.replace(i, '_')
      symbol = symbol.lower()
      if symbol[0] in string.digits or keyword.iskeyword(symbol):
         symbol = '_' + symbol + '_'
      # Split by '.' delimiters
      orig_tokens = symbol.strip().split('.')
      tokens = []
      for token in orig_tokens:
         if token.strip() != '':
            tokens.append(token.strip())
      symbol = '.'.join(tokens)
      return symbol, tokens

   def add(self, symbol, id = -1):
      symbol, tokens = self.normalize_symbol(symbol)
      if self.symbols.has_key(symbol):
         return symbol
      self.symbols[symbol] = None
      self.alert_add(tokens)
      return symbol

   def remove(self, symbol):
      symbol, tokens = self.normalize_symbol(symbol)
      if self.symbols.has_key(symbol):
         del self.symbols[symbol]
         self.alert_remove(tokens)
         return 1
      return 0

   def autocomplete(self, symbol):
      targets = self.symbols.keys()
      targets.sort()
      found_i = -1
      for i in range(len(targets)):
         if targets[i].startswith(symbol):
            found_i = i
            break
      if found_i != -1:
         found = targets[found_i]
         if found_i < len(targets) - 1:
            found_next = targets[found_i + 1]
            print found, found_next
            if (found_next.startswith(symbol) and 
                not found_next.startswith(found)):
               for i in range(len(symbol), len(found)):
                  if found[i] != found_next[i]:
                     break
               found = found[:i]
         return found
      return symbol

   def exists(self, symbol):
      return self.symbols.has_key(symbol)
