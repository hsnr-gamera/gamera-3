#
# This file has been taken from wxpython (see the file
# wx/tools/img2img.py in the wxpython source distribution)
#
# Copyright (c) 1998 Julian Smart, Robert Roebling et al
#
# This program may be freely used, copied and distributed under
# the terms of the wxWindows Library Licence, Version 3. See
# the file "copyright" of the wxpython distribution from
# http://wxpython.org/ for details.
#
"""
Common routines for the image converter utilities.
"""
import sys, os, glob, getopt, string
import wx
import compat_wx

if wx.Platform == "__WXGTK__":
    # some bitmap related things need to have a wxApp initialized...
    app = compat_wx.create_app()

wx.InitAllImageHandlers()

def convert(file, maskClr, outputDir, outputName, outType, outExt):
    if string.lower(os.path.splitext(file)[1]) == ".ico":
        icon = wx.Icon(file, wx.BITMAP_TYPE_ICO)
        img = compat_wx.create_bitmap_from_icon(icon)
    else:
        img = wx.Bitmap(file, wx.BITMAP_TYPE_ANY)

    if not compat_wx.is_ok(img):
        return 0, file + " failed to load!"
    else:
        if maskClr:
            om = img.GetMask()
            mask = compat_wx.create_mask(img, maskClr)
            img.SetMask(mask)
            if om is not None:
                om.Destroy()
        if outputName:
            newname = outputName
        else:
            newname = os.path.join(outputDir,
                                   os.path.basename(os.path.splitext(file)[0]) + outExt)
        if img.SaveFile(newname, outType):
            return 1, file + " converted to " + newname
        else:
            img = wx.ImageFromBitmap(img)
            if img.SaveFile(newname, outType):
                return 1, "ok"
            else:
                return 0, file + " failed to save!"




def main(args, outType, outExt, doc):
    if not args or ("-h" in args):
        print doc
        return

    outputDir = ""
    maskClr = None
    outputName = None

    try:
        opts, fileArgs = getopt.getopt(args, "m:n:o:")
    except getopt.GetoptError:
        print __doc__
        return

    for opt, val in opts:
        if opt == "-m":
            maskClr = val
        elif opt == "-n":
            outputName = val
        elif opt == "-o":
            outputDir = val

    if not fileArgs:
        print doc
        return

    for arg in fileArgs:
        for file in glob.glob(arg):
            if not os.path.isfile(file):
                continue
            ok, msg = convert(file, maskClr, outputDir, outputName,
                              outType, outExt)
            print msg

