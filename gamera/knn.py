#
# Copyright (C) 2001, 2002 Ichiro Fujinaga, Michael Droettboom,
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

import gamera.knncore

from gamera.knncore import CITY_BLOCK
from gamera.knncore import EUCLIDEAN
from gamera.knncore import FAST_EUCLIDEAN

class kNN(gamera.knncore.kNN):
    def __init__(self):
        gamera.knncore.kNN.__init__(self)

    def evaluate(self):
        """Evaluate the performance of the kNN classifier using
        leave-one-out cross-validation. The return value is a
        floating-point number between 0.0 and 1.0"""
        return self.leave_one_out()

    def supports_interactive():
        """Flag indicating that this classifier supports interactive
        classification."""
        return 1

    def supports_optimization():
        """Flag indicating that this classifier supports optimization."""
        return 1
