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

# GROUPING IN GENERAL:
#
# The grouping classifier interface allows glyphs to be grouped according to
# their physical relationships to each other.  The user classifies a set of
# glyphs as a "group", and other glyphs matching this general structure can
# later be located on the page.
#
# For documentation, there are two kinds of glyphs involved in this process:
#   part glyphs -- glyphs that should be grouped to form larger glyphs
#   union glyphs -- glyphs made up of part glyphs

class GridIndex:
    """Indexes glyphs using a grid, so glyphs near a given glyph are easier
    to find."""
    def __init__(self, glyphs, max_width=100, max_height=100):
        """Creates a grid index to store the given set of glyphs.  Note that
        the init function only creates a grid big enough to hold the glyphs,
        it does not actually store them...  That must be done by calling
        GridIndex.add_glyph.  max_width and max_height are the maximum size
        (in pixels) of each cell."""
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
        self._create_cells()

    def _create_cells(self):
        self.grid = []
        for i in range(self.cell_ncols * self.cell_nrows):
            self.grid.append([])

    def add_glyph(self, glyph):
        row = int((glyph.center_y - grid_offset_y) / grid_nrows)
        col = int((glyph.center_x - grid_offset_x) / grid_ncols)
        if row < 0 or row >= self.cell_nrows or col < 0 or col >= self.cell_ncols:
            raise ValueError("glyph is not within the bounding box of the initial set of images")
        self.grid[row * self.cell_ncols + col].append(glyph)

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
            if ri < 0 or ri >= self.cell_nrows or ci < 0 or ci >= self.cell_ncols:
                continue
            for glyph in self.grid[ri * self.cell_ncols + ci]:
                yield glyph
    search_order = ((0,0),                          # center
                    (-1,0), (0,-1), (1,0), (0,1),   # +
                    (-1,-1), (-1,1), (1,-1), (1,1)) # x

class GridIndexAndDict(GridIndex):
    """Extends the basic GridIndex class to allow glyphs to also be stored and
    retreived by an arbitrary key."""
    
    def _create_cells(self):
        self.flat = {}
        self.grid = []
        for i in range(self.cell_ncols * self.cell_nrows):
            self.grid.append({})

    def add_glyph_by_key(self, glyph, key):
        row = int((glyph.center_y - grid_offset_y) / grid_nrows)
        col = int((glyph.center_x - grid_offset_x) / grid_ncols)
        if row < 0 or row >= self.cell_nrows or col < 0 or col >= self.cell_ncols:
            raise ValueError("glyph is not within the bounding box of the initial set of images")
        cell_index = row * self.cell_ncols + col
        if not self.grid[cell_index].has_key(key):
            self.grid[cell_index][key] = []
        self.grid[cell_index][key].append(glyph)
        if not self.flat.has_key(key):
            self.flat[key] = []
        self.flat[key].append(glyph)

    def get_cell_by_key(self, row, col, key):
        if row < 0 or row >= self.cell_nrows:
            return []
        if col < 0 or col >= self.cell_ncols:
            return []
        return self.grid[row * self.cell_ncols + col].get(key, [])

    def get_cell_at_glyph_by_key(self, row, col, key):
        row = int((glyph.center_y - self.grid_offset_y) / self.grid_nrows)
        col = int((glyph.center_x - self.grid_offset_x) / self.grid_ncols)
        return self.grid[row * self.cell_ncols + col].get(key, [])

    def get_glyphs_around_glyph(self, glyph):
        row = int((glyph.center_y - self.grid_offset_y) / self.grid_nrows)
        col = int((glyph.center_x - self.grid_offset_x) / self.grid_ncols)
        for r, c in self.search_order:
            ri = r + row
            ci = c + col
            if ri < 0 or ri >= self.cell_nrows or ci < 0 or ci >= self.cell_ncols:
                continue
            for mapping in self.grid[ri * self.cell_ncols + ci]:
                for glyph in mapping.itervalues():
                    yield glyph

    def get_glyphs_around_glyph_by_key(self, glyph, key):
        row = int((glyph.center_y - self.grid_offset_y) / self.grid_nrows)
        col = int((glyph.center_x - self.grid_offset_x) / self.grid_ncols)
        for r, c in self.search_order:
            ri = r + row
            ci = c + col
            if ri < 0 or ri >= self.cell_nrows or ci < 0 or ci >= self.cell_ncols:
                continue
            for glyph in self.grid[ri * self.cell_ncols + ci].get(key, []):
                yield glyph
    search_order = ((0,0),                          # center
                    (-1,0), (0,-1), (1,0), (0,1),   # +
                    (-1,-1), (-1,1), (1,-1), (1,1)) # x

    def get_glyphs_by_key(self, key):
        return self.flat.get(key, [])
          
