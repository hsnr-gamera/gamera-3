# -*- mode: python; indent-tabs-mode: nil; tab-width: 3 -*-
# vim: set tabstop=3 shiftwidth=3 expandtab:
#
# Copyright (C) 2001-2005 Karl MacMillan
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

"""
Usage
=====

(as inferred from the source by Michael Droettboom)

See the docstrings of the classes and functions for more information.

.. code::

  from gamera import roman_text

  # Load an image and a classifier (see documentation elsewhere)...

  # image is a one-bit image
  # classifier is a classifier object with a production database loaded
  
  page = roman_text.ocr(image, classifier)

*page* is a ``Page`` object.  You can display how the page was
segmenteted in different ways using ``.display_sections``,
``.display_lines``, ``.display_glyphs``, and
``.display_segmentation``.

Once you have a segmented page object, you can convert it to ASCII
using the ``make_string`` function.  It takes as input a ``Section``
object, and an optional name-mapping function.  The name-mapping
function is any function that takes a string with a symbol id_name and
returns a character (which may be Unicode).  This function defines how
the names used in the training data map to characters in the output.

If no name-mapping function is provided a default one is used which
maps Gamera symbol id names to Unicode character names and then to
Unicode characters.  All periods ('.') in the Gamera id name are
converted to spaces, and then the result is used as a Unicode
character name.  For instance ``latin.capital.letter.a`` maps to
``LATIN CAPITAL LETTER A``.  (`Charts of Unicode character names`_).
The version of Unicode used corresponds with that used in your version
of Python.  For instance, Python 2.3.x supports Unicode 3.2.0.

.. _: http://www.unicode.org/charts/

.. code::

  output = []
  for section in page.sections:
    output.append(roman_text.make_string(section))

"""

from gamera import core
import unicodedata
import string

