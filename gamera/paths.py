#
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

import os, sys  # Python standard library

if 1:
  def dummy():
    pass

lib = os.path.dirname(os.path.abspath(dummy.func_code.co_filename))
# Figure out if we are in the source directory or installed
if os.path.exists(lib + "/../plugins"):
  plugins = os.path.abspath(lib + "/../plugins")
  plugins_src = os.path.abspath(lib + "/../src/gamera/plugins")
  toolkits = os.path.abspath(lib + "/../toolkits")
  sys.path.extend([lib, plugins])
  pixmaps = os.path.abspath(lib + "/../pixmaps") + "/"
  doc = os.path.abspath(lib + "/../doc") + "/"
  test = os.path.abspath(lib + "/../test")
  test_results = os.path.abspath(lib + "/../test/results")
else:
  plugins = os.path.abspath(lib + "/plugins")
  plugins_src = ""
  toolkits = os.path.abspath(lib + "/toolkits")
  sys.path.extend([lib, plugins])
  doc = os.path.abspath(lib + "/doc") + "/"
  test = os.path.abspath(lib + "/../test")
  test_results = os.path.abspath(lib + "/../test/results")
