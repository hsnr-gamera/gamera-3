// $Header$
/* ----------------------------------------------------------------------------
  config.h
  mbwall 27jun95
  Copyright (c) 1995-1996 Massachusetts Institute of Technology
                          all rights reserved
  Copyright (c) 1998-1999 Matthew Wall
                          all rights reserved
---------------------------------------------------------------------------- */
#ifndef _ga_config_h_
#define _ga_config_h_


/* ----------------------------------------------------------------------------
PREPROCESSOR DIRECTIVES

  Here are the preprocessor directives that the library understands.  If you 
are using a makefile, then put these in the line with the DEFINES macro.  For
example, to define the NO_TEMPLATES and USE_BORLAND_INST options then do

       DEFINES = -DNO_TEMPLATES -DUSE_BORLAND_INST

In MacOS or DOS, use your development environment's preprocessor directives 
option to set the values.  Beware that if you use the makefile or your 
development environment to do the defines then someone may compile a program
later on that does not use the same defines you used to compile the library.
  For best results (ie smallest chance of error), modify this header file 
rather than using the makefile or development environment.
  Some of these are already set up for the OSes with which I am familiar.  See
below for the pre-defined sets.  If you come up with a compiler/platform 
configuration that is not listed here, please send it to me so that i can 
incorporate it into the code base.


   USE_ANSI_STREAMS   For systems/environments in which streams are not desired
   USE_OLD_STREAMS    and/or required.  Turns off errors and all of the read/
   NO_STREAMS         write routines for the classes when neither of these is
		      defined or when NO_STREAMS is defined.

   USE_CPP_CASTS      If your compiler supports RTTI, or if you turn on
                      the RTTI abilities of your compiler, then define this
                      macro.  Without RTTI, if improper casts are made,
		      things will die horribly rather than dropping out in an
		      RTTI-induced exception.

   USE_PID            Define this if the system has a getpid function that
                      returns something sane and useful.

   NO_TEMPLATES       For compilers that do not do templates.  The only type
                      of genome available when this is defined is binary 
                      string and any derived classes.  list, tree, and array 
		      all use templates.  You can still use the template code,
		      but you will have to hack it yourself to make it work.

   USE_BORLAND_INST   For compilers that use the Borland instantiation model.
                      These compilers expect all of the template code to be
		      in the header file.  The Cfront model, on the other
                      hand, expects source files with names similar to the
		      header files, but all of the template code does not
		      need to be in the header files.
		    
		      When you define this flag, the source file that 
		      corresponds to the header file is explicitly included
		      at the end of the header file for all headers that
		      contain templates.

   USE_AUTO_INST      For compilers that do not do automatic instantiation
                      (such as g++ version 2.6.8) you will have to force
                      instantiations.  When this flag is not defined, GAlib
		      forces an instantiation of all of the template classes
		      that it uses (such as real genome and string genome).

   USE_GALIB_AS_LIB      For windows shared libraries, one must define whether
   USE_GALIB_AS_DLL      static member data are imported or exported.  You 
                         define one or the other of these, but not both.  The
		  	 default is USE_GALIB_AS_LIB (if you define neither).
			
   COMPILE_GALIB_AS_LIB  If you are compiling the dome library, define one of
   COMPILE_GLAIB_AS_DLL  these to indicate the windows exports.  The default
                         is USE_GALIB_AS_LIB (if you define neither).
 
   
   
             
   
   USE_RAN1           These specify which random number function to use.  Only
   USE_RAN2           one of these may be specified.  You may have to tweak 
   USE_RAN3           random.h a bit as well (these functions are not defined 
   USE_RAND 	      the same way on each platform).  For best results, use
   USE_RANDOM	      ran2 or ran3 (performance is slightly slower than most
   USE_RAND48	      system RNGs, but you'll get better results).

                      If you want to use another random number generator you
                      must hack random.h directly (see the comments in that
                      file).

   BITBASE            The built-in type to use for bit conversions.  This 
                      should be set to the type of the largest integer that
                      your system supports.  If you have long long int then
                      use it.  If you don't plan to use more than 16 or 32
                      bits to represent your binary-to-decimal mappings then
                      you can use something smaller (long int for example).
                      If you do not set this, GAlib will automatically use
                      the size of a long int.  The bitbase determines the
		      maximum number of bits you can use to represent a
		      decimal number in the binary-to-decimal genomes.

   BITS_IN_WORD       How many bits are in a word?  For many systems, a word is
                      a char and is 8 bits long.

---------------------------------------------------------------------------- */

