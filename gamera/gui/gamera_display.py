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

from wxPython.wx import *        # wxPython
from wxPython.grid import * 
from math import ceil, log       # Python standard library
from sys import maxint
import string
from gamera.core import *             # Gamera specific
from gamera import paths, util
from gamera.gui import image_menu, var_name, gui_util

##############################################################################

# we want this done on import
wxInitAllImageHandlers()

#############################################################################

##############################################################################
# SINGLE IMAGE DISPLAY
##############################################################################
#
# Image display is a scrolled window for displaying a single image

class ImageDisplay(wxScrolledWindow):
   def __init__(self, parent, id = -1, size = wxDefaultSize):
      wxScrolledWindow.__init__(self, parent, id, wxPoint(0, 0), size,
                                wxSUNKEN_BORDER)
      self.SetBackgroundColour(wxWHITE)
      self.width = 0
      self.height = 0
      self.scaling = 1.0
      self.image = None
      self.highlighted = []
      self.tmpDC = wxMemoryDC()
      self.tmpDC.SetPen(wxTRANSPARENT_PEN)
      self.color = 0
      self.menu = None
      self.rubber_on = 0
      self.rubber_origin_x = 0
      self.rubber_origin_y = 0
      self.rubber_x2 = 0
      self.rubber_y2 = 0
      self.click_callbacks = []
      self.current_color = 0
      self.dragging = 0
      self.scroll_rate = 10
      EVT_PAINT(self, self.OnPaint)
      EVT_SIZE(self, self.OnResize)
      EVT_LEFT_DOWN(self, self.OnLeftDown)
      EVT_LEFT_UP(self, self.OnLeftUp)
      EVT_MIDDLE_DOWN(self, self.OnMiddleDown)
      EVT_MIDDLE_UP(self, self.OnMiddleUp)
      EVT_MOTION(self, self.OnMotion)
      EVT_LEAVE_WINDOW(self, self.OnLeave)

   ########################################
   # THE MAIN IMAGE

   # Sets the image being displayed
   # Returns the size of the image
   def set_image(self, image, function):
      self.image = image
      self.to_string_function = function
      return self.reload_image()

   # Refreshes the image by recalling the to_string function
   def reload_image(self):
      self.width = self.image.width
      self.height = self.image.height
      self.scale()
      return (self.image.width, self.image.height)

   def add_click_callback(self, cb):
      self.click_callbacks.append(cb)

   def remove_click_callback(self, cb):
      self.click_callbacks.remove(cb)

   ########################################
   # HIGHLIGHTED CCs

   def highlight_rectangle(self, y, x, h, w, color=-1, text=''):
      if y < 0 or x < 0:
         return
      if color == -1:
         color = self.color
         self.color = self.color + 1
      rect = (x, y, w + 1, h + 1)
      self.highlighted.append((rect, None, gui_util.get_color(color)))
      if self.scaling <= 1:
         self.PaintArea(*rect)

   
   # Highlights a CC in the display.  Multiple CCs
   # may be highlighted in turn
   def highlight_cc(self, ccs, function, color=-1):
      if not util.is_sequence(ccs):
         ccs = (ccs,)
      for cc in ccs:
         if color == -1:
            color = self.color
            self.color = self.color + 1
         image = wxEmptyImage(cc.ncols, cc.nrows)
         image.SetData(apply(function, (cc,)))
         bmp = wxBitmapFromImage(image, 1)
         rect = (cc.page_offset_x(), cc.page_offset_y(),
                 cc.ncols, cc.nrows)
         self.highlighted.append((rect, bmp, gui_util.get_color(color)))
         self.PaintArea(*rect)

   # Clears a CC in the display.  Multiple CCs
   # may be highlighted in turn
   def clear_highlight_cc(self, ccs):
      if not util.is_sequence(ccs):
         ccs = (ccs,)
      for cc in ccs:
         for i in range(len(self.highlighted)):
            if (self.highlighted[i][0][0] == cc.page_offset_x() and
                self.highlighted[i][0][1] == cc.page_offset_y()):
               del self.highlighted[i]
               self.PaintArea(cc.page_offset_x(),
                              cc.page_offset_y(),
                              cc.ncols,
                              cc.nrows)
               break
      
   # Clears all of the highlighted CCs in the display   
   def clear_all_highlights(self):
      old_highlighted = self.highlighted[:]
      self.highlighted = []
      if self.scaling <= 1:
         for highlight in old_highlighted:
            self.PaintArea(*highlight[0])
      if self.scaling > 1:
         self.Refresh(0, rect=wxRect(0,0,self.GetSize().x,self.GetSize().y))

   # Adjust the scrollbars so a group of highlighted subimages are visible
   def focus(self, subimages):
      if not util.is_sequence(subimages):
         subimages = (subimages,)
      x1 = y1 = maxint
      x2 = y2 = 0
      # Get a combined rectangle of all images in the list
      for image in subimages:
         x1 = min(image.page_offset_x(), x1)
         y1 = min(image.page_offset_y(), y1)
         x2 = max(image.page_offset_x() + image.ncols, x2)
         y2 = max(image.page_offset_y() + image.nrows, y2)
      # Adjust for the scaling factor
      x1 = x1 * self.scaling
      y1 = y1 * self.scaling
      x2 = x2 * self.scaling
      y2 = y2 * self.scaling
      origin = self.GetViewStart()
      maxwidth = self.image.width * self.scaling
      maxheight = self.image.height * self.scaling
      need_to_scroll = 0
      if (x1 < origin[0] or
          x2 > origin[0] + self.GetSize().x / self.scaling):
         set_x = max(0, min(maxwidth - self.GetSize()[0] / self.scaling,
                            x1 - 25))
         need_to_scroll = 1
      else:
         set_x = origin[0]
      if (y1 < origin[1] or
          y2 > origin[1] + self.GetSize().y / self.scaling or
          need_to_scroll):
         set_y = max(0, min(maxheight - self.GetSize()[1] / self.scaling,
                            y1 - 25))
         need_to_scroll = 1
      else:
         set_y = origin[1]
      if need_to_scroll or self.scaling > 1:
         self.SetScrollbars(1,1,maxwidth,maxheight,set_x,set_y)
         self.Refresh(0, rect=wxRect(0,0,self.GetSize().x,self.GetSize().y))

   ########################################
   # SCALING
   #
   def scale(self, scale=None):
      if scale == None:
         scale = self.scaling
      w = self.width * scale - 1
      h = self.height * scale - 1
      x = max(min(self.GetViewStart()[0] * scale / self.scaling + 1,
                  w - self.GetSize().x) - 2, 0)
      y = max(min(self.GetViewStart()[1] * scale / self.scaling + 1,
                  h - self.GetSize().y) - 2, 0)
      self.scaling = scale
      self.SetScrollbars(1,1,w,h,x,y)
      self.Refresh(0, rect=wxRect(0,0,self.GetSize().x,self.GetSize().y))

   ########################################
   # CALLBACKS
   #
   def ZoomOut(self):
      if self.scaling > 0.0625:
         self.scale(self.scaling * 0.5)

   def ZoomIn(self):
      if self.scaling < 32:
         self.scale(self.scaling * 2.0)

   def OnResize(self, event):
      size = self.GetSize()
      self.tmpDC.SelectObject(wxEmptyBitmap(
         size.GetWidth(), size.GetHeight()))
      event.Skip()
      self.scale()

   def OnPaint(self, event):
      origin = self.GetViewStart()
      if self.image:
         update_regions = self.GetUpdateRegion()
         rects = wxRegionIterator(update_regions)
         # Paint only where wxWindows tells us to, this is faster
         while rects.HaveRects():
            ox = rects.GetX() / self.scaling
            oy = rects.GetY() / self.scaling
            x = (rects.GetX() + origin[0]) / self.scaling
            y = (rects.GetY() + origin[1]) / self.scaling
            if x > self.width or y > self.height:
               pass
            else:
               # For some reason, the rectangles wxWindows gives are
               # a bit too small, so we need to fudge their size
               fudge = self.scaling / 2.0
               w = max(min(int((rects.GetW() / self.scaling) + fudge),
                           self.width - ox), 0)
               h = max(min(int((rects.GetH() / self.scaling) + fudge),
                           self.height - oy), 0)
               self.PaintArea(x, y, w, h, check=0)
            rects.Next()
         self.draw_rubber()

   def PaintArea(self, x, y, w, h, check=1):
      dc = wxPaintDC(self)
      dc.SetLogicalFunction(wxCOPY)
      origin = self.GetViewStart()
      size = self.GetSizeTuple()
      ox = x - (origin[0] / self.scaling)
      oy = y - (origin[1] / self.scaling)
      if check:
         if ((x + w < (origin[0] / self.scaling) and
              y + h < (origin[1] / self.scaling)) or
             (x > (origin[0] + size[0]) / self.scaling and
              y > (origin[1] + size[1]) / self.scaling) or
             (w == 0 or h == 0)):
            return
      self.tmpDC.SetUserScale(self.scaling, self.scaling)
      if (y + h > self.image.nrows):
         h = self.image.nrows - y
      if (x + w > self.image.ncols):
         w = self.image.ncols - x
      subimage = SubImage(self.image, y, x, h, w)
      image = apply(wxEmptyImage, (w, h))
      image.SetData(apply(self.to_string_function, (subimage, )))
      bmp = image.ConvertToBitmap()
      self.tmpDC.DrawBitmap(bmp, 0, 0, 0)

      if len(self.highlighted):
         # Workaround, since wxPython's compositing is different
         # when scaling and not scaling.  Grr!!
         fudge = int(self.scaling / 2.0)
         if self.scaling == 1:
            self.tmpDC.SetTextBackground(wxBLACK)
            self.tmpDC.SetBackgroundMode(wxTRANSPARENT)
            self.tmpDC.SetLogicalFunction(wxOR)
            for rect, bmp, color in self.highlighted:
               if ((rect[0] <= x + w and
                    rect[0] + rect[2] >= x) and
                   (rect[1] <= y + h and
                    rect[1] + rect[3] >= y)):
                  if bmp != None:
                     self.tmpDC.SetTextForeground(color)
                     self.tmpDC.DrawBitmap(bmp,
                                           rect[0] - x + fudge,
                                           rect[1] - y + fudge, 0)
                  else:
                     self.tmpDC.SetBrush(wxBrush(color, wxSOLID))
                     self.tmpDC.DrawRectangle(rect[0] - x + fudge,
                                              rect[1] - y + fudge,
                                              rect[2], rect[3])
         else:
            self.tmpDC.SetTextForeground(wxBLACK)
            self.tmpDC.SetBackgroundMode(wxSOLID)
            self.tmpDC.SetLogicalFunction(wxOR)
            for rect, bmp, color in self.highlighted:
               if ((rect[0] <= x + w and
                    rect[0] + rect[2] >= x) and
                   (rect[1] <= y + h and
                    rect[1] + rect[3] >= y)):
                  if bmp != None:
                     self.tmpDC.SetTextBackground(color)
                     self.tmpDC.DrawBitmap(bmp,
                                           rect[0] - x + fudge,
                                           rect[1] - y + fudge, 0)
                  else:
                     self.tmpDC.SetBrush(wxBrush(color, wxSOLID))
                     self.tmpDC.DrawRectangle(rect[0] - x + fudge,
                                              rect[1] - y + fudge,
                                              rect[2], rect[3])
         self.tmpDC.SetBackgroundMode(wxSOLID)
         self.tmpDC.SetLogicalFunction(wxCOPY)

      self.tmpDC.SetUserScale(1, 1)
      dc.Blit(ox * self.scaling, oy * self.scaling,
              w * self.scaling, h * self.scaling,
              self.tmpDC, 0, 0)

   ############################################################
   # RUBBER BAND
   def OnLeftDown(self, event):
      self.rubber_on = 1
      self.draw_rubber()
      origin = self.GetViewStart()
      x = min((event.GetX() + origin[0]) / self.scaling,
              self.image.ncols - 1)
      y = min((event.GetY() + origin[1]) / self.scaling,
              self.image.nrows - 1)
      if (x > self.rubber_origin_x and
          x < self.rubber_origin_x + self.block_w):
         self.rubber_origin_x, self.rubber_x2 = \
                               self.rubber_x2, self.rubber_origin_x
         if (y > self.rubber_origin_y and
             y < self.rubber_origin_y + self.block_h):
            self.rubber_origin_y, self.rubber_y2 = \
                                  self.rubber_y2, self.rubber_origin_y
            self.draw_rubber()
            return
         elif (y < self.rubber_y2 and
               y > self.rubber_y2 - self.block_h):
            self.draw_rubber()
            return
      elif (x < self.rubber_x2 and x > self.rubber_x2 - self.block_w):
         if (y > self.rubber_origin_y and
             y < self.rubber_origin_y + self.block_h):
            self.rubber_origin_y, self.rubber_y2 = \
                                  self.rubber_y2, self.rubber_origin_y
            self.draw_rubber()
            return
         elif (y < self.rubber_y2 and
               y > self.rubber_y2 - self.block_h):
            self.draw_rubber()
            return
      self.rubber_origin_x = self.rubber_x2 = x
      self.rubber_origin_y = self.rubber_y2 = y

   def OnLeftUp(self, event):
      if self.rubber_on:
         self.draw_rubber()
         origin = self.GetViewStart()
         self.rubber_x2 = min((event.GetX() + origin[0]) / self.scaling,
                              self.image.ncols - 1)
         self.rubber_y2 = min((event.GetY() + origin[1]) / self.scaling,
                              self.image.nrows - 1)
         self.draw_rubber()
         try:
            for i in self.click_callbacks:
               i(self.rubber_y2, self.rubber_x2)
         except Exception, e:
            print e
         self.OnLeave(event)

   def draw_rubber(self):
      dc = wxPaintDC(self)
      origin = self.GetViewStart()
      x = min(self.rubber_origin_x, self.rubber_x2)
      y = min(self.rubber_origin_y, self.rubber_y2)
      x2 = max(self.rubber_origin_x, self.rubber_x2)
      y2 = max(self.rubber_origin_y, self.rubber_y2)
      x = x * self.scaling - origin[0]
      y = y * self.scaling - origin[1]
      x2 = x2 * self.scaling - origin[0]
      y2 = y2 * self.scaling - origin[1]
      w = x2 - x
      h = y2 - y
      dc.SetLogicalFunction(wxXOR)
      pen = wxGREY_PEN
      pen.SetStyle(wxSHORT_DASH)
      dc.SetPen(pen)
      dc.SetBrush(wxTRANSPARENT_BRUSH)
      dc.DrawRectangle(x, y, w, h)
      dc.SetPen(wxTRANSPARENT_PEN)
      brush = wxBLUE_BRUSH
      brush.SetColour(wxColor(167, 105, 39))
      dc.SetBrush(brush)
      self.block_w = block_w = min(w / 2, 8)
      self.block_h = block_h = min(h / 2, 8)
      dc.DrawRectangle(x + 1, y + 1, block_w, block_h)
      dc.DrawRectangle(x2 - block_w - 1, y + 1, block_w, block_h)
      dc.DrawRectangle(x + 1, y2 - block_h - 1, block_w, block_h)
      dc.DrawRectangle(x2 - block_w - 1, y2 - block_h - 1, block_w, block_h)
      dc.SetLogicalFunction(wxCOPY)

   def OnMiddleDown(self, event):
      self.dragging = 1
      self.dragging_x = event.GetX()
      self.dragging_y = event.GetY()
      self.dragging_origin_x, self.dragging_origin_y = self.GetViewStart()

   def OnMiddleUp(self, event):
      self.dragging = 0

   def OnMotion(self, event):
      if self.rubber_on:
         self.draw_rubber()
         origin = self.GetViewStart()
         self.rubber_x2 = min((event.GetX() + origin[0]) / self.scaling,
                              self.image.ncols - 1)
         self.rubber_y2 = min((event.GetY() + origin[1]) / self.scaling,
                              self.image.nrows - 1)
         self.draw_rubber()
      if self.dragging:
         self.Scroll(self.dragging_origin_x - (event.GetX() - self.dragging_x),
                     self.dragging_origin_y - (event.GetY() - self.dragging_y))

   def OnLeave(self, event):
      if self.rubber_on:
         if self.rubber_origin_x > self.rubber_x2:
            self.rubber_origin_x, self.rubber_x2 = self.rubber_x2, self.rubber_origin_x
         if self.rubber_origin_y > self.rubber_y2:
            self.rubber_origin_y, self.rubber_y2 = self.rubber_y2, self.rubber_origin_y
         self.rubber_on = 0
         self.OnRubber()
      if self.dragging:
         self.dragging = 0

   def OnRubber(self):
      #deliberately empty -- this is a callback to be overridden by subclasses
      pass

   def MakeView(self):
      name = var_name.get("view", image_menu.shell.locals)
      if name:
         image_menu.shell.locals['__image__'] = self.image
         if (self.rubber_y2 == self.rubber_origin_y and
             self.rubber_x2 == self.rubber_origin_x):
            image_menu.shell.run(
               "%s = SubImage(__image__, 0, 0, %d, %d)" %
               (name, self.image.nrows, self.image.ncols))
         else:
            image_menu.shell.run(
               "%s = SubImage(__image__, %d, %d, %d, %d)" %
               (name, self.rubber_origin_y, self.rubber_origin_x,
                self.rubber_y2 - self.rubber_origin_y + 1,
                self.rubber_x2 - self.rubber_origin_x + 1))
         del image_menu.shell.locals['__image__']
         # changed from adding a blank line - this is not ideal,
         # but it seems better than image_menu.shell.pushcode(""). KWM
         image_menu.shell_frame.icon_display.update_icons()

   def MakeCopy(self):
      name = var_name.get("copy", image_menu.shell.locals)
      if name:
         image_menu.shell.locals['__image__'] = self.image
         if (self.rubber_y2 == self.rubber_origin_y and
             self.rubber_x2 == self.rubber_origin_x):
            image_menu.shell.pushcode(
               "%s = __image__.image_copy()" % name)
         else:
            image_menu.shell.run(
               "%s = SubImage(__image__, %d, %d, %d, %d).image_copy()" %
               (name, self.rubber_origin_y, self.rubber_origin_x,
                self.rubber_y2 - self.rubber_origin_y + 1,
                self.rubber_x2 - self.rubber_origin_x + 1))
         del image_menu.shell.locals['__image__']
         # changed from adding a blank line - this is not ideal,
         # but it seems better than image_menu.shell.pushcode(""). KWM
         image_menu.shell_frame.icon_display.update_icons()

         
