/*

 *

 * Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan

 *

 * This program is free software; you can redistribute it and/or

 * modify it under the terms of the GNU General Public License

 * as published by the Free Software Foundation; either version 2

 * of the License, or (at your option) any later version.

 *

 * This program is distributed in the hope that it will be useful,

 * but WITHOUT ANY WARRANTY; without even the implied warranty of

 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the

 * GNU General Public License for more details.

 * 

 * You should have received a copy of the GNU General Public License

 * along with this program; if not, write to the Free Software

 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

 */



/*

  Much of this code is a modified version of Vigra code and should be

  covered by the following copyright information:

*/



/************************************************************************/

/*                                                                      */

/*               Copyright 1998-2001 by Ullrich Koethe                  */

/*       Cognitive Systems Group, University of Hamburg, Germany        */

/*                                                                      */

/*    This file is part of the VIGRA computer vision library.           */

/*    ( Version 1.1.4, Nov 23 2001 )                                    */

/*    ( Version 1.1.4, Nov 23 2001 )                                    */

/*    You may use, modify, and distribute this software according       */

/*    to the terms stated in the LICENSE file included in               */

/*    the VIGRA distribution.                                           */

/*                                                                      */

/*    The VIGRA Website is                                              */

/*        http://kogs-www.informatik.uni-hamburg.de/~koethe/vigra/      */

/*    Please direct questions, bug reports, and contributions to        */

/*        koethe@informatik.uni-hamburg.de                              */

/*                                                                      */

/*  THIS SOFTWARE IS PROVIDED AS IS AND WITHOUT ANY EXPRESS OR          */

/*  IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED      */

/*  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. */

/*                                                                      */

/************************************************************************/





#ifndef kwm03072002_vigra_support

#define kwm03072002_vigra_support



#include "vigra/rgbvalue.hxx"

#include "vigra/accessor.hxx"

#include "vigra/interpolating_accessor.hxx"

#include "image_types.hpp"

#include "static_image.hpp"



using namespace vigra;



/*

  This is the necessary support to use the VIGRA library. Included are

  convenience functions to make the calling of the vigra library algorithms

  easiers and accessors for a variety of Gamera image types. The accessor

  also invert 0 and 1 from the Gamera standards for OneBitPixel for

  interoperability with vigra.

*/



namespace Gamera {



  template<class T>

  class Accessor {

  public:

    typedef T value_type;

    typedef T VALUETYPE;



    template <class ITERATOR>

    VALUETYPE operator()(ITERATOR const & i) const {

      return m_accessor(i);

    }

    

    template <class ITERATOR, class DIFFERENCE>

    VALUETYPE operator()(ITERATOR & i, DIFFERENCE diff) const

    {

      ITERATOR tmp = i + diff;

      return m_accessor(tmp);

    }



    template <class V, class ITERATOR>

    void set(V const & value, ITERATOR & i) const 

    {

      VALUETYPE tmp = vigra::detail::RequiresExplicitCast<VALUETYPE>::cast(value);

      m_accessor.set(tmp, i);

    }



    template <class V, class ITERATOR, class DIFFERENCE>

    void set(V const & value, ITERATOR & i, DIFFERENCE diff) const 

    { 

        VALUETYPE tmp = vigra::detail::RequiresExplicitCast<VALUETYPE>::cast(value); 

	ITERATOR tmpi = i + diff;

	m_accessor.set(tmp, tmpi);

    }



    ImageAccessor<T> m_accessor;

  };

  

  template <class SEQUENCE>

  class SequenceAccessor : public Gamera::Accessor<SEQUENCE> {

  public:

    typedef typename SEQUENCE::value_type component_type;

    typedef typename SEQUENCE::iterator iterator;



    template <class ITERATOR>

    iterator begin(ITERATOR & i) const { 

        return (*i).begin(); 

    }

    

    template <class ITERATOR>

    iterator end(ITERATOR & i)  const {

         return (*i).end(); 

    }