// By default, we use the old streams library.  
#if !defined(USE_OLD_STREAMS) && \
    !defined(USE_ANSI_STREAMS) && \
    !defined(NO_STREAMS)
#define USE_ANSI_STREAMS
#endif






// ----------------------------------------------------------------------------
// Here are the defines needed for some of the compilers/OSes on which I have
// been able to test the GA library.  You may have to remove and/or modify
// these to get things to work on your system.
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Metrowerks' Codewarrior for MacOS, PalmOS, or Win32 (I have not tested CW
// on other platforms yet).
#if defined(__MWERKS__)
#if __option(RTTI)
#define USE_CPP_CASTS
#endif
#define USE_BORLAND_INST
#define USE_AUTO_INST


// ----------------------------------------------------------------------------
// Symantec C++ for mac.  This compiler does not handle templates very well, 
// so if you want to use any of the template components of GAlib then you will
// probably have to do some hacking to get things to work.
#elif defined(__SC__)
#define USE_BORLAND_INST


// ----------------------------------------------------------------------------
// borland c++ compiler
//
// You may or may not need the BORLAND_INST flag defined when you use a borland
// compiler.  I did not need it when I compiled using version 4.0, but I did
// need it when I compiled with an earlier version (I think it was 3.x but I
// do not remember for certain).
//   Note that the default random number generator when using a borland (or
// any PC compiler, for that matter) is the basic system's RNG.
// I did this because of the hassles of 16- vs 32-bit DOS/Windows rubbish.  If
// you want a better RNG, you can use the others in GAlib, but you'll have to
// do a bit of checking to make sure it works with your DOS/Windows config.
// All of the RNGs work fine under all of the 32-bit OSes I've tried, but they
// don't do so well in a 16-bit OS.
//  Use the randtest example to check GAlib's RNG after you compile everything.
#elif defined(__BORLANDC__)
#define USE_RAND		// comment this if you're using a 32-bit OS
//#define USE_BORLAND_INST


// ----------------------------------------------------------------------------
// MicroSoft's Visual C++ programming environment.
#elif defined(_MSC_VER)
#if defined(_CPPRTTI)
#define USE_CPP_CASTS
#endif
#define USE_BORLAND_INST
#define USE_AUTO_INST
//#pragma warning (disable : 4244)
#pragma warning (disable : 4305)    // ignore double-to-float warnings
#pragma warning (disable : 4355)    // allow us to use this in constructors
//#pragma warning (disable : 4250)    // ignore the dominated multiple inherits


// ----------------------------------------------------------------------------
// for g++ 2.6.3 - 2.8.x
// if you use 2.8.x then you might want uncomment the USE_CPP_CASTS macro
// since 2.8 will do rtti without requiring the -frtti flag.
#elif defined(__GNUG__)
#define USE_PID
#define USE_BORLAND_INST
//#define USE_CPP_CASTS


// ----------------------------------------------------------------------------
// irix 5.3 and irix 6.x
#elif defined(__sgi)
#define USE_PID
#include <sgidefs.h>
#if (_MIPS_SIM == _MIPS_SIM_NABI32)
#define USE_CPP_CASTS
#elif (_MIPS_SIM == _MIPS_SIM_ABI64)
#define USE_CPP_CASTS
#elif (_MIPS_SIM == _MIPS_SIM_ABI32)
#define USE_AUTO_INST
#endif


// ----------------------------------------------------------------------------
// This is an unknown/untested platform and/or compiler.  The defaults below 
// might work for you, but then again, they might not.  You may have to adjust
// the values of the macros until everything works properly.  Comment out the
// #error directive to allow things to compile properly.  Eventually I'll get
// areound to replacing this with autoconf...
#else
#error   Unknown/untested compiler/operating system!  Check these settings!