class ImageWindow(wxPanel):
   def __init__(self, parent = None, id = -1):
      wxPanel.__init__(self, parent, id)
      self.SetAutoLayout(true)
      self.toolbar = wxToolBar(self, -1, style=wxTB_HORIZONTAL)
      from gamera.gui import gamera_icons
      self.toolbar.AddSimpleTool(10, gamera_icons.getIconRefreshBitmap(),
                                 "Refresh")
      EVT_TOOL(self, 10, self.OnRefreshClick)
      self.toolbar.AddSeparator()
      self.toolbar.AddSimpleTool(20, gamera_icons.getIconZoomInBitmap(),
                                 "Zoom in")
      EVT_TOOL(self, 20, self.OnZoomInClick)
      self.toolbar.AddSimpleTool(30, gamera_icons.getIconZoomOutBitmap(),
                                 "Zoom Out")
      EVT_TOOL(self, 30, self.OnZoomOutClick)
      self.toolbar.AddSeparator()
      self.toolbar.AddSimpleTool(31, gamera_icons.getIconMakeViewBitmap(),
                                 "Make new view")
      EVT_TOOL(self, 31, self.OnMakeViewClick)
      self.toolbar.AddSimpleTool(32, gamera_icons.getIconImageCopyBitmap(),
                                 "Make new copy")
      EVT_TOOL(self, 32, self.OnMakeCopyClick)
      lc = wxLayoutConstraints()
      lc.top.SameAs(self, wxTop, 0)
      lc.left.SameAs(self, wxLeft, 0)
      lc.right.SameAs(self, wxRight, 0)
      lc.height.AsIs()
      self.toolbar.SetAutoLayout(true)
      self.toolbar.SetConstraints(lc)
      self.id = self.get_display()
      lc = wxLayoutConstraints()
      lc.top.Below(self.toolbar, 0)
      lc.left.SameAs(self, wxLeft, 0)
      lc.right.SameAs(self, wxRight, 0)
      lc.bottom.SameAs(self, wxBottom, 0)
      self.id.SetAutoLayout(true)
      self.id.SetConstraints(lc)

   def get_display(self):
      return ImageDisplay(self)
      
   def close(self):
      self.Destroy()

   def add_click_callback(self, cb):
      self.id.add_click_callback(cb)

   def remove_click_callback(self, cb):
      self.id.remove_click_callback(cb)

   def refresh(self):
      self.id.Refresh(0)

   def set_image(self, image, function):
      return self.id.set_image(image, function)

   def highlight_rectangle(self, y, x, h, w, color=-1, text=''):
      self.id.highlight_rectangle(y, x, h, w, color, text)

   def highlight_cc(self, cc, function):
      self.id.highlight_cc(cc, function)

   def clear_highlight_cc(self, cc):
      self.id.clear_highlight_cc(cc)

   def clear_all_highlights(self):
      self.id.clear_all_highlights()


   ########################################
   # CALLBACKS
   def OnRefreshClick(self, event):
      self.id.reload_image()

   def OnZoomInClick(self, event):
      self.id.ZoomIn()

   def OnZoomOutClick(self, event):
      self.id.ZoomOut()

   def OnMakeViewClick(self, event):
      self.id.MakeView()

   def OnMakeCopyClick(self, event):
      self.id.MakeCopy()

   def focus(self, rect):
      self.id.focus(rect)


