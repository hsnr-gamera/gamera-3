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



from backport.config import *



from inspect import getfile

from os.path import split, join, expanduser



class GameraConfigOptionParser(ConfigOptionParser):

   extra_files = []



   def add_file(self, file):

      self.extra_files.append(file)

   

   def get_config_files(self):

      dir = split(getfile(GameraConfigOptionParser))[0]

      return ([join(dir, "gamera.cfg"), join(expanduser("~"), ".gamera")] +

              self.extra_files)



config = GameraConfigOptionParser()



__all__ = ["config"]
