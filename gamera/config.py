#
# Copyright (C) 2001 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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

import string, sys, os, urllib, imp

options = {}

def get_options_by_prefix(option=''):
   result = {}
   for key, val in options.iteritems():
      if key.startswith(option):
         result[key[len(option):]] = val
   return result

def get_option(option):
   if options.has_key(option):
      return options[option]
   else:
      return None

def add_option(option, value=''):
   global options
   if value == '':
      value = 1
   options[option] = value

def add_option_default(option, value):
   global options
   if not options.has_key(option):
      options[option] = value

def parse_file(file):
   if string.find(file, ":") == -1:
      file = "file:" + file
   try:
      filename, header = urllib.urlretrieve(file)
   except:
      print "Could not load config file:", file
      return
   fd = open(filename, 'r')
   try:
      opt = imp.load_source("opt", file, fd)
   except SyntaxError:
      print "Syntax error in config file: ", file
      return
   except ImportError:
      print "Error importing file: ", file
      return
   for x in dir(opt):
      if not x.startswith("__"):
         add_option(x, opt.__dict__[x])
   fd.close()
   del opt

def parse():
   user_local = os.path.expanduser("~/.gamera")
   parse_file(user_local)

   for arg in sys.argv:
      if arg.startswith('--'):
         option, value = map(string.strip, string.split(arg[2:], "="))
         add_option(option, value)
      elif arg.startswith('-'):
         option, value = map(string.strip, string.split(arg[1:], "="))
         add_option(option, value)

   if get_option("configfile") != None:
      parse_file(get_option("configfile"))
