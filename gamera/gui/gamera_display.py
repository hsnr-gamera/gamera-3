#
# Copyright (C) 2001, 2002 Ichiro Fujinaga, Michael Droettboom,
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

from wxPython.wx import *         # wxPython
from wxPython.grid import *
from wxPython.lib.stattext import wxGenStaticText as wxStaticText
from math import ceil, log, floor # Python standard library
from sys import maxint
import sys, string, time, weakref
# This is a work around for a problem with compositing in wxPython
# for wxGTK (wxBlack and wxWhite are reversed for the compositing).
if sys.platform == 'win32':
   real_white = wxWHITE
   real_black = wxBLACK
else:
   real_white = wxBLACK
   real_black = wxWHITE
from gamera.core import *             # Gamera specific
from gamera import paths, util
from gamera.gui import image_menu, var_name, gui_util, toolbar
import gui_support

##############################################################################

# we want this done on import
wxInitAllImageHandlers()
config.add_option_default("display_scroll_amount", 32)

#############################################################################

##############################################################################
# SINGLE IMAGE DISPLAY
##############################################################################
#
# Image display is a scrolled window for displaying a single image

class ImageDisplay(wxScrolledWindow):
   def __init__(self, parent, id = -1, size = wxDefaultSize):
      wxScrolledWindow.__init__(
         self, parent, id, wxPoint(0, 0), size,
         wxCLIP_CHILDREN|wxNO_FULL_REPAINT_ON_RESIZE|wxSIMPLE_BORDER)
      self.SetBackgroundColour(wxWHITE)

      self.scaling = 1.0
      self.scaling_quality = 0
      self.view_function = None
      self.image = None
      self.original_image = None
      self.highlights = []
      self.color = 0
      self.rubber_on = 0
      self.rubber_origin_x = 0
      self.rubber_origin_y = 0
      self.rubber_x2 = 0
      self.rubber_y2 = 0
      self.click_callbacks = []
      self.current_color = 0
      self.dragging = 0
      self.scroll_amount = config.get_option("display_scroll_amount")
      self.block_w = 8
      self.block_h = 8

      EVT_PAINT(self, self._OnPaint)
      EVT_SIZE(self, self._OnResize)
      EVT_LEFT_DOWN(self, self._OnLeftDown)
      EVT_LEFT_UP(self, self._OnLeftUp)
      EVT_MIDDLE_DOWN(self, self._OnMiddleDown)
      EVT_MIDDLE_UP(self, self._OnMiddleUp)
      EVT_RIGHT_DOWN(self, self._OnRightDown)
      EVT_MOTION(self, self._OnMotion)
      EVT_LEAVE_WINDOW(self, self._OnLeave)

   ########################################
   # THE MAIN IMAGE

   # Sets the image being displayed
   # Returns the size of the image
   def set_image(self, image, view_function=None, weak=1):
      if weak:
         self.original_image = weakref.proxy(image)
         self.image = weakref.proxy(image)
      else:
         self.original_image = image
         self.image = image
      self.view_function = view_function
      return self.reload_image()

   # Refreshes the image by recalling the to_string function
   def reload_image(self, *args):
      if self.view_function != None:
         self.image = apply(self.view_function, (self.original_image,))
      self.scale()
      return (self.image.width, self.image.height)

   def add_click_callback(self, cb):
      self.click_callbacks.append(cb)

   def remove_click_callback(self, cb):
      self.click_callbacks.remove(cb)

   ########################################
   # HIGHLIGHTED CCs

   def _get_highlight_color(self, color):
      if util.is_sequence(color):
         if len(color) == 3:
            return wxColor(*color)
         else:
            return -1
      elif isinstance(color, wxColor):
         return color
      elif type(color) == type(0) and color >= 0:
         return gui_util.get_color(color)
      return -1

   # Highlights only a particular list of ccs in the display
   def highlight_cc(self, ccs, color=-1):
      color = self._get_highlight_color(color)
      ccs = util.make_sequence(ccs)
      use_color = color
      old_highlights = self.highlights[:]
      self.highlights = []
      for cc in ccs:
         found_it = 0
         if color == -1:
            for old_cc, use_color in old_highlights:
               if old_cc == cc:
                  found_it = 1
                  break
            if not found_it:
               use_color = gui_util.get_color(self.color)
               self.color += 1
         self.highlights.append((cc, use_color))
         if not found_it:
            self.PaintAreaRect(cc)
      for cc in old_highlights:
         if cc not in self.highlights:
            self.PaintAreaRect(cc[0])
   highlight_ccs = highlight_cc

   # Adds a list of ccs to be highlighted in the display
   def add_highlight_cc(self, ccs, color=-1):
      color = self._get_highlight_color(color)
      ccs = util.make_sequence(ccs)
      use_color = color
      for cc in ccs:
         if cc not in self.highlights:
            if color == -1:
               use_color = gui_util.get_color(self.color)
               self.color += 1
            self.highlights.append((cc, use_color))
            self.PaintAreaRect(cc)
   add_highlight_ccs = add_highlight_cc

   # Clears a CC in the display.  Multiple CCs
   # may be unhighlighted in turn
   def unhighlight_cc(self, ccs):
      ccs = util.make_sequence(ccs)
      last_one = 0
      for cc in ccs:
         for i in range(last_one, len(self.highlights)):
            if (self.highlights[i][0] == cc):
               del self.highlights[i]
               self.PaintAreaRect(cc)
               last_one = i
               break
   unhighlight_ccs = unhighlight_cc
      
   # Clears all of the highlighted CCs in the display   
   def clear_all_highlights(self):
      old_highlights = self.highlights[:]
      self.highlights = []
      for highlight in old_highlights:
         self.PaintAreaRect(highlight[0])

   # Adjust the scrollbars so a group of highlighted subimages are visible
   def focus(self, glyphs):
      if not util.is_sequence(glyphs):
         glyphs = (glyphs,)
      x1 = y1 = maxint
      x2 = y2 = 0
      # Get a combined rectangle of all images in the list
      for image in glyphs:
         x1 = min(image.ul_x, x1)
         y1 = min(image.ul_y, y1)
         x2 = max(image.lr_x, x2)
         y2 = max(image.lr_y, y2)
      self.focus_rect(x1, y1, x2, y2)

   def focus_rect(self, x1, y1, x2, y2):
      scroll_amount = self.scroll_amount
      # Adjust for the scaling factor
      scaling = self.scaling
      x1, y1, x2, y2 = tuple([x * scaling for x in (x1, y1, x2, y2)])
      origin = [x * self.scroll_amount for x in self.GetViewStart()]
      maxwidth = self.image.width * self.scaling
      maxheight = self.image.height * self.scaling
      need_to_scroll = 0
      size = self.GetSize()
      if (x1 < origin[0] or
          x2 > origin[0] + size.x / scaling):
         set_x = max(0, min(maxwidth - size[0] / self.scaling,
                            x1))
         need_to_scroll = 1
      else:
         set_x = origin[0]
      if (y1 < origin[1] or
          y2 > origin[1] + size.y / scaling or
          need_to_scroll):
         set_y = max(0, min(maxheight - size[1] / self.scaling,
                            y1))
         need_to_scroll = 1
      else:
         set_y = origin[1]
      if need_to_scroll:
         self.SetScrollbars(scroll_amount, scroll_amount,
                            floor(maxwidth / scroll_amount),
                            floor(maxheight / scroll_amount),
                            set_x / scroll_amount,
                            set_y / scroll_amount)
         self.RefreshAll()

   ########################################
   # SCALING
   #
   def scale(self, scale=None):
      scaling = self.scaling
      if scale == scaling:
         return
      elif scale < scaling:
         self.Clear()
      wxBeginBusyCursor()
      if scale == None or scale <= 0:
         scale = scaling
      scale = pow(2.0, floor(log(scale) / log(2.0)))
      scroll_amount = self.scroll_amount
      scaling = self.scaling
      w = self.image.width * scale
      h = self.image.height * scale
      origin = [x * self.scroll_amount for x in self.GetViewStart()]
      size = self.GetSize()
      x = max(min(origin[0] * scale / scaling + 1,
                  w - size.x) - 2, 0)
      y = max(min(origin[1] * scale / scaling + 1,
                  h - size.y) - 2, 0)
      self.scaling = scale
      
      self.SetScrollbars(scroll_amount, scroll_amount,
                         floor(w / scroll_amount),
                         floor(h / scroll_amount),
                         floor((x / scroll_amount) + 0.5),
                         floor((y / scroll_amount) + 0.5))
      self.RefreshAll()
      wxEndBusyCursor()

   def ZoomOut(self, *args):
      if self.scaling > pow(2, -8):
         self.scale(self.scaling * 0.5)

   def ZoomNorm(self, *args):
      self.scale(1.0)

   def ZoomIn(self, *args):
      if self.scaling < pow(2, 8):
         self.scale(self.scaling * 2.0)

   def ZoomView(self, *args):
      scroll_amount = self.scroll_amount
      # Zooms in as best it can on the current view
      x = min(self.rubber_origin_x, self.rubber_x2)
      y = min(self.rubber_origin_y, self.rubber_y2)
      x2 = max(self.rubber_origin_x, self.rubber_x2)
      y2 = max(self.rubber_origin_y, self.rubber_y2)
      rubber_w = (x2 - x) + scroll_amount
      rubber_h = (y2 - y) + scroll_amount
      size = self.GetSize()
      if rubber_w == scroll_amount and rubber_h == scroll_amount:
         rubber_w = self.image.ncols
         rubber_h = self.image.nrows
         x = y = x2 = y2 = 0
      scaling = min(float(size.x) / float(rubber_w),
                    float(size.y) / float(rubber_h))
      self.scale(scaling)
      self.focus_rect(x, y, x2, y2)

   def SetZoomType(self, type):
      self.scaling_quality = type
      if self.scaling != 1.0:
         self.RefreshAll()

   ########################################
   # RUBBER BAND
   #
   def draw_rubber(self, dc=None):
      if dc == None:
         dc = wxClientDC(self)
      scaling = self.scaling
      origin = [x * self.scroll_amount for x in self.GetViewStart()]
      x = min(self.rubber_origin_x, self.rubber_x2)
      y = min(self.rubber_origin_y, self.rubber_y2)
      x2 = max(self.rubber_origin_x, self.rubber_x2)
      y2 = max(self.rubber_origin_y, self.rubber_y2)
      x, y, x2, y2 = tuple([a * scaling for a in (x, y, x2, y2)])
      x -= origin[0]
      y -= origin[1]
      x2 -= origin[0]
      y2 -= origin[1]
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

   def OnRubber(self):
      #deliberately empty -- this is a callback to be overridden by subclasses
      pass

   ########################################
   # UTILITY
   #
   def MakeView(self, *args):
      name = var_name.get("view", image_menu.shell.locals)
      if name:
         if (self.rubber_y2 == self.rubber_origin_y and
             self.rubber_x2 == self.rubber_origin_x):
            subimage = self.original_image.subimage(
               0, 0,
               self.original_image.nrows, self.original_image.ncols)
         else:
            subimage = self.original_image.subimage(
               self.rubber_origin_y + self.original_image.ul_y,
               self.rubber_origin_x + self.original_image.ul_x,
               self.rubber_y2 - self.rubber_origin_y + 1,
               self.rubber_x2 - self.rubber_origin_x + 1)
         image_menu.shell.locals[name] = subimage
         # changed from adding a blank line - this is not ideal,
         # but it seems better than image_menu.shell.pushcode(""). KWM
         image_menu.shell.update()

   def MakeCopy(self, *args):
      name = var_name.get("copy", image_menu.shell.locals)
      if name:
         if (self.rubber_y2 == self.rubber_origin_y and
             self.rubber_x2 == self.rubber_origin_x):
            copy = self.original_image.image_copy()
         else:
            copy = self.original_image.subimage(
               self.rubber_origin_y + self.original_image.ul_y,
               self.rubber_origin_x + self.original_image.ul_x,
               self.rubber_y2 - self.rubber_origin_y + 1,
               self.rubber_x2 - self.rubber_origin_x + 1).image_copy()
         image_menu.shell.locals[name] = copy
         # changed from adding a blank line - this is not ideal,
         # but it seems better than image_menu.shell.pushcode(""). KWM
         image_menu.shell_frame.icon_display.update_icons()

   ########################################
   # CALLBACKS
   #
   def _OnResize(self, event):
      size = self.GetSize()
      if size.x > 0 and size.y > 0:
         event.Skip()
         self.scale(self.scaling)
      else:
         event.Skip()

   def _OnPaint(self, event):
      if not self.image:
         return
      scaling = self.scaling
      origin = [x * self.scroll_amount for x in self.GetViewStart()]
      origin_scaled = [x / scaling for x in origin]
      update_regions = self.GetUpdateRegion()
      rects = wxRegionIterator(update_regions)
      # Paint only where wxWindows tells us to, this is faster
      dc = wxPaintDC(self)
      while rects.HaveRects():
         ox = rects.GetX() / scaling
         oy = rects.GetY() / scaling
         x = ox + origin_scaled[0]
         y = oy + origin_scaled[1]
         # For some reason, the rectangles wxWindows gives are
         # a bit too small, so we need to fudge their size
         if not (x > self.image.width or y > self.image.height):
            # this used to be int(self.scaling / 2) but this works
            # with the new scaled_to_string. KWM
            fudge = int(self.scaling) * 2
            w = (max(min(int((rects.GetW() / scaling) + fudge),
                         self.image.ncols - ox), 0))
            h = (max(min(int((rects.GetH() / scaling) + fudge),
                         self.image.nrows - oy), 0))
            # Quantize for scaling
            if scaling > 1:
               x = int(int(x / scaling) * scaling)
               y = int(int(y / scaling) * scaling)
               w = int(int(w / scaling) * scaling)
               h = int(int(h / scaling) * scaling)
            self.PaintArea(x + self.image.ul_x, y + self.image.ul_y,
                           w, h, check=0, dc=dc)
         rects.Next()
      self.draw_rubber(dc)

   def PaintAreaRect(self, rect):
      self.PaintArea(rect.ul_x, rect.ul_y, rect.ncols, rect.nrows, 1)

   def PaintArea(self, x, y, w, h, check=1, dc=None):
      redraw_rubber = 0
      if dc == None:
         dc = wxClientDC(self)
         self.draw_rubber(dc)
         redraw_rubber = 1
      dc.SetLogicalFunction(wxCOPY)
      scaling = self.scaling
      origin = [a * self.scroll_amount for a in self.GetViewStart()]
      origin_scaled = [a / scaling for a in origin]
      size_scaled = [a / scaling for a in self.GetSizeTuple()]

      if (y + h >= self.image.lr_y):
         h = self.image.lr_y - y + 1
      if (x + w >= self.image.lr_x):
         w = self.image.lr_x - x + 1

      if check:
         if ((x + w < origin_scaled[0]) and
             (y + h < origin_scaled[1]) or
             (x > origin_scaled[0] + size_scaled[0] and
              y > origin_scaled[1] + size_scaled[1])):
            return
         
      # If the update region is smaller than a single pixel, you get
      # "Floating point exceptions", so just bail, since you can't see
      # zero pixels anyway <wink>
      if float(w) * scaling < 1.0 or float(h) * scaling < 1.0:
         if redraw_rubber:
            self.draw_rubber(dc)
         return

      subimage = self.image.subimage(y, x, h, w)
      image = None
      if scaling != 1.0:
         # For the high quality scalings a greyscale (or rgb) is required
         # to really realize the increased quality. There is some smoothing
         # that happens for zooming in, but for zooming out grey pixels are
         # required for the smoothing. This simply does a conversion on the
         # fly, which can be very slow, but limits the total memory usage.
         # The other option is to cache a greyscale copy of the image, but
         # that could use too much memory.
         if self.scaling_quality > 0 and subimage.data.pixel_type == ONEBIT:
            subimage = subimage.to_greyscale()
         subimage = subimage.scale_copy(scaling, self.scaling_quality)
      image = wxEmptyImage(subimage.ncols, subimage.nrows)
      subimage.to_buffer(image.GetDataBuffer())

      bmp = wxBitmapFromImage(image)
      x = (x - self.image.ul_x) * scaling - origin[0]
      y = (y - self.image.ul_y) * scaling - origin[1]
      dc.DrawBitmap(bmp, x, y, 0)

      if len(self.highlights):
         dc.SetTextBackground(real_white)
         dc.SetBackgroundMode(wxTRANSPARENT)
         for highlight, color in self.highlights:
            if subimage.intersects(highlight):
               subhighlight = highlight.clip_image(subimage)
               h = subhighlight.nrows
               w = subhighlight.ncols
               if scaling != 1.0:
                  # If zooming has caused the glyph to shrink into
                  # nothingness, don't draw it.
                  if float(h) * scaling <= 1 or float(w) * scaling <= 1:
                     continue
                  subhighlight = subhighlight.scale_copy(scaling, 0)
               image = wxEmptyImage(subhighlight.ncols, subhighlight.nrows)
               subhighlight.to_buffer(image.GetDataBuffer())
               bmp = wxBitmapFromImage(image, 1)
               dc.SetTextForeground(real_black)
               dc.SetLogicalFunction(wxAND_INVERT)
               x_cc = x + (subhighlight.ul_x - subimage.ul_x) * scaling
               y_cc = y + (subhighlight.ul_y - subimage.ul_y) * scaling
               dc.DrawBitmap(bmp, x_cc, y_cc)
               dc.SetTextForeground(color)
               dc.SetLogicalFunction(wxOR)
               dc.DrawBitmap(bmp, x_cc, y_cc)
         dc.SetBackgroundMode(wxSOLID)
         dc.SetLogicalFunction(wxCOPY)
      if redraw_rubber:
         self.draw_rubber(dc)
            
   def RefreshAll(self):
      size = self.GetSize()
      self.Refresh(0, rect=wxRect(0, 0, size.x, size.y))

   def _OnLeftDown(self, event):
      self.rubber_on = 1
      self.draw_rubber()
      origin = [x * self.scroll_amount for x in self.GetViewStart()]
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
      self.rubber_origin_x = self.rubber_x2 = int(x)
      self.rubber_origin_y = self.rubber_y2 = int(y)

   def _OnLeftUp(self, event):
      if self.rubber_on:
         self.draw_rubber()
         origin = [x * self.scroll_amount for x in self.GetViewStart()]
         self.rubber_x2 = int(min((event.GetX() + origin[0]) / self.scaling,
                                  self.image.ncols - 1))
         self.rubber_y2 = int(min((event.GetY() + origin[1]) / self.scaling,
                                  self.image.nrows - 1))
         self.draw_rubber()
         for i in self.click_callbacks:
            try:
               i(self.rubber_y2, self.rubber_x2)
            except Exception, e:
               print e
         self._OnLeave(event)

   def _OnMiddleDown(self, event):
      self.dragging = 1
      self.dragging_x = event.GetX()
      self.dragging_y = event.GetY()
      self.dragging_origin_x, self.dragging_origin_y = \
         [x * self.scroll_amount for x in self.GetViewStart()]

   def _OnMiddleUp(self, event):
      self.dragging = 0

   def _OnRightDown(self, event):
      from gamera.gui import image_menu
      image_menu.ImageMenu(
         self, event.GetX(), event.GetY(),
         self.image)
      self.reload_image()

   def _OnMotion(self, event):
      if self.rubber_on:
         self.draw_rubber()
         origin = [x * self.scroll_amount for x in self.GetViewStart()]
         self.rubber_x2 = int(min((event.GetX() + origin[0]) / self.scaling,
                                  self.image.ncols - 1))
         self.rubber_y2 = int(min((event.GetY() + origin[1]) / self.scaling,
                                  self.image.nrows - 1))
         self.draw_rubber()

      if self.dragging:
         self.Scroll(
            (self.dragging_origin_x - (event.GetX() - self.dragging_x)) /
            self.scroll_amount,
            (self.dragging_origin_y - (event.GetY() - self.dragging_y)) /
            self.scroll_amount)

   def _OnLeave(self, event):
      if self.rubber_on:
         if self.rubber_origin_x > self.rubber_x2:
            self.rubber_origin_x, self.rubber_x2 = \
                                  self.rubber_x2, self.rubber_origin_x
         if self.rubber_origin_y > self.rubber_y2:
            self.rubber_origin_y, self.rubber_y2 = \
                                  self.rubber_y2, self.rubber_origin_y
         self.rubber_on = 0
         self.OnRubber()
      if self.dragging:
         self.dragging = 0

         
