#
# various editing techniques to improve kNN Classifier performance
# Copyright (C) 2007, 2008 Colin Baumgarten
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

from random import randint
from gamera.args import Args, Int, Check
from gamera.knn import kNNInteractive
from gamera.util import ProgressFactory

def _randomSetElement(set):
    """Retrieve a random element from the given set

    *set*
      The set from which to select a random element"""
    randPos = randint(0, len(set) - 1);
    for pos, elem in enumerate(set):
        if pos == randPos:
            return elem

def _copyClassifier(original, k = 0):
    """Copy a given kNNClassifer by constructing a new one with identical
parameters.

    *original*
      The classifier to be copied

    *k*
      If the copy shall have another k-value as the original, set k accordingly.
      k = 0 means, that the original's k-value will be used"""
    if k == 0:
        k = original.num_k
    return kNNInteractive(
                list(original.get_glyphs()), original.features,
                original._perform_splits,
                k)

def _getMainId(classificationResult):
    """Classification results returned from a kNN Classifier are in a list
containing '(confidence, className)' tuples. So to determine the 'main class',
the classname with the highest confidence has to be returned.
"""
    # classificationResult is a tuple of the form
    #   ([idname1, idname2, ...], confidences_for_idname1)
    return classificationResult[0][0][1]

class AlgoRegistry(object):
    """Registry containing a list of all available editing algorithms. Besides
the callable itself, the registry also stores its docstring, a displayname and
type-information about any additionally required arguments. This information
will be used by the gui to show a dialog to execute any of the available
algorithms on a classifier.

An editing algorithm is any callable object, that takes at least one parameter
-  a *gamera.knn.kNNInteractive* classifier - and returns a new edited
*kNNInteractive* classifier.

To add your own algorithm, use one of the *register()* methods or alternatively,
let your callable inherit from *EditingAlgorithm* to register your algorithm,"""
    algorithms = []

    def registerData(algoData):
        """Register a new editing Algorithm using metadata from an
*AlgoData* object"""
        AlgoRegistry.algorithms.append(algoData)
    registerData = staticmethod(registerData)

    def register(name, callable, args = Args(), doc = ""):
        """Register a new editing Algorithm: The parameters are the same as in
*AlgoData.__init__*, so see its doc for an explanation of the parameters."""
        AlgoRegistry.registerData(AlgoData(name, callable, args, doc))
    register = staticmethod(register)

class AlgoData(object):
    """Class holding all metadata about an editing algorithm, that is required by
*AlgoRegistry*

*name*
    Name of the algorithm

*callable*
    The callable object implementing the algorithm. Its first parameter has to
    be a *kNNInteractive* Classifier and it has to return a new *kNNInteractive*
    classifier. If the algorithm requires any additional parameters, they have
    to be specified in the *args* parameter

*args*
    A *gamera.args.Args* object specifying any additional parameters required by
    the algorithm

*doc*
    A docstring in reStructured Text format, describing the algorithm and its
    parameters"""
    def __init__(self, name, callable, args = Args(), doc = ""):
        self.name = name
        self.callable = callable
        self.args = args
        self.doc = doc


class EditingAlgorithm(object):
    """Convenience class to automatically register editing algorithms with the
*AlgoRegistry*. If you implement your algorithm as a callable class, you can
just inherit from this class to let it automatically be registered. Just add
two properties to the class:

*name*
    The name of your algorithm
*args*
    Type info about any additional required arguments (a *gamera.args.Args* object)
"""
    def __init__(self):
        AlgoRegistry.register(self.name, self, self.args, self.__doc__)

class EditMnn(EditingAlgorithm):
    """**edit_mnn** (kNNInteractive *classifier*, int *k* = 0, bool *protectRare*, int *rareThreshold*)
    
Wilson's *Modified Nearest Neighbour* (MNN, aka *Leave-one-out-editing*).
The algorithm removes 'bad' glyphs from the classifier, i.e. glyphs that
are outliers from their class in featurespace, usually because they have been 
manually misclassified or are not representative for their class

    *classifier*
        The classifier from which to create an edited copy
    *internalK*
        The k value used internally by the editing algorithm. If 0 is given for 
        this parameter, the original classifier's k is used (recommended).        
    *protect rare classes*
        The algorithm tends to completely delete the items of rare classes,
        removing this whole class from the classifier. If this is not 
        desired these rare classes can be explicitly protected from deletion.
        Note that enabling this option causes additional computing effort    
    *rare class threshold*
        In case *protect rare classes* is enabled, classes with less than this
        number of elements are considered to be rare

Reference: D. Wilson: 'Asymptotic Properties of NN Rules Using Edited Data'.
*IEEE Transactions on Systems, Man, and Cybernetics*, 2(3):408-421, 1972
"""
    name = "Wilson's Modified Nearest Neighbour (MNN)"
    args = Args([Int("Internal k", default = 0),
                 Check("Protect rare classes", default = True),
                 Int("Rare class threshold", default = 3)])

    def __call__(self, classifier, k = 0, protectRare = True,
                 rareThreshold = 3):

        editedClassifier = _copyClassifier(classifier, k)
        toBeRemoved = set()
        progress = ProgressFactory("Generating edited MNN classifier...",
                                      len(classifier.get_glyphs()))

        # classify each glyph with its leave-one-out classifier
        for i, glyph in enumerate(classifier.get_glyphs()):
            editedClassifier.get_glyphs().remove(glyph)
            detectedClass = _getMainId(\
                                editedClassifier.guess_glyph_automatic(glyph))
            # check if recognized class complies with the true class
            if glyph.get_main_id() != detectedClass:
                toBeRemoved.add(glyph)
            editedClassifier.get_glyphs().add(glyph)
            progress.step()

        rareClasses = self._getRareClasses(classifier.get_glyphs(),
                                           protectRare, rareThreshold)
        
        # remove 'bad' glyphs, if they are not in a rare class
        for glyph in toBeRemoved:
            if glyph.get_main_id() in rareClasses:
                continue
            editedClassifier.get_glyphs().remove(glyph)
            
        progress.kill()
        return editedClassifier
    
    def _getRareClasses(self, glyphs, protectRare, rareThreshold):
        """Produces a set containing the names of all rare classes"""
        result = set()
        
        if not protectRare: 
            return result
        
        # histogram of classNames
        histo = {}        
        for g in glyphs:
            count = histo.get(g.get_main_id(), 0)
            histo[g.get_main_id()] = count + 1
        
            
        for className in histo:
            if histo[className] < rareThreshold:
                result.add(className)        
        return result

