from distutils.core import setup, Extension

setup(name = "gameracore", version="1.1",
      ext_modules = [Extension("gameracore", ["src/gameramodule.cpp",
                                          "src/sizeobject.cpp",
                                          "src/pointobject.cpp",
                                          "src/dimensionsobject.cpp",
                                          "src/rectobject.cpp",
                                          "src/regionobject.cpp",
                                          "src/regionmapobject.cpp",
                                          "src/rgbpixelobject.cpp",
                                          "src/imagedataobject.cpp",
                                          "src/imageobject.cpp"
                                          ], include_dirs=["include"],
                               libraries=["stdc++"]),
                     Extension("knn", ["src/knnmodule.cpp"],
                               include_dirs=["include"], libraries=["stdc++"])]
      )
