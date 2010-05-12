#
# Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, Karl MacMillan
#               2009-2010 Christoph Dalitz
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

import sys
import core # grab all of the standard gamera modules
import util, gamera_xml, config
from fudge import Fudge
from gamera.gui import has_gui, gui_util
from gamera.gameracore import CONFIDENCE_DEFAULT

"""This file defines the Python part of classifiers.  These wrapper classes
contain a reference to a core classifier class (unusally written in C++).
They add functionality for XML loading/saving, splitting/grouping, and
keeping track of a database of glyphs (in the Interactive case.)"""

class ClassifierError(Exception):
   pass

class _Classifier:
   """The base class for both the interactive and noninteractive classifier."""

   ########################################
   # INFORMATION
   def get_name(self):
      return self.__class__.__name__

   def is_dirty(self):
      return self._database.is_dirty and len(self._database)
   def _set_is_dirty(self, val):
      self._database.is_dirty = val
   is_dirty = property(is_dirty, _set_is_dirty)

   def get_database(self):
      return self._database
   def set_database(self, other):
      raise RuntimeError("You can't change the database.  This exception should never be raised.  Please report it to the authors.")
   database = property(get_database, set_database)

   def __len__(self):
      return len(self._database)

   ########################################
   # GROUPING
   def group_list_automatic(self, glyphs, grouping_function=None,
                            evaluate_function=None, max_parts_per_group=4,
                            max_graph_size=16):
      """**group_list_automatic** (ImageList *glyphs*, Function
*grouping_function* = ``None``, Function *evaluate_function* = ``None``,
int *max_parts_per_group* = 4, int *max_graph_size* = 16)

Classifies the given list of glyphs.  Adjacent glyphs are joined
together if doing so results in a higher global confidence.  Each part
of a joined glyph is classified as HEURISTIC with the prefix
``_group``.

*glyphs*
  The list of glyphs to group and classify.

*grouping_function*
  A function that determines how glyphs are initially
  combined.  This function must take exactly two arguments, which the
  grouping algorithm will pass an arbitrary pair of glyphs from
  *glyphs*.  If the two glyphs should be considered for grouping, the
  function should return ``True``, else ``False``.  

  If no *grouping_function* is provided, a default one will be used.

*evaluate_function*
   A function that evaluates a grouping of glyphs.  This function must
   take exactly one argument which is a list of glyphs.  The function
   should return a confidence value between 0 and 1 (1 being most
   confident) representing how confidently the grouping forms a valid
   character.

   If no *evaluate_function* is provided, a default one will be used.

*max_parts_per_group*
   The maximum number of connected components that will be grouped
   together and tested as a group.  For performance reasons, this
   number should be kept relatively small.

*max_graph_size*
   Subgraphs (potentially connected areas of the image) larger than
   the given number of nodes will be ignored.  This is a hack to
   prevent the runtime of the algorithm from exploding.

The function returns a 2-tuple (pair) of lists: (*add*, *remove*).
*add* is a list of glyphs that were created by classifying any glyphs
as a split (See Initialization_) or grouping.  *remove* is a list of
glyphs that are no longer valid due to reclassifying glyphs from a
split to something else.

The list *glyphs* is never modified.  Instead, detected parts of groups
are classified as ``_group._part.*``, where ``*`` stands for the class
name of the grouped glyph. This means that after calling this function,
you must remove the *remove* CCs and all CCs with a class name beginning with
```_group._part`` from *glyph*, and you must add all glyphs from *add*
to it. Or you can instead call group_and_update_list_automatic_, which does 
this automatically for you.
"""
      glyphs = [x for x in glyphs if x.classification_state != 3]
      if len(glyphs) == 0:
         return [], []

      splits, removed = self.classify_list_automatic(glyphs)
      glyphs = [x for x in glyphs if not x.get_main_id().startswith('split')]
      if grouping_function is None:
         grouping_function = BoundingBoxGroupingFunction(4)
      G = self._pregroup(glyphs, grouping_function)
      if evaluate_function is None:
         evaluate_function = self._evaluate_subgroup
      found_unions = self._find_group_unions(
         G, evaluate_function, max_parts_per_group, max_graph_size)
      return found_unions + splits, removed

   def group_and_update_list_automatic(self, glyphs, *args, **kwargs):
      """**group_and_update_list_automatic** (ImageList *glyphs*, Function
*grouping_function* = ``None``, Function *evaluate_function* = ``None``,
int *max_parts_per_group* = 5, int *max_graph_size* = 16)

A convenience wrapper around group_list_automatic_ that returns
a list of glyphs that is already updated for splitting and grouping."""
      added, removed = self.group_list_automatic(glyphs, *args, **kwargs)
      glyphs = [g for g in glyphs if not g.get_main_id().startswith("_group._part")]
      return self._update_after_classification(glyphs, added, removed)

   def _pregroup(self, glyphs, function):
      from gamera import graph
      G = graph.Undirected()
      G.add_nodes(glyphs)
      progress = util.ProgressFactory("Pre-grouping glyphs...", len(glyphs))
      try:
         for i in range(len(glyphs)):
            gi = glyphs[i]
            for j in range(i + 1, len(glyphs)):
               gj = glyphs[j]
               if function(gi, gj):
                  G.add_edge(gi, gj)
            progress.step()
      finally:
         progress.kill()
      return G

   def _evaluate_subgroup(self, subgroup):
      import image_utilities
      if len(subgroup) > 1:
         union = image_utilities.union_images(subgroup)
         classification, confidence = self.guess_glyph_automatic(union)
         classification_name = classification[0][1]
         if (classification_name.startswith("_split") or
             classification_name.startswith("skip")):
            return 0.0
         else:
            return classification[0][0]
      if len(subgroup):
         classification = subgroup[0].id_name[0]
         if classification[1].startswith('_group._part'):
            return 0.0
         return classification[0]
      raise ValueError("Something is wrong here...  Either you don't have classifier data or there is an internal error in the grouping algorithm.")

   def _find_group_unions(self, G, evaluate_function, max_parts_per_group=5,
                          max_graph_size=16):
      import image_utilities
      progress = util.ProgressFactory("Grouping glyphs...", G.nsubgraphs)
      try:
         found_unions = []
         for root in G.get_subgraph_roots():
            if G.size_of_subgraph(root) > max_graph_size:
               continue
            best_grouping = G.optimize_partitions(
               root, evaluate_function, max_parts_per_group, max_graph_size)
            if not best_grouping is None:
               for subgroup in best_grouping:
                  if len(subgroup) > 1:
                     union = image_utilities.union_images(subgroup)
                     found_unions.append(union)
                     classification, confidence = self.guess_glyph_automatic(union)
                     union.classify_heuristic(classification)
                     part_name = "_group._part." + classification[0][1]
                     for glyph in subgroup:
                        glyph.classify_heuristic(part_name)
            progress.step()
      finally:
         progress.kill()
      return found_unions

   ########################################
   # AUTOMATIC CLASSIFICATION
   def classify_glyph_automatic(self, glyph, max_recursion=10, recursion_level=0):
      """**classify_glyph_automatic** (Image *glyph*, int *max_recursion* = 10)

Classifies a glyph and sets its ``classification_state`` and
``id_name``.  (If you don't want the glyph changed, use
guess_glyph_automatic_.)

*glyph*
  The glyph to classify.

*max_recursion* (optional)
  Limit the number of split recursions.

Returns a 2-tuple (pair) of lists: (*add*, *remove*).  *add* is a list
of glyphs that were created by classifying *glyph* as a split (See
Initialization_).  *remove* is a list of glyphs that are no longer
valid due to reclassifying *glyph* from a split to something else.
Most often, both of these lists will be empty.  You will normally want
to use these lists to update the collection of glyphs on the current
page."""
      if recursion_level > max_recursion:
         return [], []
      # Since we only have one glyph to classify, we can't do any grouping
      if (len(self.database) and
          glyph.classification_state in (core.UNCLASSIFIED, core.AUTOMATIC)):
         self.generate_features(glyph)
         removed = glyph.children_images
         (id, conf) = self._classify_automatic_impl(glyph)
         glyph.classify_automatic(id)
         glyph.confidence = conf
         splits = self._do_splits(self, glyph)
         all_splits = splits[:]
         for g2 in splits:
            recurse_splits, recurse_removed = self.classify_glyph_automatic(
               g2, max_recursion, recursion_level + 1)
            all_splits.extend(recurse_splits)
            removed.extend(recurse_removed)
         return all_splits, removed
      return [], []

   def _classify_list_automatic(self, glyphs, max_recursion=10, recursion_level=0, progress=None):

      # There is a slightly convoluted handling of the progress bar here, since
      # this function is called recursively on split glyphs
      if recursion_level == 0:
         progress = util.ProgressFactory("Classifying glyphs...", len(glyphs))
      try:
         if (recursion_level > max_recursion):
            return [], []
         added = []
         removed = {}
         for glyph in glyphs:
            if glyph.classification_state in (core.UNCLASSIFIED, core.AUTOMATIC):
               for child in glyph.children_images:
                  removed[child] = None
         for glyph in glyphs:
            if not removed.has_key(glyph):
               self.generate_features(glyph)
               if (glyph.classification_state in
                   (core.UNCLASSIFIED, core.AUTOMATIC)):
                  (id, conf) = self._classify_automatic_impl(glyph)
                  glyph.classify_automatic(id)
                  glyph.confidence = conf
                  adds = self._do_splits(self, glyph)
                  progress.add_length(len(adds))
                  added.extend(adds)
            progress.step()
         if len(added):
            added_recurse, removed_recurse = self._classify_list_automatic(
               added, max_recursion, recursion_level+1, progress)
            added.extend(added_recurse)
            for glyph in removed_recurse:
               removed[glyph] = None
      finally:
         if recursion_level == 0:
            progress.kill()
      return added, removed.keys()

   def classify_list_automatic(self, glyphs, max_recursion=10, progress=None):
      """**classify_list_automatic** (ImageList *glyphs*, int *max_recursion* = 10)

Classifies a list of glyphs and sets the classification_state and
id_name of each glyph.  (If you don't want it set, use guess_glyph_automatic_.)

*glyphs*
  A list of glyphs to classify.

*max_recursion*
  The maximum level of recursion to follow when splitting glyphs.
  Since some glyphs will split into parts that then classify as
  ``_split`` in turn, a maximum depth should be set to avoid infinite
  recursion.  This number can normally be set quite low, depending on
  the application.

Return type: (*add*, *remove*)

  The list *glyphs* is never modified by the function.  Instead, it
  returns a 2-tuple (pair) of lists: (*add*, *remove*).  *add* is a
  list of glyphs that were created by classifying glyphs as a
  split (See Initialization_).  *remove* is a list of glyphs that are
  no longer valid due to reclassifying glyphs from a split to
  something else.  Most often, both of these lists will be empty.  You
  will normally want to use these lists to update the collection of
  glyphs on the current page.  If you just want a new list returned
  with these updates already made, use classify_and_update_list_automatic_.
"""
      return self._classify_list_automatic(glyphs, max_recursion, 0, progress)

   def _update_after_classification(self, glyphs, added, removed):
      result = glyphs + added
      for g in removed:
         if g in result:
            result.remove(g)
      return result

   def classify_and_update_list_automatic(self, glyphs, *args, **kwargs):
      """**classify_and_update_list_automatic** (ImageList *glyphs*, Int
*max_recursion* = ``10``)

A convenience wrapper around classify_list_automatic_ that returns
a list of glyphs that is already updated based on splitting."""
      added, removed = self.classify_list_automatic(glyphs, *args, **kwargs)
      return self._update_after_classification(glyphs, added, removed)

   # Since splitting glyphs is optional (when the classifier instance is
   # created) we have two versions of this function, so that there needn't
   # be an 'if' statement everywhere.
   def _do_splits_impl(self, glyph):
      id = glyph.get_main_id()
      if (id.startswith('_split.')):
         parts = id.split('.')
         if (len(parts) != 2 or not hasattr(glyph, parts[1])):
            raise ClassifierError("'%s' is not a known or valid split function." % parts[1])
         try:
            if sys.version_info[:2] < (2, 4):
               splits = getattr(glyph, parts[1]).__call__(glyph)
            else:
               splits = getattr(glyph, parts[1]).__call__()
         except core.SegmentationError:
            if len(glyph.id_name) >= 2:
               glyph.id_name = glyph.id_name[1:]
            else:
               glyph.id_name = [(0.0, '_ERROR')]
            return []
         else:
            glyph.children_images = splits
            return splits
      return []

   def _do_splits_null(self, glyph):
      return []

   ########################################
   # XML
   # Note that unclassified glyphs in the XML file are ignored.
   def to_xml(self, stream, with_features=True):
      """**to_xml** (stream *stream*)

Saves the training data in XML format to the given stream (which could
be any object supporting the file protocol, such as a file object or StringIO
object)."""
      self.is_dirty = False
      return gamera_xml.WriteXML(
         glyphs=self.get_glyphs()).write_stream(stream, with_features)

   def to_xml_filename(self, filename, with_features=True):
      """**to_xml_filename** (FileSave *filename*)

Saves the training data in XML format to the given filename."""
      self.is_dirty = False
      return gamera_xml.WriteXMLFile(
         glyphs=self.get_glyphs()).write_filename(filename, with_features)

   def from_xml(self, stream):
      """**from_xml** (stream *stream*)

Loads the training data from the given stream (which could be any object 
supporting the file protocol, such as a file object or StringIO object.)"""
      self._from_xml(gamera_xml.LoadXML().parse_stream(stream))

   def from_xml_filename(self, filename):
      """**from_xml_filename** (FileOpen *filename*)

Loads the training data from the given filename."""
      stream = gamera_xml.LoadXML().parse_filename(filename)
      self._from_xml(stream)

   def _from_xml(self, xml):
      database = [x for x in xml.glyphs
                  if x.classification_state != core.UNCLASSIFIED]
      self.set_glyphs(database)

   def merge_from_xml(self, stream):
      """**merge_from_xml** (stream *stream*)

Loads the training data from the given stream (which could be a file
handle or StringIO object) and adds it to the existing training data."""
      self._merge_xml(gamera_xml.LoadXML().parse_stream(stream))

   def merge_from_xml_filename(self, filename):
      """**merge_from_xml_filename** (stream *stream*)

Loads the training data from the given filename and adds it to the
existing training data."""
      self._merge_xml(gamera_xml.LoadXML().parse_filename(filename))

   def _merge_xml(self, xml):
      database = [x for x in xml.glyphs
                  if x.classification_state != core.UNCLASSIFIED]
      self.merge_glyphs(database)

   ##############################################
   # Features
   def generate_features_on_glyphs(self, glyphs):
      """Generates features for all the given glyphs."""
      progress = util.ProgressFactory("Generating features...",
                                      len(glyphs) / 16)
      try:
         for i, glyph in enumerate(glyphs):
            self.generate_features(glyph)
            if i & 0xf == 0:
               progress.step()
      finally:
         progress.kill()
   
