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
  Compressed Image Data (run-length compression)

  Authors
  -------
  Karl MacMillan <karlmac@peabody.jhu.edu>
  Michael Droettboom <mdboom@jhu.edu>

  History
  -------
  Started 5/15/2002 KWM
  Virtually rewritten 4/1/2005 MGD
*/

#include "image_data.hpp"
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
      all the way through the list of runs for every single access.  The iterators
      have certain tricks to make this faster, but performance will be much better
      for reading than writing.

      Encoding Scheme
      ---------------

      The old implementation stored only 'black' - i.e. non-zero
      pixels.  This seemed to not work at all, but the original
      author, Karl, no longer works on Gamera so it is hard to say
      whether this never fully worked or some other change in Gamera
      broke this code.  The CVS history seems to point to the former.

      The new implementation stores a run for each value, storing only
      the run's end.  Pixels "off the end" of a list of runs are
      assumed to be zero.  This keeps the memory consumption for large
      white areas of the image to a minimum.  This change helps keep
      the iterators in sync with the data.  In the old scheme, when an
      iterator was in a white area, it was impossible to tell what the
      next black area would be without rescanning the entire chunk.
      The first fix tried was to store an iterator to the next run,
      and then do a containment test when returning the pixel value.
      In profiling, this overhead seemed too heavy.  Now, the iterator
      is always inside some run, regardless of color (or at the end of
      a run chunk where the pixel is assumed to be white).

      This makes it easy to move to the next/previous run, except if
      the run list is changed out from under us.  This will make the
      iterator's pointer to the current run invalid.  The RleVector
      object (which manages the runs) has a value m_dirty which
      increments every time the structure of the run lists changes
      (but not merely the pixel values).  The iterators on the run
      data check their own internal copy of m_dirty against
      RleVector's everytime a pixel access needs to be made.  If
      different, a full search of the run list is performed to find
      the correct current run, and then the iterator's copy of m_dirty
      is updated.  If the same, the iterator's pointer to the current
      run is valid.  m_dirty is an integer value and not a flag
      because multiple iterators may by working on the same data at
      the same time.  This makes the semaphore work for all of them
      and not just the first one to access the data after an
      underlying change.

      In order to reduce the amount of time needed to find a
      particular run (which is prohibitive when the list of runs is
      very long) we store an array of lists of runs. Each list stores
      a range of coordinates determined by the static variable
      RLE_CHUNK. This means that we will sometimes break runs when
      they could be encoded as single run, but it makes the
      performance more even. To find the list that stores a particular
      position, simply divide by RLE_CHUNK - i.e.

        list_of_runs = array_of_lists[pos / RLE_CHUNK]
   
      Once you have the appropriate list, it is still necessary to
      scan through to find the particular run (or lack of run if the
      pixel is white). All we have done by using this array is to
      limit the length of the list that needs to be scanned by
      RLE_CHUNK.

      

      SPACE REDUCTION
      ---------------

      A further optimization for space has been added to take
      advantage of the fact that the positions stored in the run can
      be stored as an offest from the first possible position in a
      given list of runs (which I call a 'chunk'). If the positions
      stored in the runs are relative to the current chunk, we only
      need a type large enough to hold RLE_CHUNK positions. Setting
      RLE_CHUNK to 256 allows us to use an unsigned char for the
      positions in the run. If these relative positions weren't used
      we would have to allow the positions to be very large (probably
      at least size_t). The drawback to this space reduction is that
      we now have to convert between two sets of positions (global and
      relative).
    */

    /*
      see note above - this must be smaller than the largest number that the
      type of end in the Run class can hold.  This must be a power of 2
    */
    static const size_t RLE_CHUNK = 256;
    static const size_t RLE_CHUNK_1 = 255;   // RLE_CHUNK - 1
    static const size_t RLE_CHUNK_BITS = 8;  // log2(RLE_CHUNK)

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
      Run(runsize_t e, T v)
	: end(e), value(v) {
      }
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
      return global_pos & RLE_CHUNK_1;
    }

    inline size_t get_global_pos(runsize_t rel_pos, size_t chunk) {
      return size_t(rel_pos) + (chunk << RLE_CHUNK_BITS);
    }

    inline size_t get_chunk(size_t pos) {
      return pos >> RLE_CHUNK_BITS;
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
	m_dirty = vec->m_dirty;
      }
      RLEProxy(T* vec, size_t pos, const iterator* it) {
	m_vec = vec;
	m_pos = pos;
	m_iterator = it;
	m_dirty = vec->m_dirty;
      }
      // this is for RleVector[] - so, so, stupid, but oh well
      RLEProxy(T* vec, size_t pos, iterator i) {
	m_vec = vec;
	m_pos = pos;
	m_i = i;
	m_iterator = &m_i;
	m_dirty = vec->m_dirty;
      }
      void operator=(value_type v) {
	if (m_dirty == m_vec->m_dirty && m_iterator != 0)
 	  m_vec->set(m_pos, v, *m_iterator);
 	else
	  m_vec->set(m_pos, v);
      }
      operator value_type() const {
	if (m_dirty == m_vec->m_dirty && m_iterator != 0)
	  return (*m_iterator)->value;
	return m_vec->get(m_pos);
      }
    private:
      T* m_vec;
      size_t m_pos;
      const iterator* m_iterator;
      iterator m_i;
      size_t m_dirty;
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
	if (i->end >= rel_pos)
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
    
      RleVectorIteratorBase() : m_dirty(0) { }
      RleVectorIteratorBase(V* vec, size_t pos) : m_dirty(0) {
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
	  if (m_i != m_vec->m_data[m_chunk].begin()) {
	    iterator prev_i = prev(m_i);
	    if (get_rel_pos(m_pos) <= prev_i->end) {
	      m_i = prev_i;
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
      self operator+(size_t n) const {
	self tmp;
	tmp.m_vec = m_vec;
	tmp.m_pos = m_pos;
	tmp.m_chunk = m_chunk;
	tmp.m_i = m_i;
	tmp.m_dirty = m_dirty;
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
      self operator-(size_t n) const {
	self tmp;
	tmp.m_vec = m_vec;
	tmp.m_pos = m_pos;
	tmp.m_chunk = m_chunk;
	tmp.m_dirty = m_dirty;
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
      bool operator<=(const self& other) const {
	return m_pos <= other.m_pos;
      }
      bool operator>(const self& other) const {
	return m_pos > other.m_pos;
      }
      bool operator>=(const self& other) const {
	return m_pos >= other.m_pos;
      }
      difference_type operator-(const self& other) const {
	return m_pos - other.m_pos;
      }
      value_type get() const {
	// Unfortunately, for const-correctness reasons, I can't change
	// m_i or m_dirty here, so multiple calls to the get without moving
	// the iterator when the data is dirty will result in a search through
	// the run list chunk each time.
	iterator i;
	if (m_dirty != m_vec->m_dirty)
	  i = find_run_in_list(m_vec->m_data[m_chunk].begin(),
			       m_vec->m_data[m_chunk].end(), get_rel_pos(m_pos));
	else 
	  i = m_i;
	if (i != m_vec->m_data[m_chunk].end())
	  return i->value;
	return 0;
      }
      void set(const value_type& v) {
	if (m_dirty != m_vec->m_dirty) {
	  m_i = find_run_in_list(m_vec->m_data[m_chunk].begin(),
				 m_vec->m_data[m_chunk].end(), get_rel_pos(m_pos));
	  m_dirty = m_vec->m_dirty;
	}
	m_vec->set(m_pos, v, m_i);
      }
    protected:
      bool check_chunk() {
	if (m_dirty != m_vec->m_dirty || m_chunk != get_chunk(m_pos)) {
	  if (m_pos >= m_vec->m_size) {
	    m_chunk = m_vec->m_data.size() - 1;
	    m_i = m_vec->m_data[m_chunk].end();
	  } else {
	    m_chunk = get_chunk(m_pos);
	    m_i = find_run_in_list(m_vec->m_data[m_chunk].begin(),
				   m_vec->m_data[m_chunk].end(), get_rel_pos(m_pos));
	  }
	  m_dirty = m_vec->m_dirty;
	  return true;
	} else {
	  return false;
	}
      }
      V* m_vec;
      size_t m_pos;
      size_t m_chunk;
      iterator m_i;
      size_t m_dirty;
    };

    template<class V>
    class RleVectorIterator : public RleVectorIteratorBase<V, RleVectorIterator<V>,
							   typename V::list_type::iterator> {
	public:
      typedef RleVectorIterator self;
      typedef RleVectorIteratorBase<V, self, typename V::list_type::iterator> base;

      using base::m_i;
      using base::m_vec;
      using base::m_chunk;
      using base::m_pos;
      using base::m_dirty;

      typedef RLEProxy<V> proxy_type;
      typedef proxy_type reference;
      typedef proxy_type pointer;
    
      RleVectorIterator() : base() { }
      RleVectorIterator(V* vec, size_t pos) : base(vec, pos) { }

      proxy_type operator*() const {
	// Unfortunately, for const-correctness reasons, I can't change
	// m_i or m_dirty here, so multiple calls to the get without moving
	// the iterator when the data is dirty will result in a search through
	// the run list chunk each time.
	typename base::iterator i;
	if (m_dirty != m_vec->m_dirty)
	  i = find_run_in_list(m_vec->m_data[m_chunk].begin(),
			       m_vec->m_data[m_chunk].end(), get_rel_pos(m_pos));
	else 
	  i = m_i;
	if (i != m_vec->m_data[m_chunk].end())
	  return proxy_type(m_vec, m_pos, &i);
	return proxy_type(m_vec, m_pos);
      }
    };

    template<class V>
    class ConstRleVectorIterator
      : public RleVectorIteratorBase<V, ConstRleVectorIterator<V>,
				     typename V::list_type::const_iterator> {
	public:
      typedef ConstRleVectorIterator self;
      typedef RleVectorIteratorBase<V, self, typename V::list_type::const_iterator> base;

      using base::m_i;
      using base::m_vec;
      using base::m_chunk;
      using base::m_pos;
      using base::m_dirty;

      typedef void reference;
      typedef typename V::value_type* pointer;

      ConstRleVectorIterator() { }
      ConstRleVectorIterator(V* vec, size_t pos) : base(vec, pos) { }

      typename V::value_type operator*() const {
	// Unfortunately, for const-correctness reasons, I can't change
	// m_i or m_dirty here, so multiple calls to the get without moving
	// the iterator when the data is dirty will result in a search through
	// the run list chunk each time.
	typename base::iterator i;
	if (m_dirty != m_vec->m_dirty)
	  i = find_run_in_list(m_vec->m_data[m_chunk].begin(),
			       m_vec->m_data[m_chunk].end(), get_rel_pos(m_pos));
	else 
	  i = m_i;
	if (i != m_vec->m_data[m_chunk].end()) {
	  return i->value;
	}
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

      RleVector(size_t size = 0) : m_size(size), m_data((size >> RLE_CHUNK_BITS) + 1), m_dirty(0) { }
      void resize(size_t size) {
	m_size = size;
	m_data.resize((m_size >> RLE_CHUNK_BITS) + 1);
      }
      size_t size() const { return m_size; }

      /*
	Return the value at the specified position.
      */
      value_type get(size_t pos) const {
	assert(pos < m_size);
	size_t chunk = get_chunk(pos);
	runsize_t rel_pos = get_rel_pos(pos);
	// seems redundant and probably just slows things down, so removed
// 	if (m_data[chunk].empty())
// 	  return 0;

	typename list_type::const_iterator i;
	for (i = m_data[chunk].begin(); i != m_data[chunk].end(); ++i) {
	  if (i->end >= rel_pos)
	    return i->value;
	}
	return 0;
      }

      reference operator[](size_t pos) {
	size_t chunk = get_chunk(pos);
	typename list_type::iterator i = find_run_in_list(m_data[chunk].begin(),
							  m_data[chunk].end(),
							  get_rel_pos(pos));
	if (i != m_data[chunk].end()) {
	  return proxy_type(this, pos, i);
	}
	return proxy_type(this, pos);
      }

      /*
	Set the value at the specified position. This will
	create, split, or merge runs as necessary.
      */
      void set(size_t pos, value_type v) {
	size_t chunk = get_chunk(pos);
	if (m_data[chunk].empty())
	  set(pos, v, m_data[chunk].end());
	else {
	  typename list_type::iterator i = find_run_in_list
	    (m_data[chunk].begin(),
	     m_data[chunk].end(), get_rel_pos(pos));
	  set(pos, v, i);
	}
      }

      void set(size_t pos, value_type v, typename list_type::iterator i) {
	assert(pos < m_size);
	size_t chunk = get_chunk(pos);
	runsize_t rel_pos = get_rel_pos(pos);
	/*
	  If the list is empty our job is easy - just insert
	  a run.
	*/
	if (m_data[chunk].empty()) {
	  //// Empty run list, create new run(s) 
	  if (v != 0) {
	    if (rel_pos > 0)
	      m_data[chunk].push_back(run_type(rel_pos - 1, 0));
	    m_data[chunk].push_back(run_type(rel_pos, v));
	    m_dirty++;
	  }
	} else {
	  if (i != m_data[chunk].end())
	    insert_in_run(pos, v, i);
	  else if (v != 0) {
	    //// At end of run list -- append new runs
	    typename list_type::iterator last = prev(m_data[chunk].end());
	    if (rel_pos - last->end > 1) {
	      m_data[chunk].push_back(run_type(rel_pos - 1, 0));
	    } else {
	      if (last->value == v) {
		last->end++;
		return;
	      } 
	    }
	    m_data[chunk].push_back(run_type(rel_pos, v));
	    m_dirty++;
	  }
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
	  std::cout << "address: " << &(m_data[j]);
	  for (i = m_data[j].begin(); i != m_data[j].end(); ++i) {
	    std::cout << " end: " << int(i->end)
		      << " value: " << i->value << std::endl << std::endl;
	    total++;
	  }
	}
	std::cout << "object contained " << total << " runs." << std::endl;
      }
      /*
	This method is used to insert another run into the middle of
	an existing run. It handles resizing or splitting the run as
	necessary and will merge the inserted run as necessary.
      */
      inline void insert_in_run(size_t pos, value_type v, typename list_type::iterator i) {
	if (i->value != v) {
	  size_t chunk = get_chunk(pos);
	  runsize_t rel_pos = get_rel_pos(pos);
	  if (i != m_data[chunk].begin()) {
	    typename list_type::iterator prev_i = prev(i);
	    if (i->end - prev_i->end == 1) {
	      //// run of length 1
	      i->value = v;
	      merge_runs(i, chunk);
	      return;
	    }
	    if (prev_i->end + 1 == rel_pos) {
	      //// at beginning of run
	      // we do this value check here to avoid a creation/deletion for 
	      // the merge
	      if (prev_i->value == v)
		prev_i->end++; 
	      else 
		m_data[chunk].insert(i, run_type(rel_pos, v));
	      ++m_dirty;
	      return;
	    }
	  } else {
	    if (i->end == 0) {
	      //// first run of length 1
	      i->value = v;
	      merge_runs_after(i, chunk);
	      return;
	    } else if (rel_pos == 0) {
	      //// at beginning of first run
	      m_data[chunk].insert(i, run_type(0, v));
	      ++m_dirty;
	      return;
	    }
	  }
	    
	  ++m_dirty;
	  if (i->end == rel_pos) {
	    //// at end of run
	    i->end--;
	    // we do this value check here to avoid a creation/deletion for 
	    // the merge
	    typename list_type::iterator next_i = next(i);
	    if (next_i != m_data[chunk].end())
	      if (next_i->value == v)
		return;
	    m_data[chunk].insert(next_i, run_type(rel_pos, v));
	    return;
	  } 
	  
	  //// in middle of run
	  runsize_t old_end = i->end;
	  i->end = rel_pos - 1;
	  typename list_type::iterator next_i = next(i);
	  m_data[chunk].insert(next_i, run_type(rel_pos, v));
	  m_data[chunk].insert(next_i, run_type(old_end, i->value));
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
	    p->end = i->end;
	    m_data[chunk].erase(i);
	    i = p;
	    m_dirty++;
	  }
	}
	typename list_type::iterator n = next(i);
	if (n != m_data[chunk].end()) {
	  if (n->value == i->value) {
	    i->end = n->end;
	    m_data[chunk].erase(n);
	    m_dirty++;
	  }
	}
      }
//       /*
// 	These two methods do the same thing as merge_runs above, but
// 	in two separate steps. This allows other layers that know that
// 	there is no possibility of needing to merge in one particular
// 	direction to avoid the extra checking.
//       */
      void merge_runs_before(typename list_type::iterator i, size_t chunk) {
	if (i != m_data[chunk].begin()) {
	  typename list_type::iterator p = prev(i);
	  if (p->value == i->value) {
	    p->end = i->end;
	    m_data[chunk].erase(i);
	    m_dirty++;
	  }
	}
      }
      /*
 	see above.
      */
      void merge_runs_after(typename list_type::iterator i, size_t chunk) {
	typename list_type::iterator n = next(i);
	if (n != m_data[chunk].end()) {
	  if (n->value == i->value) {
	    i->end = n->end;
	    m_data[chunk].erase(n);
	    m_dirty++;
	  }
	}
      }
    public:
      size_t m_size;
      std::vector<list_type> m_data;
      size_t m_dirty;
    };
  } // namespace RleDataDetail
  /*
    This is an RleVector with the additional interface necessary to allow
    it to be used with a ImageView or ConnectedComponent object.
  */
  template<class T>
  class RleImageData : public RleDataDetail::RleVector<T>,
		       public ImageDataBase {
  public:
    using RleDataDetail::RleVector<T>::resize;
    typedef T value_type;
    typedef typename RleDataDetail::RleVector<T>::reference reference;
    typedef typename RleDataDetail::RleVector<T>::pointer pointer;
    typedef typename RleDataDetail::RleVector<T>::iterator iterator;
    typedef typename RleDataDetail::RleVector<T>::const_iterator const_iterator;

    RleImageData(const Size& size, const Point& offset)
      : RleDataDetail::RleVector<T>((size.height() + 1) * (size.width() + 1)),
	ImageDataBase(size, offset) {
    }
    RleImageData(const Size& size)
      : RleDataDetail::RleVector<T>((size.height() + 1) * (size.width() + 1)),
	ImageDataBase(size) {
    }

    RleImageData(const Dim& dim, const Point& offset)
      : RleDataDetail::RleVector<T>(dim.nrows() * dim.ncols()),
	ImageDataBase(dim, offset) {
    }
    RleImageData(const Dim& dim)
      : RleDataDetail::RleVector<T>(dim.nrows() * dim.ncols()),
	ImageDataBase(dim) {
    }

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
    virtual size_t bytes() const {
      size_t run_size = sizeof(RleDataDetail::Run<T>);
      size_t run_ptr_size = sizeof(RleDataDetail::Run<T>*);
      size_t num_runs = 0;
      for (size_t i = 0; i < this->m_data.size(); ++i)
	num_runs += this->m_data[i].size();
      return num_runs * (run_size + run_ptr_size + run_ptr_size);
    }
    virtual double mbytes() const { return bytes() / 1048576.0; }
    virtual void dimensions(size_t rows, size_t cols) {
      m_stride = cols;
      resize(rows * cols);
    }
    virtual void dim(const Dim& dim) {
      m_stride = dim.ncols();
      resize(dim.nrows() * dim.ncols());
    }
    virtual Dim dim() const {
      size_t size = ((RleDataDetail::RleVector<T>*)(this))->m_size;
      return Dim(m_stride, size / m_stride);      
    }
  protected:
    virtual void do_resize(size_t size) {
      resize(size);
    }
  };
}

#endif