class Group:
    def __init__(self, id, glyphs):
        self.id = id
        self.glyphs = glyphs

class GroupingClassifier:
    """Base class of all grouping classifiers.  This is an actual
       concrete class that can be used to store grouping information,
       but it does not actually do any grouping."""
    _group_regex = re.compile("group\..*")
    _part_regex = re.compile("group\.part\..*")
    _union_regex = re.compile("group\.(?!part).*")
    
    def __init__(self, groups=[], main_classifier=None):
        # main_classifier points to the classifier in which the
        # grouping classifier resides
        self.set_parent_classifier(main_classifier)
        self.set_groups(groups)

    def set_parent_classifier(self, classifier):
        self.main_classifier = classifier

    ########################################
    # GET/SET GROUPS

    def get_groups(self):
        """Returns a list of Group objects, representing all of the groups
        in the classifier."""
        return self.groups

    def set_groups(self, groups):
        """Clears the currently stored groups and replaces them with the given
        list of groups."""
        self.clear_groups()
        for id, group in groups:
            self.classify_group_manual(group, id)

    def merge_groups(self, groups):
        """Adds the given list of groups to the currently stored set of groups."""
        self.classify_groups_manual(groups)
        for id, group in groups:
            self.classify_group_manual(group, id)

    def clear_groups(self):
        """Clears all currently stored groups."""
        self.groups = []

    ########################################
    # MANUAL CLASSIFICATION

    def _reclassify_groups_manual(self, group, id):
        """This function should only be called using a list of union glyphs.
        It changes the classification name on the union glyph, so that the
        user can override an incorrect automatic grouping."""
        for glyph in group:
            self.remove_groups_containing(glyph)
        return [], group

    def remove_groups_containing(self, glyph):
        """Remove any groups that involve the given glyph.  The given glyph
        may by a part glyph OR a union glyph."""
        remove = []
        for group in self.groups:
            for g in group.glyphs:
                if glyph is g:
                    remove.append(group)
                else:
                    for child in g.children_images:
                        if glyph is child:
                            remove.append(group)
                            break
        removed_glyphs = {}
        for group in remove:
            self.groups.remove(group)
            for glyph in group:
                removed_glyphs[glyph] = None
                for child in glyph.children_images:
                    removed_glyphs[glyph] = None
        return removed_glyphs.keys()

    def classify_group_manual(self, group, id):
        """Creates a new group made up of the given list of glyphs.  Returns
        two lists of glyphs that should be added or removed to the current
        database due to this operation."""
        all_groups = 1
        for glyph in group:
            if not (glyph.match_id_name(self._union_regex)):
                all_groups = 0
                break
        if len(group) == 1 or all_groups:
            return self._reclassify_groups_manual(group, id)
        import image_utilities
        union = image_utilities.union_images(group)
        self.groups.append(Group(id, group))
        union.classify_heuristic('group.' + id)
        part_name = 'group.part.' + id
        for glyph in group:
            glyph.classify_heuristic(part_name)
        return [union], []  # added, removed

    def change_feature_set(self):
        pass

    def search(self, glyphs):
        """Performs a search over the given list of glyphs for structures that
        match the known groups.  Returns two lists of glyphs that should be added
        or removed to the current database due to this operation."""
        return [], []  # added, removed

    def noninteractive_copy(self):
        return self