#define USE_CPP_CASTS
#define USE_BORLAND_INST
#define USE_AUTO_INST
#define USE_PID
#endif















// Use the right streams library based on which streams macro was defined.
#if defined(USE_OLD_STREAMS)
#include <iostream.h>
#include <fstream.h>

#elif defined(USE_ANSI_STREAMS)
#include <iostream>
#include <fstream>

// i'm terribly sorry to do this, but it is the easiest way for me to get
// things to work properly with vcpp.
using namespace std;

#else
#ifndef NO_STREAMS
#define NO_STREAMS
#endif
#endif


// If no RNG has been selected, use the ran2 generator by default
#if !defined(USE_RAND) && !defined(USE_RANDOM) && \
    !defined(USE_RAND48) && !defined(USE_RAN2) && !defined(USE_RAN3)
#define USE_RAN2
#endif


// This defines how many bits are in a single word on your system.  Most 
// systems have a word length of 8 bits.
#ifndef BITS_IN_WORD
#define BITS_IN_WORD 8
#endif


// Use this to set the maximum number of bits that can be used in binary-to-
// decimal conversions.  You should make this type the largest integer type 
// that your system supports.
#ifndef BITBASE
#define BITBASE long int
#endif


// If the system/compiler understands C++ casts, then we use them.  Otherwise
// we default to the C-style casts.  The macros make explicit the fact that we
// are doing casts.
#if defined(USE_CPP_CASTS)
#define DYN_CAST(type,x) (dynamic_cast<type>(x))
#define CON_CAST(type,x) (const_cast<type>(x))
#define STA_CAST(type,x) (static_cast<type>(x))
#define REI_CAST(type,x) (reinterpret_cast<type>(x))
#else
#define DYN_CAST(type,x) ((type)(x))
#define CON_CAST(type,x) ((type)(x))
#define STA_CAST(type,x) ((type)(x))
#define REI_CAST(type,x) ((type)(x))
#endif


// Windows is brain-dead about how to export things, so we do this to keep the
// code (somewhat) cleaner but still accomodate windows' stupidity.
#if defined(COMPILE_GALIB_AS_DLL)
#define GA_DLLDECL __declspec(dllexport)
#elif defined(USE_GALIB_AS_DLL)
#define GA_DLLDECL __declspec(dllimport)
#else
#define GA_DLLDECL
#endif






/* ----------------------------------------------------------------------------
SPACE SAVERS and DEFAULT OPERATORS

  These directives determine which operators will be used by default for each
of the objects in GAlib.
  If space is limited, you may want to compile the library with only the parts
that you need (compiling in DOS comes to mind).  Your compiler should do this
automatically for you (ie only use the parts that you use).  If not, then 
comment out the chunks in the files you're not going to use (for example, 
comment out the ordered initializer in the list object).
  To disable a certain type of genome, simply don't compile its source file.
The following directives are defined so that you can trim out the parts of the
genetic algorithm objects that are not in separate files.
---------------------------------------------------------------------------- */
// scaling schemes
#define USE_LINEAR_SCALING           1
#define USE_SIGMA_TRUNC_SCALING      1
#define USE_POWER_LAW_SCALING        1
#define USE_SHARING                  1

// selection schemes
#define USE_RANK_SELECTOR            1
#define USE_ROULETTE_SELECTOR        1
#define USE_TOURNAMENT_SELECTOR      1
#define USE_DS_SELECTOR              1
#define USE_SRS_SELECTOR             1
#define USE_UNIFORM_SELECTOR         1

// These are the compiled-in defaults for various genomes and GA objects
#define DEFAULT_SCALING              GALinearScaling
#define DEFAULT_SELECTOR             GARouletteWheelSelector
#define DEFAULT_TERMINATOR           TerminateUponGeneration

