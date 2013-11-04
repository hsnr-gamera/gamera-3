import py.test

from gamera.core import *
init_gamera()

import features

eps = 0.0001

# testing feature_plugins

# area of bounding box, nrows * ncols, result floatvector[value]
# bounding box of whole pic, independent from content, trivial case
def test_area():
    
    #case 1
    img = Image((0,0), (8,8), ONEBIT)
    area_f = img.area()
    assert area_f[0] == 81.0
    
    #case 2
    img = Image((0,0), (3,5), ONEBIT)
    area_f = img.area()
    assert area_f[0] == 24.0
    
    #case 3
    img = Image((0,0), (3,16), ONEBIT)
    area_f = img.area()
    assert area_f[0] == 68.0
    
    
# aspect_ratio of bounding box, ncols/ nrows, result floatvector[value]
# bounding box of whole pic, independent from content, trivial case
def test_aspect_ratio():
    
    #case 1
    img = Image((0,0), (8,8), ONEBIT)
    ratio_f = img.aspect_ratio()
    assert ratio_f[0] == 1.0
    
    #case 2
    img = Image((0,0), (3,5), ONEBIT)
    ratio_f = img.aspect_ratio()
    assert abs(ratio_f[0] - 2/3.0) <= eps # must be 0, kinda redundant
    assert abs(ratio_f[0] - 0.6666666) <= eps
    
    #case 3
    img = Image((0,0), (3,16), ONEBIT)
    ratio_f = img.aspect_ratio()
    assert abs(ratio_f[0] - 0.235294118) <= eps
    

# black_area , returns number of black pixel, result floatvector[value]
def test_black_area():
    
    #case 1
    img = Image((0,0), (7,7), ONEBIT)
    img.draw_filled_rect((1,1),(3,5),1)
    black_area_f = img.black_area()
    assert black_area_f[0] == 15.0
    
    #case 2
    img.draw_line((0,3),(7,3),1)
    black_area_f = img.black_area()
    assert black_area_f[0] == 20.0
    
    #case 4
    img.set((7,7),1)
    black_area_f = img.black_area()
    assert black_area_f[0] == 21.0
    
    #case 5
    img.set((3,4),1)
    black_area_f = img.black_area()
    assert black_area_f[0] == 21.0
    
    #case 6
    # would theoretically alert, if interpolation algorithm changed,
    # because no trivial diagonals were drawn
    img = Image((0,0), (7,7), ONEBIT)
    img.draw_line((1,1),(3,5),1)
    img.draw_line((4,2),(1,6),1)
    black_area_f = img.black_area()
    assert black_area_f[0] == 10.0
    
    
# compactness, long lines low-, circle high-compactness, 
# result floatvector[value]
# size of pic has influence, i.e. 1 lonely px in 3x3 pic has different 
# compactness than 1px in 5x5 pic, empty pic has by definition max 
# compactness (1,79.. e+308), full pic has compactness=0
def test_compactness():
    
    #case 0 complete filled
    img = Image((0,0),(7,7), ONEBIT)
    img.draw_filled_rect((0,0),(7,7),1)
    comp_f = img.compactness()
    assert comp_f[0] == 0.5625
    
    #case 1 rect with white border
    img = Image((0,0), (7,7), ONEBIT)
    img.draw_filled_rect((1,1),(6,6),1)
    comp_f = img.compactness()
    assert abs(comp_f[0] - 0.777777777) <= eps
    
    #case 2 one single pixel
    img = Image((0,0), (7,7), ONEBIT)
    img.set((1,1),1)
    comp_f = img.compactness()
    assert comp_f[0] == 8.0
    
    #case 3 black border
    img = Image((0,0), (7,7), ONEBIT)
    img.draw_line((0,0),(7,0),1)
    img.draw_line((7,0),(7,7),1)
    img.draw_line((7,7),(0,7),1)
    img.draw_line((0,7),(0,0),1)
    comp_f = img.compactness()
    assert comp_f[0] == 2.0
    
    #case 4 circle
    img = Image((0,0), (7,7), ONEBIT)
    img.draw_circle((3,3),3,1)
    comp_f = img.compactness()
    assert abs(comp_f[0] - 2.647058823529) <= eps
    
    #case 5 horizontal line
    img = Image((0,0), (7,7), ONEBIT)
    img.draw_line((2,2),(6,2),1)
    comp_f = img.compactness()
    assert abs(comp_f[0] - 3.2) <= eps
    
    #case 6 diagonal line
    img = Image((0,0), (7,7), ONEBIT)
    img.draw_line((1,2),(2,7),1)
    comp_f = img.compactness()
    assert abs(comp_f[0] - 10/3.0) <= eps
    
   
