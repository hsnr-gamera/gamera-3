#!/usr/bin/env python

# vi:set tabsize=3:
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
import locale, sys, glob, os.path, cStringIO, inspect
from stat import ST_MTIME
from types import DictType
from time import strftime, localtime
try:
    locale.setlocale(locale.LC_ALL, '')
except:
    pass

try:
   from docutils.core import publish_file
   import docutils.parsers.rst
except ImportError, e:
   print "'docutils' 0.3 or later must be installed to generate the documentation."
   print "It can be downloaded at http://docutils.sf.net"
   sys.exit(1)

try:
   import SilverCity
except ImportError, e:
   print "'SilverCity' 0.9 or later must be installed to generate the documentation."
   print "It can be downloaded at http://silvercity.sf.net"
   sys.exit(1)

# SilverCity support
def code_block(name, arguments, options, content, lineno,
             content_offset, block_text, state, state_machine ):
  language = arguments[0]
  try:
    module = getattr(SilverCity, language)
    generator = getattr(module, language+"HTMLGenerator")
  except AttributeError:
    error = state_machine.reporter.error( "No SilverCity lexer found "
      "for language '%s'." % language, 
      docutils.nodes.literal_block(block_text, block_text), line=lineno )
    return [error]
  io = cStringIO.StringIO()
  generator().generate_html( io, '\n'.join(content) )
  html = '<div class="code-block">\n%s\n</div>\n' % io.getvalue()
  raw = docutils.nodes.raw('',html, format = 'html')
  return [raw]
code_block.arguments = (1,0,0)
code_block.options = {'language' : docutils.parsers.rst.directives.unchanged }
code_block.content = 1
  
docutils.parsers.rst.directives.register_directive( 'code', code_block )

try:
   from gamera import core, args, paths, util
   from gamera.enums import *
except ImportError, e:
   print "Cannot load gameracore."
   print "Gamera must be built before you can regenerate the documentation."
   sys.exit(1)

doc_path = "./"
doc_src_path = "./src/"
doc_images_path = "./images/"
doc_src_images_path = "./src/images/"
icons_path = "../gamera/pixmaps/"

def ui(s):
   sys.stdout.write(s)
   sys.stdout.flush()

def get_rest_docs():
   for file in glob.glob(doc_src_path + "*.txt"):
      path, filename = os.path.split(file)
      root, ext = os.path.splitext(filename)
      yield root, open(file, 'r'), strftime("%B %d, %Y", localtime(os.stat(file)[ST_MTIME]))

def method_doc(s, func, level):
   s.write("``%s``\n" % func.__name__)
   s.write(levels[level] * (len(func.__name__) + 4) + "\n\n")
   if func.return_type != None:
      s.write(func.return_type.rest_repr() + " ")
   header = "**%s** (%s)\n" % (func.__name__, ', '.join(['``%s`` *%s*' % (x.rest_repr(), x.name) for x in func.args.list]))
   s.write(header)
   s.write("\n\n")
   if func.self_type != None:
      s.write(":Operates on: ``%s``\n" % func.self_type.rest_repr())
   if func.return_type != None:
      s.write(":Returns: ``%s``\n" % (func.return_type.rest_repr()))
   if func.category == None:
      category = func.module.category
   else:
      category = func.category
   if category != None:
      s.write(":Category: %s\n" % category)
   s.write(":Defined in: %s\n" % os.path.split(inspect.getsourcefile(func))[1])
   if func.module.author != None:
      if func.module.url != None and 0:
         s.write(":Author: `%s`__\n.. __: %s\n" % (func.module.author, func.module.url))
      else:
         s.write(":Author: %s\n" % (func.module.author,))
   if func.image_types_must_match:
      s.write("\n*All images passed in (including self) must have the same pixel type.*\n\n")
   if func.__doc__ == None or func.__doc__ == "":
      s.write("\n.. warning:: No documentation written.\n\n")
   else:
      s.write("\n\n%s\n" % func.__doc__)
   method_example(s, func, level)