class ImageWindow(wxPanel):
   def __init__(self, parent = None, id = -1):
      wxPanel.__init__(self, parent, id)
      self.id = self.get_display()
      self.SetAutoLayout(true)
      self.toolbar = toolbar.ToolBar(self, -1)
      from gamera.gui import gamera_icons
      self.toolbar.AddSimpleTool(10, gamera_icons.getIconRefreshBitmap(),
                                 "Refresh", self.id.reload_image)
      self.toolbar.AddSeparator()
      self.toolbar.AddSimpleTool(20, gamera_icons.getIconZoomInBitmap(),
                                 "Zoom in", self.id.ZoomIn)
      self.toolbar.AddSimpleTool(21, gamera_icons.getIconZoomNormBitmap(),
                                 "Zoom 100%", self.id.ZoomNorm)
      self.toolbar.AddSimpleTool(22, gamera_icons.getIconZoomOutBitmap(),
                                 "Zoom out", self.id.ZoomOut)
      self.toolbar.AddSimpleTool(23, gamera_icons.getIconZoomViewBitmap(),
                                 "Zoom in on selected region", self.id.ZoomView)
      self.toolbar.AddControl(wxStaticText(self.toolbar, -1, "Quality: "))
      self.zoom_slider = wxComboBox(
         self.toolbar, 24, choices=['low','medium','high'], style=wxCB_READONLY)
      EVT_COMBOBOX(self, 24, self._OnZoomTypeChange)
      self.toolbar.AddControl(self.zoom_slider)

      self.toolbar.AddSeparator()

      self.toolbar.AddSimpleTool(31, gamera_icons.getIconMakeViewBitmap(),
                                 "Make new view", self.id.MakeView)
      self.toolbar.AddSimpleTool(32, gamera_icons.getIconImageCopyBitmap(),
                                 "Make new copy", self.id.MakeCopy)
      lc = wxLayoutConstraints()
      lc.top.SameAs(self, wxTop, 0)
      lc.left.SameAs(self, wxLeft, 0)
      lc.right.SameAs(self, wxRight, 0)
      lc.height.AsIs()
      self.toolbar.SetAutoLayout(true)
      self.toolbar.SetConstraints(lc)
      lc = wxLayoutConstraints()
      lc.top.Below(self.toolbar, 0)
      lc.left.SameAs(self, wxLeft, 0)
      lc.right.SameAs(self, wxRight, 0)
      lc.bottom.SameAs(self, wxBottom, 0)
      self.id.SetAutoLayout(true)
      self.id.SetConstraints(lc)
      self.Layout()

   def get_display(self):
      return ImageDisplay(self)

   ########################################
   # CALLBACKS
   def _OnZoomTypeChange(self, event):
      self.id.SetZoomType(
         self.zoom_slider.GetSelection())


