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

from __future__ import generators
import sys, re

class GridIndex:
    def __init__(self, glyphs, max_width=100, max_height=100):
        min_x = min_y = sys.maxint
        max_x = max_y = 0
        for glyph in glyphs:
            max_x = max(max_x, glyph.lr_x)
            max_y = max(max_y, glyph.lr_y)
            min_x = min(min_x, glyph.ul_x)
            min_y = min(min_y, glyph.ul_y)
        cell_width = self.cell_width = int(max_width)
        cell_height = self.cell_height = int(max_height)
        grid_ncols = self.grid_ncols = max_x - min_x
        grid_nrows = self.grid_nrows = max_y - min_y
        cell_ncols = self.cell_ncols = int(grid_ncols / max_width) + 1
        cell_nrows = self.cell_nrows = int(grid_nrows / max_height) + 1
        grid_offset_x = self.grid_offset_x = min_x
        grid_offset_y = self.grid_offset_y = min_y
        self.grid = []
        for i in range(cell_ncols * cell_nrows):
            self.grid.append([])
        for glyph in glyphs:
            row = int((glyph.center_y - grid_offset_y) / grid_nrows)
            col = int((glyph.center_x - grid_offset_x) / grid_ncols)
            glyph.properties.grid_row = row
            glyph.properties.grid_col = col
            self.grid[row * self.cell_ncols + col].append(glyph)
        print cell_nrows, cell_ncols, cell_height, cell_width
        x = 0
        for cell in self.grid:
            x += len(cell)
        print "Average cell size: ", float(x) / float(len(self.grid))

    def get_cell(self, row, col):
        if row < 0 or row >= self.cell_nrows:
            return []
        if col < 0 or col >= self.cell_ncols:
            return []
        return self.grid[row * self.cell_ncols + col]

    def get_cell_at_glyph(self, glyph):
        row = int((glyph.center_y - self.grid_offset_y) / self.grid_nrows)
        col = int((glyph.center_x - self.grid_offset_x) / self.grid_ncols)
        return self.get_cell(row, col)

    def get_glyphs_around_glyph(self, glyph):
        row = int((glyph.center_y - self.grid_offset_y) / self.grid_nrows)
        col = int((glyph.center_x - self.grid_offset_x) / self.grid_ncols)
        for r, c in self.search_order:
            ri = r + row
            ci = c + col
            if ri < 0 or ri >= self.cell_nrows:
                continue
            if ci < 0 or ci >= self.cell_ncols:
                continue
            for glyph in self.grid[ri * self.cell_ncols + ci]:
                yield glyph

    search_order = ((0,0),                          # center
                    (-1,0), (0,-1), (1,0), (0,1),   # +
                    (-1,-1), (-1,1), (1,-1), (1,1)) # x
                
class Group:
    def __init__(self, id, glyphs):
        self.id = id
        self.glyphs = glyphs

# TODO: Better define this API
class GroupingClassifier:
    """Base class of all grouping classifiers.  The API defined here
       must be filled out in the child class."""
    _group_regex = re.compile("group\..*")
    _part_regex = re.compile("group\.part\..*")
    _union_regex = re.compile("group\.(?!part).*")
    
    def __init__(self, groups=[], main_classifier=None):
        # The main_classifier points to the classifier in which the
        # grouping classifier resides
        self.main_classifier = main_classifier
        self.set_groups(groups)

    def get_groups(self):
        return self.groups

    def set_groups(self, groups):
        self.clear_groups()
        for id, group in groups:
            self.classify_group_manual(group, id)

    def merge_groups(self, groups):
        self.classify_groups_manual(groups)
        for id, group in groups:
            self.classify_group_manual(group, id)

    def clear_groups(self):
        self.groups = []

    def classify_group_manual(self, group, id):
        return [], []  # added, removed

    def remove_groups_containing(self, glyph):
        pass
    
    def search(self, glyphs):
        return [], []  # added, removed
