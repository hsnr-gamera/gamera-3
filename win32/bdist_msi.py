"""distutils.command.bdist_wininst

Implements the Distutils 'bdist_msi' command: create a windows installer
exe-program."""

# This module should be kept compatible with Python 1.5.2.

__revision__ = "$Id$"

import sys, os, string
from distutils.core import Command
from distutils.util import get_platform
from distutils.dir_util import create_tree, remove_tree
from distutils.errors import *
from distutils.sysconfig import get_python_version
from distutils import log

class bdist_msi (Command):

    description = "create an executable MSI-based installer for MS Windows"

    user_options = [('bdist-dir=', None,
                     "temporary directory for creating the distribution"),
                    ('keep-temp', 'k',
                     "keep the pseudo-installation tree around after " +
                     "creating the distribution archive"),
                    ('target-version=', 'v',
                     "require a specific python version" +
                     " on the target system"),
                    ('no-target-compile', 'c',
                     "do not compile .py to .pyc on the target system"),
                    ('no-target-optimize', 'o',
                     "do not compile .py to .pyo (optimized)"
                     "on the target system"),
                    ('dist-dir=', 'd',
                     "directory to put final built distributions in"),
                    ('bitmap=', 'b',
                     "bitmap to use for the installer instead of python-powered logo"),
                    ('title=', 't',
                     "title to display on the installer background instead of default"),
                    ('skip-build', None,
                     "skip rebuilding everything (for testing/debugging)"),
                    ('install-script=', None,
                     "basename of installation script to be run after"
                     "installation or before deinstallation"),
                   ]

    boolean_options = ['keep-temp', 'no-target-compile', 'no-target-optimize',
                       'skip-build']

    def initialize_options (self):
        self.bdist_dir = None
        self.keep_temp = 0
        self.no_target_compile = 0
        self.no_target_optimize = 0
        self.target_version = None
        self.dist_dir = None
        self.bitmap = None
        self.title = None
        self.skip_build = 0
        self.install_script = None

    # initialize_options()


    def finalize_options (self):
        if self.bdist_dir is None:
            bdist_base = self.get_finalized_command('bdist').bdist_base
            self.bdist_dir = os.path.join(bdist_base, 'msi')
        if not self.target_version:
            self.target_version = ""
        if self.distribution.has_ext_modules():
            short_version = get_python_version()
            if self.target_version and self.target_version != short_version:
                raise DistutilsOptionError, \
                      "target version can only be" + short_version
            self.target_version = short_version

        self.set_undefined_options('bdist', ('dist_dir', 'dist_dir'))

        if self.install_script:
            for script in self.distribution.scripts:
                if self.install_script == os.path.basename(script):
                    break
            else:
                raise DistutilsOptionError, \
                      "install_script '%s' not found in scripts" % \
                      self.install_script
    # finalize_options()


    def run (self):
        if (sys.platform != "win32"):
            raise DistutilsPlatformError \
                  ("MSI must be created on a Win32 host")
        
        if not self.skip_build:
            self.run_command('build')

        install = self.reinitialize_command('install', reinit_subcommands=1)
        install.root = self.bdist_dir
        install.skip_build = self.skip_build
        install.warn_dir = 0
        
        install_lib = self.reinitialize_command('install_lib')

        for key in ('purelib', 'platlib', 'headers', 'scripts', 'data'):
            value = '.'#string.upper(key)
            if key == 'headers':
                value += '/Include/$dist_name'
            if key == 'scripts':
                value += '/scripts'
            if key == 'platlib' or key == 'purelib':
                value += '/Lib/site-packages'
            setattr(install,
                    'install_' + key,
                    value)
        log.info("installing to %s", self.bdist_dir)
        install.ensure_finalized()

        # avoid warning of 'install_lib' about installing
        # into a directory not in sys.path
        sys.path.insert(0, os.path.join(self.bdist_dir, 'PURELIB'))

        install.run()

        del sys.path[0]

        fullname = self.distribution.get_fullname()
        self.create_msi(fullname,self.bitmap)   
    # run()
    
    def get_inidata (self):
        # Return data describing the installation.

        lines = []
        metadata = self.distribution.metadata

        # Write the [metadata] section.  Values are written with
        # repr()[1:-1], so they do not contain unprintable characters, and
        # are not surrounded by quote chars.
        lines.append("[metadata]")

        # 'info' will be displayed in the installer's dialog box,
        # describing the items to be installed.
        info = (metadata.long_description or '') + '\n'

        for name in ["author", "author_email", "description", "maintainer",
                     "maintainer_email", "name", "url", "version"]:
            data = getattr(metadata, name, "")
            if data:
                info = info + ("\n    %s: %s" % \
                               (string.capitalize(name), data))
                lines.append("%s=%s" % (name, repr(data)[1:-1]))

        # The [setup] section contains entries controlling
        # the installer runtime.
        lines.append("\n[Setup]")
        if self.install_script:
            lines.append("install_script=%s" % self.install_script)
        lines.append("info=%s" % repr(info)[1:-1])
        lines.append("target_compile=%d" % (not self.no_target_compile))
        lines.append("target_optimize=%d" % (not self.no_target_optimize))
        if self.target_version:
            lines.append("target_version=%s" % self.target_version)

        title = self.title or self.distribution.get_fullname()
        lines.append("title=%s" % repr(title)[1:-1])
        import time
        import distutils
        build_info = "Built %s with distutils-%s" % \
                     (time.ctime(time.time()), distutils.__version__)
        lines.append("build_info=%s" % build_info)
        return string.join(lines, "\n")

    # get_inidata()

    def create_msi (self, fullname, bitmap=None):
        import struct
        
        self.mkpath(self.dist_dir)

        cfgdata = self.get_inidata()
        
        if self.target_version:
            # if we create an installer for a specific python version,
            # it's better to include this in the name
            installer_name = "%s.win32-py%s" % (fullname, self.target_version)
        else:
            installer_name = "%s.win32" % fullname
        self.announce("creating %s" % installer_name)
        
        mm_file_name = self.create_mm_files(installer_name)
        shortnames = ["/DKC.mmh","/DKCCompany.mmh","/Gamera.mmh", "/GameraUI.mmh", "/turtle.jpg"]
        self.copy_supporting_files(shortnames)
        
        current_dir = os.getcwd()
        os.chdir(self.dist_dir)
        successfully_spawned = 0
        had_error = 0
        for prefix in string.split(os.environ['path'],';'):
            absolute_path = prefix + "\\" 'mm'
            try:
                had_error = os.spawnl(os.P_WAIT,absolute_path)
                if had_error:
                    raise DistutilsPlatformError(
                    "MakeMSI did not complete successfully.  Please double check the MakeMSI configuration scripts and try again.")
                successfully_spawned = 1
                break
            except: pass
        
        if not successfully_spawned:            
            raise DistutilsPlatformError(
                "Could not spawn the MakeMSI script.  Please check that you have properly installed MakeMSI and try again.")

        open(installer_name + ".msi","wb").write(open("out/Gamera2.MM/MSI/" + installer_name + ".msi", "rb").read())
        remove_tree("./out")
        shortnames.append("/Gamera2.GUIDS")
        os.remove("Gamera2.MM")
        os.remove("Gamera2.ver")
        os.remove("License.txt")
        os.chdir(current_dir)
        self.remove_supporting_files(shortnames)
    # create_msi()

    def create_mm_files(self,installer_name):
        file = open(self.dist_dir + "/License.txt", "wb")
        file.write("License file for Gamera")
        
        mm_name = self.dist_dir + "/Gamera2.MM"
        file = open(mm_name, "wb")
        contents = """
#include "Gamera.MMH"
#define+ MmMode P

<$DirectoryTree Key="PY23LOC" Dir="a:\\">

#(
   <$RegistryRead
       Property="PY23LOC"
       HKEY="LOCAL_MACHINE"
       Key="Software\\Python\\PythonCore\\2.3\\InstallPath"
       Condition="not Installed"
       Message="Installation cannot continue, since Python 2.3 was not found on this computer.  Please download the Python 2.3 installer from www.python.org and try again."
   >
#)

#(
   <$RegistryRead
       Property="WXPY24"
       HKEY="LOCAL_MACHINE"
       Key="Software\\Python\\PythonCore\\2.3\\Modules\\wxPython"
       Condition="not Installed"
       Message="Installation cannot continue, since wxPython 2.4 was not found on this computer.  Please download the wxPython 2.4 installer from www.wxpython.org and try again."
   >
#)

<$DirectoryTree Key="PROGMENU"  Dir="[ProgramMenuFolder]\\Gamera2">

<$Component "shortcut" Directory_="[PROGMENU]">
        <$Files "..\\win32\\msisource\\Gamera.py" DestDir="[PROGMENU]">
<$/Component>

<$Files "%s" SubDir=TREE DestDir="[PY23LOC]">

""" % string.join(["..\\",self.bdist_dir,"\\*"],'')
        file.write(contents)
        
        ver_name = self.dist_dir + "/Gamera2.ver"
        file = open(ver_name, "wb")
        import datetime
        curr_date = datetime.date.today()
        contents = """
;----------------------------------------------------------------------------
;
;    MODULE NAME:   Gamera2.VER
;
;        $Author$
;      $Revision$
;          $Date$
;       $Logfile:   ./Gamera2.ver.pvcs  $
;      COPYRIGHT:   (C)opyright Dennis Bareis, Australia, 2003
;                   All rights reserved.
;
;    DESCRIPTION:   Simple sample/test MSI.
;
;++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; ProductName = Gamera 2
; DESCRIPTION = This is the installer for Gamera 2
; Licence     = License.txt
; Installed   = WINDOWS_ALL
; MsiName     = %s
;++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



;############################################################################
VERSION : 2.20.01
DATE    : %s
CHANGES : MSI Release for %s """ % (installer_name, curr_date.ctime(), self.distribution.get_fullname())
        file.write(contents)
        return mm_name
    #create_mm_files()

    def copy_supporting_files(self, shortnames):
        for name in shortnames:
            open(self.dist_dir + name, "wb").write(open(self.dist_dir + "/../win32/msisource" + name,"rb").read())
    #copy_supporting_files()

    def remove_supporting_files(self,shortnames):
        for name in shortnames:
            os.remove(self.dist_dir + name)
    #remove_supporting_files()

# class bdist_wininst
