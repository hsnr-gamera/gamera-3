#!/usr/bin/python
#
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

# This file is a Python port of makedmg by Frank Vercruesse
# found in the wxPython source distribution

import sys
import os.path
import commands

HDI_UTIL_EXEC = "/usr/bin/hdiutil"
HDI_DRV_EXEC = "/usr/bin/hdid"
NEW_FS_EXEC = "/sbin/newfs_hfs"
DU_EXEC = "/usr/bin/du"
DITTO_EXEC = "/usr/bin/ditto"

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

def quote(s):
    return '"%s"' % s

def _make_dmg(src, dst, name):
    if not (os.path.isdir(dst) and os.path.isdir(src)):
        raise IOError("'%s' and '%s' must be directories" % (src, dst))

    output = run_command(DU_EXEC, "-sk", quote(src))
    dmgsize = int(output.split()[0])
    dmgsize /= 1024
    dmgsize = int(dmgsize + 4)
    dmgsize = max(5, dmgsize)

    run_command_at(dst, HDI_UTIL_EXEC, "create", "-megabytes", dmgsize, "-ov", quote('_' + name))

    output = run_command_at(dst, HDI_DRV_EXEC, "-nomount", quote("_%s.dmg" % name))
    dev = output.split()[0]
    for line in output.split("\n"):
        dev, pname = line.split()
        if pname == "Apple_HFS":
            part = dev.split("/")[-1]
            raw = "/dev/r" + part
            break

    try:
        output = run_command_at(dst, NEW_FS_EXEC, "-v", quote(name), raw)
    except IOError:
        run_command(HDI_UTIL_EXEC, "eject", dev)
        raise IOError("Couldn't format disk image.")
    run_command(HDI_UTIL_EXEC, "eject", dev)

    output = run_command_at(dst, HDI_DRV_EXEC, quote("_%s.dmg" % name))
    dev = output.split()[0]
    for line in output.split("\n"):
        parts = line.split()
        if parts[1] == "Apple_HFS":
            vname = parts[2]
            break

    try:
        run_command(DITTO_EXEC, quote(src), quote(vname))
    except IOError:
        run_command(HDI_UTIL_EXEC, "eject", dev)
        raise IOError("couldn't copy files")

    run_command(HDI_UTIL_EXEC, "eject", dev)

    run_command_at(dst, HDI_UTIL_EXEC, "convert", quote("_%s.dmg" % name), "-format", "UDCO", "-o", quote(name))

def make_dmg(src, dst, name):
    try:
        _make_dmg(src, dst, name)
    except IOError, e:
        print e
        sys.exit(1)

__all__ = ["make_dmg"]

if __name__ == "__main__":
    make_dmg(*sys.argv[-3:])
