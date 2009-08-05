# Utilities to cross-compile for Windows on a Debian box
# with the mingw32 cross-compiler installed.

# This is probably extremely brittle wrt new versions of distutils,
# since all of this is just monkey-patched in.  Here's hoping that
# someday distutils will be replaced.

# To use, specify the mingw32_cross compiler to distutils:
#
#    python setup.py --compiler=mingw32_cross

import os
import sys

if sys.hexversion < 0x2050000:
    print "Cross-compiling has only been tested with Python 2.5.  Your "
    print "results may vary."

# Fool our own scripts that decide what to build
sys.platform = 'cygwin'

from distutils import cygwinccompiler, util, ccompiler
from distutils.command import build_ext, bdist_wininst

if sys.version_info[2] == 0:
    LATEST_PYTHON_RELEASE = "%d.%d" % tuple(sys.version_info[:2])
else:
    LATEST_PYTHON_RELEASE = "%d.%d.%d" % tuple(sys.version_info[:3])
PYTHON_LIB = "python%d%d" % tuple(sys.version_info[:2])
PYTHON_WIN32 = "python-win32-%s" % LATEST_PYTHON_RELEASE

prefix = 'i686-pc-mingw32-'
os.environ['CC'] = '%sgcc' % prefix
os.environ['CXX'] = '%sg++' % prefix
os.environ['LD'] = '%sld' % prefix
os.environ['LDFLAGS'] = '-L./%s/dll/ -l%s' % (
    PYTHON_WIN32, PYTHON_LIB)
os.environ['CFLAGS'] = '-I./%s/' % PYTHON_WIN32
os.environ['CXXFLAGS'] = '-I./%s/' % PYTHON_WIN32

if os.system(os.environ['CC'] + " --version") != 0:
    print "It does not appear that you have the mingw32 cross compiler"
    print "installed.  On Debian use:"
    print "   sudo apt-get install mingw32"
    sys.exit(1)

if not os.path.exists(PYTHON_WIN32) or not os.path.isdir(PYTHON_WIN32):
    os.mkdir(PYTHON_WIN32)
    os.chdir(PYTHON_WIN32)
    if (os.system("wget http://python.org/ftp/python/%s/python-%s.msi" %
                  (LATEST_PYTHON_RELEASE, LATEST_PYTHON_RELEASE)) != 0):
        print "Error getting Python Windows distribution"
        sys.exit(1)
    if (os.system("cabextract python-%s.msi" % LATEST_PYTHON_RELEASE) != 0):
        print "Error extracting Python Windows dist.  Do you have cabextract installed?"
        sys.exit(1)
    os.mkdir("dll")
    os.rename("%s.dll" % PYTHON_LIB, "dll/%s.dll" % PYTHON_LIB)
    os.chdir("..")

def monkey_patch_get_platform():
   return 'win32'
util.get_platform = monkey_patch_get_platform

def monkey_patch_get_versions():
    """ Try to find out the versions of gcc, ld and dllwrap.
        If not possible it returns None for it.
    """
    from distutils.version import StrictVersion
    from distutils.spawn import find_executable
    import re

    gcc_exe = find_executable(os.environ.get('CC') or 'gcc')
    if gcc_exe:
        out = os.popen(gcc_exe + ' -dumpversion','r')
        out_string = out.read()
        out.close()
        result = re.search('(\d+\.\d+(\.\d+)*)',out_string)
        if result:
            gcc_version = StrictVersion(result.group(1))
        else:
            gcc_version = None
    else:
        gcc_version = None
    ld_exe = find_executable(os.environ.get('LD') or 'ld')
    if ld_exe:
        out = os.popen(ld_exe + ' -v','r')
        out_string = out.read()
        out.close()
        result = re.search('(\d+\.\d+(\.\d+)*)',out_string)
        if result:
            ld_version = StrictVersion(result.group(1))
        else:
            ld_version = None
    else:
        ld_version = None
    dllwrap_exe = find_executable(os.environ.get('DLLWRAP') or 'dllwrap')
    if dllwrap_exe:
        out = os.popen(dllwrap_exe + ' --version','r')
        out_string = out.read()
        out.close()
        result = re.search(' (\d+\.\d+(\.\d+)*)',out_string)
        if result:
            dllwrap_version = StrictVersion(result.group(1))
        else:
            dllwrap_version = None
    else:
        dllwrap_version = None
    return (gcc_version, ld_version, dllwrap_version)
