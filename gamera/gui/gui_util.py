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

from wxPython.wx import *        # wxPython

colors = (wxColor(0xbc, 0x2d, 0x2d), wxColor(0xb4, 0x2d, 0xbc),
          wxColor(0x2d, 0x34, 0xbc), wxColor(0x2d, 0xbc, 0x2d),
          wxColor(0x2d, 0xbc, 0xbc), wxColor(0xbc, 0xb7, 0x2d),
          wxColor(0xbc, 0x88, 0x2d), wxColor(0xaa, 0xaa, 0xaa))

def get_color(number):
   return colors[number & 0x7]

# Displays a message box
def message(message):
   dlg = wxMessageDialog(None, message, "Message",
                         wxOK | wxICON_INFORMATION)
   dlg.ShowModal()
   dlg.Destroy()
