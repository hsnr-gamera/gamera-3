Introduction
------------
    In this folder you will find createmsi, a small application that was written to ease the creation of Gamera MSI
installation packages featuring multiple compilers.  This application makes use of the following software:

cygwin (optional--needed for the mingw compiler)
    http://www.cygwin.com
Microsoft Visual Studio (optional--needed for MSOC compiler)
    http://msdn.microsoft.com/vstudio/productinfo/trial/default.aspx
Intel Compiler Library 8.0 (optional--note that support for this is very experimental at this time)
    http://www.intel.com/software/products/compilers/cwin/
MakeMSI by Dennis Bareis (required--needed for building the MSI Installer Packages)
    http://www.labyrinth.net.au/~dbareis/makemsi.htm
Microsoft Platform SDK (for low level MSI creation utilities on which MakeMSI depends)
    http://www.microsoft.com/msdownload/platformsdk/sdkupdate/
Please note that while each compiler individually is optional, you will need at least one to be able to compile gamera.
Also, ICL does not have a complete set of include files, so some form of the Microsoft compiler will be needed.


If you are looking for a more straightforward approach to making packages with only one version of the C++ extensions
(suitable for most needs), installing the Platform SDK and MakeMSI and then issuing the bdist_msi command to setup.py is
recommended.  However, if you wish to include multiple compiled (target optimized) binaries in one installer, read on.

Instructions
------------
    To get started, point your favorite command line shell to the directory in which you found this readme.  Then, you will
need to compile the createmsi application using the compiler(s) you have:
For mingw:
cc -mno-cygwin -o createmsi.exe createmsi.c
For MSOC:
cl createmsi.c
For ICL:
icl createmsi.c

    Please note that you might have to set environment variables in order to get each compiler to work properly.  If you are
uncomfortable with doing so, each of the aforementioned compilers has its own specialized build environment shell in
which all the necessary environment variables will be set for you.

    Once you have done this, you should see a file called createmsi.exe in the folder.  Now you will be able to create the
MSI.  createmsi.exe takes the following arguments:

createmsi -[c|compiler|i|include] <compilername>

     Note that c and compiler are interchangeable, as are i and include.  When you use the c or compiler flag, you are
instructing createmsi to do a clean rebuild using that compiler.  If instead, you want to merely use the existing build
products (assuming they exist), you would use i or include.  Please keep in mind that your output MSI will be incorrect
if you do not ensure that the build products are valid (check in .../gamera/dist/[compilername]).  Also note that passing
/c is equivalent to passing -c, for those who are more comfortable with that syntax.  You may specify one, two, or all
three compilers.  The choices are:

mingw
msvc
icl

    For each compiler, a feature will be created in the resulting MSI, allowing you to distribute packages built with
several compilers without having to distribute them in separate files.  Please note that this multiple compiler support
still very much a work in progress, so your mileage may vary.
    If you are planning on using ICL for your builds, you will find that Python 2.3.x's distutils does not have built-in
support for it.  Gamera includes a file called iclcompiler.py that automatically registers itself with distutils.  This
file will facilitate ICL support, but such support for Gamera is currently experimental.  Please report any problems to
the mailing list and I will attempt to remedy them as soon as possible.

Examples:
createmsi -c msvc		Rebuilds Gamera with MSOC to build a single-featured MSI
createmsi -c msvc -i mingw	Rebuilds Gamera with MSOC; builds MSI out of new MSOC products and existing mingw products
createmsi /c msvc		Same as the first example
createmsi /i icl		Uses the existing ICL build products to build a single-featured MSI

This process takes about 2 minutes when compiling with MSOC on a Pentium M 1.6 GHz machine.  Afterwards, you will find the
installer in the source tree's win32 directory (by default, the parent directory of the one containing this readme), named
Gamera2.msi.

Known Issues
------------
1) There is currently no mutual exclusivity in the installer GUI when including multiple compiler packages.  It will be
   necessary to specify the "Entire feature will be unavailable" option for those that are not desired.
2) Disk space requirements are not correctly reported in the Installer for the individual features.
3) Error handling is still very rough at this point, so it's best to keep a watchful eye on the build process for intermediate
   failures in the build (at least until you know the process works).  These will currently not be caught automatically (but
   usually will result in a terminal error anyway).
4) ICL support is sketchy at this point.  We are actively working on the distutils plugin included in this package, but it is
   not known to be entirely stable as of yet.

Planned Enhancements
--------------------
1) Address the known issues ASAP
2) Develop a way to easily pass different compiler arguments to facilitate machine-targeted optimized compiles
3) Attempt support for the free Microsoft Visual C++ 2003 Toolkit without requiring distutils modification
4) Possibly support ICL 7.x when 8.0 support is considered fully working.

Reporting Bugs
--------------
Please feel free to report any issues you find with this toolset (as I'm sure there will be quite a few) on the
Gamera mailing list http://groups.yahoo.com/group/gamera-devel

Files
-----
In this folder you should find the following files:

createmsi.c:
Source for createmsi app

DKC.mmh, DKCCompany.mmh, Gamera2.ver, Gamera.mmh, GameraUI.mmh:
Files to specify the general behavior of the generated MSI installer within the MakeMSI system.

Gamera2.MM.template:
File that is used as a basis for building the MSI installer within the MakeMSI system.

Gamera.py:
Script to be installed in Program Menu.

LeftSide.bmp, Turtle.jpg:
Artwork for the installer UI.

License.txt:
Placeholder license file for installer.

README.txt:
This readme file.


Version History
----------------

1.00 Initial Release (05-07-04)