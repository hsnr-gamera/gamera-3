/*
 *
 * Copyright (C) 2002 Ichiro Fujinaga, Michael Droettboom, and Karl MacMillan
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
  Compressed Image Data (run-length compression)

  Authors
  -------
  Karl MacMillan <karlmac@peabody.jhu.edu>

  History
  -------
  Started 5/15/2002 KWM
*/

#include "dimensions.hpp"
#include "accessor.hpp"

#include <vector>
#include <list>
#include <utility>
#include <cassert>
#include <iterator>

#ifndef kwm05072002_rle_data
#define kwm05072002_rle_data

namespace Gamera {

  namespace RleDataDetail {
    /*
      This file contains a run-length compressed vector (not quite a complete
      interface for std::vector) and a Image Data object based on that vector.
      This is for use with very large 1-bit matrices. The performance should be
      acceptable for images that compress well (i.e. have few transitions from
      white to black), but will perform _very_ poorly as the number of transitions
      increases. Random-access is provided to this data, but this requires searching
      all the way through the list of runs for every singe acess (except in the case
      of the iterators).

      Encoding Scheme
      ---------------

      This implementation only stores 'black' - i.e. non-zero pixels. The run class
      below holds the beginning and end position of the run. This was done to make
      the code a little easier to understand - it works out to the same amount of space
      as storing the lengths of black and white.

      In order to reduce the amount of time needed to find a particular run (which
      is prohibitive when the list of runs is very long) we store an array of lists
      of runs. Each list stores a range of coordinates determined by the static variable
      RLE_CHUNK. This means that we will sometimes break runs when they could be
      encoded as single run, but it makes the performance more even. To find the
      list that stores a particular position, simply divide by RLE_CHUNK - i.e.

      list_of_runs = array_of_lists[pos / RLE_CHUNK]
    
      Once you have the appropriate list, it is still necessary to scan through
      to find the particular run (or lack of run if the pixel is white). All we
      have done by using this array is to limit the length of the list that needs
      to be scanned by RLE_CHUNK / 2.

      SPACE REDUCTION
      ---------------

      A further optimization for space has been added to take advantage of the fact
      that the positions stored in the run can be stored as an offest from the first
      possible position in a given list of runs (which I call a 'chunk'). If the positions
      stored in the runs are relative to the current chunk, we only need a type large enough
      to hold RLE_CHUNK positions. Setting RLE_CHUNK to 256 allows us to use an unsigned char
      for the positions in the run. If these relative positions weren't used we would have
      to allow the positions to be very large (probably at least size_t). The drawback to this
      space reduction is that we now have to deal with two sets of positions (global and
      relative).
    */

    /*
      see note above - this must be smaller than the largest number that the
      type of end in the Run class can hold
    */
    static const size_t RLE_CHUNK = 256;

    /*
      Again, see note above - this should be selected with reference to
      RLE_CHUNK.
    */
    typedef unsigned char runsize_t;

    /*
      These are convenience functions to make dealing
      with the list iterators a little easier.
    */
    template<class T>
    T next(T i) {
      return ++i;
    }
  
    template<class T>
    T prev(T i) {
      return --i;
    }
  
    /*
      This class holds the actual run as a value, beginning position, and
      ending position. It also includes some methods for convenience. It
      doesn't hide the data members because it is not exported as a public
      interface. IMPORTANT - all positions are relative to the current
      chunk.
    */
    template<class T>
    class Run {
    public:
      typedef T value_type;
      Run(runsize_t s, runsize_t e, T v)
	: start(s), end(e), value(v) {
      }
      // determine intersection with a point
      bool contains(runsize_t pos) const {
	if (pos >= start && pos <= end) {
	  return true;
	} else {
	  return false;
	}
      }
      // the length of the run
      size_t length() const {
	return end - start + 1;
      }
      // beginning of the run
      runsize_t start;
      // end of the run
      runsize_t end;
      // The value of the run (for connected-component
      // labeling).
      T value;
    };

    /*
      These are some quick functions to help with handling the
      positions in the runs.
    */
    inline runsize_t get_rel_pos(size_t global_pos) {
      size_t chunk = global_pos / RLE_CHUNK;
      return runsize_t(global_pos - (RLE_CHUNK * chunk));
    }

    inline size_t get_global_pos(runsize_t rel_pos, size_t chunk) {
      return size_t(rel_pos) + (chunk * RLE_CHUNK);
    }

    inline size_t get_chunk(size_t pos) {
      return pos / RLE_CHUNK;
    }

