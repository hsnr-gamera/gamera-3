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

######################################################################
# Import Python stdlib
from __future__ import generators
import locale, sys, glob, cStringIO, inspect, getopt, os.path, shutil
from stat import ST_MTIME
from time import strftime, localtime
import traceback
try:
   locale.setlocale(locale.LC_ALL, '')
except:
   pass

######################################################################
# Import docutils
try:
   from docutils.core import publish_file
   import docutils.parsers.rst
except ImportError, e:
   print "'docutils' 0.3 or later must be installed to generate the documentation."
   print "It can be downloaded at http://docutils.sf.net"
   sys.exit(1)

######################################################################
# Import SilverCity
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

######################################################################
# Import Gamera

try:
   from gamera import core, args, paths, util
   from gamera.plugins import _png_support
   from gamera.plugins import _image_conversion
   from gamera.enums import *
except ImportError, e:
   print "Cannot load gameracore."
   print "Gamera must be built before you can generate the documentation."
   sys.exit(1)

######################################################################

_underline_levels = "=-`:'"
def underline(level, s, extra=0):
   return _underline_levels[level] * (len(s) + extra)

class DocumentationGenerator:
   def __init__(self, root_path=".", classes=[]):
      self.set_paths(root_path)
      self.classes = classes

   def set_paths(self, root):
      def check_path(path):
         if not os.path.exists(path):
            raise RuntimeError("Documentation directory '%s' does not exist" % os.path.abspath(path))
         
      join = os.path.join
      self.root_path = root
      check_path(self.root_path)
      self.src_path = join(root, "src/")
      check_path(self.src_path)
      self.src_images_path = join(self.src_path, "images/")
      check_path(self.src_images_path)
      self.icons_path = join(root, "../gamera/pixmaps/")
      self.output_path = join(self.root_path, "html/")
      self.output_images_path = join(self.output_path, "images/")
      if not os.path.exists(self.output_path):
         os.mkdir(self.output_path)
      if not os.path.exists(self.output_images_path):
         os.mkdir(self.output_images_path)

   def generate(self):
      self.copy_css(self.src_path, self.output_path)
      PluginDocumentationGenerator(self)
      ClassDocumentationGenerator(self, self.classes)
      self.copy_images([self.src_images_path, self.icons_path],
                       self.output_images_path)
      self.generate_generic_pngs()
      self.convert_to_html()
      print

   def generate_generic_pngs(self):
      print "Copying over generic images"
      for pixel_type in ALL:
         if pixel_type in (FLOAT, COMPLEX):
            pixel_type_name = "GreyScale"
         else:
            pixel_type_name = util.get_pixel_type_name(pixel_type)
         image = core.load_image(
            os.path.join(paths.test,
                         pixel_type_name + "_generic.tiff"))
         print "  " + image.pixel_type_name
         _png_support.save_PNG(
            image,
            os.path.join(self.output_images_path,
                         "%s_generic.png" % (pixel_type_name)))

   def copy_css(self, input_path, output_path, css_file="default.css"):
      print "Copying CSS file"
      shutil.copyfile(os.path.join(input_path, css_file),
                      os.path.join(output_path, css_file))

   def copy_images(self, src_paths, output_path):
      print "Copying images"
      for path in src_paths:
         if os.path.exists(path):
            for file in glob.glob(os.path.join(path, "*.*")):
               path, filename = os.path.split(file)
               shutil.copyfile(file, os.path.join(output_path, filename))

   def get_rest_docs(self):
      for file in glob.glob(self.src_path + "*.txt"):
         path, filename = os.path.split(file)
         root, ext = os.path.splitext(filename)
         yield (file, root,
                open(file, 'r'),
                strftime("%B %d, %Y", localtime(os.stat(file)[ST_MTIME])))

   def convert_to_html(self):
      print "Converting to HTML"
      for filename, rootname, fd, mtime in self.get_rest_docs():
         output_file = os.path.join(self.output_path, rootname + ".html")
         if (not os.path.exists(output_file) or
             os.stat(filename)[ST_MTIME] > os.stat(output_file)[ST_MTIME]):
            print "  Generating " + rootname
            lines = fd.readlines()
            lines = lines[:3] + ["\n", "**Last modifed**: %s\n\n" % mtime, ".. contents::\n", "\n"] + lines[3:]
            fd = cStringIO.StringIO(''.join(lines))
            try:
               publish_file(source=fd, destination_path=output_file, writer_name="html")
            except KeyboardInterrupt, e:
               raise e
            except Exception, e:
               traceback.print_exc()
               print "Unable to convert '%s' to HTML: %s" % (filename, e)
               if os.path.exists(output_file):
                  os.remove(output_file)