class NonInteractiveClassifier(_Classifier):
   def __init__(self, database=[], perform_splits=True):
      """**NonInteractiveClassifier** (ImageList *database* = ``[]``, bool *perform_splits* = ``True``)

Creates a new classifier instance.

*database*
        Can come in two forms:

           - When a list (or Python iterable) each element is a glyph to
             use as training data for the classifier

           - *For non-interactive classifiers only*, when *database* is a filename,
             the classifier will be "unserialized" from the given file.

        Any images in the list that were manually classified (have
	classification_state == MANUAL) will be used as training data
	for the classifier.  Any UNCLASSIFIED or AUTOMATICALLY
	classified images will be ignored.

	When initializing a noninteractive classifier, the database
	*must* be non-empty.

*perform_splits*
          If ``perform_splits`` is ``True``, glyphs trained with names
	  beginning with ``_split.`` are run through a given splitting
	  algorithm.  For instance, glyphs that need to be broken into
	  upper and lower halves for further classification of those
	  parts would be trained as ``_split.splity``.  When the
	  automatic classifier encounters glyphs that most closely
	  match those trained as ``_split``, it will perform the
	  splitting algorithm and then continue to recursively
	  classify its parts.

	  The `splitting algorithms`__ are documented in the plugin documentation.

.. __: plugins.html#segmentation

          New splitting algorithms can be created by `writing plugin`__ methods
          in the category ``Segmentation``.  

.. __: writing_plugins.html

      """
      if type(database) == list:
         self._database = util.CallbackList(database)
         self.set_glyphs(database)
      elif database[-4:] == ".xml":
         self._database = util.CallbackList(database)
         self.from_xml_filename(database)
      else:
         self._database = util.CallbackList([])
         self.unserialize(database)

      if perform_splits:
         self._do_splits = self.__class__._do_splits_impl
      else:
         self._do_splits = self.__class__._do_splits_null
      self._perform_splits = perform_splits

      self.confidence_types = [CONFIDENCE_DEFAULT]

   def __del__(self):
      # Seems redundant, but this triggers a callback to the GUI, if running.
      del self._database

   def is_interactive():
      """Boolean **is_interactive** ()

Returns ``True`` if classifier is interactive, else ``False``."""
      return False
   is_interactive = staticmethod(is_interactive)

   ########################################
   # BASIC DATABASE MANIPULATION FUNCTIONS
   def get_glyphs(self):
      """ImageList **get_glyphs** ()

Returns a list of the glyphs in the classifier."""
      return list(self.database)
      
   def set_glyphs(self, glyphs):
      """**set_glyphs** (ImageList *glyphs*)

Sets the training data for the classifier to the given list of glyphs.

On some non-interactive classifiers, this operation can be quite
expensive.
"""
      self.generate_features_on_glyphs(glyphs)
      self.database.clear()
      self.database.extend(glyphs)
      self.instantiate_from_images(self.database)

   def merge_glyphs(self, glyphs):
      """**merge_glyphs** (ImageList *glyphs*)

Adds the given glyphs to the current set of training data.

On some non-interactive classifiers, this operation can be quite
expensive."""
      self.generate_features_on_glyphs(glyphs)
      self.database.extend(glyphs)
      self.instantiate_from_images(self.database)

   def clear_glyphs(self):
      """**clear_glyphs** ()

Removes all training data from the classifier.
"""
      self.database.clear()

   def load_settings(self, filename):
      """**load_settings** (FileOpen *filename*)

Loads classifier-specific settings from the given filename.  The
format of this file is entirely dependant on the concrete classifier
type."""
      _Classifier.load_settings(self, filename)
      self.instantiate_from_images(self.database)

   ########################################
   # AUTOMATIC CLASSIFICATION
   # (most of this is implemented in the base class, _Classifier)
   def guess_glyph_automatic(self, glyph):
      """id_name **guess_glyph_automatic** (Image *glyph*)

Classifies the given *glyph* without setting its classification.  The
return value is a tuple of the form ``(id_name,confidencemap)``, where
*idname* is a list of the form `idname`_, and *confidencemap* is a
map of the form `confidence`_ listing the confidences of the main id.

.. _idname: #id-name

.. _confidence: #confidence
"""
      self.generate_features(glyph)
      return self.classify(glyph)

   def _classify_automatic_impl(self, glyph):
      return self.classify(glyph)

