# -*- mode: python; indent-tabs-mode: nil; tab-width: 3 -*-
# vim: set tabstop=3 shiftwidth=3 expandtab:
#
# Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom,
#                         and Karl MacMillan
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

from __future__ import generators

import wx
import wx.grid as gridlib

from math import sqrt, ceil, log, floor # Python standard library
from sys import maxint
import sys, string, weakref

from gamera.core import *          # Gamera specific
from gamera.config import config
from gamera import paths, util, plugin
from gamera.gui import image_menu, var_name, gui_util, toolbar, has_gui
import gamera.plugins.gui_support  # Gamera plugin

##############################################################################

# we want this done on import
wx.InitAllImageHandlers()

def _sort_by_nrows(a, b):
   return cmp(a.nrows, b.nrows)

def _sort_by_ncols(a, b):
   return cmp(a.nrows, b.nrows)

#############################################################################

##############################################################################
# SINGLE IMAGE DISPLAY
##############################################################################
#
# Image display is a scrolled window for displaying a single image

class ImageDisplay(wx.ScrolledWindow, util.CallbackObject):
   def __init__(self, parent, id = -1, size = wx.DefaultSize):
      util.CallbackObject.__init__(self)
      wx.ScrolledWindow.__init__(
         self, parent, id, wx.Point(0, 0), size,
         wx.CLIP_CHILDREN|wx.NO_FULL_REPAINT_ON_RESIZE)
      self.SetBackgroundColour(wx.WHITE)

      self.scaling = 1.0
      self.scaling_quality = 0
      self.view_function = None
      self.image = None
      self.original_image = None
      self.highlights = []
      self.boxed_highlights = 0
      self._boxed_highlight = None
      self._boxed_highlight_position = 0
      self._boxed_highlight_timer = wx.Timer(self, 100)
      wx.EVT_TIMER(self, 100, self._OnBoxHighlight)
      self.boxes = []
      self.rubber_on = 0
      self.rubber_origin_x = 0
      self.rubber_origin_y = 0
      self.rubber_x2 = 0
      self.rubber_y2 = 0
      self.current_color = 0
      self.dragging = 0
      self.scroll_amount = 32
      self.block_w = 8
      self.block_h = 8

      wx.EVT_PAINT(self, self._OnPaint)
      wx.EVT_SIZE(self, self._OnResize)
      wx.EVT_LEFT_DOWN(self, self._OnLeftDown)
      wx.EVT_LEFT_UP(self, self._OnLeftUp)
      wx.EVT_MIDDLE_DOWN(self, self._OnMiddleDown)
      wx.EVT_MIDDLE_UP(self, self._OnMiddleUp)
      wx.EVT_RIGHT_DOWN(self, self._OnRightDown)
      wx.EVT_MOTION(self, self._OnMotion)
      wx.EVT_LEAVE_WINDOW(self, self._OnLeave)
      wx.EVT_MOUSEWHEEL(self, self._OnMouseWheel)

   if wx.VERSION >= (2, 5):
      def SetScrollbars(self, x_amount, y_amount, w, h, x, y):
         self.SetVirtualSize((w * x_amount, h * y_amount))
         self.SetScrollRate(x_amount, y_amount)
         self.Scroll(x, y)

   ########################################
   # THE MAIN IMAGE

   # Sets the image being displayed
   # Returns the size of the image
   def set_image(self, image, view_function=None, weak=True):
      if weak:
         self.original_image = weakref.proxy(image)
         self.image = weakref.proxy(image)
      else:
         self.original_image = image
         self.image = image
      self.clear_all_highlights()
      self.view_function = view_function
      self.Raise()
      return self.reload_image()

   # Refreshes the image by recalling the to_string function
   def reload_image(self, *args):
      if self.view_function != None and self.original_image != None:
         self.image = getattr(self.original_image, self.view_function)()
      if self.image != None:
         self.scale()
         return (self.image.width, self.image.height)

   ########################################
   # BOXES

   def add_box(self, y, x, h, w):
      self.boxes.append(Rect(y, x, h, w))
      self.RefreshAll()

   def clear_all_boxes(self):
      self.boxes = []
      self.RefreshAll()

   def draw_box(self, box, dc=None):
      if dc == None:
         dc = wx.ClientDC(self)
      scaling = self.scaling
      origin = [x * self.scroll_amount for x in self.GetViewStart()]

      x, y, x2, y2 = tuple([int(a * scaling) for a in (box.ul_x, box.ul_y,
                                                       box.lr_x + 1, box.lr_y + 1)])
      x -= origin[0]
      y -= origin[1]
      x2 -= origin[0]
      y2 -= origin[1]
      w = x2 - x
      h = y2 - y

      dc.SetLogicalFunction(wx.COPY)
      pen = wx.Pen(wx.Color(0, 0, 255), 2, wx.SOLID)
      dc.SetPen(pen)
      dc.SetBrush(wx.TRANSPARENT_BRUSH)      
      dc.DrawRectangle(x, y, w, h)

   def draw_boxes(self, dc=None):
      for b in self.boxes:
         self.draw_box(b, dc)


   ########################################
   # HIGHLIGHTED CCs

   def _OnBoxHighlight(self, event):
      if len(self.highlights) == 1 and not self.rubber_on:
         self._boxed_highlight_position &= 0x7  # mod 8
         highlight = self.highlights[0][0]
         i = (16 + abs(4 - self._boxed_highlight_position)) / self.scaling
         self.draw_rubber()
         self.rubber_origin_x = highlight.ul_x - i
         self.rubber_origin_y = highlight.ul_y - i
         self.rubber_x2 = highlight.lr_x + i
         self.rubber_y2 = highlight.lr_y + i
         self._boxed_highlight_position -= 1
         self.draw_rubber()

   def highlight_rectangle(self, r, color, text=None):
      cc = self.image.subimage(r)
      self.add_highlight_cc(cc, color)

   # Highlights only a particular list of ccs in the display
   def highlight_cc(self, ccs, color=None):
      ccs = util.make_sequence(ccs)
      # For efficiency, if we're updating fewer than 10 glyphs, we only
      # update that part of the display, otherwise, we update the entire
      # display
      refresh_at_once = len(ccs) > 10 or len(self.highlights) > 10
      use_color = color
      old_highlights = self.highlights[:]
      self.highlights = []
      for cc in ccs:
         # If the highlight is in the old highlights, use the old
         # color if the color is unspecified.
         # This is really just for "color stability."
         if color == None:
            use_color = None
            for old_cc, old_color in old_highlights:
               if old_cc == cc:
                  use_color = old_color
                  break
         self.highlights.append((cc, gui_util.get_color(use_color)))
         if not refresh_at_once:
            self.PaintAreaRect(cc)
      if not refresh_at_once:
         for cc in old_highlights:
            if cc not in self.highlights:
               self.PaintAreaRect(cc[0])
      else:
         self.RefreshAll()
   highlight_ccs = highlight_cc

   # Adds a list of ccs to be highlighted in the display
   def add_highlight_cc(self, ccs, color=None):
      ccs = util.make_sequence(ccs)
      # For efficiency, if we're updating fewer than 10 glyphs, we only
      # update that part of the display, otherwise, we update the entire
      # display
      refresh_at_once = len(ccs) > 10
      use_color = color
      for cc in ccs:
         # Only add cc if not already in highlighted list
         add = 1
         for hl, color in self.highlights:
            if cc == hl:
               add = 0
               break
         if add:
            self.highlights.append(
               (cc, gui_util.get_color(use_color)))
            if not refresh_at_once:
               self.PaintAreaRect(cc)
      if refresh_at_once:
         self.RefreshAll()
   add_highlight_ccs = add_highlight_cc

   # Clears a CC in the display.  Multiple CCs
   # may be unhighlighted in turn
   def unhighlight_cc(self, ccs):
      ccs = util.make_sequence(ccs)
      refresh_at_once = len(ccs) > 10
      for cc in ccs:
         for i in range(0, len(self.highlights)):
            if (self.highlights[i][0] == cc):
               del self.highlights[i]
               if not refresh_at_once:
                  self.PaintAreaRect(cc)
               break
      if refresh_at_once:
         self.RefreshAll()
   unhighlight_ccs = unhighlight_cc
      
   # Clears all of the highlighted CCs in the display   
   def clear_all_highlights(self):
      old_highlights = self.highlights[:]
      refresh_at_once = len(old_highlights) > 10
      self.highlights = []
      if not refresh_at_once:
         for highlight in old_highlights:
            self.PaintAreaRect(highlight[0])
      else:
         self.RefreshAll()

   # Adjust the scrollbars so a group of highlighted subimages are visible
   def focus_glyphs(self, glyphs):
      glyphs = util.make_sequence(glyphs)
      if len(glyphs) == 0:
         return
      # Get a combined rectangle of all images in the list
      self.focus_rect(glyphs[0].union_rects(glyphs))

   def focus_rect(self, rect):
      self.focus_points(rect.ul_x, rect.ul_y, rect.lr_x, rect.lr_y)

   def focus_points(self, x1, y1, x2, y2, force=0):
      if self.image is None:
         return
      scroll_amount = self.scroll_amount
      # Adjust for the scaling factor
      scaling = self.scaling
      x1, y1, x2, y2 = tuple([x * scaling for x in (x1, y1, x2, y2)])
      origin_x, origin_y = tuple(
         [x * self.scroll_amount for x in self.GetViewStart()])
      maxwidth = self.image.width * scaling
      maxheight = self.image.height * scaling
      size = self.GetSize()
      if force:
         need_to_scroll = 1
         set_x = max(0, min(maxwidth - size.x / self.scaling, x1 - 20))
         set_y = max(0, min(maxheight - size.y / self.scaling, y1 - 20))
      else:
         need_to_scroll = 0
         if (x1 < origin_x or x2 > origin_x + size.x - 32):
            set_x = max(0, min(maxwidth - size.x / self.scaling, x1 - 20))
            need_to_scroll = 1
         else:
            set_x = origin_x
         if (y1 < origin_y or y2 > origin_y + size.y - 32 or
             need_to_scroll):
            set_y = max(0, min(maxheight - size.y / self.scaling, y1 - 20))
            need_to_scroll = 1
         else:
            set_y = origin_y
      if need_to_scroll:
         self.SetScrollbars(scroll_amount, scroll_amount,
                            int(maxwidth / scroll_amount),
                            int(maxheight / scroll_amount),
                            int(set_x / scroll_amount),
                            int(set_y / scroll_amount))
         self.RefreshAll()

   def _OnBoxHighlightsToggle(self, event):
      self.boxed_highlights = event.GetIsDown()
      if self.boxed_highlights:
         if not self._boxed_highlight_timer.IsRunning():
            self._boxed_highlight_timer.Start(100)
      else:
         if self._boxed_highlight_timer.IsRunning():
            self._boxed_highlight_timer.Stop()
      self.RefreshAll()

   ########################################
   # SCALING
   #
   def scale(self, new_scale=None):
      if self.image is None:
         return
      old_scale = self.scaling
      if new_scale < old_scale:
         # self.Clear()
         self.RefreshAll()
      if new_scale == None or new_scale <= 0:
         new_scale = old_scale
         
      # Clamp scaling to a reasonable range
      # Going outside of this range has been known to cause segfaults
      new_scale = min(max(new_scale, 0.1), 16)

      # Quantize scaling to integers, (or 1.0 / integer)
      # This makes scrolling look nice
      if new_scale >= 1.0:
         new_scale = floor(new_scale)
      else:
         new_scale = 1.0 / ceil(1.0 / new_scale)
      scroll_amount = self.scroll_amount
      w = self.image.width * new_scale
      h = self.image.height * new_scale
      origin = [x * self.scroll_amount for x in self.GetViewStart()]
      size = self.GetSize()
      # These weird looking functions ensure that the scrollbars don't
      # jump around too much when zooming in/out
      x = max(min(origin[0] * new_scale / old_scale + 1, w - size.x) - 2, 0)
      y = max(min(origin[1] * new_scale / old_scale + 1, h - size.y) - 2, 0)
      self.scaling = new_scale

      wx.BeginBusyCursor()
      try:
         if self.highlights == []:
            self.SetScrollbars(scroll_amount, scroll_amount,
                               int(w / scroll_amount),
                               int(h / scroll_amount),
                               int((x / scroll_amount) + 0.5),
                               int((y / scroll_amount) + 0.5))
         else:
            self.focus_glyphs([x[0] for x in self.highlights])
      finally:
         self.RefreshAll()
         wx.EndBusyCursor()
         
   def ZoomOut(self, *args):
      if self.scaling > 0.125:
         self.scale(self.scaling * 0.5)

   def ZoomNorm(self, *args):
      self.scale(1.0)

   def ZoomIn(self, *args):
      if self.scaling < 64:
         self.scale(self.scaling * 2.0)

   def ZoomView(self, *args):
      if self.image is None:
         return
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
                    float(size.y) / float(rubber_h)) * 0.95
      self.scale(scaling)
      self.focus_points(x, y, x2, y2, force=1)

   def SetZoomType(self, type):
      self.scaling_quality = type
      if self.scaling != 1.0:
         self.RefreshAll()

   ########################################
   # RUBBER BAND
   #
   def draw_rubber(self, dc=None):
      if dc == None:
         dc = wx.ClientDC(self)
      scaling = self.scaling
      origin = [x * self.scroll_amount for x in self.GetViewStart()]
      x = min(self.rubber_origin_x, self.rubber_x2)
      y = min(self.rubber_origin_y, self.rubber_y2)
      x2 = max(self.rubber_origin_x, self.rubber_x2)
      y2 = max(self.rubber_origin_y, self.rubber_y2)
      x, y, x2, y2 = tuple([int(a * scaling) for a in (x, y, x2, y2)])
      x -= origin[0]
      y -= origin[1]
      x2 -= origin[0]
      y2 -= origin[1]
      w = x2 - x
      h = y2 - y
      dc.SetLogicalFunction(wx.XOR)
      pen = wx.GREY_PEN
      pen.SetStyle(wx.DOT)
      dc.SetPen(pen)
      dc.SetBrush(wx.TRANSPARENT_BRUSH)
      dc.DrawRectangle(x, y, w, h)
      dc.SetPen(wx.TRANSPARENT_PEN)
      brush = wx.BLUE_BRUSH
      brush.SetColour(wx.Color(167, 105, 39))
      dc.SetBrush(brush)
      self.block_w = block_w = max(min(w / 2 - 1, 8), 0)
      self.block_h = block_h = max(min(h / 2 - 1, 8), 0)
      dc.DrawRectangle(x + 1, y + 1, block_w, block_h)
      dc.DrawRectangle(x2 - block_w - 1, y + 1, block_w, block_h)
      dc.DrawRectangle(x + 1, y2 - block_h - 1, block_w, block_h)
      dc.DrawRectangle(x2 - block_w - 1, y2 - block_h - 1, block_w, block_h)
      dc.SetLogicalFunction(wx.COPY)

   ########################################
   # UTILITY
   #
   def _OnMakeView(self, *args):
      name = var_name.get("view", image_menu.shell.locals)
      if name:
         if (self.rubber_y2 == self.rubber_origin_y and
             self.rubber_x2 == self.rubber_origin_x):
            subimage = self.original_image.subimage(self.original_image)
         else:
            subimage = self.original_image.subimage(
               (int(self.rubber_origin_x + self.original_image.ul_x),
                int(self.rubber_origin_y + self.original_image.ul_y)),
               Size(int(self.rubber_x2 - self.rubber_origin_x + 1),
                    int(self.rubber_y2 - self.rubber_origin_y)))
         image_menu.shell.locals[name] = subimage
         image_menu.shell.update()

   def _OnMakeCopy(self, *args):
      name = var_name.get("copy", image_menu.shell.locals)
      if name:
         if (self.rubber_y2 == self.rubber_origin_y and
             self.rubber_x2 == self.rubber_origin_x):
            copy = self.original_image.image_copy()
         else:
            copy = self.original_image.subimage(
               (int(self.rubber_origin_x + self.original_image.ul_x),
                int(self.rubber_origin_y + self.original_image.ul_y)),
               Size(int(self.rubber_x2 - self.rubber_origin_x + 1),
                    int(self.rubber_y2 - self.rubber_origin_y))).image_copy()
         image_menu.shell.locals[name] = copy
         image_menu.shell_frame.icon_display.update_icons()

   def _OnSave(self, *args):
      filename = gui_util.save_file_dialog(self, util.get_file_extensions("save"))
      self.original_image.save_image(filename)

   def _OnPrint(self, *args):
      dialog_data = wx.PrintDialogData()
      # TODO: This should theoretically work with wxPython 2.5 +,
      # but it actually causes OnPrint to never be called.
      if wx.VERSION < (2, 5):
         dialog_data.EnableHelp(False)
         dialog_data.EnablePageNumbers(False)
         dialog_data.EnableSelection(False)
      dialog_data.SetToPage(1)
      printer = wx.Printer(dialog_data)
      printout = GameraPrintout(self.original_image)
      if not printer.Print(self, printout, True):
         if printer.GetLastError() == wx.PRINTER_ERROR:
            gui_util.message("A printing error occurred.")
      printout.Destroy()

   ########################################
   # CALLBACKS
   #
   def _OnResize(self, event):
      if not hasattr(self, 'image'):
         return
      size = self.GetSize()
      if size.x > 0 and size.y > 0:
         event.Skip()
         self.scale(self.scaling)
      else:
         event.Skip()

   def _OnPaint(self, event):
      # If there's no image, draw the hatch pattern
      if not self.image:
         dc = wx.PaintDC(self)
         dc.SetPen(wx.TRANSPARENT_PEN)
         dc.SetBrush(wx.WHITE_BRUSH)
         size = self.GetSize()
         dc.DrawRectangle(0, 0, size.x, size.y)
         dc.SetBrush(wx.Brush(wx.BLUE, wx.FDIAGONAL_HATCH))
         dc.DrawRectangle(0, 0, size.x, size.y)
         return

      scaling = self.scaling
      PaintArea = self.PaintArea
      origin = [x * self.scroll_amount for x in self.GetViewStart()]

      dc = wx.PaintDC(self)
      dc.BeginDrawing()
      tmpdc = wx.MemoryDC()

      update_regions = self.GetUpdateRegion()
      rects = wx.RegionIterator(update_regions)
      # Paint only where wx.Windows tells us to, this is faster
      while rects.HaveRects():
         x1 = origin[0] + rects.GetX()
         y1 = origin[1] + rects.GetY()
         PaintArea(x1, y1, x1 + rects.GetW(), y1 + rects.GetH(),
                   check=False, dc=dc, tmpdc=tmpdc)
         rects.Next()

      self.draw_rubber(dc)
      self.draw_boxes(dc)
      dc.EndDrawing()

   def PaintArea(self, x1, y1, x2, y2, check=True, dc=None, tmpdc=None):
      if not self.image:
         return

      origin = [a * self.scroll_amount for a in self.GetViewStart()]
      size = self.GetSizeTuple()

      # If the update region is outside of the view, or there's
      # something wrong with it, just return immediately
      if check:
         if ((x2 < origin[0]) or
             (y2 < origin[1]) or
             (x1 > origin[0] + size[0]) or
             (y1 > origin[1] + size[1]) or
             x2 < x1 or y2 < y1):
            return

      if dc is None:
         dc = wx.ClientDC(self)
         self.draw_rubber(dc)
         redraw_rubber = True
      else:
         redraw_rubber = False

      if tmpdc is None:
         tmpdc = wx.MemoryDC()

      dc.SetPen(wx.TRANSPARENT_PEN)
      dc.SetBrush(wx.Brush(wx.WHITE, wx.SOLID))
      dc.DrawRectangle(x1 - origin[0], y1 - origin[1], x2 - x1, y2 - y1)
   
      # Localise some member variables
      scaling = self.scaling
      scaling_quality = self.scaling_quality
      image = self.image

      x1 = max(x1 - scaling * 2, 0)
      y1 = max(y1 - scaling * 2, 0)
      x2 = min(x2 + scaling * 2, image.width * scaling)
      y2 = min(y2 + scaling * 2, image.height * scaling)

      x1i = max(floor(x1 / scaling), 0)
      y1i = max(floor(y1 / scaling), 0)
      x2i = min(floor(x2 / scaling), image.width)
      y2i = min(floor(y2 / scaling), image.height)

      # Quantize the origin
      x = floor((x1 - origin[0]) / scaling) * scaling
      y = floor((y1 - origin[1]) / scaling) * scaling
         
      if x2i <= x1i or y2i <= y1i:
         return
         
      subimage = image.subimage(
         (x1i + image.ul_x, y1i + image.ul_y),
         (x2i + image.ul_x, y2i + image.ul_y)).to_rgb()

      if len(self.highlights):
         for highlight, color in self.highlights:
            subimage.draw_cc(
               highlight, 
               color.Red(), color.Green(), color.Blue())

      scaled_image = subimage.scale(scaling, scaling_quality)

      image = wx.EmptyImage(scaled_image.ncols, scaled_image.nrows)
      scaled_image.to_buffer(image.GetDataBuffer())

      bmp = wx.BitmapFromImage(image)
      tmpdc.SelectObject(bmp)
      dc.Blit(x, y, scaled_image.ncols, scaled_image.nrows,
              tmpdc, 0, 0, wx.COPY, True)

      if redraw_rubber:
         self.draw_rubber(dc)

   def PaintAreaRect(self, rect):
      # When painting a specific area, we have to make it
      # slightly bigger to adjust for scaling
      scaling = self.scaling
      adjust = self.scaling * 2
      self.PaintArea(rect.ul_x * scaling, rect.ul_y * scaling,
                     (rect.lr_x + 1) * scaling, (rect.lr_y + 1) * scaling, 1)
            
   def RefreshAll(self):
      size = self.GetSize()
      self.Refresh(0, rect=wx.Rect(0, 0, size.x, size.y))

   def _OnLeftDown(self, event):
      if not self.image:
         return
      self.CaptureMouse()
      self.rubber_on = 1
      self.draw_rubber()
      origin = [x * self.scroll_amount for x in self.GetViewStart()]
      x = min((event.GetX() + origin[0]) / self.scaling,
              self.image.ncols - 1)
      y = min((event.GetY() + origin[1]) / self.scaling,
              self.image.nrows - 1)
      if (x >= self.rubber_origin_x and
          x <= self.rubber_origin_x + self.block_w):
         self.rubber_origin_x, self.rubber_x2 = \
                               self.rubber_x2, self.rubber_origin_x
         if (y >= self.rubber_origin_y and
             y <= self.rubber_origin_y + self.block_h):
            self.rubber_origin_y, self.rubber_y2 = \
                                  self.rubber_y2, self.rubber_origin_y
            self.draw_rubber()
            return
         elif (y <= self.rubber_y2 and
               y >= self.rubber_y2 - self.block_h):
            self.draw_rubber()
            return
      elif (x <= self.rubber_x2 and
            x >= self.rubber_x2 - self.block_w):
         if (y >= self.rubber_origin_y and
             y <= self.rubber_origin_y + self.block_h):
            self.rubber_origin_y, self.rubber_y2 = \
                                  self.rubber_y2, self.rubber_origin_y
            self.draw_rubber()
            return
         elif (y <= self.rubber_y2 and
               y >= self.rubber_y2 - self.block_h):
            self.draw_rubber()
            return
      self.rubber_origin_x = self.rubber_x2 = int(x)
      self.rubber_origin_y = self.rubber_y2 = int(y)

   def _OnLeftUp(self, event):
      if self.rubber_on:
         if self.HasCapture():
            self.ReleaseMouse()
         self.draw_rubber()
         origin = [x * self.scroll_amount for x in self.GetViewStart()]
         self.rubber_x2 = int(max(min((event.GetX() + origin[0]) / self.scaling,
                                      self.image.ncols - 1), 0))
         self.rubber_y2 = int(max(min((event.GetY() + origin[1]) / self.scaling,
                                      self.image.nrows - 1), 0))
         self.trigger_callback("click",
                               self.rubber_y2 + self.original_image.ul_y,
                               self.rubber_x2 + self.original_image.ul_x,
                               event.ShiftDown(), event.ControlDown())
         self.draw_rubber()
         if self.rubber_origin_x > self.rubber_x2:
            self.rubber_origin_x, self.rubber_x2 = \
                                  self.rubber_x2, self.rubber_origin_x
         if self.rubber_origin_y > self.rubber_y2:
            self.rubber_origin_y, self.rubber_y2 = \
                                  self.rubber_y2, self.rubber_origin_y
         self.rubber_on = 0
         self.trigger_callback("rubber",
                               self.rubber_origin_y + self.original_image.ul_y,
                               self.rubber_origin_x + self.original_image.ul_x,
                               self.rubber_y2 + self.original_image.ul_y,
                               self.rubber_x2 + self.original_image.ul_x,
                               event.ShiftDown(), event.ControlDown())

   def _OnMiddleDown(self, event):
      if not self.image:
         return
      wx.SetCursor(wx.StockCursor(wx.CURSOR_BULLSEYE))
      self.dragging = 1
      self.dragging_x = event.GetX()
      self.dragging_y = event.GetY()
      self.dragging_origin_x, self.dragging_origin_y = \
         [x * self.scroll_amount for x in self.GetViewStart()]

   def _OnMiddleUp(self, event):
      wx.SetCursor(wx.STANDARD_CURSOR)
      self.dragging = 0

   def _OnRightDown(self, event):
      if not self.image:
         return
      from gamera.gui import image_menu
      menu = image_menu.ImageMenu(
         self, event.GetX(), event.GetY(),
         self.image)
      if menu.did_something:
         self.reload_image()

   def _OnMouseWheel(self, event):
      if event.ControlDown():
         if event.GetWheelRotation() < 0:
            self.ZoomIn()
         else:
            self.ZoomOut()
      else:
         event.Skip()

   def _OnMotion(self, event):
      image = self.image
      if image is None:
         return
      scaling = self.scaling
      origin = [x * self.scroll_amount for x in self.GetViewStart()]
      x2 = int(max(min((event.GetX() + origin[0] - 1) / scaling, image.ncols - 1), 0))
      y2 = int(max(min((event.GetY() + origin[1] - 1) / scaling, image.nrows - 1), 0))
      if self.rubber_on:
         self.draw_rubber()
         self.rubber_x2 = x2
         self.rubber_y2 = y2
         self.draw_rubber()
      if self.dragging:
         self.Scroll(
            (self.dragging_origin_x - (event.GetX() - self.dragging_x)) /
            self.scroll_amount,
            (self.dragging_origin_y - (event.GetY() - self.dragging_y)) /
            self.scroll_amount)
      self.trigger_callback("move", y2 + self.original_image.ul_y, x2 + self.original_image.ul_x)

   def _OnLeave(self, event):
      if not self.HasCapture() and self.rubber_on:
         if self.rubber_origin_x > self.rubber_x2:
            self.rubber_origin_x, self.rubber_x2 = \
                                  self.rubber_x2, self.rubber_origin_x
         if self.rubber_origin_y > self.rubber_y2:
            self.rubber_origin_y, self.rubber_y2 = \
                                  self.rubber_y2, self.rubber_origin_y
         self.rubber_on = 0
         self.trigger_callback("rubber", self.rubber_origin_y, self.rubber_origin_x, self.rubber_y2, self.rubber_x2, event.ShiftDown(), event.ControlDown())
      if self.dragging:
         self.dragging = 0
         