class PluginDocumentationGenerator:
   def __init__(self, docgen):
      print "Generating plugin documentation"

      self.docgen = docgen

      images = self.get_generic_images()
      images['src_images_path'] = docgen.src_images_path

      methods = self.get_methods()
      if len(methods) == 0:
         return

      self.recurse(methods, 0, images)
      self.table_of_contents(methods)
      
   def get_generic_images():
      images = {}
      for i in [ONEBIT, RGB, GREYSCALE, GREY16]:
         pixel_type_name = util.get_pixel_type_name(i)
         images[i] = core.load_image(os.path.join(
            paths.test, pixel_type_name + "_generic.tiff"))
      for i in (FLOAT, COMPLEX):
         image = core.load_image(os.path.join(
            paths.test, "GreyScale_generic.tiff"))
         if i == FLOAT:
            images[i] = _image_conversion.to_float(image)
         elif i == COMPLEX:
            try:
               images[i] = _image_conversion.to_complex(image)
            except:
               pass
      return images
   get_generic_images = staticmethod(get_generic_images)

   def get_methods():
      def methods_flatten(dest, source, flat):
         for key, val in source.items():
          if type(val) == dict:
            if key != "Test":
               if not dest.has_key(key):
                  dest[key] = {}
               methods_flatten(dest[key], val, flat)
          else:
            dest[key] = val
            flat[key] = val

      methods = core.ImageBase.methods
      flat_methods = {}
      flat_list = {}
      for pixel_type in ALL + [NONIMAGE]:
         if methods.has_key(pixel_type): 
             methods_flatten(flat_methods, methods[pixel_type], flat_list)

      return flat_methods
   get_methods = staticmethod(get_methods)

   def recurse(self, methods, level, images, s=None):
      methods_list = methods.items()
      methods_list.sort()
      for key, val in methods_list:
         if type(val) == dict:
            if level == 0:
                filename = key.lower()
                s = open(os.path.join(self.docgen.src_path, filename + ".txt"), "w")
            s.write("\n%s\n%s\n\n" % (key, underline(level, key)))
            print "  " * (level + 1) + key
            self.recurse(val, level + 1, images, s)
         else:
            self.method_doc(val, level, images, s)

   def table_of_contents(self, methods):
      def toc_recurse(s, methods, level, links, index, filename=None):
          methods_list = methods.items()
          methods_list.sort()
          for key, val in methods_list:
             if type(val) == dict and level == 0:
                 filename = key.lower()
             s.write("  " * level)
             s.write("- ")
             s.write(key)
             s.write("_")
             s.write("\n\n")
             href = "%s.html#%s" % (filename, key.lower().replace("_", "-"))
             links.append(".. _%s: %s" % (key, href))
             if type(val) == dict:
                 toc_recurse(s, val, level + 1, links, index, filename)
             else:
                 index.append(key)

      s = open(os.path.join(self.docgen.src_path, "plugins.txt"), "w")
      s.write("=======\nPlugins\n=======\n\nBy categories\n-------------\n\n")
      links = []
      index = []
      toc_recurse(s, methods, 0, links, index)
      s.write("Alphabetical\n-------------\n")
      index.sort(lambda x, y: cmp(x.lower(), y.lower()))
      letter = ord('A') - 1
      first = True
      for name in index:
          if ord(name[0].upper()) > letter:
              letter = ord(name[0].upper())
              s.write("\n\n**")
              s.write(chr(letter))
              s.write("**\n\n")
              first = True
          if not first:
              s.write(", ")
          s.write(name)
          s.write("_")
          first = False
      s.write("\n\n")
      s.write("\n".join(links))
      
   def method_doc(self, func, level, images, s):
      s.write("``%s``\n" % func.__name__)
      s.write(underline(level, func.__name__, 4))
      s.write("\n\n")
      if func.return_type != None:
         s.write(func.return_type.rest_repr() + " ")
      header = "**%s** (%s)\n" % (func.__name__, ', '.join(
          [x.rest_repr(True) for x in func.args.list]))
      s.write(header)
      s.write("\n\n")
      if func.self_type != None:
         s.write(":Operates on: %s\n" % func.self_type.rest_repr(False))
      if func.return_type != None:
         s.write(":Returns: %s\n" % (func.return_type.rest_repr(False)))
      if func.category == None:
         category = func.module.category
      else:
         category = func.category
      if category != None:
         s.write(":Category: %s\n" % category)
      file = os.path.split(inspect.getsourcefile(func))[1]
      if file == 'plugin.py':
          file = 'core.py'
      s.write(":Defined in: %s\n" % os.path.split(inspect.getsourcefile(func))[1])
      if func.author is None:
          author = func.module.author
      else:
          author = func.author
      if author != None:
         if func.module.url != None and 0:
            s.write(":Author: `%s`__\n.. __: %s\n" % (author, func.module.url))
         else:
            s.write(":Author: %s\n" % (author,))
      if func.image_types_must_match:
         s.write("\n*All images passed in (including self) must have the same pixel type.*\n\n")
      if func.__doc__ == None or func.__doc__ == "":
         s.write("\n.. warning:: No documentation written.\n\n")
      else:
         s.write("\n\n%s\n" % func.__doc__)
      self.method_example(func, level, images, s)

   def method_example(self, func, level, images, s):
      if len(func.doc_examples):
         s.write("\n----------\n\n")
      for i, doc_example in enumerate(func.doc_examples):
          if inspect.isroutine(doc_example):
              result = doc_example(images)
              src_image = None
              pixel_type = None
              s.write("**Example %d:** %s\n\n" % (i + 1, func.__name__))
          else:
              if len(doc_example):
                  if isinstance(func.self_type, args.ImageType):
                      pixel_type = doc_example[0]
                      pixel_type_name = util.get_pixel_type_name(pixel_type)
                      if pixel_type in (FLOAT, COMPLEX):
                         pixel_type_name = "GreyScale"
                      src_image = images[pixel_type].image_copy()
                      arguments = [src_image] + list(doc_example[1:])
                      doc_example = doc_example[1:]
                  else:
                      pixel_type = None
                      arguments = doc_example
              else:
                  pixel_type = None
                  arguments = []
              result = func.__call__(*tuple(arguments))
              s.write("**Example %d:** %s(%s)\n\n" %
                      (i + 1, func.__name__,
                       ", ".join([str(x) for x in doc_example])))
          if not pixel_type is None:
             s.write(".. image:: images/%s_generic.png\n\n" % pixel_type_name)
          result_filename = "%s_plugin_%02d" % (func.__name__, i)
          if isinstance(result, core.ImageBase):
             self.save_image(
                result, 
                os.path.join(self.docgen.output_images_path, result_filename + ".png"))
             s.write(".. image:: images/%s.png\n\n" % (result_filename))
          elif (result is None and
                isinstance(func.self_type, args.ImageType) and
                src_image is not None):
             self.save_image(
                src_image,
                os.path.join(self.docgen.output_images_path, result_filename + ".png"))
             s.write(".. image:: images/%s.png\n\n" % (result_filename))
          elif util.is_image_list(result):
             subst = "\n\n"
             for j, part in enumerate(result):
                result_filename = ("%s_plugin_%02d_%02d" %
                                   (func.__name__, i, j))
                self.save_image(
                   part,
                   os.path.join(self.docgen.output_images_path, result_filename + ".png"))
                s.write("|%s| " % result_filename)
                subst += (".. |%s| image:: images/%s.png\n" %
                          (result_filename, result_filename))
             s.write(subst)
             s.write("\n")
          else:
              s.write("*result* = " + repr(result) + "\n\n")
      s.write("\n\n")

   def save_image(self, image, filename):
      if image.data.pixel_type in (COMPLEX, FLOAT):
         image = _image_conversion.to_greyscale(image)
      _png_support.save_PNG(image, filename)