    template <class ITERATOR, class DIFFERENCE>

    iterator begin(ITERATOR & i, DIFFERENCE diff)  const { 

        return i[diff].begin(); 

    }

    

    template <class ITERATOR, class DIFFERENCE>

    iterator end(ITERATOR & i, DIFFERENCE diff)  const { 

        return i[diff].end(); 

    }



    template <class ITERATOR>

    int size(ITERATOR & i) const { return (*i).size(); }



    template <class ITERATOR, class DIFFERENCE>

    int size(ITERATOR & i, DIFFERENCE diff) const { return i[diff].size(); }

  };



  template <class VECTOR>

  class VectorAccessor : public Gamera::SequenceAccessor<VECTOR> {

  public:

    typedef typename VECTOR::value_type component_type;

    

    template <class ITERATOR>

    component_type getComponent(ITERATOR & i, int idx) const { 

        return (*i)[idx]; 

    }



    template <class V, class ITERATOR>

    void setComponent(V const & value, ITERATOR & i, int idx) const { 

        (*i)[idx] = detail::RequiresExplicitCast<component_type>::cast(value); 

    }



    template <class ITERATOR, class DIFFERENCE>

    component_type getComponent(ITERATOR & i, DIFFERENCE diff, int idx) const { 

        return i[diff][idx]; 

    }

    

    template <class V, class ITERATOR, class DIFFERENCE>

    void 

    setComponent(V const & value, ITERATOR & i, DIFFERENCE diff, int idx) const  { 

        i[diff][idx] = detail::RequiresExplicitCast<component_type>::cast(value); 

    }

  };



  template <class RGBVALUE>

  class RGBAccessor : public Gamera::VectorAccessor<RGBVALUE> {

  public:

    typedef typename RGBVALUE::value_type component_type;



    template <class RGBIterator>

    component_type red(RGBIterator & rgb) const {

      return (*rgb).red();

    }



    template <class V, class RGBIterator>

    void setRed(V value, RGBIterator & rgb) const {

      (*rgb).setRed(value);

    }

    

    template <class RGBIterator, class DIFFERENCE>

    component_type red(RGBIterator & rgb, DIFFERENCE diff) const {

      return rgb[diff].red();

    }



    template <class V, class RGBIterator, class DIFFERENCE>

    void setRed(V value, RGBIterator & rgb, DIFFERENCE diff) const {

      rgb[diff].setRed(value);

    }

       

    template <class RGBIterator>

    component_type green(RGBIterator & rgb) const {

      return (*rgb).green();

    }



    template <class V, class RGBIterator>

    void setGreen(V value, RGBIterator & rgb) const {

      (*rgb).setGreen(value);

    }



    template <class RGBIterator, class DIFFERENCE>

    component_type green(RGBIterator & rgb, DIFFERENCE d) const {

      return rgb[d].green();

    }



    template <class V, class RGBIterator, class DIFFERENCE>

    void setGreen(V value, RGBIterator & rgb, DIFFERENCE d) const {

      rgb[d].setGreen(value);

    }



    template <class RGBIterator>

    component_type blue(RGBIterator & rgb) const {

      return (*rgb).blue();

    }

    

    template <class V, class RGBIterator>

    void setBlue(V value, RGBIterator & rgb) const {

      (*rgb).setBlue(value);

    }



    template <class RGBIterator, class DIFFERENCE>

    component_type blue(RGBIterator & rgb, DIFFERENCE d) const {

      return rgb[d].blue();

    }

    

    template <class V, class RGBIterator, class DIFFERENCE>

    void setBlue(V value, RGBIterator & rgb, DIFFERENCE d) const {

      rgb[d].setBlue(value);

    }



//     template <class V1, class RGBIterator>

//     void set(RGBValue<V1> value, RGBIterator& rgb) const {

//       typedef typename RGBVALUE::value_type V;

//       (*rgb).setRGB(NumericTraits<V>::fromPromote(value.red()), 

// 		    NumericTraits<V>::fromPromote(value.green()), 

// 		    NumericTraits<V>::fromPromote(value.blue()));

//     }

  };



