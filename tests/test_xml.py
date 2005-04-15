import py.test

from gamera.core import *
init_gamera()

from gamera import gamera_xml

def equal_files(filea, fileb):
   a = open(filea, "r").read()
   b = open(fileb, "r").read()
   return a == b

def equal_gzip_files(filea, fileb):
   import gzip
   a = gzip.open(filea, "r").read()
   b = gzip.open(fileb, "r").read()
   return a == b

# High-level API
def test_glyphs_from_xml():
   glyphs = gamera_xml.glyphs_from_xml("data/testline.xml")
   assert len(glyphs) == 66
      
def test_glyphs_with_features_from_xml():
   glyphs = gamera_xml.glyphs_with_features_from_xml(
      "data/testline.xml", ["area", "aspect_ratio"])
   assert len(glyphs) == 66
   assert len(glyphs[0].features) == 2

def test_glyphs_to_xml():
   glyphs = gamera_xml.glyphs_from_xml("data/testline.xml")
   gamera_xml.glyphs_to_xml("tmp/testline_test1.xml", glyphs, False)
   assert equal_files("tmp/testline_test1.xml", "data/testline_test1.xml")

def test_glyphs_to_xml_with_features():
   glyphs = gamera_xml.glyphs_with_features_from_xml("data/testline.xml")
   gamera_xml.glyphs_to_xml("tmp/testline_test2.xml", glyphs, True)
   assert equal_files("tmp/testline_test2.xml", "data/testline_test2.xml")

def test_glyphs_from_xml_gz():
   glyphs = gamera_xml.glyphs_from_xml("data/testline.xml.gz")
   assert len(glyphs) == 66

def test_glyphs_to_xml_gz():
   glyphs = gamera_xml.glyphs_from_xml("data/testline.xml")
   gamera_xml.glyphs_to_xml("tmp/testline_test1.xml.gz", glyphs, False)
   assert equal_gzip_files("tmp/testline_test1.xml.gz", "data/testline_test1.xml.gz")

# Low-level API
def test_write_xml():
   glyphs = gamera_xml.glyphs_from_xml("data/testline.xml")
   writer = gamera_xml.WriteXML(glyphs)
   result_string = writer.string()
   writer.write_filename("tmp/testline_test3.xml")
   assert equal_files("tmp/testline_test3.xml", "data/testline_test3.xml")

def test_symbol_table():
   symbol_table = gamera_xml.LoadXML(parts=['symbol_table']).parse_filename("data/symbol_table.xml").symbol_table
   gamera_xml.WriteXMLFile([], symbol_table).write_filename("tmp/symbol_table.xml")
   assert equal_files("data/symbol_table.xml", "tmp/symbol_table.xml")

   symbol_table = gamera_xml.LoadXML(parts=['symbol_table']).parse_filename("data/testline.xml").symbol_table
   gamera_xml.WriteXMLFile([], symbol_table).write_filename("tmp/symbol_table2.xml")
   assert equal_files("data/symbol_table.xml", "tmp/symbol_table2.xml")

# Error cases
def _test_missing_attributes():
   glyphs = gamera_xml.glyphs_from_xml("data/missing_attributes.xml")

def test_missing_attributes():
   py.test.raises(gamera_xml.XMLError, _test_missing_attributes)

def _test_malformed():
   glyphs = gamera_xml.glyphs_from_xml("data/malformed.xml")

def test_malformed():
   py.test.raises(gamera_xml.XMLError, _test_malformed)
