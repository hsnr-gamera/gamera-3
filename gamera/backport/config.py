"""

Provides an extended option parser that loads options from a file and then

overrides them with options on the command line



Copyright (C) 2001-2005 Michael Droettboom

"""

## This program is free software; you can redistribute it and/or

## modify it under the terms of the GNU General Public License

## as published by the Free Software Foundation; either version 2

## of the License, or (at your option) any later version.



## This program is distributed in the hope that it will be useful,

## but WITHOUT ANY WARRANTY; without even the implied warranty of

## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the

## GNU General Public License for more details.

 

## You should have received a copy of the GNU General Public License

## along with this program; if not, write to the Free Software

## Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.



from optparse import OptionParser, make_option, OptionConflictError

import ConfigParser



from os.path import isfile, expanduser, split, join

from sys import platform



class ConfigOptionParser(OptionParser):

   default_options = []



   def __init__(self, *args, **kwargs):

      OptionParser.__init__(self, *args, **kwargs)

      self.add_options(self.default_options)

      self._cache = None



   def add_option(self, *args, **kwargs):

      try:

         OptionParser.add_option(self, *args, **kwargs)

      except OptionConflictError:

         pass



   def add_options(self, l):

      for x in l:

         self.add_option(x)



   def get_config_files(self):

      return []



   def parse_args(self):

      if self._cache is None:

         options, args = OptionParser.parse_args(self)

         files = self.get_config_files()

         config_parser = ConfigParser.RawConfigParser()

         config_parser.read(files)

         for section in config_parser.sections():

            for key, val in config_parser.items(section):

               if self.has_option("--" + key):

                  option = self.get_option("--" + key)

                  if (option.help.startswith("[%s]" % section) and

                      getattr(options, option.dest) == None):

                     option.process("", val, self.values, self)

         self._cache = (options, args)

      return self._cache



   def get(self, item):

      if self._cache is None:

         self.parse_args()

      return getattr(self._cache[0], item)



   def set(self, item, val):

      if self._cache is None:

         self.parse_args()

      return setattr(self._cache[0], item, val)



   def reparse_args(self):

      self._cache = None

      return self.parse_args()

      

__all__ = """

make_option ConfigOptionParser

""".split()