class ImageWindow(wx.Panel):
   def __init__(self, parent = None, id = -1):
      wx.Panel.__init__(self, parent, id, style=
                       wx.NO_FULL_REPAINT_ON_RESIZE|wx.CLIP_CHILDREN)
      self.id = self.get_display()
      self.toolbar = toolbar.ToolBar(self, -1)
      from gamera.gui import gamera_icons
      self.toolbar.AddSimpleTool(10, gamera_icons.getIconRefreshBitmap(),
                                 "Refresh", self.id.reload_image)
      self.toolbar.AddSeparator()
      self.toolbar.AddSimpleTool(
         20, gamera_icons.getIconZoomInBitmap(),
         "Zoom in", self.id.ZoomIn)
      self.toolbar.AddSimpleTool(
         21, gamera_icons.getIconZoomNormBitmap(),
         "Zoom 100%", self.id.ZoomNorm)
      self.toolbar.AddSimpleTool(
         22, gamera_icons.getIconZoomOutBitmap(),
         "Zoom out", self.id.ZoomOut)
      self.toolbar.AddSimpleTool(
         23, gamera_icons.getIconZoomViewBitmap(),
         "Zoom in on selected region", self.id.ZoomView)
      self.zoom_slider = wx.ComboBox(
         self.toolbar, 24,
         choices=['low quality','medium quality','high quality'],
         style=wx.CB_READONLY)
      wx.EVT_COMBOBOX(self, 24, self._OnZoomTypeChange)
      self.toolbar.AddControl(self.zoom_slider)

      self.toolbar.AddSeparator()
      self.toolbar.AddSimpleTool(
         31, gamera_icons.getIconMakeViewBitmap(),
         "Make new view", self.id._OnMakeView)
      self.toolbar.AddSimpleTool(
         32, gamera_icons.getIconImageCopyBitmap(),
         "Make new copy", self.id._OnMakeCopy)

      self.toolbar.AddSeparator()
      self.toolbar.AddSimpleTool(
         400, gamera_icons.getIconSaveBitmap(),
         "Save image", self.id._OnSave)
      self.toolbar.AddSimpleTool(
         401, gamera_icons.getIconPrinterBitmap(),
         "Print image", self.id._OnPrint)

      box = wx.BoxSizer(wx.VERTICAL)
      box.Add(self.toolbar, 0, wx.EXPAND)
      box.Add(self.id, 1, wx.EXPAND)
      self.SetSizer(box)
      self.Fit()

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

