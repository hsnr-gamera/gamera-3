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
from distutils.dir_util import remove_tree, create_tree
from distutils.file_util import copy_file
import os
import glob

class bdist_osx(Command):
   description = "create a Mac OS-X Installer.app package by calling out to buildpkg.py"

   user_options = [('bdist-dir=', 'd',
                    "temporary directory for creating the distribution"),
                   ('plat-name=', 'p',
                    "platform name to embed in generated filenames "
                    "(default: %s)" % get_platform()),
                   ('dist-dir=', 'd',
                    "directory to put final built distributions in")
                   ]
   
   def initialize_options (self):
      self.bdist_dir = None
      self.plat_name = None
      self.keep_temp = 0
      self.dist_dir = None
      
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

      readmes = ['README', 'readme.txt', 'LICENSE', 'ACKNOWLEDGEMENTS']
      for readme in readmes:
          if os.path.exists(readme):
              copy_file(readme, pkg_dir)

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
               RootVolumeOnly='YES',
               OutputDir=pkg_dir)

      remove_tree(self.bdist_dir)


      removals = [os.path.join(self.dist_dir, fullname + ".dmg"),
                  os.path.join(self.dist_dir, "_%s.dmg" % fullname)]
      for removal in removals:
          if os.path.exists(removal):
              os.remove(removal)

      import makedmg
      log.info("Making %s.dmg..." % fullname)
      makedmg.make_dmg(pkg_dir, self.dist_dir, fullname)

      remove_tree(pkg_dir)
      os.remove(removals[1])