##############################################################################
# MULTIPLE IMAGE DISPLAY IN A GRID
##############################################################################

class MultiImageGridRenderer(wxPyGridCellRenderer):
   def __init__(self, parent):
      wxPyGridCellRenderer.__init__(self)
      self.parent = parent

   _colors = {UNCLASSIFIED: wxColor(255,255,255),
              AUTOMATIC:    wxColor(198,145,145),
              HEURISTIC:    wxColor(240,230,140),
              MANUAL:       wxColor(180,238,180)}
   def get_color(self, image):
      return self._colors.get(
         image.classification_state, wxColor(200,200,200))

   # Draws one cell of the grid
   def Draw(self, grid, attr, dc, rect, row, col, isSelected):
      scaling = self.parent.scaling
      
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
         scaled_image = None
         if scaling != 1.0:
            # Things are complicated by the ability to provide a global scaling
            # to all of the images in the grid. Here we handle the scaling and,
            # if necessary, we also do the cropping.
            height = ceil(image.nrows * scaling)
            width = ceil(image.ncols * scaling)
            if (height >= rect.GetHeight() or
                width >= rect.GetWidth()):
               # If the scaled version is going to still be too large to fit in
               # the grid cell, we crop it first and then scale it. We could just
               # scale the whole image and then crop that to the appropriate size,
               # but that could be very expensive. Instead we figure out how big
               # of a cropped image to create so that after scaling it is the
               # appropriate size.
               sub_height = min((rect.GetHeight() + 1) / scaling, image.nrows)
               sub_width = min((rect.GetWidth() + 1) / scaling, image.ncols)
               sub_image = image.subimage(image.offset_y, image.offset_x, sub_height, sub_width)
               scaled_image = sub_image.resize_copy(ceil(sub_image.nrows * scaling),
                                                    ceil(sub_image.ncols * scaling), 0)
            else:
               # This is the easy case - just scale the image.
               scaled_image = image.resize_copy(height, width, 0)
         else:
            # If we don't scale the image we can simply crop if the image is too big to fit
            # into the grid cell or otherwise do nothing.
            if (image.nrows >= rect.GetHeight() or
                image.ncols >= rect.GetWidth()):
               height = min(image.nrows, rect.GetHeight() + 1)
               width = min(image.ncols, rect.GetWidth() + 1)
               scaled_image = image.subimage(image.offset_y, image.offset_x, height, width)
            else:
               scaled_image = image

         wx_image = wxEmptyImage(scaled_image.ncols, scaled_image.nrows)
         scaled_image.to_buffer(wx_image.GetDataBuffer())
         bmp = wx_image.ConvertToBitmap()
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
         if self.parent.display_names:
            label = self.parent.get_label(image)
            if label != '':
               dc.SetLogicalFunction(wxXOR)
               dc.SetBackgroundMode(wxTRANSPARENT)
               dc.SetTextForeground(color)
               label = self.parent.reduce_label_length(dc, rect.width, label)
               dc.DrawText(label, rect.x, rect.y)
      if image is None or hasattr(image, 'dead'):
         # If there's no image in this cell, draw a hatch pattern
         dc.SetLogicalFunction(wxCOPY)
         dc.SetBackgroundMode(wxSOLID)
         dc.SetPen(wxTRANSPARENT_PEN)
         if image is None:
            dc.SetBrush(wxWHITE_BRUSH)
            dc.DrawRectangle(rect.x, rect.y, rect.width, rect.height)
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
                           image.ncols * self.parent.scaling + GRID_PADDING),
                       min(GRID_MAX_CELL_HEIGHT,
                           image.nrows * self.parent.scaling + GRID_PADDING))
      return wxSize(50, 50)

   def Clone(self):
      return MultiImageGridRenderer()