if wx.VERSION < (2, 5):
   GridCellRenderer = gridlib.PyGridCellRenderer
else:
   GridCellRenderer = gridlib.PyGridCellRenderer
class MultiImageGridRenderer(GridCellRenderer):
   def __init__(self, parent):
      GridCellRenderer.__init__(self)
      self.parent = parent
      self.label_font = wx.Font(6, wx.SWISS, wx.NORMAL, wx.NORMAL)

   _colors = {UNCLASSIFIED: wx.Color(255,255,255),
              AUTOMATIC:    wx.Color(198,145,145),
              HEURISTIC:    wx.Color(240,230,140),
              MANUAL:       wx.Color(180,238,180)}

   # Draws one cell of the grid
   def Draw(self, grid, attr, dc, rect, row, col, isSelected):
      view_start = grid.GetViewStart()
      view_units = grid.GetScrollPixelsPerUnit()
      view_size = grid.GetClientSize()
      if not rect.Intersects(wx.Rect(
         view_start[0] * view_units[0],
         view_start[1] * view_units[1],
         view_size[0],
         view_size[1])):
         return

      scaling = self.parent.scaling
      cell_padding = grid.cell_padding
      image_list = self.parent.sorted_glyphs

      bitmap_no = row * grid.cols + col
      if bitmap_no < len(image_list):
         image = image_list[bitmap_no]
      else:
         image = None

      dc.BeginDrawing()

      if not image is None:
         classification_state = image.classification_state
         # Fill the background
         color = self._colors[classification_state]
         dc.SetPen(wx.TRANSPARENT_PEN)
         if isSelected:
            dc.SetBrush(wx.Brush(wx.BLACK, wx.SOLID))
         else:
            dc.SetBrush(wx.Brush(color, wx.SOLID))
         dc.DrawRectangle(rect.x, rect.y, rect.width, rect.height)
         # If the image is bigger than the cell, crop it. This used to be done
         # in wx.Python, but it is more efficient to do it with Gamera SubImages
         scaled_image = None
         if scaling != 1.0:
            # Things are complicated by the ability to provide a global scaling
            # to all of the images in the grid. Here we handle the scaling and,
            # if necessary, we also do the cropping.
            height = int(ceil(image.nrows * scaling))
            width = int(ceil(image.ncols * scaling))
            if (height >= rect.height or width >= rect.width):
               # If the scaled version is going to still be too large to fit in
               # the grid cell, we crop it first and then scale it. We could
               # just scale the whole image and then crop that to the
               # appropriate size, but that could be very expensive. Instead we
               # figure out how big of a cropped image to create so that after
               # scaling it is the appropriate size.
               sub_height = min(int((rect.height + 1) / scaling), image.nrows)
               sub_width = min(int((rect.width + 1) / scaling), image.ncols)
               sub_image = image.subimage(image.ul, Dim(sub_width, sub_height))
               if scaling < 1.0:
                  scaled_image = sub_image.to_greyscale().resize(
                     Dim(int(ceil(sub_width * scaling)),
                         int(ceil(sub_height * scaling))), 1)
               else:
                  scaled_image = sub_image.resize(
                     Dim(int(ceil(sub_width * scaling)),
                         int(ceil(sub_height * scaling))), 0)
            else:
               # This is the easy case - just scale the image.
               if scaling < 1.0:
                  scaled_image = image.to_greyscale().resize(
                     Dim(width, height), 1)
               else:
                  scaled_image = image.resize(Dim(width, height), 0)
         else:
            # If we don't scale the image we can simply crop if the image is too
            # big to fit into the grid cell or otherwise do nothing.
            if (image.nrows >= rect.height or image.ncols >= rect.width):
               height = min(image.nrows, rect.height + 1)
               width = min(image.ncols, rect.width + 1)
               scaled_image = image.subimage(image.ul, Dim(width, height))
            else:
               scaled_image = image

         width = scaled_image.ncols
         height = scaled_image.nrows
         x = rect.x + (rect.width - width) / 2
         y = rect.y + (rect.height - height) / 2
         wx_image = wx.EmptyImage(width, height)
         scaled_image.to_buffer_colorize(
            wx_image.GetDataBuffer(),
            color.Red(), color.Green(), color.Blue(),
            isSelected)
         bmp = wx_image.ConvertToBitmap()

         # Display centered within the cell
         tmp_dc = wx.MemoryDC()
         tmp_dc.SelectObject(bmp)
         dc.Blit(x, y, width, height, tmp_dc, 0, 0, wx.COPY)

         if self.parent.display_names:
            label = self.parent.get_label(image)
            if label != '':
               dc.SetBackgroundMode(wx.TRANSPARENT)
               # The default font is too big under windows - this should
               # probably be a configurable option . . .
               dc.SetFont(self.label_font)
               # the the logical function is ignored for Windows, so we have
               # to set the Foreground and Background colors manually
               if isSelected:
                  dc.SetTextForeground(color)
               else:
                  dc.SetTextForeground(wx.BLACK)
               label = self.parent.reduce_label_length(dc, rect.width, label)
               dc.DrawText(label, rect.x, rect.y)

      if image is None or hasattr(image, 'dead'):
         # If there's no image in this cell, draw a hatch pattern
         dc.SetLogicalFunction(wx.COPY)
         dc.SetBackgroundMode(wx.SOLID)
         dc.SetPen(wx.TRANSPARENT_PEN)
         if image is None:
            dc.SetBrush(wx.WHITE_BRUSH)
            dc.DrawRectangle(rect.x, rect.y, rect.width, rect.height)
            dc.SetBrush(wx.Brush(wx.BLUE, wx.FDIAGONAL_HATCH))
         else:
            dc.SetBrush(wx.Brush(wx.RED, wx.FDIAGONAL_HATCH))
         dc.DrawRectangle(rect.x, rect.y, rect.width, rect.height)
      dc.SetLogicalFunction(wx.COPY)
      dc.EndDrawing()

   # The images should be a little padded within the cells
   # Also, there is a max size for every cell
   def GetBestSize(self, grid, attr, dc, row, col):
      glyphs = self.parent.sorted_glyphs
      bitmap_no = row * grid.cols + col
      if bitmap_no < len(glyphs):
         image = glyphs[bitmap_no]
      else:
         image = None
      if not image is None:
         return wx.Size(
            min(grid.max_cell_width,
                int(image.ncols * grid.scaling + grid.cell_padding)),
            min(grid.max_cell_height,
                int(image.nrows * grid.scaling + grid.cell_padding)))
      else:
         return wx.Size(grid.cell_padding * 2, grid.cell_padding * 2)

   def Clone(self):
      return MultiImageGridRenderer(self.parent)