def method_example(s, func, level):
   if len(func.doc_examples):
      s.write("\n----------\n\n")
   for i, doc_example in enumerate(func.doc_examples):
      if isinstance(func.self_type, args.ImageType):
         pixel_type = doc_example[0]
         doc_example = doc_example[1:]
         pixel_type_name = util.get_pixel_type_name(pixel_type)
         image = core.load_image(os.path.join(paths.test, pixel_type_name + "_generic.tiff"))
         result = func.__call__(*tuple([image] + list(doc_example)))
         s.write("**Example %d:** %s%s\n\n" % (i + 1, func.__name__, str(tuple(doc_example))))
         s.write(".. image:: images/%s_generic.png\n\n" % pixel_type_name)
      else:
         s.write("**Example %d:** %s%s\n\n" % (i + 1, func.__name__, str(tuple(doc_example))))
         result = func.__call__(*doc_example)
      result_filename = "%s_plugin_%02d" % (func.__name__, i)
      if isinstance(result, core.ImageBase):
         result.save_PNG(doc_images_path + result_filename + ".png")
         s.write(".. image:: images/%s.png\n\n" % (result_filename))
      elif result is None:
         image.save_PNG(doc_images_path + result_filename + ".png")
         s.write(".. image:: images/%s.png\n\n" % (result_filename))
      elif isinstance(func.return_type, args.ImageList):
         subst = "\n\n"
         for j, part in enumerate(result):
            result_filename = "%s_plugin_%02d_%02d" % (func.__name__, i, j)
            part.save_PNG(doc_images_path + result_filename + ".png")
            s.write("|%s| " % result_filename)
            subst += ".. |%s| image:: images/%s.png\n" % (result_filename, result_filename)
         s.write(subst)
      else:
         s.write("*result* = " + repr(result) + "\n\n")
   s.write("\n\n")
           
levels = "=-`:'"
def generate_plugin_docs():
   def recurse(s, methods, level):
      methods_list = methods.items()
      methods_list.sort()
      for key, val in methods_list:
         if type(val) == DictType:
            s.write("\n%s\n%s\n\n" % (key, levels[level] * len(key)))
            ui("  " * (level + 1) + key + "\n")
            recurse(s, val, level + 1)
         else:
            method_doc(s, val, level)

   def methods_flatten(dest, source, flat):
      for key, val in source.items():
       if type(val) == DictType:
         if key != "Test":
            if not dest.has_key(key):
               dest[key] = {}
            methods_flatten(dest[key], val, flat)
       else:
         dest[key] = val
         flat[key] = val

   ui("Generating plugin documentation ======\n")
   methods = core.ImageBase.methods
   flat_methods = {}
   flat_list = {}
   for pixel_type in ALL + [NONIMAGE]:
      methods_flatten(flat_methods, methods[pixel_type], flat_list)

   flat_list = flat_list.keys()
   flat_list.sort()

   s = open(doc_src_path + "plugins.txt", "w")
   s.write("""=======
Plugins
=======

**Alphabetical list**

%s
   """ % (', '.join(['%s_' % x for x in flat_list])))
   recurse(s, flat_methods, 0)
   s.close()

def generate_generic_pngs():
   for pixel_type in ALL:
      if pixel_type != FLOAT:
         pixel_type_name = util.get_pixel_type_name(pixel_type)
         image = core.load_image(os.path.join(paths.test, pixel_type_name + "_generic.tiff"))
         print image.pixel_type_name
         image.save_PNG("%s%s_generic.png" % (doc_images_path, pixel_type_name))

def copy_images(output_path):
   if not os.path.exists(output_path):
      os.mkdir(output_path)
   for path in (doc_images_path, doc_src_images_path, icons_path):
      for file in glob.glob(path + "*.png"):
         path, filename = os.path.split(file)
         open(output_path + filename, "wb").write(open(file, "rb").read())

def gendoc():
   core.init_gamera()
   generate_plugin_docs()
   ui("Generating and copying images\n")
   generate_generic_pngs()
   copy_images("html/images/")
   ui("Generating HTML\n")
   output_path = doc_path + "html/"
   for name, fd, mtime in get_rest_docs():
      ui("  Generating " + name + "\n")
      lines = fd.readlines()
      lines = lines[:3] + ["\n", "**Last modifed**: %s\n\n" % mtime, ".. contents::\n", "\n"] + lines[3:]
      fd = cStringIO.StringIO(''.join(lines))
      publish_file(source=fd, destination_path=os.path.join(output_path, name + ".html"),
                   writer_name="html")
   ui("\n")
      
if __name__ == "__main__":
   gendoc()
   
