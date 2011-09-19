#
# Copyright (C) 2001-2007 Karl MacMillan, Michael Droettboom,
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#

# Convert Images
#
# This script uses the wxPython img2py tool (included in this directory)
# to convert all of the pngs in the this directory into a python script
# called gamera_icons.py. This script can be copied to the main gamera
# directory to be used by gamera. This is easier to manage than the
# individual files, especially since distutils doesn't handle arbitrary
# data files.

import sys, glob, os
import img2py
import wx

if __name__ == "__main__":
    app = wx.App()

    files = glob.glob('*.png')
    first = 1
    for x in files:
        # This converts filenames in the form of file_name.png
        # into names of the form FileName suitable for method
        # names in the gamera_icons module
        base_name = x.split('.')[0]
        names = base_name.split('_')
        name = ''
        for j in names:
            name += j[0].upper() + j[1:]
        args = []
        # -a means append the image - only done after the first image
        if first == 0:
            args.append('-a')
        else:
            first = 0
        # -n sets the name pattern for the methods to access
        # this particular image
        args.append('-n')
        args.append(name)
        args.append(x)
        args.append('../gui/gamera_icons.py')
        # run the script
        img2py.main(args)
