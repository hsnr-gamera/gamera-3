// $Header$
/* ----------------------------------------------------------------------------
  string.C
  mbwall 21mar95
  Copyright (c) 1995-1996 Massachusetts Institute of Technology
                          all rights reserved

 DESCRIPTION:
   Source file for the string specialization of the array genome.
---------------------------------------------------------------------------- */
#include <ga/GAStringGenome.h>

const char * 
GA1DArrayAlleleGenome<char>::className() const {return "GAStringGenome";}
int GA1DArrayAlleleGenome<char>::classID() const {return GAID::StringGenome;}

GA1DArrayAlleleGenome<char>::
GA1DArrayAlleleGenome(unsigned int length, const GAAlleleSet<char> & s,
		      GAGenome::Evaluator f, void * u) :
GA1DArrayGenome<char>(length, f, u){
  naset = 1;
  aset = new GAAlleleSet<char>[1];
  aset[0] = s;

  initializer(DEFAULT_STRING_INITIALIZER);
  mutator(DEFAULT_STRING_MUTATOR);
  comparator(DEFAULT_STRING_COMPARATOR);
  crossover(DEFAULT_STRING_CROSSOVER);
}

GA1DArrayAlleleGenome<char>::
GA1DArrayAlleleGenome(const GAAlleleSetArray<char> & sa,
		      GAGenome::Evaluator f, void * u) :
GA1DArrayGenome<char>(sa.size(), f, u){
  naset = sa.size();
  aset = new GAAlleleSet<char>[naset];
  for(int i=0; i<naset; i++)
    aset[i] = sa.set(i);

  initializer(DEFAULT_STRING_INITIALIZER);
  mutator(DEFAULT_STRING_MUTATOR);
  comparator(DEFAULT_STRING_COMPARATOR);
  crossover(DEFAULT_STRING_CROSSOVER);
}

GA1DArrayAlleleGenome<char>::~GA1DArrayAlleleGenome(){
  delete [] aset;
}


#ifndef NO_STREAMS
// The read specialization takes in each character whether it is whitespace or
// not and stuffs it into the genome.  This is unlike the default array read.
int
GA1DArrayAlleleGenome<char>::read(istream & is)
{
  unsigned int i=0;
  char c;
  do{
    is.get(c);
    if(!is.fail()) gene(i++, c);
  } while(!is.fail() && !is.eof() && i < nx);

  if(is.eof() && i < nx){
    GAErr(GA_LOC, className(), "read", gaErrUnexpectedEOF);
    is.clear(ios::badbit | is.rdstate());
    return 1;
  }
  return 0;
}

// Unlike the base array genome, here when we write out we don't put any
// whitespace between genes.  No newline at end of it all.
int
GA1DArrayAlleleGenome<char>::write(ostream & os) const
{
  for(unsigned int i=0; i<nx; i++)
    os << gene(i);
  return 0;
}
#endif



// These must be included _after_ the instantiations because some compilers get
// all wigged out about the declaration/specialization order.  Note that some
// compilers require a syntax different than others when forcing the 
// instantiation (i.e. GNU wants the 'template class', borland does not).
#ifndef USE_AUTO_INST
#include <ga/GAAllele.C>
#include <ga/GA1DArrayGenome.C>

#if defined(__BORLANDC__)
GAAlleleSet<char>;
GAAlleleSetCore<char>;
GAAlleleSetArray<char>;

GAArray<char>;
GA1DArrayGenome<char>;
GA1DArrayAlleleGenome<char>;
#else
template class GAAlleleSet<char>;
template class GAAlleleSetCore<char>;
template class GAAlleleSetArray<char>;

template class GAArray<char>;
template class GA1DArrayGenome<char>;
template class GA1DArrayAlleleGenome<char>;
#endif
#endif
