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

import sys, os, atexit
import util

system_file = os.path.expanduser("~/.gamera_system")
user_file = os.path.expanduser("~/.gamera_user")

class Options(dict):
   def __getitem__(self, key):
      return self.setdefault(key, Namespace(key))
   __getattr__ = __getitem__

class Namespace(dict):
   def __init__(self, name):
      dict.__init__(self)
      self._name = name
   def __getitem__(self, key):
      return self.setdefault(key, Option(key))
   def __getattr__(self, key):
      return self.setdefault(key, Option(key)).get()
   def __setattr__(self, key, val):
      if key == '_name':
         object.__setattr__(self, key, val)
      else:
         self.setdefault(key, Option(key)).set(val)

class Option:
   def __init__(self, name):
      self._name = name
      self._defined = 0
      self._value = []
      self._default = ''
      self._type = str
      self._help = 'Undefined option'
      self.system = 0

   def __repr__(self):
      return str(self.get())
   
   def define(self, default='', help='', system=0):
      self._default = default
      self._type = type(default)
      self._help = help
      self.system = system
      self._defined = 1

   def get(self):
      if self._value == [] and self._defined:
         return self._default
      if self._type == list:
         return self._value
      else:
         return self._value[0]

   def set(self, val):
      if self._defined and self._type != list:
         try:
            val = self._type(val)
         except Exception, e:
            print "Error setting option %s to %s\n%s" % (self._name, str(val), str(e))
      if val not in self._value:
         self._value.insert(0, val)

   def set_all(self, val):
      if not self._defined or self._type == list:
         self._value = val
      else:
         try:
            self._value = [self._type[v] for v in val]
         except Exception, e:
            print "Error setting option %s to %s\n%s" % (self._name, str(val), str(e))

   def get_help(self):
      return self._help

   def get_type(self):
      return self._type

   def get_name(self):
      return self._name

   def get_default(self):
      return self._default

   def is_defined(self):
      return self._defined

options = Options()

def define_option(namespace, name, default='', help='', system=0):
   options[namespace][name].define(default, help, system)

def get_pair(line):
   line = line.strip()
   if '=' in line:
      parts = [x.strip() for x in line.split('=')]
      if len(parts) > 2:
         parts = [parts[0], '='.join(parts[1:])]
      return tuple(parts)
   else:
      return (line, 1)

last_category = 'DEFAULT'
def get_triple(line):
   global last_category
   if '.' in line:
      parts = [x.strip() for x in line.split('.')]
      if len(parts) > 2:
         parts = [parts[0], '.'.join(parts[1:])]
      last_category = parts[0]
      line = parts[1]
   return tuple([last_category] + list(get_pair(line)))

def parse_file(filename):
   if not os.path.exists(filename):
      return
   try:
      fd = open(filename, 'r')
      try:
         current_section = "DEFAULT"
         for line in fd.xreadlines():
            line = line.strip()
            if len(line) == 0:
               continue
            if len(line) >= 2 and line[0] == '[' and line[-1] == ']':
               current_section = line[1:-1].strip()
               if current_section == '':
                  current_section = "DEFAULT"
            elif len(line) >= 1 and line[0] == '#' or line[0] == ';':
               pass
            elif '=' in line:
               key, val = get_pair(line)
               if current_section.startswith('__') or key.startswith('__'):
                  continue
               options[current_section][key].set(val)
      finally:
         fd.close()
   except Exception, e:
      print "WARNING: Error loading configuration file %s\n%s" % (filename, str(e))

def parse_all_files():
   parse_file(system_file)
   parse_file(user_file)

def parse_command_line():
   for arg in sys.argv:
      if arg.startswith('--'):
         category, key, val = get_triple(arg[2:])
         options[category][key].set(val)
   help = ('--help', '-h', '?')
   for h in help:
      if h in sys.argv:
         display_help()
         sys.exit(1)

def parse_options():
   parse_all_files()
   parse_command_line()
   atexit.register(save_files)

def write_out(fd, callback, system=1):
   sections = options.items()
   sections.sort()
   start = ''
   for section_name, section in sections:
      if section_name.startswith('__'):
         continue
      section = section.items()
      section.sort()
      opts = []
      for option_name, option in section:
         if (option.system == system and
             not option_name.startswith('__') and
             (system or option.is_defined())):
            opts.append(option)
      if len(opts):
         fd.write(start)
         fd.write("[%s]\n" % section_name)
         start = '\n'
         for option in opts:
            callback(fd, option, section_name)

def ini_file_callback(fd, option, section):
   util.word_wrap(fd, "# %s" % option.get_help())
   if option.get_type() == list:
      vals = option.get()
      vals.reverse()
      for val in vals:
         fd.write("%s = %s\n" % (option.get_name(), str(val)))
   else:
      fd.write("%s = %s\n" % (option.get_name(), str(option.get())))

def help_callback(fd, option, section):
   if option.get_type() != list:
      util.word_wrap(
         fd, '--%s.%s=%s (%s)' %
         (section, option.get_name(),
          str(option.get()), str(option.get_default())),
         1)
      util.word_wrap(fd, option.get_help(), 3)

def display_help():
   print "\nCommandline help:\n"
   write_out(sys.stdout, help_callback, 0)
   print

def save_file(filename, system=1):
   write_out(open(filename, 'w'), ini_file_callback, system)

def save_files():
   if not os.path.exists(user_file):
      save_file(user_file, 0)
   save_file(system_file, 1)