class ClassDocumentationGenerator:
   def __init__(self, docgen, classes):
      if not len(classes):
         return
      print "Generating class documentation"
      self.docgen = docgen
      self.class_names = []
      for cls in classes:
         self.document_class(cls)
      self.table_of_contents(classes)

   def document_class(self, cls_desc):
      module_name, cls_name, members = cls_desc
      combined_name = ".".join([module_name, cls_name])
      self.class_names.append((cls_name, combined_name))
      print "  ", combined_name
      if type(members) in (str, unicode):
         members = members.split()
      s = open(os.path.join(self.docgen.src_path, combined_name + ".txt"), "w")
      s.write("class ``%s``\n%s\n\n" % (combined_name, underline(0, combined_name, 10)))
      s.write("``%s``\n%s\n\n" % (cls_name, underline(1, combined_name, 4)))
      s.write(".. docstring:: %s %s\n   :no_title:\n\n" % (module_name, cls_name))
      s.write(".. docstring:: %s %s %s\n\n" % (module_name, cls_name, " ".join(members)))

   def table_of_contents(self, classes):
      s = open(os.path.join(self.docgen.src_path, "classes.txt"), "w")
      s.write("=======\nClasses\n=======\n\n")
      s.write("Alphabetical\n-------------\n")
      self.class_names.sort(lambda x, y: cmp(x.lower(), y.lower()))
      letter = '~'
      first = True
      links = []
      for name, combined_name in self.class_names:
          if name[0].upper() != letter:
              letter = name[0].upper()
              s.write("\n\n**")
              s.write(letter)
              s.write("**\n\n")
              first = True
          if not first:
              s.write(", ")
          s.write("%s_ (%s)" % (name, combined_name))
          first = False
          links.append(".. _%s: %s.html" % (name, combined_name))
      s.write("\n\n")
      s.write("\n".join(links))
      s.close()