class Page:
    def __init__(self, image, glyphs):
        """Create a page with the given image."""
        self.sections = []
        self.image = image
        self.glyphs = glyphs
        self.section_search_size = 1
        self.__fill = 0
        self.__noise_size = 0
        self.__large_size = 0

    def __section_size_test(self, glyph):
        """Filter for section finding - removes very small and
        very large glyphs"""
        black_area = glyph.black_area()[0]
        if black_area > self.__noise_size and \
                glyph.nrows < self.__large_size and \
                glyph.ncols < self.__large_size:
            return 1
        else:
            return 0

    def __avg_glyph_size(self, glyphs):
        """Compute the average glyph size for the page"""
        total = 0.0
        for g in glyphs:
            total += g.nrows
            total += g.ncols
        return total / (2 * len(glyphs))

    def __find_intersecting_rects(self, glyphs, index):
        """For section finding - return the index of glyphs intersecting
        the glyph and the index passed in."""
        g = glyphs[index]
        inter = []
        for i in range(len(glyphs)):
            if i == index:
                continue
            if g.intersects(glyphs[i]):
                inter.append(i)
        return inter

    def segment(self):
        """Segment the page into sections and lines. Also computes
        all of the statistics for the sections and lines."""
        self.find_sections()
        self.find_lines()
        for s in self.sections:
            s.calculate_stats()
            for line in s.lines:
                line.calculate_stats()

    def find_lines(self):
        """Find the lines within all of the sections. Must be called
        after find_sections."""
        for s in self.sections:
            s.find_lines()

    def find_sections(self):
        """Find the sections within an image - this finds large blocks
        of text making it possible to find the lines within complex
        text layouts."""
      
        glyphs = self.glyphs

        FUDGE = self.__avg_glyph_size(glyphs) * self.section_search_size

        # remove noise and large objects
        self.__noise_size = FUDGE
        self.__large_size = FUDGE * 20
        new_glyphs = []
        for g in glyphs:
            if self.__section_size_test(g):
                new_glyphs.append(g)
            else:
                if self.__fill:
                    g.fill_white()
        glyphs = new_glyphs
        
        # Sort the glyphs left-to-right and top-to-bottom
        glyphs.sort(lambda x, y: cmp(x.ul_x, y.ul_x))
        glyphs.sort(lambda x, y: cmp(x.ul_y, y.ul_y))
        
        # Create rectangles for each glyph that are bigger by FUDGE
        big_rects = []
        for g in glyphs:
            ul_y = max(0, g.ul_y - FUDGE)
            ul_x = max(0, g.ul_x - FUDGE)
            lr_y = min(self.image.lr_y, g.lr_y + FUDGE)
            lr_x = min(self.image.lr_x, g.lr_x + FUDGE)
            ul_x = int(ul_x); ul_y = int(ul_y)
            nrows = int(lr_y - ul_y + 1)
            ncols = int(lr_x - ul_x + 1)
            big_rects.append(core.Rect(core.Point(ul_x, ul_y), core.Dim(ncols, nrows)))

        # Search for intersecting glyphs and merge them. This is
        # harder than it seems at first because we want everything
        # to merge together that intersects regardless of the order
        # in the list. It ends up being similar to connected-component
        # labeling. This is prone to be kind-of slow.
        current = 0
        rects = big_rects
        while(1):
            # Find the indexexes of any rects that interesect with current
            inter = self.__find_intersecting_rects(rects, current)
            # If we found intersecting rectangles merge them with them current
            # rect, remove them from the list, and start the whole process
            # over. We start over to make certain that everything that should
            # be merged is.
            if len(inter):
                g = rects[current]
                new_rects = [g]
                for i in range(len(rects)):
                    if i == current:
                        continue
                    if i in inter:
                        g.union(rects[i])
                    else:
                        new_rects.append(rects[i])
                rects = new_rects
                current = 0
            # If we didn't find anything that intersected move on to the next
            # rectangle.
            else:
                current += 1
            # Bail when we are done.
            if current >= len(rects):
                break
        
        # Create the sections
        sections = []
        for rect in rects:
            sections.append(Section(rect))
        
        # Place the original (small) glyphs into the sections
        for glyph in self.glyphs:
            if self.__section_size_test(glyph):
                for s in sections:
                    if s.bbox.intersects(glyph):
                        s.add_glyph(glyph)
                        break
        
        # Fix up the bounding boxes
        for s in sections:
            s.calculate_bbox()
                
        self.sections = sections
        
    def display_sections(self, clear=1):
        """Display the sections found by placing a box around them
        in a display. If clear is true then any boxes already on
        the displayed are cleared first."""
        # display the sections
        result = self.sections                

        if self.image._display == None:
            self.image.display()
        display = self.image._display
        if clear:
            display.clear_all_boxes()
        for rect in result:
            b = rect.bbox
            display.add_box(core.Rect(core.Point(b.ul_x, b.ul_y), core.Dim(b.ncols, b.nrows)))

    def display_lines(self, clear=1):
        """Display the lines found by placing a box around them
        in a display. If clear is true then any boxes already on
        the displayed are cleared first."""
        if self.image._display == None:
            self.image.display()
        display = self.image._display
        if clear:
            display.clear_all_boxes()
        for s in self.sections:
            for line in s.lines:
                b = line.bbox
                display.add_box(core.Rect(core.Point(b.ul_x, b.ul_y), core.Dim(b.ncols, b.nrows)))

    def display_glyphs(self, clear=1):
        if self.image._display == None:
            self.image.display()
        display = self.image._display
        if clear:
            display.clear_all_boxes()
        for rect in self.glyphs:
            b = rect
            display.add_box(b.ul_y, b.ul_x, b.nrows, b.ncols)

    def display_segmentation(self):
        self.display_lines()
        self.display_sections(0)

