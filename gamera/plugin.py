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

from gamera.args import *
from gamera import paths
import gamera.core
import unittest, new, os, os.path, imp

class PluginModule:
    category = "Miscellaneous"
    cpp_sources = []
    cpp_headers = []
    cpp_defines = []
    functions = []
    version = "1.0"
    author = ""
    url = ""

    def __init__(self):
        for function in self.functions:
            function.register(self.category)

    def generate(cls):
        for x in cls.functions:
            print x.__class__.__name__, x.self_type.pixel_types
    generate = classmethod(generate)

class PluginFunction:
    return_type = None
    self_type = ImageType(("RGB", "GreyScale", "Grey16", "Float", "OneBit"))
    args = Args([])
    image_types_must_match = 0

    def register(cls, category='Miscellaneous'):
        if hasattr(cls, 'category'):
            category = cls.category
        if not hasattr(cls, "__call__"):
            parts = cls.__module__.split('.')
            cpp_module_name = '.'.join(parts[:-1] + ['_' + parts[-1]])
            module = __import__(cpp_module_name,
                                locals(),
                                globals())
            if module == None:
                return
            func = getattr(module,
                           cls.__name__)
        elif cls.__call__ is None:
            func = None
        else:
            func = new.function(cls.__call__.func_code,
                                cls.__call__.func_globals,
                                cls.__name__)
        cls.__call__ = staticmethod(func)
        if isinstance(cls.self_type, ImageType):
            gamera.core.Image.add_plugin_method(cls, func, category)
    register = classmethod(register)

    def test(cls):
        # Testing function goes here
        results = []
        if cls.image_types_must_match:
            for type in cls.self_type.pixel_types:
                self = get_test_image(type + "_generic.tiff")
                params = []
                for i in cls.args:
                    if isinstance(i, ImageType):
                        param = testing.get_test_image(type + "_generic.tiff")
                    else:
                        param = i.default
                    params.append(param)
                cls._do_test(self, params, results)
        else:
            for type in cls.self_type.pixel_types:
                self = get_test_image(type + "_generic.tiff")
                cls._test_recurse(self, [], results)
        return results
                
    test = classmethod(test)

    def _test_recurse(cls, self, params, results):
        if len(params) == len(cls.args):
            cls._do_test(self, params, results)
        else:
            arg = cls.args[level]
            if isinstance(arg, ImageType):
                for i in arg.pixel_types:
                    param = get_test_image(type + "_generic.tiff")
                    params.append(param)
                    cls._test_recurse(self, params, results)
            else:
                param = arg.default
                params.append(param)
                cls._test_recurse(self, params, results)
                    
    _test_recurse = classmethod(_test_recurse)

    def _do_test(cls, self, params, results):
        try:
            result = apply(getattr(self, cls.__name__), tuple(params))
        except Exception:
            results.append(str(Exception))
        else:
            results.append(result)
            results.append(self)
            for i in params:
                results.append(i)

    _do_test = classmethod(_do_test)

def get_test_image(filename):
    # TODO: should search test image paths
    filename = os.path.join(paths.test, filename)
    return gamera.core.load_image(filename)

def get_result_image(filename):
    filename = os.path.join(paths.test_results, filename)
    return gamera.core.load_image(filename)

def save_test_image(image, name, no):
    filename = "%s.plugin.%04d.results.tiff" % (name, no)
    image.save_image(os.path.join(paths.test_results, filename))
    return filename

class PluginTest(unittest.TestCase):
    def __init__(self, plugin):
        self.plugin = plugin
        self.plugin_class = plugin.__class__

    def _results_filename(self):
        return os.path.join(paths.test_results,
                            self.plugin_class.__name__ + ".plugin.results")
        
    def generateResults(self):
        results = self.plugin.test()
        fd = file(self._results_filename(), "w")
        image_file_no = 0
        for result in results:
            if isinstance(result, gamera.core.Image):
                image_file_name = save_test_image(result,
                                                  self.plugin_class.__name__,
                                                  image_file_no)
                fd.write("Image File: %s\n" % image_file_name)
                image_file_no += 1
            else:
                fd.write(str(result) + "\n")
        fd.close()
                        
    def runTest(self):
        try:
            fd = file(self._results_filename(), "r")
        except IOError:
            print "WARNING: Results file not found for %s plugin.  Generating a new results file..." % self.plugin_class.__name__
            self.generateResults()
            return
        results = self.plugin.test()
        compares = [x.strip() for x in fd.readlines()]
        fd.close()
        self.failUnless(len(results) == len(compares),
                        "Different number of results than expected.")
        for result, compare in zip(results, compares):
            print result, compare
            if (compare.startswith("Image File: ") and
                isinstance(result, gamera.core.Image)):
                image = get_result_image(compare[12:])
                image.display()
                result.display()
                self.failUnless(image.compare(result))
            else:
                self.failUnless(str(compare) == str(result))

    def shortDescription(self):
        return "Automatically generated test of the %s plugin." % self.plugin_class.__name__

def PluginFactory(name, func, category=None,
                  return_type=None,
                  self_type=ImageType((gamera.core.RGB,
                                       gamera.core.GREYSCALE,
                                       gamera.core.GREY16,
                                       gamera.core.ONEBIT)),
                  args=None):
    cls = new.classobj(name, (PluginFunction,), {})
    if not category is None:
        cls.category = category
    cls.return_type = return_type
    cls.self_type = self_type
    cls.args = args
    cls.__call__ = func
    return cls