# Grid constants
GRID_MAX_CELL_WIDTH = 200
GRID_MAX_CELL_HEIGHT = 200
GRID_MAX_LABEL_LENGTH = 200
GRID_PADDING = 20
GRID_PADDING_2 = 16
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
      self.display_attribute = ""
      self.display_names = 0
      self.created = 0
      self.do_updates = 0
      self.last_image_no = None
      self.scaling = 1.0
      self.tooltip = wxButton(self.GetGridWindow(), -1, "",
                              wxPoint(0, 0), wxSize(175, 24))
      self.tooltip.Show(false)
      EVT_GRID_CELL_LEFT_DCLICK(self, self.OnLeftDoubleClick)
      EVT_GRID_CELL_RIGHT_CLICK(self, self.OnRightClick)
      EVT_GRID_SELECT_CELL(self, self.OnSelect)
      EVT_GRID_CELL_CHANGE(self, self.OnSelect)
      EVT_MOTION(self.GetGridWindow(), self.OnMotion)
      EVT_LEAVE_WINDOW(self.GetGridWindow(), self.OnLeave)

   ########################################
   # Sets a new list of images.  Can be performed multiple times
   def set_image(self, list, view_function=None):
      wxBeginBusyCursor()
      self.BeginBatch()
      self.list = list
      self.do_updates = 0
      self.sort_images()
      self.frame.set_choices()
      if not self.created:
         self.rows = 1
         self.CreateGrid(1, GRID_NCOLS)
         self.created = 1
      self.EnableEditing(0)
      self.resize_grid()
      self.GetGridWindow().Refresh()
      self.ClearSelection()
      x = self.GetSize()
      self.do_updates = 1
      self.EndBatch()
      wxEndBusyCursor()
      return (x.x, 600)

   def resize_grid(self, do_auto_size=1):
      if not self.created:
         return
      wxBeginBusyCursor()
      self.BeginBatch()
      orig_rows = self.rows
      rows = max(ceil((len(self.list) - 1) / GRID_NCOLS) + 1, 1)
      cols = GRID_NCOLS
      if self.rows < rows:
         self.AppendRows(rows - self.rows)
      elif self.rows > rows:
         self.DeleteRows(rows, self.rows - rows)
      self.rows = rows
      self.cols = cols
      width = self.set_labels()
      self.SetRowLabelSize(width + 20)
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
      self.GetParent().Layout()
      # self.SetSize(self.GetSize())
      # self.ForceRefresh()
      self.EndBatch()
      wxEndBusyCursor()

   def append_glyphs(self, list):
      wxBeginBusyCursor()
      self.BeginBatch()
      self.list.extend(list)
      self.resize_grid(do_auto_size=0)
      self.EndBatch()
      wxEndBusyCursor()

   def scale(self, scaling):
      if self.scaling != scaling:
         self.scaling = scaling
         x = self.GetSize()
         self.AutoSize()
         self.SetSize(x)
         self.MakeCellVisible(self.GetGridCursorRow(), self.GetGridCursorCol())

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

   def GetAllItems(self):
      return [x for x in self.list if x != None and not hasattr(x, 'dead')]

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

   def ZoomOut(self):
      if self.scaling > pow(2, -8):
         self.scale(self.scaling * 0.5)

   def ZoomNorm(self):
      self.scale(1.0)

   def ZoomIn(self):
      if self.scaling < pow(2, 8):
         self.scale(self.scaling * 2.0)
   
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
         image_menu.ImageMenu(self, position.x, position.y,
                              image, mode=0)
         self.ForceRefresh()

   def OnLeftDoubleClick(self, event):
      bitmap_no = self.get_image_no(event.GetRow(), event.GetCol())
      if bitmap_no != None:
         self.list[bitmap_no].display()

   def get_label(self, glyph):
      if self.display_attribute == '':
         return glyph.get_main_id()
      else:
         try:
            label = str(
               eval(self.display_attribute,
                    {'x': glyph}))
         except:
            label = '<ERROR!>'
         return label

   def reduce_label_length(self, dc, width, label):
      extent = dc.GetTextExtent(label)
      while extent[0] > width:
         if len(label) < 5:
            label = '...'
            break
         label = "..." + label[5:]
         extent = dc.GetTextExtent(label)
      return label

   def set_tooltip(self, label):
      self.tooltip.SetLabel(label)
      dc = wxClientDC(self.tooltip)
      extent = dc.GetTextExtent(label)
      self.tooltip.SetDimensions(
         -1,-1,extent[0]+6,extent[1]+6,wxSIZE_AUTO)

   def OnMotion(self, event):
      origin = self.GetViewStart()
      units = self.GetScrollPixelsPerUnit()
      row = self.YToRow(event.GetY() + origin[1] * units[1])
      col = self.XToCol(event.GetX() + origin[0] * units[0])
      image_no = self.get_image_no(row, col)
      if image_no == None or image_no >= len(self.list) or image_no < 0:
         image = None
      else:
         image = self.list[image_no]

      if image == None:
         self.tooltip.Show(false)
         last_image_no = None
      else:
         self.tooltip.Show(1)
         if self.last_image_no != image_no:
            self.set_tooltip(self.get_label(image))
            self.last_image_no = image_no
         self.tooltip.Move(wxPoint(event.GetX() + 16, event.GetY() + 16))
      event.Skip()

   def display_label_at_cell(self, row, col, label):
      rect = self.CellToRect(row, col)
      origin = self.GetViewStart()
      units = self.GetScrollPixelsPerUnit()
      self.tooltip.Show()
      self.set_tooltip(label)
      self.tooltip.Move(wxPoint(rect.GetLeft() - origin[0] * units[0],
                                rect.GetBottom() - origin[1] * units[1] + 1))
 
   def OnLeave(self, event):
      x, y = event.GetX(), event.GetY()
      w, h = self.GetSizeTuple()
      if x < 0 or x > w or y < 0 or y > h:
         self.tooltip.Hide()
      event.Skip()