class Section:
    def __init__(self, bbox):
        self.bbox = core.Rect(bbox)
        self.lines = []
        self.glyphs = []
        # stats
        self.avg_glyph_area = 0
        self.avg_glyph_height = 0
        self.avg_glyph_width = 0
        self.avg_line_height = 0
        self.agv_line_width = 0

    def find_tall_glyphs(self, stdev=20):
        from gamera import stats
        tall = []
        for i in range(len(self.glyphs)):
            g = self.glyphs[i]
            if stats.samplestdev([g.nrows, self.avg_glyph_height]) > stdev:
                tall.append(i)
        return tall

    def add_line(self, line):
        self.lines.append(line)
        self.lines.sort(lambda x, y: cmp(x.bbox.ul_y, y.bbox.ul_y))

    def add_glyph(self, glyph):
        self.glyphs.append(glyph)

    def calculate_bbox(self):
        assert(len(self.glyphs) > 0)
        self.bbox = core.Rect(self.glyphs[0])
        for glyph in self.glyphs:
            self.bbox.union(glyph)

    def calculate_stats(self):
        self.calculate_glyph_stats()
        self.calculate_line_stats()

    def calculate_glyph_stats(self):
        # calculate glyph stats
        total_area = 0.0
        total_gheight = 0.0
        total_gwidth = 0.0
        
        for g in self.glyphs:
            nrows = g.nrows
            ncols = g.ncols
            total_area += nrows * ncols
            total_gheight += nrows
            total_gwidth += ncols
        l = len(self.glyphs)
        self.avg_glyph_area = total_area / l
        self.avg_glyph_height = total_gheight / l
        self.avg_glyph_width = total_gwidth / l

    def calculate_line_stats(self):
        # calculate line stats
        total_lheight = 0.0
        total_lwidth = 0.0
        for line in self.lines:
            total_lheight += line.bbox.nrows
            total_lwidth += line.bbox.ncols
        l = len(self.lines)
        self.avg_line_height = total_lheight / l
        self.avg_line_width = total_lwidth / l
        

    def __find_intersecting_lines(self, glyphs, index):
        g = glyphs[index]
        inter = []
        for i in range(len(glyphs)):
            if i == index:
                continue
            if g.bbox.intersects(glyphs[i].bbox):
                inter.append(i)
        return inter

    def find_lines(self):
        # Remove abnormally tall glyphs that might interfer with
        # line finding
        self.calculate_glyph_stats()
        tall_indexes = self.find_tall_glyphs()
        tall = []
        glyphs = []
        for i in range(len(self.glyphs)):
            if i in tall_indexes:
                tall.append(self.glyphs[i])
            else:
                glyphs.append(self.glyphs[i])
        orig_glyphs = self.glyphs
        self.glyphs = glyphs
        
        # find the lines - this is very basic for now
        lines = []
        for glyph in self.glyphs:
            found = 0
            for line in lines:
                if line.contains_glyph(glyph):
                    line.add_glyph(glyph)
                    found = 1
                    break
            if not found:
                new_line = Line(glyph)
                lines.append(new_line)

        # Merge any overlapping lines
        current = 0
        while(1):
            inter = self.__find_intersecting_lines(lines, current)
            if len(inter):
                new_lines = [lines[current]]
                for i in range(len(lines)):
                    if i == current:
                        continue
                    if i in inter:
                        new_lines[0].merge(lines[i])
                    else:
                        new_lines.append(lines[i])
                current = 0
                lines = new_lines
            else:
                current += 1
            if current >= len(lines):
                break

        # Put the tall glyphs back in by assigning them to the first line
        # we come to that intersects.
        for i in range(len(tall)):
            for line in lines:
                found = 0
                if line.bbox.contains_y(tall[i].ul_y):
                    line.add_glyph(tall[i])
                    found = 1
                    break
            if not found:
                print "Did not find lines for all tall glyphs"
                
        self.glyphs = orig_glyphs
        self.lines = lines
        