# Grid constants
config.add_option(
   '', '--grid-max-cell-width', default=200, type="int",
   help='[grid] Maximum width of a grid cell')
config.add_option(
   '', '--grid-max-cell-height', default=200, type="int",
   help='[grid] Maximum height of a grid cell')
config.add_option(
   '', '--grid-max-label-length', default=200, type="int",
   help='[grid] Maximum length (in pixels) of the row labels in the grid')
config.add_option(
   '', '--grid-cell-padding', default=8, type="int",
   help='[grid] Amount of padding around the glyphs in the grid')
config.add_option(
   '', '--grid-ncols', default=8, type="int",
   help='[grid] Number of columns in the grid')

class MultiImageDisplay(gridlib.Grid):
   def __init__(self, parent=None, id=-1):
      gridlib.Grid.__init__(self, parent, id,
                         style=wx.NO_FULL_REPAINT_ON_RESIZE|wx.CLIP_CHILDREN)
      self.renderer = MultiImageGridRenderer(self)
      self.SetDefaultRenderer(self.renderer)
      if wx.VERSION >= (2, 5):
         self.SetGridLineColour(wx.Color(130,130,254))

      self.glyphs = util.CallbackSet()
      self.sorted_glyphs = []
      self.rows = 1
      self.cols = config.get("grid_ncols")
      self.max_cell_width = config.get("grid_max_cell_width")
      self.max_cell_height = config.get("grid_max_cell_height")
      self.max_label_length = config.get("grid_max_label_length")
      self.cell_padding = config.get("grid_cell_padding")
      self.frame = parent
      self.updating = False
      self.sort_function = ""
      self.sort_order = 0
      self.display_attribute = ""
      self.display_names = False
      self.created = False
      self.do_updates = False
      self.last_tooltip = ""
      self.scaling = 1.0
      if wx.Platform == '__WXMAC__':
         tooltip_size = wx.Size(300, 24)
      else:
         tooltip_size = wx.Size(175, 32)
      self.tooltip = wx.Button(self.GetGridWindow(), -1, "",
                               wx.Point(0, 0), tooltip_size)
      self.tooltip.Show(False)
      gridlib.EVT_GRID_CELL_LEFT_DCLICK(self, self._OnLeftDoubleClick)
      gridlib.EVT_GRID_CELL_RIGHT_CLICK(self, self._OnRightClick)
      gridlib.EVT_GRID_SELECT_CELL(self, self._OnSelect)
      gridlib.EVT_GRID_CELL_CHANGE(self, self._OnSelect)
      wx.EVT_MOTION(self.GetGridWindow(), self._OnMotion)
      wx.EVT_LEAVE_WINDOW(self.GetGridWindow(), self._OnLeave)
      # if wx.VERSION <= (2, 5) or wx.Platform == "__WXMSW__":
      #   wx.EVT_SIZE(self, self._OnSize)

