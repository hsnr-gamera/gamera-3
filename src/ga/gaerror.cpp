// $Header$
/* ----------------------------------------------------------------------------
  error.C
  mbwall 28jul94
  Copyright (c) 1995 Massachusetts Institute of Technology
                     all rights reserved

 DESCRIPTION:
  This file contains all of the error messages for the library.
---------------------------------------------------------------------------- */
#include <string.h>
#include <ga/gaerror.h>
#include <stdio.h>

char gaErrMsg[512];
char _gaerrbuf1[120];
char _gaerrbuf2[120];


#ifndef NO_STREAMS
static ostream *__gaErrStream = &cerr;
#endif
static GABoolean __gaErrFlag = gaTrue;
static char *__gaErrStr[] = {
  (char *)"error reading from file: ",
  (char *)"error writing to file: ",
  (char *)"unexpected EOF encountered during read.",
  (char *)"bad probability value.  Must be between 0.0 and 1.0, inclusive.",
  (char *)"objects are different types.",
  (char *)"this method has not been defined.",
  (char *)"core deleted with references remaining.",

  (char *)"the custom replacement strategy requires a replacement function",
  (char *)"unknown replacement strategy",
  (char *)"number of children must be greater than 0",
  (char *)"replacement percentage must be between 0.0 and 1.0, inclusive",
  (char *)"number of indiv for replacement must be less than pop size",
  (char *)"index of individual is out-of-bounds",
  (char *)"population contains no individuals from which to clone new individuals",
  (char *)"there must be at least one individual in each population",
  (char *)"no sexual crossover has been defined.  no mating can occur.",
  (char *)"no asexual crossover has been defined.  no mating can occur.",

  (char *)"children must have same resize behaviour for any given dimension",
  (char *)"parents and children must have the same dimensions",
  (char *)"parents must be the same length",
  (char *)"upper limit must be greater than lower limit.",
  (char *)"bad phenotype - ID is out of bounds.",
  (char *)"bad phenotype - value is less than min or greater than max.",
  (char *)"dimensions of bounds set do not match dimensions of the genome",

  (char *)"linear scaling multiplier must be greater than 1.0",
  (char *)"sigma truncation multiplier must be greater than 0.0",
  (char *)"negative objective function score!\n\
    all raw objective scores must be positive for linear scaling.",
  (char *)"negative objective function score!\n\
    all raw objective scores must be positive for power law scaling.",
  (char *)"the cutoff for triangular sharing must be greater than 0.0",

  (char *)"cannot index an allele in a bounded, non-discretized set of alleles",
  (char *)"length of binary string exceeds maximum for this computer/OS type.",
  (char *)"specified value cannot be exactly represented with these bits.",
  (char *)"bad 'where' indicator",
  (char *)"bogus type, data may be corrupt",
  (char *)"bad links in tree.  operation aborted.",
  (char *)"cannot swap a node with its ancestor",
  (char *)"cannot insert this object into itself",
  (char *)"node relative to which insertion is made must be non-NULL.",
  (char *)"root node must have no siblings.  insertion aborted.",
  (char *)"cannot insert before a root node (only below).",
  (char *)"cannot insert after a root node (only below)."
};

void
GAErr(const GASourceLocator loc, const char *clss, const char *func,
      const char *msg1, const char *msg2, const char *msg3){
  gaErrMsg[0] = '\0';
  strcat(gaErrMsg, clss);
  strcat(gaErrMsg, "::");
  strcat(gaErrMsg, func);
  strcat(gaErrMsg, ":\n  ");
  strcat(gaErrMsg, msg1);
  strcat(gaErrMsg, "\n");
  if(msg2){
    strcat(gaErrMsg, "  ");
    strcat(gaErrMsg, msg2);
    strcat(gaErrMsg, "\n");
  }
  if(msg3){
    strcat(gaErrMsg, "  ");
    strcat(gaErrMsg, msg3);
    strcat(gaErrMsg, "\n");
  }
  sprintf(_gaerrbuf1, "  %s : %ld\n", loc.file, loc.line);
  strcat(gaErrMsg, _gaerrbuf1);
#ifndef NO_STREAMS
  if(__gaErrFlag == gaTrue) *__gaErrStream << gaErrMsg;
#endif
}

void
GAErr(const GASourceLocator loc, const char *clss, const char *func,
      GAErrorIndex i, const char *msg2, const char *msg3){
  gaErrMsg[0] = '\0';
  strcat(gaErrMsg, clss);
  strcat(gaErrMsg, "::");
  strcat(gaErrMsg, func);
  strcat(gaErrMsg, ":\n  ");
  strcat(gaErrMsg, __gaErrStr[i]);
  strcat(gaErrMsg, "\n");
  if(msg2){
    strcat(gaErrMsg, "  ");
    strcat(gaErrMsg, msg2);
    strcat(gaErrMsg, "\n");
  }
  if(msg3){
    strcat(gaErrMsg, "  ");
    strcat(gaErrMsg, msg3);
    strcat(gaErrMsg, "\n");
  }
  sprintf(_gaerrbuf1, "  %s : %ld\n", loc.file, loc.line);
  strcat(gaErrMsg, _gaerrbuf1);
#ifndef NO_STREAMS
  if(__gaErrFlag == gaTrue) *__gaErrStream << gaErrMsg;
#endif
}

void
GAErr(const GASourceLocator loc, const char *func,
      GAErrorIndex i, const char *msg2, const char *msg3){
  gaErrMsg[0] = '\0';
  strcat(gaErrMsg, func);
  strcat(gaErrMsg, ":\n  ");
  strcat(gaErrMsg, __gaErrStr[i]);
  strcat(gaErrMsg, "\n");
  if(msg2){
    strcat(gaErrMsg, "  ");
    strcat(gaErrMsg, msg2);
    strcat(gaErrMsg, "\n");
  }
  if(msg3){
    strcat(gaErrMsg, "  ");
    strcat(gaErrMsg, msg3);
    strcat(gaErrMsg, "\n");
  }
  sprintf(_gaerrbuf1, "  %s : %ld\n", loc.file, loc.line);
  strcat(gaErrMsg, _gaerrbuf1);
#ifndef NO_STREAMS
  if(__gaErrFlag == gaTrue) *__gaErrStream << gaErrMsg;
#endif
}


void
GAReportErrors(GABoolean flag){
  __gaErrFlag = flag;
}

#ifndef NO_STREAMS
void
GASetErrorStream(ostream& s){
  __gaErrStream = &s;
}
#endif