  class ComplexRealAccessor

  {

  public:

    

    /// The accessor's value type.

    typedef double value_type;

    

    /// Read real part at iterator position.

    template <class Iterator>

    value_type operator()(Iterator const & i) const {

      return (*i).real();

    }

    

    /// Read real part at offset from iterator position.

    template <class Iterator, class Difference>

    value_type operator()(Iterator const & i, Difference d) const {

      return (*(i+d)).real();

    }

    

    /// Write real part at iterator position.

    template <class Iterator>

    void set(value_type const & v, Iterator const & i) const {

      (*i) = v;

    }

    

    /// Write real part at offset from iterator position.

    template <class Iterator, class Difference>

    void set(value_type const & v, Iterator const & i, Difference d) const {

      (*(i+d)) = v;

    }

  };



  class RGBRealAccessor {

  public:



        /// The accessor's value type.

    typedef double value_type;



        /// Read real part at iterator position.

    template <class Iterator>

    value_type operator()(Iterator const & i) const {

      return (*i).luminance();

    }

    

        /// Read real part at offset from iterator position.

    template <class Iterator, class Difference>

    value_type operator()(Iterator const & i, Difference d) const {

      return (*(i+d)).luminance();

    }

    

        /// Write real part at iterator position.

    template <class Iterator>

    void set(value_type const & v, Iterator const & i) const {

      (*i).setRed(v);

      (*i).setGreen(v);

      (*i).setBlue(v);

    }



        /// Write real part at offset from iterator position.

    template <class Iterator, class Difference>

    void set(value_type const & v, Iterator const & i, Difference d) const {

      (*(i+d)).setRed(v);

      (*(i+d)).setGreen(v);

      (*(i+d)).setBlue(v);

    }

  };

  

  /*

    The CCAccessor provides filtering of pixels based on an image label. This serves the

    same purpose as the CCProxy in connected_component_iterators.hpp.

  */



  class CCAccessor {

  public:

    typedef OneBitPixel value_type;

    typedef OneBitPixel VALUETYPE;



    CCAccessor(OneBitPixel label) : m_label(label) { }

    

    template <class ITERATOR>

    VALUETYPE operator()(ITERATOR const & i) const {

      if (m_label == m_accessor(i))

	return 0;

      else

	return 1;

    }

    

    template <class ITERATOR, class DIFFERENCE>

    VALUETYPE operator()(ITERATOR & i, DIFFERENCE diff) const

    {

      ITERATOR tmp = i + diff;

      if (m_label == m_accessor(tmp))

        return 0; 

      else

	return 1;

    }



    template <class V, class ITERATOR>

    void set(V const & value, ITERATOR & i) const 

    {

      VALUETYPE tmp = vigra::detail::RequiresExplicitCast<VALUETYPE>::cast(value);

      if (m_accessor(i) == m_label)

	if (tmp) {

	  m_accessor.set(0, i);

	} else {

	  m_accessor.set(m_label, i);

	}

    }



    template <class V, class ITERATOR, class DIFFERENCE>

    void set(V const & value, ITERATOR & i, DIFFERENCE diff) const 

    { 

        VALUETYPE tmp = vigra::detail::RequiresExplicitCast<VALUETYPE>::cast(value); 

	ITERATOR tmpi = i + diff;

	if (m_accessor(tmpi) == m_label)

	  if (tmp) {

	    m_accessor.set(0, tmpi);

	  } else {

	    m_accessor.set(m_label, tmpi);

	  }

    }

    OneBitPixel m_label;

    ImageAccessor<value_type> m_accessor;

  };



  class RawCCAccessor {

  public:

    typedef OneBitPixel value_type;

    typedef OneBitPixel VALUETYPE;



    RawCCAccessor(OneBitPixel label) : m_label(label) { }

    

    template <class ITERATOR>

