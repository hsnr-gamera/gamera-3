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

from threading import *

import gamera.knncore

from gamera.knncore import CITY_BLOCK
from gamera.knncore import EUCLIDEAN
from gamera.knncore import FAST_EUCLIDEAN

class GaWorker(Thread):
    def __init__(self, knn):
        Thread.__init__(self)
        self.knn = knn

    def run(self):
        self.ga_initial = self.knn._ga_create()
        self.ga_best = self.ga_initial
        while(1):
            if self.knn.ga_worker_stop:
                return
            self.knn.ga_best = self.knn._ga_step()
            self.knn.ga_generation += 1
            for x in self.knn.ga_callbacks:
                x(self.knn)

class kNN(gamera.knncore.kNN):
    def __init__(self):
        gamera.knncore.kNN.__init__(self)
        self.ga_initial = 0.0
        self.ga_best = 0.0
        self.ga_worker_thread = None
        self.ga_worker_stop = 0
        self.ga_generation = 0
        self.ga_callbacks = []

    def evaluate(self):
        """Evaluate the performance of the kNN classifier using
        leave-one-out cross-validation. The return value is a
        floating-point number between 0.0 and 1.0"""
        return self.leave_one_out()

    def supports_interactive(self):
        """Flag indicating that this classifier supports interactive
        classification."""
        return 1

    def supports_optimization(self):
        """Flag indicating that this classifier supports optimization."""
        return 1

    def start_optimizing(self):
        """Start optizing the classifier using a Genetic Algorithm"""
        self.ga_worker_stop = 0
        self.ga_worker_thread = GaWorker(self)
        self.ga_worker_thread.setDaemon(1)
        self.ga_worker_thread.start()
        return

    def stop_optimizing(self):
        """Stop optimization with the Genetic Algorithm. WARNING: this function
        has to wait for the current GA generation to finish before returning, which
        could take several secongs."""
        if not self.ga_worker_thread:
            return
        self.ga_worker_stop = 1
        self.ga_worker_thread.join()
        self.ga_worker_thread = None
        self._ga_destroy()
        
    def add_optimization_callback(self, func):
        """Add a function to be called everytime time the optimization updates the
        classifier."""
        self.ga_callbacks.append(func)

    def remove_optimization_callback(self, func):
        """Remove a function that is called everytime time the optimization updates the
        classifier."""
        try:
            self.ga_callbacks.remove(func)
        except:
            pass
