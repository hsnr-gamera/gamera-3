/************************************************************************/
/*                                                                      */
/*               Copyright 1998-2001 by Ullrich Koethe                  */
/*       Cognitive Systems Group, University of Hamburg, Germany        */
/*                                                                      */
/*    This file is part of the VIGRA computer vision library.           */
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
 
 
#ifndef kwm05192002_vigra_iterator
#define kwm05192002_vigra_iterator

#include "vigra/utilities.hxx"
#include "vigra/iteratortraits.hxx"
#include "accessor.hpp"
#include "image_view_iterators.hpp"
#include "iterator_base.hpp"

namespace Gamera { 

  template <class ValueType, class Iterator>
  class ImageIteratorBase {
  public:
    typedef ValueType PixelType;
    typedef ValueType value_type;
    
    class MoveX {
    public:
      MoveX & operator=(MoveX const & rhs) {
	current_ = rhs.current_;
	return *this;
      }
      void operator++() {++current_;}
      void operator++(int) {++current_;}
      void operator--() {--current_;}
      void operator--(int) {--current_;}
      void operator+=(int dx) {current_ += dx; }
      void operator-=(int dx) {current_ -= dx; }
      bool operator==(MoveX const & rhs) const {
	return current_ == rhs.current_; }
      bool operator!=(MoveX const & rhs) const {
	return current_ != rhs.current_; }
      bool operator<(MoveX const & rhs) const {
	return current_ < rhs.current_; }
      int operator-(MoveX const & rhs) const {
	return current_ - rhs.current_; }
      MoveX(Iterator base)
        : current_(base) {}
      MoveX(MoveX const & rhs)
        : current_(rhs.current_) {}
      Iterator current_;
    };

    class MoveY {
    public:
      MoveY & operator=(MoveY const & rhs) {
	width_ = rhs.width_;
	offset_ = rhs.offset_;
	return *this;
      }
      void operator++() {offset_ += width_; }
      void operator++(int) {offset_ += width_; }
      void operator--() {offset_ -= width_; }
      void operator--(int) {offset_ -= width_; }
      void operator+=(int dy) {offset_ += dy*width_; }
      void operator-=(int dy) {offset_ -= dy*width_; }
      bool operator==(MoveY const & rhs) const {
	return (offset_ == rhs.offset_); }
      bool operator!=(MoveY const & rhs) const {
	return (offset_ != rhs.offset_); }
      bool operator<(MoveY const & rhs) const {
	return (offset_ < rhs.offset_); }
      int operator-(MoveY const & rhs) const {
	return ((offset_ - rhs.offset_) / width_);
      }
      MoveY(size_t width)
        : width_(width), offset_(0) {}
      MoveY(MoveY const & rhs)
        : width_(rhs.width_), offset_(rhs.offset_) {}
      size_t width_;
      int offset_;
    };

    bool operator==(ImageIteratorBase const & rhs) const {
      return (x == rhs.x) && (y == rhs.y);
    }
    bool operator!=(ImageIteratorBase const & rhs) const {
      return (x != rhs.x) || (y != rhs.y);
    }
    Diff2D operator-(ImageIteratorBase const & rhs) const {
      return Diff2D(x - rhs.x, y - rhs.y);
    }
    MoveX x;
    MoveY y;
  protected:
    ImageIteratorBase(Iterator base, size_t width)
      : x(base), y(width) {}
    ImageIteratorBase(ImageIteratorBase const & rhs)
      : x(rhs.x), y(rhs.y) {}
    ImageIteratorBase()
      : x(Iterator()), y(0) {}
    ImageIteratorBase & operator=(ImageIteratorBase const & rhs) {
      if(this != &rhs)
        {
	  x = rhs.x;
	  y = rhs.y;
        }
      return *this;
    }
    ImageIteratorBase & operator+=(Diff2D const & s) {
      x += s.x;
      y += s.y;
      return *this;
    }
    ImageIteratorBase & operator-=(Diff2D const & s) {
      x -= s.x;
      y -= s.y;
      return *this;
    }
    int width() const { return y.width_; }
  }; 
    
  template <class Image, class I>
  class ImageIterator : public ImageIteratorBase<typename Image::value_type, I> {
  public:
    typedef typename Image::value_type value_type;
    typedef value_type PixelType;
    typedef typename Image::reference reference;
    typedef reference index_reference;
    typedef typename Image::pointer pointer;
    typedef Diff2D difference_type;
    typedef image_traverser_tag  iterator_category;
    typedef I row_iterator;
    typedef ImageViewDetail::RowIterator<Image,
	    typename Image::data_type::iterator> column_iterator;

    ImageIterator(Image* mat, I base, size_t offset)
      : ImageIteratorBase<value_type, I>(base, offset), m_mat(mat) { }
    ImageIterator(const ImageIterator & rhs)
      : ImageIteratorBase<value_type, I>(rhs) {
      m_mat = rhs.m_mat;
    }    
    ImageIterator()
      : ImageIteratorBase<value_type, I>() {}    
    
