from gamera import core, config
from gamera.gui import gamera_display, gui_util
from wxPython.wx import *
from wxPython.lib.mixins.listctrl import wxListCtrlAutoWidthMixin
import glob, string, os.path

class FileList(wxListCtrl, wxListCtrlAutoWidthMixin):
    def __init__(self, parent, ID, image_display):
        wxListCtrl.__init__(self, parent, ID,
                            style=wxLC_REPORT | wxNO_FULL_REPAINT_ON_RESIZE)
        EVT_LIST_ITEM_SELECTED(self, -1, self.OnItemSelected)
        self.image_display = image_display
        self.open_dir("")

    def OnItemSelected(self, e):
        cur = e.m_itemIndex
        try:
            image = core.load_image(self.files[cur])
        except Exception, e:
            gui_util.message("Loading image %s failed. There error was:\n%s"
                             % (self.files[cur], str(e)))
            return
        self.image_display.id.set_image(image, weak=0)
        width, height = self.image_display.id.GetSize()
        scale = max(float(width) / float(image.width), (float(height) / float(image.height)))
        self.image_display.id.scale(scale)

    def open_dir(self, dir):
        self.files = glob.glob(dir + "/*.tif*")
        self.files.sort()
        self.set_files()
        
    def set_files(self):
        self.ClearAll()
        self.InsertColumn(0, "name")
        self.InsertColumn(1, "type")
        self.InsertColumn(2, "rows")
        self.InsertColumn(3, "cols")
        for i in range(len(self.files)):
            self.InsertStringItem(i, os.path.split(self.files[i])[-1])
            info = core.image_info(self.files[i])

            if (info.ncolors == 1):
                if (info.depth == 1):
                    self.SetStringItem(i, 1, "ONEBIT")
                elif (info.depth == 8):
                    self.SetStringItem(i, 1, "GREYSCALE")
                elif (info.depth == 16):
                    self.SetStringItem(i, 1, "GREY16")
                else:
                    self.SetStringItem(i, 1, "UNKNOWN")
            elif (info.ncolors == 3):
                self.SetStringItem(i, 1, "RGB")
            else:
                self.SetStringItem(i, 1, "UNKNOWN")
            self.SetStringItem(i, 2, str(info.nrows))
            self.SetStringItem(i, 3, str(info.ncols))

        self.SetColumnWidth(3, wxLIST_AUTOSIZE)
        self.SetColumnWidth(2, wxLIST_AUTOSIZE)
        self.SetColumnWidth(1, wxLIST_AUTOSIZE)
        self.SetColumnWidth(0, wxLIST_AUTOSIZE)
        

class ImageBrowserFrame(wxFrame):
    def __init__(self):
        wxFrame.__init__(self, NULL, -1, "Image File Browser",
                         wxDefaultPosition,(400, 500))
        self.splitter = wxSplitterWindow(self, -1)
        self.image = gamera_display.ImageWindow(self.splitter, -1)
        # The ImageWindow is not a happy camper without an image
        # so we make a small white image for it to play with and
        # be happy about.
        i = core.Image(0, 0, 10, 10, core.ONEBIT, core.DENSE)
        i.fill_white()
        self.image.id.set_image(i, weak=0)
        self.file = FileList(self.splitter, -1, self.image)
        self.splitter.SetMinimumPaneSize(20)
        self.splitter.SplitVertically(self.file, self.image)
        self.splitter.Show(1)
        self.image.id.RefreshAll()

        menubar = wxMenuBar()
        file = wxMenu()
        id = NewId()
        file.Append(id, "Open", "Open a directory of files")
        EVT_MENU(self, id, self.OnOpen)
        menubar.Append(file, "&File")
        self.SetMenuBar(menubar)


    def OnOpen(self, e):
        dir = gui_util.directory_dialog(self, 0)
        if dir:
            self.file.open_dir(dir)