##############################################################################
# MULTIPLE IMAGE DISPLAY IN A GRID
##############################################################################

class MultiImageGridRenderer(wxPyGridCellRenderer):
   def __init__(self, parent):
      wxPyGridCellRenderer.__init__(self)
      self.parent = parent

   def get_color(self, image):
      if hasattr(image, 'classification_color'):
         color = image.classification_color()
      if color == None:
         color = (255,255,255)
      color = wxColor(red = color[0], green = color[1],
                      blue = color[2])
      return color

   # Draws one cell of the grid
   def Draw(self, grid, attr, dc, rect, row, col, isSelected):
      global stuff
      
      bitmap_no = row * GRID_NCOLS + col
      if bitmap_no < len(self.parent.list):
         image = self.parent.list[bitmap_no]
      else:
         image = None
      if image != None:
         dc.SetBackgroundMode(wxSOLID)
         # Fill the background
         color = self.get_color(image)
         dc.SetPen(wxTRANSPARENT_PEN)
         if isSelected:
            dc.SetBrush(wxBrush(wxBLACK, wxSOLID))
         else:
            dc.SetBrush(wxBrush(color, wxSOLID))
         dc.DrawRectangle(rect.x, rect.y, rect.width, rect.height)
         # If the image is bigger than the cell, crop it. This used to be done
         # in wxPython, but it is more efficient to do it with Gamera SubImages
         image = None
         if (image.ncols >= rect.GetWidth() or
             image.nrows >= rect.GetHeight()):
            height = min(rect.GetHeight() + 1, image.nrows)
            width = min(rect.GetWidth() + 1, image.ncols)
            # If we are dealing with a CC, we have to make a smaller CC withthe
            # same label. This is unfortunately a two step process.
            if isinstance(image, CC):
               tmp_image = SubImage(image, 0, 0, height, width)
               sub_image = CC(tmp_image, image.label())
            # Otherwise just a SubImage will do.
            else:
               sub_image = SubImage(image, 0, 0, height, width)
            image = wxEmptyImage(width, height)
            s = sub_image.to_string()
            image.SetData(s)
         else:
            image = wxEmptyImage(image.ncols, image.nrows)
            s = image.to_string()
            image.SetData(s)
         bmp = image.ConvertToBitmap()
         # Display centered within the cell
         x = rect.x + (rect.width / 2) - (bmp.GetWidth() / 2)
         y = rect.y + (rect.height / 2) - (bmp.GetHeight() / 2)
         if isSelected:
            dc.SetLogicalFunction(wxSRC_INVERT)
         else:
            dc.SetLogicalFunction(wxAND)
         dc.DrawBitmap(bmp, x, y, 0)
         if isSelected:
            dc.SetLogicalFunction(wxAND)
            dc.SetBrush(wxBrush(color, wxSOLID))
            dc.SetPen(wxTRANSPARENT_PEN)
            dc.DrawRectangle(rect.x, rect.y, rect.width, rect.height)
      else:
         # If there's no image in this cell, draw a hatch pattern
         dc.SetBackgroundMode(wxSOLID)
         dc.SetBrush(wxBrush(wxLIGHT_GREY, wxSOLID))
         dc.SetPen(wxTRANSPARENT_PEN)
         dc.SetLogicalFunction(wxCOPY)
         dc.DrawRectangle(rect.x, rect.y, rect.width, rect.height)
         dc.SetBackgroundMode(wxSOLID)
         dc.SetBrush(wxBrush(wxBLUE, wxFDIAGONAL_HATCH))
         dc.DrawRectangle(rect.x, rect.y, rect.width, rect.height)
      dc.SetLogicalFunction(wxCOPY)

   # The images should be a little padded within the cells
   # Also, there is a max size for every cell
   def GetBestSize(self, grid, attr, dc, row, col):
      bitmap_no = row * GRID_NCOLS + col
      if bitmap_no < len(self.parent.list):
         image = self.parent.list[bitmap_no]
      else:
         image = None
      if image != None:
         return wxSize(min(GRID_MAX_CELL_WIDTH,
                           image.ncols + GRID_PADDING),
                       min(GRID_MAX_CELL_HEIGHT,
                           image.nrows + GRID_PADDING))
      return wxSize(0, 0)

   def Clone(self):
      return MultiImageGridRenderer()