#define DEFAULT_1DBINSTR_INITIALIZER UniformInitializer
#define DEFAULT_1DBINSTR_MUTATOR     FlipMutator
#define DEFAULT_1DBINSTR_COMPARATOR  BitComparator
#define DEFAULT_1DBINSTR_CROSSOVER   OnePointCrossover
#define DEFAULT_2DBINSTR_INITIALIZER UniformInitializer
#define DEFAULT_2DBINSTR_MUTATOR     FlipMutator
#define DEFAULT_2DBINSTR_COMPARATOR  BitComparator
#define DEFAULT_2DBINSTR_CROSSOVER   OnePointCrossover
#define DEFAULT_3DBINSTR_INITIALIZER UniformInitializer
#define DEFAULT_3DBINSTR_MUTATOR     FlipMutator
#define DEFAULT_3DBINSTR_COMPARATOR  BitComparator
#define DEFAULT_3DBINSTR_CROSSOVER   OnePointCrossover

#define DEFAULT_BIN2DEC_ENCODER      GABinaryEncode
#define DEFAULT_BIN2DEC_DECODER      GABinaryDecode
#define DEFAULT_BIN2DEC_COMPARATOR   BitComparator

#define DEFAULT_1DARRAY_INITIALIZER  NoInitializer
#define DEFAULT_1DARRAY_MUTATOR      SwapMutator
#define DEFAULT_1DARRAY_COMPARATOR   ElementComparator
#define DEFAULT_1DARRAY_CROSSOVER    OnePointCrossover
#define DEFAULT_2DARRAY_INITIALIZER  NoInitializer
#define DEFAULT_2DARRAY_MUTATOR      SwapMutator
#define DEFAULT_2DARRAY_COMPARATOR   ElementComparator
#define DEFAULT_2DARRAY_CROSSOVER    OnePointCrossover
#define DEFAULT_3DARRAY_INITIALIZER  NoInitializer
#define DEFAULT_3DARRAY_MUTATOR      SwapMutator
#define DEFAULT_3DARRAY_COMPARATOR   ElementComparator
#define DEFAULT_3DARRAY_CROSSOVER    OnePointCrossover

#define DEFAULT_1DARRAY_ALLELE_INITIALIZER  UniformInitializer
#define DEFAULT_1DARRAY_ALLELE_MUTATOR      FlipMutator
#define DEFAULT_1DARRAY_ALLELE_COMPARATOR   ElementComparator
#define DEFAULT_1DARRAY_ALLELE_CROSSOVER    OnePointCrossover
#define DEFAULT_2DARRAY_ALLELE_INITIALIZER  UniformInitializer
#define DEFAULT_2DARRAY_ALLELE_MUTATOR      FlipMutator
#define DEFAULT_2DARRAY_ALLELE_COMPARATOR   ElementComparator
#define DEFAULT_2DARRAY_ALLELE_CROSSOVER    OnePointCrossover
#define DEFAULT_3DARRAY_ALLELE_INITIALIZER  UniformInitializer
#define DEFAULT_3DARRAY_ALLELE_MUTATOR      FlipMutator
#define DEFAULT_3DARRAY_ALLELE_COMPARATOR   ElementComparator
#define DEFAULT_3DARRAY_ALLELE_CROSSOVER    OnePointCrossover

#define DEFAULT_STRING_INITIALIZER   UniformInitializer
#define DEFAULT_STRING_MUTATOR       FlipMutator
#define DEFAULT_STRING_COMPARATOR    ElementComparator
#define DEFAULT_STRING_CROSSOVER     UniformCrossover

#define DEFAULT_REAL_INITIALIZER     UniformInitializer
#define DEFAULT_REAL_MUTATOR         GARealGaussianMutator
#define DEFAULT_REAL_COMPARATOR      ElementComparator
#define DEFAULT_REAL_CROSSOVER       UniformCrossover

#define DEFAULT_TREE_INITIALIZER     NoInitializer
#define DEFAULT_TREE_MUTATOR         SwapSubtreeMutator
#define DEFAULT_TREE_COMPARATOR      TopologyComparator
#define DEFAULT_TREE_CROSSOVER       OnePointCrossover

#define DEFAULT_LIST_INITIALIZER     NoInitializer
#define DEFAULT_LIST_MUTATOR         SwapMutator
#define DEFAULT_LIST_COMPARATOR      NodeComparator
#define DEFAULT_LIST_CROSSOVER       OnePointCrossover

#endif