##    def _OnSize(self, evt):
##       self.ForceRefresh()

   # This is to get around a bug in wxPython 2.6.x
   def AutoSize(self):
      size = self.GetSize()
      gridlib.Grid.AutoSize(self)
      self.SetSize(size)

   def get_is_dirty(self):
      return self.glyphs.is_dirty and len(self.glyphs)
   def set_is_dirty(self, value):
      self.glyphs.is_dirty = value
   is_dirty = property(get_is_dirty, set_is_dirty)

   if wx.VERSION >= (2,5):
      def get_renderer(self):
         return self.renderer.Clone()
   else:
      def get_renderer(self):
         return self.GetDefaultRenderer()

   ########################################
   # BASIC UTILITY
   
   def set_image(self, list, view_function=None):
      wx.BeginBusyCursor()
      self.BeginBatch()
      try:
         self.glyphs.clear()
         self.glyphs.update(list)
         self.do_updates = False
         self.sort_images()
         self.frame.set_choices()
         if not self.created:
            self.rows = 1
            self.CreateGrid(1, self.cols)
            for col in range(self.cols):
               self.SetCellRenderer(0, col, self.get_renderer())
               self.SetReadOnly(0, col, True)
            self.created = True
         self.EnableEditing(0)
         self.resize_grid()
         self.ClearSelection()
         x = self.GetSize()
         self.do_updates = True
      finally:
         self.EndBatch()
         wx.EndBusyCursor()
      return (x.x, 600)

   def resize_grid(self, do_auto_size=True):
      if not self.created:
         return
      wx.BeginBusyCursor()
      self.BeginBatch()
      try:
         start_row = self.GetGridCursorRow()
         start_col = self.GetGridCursorCol()
         orig_rows = self.rows
         rows = int(max(ceil(float(len(self.sorted_glyphs)) / float(self.cols)), 1))
         cols = self.cols
         if rows < orig_rows:
            self.DeleteRows(0, orig_rows - rows)
         elif rows > orig_rows:
            self.AppendRows(rows - orig_rows)
            for row in xrange(rows - 1, -1, -1):
               for col in xrange(cols):
                  self.SetCellRenderer(row, col, self.get_renderer())
                  self.SetReadOnly(row, col, True)
         self.rows = rows
         self.cols = cols
         row_size = 1
         
         width = self.set_labels()
         self.SetRowLabelSize(width + 20)
         self.SetColLabelSize(20)
         if do_auto_size:
            self.AutoSize()
         if start_row < self.GetNumberRows() and start_col < self.GetNumberCols():
            # self.SetGridCursor(start_row, start_col)
            self.MakeCellVisible(start_row, start_col)
      finally:
         self.EndBatch()
         wx.EndBusyCursor()

   def append_glyphs(self, glyphs, resize=True):
      wx.BeginBusyCursor()
      self.BeginBatch()
      try:
         i = None
         for i in range(len(self.sorted_glyphs) - 1, -1, -1):
            if not self.sorted_glyphs[i] is None:
               break
         if not i is None:
            del self.sorted_glyphs[i+1:]
         added = False
         for g in glyphs:
            if not g in self.glyphs:
               self.glyphs.add(g)
               self.sorted_glyphs.append(g)
               added = True
         if added and resize:
            self.resize_grid(False)
      finally:
         self.EndBatch()
         wx.EndBusyCursor()

   def remove_glyphs(self, list, resize=True):
      for glyph in list:
         if glyph in self.glyphs:
            glyph.dead = True
            self.glyphs.remove(glyph)
      if resize:
         self.resize_grid(False)

   def append_and_remove_glyphs(self, add, remove):
      if len(add):
         self.append_glyphs(add, False)
      if len(remove):
         self.remove_glyphs(remove, False)
      self.resize_grid(False)
      
   def scale(self, scaling):
      if self.scaling != scaling:
         original_scaling = self.scaling
         self.scaling = scaling
         view_start = self.GetViewStart()
         units = self.GetScrollPixelsPerUnit()
         original_col = self.XToCol(view_start[0] * units[0])
         if original_col < 0:
            original_col = self.GetNumberCols() - 1
         original_row = self.YToRow(view_start[1] * units[1])
         if original_row < 0:
            original_row = self.GetNumberRows() - 1
         self.AutoSize()
         rect = self.CellToRect(original_row, original_col)
         if rect.x != -1 and rect.y != -1:
            self.Scroll(int(rect.x / units[0]), int(rect.y / units[1]))
         
   def get_glyphs(self):
      return list(self.glyphs)

   ########################################
   # SORTING

   def _sort_by_name_func(self, a, b):
      if a.id_name == [] and b.id_name == []:
         r = 0
      elif a.id_name == []:
         r = 1
      elif b.id_name == []:
         r = -1
      else:
         r = cmp(a.get_main_id(), b.get_main_id())
      if r == 0:
         r = cmp(b.classification_state, a.classification_state)
         if r == 0 and a.classification_state != UNCLASSIFIED:
            r = cmp(b.id_name[0][0], a.id_name[0][0])
      return r

   def _split_classified_from_unclassified(self, list):
      # Find split between classified and unclassified
      if not len(list):
         return [], []
      for i in range(len(list)):
         if list[i].classification_state == UNCLASSIFIED:
            break
      return list[:i], list[i:]

   def _insert_for_line_breaks(self, list):
      # Make sure each new label begins in a new row
      column = 0
      prev_id = -1
      new_list = []
      for image in list:
         main_id = image.get_main_id()
         if main_id != prev_id and column != 0:
            new_list.extend([None] * (self.cols - column))
            column = 0
         new_list.append(image)
         column += 1
         column %= self.cols
         prev_id = main_id
      return new_list

   # To minimize the size of the grid, we sort the images
   # first by height, and then within each row by width
   def _sort_by_size(self, list):
      list.sort(_sort_by_nrows)
      outlist = []
      while len(list):
         if len(list) < self.cols:
            sublist = list; list = []
         else:
            sublist = list[:self.cols]; list = list[self.cols:]
         sublist.sort(_sort_by_ncols)
         outlist.extend(sublist)
      return outlist

   def default_sort(self, list):
      # If we've done no classification, use the default sort from
      # MultiImageDisplay
      self.last_sort = "default"
      # mark that we want to display row labels
      self.display_row_labels = 1
      # Sort by label
      list.sort(self._sort_by_name_func)
      # Find split between classified and unclassified
      classified, unclassified = self._split_classified_from_unclassified(list)
      # Sort the unclassified by size
      unclassified = self._sort_by_size(unclassified)
      # Merge back together
      list = classified + unclassified
      # Make sure each new label begins in a new row
      new_list = self._insert_for_line_breaks(list)
      return new_list

   # Sorts the list of images by a given function, or the
   # default function if None is given
   def sort_images(self, function=None, order=0):
      wx.BeginBusyCursor()
      self.BeginBatch()
      self.last_sort = None
      self.display_row_labels = not function
      try:
         orig_len = len(self.sorted_glyphs)
         if function != None:
            self.sort_function = function
            self.sort_order = order

         if self.sort_function == '':
            self.sorted_glyphs = self.default_sort(list(self.glyphs))
         else:
            sort_string = self.sort_function
            error_messages = {}
            # self.list = self.GetAllItems()
            for image in self.glyphs:
               try:
                  image.sort_cache = eval("x." + sort_string, {'x': image})
               except Exception, e:
                  error_messages[str(e)] = None
                  image.sort_cache = None
            if len(error_messages):
               message = '\n\n'.join(error_messages.keys())
               gui_util.message(message)
               for item in self.glyphs:
                  del item.sort_cache
               return
            self.sorted_glyphs = list(self.glyphs)
            self.sorted_glyphs.sort(util.fast_cmp)
            for item in self.glyphs:
               del item.sort_cache
         if self.sort_order:
            self.sorted_glyphs.reverse()
         self.resize_grid()
         self.ClearSelection()
         self.MakeCellVisible(0, 0)
      finally:
         self.EndBatch()
         wx.EndBusyCursor()

   def set_labels(self):
      self.BeginBatch()
      max_label = 1
      for i in range(self.cols):
         self.SetColLabelValue(i, "")
      dc = wx.ClientDC(self)
      for i in range(self.rows):
         try:
            image = self.sorted_glyphs[i * self.cols]
         except IndexError:
            self.SetRowLabelValue(i, "")
         else:
            if image == None or image.classification_state == UNCLASSIFIED:
               self.SetRowLabelValue(i, "")
            elif self.display_row_labels:
               label = self.get_label(image)
               label = self.reduce_label_length(
                  dc, self.max_label_length * 0.6, label)
               max_label = max(dc.GetTextExtent(label)[0], max_label)
               self.SetRowLabelValue(i, label)
            else:
               max_label = max(dc.GetTextExtent("")[0], max_label)
               self.SetRowLabelValue(i, "")
      self.EndBatch()
      return min(max_label, self.max_label_length)

   ########################################
   # SELECTING

   def select_images(self, function):
      wx.BeginBusyCursor()
      self.BeginBatch()
      self.updating = True
      try:
         self.ClearSelection()
         for i in range(len(self.sorted_glyphs)):
            x = self.sorted_glyphs[i]
            if x != None:
               try:
                  result = function(x)
               except Exception, err:
                  gui_util.message(str(err))
                  return
               if result:
                  row = i / self.cols
                  col = i % self.cols
                  self.SelectBlock(
                     row, col, row, col, True)
         # self.OnSelectImpl()
      finally:
         self.updating = False
         self.EndBatch()
         wx.EndBusyCursor()

   ########################################
   # UTILITY FUNCTIONS

   def get_image_no(self, row, col):
      no =  row * self.cols + col
      if no >= len(self.sorted_glyphs):
         return None
      else:
         return no

   def GetAllItems(self):
      return self.glyphs

   def GetSelectedCoords(self, row=None, col=None):
      if self.IsSelection():
         # This is really dumb.  wxGrid has three ways to select things,
         # so we have to iterate through using all of them.
         coords = []
         for ul, lr in zip(self.GetSelectionBlockTopLeft(),
                           self.GetSelectionBlockBottomRight()):
            for r in range(ul[0], lr[0] + 1):
               for c in range(ul[1], lr[1] + 1):
                  yield (r, c)
         for r, c in self.GetSelectedCells():
            yield (r, c)
         for r in self.GetSelectedRows():
            for c in range(self.GetNumberCols()):
               yield (r, c)
         for c in self.GetSelectedCols():
            for r in range(self.GetNumberRows()):
               yield (r, c)
      elif not row is None:
         yield (row, col)


   def GetSelectedValidImages(self, row=None, col=None):
      for r, c in self.GetSelectedCoords(row, col):
         no =  r * self.cols + c
         if no < len(self.sorted_glyphs):
            image = self.sorted_glyphs[no]
            if not image is None:
               yield r, c, no, image
      return

   def GetSelectedIndices(self, row=None, col=None):
      for r, c, index, image in self.GetSelectedValidImages(row, col):
         yield index

   def GetSelectedItems(self, row=None, col=None):
      # This is not a generator for backward compatibility for callers
      images = []
      for r, c, index, image in self.GetSelectedValidImages(row, col):
         images.append(image)
      return images
      
   def RefreshSelected(self, row=None, col=None):
      for r, c, index, image in self.GetSelectedValidImages(row, col):
         self.SetCellValue(r, c, "")

   def SelectGlyphs(self, glyphs):
      matches = []
      for g in glyphs:
         for i, g2 in enumerate(self.sorted_glyphs):
            if g is g2:
               matches.append(i)
      if len(matches):
         self.BeginBatch()
         first = 0
         self.updating = True
         last_index = matches[-1]
         for index in matches:
            row = index / self.cols
            col = index % self.cols
            if index is last_index:
               self.updating = False
            self.SelectBlock(row, col, row, col, first)
            if first == 0:
               self.MakeCellVisible(row, col)
               first = 1
         self.updating = False
         self.EndBatch()

   ########################################
   # CALLBACKS

   def ZoomOut(self):
      if self.scaling > pow(2, -4):
         self.scale(self.scaling * 0.5)

   def ZoomNorm(self):
      self.scale(1.0)

   def ZoomIn(self):
      if self.scaling < pow(2, 4):
         self.scale(self.scaling * 2.0)
   
   def _OnSelectImpl(self):
      pass

   def _OnSelect(self, event):
      event.Skip()
      # self.ForceRefresh()
      self._OnSelectImpl()

   def _OnRightClick(self, event):
      row = event.GetRow()
      col = event.GetCol()
      images = self.GetSelectedItems(row, col)
      if images != None:
         position = event.GetPosition()
         image_menu.ImageMenu(self, position.x, position.y,
                              images, mode=0)
         # self.ForceRefresh()

   def _OnLeftDoubleClick(self, event):
      bitmap_no = self.get_image_no(event.GetRow(), event.GetCol())
      if bitmap_no != None and self.sorted_glyphs[bitmap_no] != None:
         self.sorted_glyphs[bitmap_no].display()

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
      if extent[0] > width:
         for i in range(1, len(label)):
            new_label = "..." + label[-i:]
            extent = dc.GetTextExtent(new_label)
            if extent[0] > width:
               if i == 1:
                  new_label = "..."
               else:
                  new_label = "..." + label[-(i-1):]
               break
         return new_label
      else:
         return label

   if wx.Platform == "__WXMAC__":
      _tooltip_extra = 32
   else:
      _tooltip_extra = 12
   def set_tooltip(self, label):
      #self.tooltip.SetLabel(label.decode('utf8'))
      self.tooltip.SetLabel(label)
      dc = wx.ClientDC(self.tooltip)
      extent = dc.GetTextExtent(label)
	
      self.tooltip.SetDimensions(
         -1,-1,extent[0]+self._tooltip_extra,extent[1]+self._tooltip_extra,
         wx.SIZE_AUTO)

   def _OnMotion(self, event):
      origin = self.GetViewStart()
      units = self.GetScrollPixelsPerUnit()
      row = self.YToRow(event.GetY() + origin[1] * units[1])
      col = self.XToCol(event.GetX() + origin[0] * units[0])
      image_no = self.get_image_no(row, col)
      if image_no == None or image_no >= len(self.sorted_glyphs) or image_no < 0:
         image = None
      else:
         image = self.sorted_glyphs[image_no]

      if image == None:
         if self.last_tooltip != "":
            self.tooltip.Show(False)
            self.last_tooltip = ""
      else:
         message = self.get_label(image)
         if message != self.last_tooltip:
            self.tooltip.Show(True)
            self.set_tooltip(message)
            self.last_tooltip = message
         self.tooltip.Move(wx.Point(event.GetX() + 16, event.GetY() + 16))
      event.Skip()

   def display_label_at_cell(self, row, col, label):
      rect = self.CellToRect(row, col)
      origin = self.GetViewStart()
      units = self.GetScrollPixelsPerUnit()
      self.tooltip.Show()
      self.set_tooltip(label)
      self.tooltip.Move(wx.Point(rect.GetLeft() - origin[0] * units[0],
                                rect.GetBottom() - origin[1] * units[1] + 1))
 
   def _OnLeave(self, event):
      x, y = event.GetX(), event.GetY()
      w, h = self.GetSizeTuple()
      if x < 0 or x > w or y < 0 or y > h:
         self.tooltip.Hide()
      event.Skip()