    VALUETYPE operator()(ITERATOR const & i) const {

      if (m_label == m_accessor(i))

	return 1;

      else

	return 0;

    }

    

    template <class ITERATOR, class DIFFERENCE>

    VALUETYPE operator()(ITERATOR & i, DIFFERENCE diff) const

    {

      ITERATOR tmp = i + diff;

      if (m_label == m_accessor(tmp))

        return 1; 

      else

	return 0;

    }



    template <class V, class ITERATOR>

    void set(V const & value, ITERATOR & i) const 

    {

      VALUETYPE tmp = vigra::detail::RequiresExplicitCast<VALUETYPE>::cast(value);

      if (m_accessor(i) == m_label)

	if (tmp) {

	  m_accessor.set(m_label, i);

	} else {

	  m_accessor.set(0, i);

	}

    }



    template <class V, class ITERATOR, class DIFFERENCE>

    void set(V const & value, ITERATOR & i, DIFFERENCE diff) const 

    { 

        VALUETYPE tmp = vigra::detail::RequiresExplicitCast<VALUETYPE>::cast(value); 

	ITERATOR tmpi = i + diff;

	if (m_accessor(tmpi) == m_label)

	  if (tmp) {

	    m_accessor.set(m_label, tmpi);

	  } else {

	    m_accessor.set(0, tmpi);

	  }

    }

    OneBitPixel m_label;

    ImageAccessor<value_type> m_accessor;

  };



  /*

    The OneBitAccessor hides the fact that OneBitValues can be something other

    than 0 or 1 in Gamera images.

  */

  class OneBitAccessor {

  public:

    typedef OneBitPixel value_type;

    typedef OneBitPixel VALUETYPE;

    

    template <class ITERATOR>

    VALUETYPE operator()(ITERATOR const & i) const {

      if (m_accessor(i))

	return 0;

      else

	return 1;

    }

    

    template <class ITERATOR, class DIFFERENCE>

    VALUETYPE operator()(ITERATOR & i, DIFFERENCE diff) const

    { 

      if (m_accessor(i + diff))

	return 0;

      else

	return 1;

    }

    

    template <class V, class ITERATOR>

    void set(V const & value, ITERATOR & i) const 

    { 

      if (value)

	m_accessor.set(0, i);

      else

	m_accessor.set(1, i);

    }

    

    template <class V, class ITERATOR, class DIFFERENCE>

    void set(V const & value, ITERATOR & i, DIFFERENCE diff) const 

    { 

      if (value)

	m_accessor.set(0, i + diff);

      else

	m_accessor.set(1, i + diff);

    }



    ImageAccessor<value_type> m_accessor;

  };



  // A OneBitAccessor that doesn't do any pixel inversion.  Should

  // only be used when converting a OneBit image to another OneBit image.



  class RawOneBitAccessor {

  public:

    typedef OneBitPixel value_type;

    typedef OneBitPixel VALUETYPE;

    

    template <class ITERATOR>

    VALUETYPE operator()(ITERATOR const & i) const {

      return m_accessor(i);

    }

    

    template <class ITERATOR, class DIFFERENCE>

    VALUETYPE operator()(ITERATOR & i, DIFFERENCE diff) const

    { 

      return m_accessor(i + diff);

    }

    

    template <class V, class ITERATOR>

    void set(V const & value, ITERATOR & i) const 

    { 

      if (value)

	m_accessor.set(1, i);

      else

	m_accessor.set(0, i);

    }

    

    template <class V, class ITERATOR, class DIFFERENCE>

    void set(V const & value, ITERATOR & i, DIFFERENCE diff) const 

    { 

      if (value)

	m_accessor.set(1, i + diff);

      else

	m_accessor.set(0, i + diff);

    }



    ImageAccessor<value_type> m_accessor;

  };



  /*

    These classes are used to make the selection of the appropriate accessor for a

    given type easier. They are, in essence, compile time factories that use

    specialization to choose the correct types. Also, the make_accessor static function

    is used to give the same function signature regardless of the arguments to the

    accessor.

  */