# Grid constants
GRID_MAX_CELL_WIDTH = 200
GRID_MAX_CELL_HEIGHT = 200
GRID_PADDING = 8
GRID_NCOLS = 8
class MultiImageDisplay(wxGrid):
   def __init__(self, parent = None, id = -1, size = wxDefaultSize):
      wxGrid.__init__(self, parent, id)
      self.list = []
      self.rows = 1
      self.cols = GRID_NCOLS
      self.frame = parent
      self.updating = 0
      self.sort_function = ""
      self.created = 0
      self.do_updates = 0
      EVT_GRID_CELL_LEFT_DCLICK(self, self.OnLeftDoubleClick)
      EVT_GRID_CELL_RIGHT_CLICK(self, self.OnRightClick)
      EVT_GRID_SELECT_CELL(self, self.OnSelect)
      EVT_GRID_CELL_CHANGE(self, self.OnSelect)

   ########################################
   # Sets a new list of images.  Can be performed multiple times
   def set_image(self, list, function):
      wxBeginBusyCursor()
      self.BeginBatch()
      self.list = list
      self.do_updates = 0
      self.sort_images()
      self.frame.set_choices(self.list[0])
      if not self.created:
         self.rows = 1
         self.CreateGrid(1, GRID_NCOLS)
         self.created = 1
      self.EnableEditing(0)
      self.resize_grid()
      self.ClearSelection()
      x = self.GetSize()
      self.do_updates = 1
      self.EndBatch()
      wxEndBusyCursor()
      return (x.x, 600)

   def resize_grid(self, do_auto_size=1):
      wxBeginBusyCursor()
      self.BeginBatch()
      orig_rows = self.rows
      rows = (len(self.list) - 1) / GRID_NCOLS + 1
      cols = GRID_NCOLS
      if self.rows < rows:
         self.AppendRows(rows - self.rows)
      elif self.rows > rows:
         self.DeleteRows(rows, self.rows - rows)
      self.rows = rows
      self.cols = cols
      width = self.set_labels()
      self.SetRowLabelSize(width * 6 + 10)
      self.SetColLabelSize(20)
      row = 0
      col = 0
      x = self.GetSize()
      for row in range(orig_rows - 1, self.rows):
         for col in range(GRID_NCOLS):
            self.SetCellRenderer(row, col, MultiImageGridRenderer(self))
            self.SetReadOnly(row, col, TRUE)
         if not do_auto_size:
            self.AutoSizeRow(row)
      if not do_auto_size and orig_rows != self.rows:
         self.AutoSizeRow(self.rows - 1)
      elif do_auto_size:
         self.AutoSize()
      self.SetSize(x)
      self.EndBatch()
      wxEndBusyCursor()

   ########################################
   # SORTING

   # To minimize the size of the grid, we sort the images
   # first by height, and then within each row by width
   def default_sort(self, list):
      list.sort(lambda x, y: cmp(x.nrows, y.nrows))
      outlist = []
      while len(list):
         if len(list) < GRID_NCOLS:
            sublist = list; list = []
         else:
            sublist = list[:GRID_NCOLS]; list = list[GRID_NCOLS:]
         sublist.sort(lambda x, y: cmp(x.ncols, y.ncols))
         outlist.extend(sublist)
      return outlist

   # Sorts the list of images by a given function, or the
   # default function if None is given
   def sort_images(self, function=None, order=0):
      wxBeginBusyCursor()
      self.BeginBatch()
      if function != None:
         self.sort_function = function
      if self.sort_function == "":
         self.list = self.default_sort(self.list)
      else:
         for item in self.list:
            item.sort_cache = None
         self.list.sort(self.sort_function)
         for item in self.list:
            del item.sort_cache
      if order:
         self.list.reverse()
      if self.do_updates:
         width = self.set_labels()
         self.SetRowLabelSize(width * 6 + 10)
         x = self.GetSize()
         self.AutoSize()
         self.SetSize(x)
         self.ClearSelection()
         self.MakeCellVisible(0, 0)
      self.EndBatch()
      wxEndBusyCursor()

   def set_labels(self):
      for i in range(self.cols):
         self.SetColLabelValue(i, "")
      for i in range(self.rows):
         self.SetRowLabelValue(i, "")
      return 1

   ########################################
   # SELECTING

   def select_images(self, function):
      self.updating = 1
      self.ClearSelection()
      for i in range(len(self.list)):
         x = self.list[i]
         if x != None:
          try:
             result = function(x)
          except Exception, err:
             gui_util.message(str(err))
             return
          if result:
             self.SelectBlock(i / GRID_NCOLS, i % GRID_NCOLS,
                              i / GRID_NCOLS, i % GRID_NCOLS, true)
      self.updating = 0
      self.OnSelectImpl()

   ########################################
   # UTILITY FUNCTIONS

   def get_image_no(self, row, col):
      no =  row * GRID_NCOLS + col
      if no >= len(self.list):
         return None
      else:
         return no

   def GetSelectedItems(self, row = None, col = None):
      if row != None:
         bitmap_no = self.get_image_no(row, col)
         # if bitmap_no != None:
         #   self.SetGridCursor(row, col)
      image = []
      if self.IsSelection():
         for row in range(self.rows):
            for col in range(self.cols):
               if self.IsInSelection(row, col):
                  index = self.get_image_no(row, col)
                  if index != None:
                     item = self.list[index]
                     if item != None:
                        image.append(item)
      elif row != None:
         image = [self.list[bitmap_no]]
      return image

   def RefreshSelected(self, row = None, col = None):
      if row != None:
         bitmap_no = self.get_image_no(row, col)
         if bitmap_no != None:
            self.SetGridCursor(row, col)
      if self.IsSelection():
         for row in range(self.rows):
            for col in range(self.cols):
               if self.IsInSelection(row, col):
                  self.SetCellValue(row, col, "")
      elif row != None:
         self.SetCellValue(row, col, "")
   

   ########################################
   # CALLBACKS

   def OnSelectImpl(self):
      pass

   def OnSelect(self, event):
      bitmap_no = self.get_image_no(event.GetRow(), event.GetCol())
      if bitmap_no != None:
         event.Skip()
         self.OnSelectImpl()

   def OnRightClick(self, event):
      row = event.GetRow()
      col = event.GetCol()
      image = self.GetSelectedItems(row, col)
      if image != None:
         position = event.GetPosition()
         menu = image_menu.ImageMenu(self, position.x,
                                       position.y,
                                       image, image, mode=0)
         menu.PopupMenu()
         self.ForceRefresh()

   def OnLeftDoubleClick(self, event):
      bitmap_no = self.get_image_no(event.GetRow(), event.GetCol())
      if bitmap_no != None:
         self.list[bitmap_no].display()

