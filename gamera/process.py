# vi:set tabsize=3:

#

# Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom,

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



import os.path, sys, cStringIO

from util import Set

from dis import dis

from inspect import getmodule

from types import *



from args import *



# Finds the member variables that a method accesses and sets

def find_method_dependencies(method):

   buffer = cStringIO.StringIO()

   stdout = sys.stdout

   sys.stdout = buffer

   dis(method)

   bytecodes = [x.split() for x in buffer.getvalue().split("\n")]

   sys.stdout = stdout

   buffer.close()



   sets = Set()

   requires = Set()

   self = 0

   for bytecode in bytecodes:

      if len(bytecode) > 3:

         if self:

            if bytecode[1] == "STORE_ATTR":

               sets.append(bytecode[3][1:-1])

            elif bytecode[1] == "LOAD_ATTR":

               requires.append(bytecode[3][1:-1])

            self = 0

         elif (len(bytecode) > 3 and bytecode[1] == "LOAD_FAST" and

               bytecode[2] == "0"):

            self = 1

   requires = filter(lambda x: x not in sets, requires)

   if sets == [] and requires == []:

      print ("WARNING: %s appears to have no side effects." 

             "What are you, some kind of functional programmer? ;)" %

             method.__name__)

   return sets, requires



# Generic base class for high-level processes.

# Manages saving/loading of data necessary to perform some/all of a process.

class Process:

   def __init__(self, base_filename, display_only=0):

      self.base_filename = os.path.splitext(os.path.abspath(base_filename))[0]

      self.display_only = display_only

      self.current_step = 0



   def get_step_number(self, step, default = 0):

      if step == None:

         return default

      elif type(step) == StringType and step in self.steps:

         return self.steps.index(step)

      elif type(step) == IntType and step < len(self.steps):

         return step

      raise KeyError("%s does not identify a step in the process." % step)



   def get_filename(self, extension):

      # TODO: change so this returns a unique filename

      return self.base_filename + "." + extension



   def process(self, first_step=None, last_step=None, save_members=None):

      first_step = self.get_step_number(first_step)

      last_step = self.get_step_number(last_step, len(self.steps) - 1)



      for step in self.steps[first_step:last_step + 1]:

         if (step[:5] == "DEBUG" and step != self.steps[last_step]):

            continue

         self.__do_step(step)

      if save_members == None:

         for saveable in self.saveable_members():

            if (self.__dict__.has_key(saveable)):

               self.__save_attr(saveable)

      else:

         for save in save_members:

            self.__save_attr(save)



   def next(self):

      if self.current_step >= len(self.steps):

         return -1

      sets, requires = self.do_step(self.current_step)

      self.__save_attr(sets)

      self.current_step += 1



   def saveable_members(self):

      return [x[5:] for x in self.__class__.__dict__.keys()

              if x[:5] == "save_"]



   def __do_step(self, step):

      step_no = self.get_step_number(step)

      function = getattr(self, step)

      sets, requires = find_method_dependencies(function)

      for load in requires:

         if not hasattr(self, load):

            self.__load_attr(load, step)

      print "Step %d: %s" % (step_no, step)

      if not self.display_only:

         function()

      return sets, requires



   def __load_attr(self, load, step="unknown"):

      if not hasattr(self, "load_" + load):

         raise RuntimeError("The %s step requires the variable '%s'. "

                            "However, there is no load_%s function defined."

                            % (step, load, load))

      else:

         print "Loading %s" % load

         if not self.display_only:

            getattr(self, "load_" + load)()



   def __save_attr(self, save):

      if not hasattr(self, save):

         print ("WARNING: %s is not defined, therefore it can not be saved."

                % save)

      if not hasattr(self, "save_" + save):

         raise RuntimeError("The Gamera process system wants to save %s, "

                            "but there is no save_%s function defined."

                            % (save, save))

      else:

         print "Saving %s" % save

         if not self.display_only:

            getattr(self, "save_" + save)()



   steps = []





class ProcessWizard(Wizard):

   def __init__(self, shell, locals, process):

      self.shell = shell

      self.parent = None

      self.locals = locals

      self.process = process

      self.caption = self.process.__name__ + " process"

      self.init_args = ()



      self.dlg_select_steps = Args(

          [Radio("Perform the entire process", "All"),

           Radio("Perform part of the process", "Custom"),

           Choice("   Starting step", self.process.steps),

           Choice("   Ending step", self.process.steps, -1)],

#          caption=self.caption,

          function='cb_select_steps',

          title=('Select which steps of the %s process you would like to perform.' % self.process.__name__))



      if hasattr(self, 'dlg_init'):

         self.show(self.dlg_init)

      else:

         self.show(self.dlg_select_steps)



   def cb_init(self, *args):

      self.init_args = args

      return self.dlg_select_steps



   def cb_select_steps(self, all, custom, start_step, end_step):

      if all:

         start_step = 0

         end_step = len(self.process.steps) - 1

         self.start_step = start_step

         self.end_step = end_step

         self.saves = None

         return None

      elif custom:

         if start_step > end_step:

            end_step, start_step = start_step, end_step

         self.start_step = start_step

         self.end_step = end_step

         saves = Set()

         for step in range(start_step, end_step + 1):

            deps = find_method_dependencies(

                getattr(self.process, self.process.steps[step]))

            saves.extend(deps[0])

            saves.extend(deps[1])

         args = []

         self.saveables = []

         for step in saves:

            if hasattr(self.process, "save_" + step):

               args.append(Check("", step, 1))

               self.saveables.append(step)



         self.dlg_select_saves = Args(

             args,

#             caption=self.caption,

             title="Select which members to save.",

             function="cb_select_saves")

         return self.dlg_select_saves



   def cb_select_saves(self, *args):

      self.saves = []

      for x in range(len(args)):

         if args[x]:

            self.saves.append(self.saveables[x])



   def done(self):

      self.shell.run("%s.%s%s.process(%s, %s, %s)" %

                     (getmodule(self.process).__name__.split('.')[-1],

                      self.process.__name__,

                      self.init_args,

                      repr(self.process.steps[self.start_step]),

                      repr(self.process.steps[self.end_step]),

                      repr(self.saves)))