edit_mnn = EditMnn()    

class EditCnn(EditingAlgorithm):
    """**edit_cnn** (kNNInteractive *classifier*, int *k* = 0, bool *randomize*)
    
Hart's *Condensed Nearest Neighbour (CNN)* editing.
This alorithm is specialized in removing superfluous glyphs - glyphs that do not
influence the recognition rate - from the classifier to improve its 
classification speed. Typically glyphs far from the classifiers decision 
boundaries are removed.

    *classifier*
        The classifier from which to create an edited copy
    *internalK*
        The k value used internally by the editing algorithm. 0 means, use the 
        same value as the given classifier (recommended)    
    *randomize*
        Because the processing order of the glyphs in the classifier impacts the
        result of this algorithm, the order will be randomized. If reproducable
        results are required, turn this option off.

Reference: P.E. Hart: 'The Condensed Nearest Neighbor rule'. *IEEE Transactions on Information Theory*, 14(3):515-516, 1968
"""
    name = "Hart's Condensed Nearest Neighbour (CNN)"
    args = Args([Int("Internal k", default=0), 
                 Check("Randomize", default = True)])

    def __call__(self, classifier, k = 0, randomize = True):
        # special case of empty classifier
        if (not classifier.get_glyphs()):
            return _copyClassifier(classifier)

        if k == 0:
            k = classifier.num_k
        
        progress = ProgressFactory("Generating edited CNN classifier...",
                                      len(classifier.get_glyphs()))

        # initialize Store (a) with a single element
        if randomize:
            elem = _randomSetElement(classifier.get_glyphs())
        else:
            elem = classifier.get_glyphs().__iter__().next()
        
        aGlyphs = [elem]
        a = kNNInteractive(aGlyphs, classifier.features, 
                           classifier._perform_splits, k)
        progress.step()
        
        # initialize Grabbag (b) with all other
        b = classifier.get_glyphs().copy()
        b.remove(aGlyphs[0]);

        # Classify each glyph in b with a as the classifier
        # If glyph is misclassified, add it to a, repeat until no elements are 
        # added to a
        changed = True
        while changed == True:
            changed = False
            # copy needed because iteration through dict is not possible while 
            # deleting items from it
            copyOfB = b.copy()
            for glyph in copyOfB:
                if glyph.get_main_id() != _getMainId(a.guess_glyph_automatic(glyph)):
                    b.remove(glyph)
                    a.get_glyphs().add(glyph)
                    progress.step()
                    changed = True
        progress.kill()
        a.num_k = 1
        return a

edit_cnn = EditCnn()

class EditMnnCnn(EditingAlgorithm):
    """**edit_mnn_cnn** (kNNInteractive *classifier*, int *k* = 0, bool *protectRare*, int *rareThreshold*, bool *randomize*)

Combined execution of Wilson's Modified Nearest Neighbour and Hart's
Condensed Nearest Neighbour. Combining the algorithms in this order is 
recommended, because first bad samples are removed to improve the classifiers 
accuracy, and then the remaining samples are condensed to speed up the classifier

For documentation of the parameters see the independent algorithms"""
    name = "MNN, then CNN"
    args = Args([Int("Internal k", default = 0),
                 Check("Protect rare classes", default = True),
                 Int("Rare class threshold", default = 3),
                 Check("Randomize", default = True)])

    def __call__(self, classifier, k = 0, protectRare = True,
                 rareThreshold = 3, randomize = True):
        return edit_cnn(edit_mnn(classifier, k, protectRare, rareThreshold),
                   randomize)

edit_mnn_cnn = EditMnnCnn()
