from gamera.core import *
from gamera import knn, classify, gamera_xml
init_gamera()

correct_classes = ['latin.lower.letter.h', 'latin.lower.ligature.ft', 'latin.capital.letter.m', '_group._part.latin.capital.letter.m', '_group._part.latin.capital.letter.m', '_group._part.latin.lower.letter.i', 'latin.lower.letter.d', 'latin.capital.letter.m', 'latin.capital.letter.t', '_group._part.latin.lower.letter.h', 'latin.lower.ligature.fi', '_group._part.latin.lower.ligature.ft', '_group._part.latin.lower.letter.h', '_group._part.latin.lower.letter.i', 'latin.lower.letter.h', 'latin.lower.letter.d', 'latin.lower.letter.d', 'latin.capital.letter.m', 'latin.capital.letter.c', 'latin.lower.letter.t', 'latin.lower.letter.t', '_group._part.latin.lower.letter.n', 'latin.lower.letter.e', 'latin.lower.letter.a', 'latin.lower.letter.r', 'latin.lower.letter.r', 'latin.lower.letter.a', '_group._part.latin.lower.letter.n', '_group._part.latin.lower.letter.i', 'latin.lower.letter.a', 'latin.lower.letter.r', 'latin.lower.letter.r', '_group._part.latin.lower.letter.h', 'latin.lower.letter.e', 'latin.lower.letter.r', 'latin.lower.letter.e', 'latin.lower.letter.n', 'latin.lower.letter.n', 'latin.lower.letter.o', 'latin.lower.letter.e', 'latin.lower.letter.s', 'latin.lower.letter.e', '_group._part.latin.lower.letter.h', '_group._part.latin.lower.letter.i', '_group._part.latin.lower.letter.g', 'latin.lower.letter.a', 'latin.lower.letter.r', 'latin.lower.letter.r', 'latin.lower.letter.o-', 'latin.lower.letter.r', 'hyphen-minus', 'comma', 'full.stop', 'comma', '_group._part.latin.lower.ligature.ft', 'noise', '_group._part.latin.lower.letter.g']


results = [
      ['latin.lower.letter.n', 'latin.capital.letter.m', 'latin.lower.letter.h', 'latin.lower.ligature.ft', 'latin.capital.letter.t', 'latin.lower.letter.g'],
      ['latin.lower.letter.n', 'latin.capital.letter.m', 'latin.lower.letter.h', 'latin.lower.ligature.ft', 'latin.lower.letter.h', 'latin.lower.letter.g'],
      ['latin.lower.letter.n', 'latin.capital.letter.m', 'latin.capital.letter.t', 'latin.lower.letter.h', 'latin.lower.ligature.ft', 'latin.lower.letter.h', 'latin.lower.letter.i', 'latin.lower.letter.g'],
      ['latin.lower.letter.n', 'latin.capital.letter.m', 'latin.capital.letter.t', 'latin.lower.letter.h', 'latin.lower.ligature.ft', 'latin.lower.letter.h', 'latin.lower.letter.i', 'latin.lower.letter.g']
   ]


def _test_grouping(classifier, ccs):
   classifier.change_feature_set(['volume64regions','moments'])
   cases = [(classify.ShapedGroupingFunction(4), 'min'),
             (classify.ShapedGroupingFunction(4), 'avg'),
             (None, 'min'),
             (None, 'avg')
            ]

   for (i, (func, criterion)) in enumerate(cases):
        if func == None:
            added,removed = classifier.group_list_automatic(ccs, criterion=criterion)
        else:
            added, removed = classifier.group_list_automatic(
               ccs,
               grouping_function = func,
               max_parts_per_group = 10,
               max_graph_size = 64,
               criterion=criterion)
   
        added.sort(lambda a,b: cmp(a.offset_x,b.offset_x))
        assert [cc.get_main_id() for cc in added] == results[i]


def _test_classification(classifier, ccs):
   (id_name, confidence) = classifier.guess_glyph_automatic(ccs[0])
   assert id_name == [(1.0, 'latin.lower.letter.h')]

   classifier.classify_glyph_automatic(ccs[1])
   assert ccs[1].id_name == [(1.0, 'latin.lower.ligature.ft')]
   
   added, removed = classifier.classify_list_automatic(ccs)
   assert [cc.get_main_id() for cc in ccs] == correct_classes
   assert added == [] and removed == []

   classifier.classify_and_update_list_automatic(ccs)
   assert [cc.get_main_id() for cc in ccs] == correct_classes

   classifier.change_feature_set(['area'])
   assert len(list(classifier.database)[0].features) == 1

   added, removed = classifier.classify_list_automatic(ccs)
   assert [cc.get_main_id() for cc in ccs] != correct_classes

   _test_grouping(classifier, ccs)


def _test_training(classifier, ccs):
   length = len(classifier.get_glyphs())
   classifier.classify_glyph_manual(ccs[0], "dummy")
   assert len(classifier.get_glyphs()) == length + 1
   added, removed = classifier.classify_list_manual(ccs, "dummy")
   assert len(classifier.get_glyphs()) == length + len(ccs)
   assert added == [] and removed == []
   classifier.classify_and_update_list_manual(ccs, "dummy")
   assert len(classifier.get_glyphs()) == length + len(ccs)
   classifier.add_to_database(ccs)
   assert len(classifier.get_glyphs()) == length + len(ccs)
   classifier.remove_from_database(ccs)
   assert len(classifier.get_glyphs()) == length
   classifier.add_to_database(ccs)
   assert len(classifier.get_glyphs()) == length + len(ccs)

def test_interactive_classifier():
   # We assume the XML reading/writing itself is fine (given
   # test_xml), but we should test the wrappers in classify anyway
   image = load_image("data/testline.png")
   ccs = image.cc_analysis()

   classifier = knn.kNNInteractive([])
   assert classifier.is_interactive()
   assert len(classifier.get_glyphs()) == 0
   
   classifier.from_xml_filename("data/testline.xml")
   assert len(classifier.get_glyphs()) == 66
   _test_classification(classifier, ccs)
   _test_training(classifier, ccs)
   length = len(classifier.get_glyphs())

   # subtract len(group_parts) because to_xml_filename() does 
   # not save "_group._part"
   group_parts = [x for x in classifier.get_glyphs() 
                  if x.get_main_id().startswith("_group._part")]
   length = length - len(group_parts)

   classifier.to_xml_filename("tmp/testline_classifier.xml")
   classifier.from_xml_filename("tmp/testline_classifier.xml")
   assert len(classifier.get_glyphs()) == length
   classifier.merge_from_xml_filename("data/testline.xml")
   assert len(classifier.get_glyphs()) == length + 66
   classifier.clear_glyphs()
   assert len(classifier.get_glyphs()) == 0
   classifier.from_xml_filename("data/testline.xml")
   assert len(classifier.get_glyphs()) == 66
   
def test_noninteractive_classifier():
   # We assume the XML reading/writing itself is fine (given
   # test_xml), but we should test the wrappers in classify anyway
   image = load_image("data/testline.png")
   ccs = image.cc_analysis()

   database = gamera_xml.glyphs_from_xml("data/testline.xml")
   classifier = knn.kNNNonInteractive(database)
   assert not classifier.is_interactive()
   assert len(classifier.get_glyphs()) == 66
   
   _test_classification(classifier, ccs)

   classifier.serialize("tmp/serialized.knn")
   classifier.clear_glyphs()
   assert len(classifier.get_glyphs()) == 0
   classifier.unserialize("tmp/serialized.knn")