class MultiImageWindow(wxPanel):
   def __init__(self, parent = None, id = -1, title = "Gamera", owner=None):
      wxPanel.__init__(self, parent, id)
      self.SetAutoLayout(true)
      self.toolbar = wxToolBar(self, -1, style=wxTB_HORIZONTAL)
      from gamera.gui import gamera_icons
      self.toolbar.AddSimpleTool(10, gamera_icons.getIconRefreshBitmap(),
                                 "Refresh")
      EVT_TOOL(self, 10, self.OnRefreshClick)
      self.toolbar.AddSeparator()

      self.toolbar.AddControl(wxStaticText(self.toolbar, -1, "Sort: "))
      self.sort_combo = wxComboBox(self.toolbar, 100, choices=[])
      self.toolbar.AddControl(self.sort_combo)
      self.toolbar.AddSimpleTool(101, gamera_icons.getIconSortAscBitmap(),
                                 "Sort Ascending")
      EVT_TOOL(self, 101, self.OnSortAscending)
      self.toolbar.AddSimpleTool(102, gamera_icons.getIconSortDecBitmap(),
                                 "Sort Descending")
      EVT_TOOL(self, 102, self.OnSortDescending)
      self.toolbar.AddSeparator()
      self.toolbar.AddControl(wxStaticText(self.toolbar, -1, "Select: "))
      self.select_combo = wxComboBox(self.toolbar, 200, choices=[])
      self.toolbar.AddControl(self.select_combo)
      self.toolbar.AddSimpleTool(201, gamera_icons.getIconSelectBitmap(),
                                 "Select by given expression")
      EVT_TOOL(self, 201, self.OnSelect)
      self.toolbar.AddSimpleTool(203, gamera_icons.getIconSelectAllBitmap(),
                                 "Select All")
      EVT_TOOL(self, 203, self.OnSelectAll)
      self.select_choices = []
      lc = wxLayoutConstraints()
      lc.top.SameAs(self, wxTop, 0)
      lc.left.SameAs(self, wxLeft, 0)
      lc.right.SameAs(self, wxRight, 0)
      lc.height.AsIs()
      self.toolbar.SetConstraints(lc)
      self.id = self.get_display()
      lc = wxLayoutConstraints()
      lc.top.Below(self.toolbar, 0)
      lc.left.SameAs(self, wxLeft, 0)
      lc.right.SameAs(self, wxRight, 0)
      lc.bottom.SameAs(self, wxBottom, 0)
      self.id.SetConstraints(lc)
      self.sort_choices = []

   # This can be overridden to change the internal display class
   def get_display(self):
      return MultiImageDisplay(self)

   def set_image(self, image, function):
      return self.id.set_image(image, function)

   def set_choices(self, prototype):
      members = find_members.find_members(prototype)
      methods = find_members.find_methods_flat_category(prototype, "Features")
      self.sort_choices = ["", "ncols", "nrows", "label()", "id",
                           "classification_state", "page_offset_x()",
                           "page_offset_y()"]
      for method in methods:
         self.sort_choices.append(method[0] + "()")
      self.sort_combo.Clear()
      for choice in self.sort_choices:
         self.sort_combo.Append(choice)
      self.select_choices = ["", "x.unclassified()",
                             "x.automatically_classified()",
                             "x.manually_classified()",
                             "x.nrows < 2 or x.ncols < 2"]
      self.select_combo.Clear()
      for choice in self.select_choices:
         self.select_combo.Append(choice)

   def close(self):
      self.Destroy()

   def refresh(self):
      self.id.ForceRefresh()

   ########################################
   # CALLBACKS

   def OnRefreshClick(self, event):
      self.id.ForceRefresh()

   def OnSortAscending(self, event, order=0):
      sort_string = string.strip(self.sort_combo.GetValue())
      if sort_string == "":
         self.id.sort_images("", order)
         return
      if sort_string[0:4] == "func":
         split = string.split(sort_string)
         if len(split) >= 1:
            final = string.join(split[1:])
         else:
            gui_util.message("The given sorting expressing contains a syntax error.")
            return
      else:
         final = ("lambda x, y: util.fast_cmp(x, (x." +
                  sort_string + "), y, (y." +
                  sort_string + "))")
      try:
         sort_func = eval(final, globals(), image_menu.shell.locals)
      except SyntaxError:
         gui_util.message("The given sorting expression contains a syntax error.")
         return
      if sort_string not in self.sort_choices:
         self.sort_choices.append(sort_string)
         self.sort_combo.Append(sort_string)
      wxBeginBusyCursor()
      self.id.sort_images(sort_func, order)
      wxEndBusyCursor()

   def OnSortDescending(self, event):
      self.OnSortAscending(event, 1)

   def OnSelect(self, event):
      select_string = string.strip(self.select_combo.GetValue())
      if select_string == "":
         return
      try:
         select_func = eval("lambda x: (" + select_string + ")", globals(),
                            image_menu.shell.locals)
      except SyntaxError:
         gui_util.message("The given selection expression contains a syntax error.")
         return
      if select_string not in self.select_choices:
         self.select_choices.append(select_string)
         self.select_combo.Append(select_string)
      wxBeginBusyCursor()
      self.id.select_images(select_func)
      wxEndBusyCursor()

   def OnSelectAll(self, event):
      self.id.SelectAll()

   def OnSelectInvert(self, event):
      self.id.SelectInvert()
   

