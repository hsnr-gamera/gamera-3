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

from plugin import *
from gamera import *
import _background_surface_threshold

class background_surface_threshold(PluginFunction):
    category = "Threshold"
    self_type = Image(["Float", "GreyScale", "Grey16"])

    def __call__(self, window_size=20, secondary_window_size=23):
        new_m = Matrix(self.nrows(), self.ncols(), "OneBit")
        self._background_surface_threshold(self.m, new_m.m, window_size, secondary_window_size)
        del self.m
        del self.m_data
        self.m = new_m.m
        self.m_data = new_m.m_data
        self.set_type("OneBit")
        if self._display:
            self.display()

plugins = [background_surface_threshold]