class MultiImageWindow(wxPanel):
   def __init__(self, parent = None, id = -1, title = "Gamera", owner=None):
      from gamera.gui import gamera_icons
      wxPanel.__init__(self, parent, id)
      self.SetAutoLayout(true)
      self.toolbar = toolbar.ToolBar(self, -1)

      self.toolbar.AddSimpleTool(
         10, gamera_icons.getIconRefreshBitmap(),
         "Refresh", self.OnRefreshClick)
      self.toolbar.AddSeparator()
      self.toolbar.AddSimpleTool(
         20, gamera_icons.getIconZoomInBitmap(),
         "Zoom in", self.OnZoomInClick)
      self.toolbar.AddSimpleTool(
         21, gamera_icons.getIconZoomNormBitmap(),
         "Zoom 100%", self.OnZoomNormClick)
      self.toolbar.AddSimpleTool(
         22, gamera_icons.getIconZoomOutBitmap(),
         "Zoom out", self.OnZoomOutClick)
      self.toolbar.AddSeparator()
      self.toolbar.AddControl(wxStaticText(self.toolbar, -1, "Display: "))
      self.display_text_combo = wxComboBox(self.toolbar, 50, choices=[],
                                     size = wxSize(150, 20))
      EVT_COMBOBOX(self.display_text_combo, 50, self.OnChangeDisplayText)
      self.toolbar.AddControl(self.display_text_combo)
      self.toolbar.AddSimpleTool(
         24, gamera_icons.getIconShowNameBitmap(),
         "Display classes on grid", self.OnDisplayClasses, 1)
      self.toolbar2 = toolbar.ToolBar(self, -1)
      self.toolbar2.AddControl(wxStaticText(self.toolbar2, -1, "Sort: "))
      self.sort_combo = wxComboBox(self.toolbar2, 100, choices=[],
                                   size=wxSize(150, 20))
      self.toolbar2.AddControl(self.sort_combo)
      self.toolbar2.AddSimpleTool(
         101, gamera_icons.getIconSortAscBitmap(),
         "Sort Ascending", self.OnSortAscending)
      self.toolbar2.AddSimpleTool(
         102, gamera_icons.getIconSortDecBitmap(),
         "Sort Descending", self.OnSortDescending)
      self.toolbar2.AddSeparator()
      self.toolbar2.AddControl(wxStaticText(self.toolbar2, -1, "Select: "))
      self.select_combo = wxComboBox(self.toolbar2, 103, choices=[],
                                     size=wxSize(150, 20))
      self.toolbar2.AddControl(self.select_combo)
      self.toolbar2.AddSimpleTool(
         104, gamera_icons.getIconSelectBitmap(),
         "Select by given expression", self.OnSelect)
      self.toolbar2.AddSimpleTool(
         105, gamera_icons.getIconSelectAllBitmap(),
         "Select All", self.OnSelectAll)
      self.select_choices = []
      lc = wxLayoutConstraints()
      lc.top.SameAs(self, wxTop, 0)
      lc.left.SameAs(self, wxLeft, 0)
      lc.right.SameAs(self, wxRight, 0)
      lc.height.AsIs()
      self.toolbar.SetAutoLayout(true)
      self.toolbar.SetConstraints(lc)
      lc = wxLayoutConstraints()
      lc.top.Below(self.toolbar, 0)
      lc.left.SameAs(self, wxLeft, 0)
      lc.right.SameAs(self, wxRight, 0)
      lc.height.AsIs()
      self.toolbar2.SetAutoLayout(true)
      self.toolbar2.SetConstraints(lc)
      self.id = self.get_display()
      lc = wxLayoutConstraints()
      lc.top.Below(self.toolbar2, 0)
      lc.left.SameAs(self, wxLeft, 0)
      lc.right.SameAs(self, wxRight, 0)
      lc.bottom.SameAs(self, wxBottom, 0)
      self.id.SetAutoLayout(true)
      self.id.SetConstraints(lc)
      self.Layout()
      self.sort_choices = []

   # This can be overridden to change the internal display class
   def get_display(self):
      return MultiImageDisplay(self)

   def set_choices(self):
      methods = [x[0] + "()" for x in ImageBase.methods_flat_category("Features", ONEBIT)]

      self.display_choices = [
         "x.get_main_id()",
         "(x.offset_y, x.offset_x, x.nrows, x.ncols)",
         "x.label",
         "x.properties"]
      self.display_text_combo.Clear()
      for choice in self.display_choices:
         self.display_text_combo.Append(choice)

      self.sort_choices = [
         "", "offset_x", "offset_y",
         "ncols", "nrows",
         "label", "get_main_id()",
         "classification_state"]
      self.sort_combo.Clear()
      for choice in self.sort_choices:
         self.sort_combo.Append(choice)
      for method in methods:
         self.sort_combo.Append(method)

      self.select_choices = [
         "", "x.unclassified()",
         "x.automatically_classified()",
         "x.manually_classified()",
         "x.nrows < 2 or x.ncols < 2"]
      self.select_combo.Clear()
      for choice in self.select_choices:
         self.select_combo.Append(choice)

   def close(self):
      self.Destroy()

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
            gui_util.message(
               "The given sorting expressing contains a syntax error.")
            return
         try:
            sort_func = eval(final, globals(), image_menu.shell.locals)
         except Exception, e:
            gui_util.message(str(e))
            return
      else:
         try:
            for image in self.id.list:
               image.sort_cache = eval("x." + sort_string, {'x': image})
         except Exception, e:
            gui_util.message(str(e))
         sort_func = util.fast_cmp
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
      except Exception, e:
         gui_util.message(str(e))
         return
      if select_string not in self.select_choices:
         self.select_choices.append(select_string)
         self.select_combo.Append(select_string)
      wxBeginBusyCursor()
      self.id.select_images(select_func)
      wxEndBusyCursor()

   def OnSelectAll(self, event):
      wxBeginBusyCursor()
      self.id.SelectAll()
      wxEndBusyCursor()

   def OnSelectInvert(self, event):
      self.id.SelectInvert()

   def OnZoomInClick(self, event):
      self.id.ZoomIn()

   def OnZoomNormClick(self, event):
      self.id.ZoomNorm()

   def OnZoomOutClick(self, event):
      self.id.ZoomOut()

   def OnChangeDisplayText(self, event):
      value = self.display_text_combo.GetValue()
      self.id.display_attribute = value
      if value not in self.display_choices:
         self.display_choices.append(value)
         self.display_text_combo.Append(value)
      if self.id.display_names:
         self.id.ForceRefresh()
         

   def OnDisplayDetails(self, event):
      self.id.display_details = event.GetIsDown()
      if self.id.display_names:
         self.id.Refresh()

   def OnDisplayClasses(self, event):
      self.id.display_names = event.GetIsDown()
      self.id.Refresh()