class MultiImageWindow(wx.Panel):
   def __init__(self, parent = None, id = -1, title = "Gamera", owner=None):
      from gamera.gui import gamera_icons
      wx.Panel.__init__(self, parent, id,
                       style=wx.NO_FULL_REPAINT_ON_RESIZE|wx.CLIP_CHILDREN)
      self.SetAutoLayout(True)
      self.toolbar = toolbar.ToolBar(self, -1)

      self.toolbar.AddSimpleTool(
         10, gamera_icons.getIconRefreshBitmap(),
         "Refresh", self._OnRefreshClick)
      self.toolbar.AddSeparator()
      self.toolbar.AddSimpleTool(
         20, gamera_icons.getIconZoomInBitmap(),
         "Zoom in", self._OnZoomInClick)
      self.toolbar.AddSimpleTool(
         21, gamera_icons.getIconZoomNormBitmap(),
         "Zoom 100%", self._OnZoomNormClick)
      self.toolbar.AddSimpleTool(
         22, gamera_icons.getIconZoomOutBitmap(),
         "Zoom out", self._OnZoomOutClick)
      self.toolbar.AddSeparator()
      self.display_text_combo = wx.ComboBox(self.toolbar, 50, choices=[],
                                     size = wx.Size(200, 20))
      wx.EVT_COMBOBOX(self.display_text_combo, 50, self._OnChangeDisplayText)
      self.toolbar.AddControl(self.display_text_combo)
      self.toolbar.AddSimpleTool(
         24, gamera_icons.getIconShowNameBitmap(),
         "Display classes on grid", self._OnDisplayClasses, 1)
      self.toolbar2 = toolbar.ToolBar(self, -1)
      self.sort_combo = wx.ComboBox(self.toolbar2, 100, choices=[],
                                   size=wx.Size(200, 20))
      self.toolbar2.AddControl(self.sort_combo)
      self.toolbar2.AddSimpleTool(
         101, gamera_icons.getIconSortAscBitmap(),
         "Sort Ascending", self._OnSortAscending)
      self.toolbar2.AddSimpleTool(
         102, gamera_icons.getIconSortDecBitmap(),
         "Sort Descending", self._OnSortDescending)
      self.toolbar2.AddSeparator()
      self.select_combo = wx.ComboBox(self.toolbar2, 103, choices=[],
                                     size=wx.Size(200, 20))
      self.toolbar2.AddControl(self.select_combo)
      self.toolbar2.AddSimpleTool(
         104, gamera_icons.getIconSelectBitmap(),
         "Select by given expression", self._OnSelect)
      self.toolbar2.AddSimpleTool(
         105, gamera_icons.getIconSelectAllBitmap(),
         "Select All", self._OnSelectAll)
      self.select_choices = []
      self.sort_choices = []

      box = wx.BoxSizer(wx.VERTICAL)
      box.Add(self.toolbar, 0, wx.EXPAND)
      box.Add(self.toolbar2, 0, wx.EXPAND)
      self.id = self.get_display()
      box.Add(self.id, 1, wx.EXPAND)
      self.SetSizer(box)
      self.Fit()
      self.box_sizer = box

   # This can be overridden to change the internal display class
   def get_display(self):
      return MultiImageDisplay(self)

   def set_choices(self):
      methods = [x[0] + "()" for x in
                 plugin.methods_flat_category("Features", ONEBIT)]
      methods.sort()

      self.display_choices = [
         "x.get_main_id()",
         "(x.offset_y, x.offset_x, x.nrows, x.ncols)",
         "x.label", "x.properties"]
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
         "", "x.classification_state == 0",
         "x.classification_state == 1",
         "x.classification_state == 2",
         "x.classification_state == 3",
         "x.nrows < 2 or x.ncols < 2"]
      self.select_combo.Clear()
      for choice in self.select_choices:
         self.select_combo.Append(choice)

   def close(self):
      self.Destroy()

   ########################################
   # CALLBACKS

   def _OnRefreshClick(self, event):
      self.id.AutoSize()
      self.id.ForceRefresh()

   def _OnSortAscending(self, event, order=0):
      sort_string = string.strip(self.sort_combo.GetValue())
      if sort_string not in self.sort_choices:
         self.sort_choices.append(sort_string)
         self.sort_combo.Append(sort_string)
      self.id.sort_images(sort_string, order)

   def _OnSortDescending(self, event):
      self._OnSortAscending(event, 1)

   def _OnSelect(self, event):
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
      self.id.select_images(select_func)

   def _OnSelectAll(self, event):
      wx.BeginBusyCursor()
      try:
         self.id.SelectAll()
      finally:
         wx.EndBusyCursor()

   def _OnSelectInvert(self, event):
      self.id.SelectInvert()

   def _OnZoomInClick(self, event):
      self.id.ZoomIn()

   def _OnZoomNormClick(self, event):
      self.id.ZoomNorm()

   def _OnZoomOutClick(self, event):
      self.id.ZoomOut()

   def _OnChangeDisplayText(self, event):
      value = self.display_text_combo.GetValue()
      self.id.display_attribute = value
      if value not in self.display_choices:
         self.display_choices.append(value)
         self.display_text_combo.Append(value)
      if self.id.display_names:
         self.id.ForceRefresh()

   def _OnDisplayDetails(self, event):
      self.id.display_details = event.GetIsDown()
      if self.id.display_names:
         self.id.Refresh()

   def _OnDisplayClasses(self, event):
      self.id.display_names = event.GetIsDown()
      self.id.Refresh()

