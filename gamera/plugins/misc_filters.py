#

#

# Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan

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



from gamera.plugin import *



class outline(PluginFunction):

    """Traces the outline of the image.  This result is obtained by

dilating the image and then XOR'ing the result with the original."""

    self_type = ImageType([ONEBIT])

    return_type = ImageType([ONEBIT])

    doc_examples = [(ONEBIT,)]



class MiscFiltersModule(PluginModule):

    category = "Filter"

    functions = [outline]

    cpp_headers = ["misc_filters.hpp"]

    author = "Michael Droettboom and Karl MacMillan"

    url = "http://gamera.dkc.jhu.edu/"

module = MiscFiltersModule()
