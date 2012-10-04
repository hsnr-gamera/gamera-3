# -*- mode: python; indent-tabs-mode: nil; tab-width: 3 -*-
# vim: set tabstop=3 shiftwidth=3 expandtab:
#
# Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom,
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#

import os, os.path
from gamera import util
from gamera.core import *

class ClassifierStat:
   def __init__(self, classifier, path, max_size=64):
      self.classifier = classifier
      self.path = path
      if not os.path.exists(path):
         os.makedirs(path)
      self.image_path = os.path.join(path, "images")
      if not os.path.exists(self.image_path):
         os.makedirs(self.image_path)
      self.max_size = max_size

   def make_example_glyphs(self):
      self.example_glyphs = {}
      for glyph in self.classifier.get_glyphs():
         for conf, id in glyph.id_name:
            if not self.example_glyphs.has_key(id):
               self.example_glyphs[id] = glyph

   def make_grid(self, rows, cols):
      grid = []
      for i in range(rows):
         row = [None] * cols
         grid.append(row)
      return grid

   def make_pages(self):
      grids = self.make_result()
      for name, grid in grids:
         filename = os.path.join(self.path, name.lower().replace(" ", "_"))
         self.make_html(filename + ".html", name, grid)
         self.make_csv(filename + ".csv", name, grid)

   def make_html(self, filename, name, grid):
      fd = open(filename, "w")
      fd.write("<html><head><title>%s</title></head><body><h1>%s</h1>" %
               (name, name))
      fd.write("<table border=\"1\">")
      for row in grid:
         fd.write("<tr>")
         for col in row:
            fd.write("<td>")
            if isinstance(col, ImageBase):
               id = col.get_main_id()
               image_filename = "images/%s.png" % id
               col.save_PNG(os.path.join(self.path, image_filename))
               fd.write('<img src="%s" width="%d" height="%d"/><br/>%s' %
                        (image_filename, min(col.width, self.max_size),
                         min(col.height, self.max_size), id))
            elif col is None:
               fd.write("&nbsp;")
            else:
               fd.write(str(col))
            fd.write("</td>")
         fd.write("</tr>")
      fd.write("</table>")
      fd.write("</body></html>")
      fd.close()

   def make_csv(self, filename, name, grid):
      def convert(x):
         if isinstance(x, ImageBase):
            return x.get_main_id()
         elif x == None:
            return ""
         else:
            return str(x)
      fd = open(filename, "w")
      for row in grid:
         formatted_row = ", ".join([convert(x) for x in row])
         fd.write(formatted_row)
         fd.write("\n")
      fd.close()
            
class ConfusionMatrix(ClassifierStat):
   title = "Confusion Matrix"
   
   def make_result(self):
      self.make_example_glyphs()
      result = {}
      for id0 in self.example_glyphs.keys():
         leaf = {}
         for id1 in self.example_glyphs.keys():
            leaf[id1] = 0
         result[id0] = leaf
         
      classifier = self.classifier
      glyphs = classifier.get_glyphs()
      progress = util.ProgressFactory("Generating confusion matrix...", len(glyphs) / 50)
      try:
         for i, glyph in enumerate(glyphs):
            idname, conf = classifier.classify_with_images(glyphs, glyph, True)
            result[glyph.get_main_id()][idname[0][1]] += 1
            if i % 50 == 0:
               progress.step()
      finally:
         progress.kill()

      ids = result.keys()
      ids.sort()
      grid = self.make_grid(len(ids) + 1, len(ids) + 1)
      for i, id in enumerate(ids):
         grid[0][i+1] = self.example_glyphs[id]
         grid[i+1][0] = self.example_glyphs[id]
      for i, id0 in enumerate(ids):
         res = result[id0]
         sum = 0
         for val in res.values():
            sum += val
         for j, id0 in enumerate(ids):
            grid[i+1][j+1] = str(int((float(res[id0]) / sum) * 100.0)) + "%"
      return [("Confusion Matrix", grid)]

class ClassNameHistogram(ClassifierStat):
   title = "Class Name Histogram"

   def make_result(self):
      self.make_example_glyphs()
      result = {}
      for id0 in self.example_glyphs.keys():
         result[id0] = 0

      for glyph in self.classifier.get_glyphs():
         id = glyph.get_main_id()
         result[id] += 1

      result = [(val, key) for key, val in result.items()]
      result.sort()
      result.reverse()

      grid = self.make_grid(len(self.example_glyphs.keys()), 2)
      for i, (val, key) in enumerate(result):
         grid[i][0] = self.example_glyphs[key]
         grid[i][1] = val
      return [("Class Name Histogram", grid)]

all_stat_pages = [ConfusionMatrix, ClassNameHistogram]
def make_stat_pages(classifier, path, pages=None, max_size=64):
   if pages is None:
      pages = all_stat_pages
   for page in pages:
      name = page.__name__.lower()
      page_path = os.path.join(path, name)
      p = page(classifier, page_path, max_size)
      p.make_pages()