##############################################################################
# TOP-LEVEL FRAMES
##############################################################################

class ImageFrameBase:
   def __init__(self, parent = None, id = -1, title = "Gamera", owner=None):
      self._frame = wxFrame(parent, id, title,
                           wxDefaultPosition, (600, 400))
      if (owner != None):
         self.owner = weakref.proxy(owner)
      else:
         self.owner = None
      EVT_CLOSE(self._frame, self._OnCloseWindow)

   def set_image(self, image, view_function=None):
      size = self._iw.id.set_image(image, view_function)
      self._frame.SetSize((max(200, min(600, size[0] + 30)),
                           max(200, min(400, size[1] + 60))))

   def close(self):
      self._iw.Destroy()
      self._frame.Destroy()

   def refresh(self):
      self._iw.id.refresh(0)

   def add_click_callback(self, cb):
      self._iw.id.add_click_callback(cb)

   def remove_click_callback(self, cb):
      self._iw.id.remove_click_callback(cb)

   def _OnCloseWindow(self, event):
      del self._iw
      if self.owner:
         self.owner.set_display(None)
      self._frame.Destroy()
      del self._frame
      print "hi"

   def Show(self, flag=1):
      self._frame.Show(flag)
   show = Show

class ImageFrame(ImageFrameBase):
   def __init__(self, parent = None, id = -1, title = "Gamera", owner=None):
      ImageFrameBase.__init__(self, parent, id, title, owner)
      self._iw = ImageWindow(self._frame)
      from gamera.gui import gamera_icons
      icon = wxIconFromBitmap(gamera_icons.getIconImageBitmap())
      self._frame.SetIcon(icon)

   def __repr__(self):
      return "<ImageFrame Window>"

   def highlight_cc(self, cc):
      self._iw.id.highlight_cc(cc)
   highlight_ccs = highlight_cc

   def unhighlight_cc(self, cc):
      self._iw.id.unhighlight_cc(cc)
   unhighlight_ccs = unhighlight_cc

   def clear_all_highlights(self):
      self._iw.id.clear_all_highlights()

   def focus(self, rect):
      self._iw.id.focus(rect)


class MultiImageFrame(ImageFrameBase):
   def __init__(self, parent = None, id = -1, title = "Gamera", owner=None):
      ImageFrameBase.__init__(self, parent, id, title, owner)
      self._iw = MultiImageWindow(self._frame)
      from gamera.gui import gamera_icons
      icon = wxIconFromBitmap(gamera_icons.getIconImageListBitmap())
      self._frame.SetIcon(icon)

   def __repr__(self):
      return "<MultiImageFrame Window>"

   def set_image(self, image, view_function=None):
      size = self._iw.id.set_image(image, view_function)


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
      self.image.to_string(image.GetDataBuffer())
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