# moments, result floatvector[X,Y,mom1,mom2,..,mom7]
# X,Y center of gravity normalized on 1.0
# position in relation to surrounding bounding box matters
def test_moments():
    
    #case 1
    img = Image((0,0), (8,8), ONEBIT)
    img.set((4,1),1)
    mom_f = img.moments()
    assert mom_f[0] == 0.5
    assert mom_f[1] == 0.125
    assert mom_f[2] == 0 # single 1px has no moments
    assert mom_f[8] == 0
    
    #case 1b central symmetric rect
    img = Image((0,0), (7,7), ONEBIT)
    img.draw_filled_rect((2,3),(5,4),1)
    mom_f = img.moments()
    assert mom_f[0] == 0.5
    assert mom_f[1] == 0.5
    assert mom_f[2] == 0.15625
    assert mom_f[3] == 0.03125
    assert mom_f[4] == 0 
    assert mom_f[5] == 0
    assert mom_f[6] == 0
    assert mom_f[7] == 0
    assert mom_f[8] == 0
    
    #case 2
    #like the Letter L on the left handside
    img = Image((0,0), (7,7), ONEBIT)
    img.draw_line((1,2),(1,5),1)
    img.draw_line((1,5),(3,5), 1) 
    mom_f = img.moments()
    assert abs(mom_f[0] - 0.21428571) <= eps
    assert abs(mom_f[1] - 0.57142857) <= eps
    assert abs(mom_f[2] - 0.09722222) <= eps
    assert abs(mom_f[3] - 0.22222222) <= eps
    assert abs(mom_f[4] - 0.08333333) <= eps
    assert abs(mom_f[5] - 0.03402069) <= eps
    assert abs(mom_f[6] + 0.01134023) <= eps
    assert abs(mom_f[7] - 0.02268046) <= eps
    assert abs(mom_f[8] + 0.06804138) <= eps 
    
    #case 3
    img = Image((0,0), (8,8), ONEBIT)
    img.draw_line((1,1),(5,4),1)
    img.draw_circle((5,4),2, 1) 
    mom_f = img.moments()
    assert abs(mom_f[0] - 0.5294117647058) <= eps
    assert abs(mom_f[1] - 0.41911764705882) <= eps
    assert abs(mom_f[2] - 0.142072053734) <= eps
    assert abs(mom_f[4] - 0.057398738041) <= eps
    assert abs(mom_f[7] + 0.0240267688551) <= eps # neg. moment
    
   
# ncols_feature, simply num of cols, result floatvector[value]
def test_ncols_feature():
    
    #case 1
    img = Image((0,0), (7,7), ONEBIT)
    ncols_f = img.ncols_feature()
    assert ncols_f[0] == 8.0
    
    #case 2
    img = Image((0,0), (3,8), ONEBIT)
    ncols_f = img.ncols_feature()
    assert ncols_f[0] == 4.0
    

# nholes, num of white runs (not border) avg, result floatvector[X, Y]
def test_nholes():
    
    #case 1
    img = Image((0,0), (7,7), ONEBIT)
    img.draw_filled_rect((1,1), (3,5), 1)
    nholes_f = img.nholes()
    assert nholes_f[0] == 0.0
    
    #case 2
    img = Image((0,0), (7,7), ONEBIT)
    img.draw_hollow_rect((1,1), (4,5), 1) 
    img.draw_hollow_rect((2,3), (6,7), 1) 
    img.draw_line((3,7),(5,7),0) #white line
    
    nholes_f = img.nholes()
    assert nholes_f[0] == 0.375 # 3 holes per 8 lines
    assert nholes_f[1] == 0.75 # 6 holes per 8 lines
    
    #case 2b // same pic double size to test scale-invariance
    img = img.scale(2.0,0)
    
    nholes_f = img.nholes()
    assert abs(nholes_f[0] - 0.375) <= eps 
    assert abs(nholes_f[1] - 0.75) <= eps
    
    
