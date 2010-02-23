import py.test

from gamera.core import *
init_gamera()

#
# Tests for converting color image to labeld CCs
#

def test_color_to_ccs():

    # when more colors than labels: exception
    def _too_many_colors():
        img = load_image("data/too_many_colors.png")
        labeled = img.colors_to_labels()
    py.test.raises(Exception, _too_many_colors)

    # test labeling
    img = Image((0,0), (40,40), RGB)
    img.draw_filled_rect((3,3),(5,5),RGBPixel(255,0,0))
    img.draw_filled_rect((10,3),(15,35),RGBPixel(0,255,0))
    img.draw_filled_rect((5,20),(30,25),RGBPixel(0,0,255))
    labeled = img.colors_to_labels({\
                RGBPixel(255,0,0):6,\
                RGBPixel(0,255,0):2,\
                RGBPixel(0,0,255):7,\
              })
    assert 6 == labeled.get((4,4))
    assert 2 == labeled.get((12,18))
    assert 7 == labeled.get((12,22))

    # test converison to CCs
    ccs = labeled.ccs_from_labeled_image()
    for c in ccs:
        if c.label == 6:
            assert [(3,3),(5,5)] == [c.ul, c.lr]
        if c.label == 2:
            assert [(10,3),(15,35)] == [c.ul, c.lr]
        if c.label == 7:
            assert [(5,20),(30,25)] == [c.ul, c.lr]
