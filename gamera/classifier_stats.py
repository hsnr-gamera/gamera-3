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

from os import path
import os
from gamera import util

# To add a your custom stats pages --
#    Create a new class that inherits from GlyphStats
#    Override _write_core to call of the the page generating functions required
#       (Do not forget to chain to the base class first.)
#    In each page generating function, call _html_start with a new filename and title
#       to get a file pointer in which you can write your stats.
#    Close the file handle by calling _html_end.
#    A front page will be automatically generated that links to each of your created
#       stats pages.

class GlyphStats:
    def __init__(self, glyphs):
        from gamera import pyplate, generate_help
        self.header = pyplate.Template(generate_help.header + "[[call header(title, 1)]]<h1>[[title]]</h1>")
        self.footer = pyplate.Template(generate_help.footer + "[[exec toplevel_path='']][[call footer(0)]]")
        self._glyphs = glyphs
        
    def write(self, directory):
        self._pages = []
        if not path.exists(directory):
            os.mkdir(directory)
        self._write_core(directory)

    def _html_start(self, directory, file, title):
        fd = open(path.join(directory, file + ".html"), 'w')
        self.header.execute(fd, {'title': title})
        self._pages.append(file)
        return fd

    def _html_end(self, stream):
        self.footer.execute(stream)
        stream.close()

    def _write_core(self, directory):
        sorted_glyphs = self._save_images(directory)
        self._table_page(directory, sorted_glyphs)
        self._histogram_page(directory, sorted_glyphs)

    def _save_images(self, directory):
        sorted_glyphs = {}
        progress = util.ProgressFactory("Saving images...")
        for i, glyph in util.enumerate(self._glyphs):
            name = glyph.get_main_id()
            if sorted_glyphs.has_key(name):
                sorted_glyphs[name].append(glyph)
            else:
                sorted_glyphs[name] = [glyph]
            number = len(sorted_glyphs[name])
            filename = "%s-%08d.tiff" % (name, number)
            glyph.save_tiff(path.join(directory, filename))
            progress.update(i, len(self._glyphs))
        progress.update(1, 1)
        return sorted_glyphs
        
    def _table_page(self, directory, sorted_glyphs):
        fd = self._html_start(directory, 'table', 'Table of glyphs')
        keys = sorted_glyphs.keys()
        keys.sort()
        for name in keys:
            fd.write("<h2>%s</h2>" % name)
            size = 0
            for i, glyph in util.enumerate(sorted_glyphs[name]):
                if size + glyph.ncols > 500:
                    fd.write("<br/>")
                    size = 0
                fd.write('<img src="%s-%08d.tiff" width="%d" height="%d"/>' %
                         (name, i, glyph.ncols, glyph.nrows))
        self._html_end(fd)

    def _histogram_page(self, directory, sorted_glyphs):
        pass