class InteractiveClassifier(_Classifier):
   def __init__(self, database=[], perform_splits=True):
      """**InteractiveClassifier** (ImageList *database* = ``[]``, bool *perform_splits* = ``True``)

Creates a new classifier instance.

*database*
        Must be a list (or Python interable) containing glyphs to use
        as training data for the classifier.

        Any images in the list that were manually classified (have
	classification_state == MANUAL) will be used as training data
	for the classifier.  Any UNCLASSIFIED or AUTOMATICALLY
	classified images will be ignored.

	When initializing a noninteractive classifier, the database
	*must* be non-empty.

*perform_splits*
	  If ``perform_splits`` is ``True``, glyphs trained with names
	  beginning with ``_split.`` are run through a given splitting
	  algorithm.  For instance, glyphs that need to be broken into
	  upper and lower halves for further classification of those
	  parts would be trained as ``_split.splity``.  When the
	  automatic classifier encounters glyphs that most closely
	  match those trained as ``_split``, it will perform the
	  splitting algorithm and then continue to recursively
	  classify its parts.

	  The `splitting algorithms`__ are documented in the plugin documentation.

.. __: plugins.html#segmentation

          New splitting algorithms can be created by `writing plugin`__ methods
          in the category ``Segmentation``.  

.. __: writing_plugins.html

      """
      self._database = util.CallbackSet()
      self.set_glyphs(database)
      if perform_splits:
         self._do_splits = self.__class__._do_splits_impl
      else:
         self._do_splits = self.__class__._do_splits_null
      self._perform_splits = perform_splits
      self._display = None
      self.confidence_types = [CONFIDENCE_DEFAULT]

   def __del__(self):
      # Seems redundant, but this triggers a callback to the GUI, if running.
      del self._database

   def is_interactive():
      return True
   is_interactive = staticmethod(is_interactive)

   ########################################
   # BASIC DATABASE MANIPULATION FUNCTIONS
   def get_glyphs(self):
      return self.database
      
   def set_glyphs(self, glyphs):
      glyphs = util.make_sequence(glyphs)
      self.clear_glyphs()
      self.generate_features_on_glyphs(glyphs)
      self.database.extend(glyphs)

   def merge_glyphs(self, glyphs):
      glyphs = util.make_sequence(glyphs)
      self.generate_features_on_glyphs(glyphs)
      self.database.extend(glyphs)

   def clear_glyphs(self):
      self.database.clear()

   ########################################
   # AUTOMATIC CLASSIFICATION
   def _classify_automatic_impl(self, glyph):
      if len(self.database) == 0:
         raise ClassifierError(
            "Cannot classify using an empty production database.")
      for child in glyph.children_images:
         if child in self.database:
            self.database.remove(child)
      return self.classify_with_images(self.database, glyph)

   def guess_glyph_automatic(self, glyph):
      if len(self.database):
         self.generate_features(glyph)
         return self.classify_with_images(self.database, glyph)
      else:
         return ([(0.0, 'unknown')], {})

   ########################################
   # MANUAL CLASSIFICATION
   def classify_glyph_manual(self, glyph, id):
      """**classify_glyph_manual** (Image *glyph*, String *id*)

Sets the classification of the given *glyph* to the given *id* and
then adds the glyph to the training data.  Call this function when the
end user definitively knows the identity of the glyph.

*glyph*
	The glyph to classify.

*id*
	The class name.

.. note::
   Here *id* is a simple string, not of the `id_name`_ format, since
   the confidence of a manual classification is always 1.0."""
      # Deal with grouping
      if id.startswith('_group'):
         raise ClassifierError(
            "You must select more than one connected component " +
            "to create a group")

      removed = {}
      for child in glyph.children_images:
         removed[child] = None
         if child in self.database:
            self.database.remove(child)
      glyph.classify_manual([(1.0, id)])
      self.generate_features(glyph)
      self.database.append(glyph)
      return self._do_splits(self, glyph), removed.keys()

   def classify_list_manual(self, glyphs, id):
      """**classify_list_manual** (ImageList *glyphs*, String *id*)

Sets the classification of the given *glyphs* to the given *id* and
then adds the glyphs to the training data.  Call this function when the
end user definitively knows the identity of the glyphs.

If *id* begins with the special prefix ``_group``, all of the glyphs
in *glyphs* are combined and the result is added to the training
data.  This is useful for characters that always appear with multiple
connnected components, such as the lower-case *i*.

*glyphs*
	The glyphs to classify.

*id*
	The class name.

.. note::
   Here *id* is a simple string, not of the `id_name`_ format, since
   the confidence of a manual classification is always 1.0."""
      if id.startswith('_group'):
         if len(glyphs) > 1:
            import image_utilities
            parts = id.split('.')
            sub = '.'.join(parts[1:])
            union = image_utilities.union_images(glyphs)
            for glyph in glyphs:
               if glyph.nrows > 2 and glyph.ncols > 2:
                  glyph.classify_heuristic('_group._part.' + sub)
                  self.generate_features(glyph)
            added, removed = self.classify_glyph_manual(union, sub)
            added.append(union)
            return added, removed
         else:
            # grouping a single glyph corrupts the classifier_glyph.xml file
            added, removed = [],[]
            if has_gui.gui:
               import wx
               cursorbusy = False
               if wx.IsBusy():
                  cursorbusy = True
                  wx.EndBusyCursor()
               gui_util.message('Grouping of only a single glyph is not allowed.')
               if cursorbusy:
                  wx.BeginBusyCursor()
            return added, removed

      added = []
      removed = util.sets.Set()
      for glyph in glyphs:
         for child in glyph.children_images:
            removed.add(child)

      new_glyphs = []
      for glyph in glyphs:
         # Don't re-insert removed children glyphs
         if not glyph in removed:
            if not glyph in self.database:
               self.generate_features(glyph)
               new_glyphs.append(glyph)
            glyph.classify_manual([(1.0, id)])
            added.extend(self._do_splits(self, glyph))
      self.database.extend(new_glyphs)
      return added, list(removed)

   def classify_and_update_list_manual(self, glyphs, *args, **kwargs):
      """**classify_and_update_list_manual** (ImageList *glyphs*, Function
*grouping_function* = ``None``, Function *evaluate_function* = ``None``,
int *max_size* = 5)

A convenience wrapper around group_list_automatic_ that returns
a list of glyphs that is already updated for splitting and grouping."""
      added, removed = self.classify_list_manual(glyphs, *args, **kwargs)
      return self._update_after_classification(glyphs, added, removed)

   def add_to_database(self, glyphs):
      """**add_to_database** (ImageList *glyphs*)

Adds the given glyph (or list of glyphs) to the classifier training data.  Will not add duplicates
to the training data.  Unlike classify_glyph_manual_, no grouping support is performed.
"""
      glyphs = util.make_sequence(glyphs)
      new_glyphs = []
      for glyph in glyphs:
         if (glyph.classification_state == core.MANUAL and
             not glyph in self.database):
            self.generate_features(glyph)
            new_glyphs.append(glyph)
      self.database.extend(new_glyphs)

   def remove_from_database(self, glyphs):
      """**remove_from_database** (ImageList *glyphs*)

Removes the given glyphs from the classifier training data.  Ignores silently
if a given glyph is not in the training data.
"""
      glyphs = util.make_sequence(glyphs)
      for glyph in glyphs:
         if glyph in self.database:
            self.database.remove(glyph)

   def display(self, current_database=[], context_image=None, symbol_table=[]):
      """**display** (ImageList *current_database* = ``[]``, Image
*context_image* = ``None``, List *symbol_table* = ``[]``)

Displays the `interactive classifier window`__, which is where manual
training usually takes place.

*current_database*
  A list of glyphs yet to be trained.

*context_image*
  An image of the page where the glyphs in *current_database* came
  from.

*symbol_table*
  A set of id names to insert by default into the symbol table.

.. __: gui.html#interactive-classifier-window"""
      if has_gui.gui and self._display is None:
         self._display = has_gui.gui.ShowClassifier(
            self, current_database, context_image, symbol_table)
      else:
         self._display.Show(1)
      return self._display

   def set_display(self, display):
      self._display = display

########################################
# GROUPING UTILITIES

class BasicGroupingFunction:
   def __init__(self, threshold):
      self._threshold = threshold

   def __call__(self, a, b):
      return Fudge(a, self._threshold).intersects(b)

class ShapedGroupingFunction:
   def __init__(self, threshold):
      from gamera.plugins import structural
      self._function = structural.shaped_grouping_function
      self._threshold = threshold

   def __call__(self, a, b):
      return self._function(a, b, self._threshold)

class BoundingBoxGroupingFunction:
   def __init__(self, threshold):
      from gamera.plugins import structural
      self._function = structural.bounding_box_grouping_function
      self._threshold = threshold

   def __call__(self, a, b):
      return self._function(a, b, self._threshold)

def average_bb_distance(ccs):
   """Calculates the average distance between the bounding boxes
in the given list of ccs."""
   average = 0
   for i, cc in enumerate(ccs):
      minimum = None
      for j in range(i + 1, len(ccs)):
         distance = cc.distance_bb(ccs[j])
         if distance < minimum or minimum is None:
            minimum = distance
      if not minimum is None:
         average += minimum
   average /= float(len(ccs))
   return average