  template<class T>

  struct choose_accessor {

    typedef Accessor<typename T::value_type> accessor;

    static accessor make_accessor(const T& mat) {

      return accessor();

    }

    typedef Accessor<typename T::value_type> raw_accessor;

    static raw_accessor make_raw_accessor(const T& mat) {

      return raw_accessor();

    }

    typedef accessor real_accessor;

    static real_accessor make_real_accessor(const T& mat) {

      return real_accessor();

    }

    typedef BilinearInterpolatingAccessor<raw_accessor, typename T::value_type> interp_accessor;

    static interp_accessor make_interp_accessor(const T& mat) {

      return interp_accessor(make_raw_accessor(mat));

    }

  };



  template<>

  struct choose_accessor<RGBImageView> {

    typedef Gamera::RGBAccessor<RGBPixel> accessor;

    static accessor make_accessor(const RGBImageView& mat) {

      return accessor();

    }

    typedef Gamera::RGBAccessor<RGBPixel> raw_accessor;

    static raw_accessor make_raw_accessor(const RGBImageView& mat) {

      return raw_accessor();

    }

    typedef RGBRealAccessor real_accessor;

    static real_accessor make_real_accessor(const RGBImageView& mat) {

      return real_accessor();

    }

    typedef BilinearInterpolatingAccessor<raw_accessor, RGBPixel> interp_accessor;

    static interp_accessor make_interp_accessor(const RGBImageView& mat) {

      return interp_accessor(make_raw_accessor(mat));

    }

  };



  template<>

  struct choose_accessor<ComplexImageView> {

    typedef Accessor<ComplexPixel> accessor;

    static accessor make_accessor(const ComplexImageView& mat) {

      return accessor();

    }

    typedef Accessor<ComplexImageView::value_type> raw_accessor;

    static raw_accessor make_raw_accessor(const ComplexImageView& mat) {

      return raw_accessor();

    }

    typedef ComplexRealAccessor real_accessor;

    static real_accessor make_real_accessor(const ComplexImageView& mat) {

      return real_accessor();

    }

    typedef BilinearInterpolatingAccessor<raw_accessor, ComplexImageView::value_type> interp_accessor;

    static interp_accessor make_interp_accessor(const ComplexImageView& mat) {

      return interp_accessor(make_raw_accessor(mat));

    }

  };



  template<>

  struct choose_accessor<OneBitImageView> {

    typedef OneBitAccessor accessor;

    static accessor make_accessor(const OneBitImageView& mat) {

      return accessor();

    }

    typedef RawOneBitAccessor raw_accessor;

    static raw_accessor make_raw_accessor(const OneBitImageView& mat) {

      return raw_accessor();

    }

    typedef accessor real_accessor;

    static real_accessor make_real_accessor(const OneBitImageView& mat) {

      return real_accessor();

    }

    typedef BilinearInterpolatingAccessor<raw_accessor, OneBitPixel> interp_accessor;

    static interp_accessor make_interp_accessor(const OneBitImageView& mat) {

      return interp_accessor(make_raw_accessor(mat));

    }

  };



  template<>

  struct choose_accessor<OneBitRleImageView> {

    typedef OneBitAccessor accessor;

    static accessor make_accessor(const OneBitRleImageView& mat) {

      return accessor();

    }

    typedef RawOneBitAccessor raw_accessor;

    static raw_accessor make_raw_accessor(const OneBitRleImageView& mat) {

      return raw_accessor();

    }

    typedef accessor real_accessor;

    static real_accessor make_real_accessor(const OneBitRleImageView& mat) {

      return real_accessor();

    }

    typedef BilinearInterpolatingAccessor<raw_accessor, OneBitPixel> interp_accessor;

    static interp_accessor make_interp_accessor(const OneBitRleImageView& mat) {

      return interp_accessor(make_raw_accessor(mat));

    }

  };