    ImageIterator & operator=(const ImageIterator & rhs) {
      if(this != &rhs) {
	ImageIteratorBase<value_type, I>::operator=(rhs);
      }
      return *this;
    }

    ImageIterator & operator+=(Diff2D const & s) {
      ImageIteratorBase<value_type, I>::operator+=(s);
      return *this;
    }
    ImageIterator & operator-=(Diff2D const & s) {
      ImageIteratorBase<value_type, I>::operator-=(s);
      return *this;
    }
    Diff2D operator-(ImageIterator const & rhs) const {
      return Diff2D(x - rhs.x, y - rhs.y);
    }
    ImageIterator operator+(Diff2D const & s) const {
      ImageIterator ret(*this);
      ret += s;
      return ret;
    }
    ImageIterator operator-(Diff2D const & s) const {
      ImageIterator ret(*this);
      ret -= s;
      return ret;
    }
    value_type operator*() const {
      return *(x.current_ + y.offset_);
    }
    pointer operator->() const {
      return const_cast<pointer>(&*x.current_);
    }
    value_type operator[](Diff2D const & d) const {
      return m_accessor(x.current_ + d.x + (d.y * width()));
    }
    value_type operator()(int dx, int dy) const {
      return m_accessor(x.current_ + dx + (dy * width()));
    }
    row_iterator operator[](int dy) const {
      return x.current_ + (dy * width());
    }
    row_iterator rowIterator() const
    {
      return x.current_ + y.offset_;
    }
    column_iterator columnIterator() const { 
      return column_iterator(m_mat, x.current_ + y.offset_);
    }
    value_type get() const {
      return m_accessor(x.current_ + y.offset_);
    }
    void set(value_type v) {
      m_accessor.set(v, x.current_ + y.offset_);
    }
  private:
    ImageAccessor<value_type> m_accessor;
    Image* m_mat;
  };

  template <class Image, class I>
  class ConstImageIterator : public ImageIteratorBase<typename Image::value_type, I> {
  public:
    typedef typename Image::value_type value_type;
    typedef value_type PixelType;
    typedef typename Image::reference reference;
    typedef reference index_reference;
    typedef typename Image::pointer pointer;
    typedef Diff2D difference_type;
    typedef image_traverser_tag  iterator_category;
    typedef I row_iterator;
    typedef ImageViewDetail::ConstRowIterator<Image,
	    typename Image::data_type::const_iterator> column_iterator;

    ConstImageIterator(Image* mat, I base, size_t offset)
      : ImageIteratorBase<value_type, I>(base, offset), m_mat(mat) {
    }
    ConstImageIterator(const ConstImageIterator & rhs)
      : ImageIteratorBase<value_type, I>(rhs) {
      m_mat = rhs.m_mat;
    }    
    ConstImageIterator()
      : ImageIteratorBase<value_type, I>() {}    
    
    ConstImageIterator & operator=(const ConstImageIterator & rhs) {
      if(this != &rhs) {
	ImageIteratorBase<value_type, I>::operator=(rhs);
      }
      return *this;
    }

    ConstImageIterator & operator+=(Diff2D const & s) {
      ImageIteratorBase<value_type, I>::operator+=(s);
      return *this;
    }
    ConstImageIterator & operator-=(Diff2D const & s) {
      ImageIteratorBase<value_type, I>::operator-=(s);
      return *this;
    }
    Diff2D operator-(ConstImageIterator const & rhs) const {
      return Diff2D(x - rhs.x, y - rhs.y);
    }
    ConstImageIterator operator+(Diff2D const & s) const {
      ConstImageIterator ret(*this);
      ret += s;
      return ret;
    }
    ConstImageIterator operator-(Diff2D const & s) const {
      ConstImageIterator ret(*this);
      ret -= s;
      return ret;
    }
    value_type operator*() const {
      return *(x.current_ + y.offset_);
    }
    pointer operator->() const {
      return const_cast<pointer>(&*x.current_);
    }
    value_type operator[](Diff2D const & d) const {
      return m_accessor(x.current_ + d.x + (d.y * width()));
    }
    value_type operator()(int dx, int dy) const {
      return m_accessor(x.current_ + dx + (dy * width()));
    }
    row_iterator operator[](int dy) const {
      return x.current_ + dy * width();
    }
    row_iterator rowIterator() const
    {
      return x.current_ + y.offset_;
    }
    column_iterator columnIterator() const { 
      return column_iterator(m_mat, x.current_ + y.offset_);
    }
    value_type get() const {
      return m_accessor(x.current_ + y.offset_);
    }
  private:
    ImageAccessor<value_type> m_accessor;
    Image* m_mat;
  };
}

#endif
