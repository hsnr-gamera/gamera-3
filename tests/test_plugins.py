from gamera.core import *
init_gamera()
from gamera import gendoc
from gamera import plugin

# This is a fairly complex test harness, which generates
# a "wrapper" call around every plugin so each plugin can
# be tested separately by py.test

class PluginTester(gendoc.PluginDocumentationGenerator):
   def __init__(self):
      self.images = self.get_generic_images()
      self.methods = plugin._methods_flatten(plugin.plugin_methods)

class TestPlugins:
   pass

def make_test(tester, method):
   def test(self):
      tester.run_example(method, tester.images)
   return test

tester = PluginTester()
for name, method in tester.methods:
   setattr(TestPlugins, "test_plugin_" + name, make_test(tester, method))
