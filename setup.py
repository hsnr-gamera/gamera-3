from distutils.core import setup, Extension

setup(name = "gameracore", version="1.1",
      ext_modules = [Extension("gamera", ["src/gameramodule.cpp",
                                          "src/sizeobject.cpp",
                                          "src/pointobject.cpp",
                                          "src/dimensionsobject.cpp",
                                          "src/rectobject.cpp"
                                          ], include_dirs=["include"])]
      )
