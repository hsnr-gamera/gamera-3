# vi:set tabsize=3:
#
# Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom,
#                          and Karl MacMillan
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

def graphviz_output(G, filename, string_function=str):
   """**graphviz_output** (*graph*, *filename*, *string_function* = str)

Writes a graph in the format used by the ``dot`` tool in the graphviz_ package for
graph visualisation.

.. _graphviz: http://www.graphviz.org/

*graph*
   A Gamera ``Graph`` object.

*filename*
   Filename to output to.

*string_function*
   A function to convert node values to strings.  By default this will use Python's
   standard ``str`` function.
"""
   fd = open(filename, 'w')
   if G.is_directed():
      fd.write("digraph G {\n")
      for node in G.get_nodes():
         fd.write('   "%s";\n' % 
                  string_function(node()))
         for edge in node.edges:
            fd.write('   "%s" -> "%s"' % (
               string_function(node()), string_function(edge.to_node())))
            if edge.label is not None:
               fd.write(' [ label = %f ]' % (edge.label))
            fd.write(';\n')
      fd.write("}\n")
   else:
      fd.write("graph G {\n")
      for node in G.get_nodes():
         fd.write('   "%s";\n' % 
                  string_function(node()))
      for edge in G.get_edges():
         fd.write('   "%s" -- "%s"' % (
            string_function(edge.from_node()), string_function(edge.to_node())))
         if edge.label is not None:
            fd.write(' [ label = %f ]' % (edge.label))
         fd.write(";\n")   
      fd.write("}\n")
   fd.close()