##############################################################################
# TOP-LEVEL FRAMES
##############################################################################

class ImageFrameBase(wxFrame):
   def __init__(self, parent = None, id = -1, title = "Gamera", owner=None):
      wxFrame.__init__(self, parent, id, title,
                       wxDefaultPosition, (600, 400))
      self.owner = owner
      EVT_CLOSE(self, self.OnCloseWindow)

   def set_image(self, image, function):
      size = self.iw.set_image(image, function)
      self.SetSize((max(200, min(600, size[0] + 30)),
                    max(200, min(400, size[1] + 60))))

   def close(self):
      self.iw.Destroy()
      self.Destroy()

   def refresh(self):
      self.iw.refresh(1)

   def add_click_callback(self, cb):
      self.iw.add_click_callback(cb)

   def remove_click_callback(self, cb):
      self.iw.remove_click_callback(cb)

   def OnCloseWindow(self, event):
      del self.iw
      if self.owner:
         self.owner.set_display(None)
      self.Destroy()

class ImageFrame(ImageFrameBase):
   def __init__(self, parent = None, id = -1, title = "Gamera", owner=None):
      ImageFrameBase.__init__(self, parent, id, title, owner)
      self.iw = ImageWindow(self)
      from gamera.gui import gamera_icons
      icon = wxIconFromBitmap(gamera_icons.getIconImageBitmap())
      self.SetIcon(icon)

   def __repr__(self):
      return "<ImageFrame Window>"

   def highlight_rectangle(self, y, x, h, w, color=-1, text=''):
      self.iw.highlight_rectangle(y, x, h, w, color, text)

   def highlight_cc(self, cc, function):
      self.iw.highlight_cc(cc, function)

   def clear_highlight_cc(self, cc):
      self.iw.clear_highlight_cc(cc)

   def clear_all_highlights(self):
      self.iw.clear_all_highlights()

   def focus(self, rect):
      self.iw.focus(rect)

   def wait(self):
      wxYield()
      wxMessageDialog(self, "Continue", style=wxOK).ShowModal()
      