    /*
      RLEProxy
    
      The RleVectorIterator cannot return a reference for
      assignment because assignment is not a simple operation.
      This proxy class is returned instead which allows either
      conversion to the value for read access or setting a value
      in the runlength data for assignment. It is similar to
      the CCProxy in connected_componenent_iterators.hpp.

      There are two cases for the Proxy:
      a) we have a position that is in the middle of a run (so we have
         a pointer to an iterator that points to the run - we use
	 a pointer so we can determine whether or not we have the
	 iterator by comparison to 0).
      b) we only have the position and not an iterator.
      Case 'a' allows us to avoid a rather slow lookup, but we can't
      always use this optimization.
    */
    template<class T>
    // T is the RleVector type
    class RLEProxy {
    public:
      typedef typename T::value_type value_type;
      typedef typename T::list_type::iterator iterator;

      RLEProxy(T* vec, size_t pos) {
	m_vec = vec;
	m_pos = pos;
	m_iterator = 0;
      }
      RLEProxy(T* vec, size_t pos, const iterator* it) {
	m_vec = vec;
	m_pos = pos;
	m_iterator = it;
      }
      // this is for RleVector[] - so, so, stupid, but oh well
      RLEProxy(T* vec, size_t pos, iterator i) {
	m_vec = vec;
	m_pos = pos;
	m_i = i;
	m_iterator = &m_i;
      }
      void operator=(value_type v) {
	if (m_iterator != 0)
	  m_vec->insert_in_run(m_pos, v, *m_iterator);
	else
	  m_vec->set(m_pos, v);
      }
      operator value_type() const {
	if (m_iterator != 0) {
	  return (*m_iterator)->value;
	} else {
	  return 0;
	}
      }
    private:
      T* m_vec;
      size_t m_pos;
      const iterator* m_iterator;
      iterator m_i;
    };
  
    /*
      RleVectorIterator and ConstRleVectorIterator provide STL style
      iterator access to the run-length compressed data. Currently
      they are lazy - they only get a value when they are dereferenced.
      It is probably possible to speed these up slightly by keeping up
      with which run we are currently in, but it would mean messing around
      in the internals of RleVector. For now I consider the tradeoff of
      performance for separation and simplicity to be worthwhile.
    */

    // helper for the iterators
    template<class I>
    I find_run_in_list(I i, I end, runsize_t rel_pos) {
      for (; i != end; ++i) {
	if (i->contains(rel_pos))
	  return i;
      }
      return i;
    }

    template<class V, class Iterator, class ListIterator>
    class RleVectorIteratorBase {
    public:
      typedef typename V::value_type value_type;
      typedef int difference_type;
      typedef std::random_access_iterator_tag iterator_tag;
    
      typedef Iterator self;
      typedef ListIterator iterator;
    