  template<>

  struct choose_accessor<StaticImage<OneBitPixel> > {

    typedef OneBitAccessor accessor;

    static accessor make_accessor(const StaticImage<OneBitPixel>& mat) {

      return accessor();

    }

    typedef RawOneBitAccessor raw_accessor;

    static raw_accessor make_raw_accessor(const StaticImage<OneBitPixel>& mat) {

      return raw_accessor();

    }

    typedef accessor real_accessor;

    static real_accessor make_real_accessor(const StaticImage<OneBitPixel>& mat) {

      return real_accessor();

    }

    typedef BilinearInterpolatingAccessor<raw_accessor, OneBitPixel> interp_accessor;

    static interp_accessor make_interp_accessor(const StaticImage<OneBitPixel>& mat) {

      return interp_accessor(make_raw_accessor(mat));

    }

  };



  template<>

  struct choose_accessor<Cc> {

    typedef CCAccessor accessor;

    static accessor make_accessor(const Cc& mat) {

      return accessor(mat.label());

    }

    typedef RawCCAccessor raw_accessor;

    static raw_accessor make_raw_accessor(const Cc& mat) {

      return raw_accessor(mat.label());

    }

    typedef accessor real_accessor;

    static real_accessor make_real_accessor(const Cc& mat) {

      return real_accessor(mat.label());

    }

    typedef BilinearInterpolatingAccessor<raw_accessor, OneBitPixel> interp_accessor;

    static interp_accessor make_interp_accessor(const Cc& mat) {

      return interp_accessor(make_raw_accessor(mat));

    }

  };



  template<>

  struct choose_accessor<RleCc> {

    typedef CCAccessor accessor;

    static accessor make_accessor(const RleCc& mat) {

      return accessor(mat.label());

    }

    typedef RawCCAccessor raw_accessor;

    static raw_accessor make_raw_accessor(const RleCc& mat) {

      return raw_accessor(mat.label());

    }

    typedef accessor real_accessor;

    static real_accessor make_real_accessor(const RleCc& mat) {

      return real_accessor(mat.label());

    }

    typedef BilinearInterpolatingAccessor<raw_accessor, OneBitPixel> interp_accessor;

    static interp_accessor make_interp_accessor(const RleCc& mat) {

      return interp_accessor(make_raw_accessor(mat));

    }

  };



  /*

    These three functions are for convenience. They create the arguments for Vigra

    algorithms including the appropriate iterators and the corrent accessor for the type.

  */



  template<class Mat>

  inline triple<typename Mat::ConstIterator, typename Mat::ConstIterator,

		typename choose_accessor<Mat>::accessor>

  src_image_range(const Mat& img) {

    return triple<typename Mat::ConstIterator, typename Mat::ConstIterator,

      typename choose_accessor<Mat>::accessor> (img.upperLeft(), img.lowerRight(),

						choose_accessor<Mat>::make_accessor(img));

  }



  template<class Mat>

  inline std::pair<typename Mat::ConstIterator,

	      typename choose_accessor<Mat>::accessor>

  src_image(const Mat& img) {

    return std::pair<typename Mat::ConstIterator,

      typename choose_accessor<Mat>::accessor> (img.upperLeft(),

						choose_accessor<Mat>::make_accessor(img));

  }



  template<class Mat>

  inline triple<typename Mat::Iterator, typename Mat::Iterator,

		typename choose_accessor<Mat>::accessor>

  dest_image_range(Mat& img) {

    return triple<typename Mat::Iterator, typename Mat::Iterator,

      typename choose_accessor<Mat>::accessor> (img.upperLeft(), img.lowerRight(),

				       choose_accessor<Mat>::make_accessor(img));

  }



  template<class Mat>

  inline std::pair<typename Mat::Iterator, typename choose_accessor<Mat>::accessor>

  dest_image(Mat& img) {

    return std::pair<typename Mat::Iterator, typename choose_accessor<Mat>::accessor>(img.upperLeft(),

	       choose_accessor<Mat>::make_accessor(img));

  }

}