class MultiImageFrame(ImageFrameBase):
   def __init__(self, parent = None, id = -1, title = "Gamera", owner=None):
      ImageFrameBase.__init__(self, parent, id, title, owner)
      self.iw = MultiImageWindow(self)
      from gamera.gui import gamera_icons
      icon = wxIconFromBitmap(gamera_icons.getIconImageListBitmap())
      self.SetIcon(icon)

   def __repr__(self):
      return "<MultiImageFrame Window>"

   def set_image(self, image, function):
      size = self.iw.set_image(image, function)


##############################################################################
# GRAPHING
##############################################################################

# Draws a horizontal bar graph
def graph_horiz(data, dc, x1, y1, x2, y2, mark=None, border=1):
   scale_x = float(x2 - x1) / float(len(data))
   scale_y = (y2 - y1) / max(data)
   dc.SetPen(wxTRANSPARENT_PEN)
   light_blue = wxColor(128, 128, 255)
   for i in range(len(data)):
      datum = data[i] * scale_y
      dc.SetBrush(wxWHITE_BRUSH)
      dc.DrawRectangle(x1 + i * scale_x, y1,
                       scale_x + 1, y2 - y1)
      dc.SetBrush(wxBrush(light_blue, wxSOLID))
      dc.DrawRectangle(x1 + i * scale_x, y2 - datum,
                       scale_x + 1, datum)
   if mark:
      dc.SetBrush(wxCYAN_BRUSH)
      dc.SetLogicalFunction(wxXOR)
      dc.DrawRectangle(x1 + mark * scale_x, y1,
                       scale_x + 1, y2 - y1)
      dc.SetLogicalFunction(wxCOPY)
   if border:
      dc.SetPen(wxBLACK_PEN)
      dc.SetBrush(wxTRANSPARENT_BRUSH)
      dc.DrawRectangle(x1 - 1, y1 - 1, x2 - x1 + 2, y2 - y1 + 1)