      RleVectorIteratorBase() { }
      RleVectorIteratorBase(V* vec, size_t pos) {
	m_vec = vec;
	m_pos = pos;
	// find the current iterator (if there is one)
	size_t m_chunk = get_chunk(m_pos);
	runsize_t rel_pos = get_rel_pos(m_pos);
	m_i = find_run_in_list(m_vec->m_data[m_chunk].begin(),
			       m_vec->m_data[m_chunk].end(), rel_pos);
      }
      self& operator++() {
	m_pos++;
	if (!check_chunk()) {
	  if (m_i != m_vec->m_data[m_chunk].end()) {
	    if (get_rel_pos(m_pos) > m_i->end) {
	      ++m_i;
	    }
	  }
	}
	return (self&)*this;
      }
      self operator++(int) {
	self tmp;
	tmp.m_vec = m_vec;
	tmp.m_pos = m_pos;
	tmp.m_chunk = m_chunk;
	tmp.m_i = m_i;
	this->operator++();
	return tmp;
      }
      self& operator--() {
	m_pos--;
	if (!check_chunk()) {
	  if (m_i != m_vec->m_data[m_chunk].end()) {
	    if (get_rel_pos(m_pos) < m_i->start) {
	      if (m_i != m_vec->m_data[m_chunk].begin())
		--m_i;
	      else
		m_i = m_vec->m_data[m_chunk].end();
	    }
	  }
	}
	return (self&)*this;
      }
      self operator--(int) {
	self tmp;
	tmp.m_vec = m_vec;
	tmp.m_pos = m_pos;
	tmp.m_chunk = m_chunk;
	tmp.m_i = m_i;
	this->operator++();
	return tmp;
      }
      self& operator+=(size_t n) {
	m_pos += n;
	if (!check_chunk()) {
	  m_i = find_run_in_list(m_vec->m_data[m_chunk].begin(),
				 m_vec->m_data[m_chunk].end(), get_rel_pos(m_pos));
	}
	return (self&)*this;
      }
      self operator+(size_t n) {
	self tmp;
	tmp.m_vec = m_vec;
	tmp.m_pos = m_pos;
	tmp.m_chunk = m_chunk;
	tmp.m_i = m_i;
	tmp += n;
	return tmp;
      }
      self& operator-=(size_t n) {
	m_pos -= n;
	if (!check_chunk()) {
	  m_i = find_run_in_list(m_vec->m_data[m_chunk].begin(),
				 m_vec->m_data[m_chunk].end(), get_rel_pos(m_pos));
	}
	return (self&)*this;
      }
      self operator-(size_t n) {
	self tmp;
	tmp.m_vec = m_vec;
	tmp.m_pos = m_pos;
	tmp.m_chunk = m_chunk;
	tmp.m_i = m_i;
	tmp -= n;
	return tmp;
      }
      bool operator==(const self& other) const {
	return m_pos == other.m_pos;
      }
      bool operator!=(const self& other) const {
	return m_pos != other.m_pos;
      }
      bool operator<(const self& other) const {
	return m_pos < other.m_pos;
      }
      bool operator>(const self& other) const {
	return m_pos > other.m_pos;
      }
      difference_type operator-(const self& other) const {
	return m_pos - other.m_pos;
      }
      value_type get() const {
	if (m_i != m_vec->m_data[m_chunk].end())
	  return m_i->value;
	else
	  return 0;
      }
      void set(const value_type& v) const {
	if (m_i != m_vec->m_data[m_chunk].end())
	  m_vec->insert_in_run(m_pos, v, m_i);
	else
	  m_vec->set(m_pos, v);
      }
    protected:
      bool check_chunk() {
	if (m_chunk != get_chunk(m_pos)) {
	  m_chunk = get_chunk(m_pos);
	  m_i = find_run_in_list(m_vec->m_data[m_chunk].begin(),
				 m_vec->m_data[m_chunk].end(), get_rel_pos(m_pos));
	  return true;
	} else {
	  return false;
	}
      }
      V* m_vec;
      size_t m_pos;
      size_t m_chunk;
      iterator m_i;
    };

    template<class V>
    class RleVectorIterator : public RleVectorIteratorBase<V, RleVectorIterator<V>,
							   typename V::list_type::iterator> {
    public:
      typedef RLEProxy<V> proxy_type;
      typedef proxy_type reference;
      typedef proxy_type pointer;
    
      typedef RleVectorIterator self;
      typedef RleVectorIteratorBase<V, self, typename V::list_type::iterator> base;
    
      RleVectorIterator() { }
      RleVectorIterator(V* vec, size_t pos) : base(vec, pos) { }

      proxy_type operator*() const {
	if (m_i != m_vec->m_data[m_chunk].end()) {
	  return proxy_type(m_vec, m_pos, &m_i);
	} else {
	  return proxy_type(m_vec, m_pos);
	}
      }

    };

    template<class V>
    class ConstRleVectorIterator
      : public RleVectorIteratorBase<V, ConstRleVectorIterator<V>,
				     typename V::list_type::const_iterator> {
    public:
      typedef void reference;
      typedef typename V::value_type* pointer;

      typedef ConstRleVectorIterator self;
      typedef RleVectorIteratorBase<V, self, typename V::list_type::const_iterator> base;

      ConstRleVectorIterator() { }
      ConstRleVectorIterator(V* vec, size_t pos) : base(vec, pos) { }

      typename V::value_type operator*() const {
	if (m_i != m_vec->m_data[m_chunk].end())
	  return m_i->value;
	else
	  return 0;
      }

    };

    /*
      RleVector is a run-length compressed vector. It is optimized for
      space efficiency for document images (i.e. images with a background
      color) and places an emphasis on correctness rather than absolute
      performance.
    */
    template<class Data>
    class RleVector {
    public:
      // typedefs for convenience
      typedef RLEProxy<RleVector> proxy_type;
      typedef Data value_type;
      typedef proxy_type reference;
      typedef proxy_type pointer;
      typedef int difference_type;

      typedef Run<value_type> run_type;
      typedef std::list<run_type> list_type;
      typedef RleVector self;

      // iterators
      typedef RleVectorIterator<self> iterator;
      typedef ConstRleVectorIterator<const self> const_iterator;

      RleVector(size_t size = 0) : m_size(size), m_data(size / RLE_CHUNK + 1) { }
      void resize(size_t size) {
	m_size = size;
	m_data.resize(m_size / RLE_CHUNK + 1);
      }
      size_t size() const { return m_size; }