# nholes_extended, divides pic in 4 parts and does nholes analysis on each of it
# , result floatvector[X, Y]
def test_nholes_extended():
    
    #case 1
    img = Image((0,0), (7,7), ONEBIT)
    img.draw_filled_rect((1,1), (3,5), 1)
    nholes_ex_f = img.nholes_extended()
    assert nholes_ex_f[0] == 0.0
    
    #case 2
    img = Image((0,0), (7,7), ONEBIT)
    img.draw_hollow_rect((1,4), (3,6), 1)
    img.draw_hollow_rect((3,2), (5,5), 1)  
    img.draw_hollow_rect((1,0), (3,2), 1)      
    img.draw_line((1,0),(3,0),0) #white line
    img.set(Point(3,3),0) #white Point
    img.set(Point(7,5),1)
    nholes_ex_f = img.nholes_extended()
    #vertical
    assert nholes_ex_f[0] == 0.5    
    assert nholes_ex_f[1] == 1.5    
    assert nholes_ex_f[2] == 0.5  
    assert nholes_ex_f[3] == 0.0    
    #horizontal
    assert nholes_ex_f[4] == 0.5  
    assert nholes_ex_f[5] == 0.0    
    assert nholes_ex_f[6] == 1.5  
    assert nholes_ex_f[7] == 0.0  
     
    
# nrows_feature, simply num of rows, result floatvector[value]
def test_nrows_feature():
    
    #case 1
    img = Image((0,0), (7,7), ONEBIT)
    nrows_f = img.nrows_feature()
    assert nrows_f[0] == 8.0
    
    #case 2
    img = Image((0,0), (7,5), ONEBIT)
    nrows_f = img.nrows_feature()
    assert nrows_f[0] == 6.0   
    
    
# skeleton_features, thins out a structure to a skeleton and runs several tests
# on it, returns floatvector[numOf 4connected pxs ~ X, numOf 3connectd pxs ~ T,
# avgOf bendpoints, numOf endPoints, numOf x-axis crossing pxs, numOf y-axis 
# crossing pxs] 
# x- and y-axis through centre of masse
def test_skeleton_features():
    
    # capital Letter A with unsteadiness like hand-written
    img = Image((0,0), (49,49), ONEBIT)
    img.draw_filled_rect((3,41),(13,45),1) # left base of letter A
    img.draw_filled_rect((31,42),(42,46),1) # right base of letter A
    img.draw_line((5,40),(12,40),1)
    img.draw_filled_rect((6,38),(13,39),1)
    img.draw_line((7,37),(14,37),1)
    img.draw_line((8,36),(27,36),1)
    img.draw_filled_rect((9,34),(28,35),1)
    img.draw_filled_rect((10,32),(35,33),1)
    img.draw_line((11,31),(34,31),1)
    img.draw_line((11,30),(18,30),1)
    img.draw_line((12,29),(18,29),1)
    img.draw_line((12,28),(19,28),1)
    img.draw_line((13,27),(20,27),1)
    img.draw_filled_rect((14,25),(20,26),1)
    img.draw_line((15,24),(21,24),1)
    img.draw_line((16,23),(21,23),1)
    img.draw_filled_rect((16,21),(22,22),1)
    img.draw_filled_rect((17,19),(23,20),1)
    img.draw_filled_rect((18,17),(23,18),1)
    img.draw_filled_rect((19,13),(29,16),1)
    img.draw_filled_rect((20,10),(28,12),1)
    img.draw_filled_rect((21,8),(27,9),1)
    img.draw_line((21,7),(26,7),1)
    img.draw_filled_rect((22,5),(26,6),1)
    img.draw_filled_rect((23,16),(30,17),1)
    img.draw_filled_rect((25,18),(30,19),1)
    img.draw_line((25,20),(31,20),1)
    img.draw_filled_rect((26,21),(31,23),1)
    img.draw_filled_rect((27,24),(32,27),1)
    img.draw_filled_rect((28,27),(33,28),1)
    img.draw_line((27,29),(34,29),1)
    img.draw_line((24,30),(34,30),1)
    img.draw_line((29,34),(36,34),1)
    img.draw_line((30,35),(36,35),1)
    img.draw_line((31,36),(36,36),1)
    img.draw_line((31,37),(37,37),1)
    img.draw_line((32,38),(37,38),1)
    img.draw_filled_rect((32,39),(38,40),1)
    img.draw_line((33,41),(40,41),1)
    
    skel_f = img.skeleton_features()
    assert skel_f[0] == 2.0    # num of X-Joins (4 connected pxs)
    assert skel_f[1] == 10.0   # num of T-Joins (3 connected pxs) ?!?!?
    assert abs(skel_f[2] - 0.40659340) <= eps # avg of bend points
    assert skel_f[3] == 4.0    # num of endpoints
    assert skel_f[4] == 2.0    # num of x-crossings through center
    assert skel_f[5] == 2.0    # num og y-crossings through center
    