# Draws a vertical bar graph
def graph_vert(data, dc, x1, y1, x2, y2, mark=None, border=1):
   scale_y = float(y2 - y1) / float(len(data))
   scale_x = (x2 - x1) / max(data)
   light_blue = wxColor(128, 128, 255)
   dc.SetPen(wxTRANSPARENT_PEN)
   for i in range(len(data)):
      datum = data[i] * scale_x
      dc.SetBrush(wxWHITE_BRUSH)
      dc.DrawRectangle(x1, y1 + i * scale_y, 
                       x2 - x1, scale_y + 1)
      dc.SetBrush(wxBrush(light_blue, wxSOLID))
      dc.DrawRectangle(x1, y1 + i * scale_y,
                       datum, scale_y + 1)
   if mark:
      dc.SetBrush(wxCYAN_BRUSH)
      dc.SetLogicalFunction(wxXOR)
      dc.DrawRectangle(x1, y1 + mark * scale_y,
                       x2 - x1, scale_y + 1)
      dc.SetLogicalFunction(wxCOPY)
   if border:
      dc.SetPen(wxBLACK_PEN)
      dc.SetBrush(wxTRANSPARENT_BRUSH)
      dc.DrawRectangle(x1 - 1, y1 - 1, x2 + 1 - x1, y2 - y1)

# Draws a grey-scale scale
def graph_scale(dc, x1, y1, x2, y2):
   scale_x = float(x2 - x1) / float(255)
   dc.SetPen(wxTRANSPARENT_PEN)
   for i in range(255):
      dc.SetBrush(wxBrush(wxColor(i, i, i), wxSOLID))
      dc.DrawRectangle(x1 + i * scale_x, y1,
                       scale_x + 1, y2 - y1)
   dc.SetPen(wxBLACK_PEN)
   dc.SetBrush(wxTRANSPARENT_BRUSH)
   dc.DrawRectangle(x1 - 1, y1 - 1, x2 - x1 + 2, y2 - y1 + 1)

# Clears an area of a wxDC
def clear_dc(dc):
   width = dc.GetSize().x
   height = dc.GetSize().y
   dc.SetBackgroundMode(wxSOLID)
   dc.SetBrush(wxBrush(wxWHITE, wxSOLID))
   dc.SetPen(wxTRANSPARENT_PEN)
   dc.DrawRectangle(0, 0, width, height)


HISTOGRAM_PAD = 30
class HistogramDisplay(wxFrame):
   def __init__(self, data=None, mark=None, parent=None,
                title="Histogram"):
      wxFrame.__init__(self, parent, -1, title,
                       style=wxRESIZE_BORDER|wxCAPTION)
      self.data = data
      self.mark = mark
      EVT_PAINT(self, self.OnPaint)

   def OnPaint(self, event):
      dc = wxPaintDC(self)
      width = dc.GetSize().x
      height = dc.GetSize().y
      clear_dc(dc)
      graph_horiz(self.data, dc, HISTOGRAM_PAD, HISTOGRAM_PAD,
                  width - HISTOGRAM_PAD, height - HISTOGRAM_PAD * 1.5,
                  mark = self.mark)
      graph_scale(dc, HISTOGRAM_PAD, height - HISTOGRAM_PAD * 1.5,
                  width - HISTOGRAM_PAD, height - HISTOGRAM_PAD)


class ProjectionsDisplay(wxFrame):
   def __init__(self, x_data=None, y_data=None, image=None, parent=None, title="Projections"):
      wxFrame.__init__(self, parent, -1, title,
                       style=wxCAPTION,
                       size=((image.ncols * 2) + (HISTOGRAM_PAD * 3),
                             (image.nrows * 2) + (HISTOGRAM_PAD * 3)))
      self.x_data = x_data
      self.y_data = y_data
      self.image = image
      EVT_PAINT(self, self.OnPaint)

   def OnPaint(self, event):
      dc = wxPaintDC(self)
      clear_dc(dc)
      dc_width = dc.GetSize().x
      dc_height = dc.GetSize().y
      mat_width = self.image.ncols
      mat_height = self.image.nrows
      image = wxEmptyImage(self.image.ncols, self.image.nrows)
      image.SetData(self.image.to_string())
      bmp = image.ConvertToBitmap()
      # Display centered within the cell
      x = (dc_width / 2) + (HISTOGRAM_PAD / 2)
      y = HISTOGRAM_PAD
      dc.DrawBitmap(bmp, x, y, 0)
      graph_vert(self.x_data, dc, x - HISTOGRAM_PAD - mat_width, y,
                 x - HISTOGRAM_PAD, y + mat_height, border=0)
      graph_horiz(self.y_data, dc, x, y + mat_height + HISTOGRAM_PAD,
                  x + mat_width, y + (mat_height * 2) + HISTOGRAM_PAD, border=0)

class ProjectionDisplay(wxFrame):
   def __init__(self, data, title="Projection"):
      wxFrame.__init__(self, -1, -1, title,
                       style=wxCAPTION,
                       size=((len(data) * 2) + (HISTOGRAM_PAD * 3),
                             max(data) + (HISTOGRAM_PAD * 3)))
      self.data = data
      EVT_PAINT(self, self.OnPaint)

   def OnPaint(self, event):
      dc = wxPaintDC(self)
      clear_dc(dc)
      dc_width = dc.GetSize().x
      dc_height = dc.GetSize().y
      # Display centered within the cell
      x = (dc_width / 2) + (HISTOGRAM_PAD / 2)
      y = HISTOGRAM_PAD
      dc.DrawBitmap(bmp, x, y, 0)
      graph_horiz(self.data, dc, x, y + HISTOGRAM_PAD,
                  x + mat_width, y + HISTOGRAM_PAD, border=0)