class Line:
    def __init__(self, glyph):
        self.center = 0
        self.bbox = core.Rect(core.Point(glyph.ul_x, glyph.ul_y), core.Dim(glyph.ncols, glyph.nrows))
        self.glyphs = []
        self.add_glyph(glyph)

    def add_glyph(self, glyph):
        self.glyphs.append(glyph)
        self.glyphs.sort(lambda x, y: cmp(x.ul_x, y.ul_x))
        self.bbox.union(glyph)

    def calculate_stats(self):
        total_center = 0
        for glyph in self.glyphs:
            total_center += (glyph.ul_y + glyph.lr_y) / 2
            self.bbox.union(glyph)
        self.center = total_center / len(self.glyphs)

    def contains_glyph(self, glyph):
        center = (glyph.ul_y + glyph.lr_y) / 2
        if self.bbox.contains_y(center):
            return 1
        else:
            return 0

    def merge(self, line):
        self.glyphs.extend(line.glyphs)
        self.calculate_stats()
        
def name_lookup_old(id_name):
    """Converts a symbol name into a single character."""
    split_string = string.split(id_name, '.')
    l = len(split_string)
    if l > 1:
        if split_string[0] == "lower":
            return string.lower(split_string[-1])
        elif split_string[0] == "upper":
            return string.upper(split_string[-1])
        elif split_string[0] == "symbol":
            if l == 2:
                s = split_string[1]
                if s == "comma":
                    return ","
                elif s == "dot":
                    return ""
                elif s == "plus":
                    return "+"
                elif s == "lessthan":
                    return "<"
                else:
                    print "ERROR: Name not known about:", id_name
                    return ""
            elif l == 3:
                if split_string[1] == "paren":
                    if split_string[2] == "close":
                        return ")"
                    else:
                        return "("
                elif split_string[1] == "bracket":
                    if split_string[2] == "close":
                        return "]"
                    else:
                        return "["
                else:
                    print "ERROR: Name not known about:", id_name
                    return ""
            else:
                print "ERROR: Name not known about:", id_name
                return ""
        elif split_string[0] == "digit":
            return split_string[1]
        elif split_string[0] == "line":
            return ""
        elif split_string[0] == "punctuation":
            if split_string[1] == "comma":
                return ","
            else:
                return ""
        else:
            print "ERROR: Name not known about:", id_name
            return ""
    else:
        return ""

def name_lookup_unicode(id_name):
    name = id_name.replace(".", " ")
    name = name.upper()
    try:
        return unicodedata.lookup(name)
    except KeyError:
        print "ERROR: Name not found:", name
        return ""
    
def make_string(lines, name_lookup_func=name_lookup_unicode):
    s = ""

    for line in lines:
        glyphs = line.glyphs
        total_space = 0
        for i in range(len(glyphs) - 1):
            total_space += glyphs[i + 1].ul_x - glyphs[i].lr_x
        average_space = total_space / len(glyphs)
        for i in range(len(glyphs)):
            if i > 0:
                if (glyphs[i].ul_x - glyphs[i - 1].lr_x) > (average_space * 2):
                    
                    s = s + " "
            s = s + name_lookup_func(glyphs[i].get_main_id())
        s = s + "\n"
    return s

def output(lines, filename="text.txt"):
    s = make_string(lines)
    f = file(filename, "w")
    f.write(s)
    f.flush()
    f.close()

def ocr(image, classifier = None, glyphs = None):
    """Performs basic OCR segmentation.

*image*: OneBit image
*classifier*: a classifier with a production database loaded (optional)
*glyphs*: a list of glyphs from the image (optional)

Returns a ``Page`` object.
"""
    if glyphs is None:
        glyphs = image.cc_analysis()
    if classifier is not None:
        #classifier.classify_list_automatic(glyphs)
        glyphs = classifier.group_and_update_list_automatic(glyphs)
    page = Page(image, glyphs)
    page.segment()
    return page