# top_bottom, simply detects the first and last row in which black Pixel appear,
# returns floatvector[firstrow/nrows, lastrow/nrows]
def test_top_bottom():
    
    #like the Letter L on the left handside
    img = Image((0,0), (7,7), ONEBIT)
    img.draw_line((1,2),(1,5),1)
    img.draw_line((1,5),(3,5), 1)    
    
    top_bottom_f = img.top_bottom()
    assert top_bottom_f[0] == 0.25
    assert top_bottom_f[1] == 0.625
 
    
# volume, numOf black pxs (black_area) / (ncols*nrows) (~bounding box), returns
# floatvector[value]
def test_volume():
    #like the Letter L on the left handside
    img = Image((0,0), (7,7), ONEBIT)
    img.draw_line((1,2),(1,5),1)
    img.draw_line((1,5),(3,5), 1)    
    
    vol_f = img.volume()
    assert vol_f[0] == 0.09375
  
    
# volume16regions, divides the pic in 16 regions and calculate the volume of each
# region seperatly (black_area/(region.cols*region.rows), returnsfloatvector[ 
# first column from top to bottom, 2nd column ... ] 
def test_volume_16_regions():
    
    img = Image((0,0),(7,7), ONEBIT)
    img.draw_hollow_rect((1,1),(6,4),1)
    img.draw_hollow_rect((2,4),(7,7),1)
    
    vol16_f = img.volume16regions()
    assert vol16_f[0] == 0.25 
    assert vol16_f[1] == 0.5
    assert vol16_f[2] == 0.25
    assert vol16_f[3] == 0
    assert vol16_f[4] == 0.5
    assert vol16_f[5] == 0
    assert vol16_f[6] == 0.75
    assert vol16_f[7] == 0.75
    assert vol16_f[8] == 0.5
    assert vol16_f[9] == 0
    assert vol16_f[10] == 0.5
    assert vol16_f[11] == 0.5
    assert vol16_f[12] == 0.25
    assert vol16_f[13] == 0.5
    assert vol16_f[14] == 0.75
    assert vol16_f[15] == 0.75

    img = Image((0,0),(0,0),ONEBIT)
    img.set((0,0),1)
    vol16_f = img.volume16regions()
    for i in range(15):
        assert vol16_f[i] == 1.0
    
    
# volume64regions, divides the pic in 64 regions and calculate the volume of each
# region seperatly (black_area/(region.cols*region.rows), returnsfloatvector[ 
# first column from top to bottom, 2nd column ... ] 
def test_volume_64_regions():
    
    img = Image((0,0),(15,15),ONEBIT)
    img.draw_hollow_rect((0,0),(15,15),1)
    img.draw_filled_rect((7,7),(8,8),1)
    
    vol64_f = img.volume64regions()
    assert vol64_f[0] == 0.75            #ul-region
    assert vol64_f[1] == 0.5             #1st col, 2nd row
    assert vol64_f[7] == 0.75            #ll-region
    assert vol64_f[8] == 0.5             #2nd column, 1st row
    assert vol64_f[9] == 0               #2nd column, 2nd row
    assert vol64_f[27] == 0.25           #ul-region from center
    assert vol64_f[63] == 0.75           #lr-region
    
    img = Image((0,0),(0,0),ONEBIT)
    img.set((0,0),1)
    vol64_f = img.volume64regions()
    for i in range(63):
        assert vol64_f[i] == 1.0
        
# zernike_moments, more complex feature extraction, the shape of the origin
# can be reconstructed outta the first few ZMs. The higher order moments 
# describe the shape even better up to the perfect copy for order n to 
# infinity.
# returnsfloatvector[magnitudes of NZM20, NZM22, ... , NZM66] 
        