      /*
	Return the value at the specified position.
      */
      value_type get(size_t pos) const {
	assert(pos < m_size);
	size_t chunk = pos / RLE_CHUNK;
	runsize_t rel_pos = runsize_t(pos - (RLE_CHUNK * chunk));
	if (m_data[chunk].empty())
	  return 0;

	typename list_type::const_iterator i;
	for (i = m_data[chunk].begin(); i != m_data[chunk].end(); ++i) {
	  if (i->contains(rel_pos)) {
	    return i->value;
	  }
	  if (i->end > rel_pos)
	    return 0;
	}
	return 0;
      }
      reference operator[](size_t pos) {
	size_t chunk = get_chunk(pos);
	typename list_type::iterator i = find_run_in_list(m_data[chunk].begin(),
							  m_data[chunk].end(),
							  get_rel_pos(pos));
	if (i != m_data[chunk].end())
	  return proxy_type(this, pos, i);
	else
	  return proxy_type(this, pos);
      }
      /*
	Set the value at the specified position. This will
	create, split, or merge runs as necessary.
      */
      void set(size_t pos, value_type v) {
	assert(pos < m_size);
	if (v == 0)
	  return;

	size_t chunk = pos / RLE_CHUNK;
	runsize_t rel_pos = runsize_t(pos - (RLE_CHUNK * chunk));
	/*
	  If the list is empty our job is easy - just insert
	  a run.
	*/
	if (m_data[chunk].empty()) {
	  if (v != 0) {
	    m_data[chunk].push_back(run_type(rel_pos, rel_pos, v));
	  }
	} else {
	  typename list_type::iterator i;
	  /*
	    If the list is not empty we need to loop through
	    and find out if the position is in the middle or
	    touching another run.
	  */
	  typename list_type::iterator end = m_data[chunk].end();
	  for (i = m_data[chunk].begin(); i != end; ++i) {
	    if (i->contains(rel_pos)) {
	      insert_in_run(pos, v, i);
	      return;
	    } 
	    if (i->start > pos) {
	      m_data[chunk].insert(i, run_type(rel_pos, rel_pos, v));
	      merge_runs(prev(i), chunk);
	      return;
	    }
	  }
	  m_data[chunk].push_back(run_type(rel_pos, rel_pos, v));
	  merge_runs(prev(m_data[chunk].end()), chunk);
	}
      }
      /*
	Iterator access
      */
      iterator begin() {
	return iterator(this, 0);
      }
      iterator end() {
	return iterator(this, m_size);
      }
      const_iterator begin() const {
	return const_iterator(this, 0);
      }
      const_iterator end() const {
	return const_iterator(this, m_size);
      }

      /*
	This will print out a list of the the runs currently in
	m_data - it is for debugging.
      */
      void dump() {
	typename list_type::iterator i;
	size_t total = 0;
	for (size_t j = 0; j < m_data.size(); j++) {
	  for (i = m_data[j].begin(); i != m_data[j].end(); ++i) {
	    std::cout << "start: " << i->start << " end: " << i->end
		      << " value: " << i->value << std::endl << std::endl;
	    total++;
	  }
	  std::cout << "object contained " << total << " runs." << std::endl;
	}
      }
      /*
	This method is used to insert another run into the middle of
	an existing run. It handles resizing or splitting the run as
	necessary and will merge the inserted run as necessary.
      */
      void insert_in_run(size_t pos, value_type v, typename list_type::iterator i) {
	if (i->value != v) {
	  size_t chunk = pos / RLE_CHUNK;
	  runsize_t rel_pos = runsize_t(pos - (RLE_CHUNK * chunk));
	  if (i->length() == 1) {
	    i->value = v;
	    merge_runs(i, chunk);
	  } else {
	    if (i->start == pos) {
	      i->start++;
	      m_data[chunk].insert(i, run_type(rel_pos, rel_pos, v));
	      merge_runs_before(prev(i), chunk);
	    } else if (i->end == pos) {
	      i->end--;
	      m_data[chunk].insert(next(i), run_type(rel_pos, rel_pos, v));
	      merge_runs_after(next(i), chunk);
	    }
	  }
	}
      }
      /*
	This method merges runs that are touching and contain the same
	value. This is necessary to keep the minimum number of runs in
	the list.
      */
      void merge_runs(typename list_type::iterator i, size_t chunk) {
	if (i != m_data[chunk].begin()) {
	  typename list_type::iterator p = prev(i);
	  if (p->value == i->value) {
	    if (p->end == i->start || (p->end + 1) == i->start) {
	      p->end = i->end;
	      m_data[chunk].erase(i);
	      i = p;
	    }
	  }
	}
	if (next(i) != m_data[chunk].end()) {
	  typename list_type::iterator n = next(i);
	  if (n->value == i->value) {
	    if (n->start == i->end || n->start == (i->end + 1)) {
	      i->end = n->end;
	      m_data[chunk].erase(n);
	    }
	  }
	}
      }
      /*
	These two methods do the same thing as merge_runs above, but
	in two separate steps. This allows other layers that know that
	there is no possibility of needing to merge in one particular
	direction to avoid the extra checking.
      */
      void merge_runs_before(typename list_type::iterator i, size_t chunk){
	if (i != m_data[chunk].begin()) {
	  typename list_type::iterator p = prev(i);
	  if (p->value == i->value) {
	    if (p->end == i->start || (p->end + 1) == i->start) {
	      p->end = i->end;
	      m_data[chunk].erase(i);
	    }
	  }
	}
      }
      /*
	see above.
      */
      void merge_runs_after(typename list_type::iterator i, size_t chunk){
	if (next(i) != m_data[chunk].end()) {
	  typename list_type::iterator n = next(i);
	  if (n->value == i->value) {
	    if (n->start == i->end || n->start == (i->end + 1)) {
	      i->end = n->end;
	      m_data[chunk].erase(n);
	    }
	  }
	}
      }
    public:
      size_t m_size;
      std::vector<list_type> m_data;
    };
  } // namespace RleDataDetail
  /*
    This is an RleVector with the additional interface necessary to allow
    it to be used with a ImageView or ConnectedComponent object.
  */
  template<class T>
  class RleImageData : public RleDataDetail::RleVector<T> {
  public:
    typedef T value_type;
    typedef typename RleDataDetail::RleVector<T>::reference reference;
    typedef typename RleDataDetail::RleVector<T>::pointer pointer;
    typedef typename RleDataDetail::RleVector<T>::iterator iterator;
    typedef typename RleDataDetail::RleVector<T>::const_iterator const_iterator;

