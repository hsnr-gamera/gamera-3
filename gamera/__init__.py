import __version__
__version__ = __version__.ver

import config
config.parse()
               
import magic_import
magic_import.magic_import_setup()