cygwinccompiler.get_versions = monkey_patch_get_versions

# the same as cygwin plus some additional parameters
class Mingw32CrossCCompiler (cygwinccompiler.CygwinCCompiler):
    compiler_type = 'mingw32'

    def __init__ (self,
                  verbose=0,
                  dry_run=0,
                  force=0):

        cygwinccompiler.CygwinCCompiler.__init__ (self, verbose, dry_run, force)

        # ld_version >= "2.13" support -shared so use it instead of
        # -mdll -static
        if self.ld_version >= "2.13":
            shared_option = "-shared"
        else:
            shared_option = "-mdll -static"

        # A real mingw32 doesn't need to specify a different entry point,
        # but cygwin 2.91.57 in no-cygwin-mode needs it.
        if self.gcc_version <= "2.91.57":
            entry_point = '--entry _DllMain@12'
        else:
            entry_point = ''

        gcc = os.environ.get('CC') or 'gcc'
        so = os.environ.get('CC') or 'gcc'
        cxx = os.environ.get('CXX') or 'g++'
        linker_exe = os.environ.get('CC') or 'gcc'
        linker_dll = os.environ.get('CC') or 'gcc'
        cflags = os.environ.get('CFLAGS') or ''
        cxxflags = os.environ.get('CXXFLAGS') or ''
        ldflags = os.environ.get('LDFLAGS') or ''

        self.set_executables(compiler='%s -mno-cygwin -O -Wall -static-libgcc %s' % (gcc, cflags),
                             compiler_so='%s -mno-cygwin -mdll -O -Wall -static-libgcc %s' % (so, cflags),
                             compiler_cxx='%s -mno-cygwin -O -Wall -static-libgcc %s' % (cxx, cxxflags),
                             linker_exe='%s -mno-cygwin -static-libgcc' % linker_exe,
                             linker_so='%s -mno-cygwin %s %s %s -static-libgcc'
                                        % (linker_dll, shared_option,
                                           entry_point, ldflags))
        # Maybe we should also append -mthreads, but then the finished
        # dlls need another dll (mingwm10.dll see Mingw32 docs)
        # (-mthreads: Support thread-safe exception handling on `Mingw32')

        # no additional libraries needed
        self.dll_libraries=[]

    # __init__ ()

# class Mingw32CCompiler

cygwinccompiler.Mingw32CCompiler = Mingw32CrossCCompiler

_build_extension = build_ext.build_ext.build_extension
def monkey_patch_build_extension(self, ext):
    _build_extension(self, ext)

    fullname = self.get_ext_fullname(ext.name)
    if self.inplace:
        # ignore build-lib -- put the compiled extension into
        # the source tree along with pure Python modules

        modpath = string.split(fullname, '.')
        package = string.join(modpath[0:-1], '.')
        base = modpath[-1]

        build_py = self.get_finalized_command('build_py')
        package_dir = build_py.get_package_dir(package)
        ext_filename = os.path.join(package_dir,
                                    self.get_ext_filename(base))
    else:
        ext_filename = os.path.join(self.build_lib,
                                    self.get_ext_filename(fullname))

    os.rename(ext_filename, ext_filename[:-2] + "pyd")

build_ext.build_ext.build_extension = monkey_patch_build_extension

_bdist_wininst_run = bdist_wininst.bdist_wininst.run
def monkey_patch_bdist_wininst_run(self):
    real_platform = sys.platform
    real_os_name = os.name
    sys.platform = 'win32'
    os.name = 'nt'
    _bdist_wininst_run(self)
    sys.platform = real_platform
    os.name = real_os_name
bdist_wininst.bdist_wininst.run = monkey_patch_bdist_wininst_run

def get_exe_bytes (self):
    # wininst-x.y.exe is in the same directory as this file
    directory = os.path.dirname(bdist_wininst.__file__)
    # we must use a wininst-x.y.exe built with the same C compiler
    # used for python.  XXX What about mingw, borland, and so on?
    filename = os.path.join(directory, "wininst-9.0.exe")
    return open(filename, "rb").read()
bdist_wininst.bdist_wininst.get_exe_bytes = get_exe_bytes
