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

from pydoc import ErrorDuringImport, locate, describe, html, pathdirs
from string import join

def serve(port, callback=None, completer=None):
    import BaseHTTPServer, mimetools, select

    # Patch up mimetools.Message so it doesn't break if rfc822 is reloaded.
    class Message(mimetools.Message):
        def __init__(self, fp, seekable=1):
            Message = self.__class__
            Message.__bases__[0].__bases__[0].__init__(self, fp, seekable)
            self.encodingheader = self.getheader('content-transfer-encoding')
            self.typeheader = self.getheader('content-type')
            self.parsetype()
            self.parseplist()

    class DocHandler(BaseHTTPServer.BaseHTTPRequestHandler):
        def send_document(self, title, contents):
            try:
                self.send_response(200)
                self.send_header('Content-Type', 'text/html')
                self.end_headers()
                self.wfile.write(html.page(title, contents))
            except IOError: pass

        def do_GET(self):
            path = self.path
            if path[-5:] == '.html': path = path[:-5]
            if path[:1] == '/': path = path[1:]

            parts = path.split('/')
            if parts[0] == 'plugin':
                module = __import__(parts[1])
                self.send_document(parts[1] + " module", module.module.describe_html())
                return
            if path and path != '.':
                try:
                    obj = locate(path, forceload=1)
                except ErrorDuringImport, value:
                    self.send_document(path, html.escape(str(value)))
                    return
                if obj:
                    self.send_document(describe(obj), html.document(obj, path))
                else:
                    self.send_document(path,
'no Python documentation found for %s' % repr(path))
            else:
                heading = html.heading(
'<big><big><strong>Gamera</strong></big></big>',
'#ffffff', '#7799ee')
                def bltinlink(name):
                    return '<a href="%s.html">%s</a>' % (name, name)
                names = filter(lambda x: x != '__main__',
                               sys.builtin_module_names)
                contents = html.multicolumn(names, bltinlink)
                indices = ['<p>' + html.bigsection(
                    'Built-in Modules', '#ffffff', '#ee77aa', contents)]

                seen = {}
                for dir in pathdirs():
                    indices.append(html.index(dir, seen))
                contents = heading + join(indices) + '''<p align=right>
<font color="#909090" face="helvetica, arial"><strong>
pydoc</strong> by Ka-Ping Yee &lt;ping@lfw.org&gt;</font>'''
                self.send_document('Index of Modules', contents)

        def log_message(self, *args): pass

    class DocServer(BaseHTTPServer.HTTPServer):
        def __init__(self, port, callback):
            host = (sys.platform == 'mac') and '127.0.0.1' or 'localhost'
            self.address = ('', port)
            self.url = 'http://%s:%d/' % (host, port)
            self.callback = callback
            self.base.__init__(self, self.address, self.handler)

        def serve_until_quit(self):
            import select
            self.quit = 0
            while not self.quit:
                rd, wr, ex = select.select([self.socket.fileno()], [], [], 1)
                if rd: self.handle_request()

        def server_activate(self):
            self.base.server_activate(self)
            if self.callback: self.callback(self)

    DocServer.base = BaseHTTPServer.HTTPServer
    DocServer.handler = DocHandler
    DocHandler.MessageClass = Message
    try:
        try:
            DocServer(port, callback).serve_until_quit()
        except (KeyboardInterrupt, select.error):
            pass
    finally:
        if completer: completer()

class History:
   size = 100
    
   def __init__(self):
      self.data = [None] * self.size
      self.pointer = 0

   def add_element(self, element):
      self.data[pointer] = element
      self.pointer += 1
      self.pointer %= self.size

   def go_back(self):
      if self.can_go_back():
         self.pointer -= 1
         self.pointer %= self.size
         return self.data[self.pointer]

   def go_forward(self):
      if self.can_go_forward():
         self.pointer += 1
         self.pointer %= self.size
         return self.data[self.pointer]
   
   def can_go_back(self):
      return self.data[(self.pointer - 1) % self.size] != None

   def can_go_forward(self):
      return self.data[(self.pointer + 1) % self.size] != None

class HelpWindow(wxHtmlWindow):
   def __init__(self, parent):
      wxHtmlWindow.__init__(self, parent, -1)
      self._parent = parent

   def OnOpeningURL(self, type, url):
      if not ':' in url:
          url = ("http://127.0.0.1:%d/%s" %
                 (config.get_option("help_server_port"),
                  url))
          wxHtmlWindow.LoadPage(self, url)
      return wxHTML_OPEN

   def OnSetTitle(self, title):
      self._parent.adjust_toolbar()
      self._parent.SetTitle(title)

class HelpFrame(wxFrame):
   def __init__(self, parent, owner):
      self._history = History()
      wxFrame.__init__(self, parent, -1, "Gamera help window",
                       (0, 0), (600,600))
      self.window = HelpWindow(self)
      self._create_toolbar()
      self.Show()
      EVT_CLOSE(self, self.OnClose)
      self._owner = owner
      self._owner.help_window = self

   def _create_toolbar(self):
      tools = (("GoBack", wxART_GO_BACK),
               ("GoForward", wxART_GO_FORWARD))
      self.toolbar = self.CreateToolBar(wxTB_HORIZONTAL, -1)
      for i, tool in enumerate(tools):
         self.toolbar.AddSimpleTool(
             10 + i, wxArtProvider_GetBitmap(tool[1], wxART_TOOLBAR, (16, 16)),
             tool[0])
         EVT_TOOL(self, 10 + i, getattr(self, 'On' + tool[0]))
      self.adjust_toolbar()

   def OnGoBack(self, event):
      self.window.HistoryBack()
      self.adjust_toolbar()

   def OnGoForward(self, event):
      self.window.HistoryForward()
      self.adjust_toolbar()

   def adjust_toolbar(self):
      print "Adjusting" 
      self.toolbar.EnableTool(10, self.window.HistoryCanBack())
      self.toolbar.EnableTool(11, self.window.HistoryCanForward())
       
   def OnClose(self, event):
      self._owner.help_window = None
      self.Destroy()

   def load_page(self, page):
      self.window.LoadPage("http://127.0.0.1:%d/%s" %
                           (config.get_option("help_server_port"),
                            page))
      self.Raise()

   def set_page(self, content):
      self.window.SetPage(content)
      self.Raise()
