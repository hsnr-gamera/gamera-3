from pyplate import *
from os import path
import os
import sys
from distutils.core import Extension
from gamera import paths

# magic_import and magic_import_setup
#
# This allows us to silently ignore importing modules prefixed
# with and underscore. This allows us to import plugins that
# depend on C++ moduels that are prefixed with underscores
# when those C++ modules don't yet exist (which is the case
# when we are generating and compiling the plugins!).
def magic_import(name, globals_={}, locals_={}, fromlist=[]):
  if (name[0] == '_' and name != "__future__") or name == "gamera":
    return None
  else:
    return std_import(name, globals_, locals_, fromlist)
  
def magic_import_setup():
  global std_import
  # Save the standard __import__ function so we can chain to it
  std_import = __builtins__['__import__']
  # Override the __import__ function with our new one
  __builtins__['__import__'] = magic_import

def restore_import():
  global std_import
  __builtins__['__import__'] = std_import


def generate_plugin(plugin_filename):
  
  template = Template("""
  [[exec import string]]
  [[exec from os import path]]
  #include \"gameramodule.hpp\"
  #include \"Python.h\"
  [[for header in module.cpp_headers]]
    #include \"[[header]]\"
  [[end]]
  using namespace Gamera;
  using namespace Gamera::Python;
  
  [[# The module name is prepended with an underscore #]]
  [[# exec module_name = "_" + __file__.split(".")[0] #]]
  [[exec plug_path, filename = path.split(__file__)]]
  [[exec module_name = '_' + filename.split('.')[0] ]]

  extern \"C\" {
    void init[[module_name]](void);
    [[for function in module.functions]]
      static PyObject* call_[[function.__class__.__name__]](PyObject* self, PyObject* args);
    [[end]]
  }

  static PyMethodDef [[module_name]]_methods[] = {
    [[for function in module.functions]]
      { \"[[function.__class__.__name__]]\",
        call_[[function.__class__.__name__]], METH_VARARGS },
    [[end]]
    { NULL }
  };

  static PyTypeObject* image_type;
  static PyTypeObject* subimage_type;
  static PyTypeObject* cc_type;
  static PyTypeObject* data_type;

  [[for function in module.functions]]
    [[exec pyarg_format = 'O']]
    static PyObject* call_[[function.__class__.__name__]](PyObject* self, PyObject* args) {
      PyObject* real_self;
      [[for x in function.args.list]]
        [[if isinstance(x, Int)]]
          int [[x.name + '_arg']];
          [[exec pyarg_format = pyarg_format + 'i']]
        [[elif isinstance(x, Float)]]
          double [[x.name + '_arg']];
          [[exec pyarg_format = pyarg_format + 'd']]
        [[elif isinstance(x, String)]]
          char* [[x.name + '_arg']];
          [[exec pyarg_format = pyarg_format + 'i']]
        [[elif isinstance(x, ImageType)]]
          PyObject* [[x.name + '_arg']];
          [[exec pyarg_format = pyarg_format + 'O']]
        [[end]]
      [[end]]

      [[if isinstance(function.return_type, Int)]]
        int return_value;
      [[elif isinstance(function.return_type, Float)]]
        double return_value;
      [[elif isinstance(function.return_type, String)]]
        char* return_value;
      [[elif isinstance(function.return_type, ImageType)]]
        Image* return_value;
      [[end]]

      if (PyArg_ParseTuple(args, \"[[pyarg_format]]\", &real_self,
      [[for i in range(len(function.args.list))]]
        [[if i != 0]],[[end]]
        &[[function.args.list[i].name + '_arg']]
      [[end]]
      ) <= 0)
        return 0;

      if (!PyObject_TypeCheck(real_self, image_type)) {
        PyErr_SetString(PyExc_TypeError, \"Object is not an image as expected!\");
        return 0;
      }
      [[exec tmp = [] ]]
      [[for type in function.self_type.pixel_types]]
        [[if type == 'OneBit']]
          [[exec tmp.append('OneBitRleImageView')]]
          [[exec tmp.append('RleCc')]]
          [[exec tmp.append('Cc')]]
        [[end]]
        [[exec tmp.append(type + 'ImageView')]]
      [[end]]
      [[exec function.self_type.pixel_types = tmp]]
      [[exec function.self_type.name = 'real_self']]
      [[exec images = [function.self_type] ]]
      [[for x in function.args.list]]
        [[if isinstance(x, ImageType)]]
          [[exec tmp = [] ]]
          [[for type in x.pixel_types]]
            [[if type == 'OneBit']]
              [[exec tmp.append('OneBitRleImageView')]]
              [[exec tmp.append('RleCc')]]
              [[exec tmp.append('Cc')]]
            [[end]]
            [[exec tmp.append(type + 'ImageView')]]
          [[end]]
          [[exec x.pixel_types = tmp]]
          [[exec images.append(x)]]
          if (!PyObject_TypeCheck([[x.name + '_arg']], image_type)) {
            PyErr_SetString(PyExc_TypeError, \"Object is not an image as expected!\");
            return 0;
          }
        [[end]]
      [[end]]

      [[def switch(layer, args)]]
        switch(get_image_combination([[images[layer].name]], cc_type)) {
          [[for type in images[layer].pixel_types]]
            [[exec current = '*((' + type + '*)((RectObject*)' + images[layer].name + ')->m_x)']]
            case [[type.upper()]]:
              [[if layer == len(images) - 1]]
                [[if function.return_type != None]]
                  return_value =
                [[end]]
                [[function.__class__.__name__]]
                (
                [[exec tmp_args = args + [current] ]]
                [[exec arg_string = tmp_args[0] + ', ']]
                [[exec current_image = 1]]
                [[for i in range(len(function.args.list))]]
                  [[if isinstance(function.args.list[i], ImageType)]]
                    [[exec arg_string += tmp_args[current_image]]]
                    [[exec current_image += 1]]
                  [[else]]
                    [[exec arg_string += function.args.list[i].name + '_arg']]
                  [[end]]
                  [[if i < len(function.args.list) - 1]]
                    [[exec arg_string += ', ']]
                  [[end]]
                [[end]]
                [[arg_string]]

                );
              [[else]]
                [[call switch(layer + 1, args + [current])]]
              [[end]]
            break;
          [[end]]
        }
      [[end]]
    [[call switch(0, [])]]

    [[if function.return_type == None]]
      Py_INCREF(Py_None);
      return Py_None;
    [[elif isinstance(function.return_type, ImageType)]]
      return create_ImageObject(return_value, image_type, subimage_type, cc_type, data_type);
    [[else]]
      return return_value;
    [[end]]
    }
  [[end]]


  DL_EXPORT(void) init[[module_name]](void) {
    Py_InitModule(\"[[module_name]]\", [[module_name]]_methods);
    PyObject* mod = PyImport_ImportModule(\"gamera.core\");
    if (mod == 0) {
      printf(\"Could not load gamera.py - falling back to gameracore\n\");
      mod = PyImport_ImportModule(\"gamera.gameracore\");
      if (mod == 0) {
        PyErr_SetString(PyExc_RuntimeError, \"Unable to load gameracore.\");
        return;
      }
    }
    PyObject* dict = PyModule_GetDict(mod);
    if (dict == 0) {
      PyErr_SetString(PyExc_RuntimeError, \"Unable to get module dictionary\");
      return;
    }
    image_type = (PyTypeObject*)PyDict_GetItemString(dict, \"Image\");
    subimage_type = (PyTypeObject*)PyDict_GetItemString(dict, \"SubImage\");
    cc_type = (PyTypeObject*)PyDict_GetItemString(dict, \"CC\");
    data_type = (PyTypeObject*)PyDict_GetItemString(dict, \"ImageData\");

  }
  
  """)
  
  import plugin
  magic_import_setup()

  plug_path, filename = path.split(plugin_filename)
  module_name = filename.split('.')[0]
  plugin_module = __import__(module_name)
  module_name = "_" + module_name
  cpp_filename = path.join(plug_path, module_name + ".cpp")
  output_file = open(cpp_filename, "w")
  sys.path.append(plug_path)
  #plugin_module.__dict__.update(locals())
  template.execute(output_file, plugin_module.__dict__)
  #os.system("indent -sob -kr " + cpp_filename)
  restore_import()

  # make the a distutils extension class for this plugin
  cpp_files = [cpp_filename]
  for file in plugin_module.module.cpp_sources:
    cpp_files.append(plug_path + file)
  return Extension("gamera.plugins." + module_name, cpp_files,
                   include_dirs=["include", plug_path],
                   libraries=["stdc++"])
