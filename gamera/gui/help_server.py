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

from wxPython.wx import *
from wxPython.html import *

from gamera import util, config
from gamera.plugin import PluginFunction
from gamera import pyplate
from gamera import paths

from string import join
import inspect

header = """

[[exec from gamera.core import *]]
[[exec demarcation_color = '#dddddd']]

[[def header(title)]]
<html>
<head>
<style>
h1, h2, h3 {background-color: [[demarcation_color]]}
h2 span    {float: right}
body       {background-color: #ffffff}
</style>
<title>[[title]]</title>
</head>
<body>
<h1>[[title]]</h1>
[[end]]"""

footer = """
[[def footer(x)]]
</body>
</html>
[[end]]"""

plugin_page = pyplate.Template(
    header + footer +
    """
    
    [[call header(__module__ + ' module')]]
    <ul>
    [[for function in functions]]
      <h2>
        [[function.__class__.__name__]]([[ ', '.join([x.html_repr() for x in function.args]) ]])
        <span>
          [[for pixel_type in function.self_type.pixel_types]]
            [[get_pixel_type_name(pixel_type)]]
          [[end]]
        </span>
      </h2>
    [[end]]
    </ul>
    [[call footer(5)]]
    """)

def generate_html():
    for module in paths.get_directory_of_modules(paths.plugins):
        members = {}
        for key, val in inspect.getmembers(module.module):
            members[key] = val
        plugin_page.execute_file(os.path.join(paths.doc, module.__name__ + ".html"), members)
        
        
if __name__ == "__main__":
    generate_html()
