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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#

def glyphs_comma_delim(glyphs, filename):
    """Export the id_name and features of a list of
    glyphs to a comma delimited format"""
    file = open(filename, "w")
    for x in glyphs:
        file.write(x.id_name[0][1])
        for f in x.features:
            file.write(',')
            file.write(str(f))
        file.write('\n')
    file.close()

def glyphs_space_delim(glyphs, filename):
    """Export the id_name and features of a list of
    glyphs to a space delimited format"""
    file = open(filename, "w")
    for x in glyphs:
        file.write(x.id_name[0][1])
        for f in x.features:
            file.write(' ')
            file.write(str(f))
        file.write('\n')
    file.close()