##############################################################################
# TOP-LEVEL FRAMES
##############################################################################

class ImageFrameBase:
   def __init__(self, parent = None, id = -1, title = "Gamera", owner=None):
      self._frame = wx.Frame(has_gui.gui.TopLevel(), id, title,
                            wx.DefaultPosition, (600, 400),
                            style=wx.DEFAULT_FRAME_STYLE|wx.CLIP_CHILDREN|
                            wx.NO_FULL_REPAINT_ON_RESIZE)
      if (owner != None):
         self.owner = weakref.proxy(owner)
      else:
         self.owner = None
      wx.EVT_CLOSE(self._frame, self._OnCloseWindow)

   def add_callback(self, *args):
      self._iw.id.add_callback(*args)

   def remove_callback(self, *args):
      self._iw.id.remove_callback(*args)

   def trigger_callback(self, *args):
      self._iw.id.trigger_callback(*args)

   def set_image(self, image, view_function=None, weak=1):
      size = self._iw.id.set_image(image, view_function, weak)
      self._frame.SetSize((max(200, min(600, size[0] + 30)),
                           max(200, min(400, size[1] + 60))))

   def close(self):
      self._iw.Destroy()
      self._frame.Destroy()

   def refresh(self):
      self._iw.id.refresh(0)

   def _OnCloseWindow(self, event):
      del self._iw
      if self.owner:
         self.owner.set_display(None)
      self._frame.Destroy()
      del self._frame

   def Show(self, flag=1):
      self._frame.Show(flag)
   show = Show

class ImageFrame(ImageFrameBase):
   def __init__(self, parent = None, id = -1, title = "Gamera", owner=None):
      ImageFrameBase.__init__(self, parent, id, title, owner)
      self._iw = ImageWindow(self._frame)
      from gamera.gui import gamera_icons
      icon = wx.IconFromBitmap(gamera_icons.getIconImageBitmap())
      self._frame.SetIcon(icon)
      self._frame.CreateStatusBar(2)
      self._status_bar = self._frame.GetStatusBar()
      self.add_callback("move", self._OnMove)
      self.add_callback("rubber", self._OnRubber)

   def __repr__(self):
      return "<ImageFrame Window>"

   def add_highlight_cc(self, cc, color=None):
      self._iw.id.add_highlight_cc(cc, color)
   add_highlight_ccs = add_highlight_cc

   def highlight_rectangle(self, *args):
      self._iw.id.highlight_rectangle(*args)

   def add_box(self, y, x, h, w):
      self._iw.id.add_box(y, x, h, w)

   def clear_all_boxes(self):
      self._iw.id.clear_all_boxes()

   def highlight_cc(self, cc):
      self._iw.id.highlight_cc(cc)
   highlight_ccs = highlight_cc

   def unhighlight_cc(self, cc):
      self._iw.id.unhighlight_cc(cc)
   unhighlight_ccs = unhighlight_cc

   def clear_all_highlights(self):
      self._iw.id.clear_all_highlights()

   def focus_glyphs(self, glyphs):
      self._iw.id.focus_glyphs(glyphs)

   def _OnMove(self, y, x):
      image = self._iw.id.original_image
      self._status_bar.SetStatusText(
         "(%d, %d): %s" % (x, y, image.get((x - image.ul_x, y - image.ul_y))), 0)

   def _OnRubber(self, y1, x1, y2, x2, shift, ctrl):
      image = self._iw.id.original_image
      if y1 == y2 and x1 == x2:
         self._status_bar.SetStatusText(
            "(%d, %d): %s" % (x1, y1, image.get((x2 - image.ul_x, y2 - image.ul_y))), 1)
      else:
         self._status_bar.SetStatusText(
            "(%d, %d) to (%d, %d) / (%d w, %d h) %s" %
            (x1, y1, x2, y2, abs(x1-x2), abs(y1-y2), image.get((x2 - image.ul_x, y2 - image.ul_y))), 1)

