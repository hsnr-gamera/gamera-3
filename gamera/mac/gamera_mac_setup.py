# vi:set tabsize=3:
#
# Copyright (C) 2001-2004 Ichiro Fujinaga, Michael Droettboom,
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

from distutils.core import Command
from distutils.util import get_platform
from distutils.sysconfig import get_python_lib
from distutils import log
from distutils.dir_util import remove_tree, create_tree, copy_tree
from distutils.file_util import copy_file
from os.path import join
import os
import glob
import commands
import datetime
def _run_command(exc, line):
    print line
    try:
        try:
            status, output = commands.getstatusoutput(line)
        except:
            raise IOError("Error running %s" % exc)
        if status:
            raise IOError("Error running %s" % exc)
    finally:
        print output
    return output

def run_command(exc, *args):
    line = " ".join([str(x) for x in [exc] + list(args)])
    return _run_command(exc, line)

def run_command_at(dir, exc, *args):
    line = ("cd %s;" % dir) + " ".join([str(x) for x in [exc] + list(args)])
    return _run_command(exc, line)

class bdist_osx(Command):
   description = "create a Mac OS-X Installer.app package by calling out to buildpkg.py"

   user_options = [('bdist-dir=', 'd',
                    "temporary directory for creating the distribution"),
                   ('plat-name=', 'p',
                    "platform name to embed in generated filenames "
                    "(default: %s)" % get_platform()),
                   ('dist-dir=', 'd',
                    "directory to put final built distributions in"),
		   ('nightly','n',"Name output file according to nightly build format")
                   ]
   boolean_options = ['nightly']
   def initialize_options (self):
      self.bdist_dir = None
      self.plat_name = None
      self.keep_temp = 0
      self.dist_dir = None
      self.nightly = 0
      
   def finalize_options (self):
      if self.bdist_dir is None:
         bdist_base = self.get_finalized_command('bdist').bdist_base
         self.bdist_dir = os.path.join(bdist_base, 'pkg')

      self.set_undefined_options('bdist',
                                 ('dist_dir', 'dist_dir'),
                                 ('plat_name', 'plat_name'))
      
   def run (self):
      name = self.distribution.metadata.name
      version = self.distribution.metadata.version
      fullname = "%s-%s" % (name, version)
      description = self.distribution.metadata.description
       
      self.run_command('build')
      
      install = self.reinitialize_command('install', reinit_subcommands=1)
      install.root = self.bdist_dir
      install.warn_dir = 0
      
      log.info("installing to %s" % self.bdist_dir)
      self.run_command('install')

      bin = os.path.join(self.bdist_dir, "usr/bin")
      create_tree(bin, self.distribution.scripts)
      for script in self.distribution.scripts:
         copy_file(script, bin)

      if not os.path.exists(self.dist_dir):
         os.mkdir(self.dist_dir)
      pkg_dir = os.path.join(self.dist_dir, "pkg")
      if not os.path.exists(pkg_dir):
          os.mkdir(pkg_dir)

      pkg = os.path.join(pkg_dir, fullname + ".pkg")
      if os.path.exists(pkg):
         remove_tree(pkg)

      copy_file('README', 'gamera/mac/resources/ReadMe.txt')
      copy_file('LICENSE', 'gamera/mac/resources/License.txt')

      import buildpkg
      import __version__
      log.info("Building %s.pkg..." % fullname)
      pm = buildpkg.PackageMaker(fullname, version, description)
      pm.build(self.bdist_dir, "gamera/mac/resources",
               DefaultLocation="/",
               Relocatable='NO',
               NeedsAuthorization='YES',
               UseUserMask='YES',
               OutputDir=pkg_dir)

      remove_tree(self.bdist_dir)

      removals = [os.path.join(self.dist_dir, fullname + ".dmg")]

      for removal in removals:
          if os.path.exists(removal):
              os.remove(removal)

      dmg_dir = join(self.dist_dir, 'pkg')
      dmg_dir_gamera = join(dmg_dir, 'Gamera')

      copy_tree('gamera/mac/gameraclick', join(dmg_dir, 'Gamera'))

      readmes = ['README', 'readme.txt', 'LICENSE', 'ACKNOWLEDGEMENTS']
      for readme in readmes:
          if os.path.exists(readme):
              copy_file(readme, dmg_dir_gamera)

      # dmg background image     
      copy_tree('gamera/mac/dmg_images', join(dmg_dir, '.images'))
      # wxPython link
      copy_file('gamera/mac/wxPython.html', join(dmg_dir, 'wxPython Build on Sourceforge.html'))
      imagename = "%s.osx.dmg" % fullname
      if self.nightly:
         d = datetime.date.today()
         monthstring = str(d.month)
         daystring = str(d.day)
         if d.month < 10:
            monthstring = '0' + monthstring
         if d.day < 10:
            daystring = '0' + daystring
         imagename = "gamera-2-nightly-%s%s%s.osx.dmg" % (d.year,monthstring,daystring)
      
      log.info("Making %s..." % imagename)
      # Make a read/write DMG
      output = run_command_at(self.dist_dir, "hdiutil", "create", "-format", "UDRW", "-fs", "HFS+", "-volname", "Gamera", "-srcfolder", "pkg", "temp.dmg")
      # Mount it
      output = run_command_at(self.dist_dir, "hdiutil", "mount", "temp.dmg")
      # Change the DS Store so the background image and icon sizes will be fixed
      copy_file('gamera/mac/dmg_ds_store', join('/Volumes', 'Gamera', '.DS_Store'))
      # Unmount it
      output = run_command("hdiutil unmount /Volumes/Gamera")
      # Make it read only
      
      output = run_command_at(self.dist_dir, "hdiutil", "convert", "-format", "UDRO", "-o", imagename, "temp.dmg")
      # Internet Enable it (why I can do this read only, but I can't do the background, I dunno)
      output = run_command_at(self.dist_dir, "hdiutil internet-enable -no", imagename)
      # Delete the temporary image
      os.remove(join(self.dist_dir, "temp.dmg"))
      remove_tree(pkg_dir)
      
