#include "gameramodule.hpp"

void init_SizeType(PyObject* module_dict);
void init_PointType(PyObject* module_dict);
void init_DimensionsType(PyObject* module_dict);
void init_RectType(PyObject* module_dict);

extern "C" {
  void initgameracore(void);
}

PyMethodDef gamera_module_methods[] = {
  {NULL, NULL},
};

DL_EXPORT(void)
initgameracore(void) {
  PyObject* m = Py_InitModule("gameracore", gamera_module_methods);
  PyObject* d = PyModule_GetDict(m);
  init_SizeType(d);
  init_PointType(d);
  init_DimensionsType(d);
  init_RectType(d);
}