class MultiImageFrame(ImageFrameBase):
   def __init__(self, parent = None, id = -1, title = "Gamera", owner=None):
      ImageFrameBase.__init__(self, parent, id, title, owner)
      self._iw = MultiImageWindow(self._frame)
      from gamera.gui import gamera_icons
      icon = wx.IconFromBitmap(gamera_icons.getIconImageListBitmap())
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
   max_data = max(data)
   if max_data == 0:
      scale_y = 1.0
   else:
      scale_y = (y2 - y1) / max(data)
   m = log(max_data)
   dc.SetPen(wx.TRANSPARENT_PEN)
   light_blue = wx.Color(128, 128, 255)
   for i in range(len(data)):
      datum = data[i] * scale_y
      dc.SetBrush(wx.WHITE_BRUSH)
      dc.DrawRectangle(x1 + i * scale_x, y1,
                       scale_x + 1, y2 - y1)
      dc.SetBrush(wx.Brush(light_blue, wx.SOLID))
      dc.DrawRectangle(x1 + i * scale_x, y2 - datum,
                       scale_x + 1, datum)
   if mark:
      dc.SetBrush(wx.CYAN_BRUSH)
      dc.SetLogicalFunction(wx.XOR)
      dc.DrawRectangle(x1 + mark * scale_x, y1,
                       scale_x + 1, y2 - y1)
      dc.SetLogicalFunction(wx.COPY)
   if border:
      dc.SetPen(wx.BLACK_PEN)
      dc.SetBrush(wx.TRANSPARENT_BRUSH)
      dc.DrawRectangle(x1 - 1, y1 - 1, x2 - x1 + 2, y2 - y1 + 1)

# Draws a vertical bar graph
def graph_vert(data, dc, x1, y1, x2, y2, mark=None, border=1):
   scale_y = float(y2 - y1) / float(len(data))
   max_data = max(data)
   if max_data == 0:
      scale_x = 1.0
   else:
      scale_x = (x2 - x1) / max_data
   light_blue = wx.Color(128, 128, 255)
   dc.SetPen(wx.TRANSPARENT_PEN)
   for i in range(len(data)):
      datum = data[i] * scale_x
      dc.SetBrush(wx.WHITE_BRUSH)
      dc.DrawRectangle(x1, y1 + i * scale_y, 
                       x2 - x1, scale_y + 1)
      dc.SetBrush(wx.Brush(light_blue, wx.SOLID))
      dc.DrawRectangle(x1, y1 + i * scale_y,
                       datum, scale_y + 1)
   if mark:
      dc.SetBrush(wx.CYAN_BRUSH)
      dc.SetLogicalFunction(wx.XOR)
      dc.DrawRectangle(x1, y1 + mark * scale_y, x2 - x1, scale_y + 1)
      dc.SetLogicalFunction(wx.COPY)
   if border:
      dc.SetPen(wx.BLACK_PEN)
      dc.SetBrush(wx.TRANSPARENT_BRUSH)
      dc.DrawRectangle(x1 - 1, y1 - 1, x2 + 1 - x1, y2 - y1)

# Draws a grey-scale scale
def graph_scale(dc, x1, y1, x2, y2):
   scale_x = float(x2 - x1) / float(255)
   dc.SetPen(wx.TRANSPARENT_PEN)
   for i in range(255):
      dc.SetBrush(wx.Brush(wx.Color(i, i, i), wx.SOLID))
      dc.DrawRectangle(x1 + i * scale_x, y1,
                       scale_x + 1, y2 - y1)
   dc.SetPen(wx.BLACK_PEN)
   dc.SetBrush(wx.TRANSPARENT_BRUSH)
   dc.DrawRectangle(x1 - 1, y1 - 1, x2 - x1 + 2, y2 - y1 + 1)

# Clears an area of a wxDC
def clear_dc(dc):
   width = dc.GetSize().x
   height = dc.GetSize().y
   dc.SetBackgroundMode(wx.SOLID)
   dc.SetBrush(wx.Brush(wx.WHITE, wx.SOLID))
   dc.SetPen(wx.TRANSPARENT_PEN)
   dc.DrawRectangle(0, 0, width, height)

HISTOGRAM_PAD = 10
class HistogramDisplay(wx.Frame):
   def __init__(self, data=None, mark=None, parent=None,
                title="Histogram"):
      wx.Frame.__init__(self, parent, -1, title,
                       style=wx.RESIZE_BORDER|wx.CAPTION)
      m = max(data)
      new_data = []
      for datum in data:
         if datum != 0:
            new_data.append(sqrt(datum))
      self.data = new_data
      self.mark = mark
      wx.EVT_PAINT(self, self._OnPaint)

   def _OnPaint(self, event):
      dc = wx.PaintDC(self)
      width = dc.GetSize().x
      height = dc.GetSize().y
      clear_dc(dc)
      graph_horiz(self.data, dc, HISTOGRAM_PAD, HISTOGRAM_PAD,
                  width - HISTOGRAM_PAD, height - HISTOGRAM_PAD * 1.5,
                  mark = self.mark)
      graph_scale(dc, HISTOGRAM_PAD, height - HISTOGRAM_PAD * 1.5,
                  width - HISTOGRAM_PAD, height - HISTOGRAM_PAD)


class ProjectionsDisplay(wx.Frame):
   def __init__(self, x_data=None, y_data=None, image=None, parent=None, title="Projections"):
      size = (image.ncols + max(x_data) + (HISTOGRAM_PAD * 3),
              image.nrows + max(y_data) + (HISTOGRAM_PAD * 3))
      wx.Frame.__init__(self, parent, -1, title,
                       style=wx.CAPTION,
                       size=size)
      self.x_data = x_data
      self.y_data = y_data
      self.image = image
      wx.EVT_PAINT(self, self._OnPaint)

   def _OnPaint(self, event):
      dc = wx.PaintDC(self)
      clear_dc(dc)
      dc_width = dc.GetSize().x
      dc_height = dc.GetSize().y
      mat_width = self.image.ncols
      mat_height = self.image.nrows
      image = wx.EmptyImage(self.image.ncols, self.image.nrows)
      self.image.to_buffer(image.GetDataBuffer())
      bmp = image.ConvertToBitmap()
      # Display centered within the cell
      x = HISTOGRAM_PAD * 2 + max(self.x_data)
      y = HISTOGRAM_PAD
      dc.DrawBitmap(bmp, x, y, 0)
      graph_vert(self.x_data, dc, HISTOGRAM_PAD, y,
                 HISTOGRAM_PAD + max(self.x_data), y + mat_height, border=0)
      graph_horiz(self.y_data, dc, x, y + mat_height + HISTOGRAM_PAD,
                  x + mat_width,
                  y + (mat_height + max(self.y_data)) + HISTOGRAM_PAD, border=0)

class ProjectionDisplay(wx.Frame):
   def __init__(self, data, title="Projection"):
      wx.Frame.__init__(self, -1, -1, title,
                       style=wx.CAPTION,
                       size=((len(data) * 2) + (HISTOGRAM_PAD * 3),
                             max(data) + (HISTOGRAM_PAD * 3)))
      self.data = data
      wx.EVT_PAINT(self, self._OnPaint)

   def _OnPaint(self, event):
      dc = wx.PaintDC(self)
      clear_dc(dc)
      dc_width = dc.GetSize().x
      dc_height = dc.GetSize().y
      # Display centered within the cell
      x = (dc_width / 2) + (HISTOGRAM_PAD / 2)
      y = HISTOGRAM_PAD
      dc.DrawBitmap(bmp, x, y, 0)
      graph_horiz(self.data, dc, x, y + HISTOGRAM_PAD,
                  x + mat_width, y + HISTOGRAM_PAD, border=0)

##############################################################################
# PRINTING
##############################################################################

if wx.PostScriptDC_GetResolution() < 150:
   wx.PostScriptDC_SetResolution(600)

class GameraPrintout(wx.Printout):
   def __init__(self, image, margin = 1.0):
      wx.Printout.__init__(self, title=image.name)
      self.margin = margin
      self.image = image

   def HasPage(self, page):
      #current only supports 1 page print
      return page == 1

   def GetPageInfo(self):
      return (1, 1, 1, 1)

   def OnPrintPage(self, page):
      dc = self.GetDC()
      (ppw,pph) = self.GetPPIPrinter()      # printer's pixels per in
      (pgw,pgh) = self.GetPageSizePixels()  # page size in pixels
      (dcw,dch) = dc.GetSize()

      (mx,my) = ppw * self.margin, pph * self.margin
      (vw,vh) = pgw - mx * 2, pgh - my * 2
      scale = min([float(vw) / float(self.image.width), float(vh) / float(self.image.height)])

      from sys import stderr
      print >>stderr, "Printing at (%d, %d) resolution" % (ppw, pph)

      if self.image.pixel_type_name == "OneBit":
         self.image = self.image.to_greyscale()
      resized = self.image.scale(scale, 2)
      image = wx.EmptyImage(resized.ncols, resized.nrows)
      resized.to_buffer(image.GetDataBuffer())
      bmp = wx.BitmapFromImage(image)
      tmpdc = wx.MemoryDC()
      tmpdc.SelectObject(bmp)
      dc.Blit(int(mx), int(my), resized.ncols, resized.nrows,
              tmpdc, 0, 0, wx.COPY, True)
      return True