def test_zernike_moments():
    
    #big capital-L
    img = Image((0,0),(50,50),ONEBIT)
    img.draw_line((5,5),(5,35),3)
    img.draw_line((3,35),(20,35),1)
    
    ZM_f0 = img.zernike_moments()
    assert abs(ZM_f0[0] -  0.4041497) <= eps
    assert abs(ZM_f0[1] -  0.2138501) <= eps
    assert abs(ZM_f0[2] -  0.2063463) <= eps
    assert abs(ZM_f0[3] -  0.1483378) <= eps
    assert abs(ZM_f0[4] -  0.1623111) <= eps
    assert abs(ZM_f0[5] -  0.3384352) <= eps
    assert abs(ZM_f0[6] -  0.1148943) <= eps
    assert abs(ZM_f0[7] -  0.2504674) <= eps
    assert abs(ZM_f0[8] -  0.0819277) <= eps
    assert abs(ZM_f0[9] -  0.1827993) <= eps
    assert abs(ZM_f0[10] - 0.0524176) <= eps
    assert abs(ZM_f0[11] - 0.4059857) <= eps
    assert abs(ZM_f0[12] - 0.0920007) <= eps
    assert abs(ZM_f0[13] - 0.1367013) <= eps

    # test rotation-invariance
    img = img.rotate(90.0, 1)
    ZM_f = img.zernike_moments()
    assert abs(ZM_f[0] -  ZM_f0[0]) <= eps
    assert abs(ZM_f[1] -  ZM_f0[1]) <= eps
    assert abs(ZM_f[2] -  ZM_f0[2]) <= eps
    assert abs(ZM_f[3] -  ZM_f0[3]) <= eps
    assert abs(ZM_f[4] -  ZM_f0[4]) <= eps
    assert abs(ZM_f[5] -  ZM_f0[5]) <= eps
    assert abs(ZM_f[6] -  ZM_f0[6]) <= eps
    assert abs(ZM_f[7] -  ZM_f0[7]) <= eps
    assert abs(ZM_f[8] -  ZM_f0[8]) <= eps
    assert abs(ZM_f[9] -  ZM_f0[9]) <= eps
    assert abs(ZM_f[10] - ZM_f0[10]) <= eps
    assert abs(ZM_f[11] - ZM_f0[11]) <= eps
    assert abs(ZM_f[12] - ZM_f0[12]) <= eps
    assert abs(ZM_f[13] - ZM_f0[13]) <= eps
    
    # test mirror-invariance
    img.mirror_horizontal()
    ZM_f = img.zernike_moments()
    assert abs(ZM_f[0] -  ZM_f0[0]) <= eps
    assert abs(ZM_f[1] -  ZM_f0[1]) <= eps
    assert abs(ZM_f[2] -  ZM_f0[2]) <= eps
    assert abs(ZM_f[3] -  ZM_f0[3]) <= eps
    assert abs(ZM_f[4] -  ZM_f0[4]) <= eps
    assert abs(ZM_f[5] -  ZM_f0[5]) <= eps
    assert abs(ZM_f[6] -  ZM_f0[6]) <= eps
    assert abs(ZM_f[7] -  ZM_f0[7]) <= eps
    assert abs(ZM_f[8] -  ZM_f0[8]) <= eps
    assert abs(ZM_f[9] -  ZM_f0[9]) <= eps
    assert abs(ZM_f[10] - ZM_f0[10]) <= eps
    assert abs(ZM_f[11] - ZM_f0[11]) <= eps
    assert abs(ZM_f[12] - ZM_f0[12]) <= eps
    assert abs(ZM_f[13] - ZM_f0[13]) <= eps
    
    # test scale-invariance
    img = img.scale(3, 1)
    ZM_f = img.zernike_moments()
    assert abs(ZM_f[0] -  ZM_f0[0]) <= 0.1
    assert abs(ZM_f[1] -  ZM_f0[1]) <= 0.1
    assert abs(ZM_f[2] -  ZM_f0[2]) <= 0.1
    assert abs(ZM_f[3] -  ZM_f0[3]) <= 0.1
    assert abs(ZM_f[4] -  ZM_f0[4]) <= 0.1
    assert abs(ZM_f[5] -  ZM_f0[5]) <= 0.1
    assert abs(ZM_f[6] -  ZM_f0[6]) <= 0.1
    assert abs(ZM_f[7] -  ZM_f0[7]) <= 0.1
    assert abs(ZM_f[8] -  ZM_f0[8]) <= 0.1
    assert abs(ZM_f[9] -  ZM_f0[9]) <= 0.1
    assert abs(ZM_f[10] - ZM_f0[10]) <= 0.1
    assert abs(ZM_f[11] - ZM_f0[11]) <= 0.1
    assert abs(ZM_f[12] - ZM_f0[12]) <= 0.1
    assert abs(ZM_f[13] - ZM_f0[13]) <= 0.1
