"""Data transfer between Gamera and OpenCV
"""

from gamera.plugin import *
from gamera import config

try:
    import cv
except ImportError:
    try:
        verbose = config.get("verbosity_level")
    except Exception:
        verbose = 0
    if verbose:
        print ('Info: OpenCV Library cv '
               'could not be imported')
    
class from_cv(PluginFunction):
    """
    Instantiates a Gamera RGB image from an OpenCV
    cv.cvmat or cv.iplimage. May fail for very large images.

    Usage example:

    .. code:: Python

      from gamera.plugins import cv_io

      # read a JPEG image and convert it to a Gamera image
      cvImg = cv.LoadImage("foo.jpg")
      img = cv_io.from_cv(cvImg)
    """
    self_type = None
    return_type = ImageType([GREYSCALE, RGB, FLOAT])
    args = Args([Class("image")])
    pure_python = True

    def __call__(image, offset=None):
        from gamera.plugins import _string_io
        from gamera.core import Dim, Point, RGBPixel
        
        if offset is None:
          offset = Point(0, 0)
        if isinstance(image,cv.cvmat):
          imgcv = cv.CloneMat(image)
          cv.CvtColor(image, imgcv, cv.CV_BGR2RGB)
          return  _string_io._from_raw_string(
            offset, 
            Dim(imgcv.cols,imgcv.rows), 
            RGB, DENSE, 
            imgcv.tostring())
        elif isinstance(image,cv.iplimage):
          imgcv = cv.CreateImage(cv.GetSize(image), image.depth, image.channels)
          cv.CvtColor(image, imgcv, cv.CV_BGR2RGB)
          return _string_io._from_raw_string(
            offset, 
            Dim(imgcv.width,imgcv.height), 
            RGB, DENSE, 
            imgcv.tostring())
        else:
          raise TypeError("Image must be of type cv.cvmat or cv.iplimage")
          
    __call__ = staticmethod(__call__)

class to_cv(PluginFunction):
    """
    Returns an OpenCv image (cv.cvmat) containing a copy of
    image's data.

    Only RGB, Greyscale and Onebit images are supported.
    May fail for very large images.
    """
    self_type = ImageType([RGB, GREYSCALE, ONEBIT])
    return_type = Class("cv_image")
    def __call__(image):
        from gamera.plugins import _string_io
        
        if image.data.pixel_type == ONEBIT:
          image = image.to_rgb()
        if image.data.pixel_type == GREYSCALE:
          image = image.to_rgb()
        numImage = image.to_numeric()
        imgcv = cv.fromarray(numImage)
        cv.CvtColor(imgcv, imgcv, cv.CV_BGR2RGB)
        return imgcv
        
    __call__ = staticmethod(__call__)
  

class CVioModule(PluginModule):
    category = "ExternalLibraries/cv"
    author = "Manuel Jeltsch"
    functions = [from_cv, to_cv]
    pure_python = True
    url = ('')
module = CVioModule()

from_cv = from_cv()