    RleImageData(size_t nrows = 1, size_t ncols = 1, size_t page_offset_y = 0,
		  size_t page_offset_x = 0) : RleDataDetail::RleVector<T>(nrows * ncols) {
      m_stride = ncols;
      m_page_offset_y = page_offset_y;
      m_page_offset_x = page_offset_x;
    }
    RleImageData(const Size<size_t>& size, size_t page_offset_y = 0,
		  size_t page_offset_x = 0)
      : RleDataDetail::RleVector<T>((size.height() + 1) * (size.width() + 1)) {
      m_stride = size.width() + 1;
      m_page_offset_x = page_offset_x;
      m_page_offset_y = page_offset_y;
    }
    RleImageData(const Dimensions<size_t>& dim, size_t page_offset_y = 0,
		  size_t page_offset_x = 0)
      : RleDataDetail::RleVector<T>(dim.nrows() * dim.ncols()) {
      m_stride = dim.ncols();
      m_page_offset_x = page_offset_x;
      m_page_offset_y = page_offset_y;
    }
    size_t stride() const { return m_stride; }
    size_t ncols() const { return m_stride; }
    size_t nrows() const { return size() / m_stride; }
    size_t page_offset_x() const { return m_page_offset_x; }
    size_t page_offset_y() const { return m_page_offset_y; }
    /*
      This is a little tricky and potentially expensive. The C++ standard
      (according the www.sgi.com/tech/stl) does not garuntee that list.size()
      is constant time - it can be O(N). So we may have to do an O(N) search
      on a large number of lists. This also cannot be exact because we have
      no way of determining the size of a list element (we don't know it's type).
      We are going to assume that each list node has two pointers to Run (this
      is what gcc does and any other sane implementation will do). This should
      give us an idea of the size, but nothing exact.
    */
    size_t bytes() const {
      size_t run_size = sizeof(RleDataDetail::Run<T>);
      size_t run_ptr_size = sizeof(RleDataDetail::Run<T>*);
      size_t num_runs = 0;
      for (size_t i = 0; i < m_data.size(); ++i)
	num_runs += m_data[i].size();
      return num_runs * (run_size + run_ptr_size + run_ptr_size);
    }
    double mbytes() const { return bytes() / 1048576.0; }

    void page_offset_x(size_t x) { m_page_offset_x = x; }
    void page_offset_y(size_t y) { m_page_offset_y = y; }
    void nrows(size_t nrows) { resize(nrows * ncols()); }
    void ncols(size_t ncols) { m_stride = ncols; resize(nrows() * m_stride); }
    void dimensions(size_t rows, size_t cols) { m_stride = cols; resize(rows * cols); }
  private:
    size_t m_stride;
    size_t m_page_offset_y;
    size_t m_page_offset_x;
  };
}

#endif