# For extracting docstrings
def docstring(name, arguments, options, content, lineno,
              content_offset, block_text, state, state_machine):
   def import_helper(module_name, object_name):
      mod = __import__(module_name)
      components = module_name.split(".")[1:] + object_name.split(".")
      for comp in components:
         mod = getattr(mod, comp)
      return mod

   def do_docstring(obj, name):
      doc_string = inspect.getdoc(obj)
      if doc_string is None:
         text = "[Not documented]\n\n"
      else:
         text = doc_string + "\n\n"
      content = docutils.statemachine.string2lines(text, convert_whitespace=1)
      if not options.has_key('no_title'):
         title_text = "``%s``" % name
         textnodes, messages = state.inline_text(title_text, lineno)
         titles = [docutils.nodes.title(title_text, '', *textnodes)] + messages
         node = docutils.nodes.section(text, *titles)
         node['name'] = name
         state_machine.document.note_implicit_target(node, node)
      else:
         node = docutils.nodes.paragraph(text)
      content = docutils.statemachine.StringList(initlist=content, parent=node)
      state.nested_parse(content, 0, node)
      return node
      
   base_obj = import_helper(*arguments[:2])
   if len(arguments) == 2:
      name = arguments[1].split(".")[-1]
      return [do_docstring(base_obj, name)]
   nodes = []
   for name in arguments[2:]:
      try:
         obj = getattr(base_obj, name)
      except AttributeError:
         print "Warning: '%s' could not be found in '%s'" % (name, base_obj.__name__)
      else:
         nodes.append(do_docstring(obj, name))
   return nodes
docstring.arguments = (2,256,0)
docstring.options = { "no_title": docutils.parsers.rst.directives.flag }
docstring.content = 0
docutils.parsers.rst.directives.register_directive( 'docstring', docstring)

def copy_images(path_obj):
   if not os.path.exists(path_obj.output_images_path):
      os.mkdir(path_obj.output_images_path)
   for path in (path_obj.doc_images_path,
                path_obj.src_images_path,
                path_obj.icons_path):
      for file in glob.glob(path + "*.*"):
         path, filename = os.path.split(file)
         open(os.path.join(path_obj.output_images_path, filename), "wb").write(
             open(file, "rb").read())

def copy_css(path_obj):
    open(os.path.join(path_obj.output_path, "default.css"), "w").write(
       open(os.path.join(path_obj.src_path, "default.css"), "r").read())

class Paths:
    def __init__(self, root="."):
        join = os.path.join
        self.root_path = root
        self.src_path = join(root, "src/")
        self.src_images_path = join(self.src_path, "images/")
        self.icons_path = join(root, "../gamera/pixmaps/")
        self.output_path = join(self.root_path, "html/")
        self.output_images_path = join(self.output_path, "images/")

def print_usage():
    print "Gamera documentation generator."
    print "Usage: gendoc.py [-d doc_directory]"
    print "  (if doc_directory is omitted, the current directory"
    print "   will be used.)"
    print 
   
def gendoc(classes=[]):
   print_usage()
   opts, args = getopt.getopt(sys.argv[1:], "d:")
   root = '.'
   for flag, value in opts:
       if flag == "-d":
           root = value
   try:
      docgen = DocumentationGenerator(root, classes=classes)
   except Exception, e:
      print "Documentation generation failed with the following exception:"
      print e
   else:
      try:
         docgen.generate()
      except KeyboardInterrupt:
         print "Documentation generation stopped by user."