/*

  Declare numeric traits for the RGB type in Gamera. This must be done in the vigra namespace,

  though it seems kind of nasty to hijack someone else's namespace . . . 

*/



namespace vigra {

  /*

    NumericTraits for Gamera RGB types.

  */

  using namespace Gamera;



  template<>

  struct NumericTraits<RGBPixel>

  {

    typedef RGBPixel Type;

    typedef RGBValue<NumericTraits<RGBPixel::value_type>::Promote> Promote;

    typedef RGBValue<NumericTraits<RGBPixel::value_type>::RealPromote> RealPromote;

    

    typedef NumericTraits<RGBPixel::value_type>::isIntegral isIntegral;

    typedef VigraFalseType isScalar;

    typedef VigraFalseType isOrdered;

    

    static RGBPixel zero() { 

      return RGBPixel(NumericTraits<RGBPixel::value_type>::zero()); 

    }

    static RGBPixel one() { 

      return RGBPixel(NumericTraits<RGBPixel::value_type>::one()); 

    }

    static RGBPixel nonZero() { 

      return RGBPixel(NumericTraits<RGBPixel::value_type>::nonZero()); 

    }

    

    static Promote toPromote(RGBPixel const & v) { 

      return Promote(v); 

    }

    static RealPromote toRealPromote(RGBPixel const & v) { 

      return RealPromote(v); 

    }

    static RGBPixel fromPromote(Promote const & v) { 

      return RGBPixel(NumericTraits<RGBPixel::value_type>::fromPromote(v.red()),

			 NumericTraits<RGBPixel::value_type>::fromPromote(v.green()),

			 NumericTraits<RGBPixel::value_type>::fromPromote(v.blue()));

    }

    static RGBPixel fromRealPromote(RealPromote const & v) {

      return RGBPixel(NumericTraits<RGBPixel::value_type>::fromRealPromote(v.red()),

			 NumericTraits<RGBPixel::value_type>::fromRealPromote(v.green()),

			 NumericTraits<RGBPixel::value_type>::fromRealPromote(v.blue()));

    }

  };

#if 0  

  template <class T2>

  struct PromoteTraits<RGBPixel, RGBValue<T2> >

  {

    typedef RGBValue<typename PromoteTraits<typename RGBPixel::value_type, T2>::Promote> Promote;

  };

#endif

  

  template<>

  struct PromoteTraits<RGBPixel, double>

  {

    typedef RGBValue<NumericTraits<RGBPixel::value_type>::RealPromote> Promote;

  };

 

  template<>

  struct PromoteTraits<double, RGBPixel>

  {

    typedef RGBValue<NumericTraits<RGBPixel::value_type>::RealPromote> Promote;

  };



template<>

struct NumericTraits<ComplexPixel> {

  typedef ComplexPixel Type;

  typedef ComplexPixel Promote;

  typedef ComplexPixel RealPromote;

  typedef VigraFalseType isIntegral;

  typedef VigraFalseType isScalar;

  typedef VigraFalseType isOrdered;

  

  static ComplexPixel zero() {

    return ComplexPixel(0.0, 0.0);

  }

  static ComplexPixel one() {

    return ComplexPixel(1.0, 0.0);

  }

  static ComplexPixel nonZero() { return one(); }

  

  static ComplexPixel epsilon() { return ComplexPixel(LDBL_EPSILON, LDBL_EPSILON); }  

  

  static ComplexPixel max() { return ComplexPixel(DBL_MAX, DBL_MAX); };

  static ComplexPixel min() { return ComplexPixel(-DBL_MAX, -DBL_MAX); };

  

  static const Promote & toPromote(const Type & v) { return v; }

  static const RealPromote & toRealPromote(const Type & v) { return v; }

  static const Type & fromPromote(const Promote & v) { return v; }

  static const Type & fromRealPromote(const RealPromote & v) { return v; }

};

}

#endif


