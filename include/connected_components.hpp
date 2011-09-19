/*
 *
 * Copyright (C) 2001-2005 Ichiro Fujinaga, Michael Droettboom, Karl MacMillan
 *               2009      Jonathan Koch, Christoph Dalitz
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef kwm01032002_connected_component_hpp
#define kwm01032002_connected_component_hpp

#include "dimensions.hpp"
#include "image.hpp"
#include "gamera.hpp"
#include "image_view_iterators.hpp"
#include "connected_components_iterators.hpp"
#include "vigra_iterators.hpp"

#include <stdexcept>
#include <exception>
#include <map>
#include <vector>

#include <stdio.h>

/*
  ConnectedComponent
	
  This class implements a filtered image view. Within a ConnectedComponent
  only those pixels that match the assigned label will be shown. This requires
  the use of a proxy type for iterator types that require an lvalue be returned
  from dereferencing (see CCProxy below). KWM

  MultiLabelCC

  This class implements a filtered image view similar to ConnectedComponent,
  but with pixels visible that not only match a single label, but a
  set of possible labels.
*/

namespace Gamera {

  template <class T> class MultiLabelCC;    
    
  template<class T>
  class ConnectedComponent
    : public ImageBase<typename T::value_type> {
  public:
    using ImageBase<typename T::value_type>::ncols;
    using ImageBase<typename T::value_type>::nrows;
    using ImageBase<typename T::value_type>::offset_x;
    using ImageBase<typename T::value_type>::offset_y;

    // standard STL typedefs
    typedef typename T::value_type value_type;
    typedef typename T::pointer pointer;
    typedef typename T::reference reference;
    typedef typename T::difference_type difference_type;
    // Gamera specific
    typedef T data_type;
	
    // Vigra typedefs
    typedef value_type PixelType;

    // convenience typedefs
    typedef ConnectedComponent self;
    typedef ImageBase<typename T::value_type> base_type;

    //
    // CONSTRUCTORS
    //
    ConnectedComponent() : base_type() {
      m_image_data = 0;
      m_label = 0;
    }

    ConnectedComponent(T& image_data)
      : base_type(image_data.offset(),
		  image_data.dim()) {
      m_image_data = &image_data;
      range_check();
      calculate_iterators();
    }
    ConnectedComponent(T& image_data, value_type label,
		       const Rect& rect)
      : base_type(rect), m_label(label) {
      m_image_data = &image_data;
      range_check();
      calculate_iterators();
    }
    ConnectedComponent(T& image_data, value_type label,
		       const Point& upper_left,
		       const Point& lower_right)
      : base_type(upper_left, lower_right), m_label(label) {
      m_image_data = &image_data;
      range_check();
      calculate_iterators();
    }
    ConnectedComponent(T& image_data, value_type label,
		       const Point& upper_left,
		       const Size& size)
      : base_type(upper_left, size), m_label(label) {
      m_image_data = &image_data;
      range_check();
      calculate_iterators();
    }

    ConnectedComponent(T& image_data, value_type label,
		       const Point& upper_left,
		       const Dim& dim)
      : base_type(upper_left, dim), m_label(label) {
      m_image_data = &image_data;
      range_check();
      calculate_iterators();
    }

    //
    // COPY CONSTRUCTORS
    //
    ConnectedComponent(const self& other, const Rect& rect)
      : base_type(rect) {
      m_image_data = other.m_image_data;
      m_label = other.label();
      range_check();
      calculate_iterators();
    }
    ConnectedComponent(const self& other, const Point& upper_left,
		       const Point& lower_right)
      : base_type(upper_left, lower_right) {
      m_image_data = other.m_image_data;
      m_label = other.label();
      range_check();
      calculate_iterators();
    }
    ConnectedComponent(const self& other, const Point& upper_left,
		       const Size& size)
      : base_type(upper_left, size) {
      m_image_data = other.m_image_data;
      m_label = other.label();
      range_check();
      calculate_iterators();
    }
    ConnectedComponent(const self& other, const Point& upper_left,
		       const Dim& dim)
      : base_type(upper_left, dim) {
      m_image_data = other.m_image_data;
      m_label = other.label();
      range_check();
      calculate_iterators();
    }

    //
    //Conversion
    //
    MultiLabelCC<T>* convert_to_mlcc(){
        MultiLabelCC<T>* mlcc=new MultiLabelCC<T>( *((T*)this->data()), m_label, this->ul(), this->lr() );
        return mlcc;
    }
    
    //
    //  FUNCTION ACCESS
    //

    value_type get(const Point& point) const {
      value_type tmp = *(m_const_begin + (point.y() * m_image_data->stride()) + point.x());
      if (tmp == m_label)
      	return tmp;
      else
      	return 0;
    }

    void set(const Point& p, value_type value) {
      // we simply set the pixel value regardless of the label
      // warning: when value different from Cc.labels, the pixel will
      // appear to be white, even though we have written a different value
      *(m_begin + (p.y() * m_image_data->stride()) + p.x()) = value;
    }

    //
    // DIMENSIONS
    //
    // redefine the dimensions change function from Rect
    virtual void dimensions_change() {
      range_check();
      calculate_iterators();
    }

    //
    // Misc
    //
    virtual T* data() const { return m_image_data; }
    ImageView<T> parent() {
      return ImageView<T>(*m_image_data, 0, 0, m_image_data->nrows(),
			   m_image_data->ncols());
    }
    ImageView<T> image() {
      return ImageView<T>(*m_image_data, this->origin(), this->dim());
    }
    value_type label() const {
      return m_label;
    }

    void label(value_type label) {
      m_label = label;
    }

    //
    // Iterators
    //
    typedef CCDetail::RowIterator<self, typename T::iterator> row_iterator;
    row_iterator row_begin() {
      return row_iterator(this, m_begin);
    }
    row_iterator row_end() {
      return row_iterator(this, m_end);
    }

    typedef CCDetail::ColIterator<self, typename T::iterator> col_iterator;
    col_iterator col_begin() {
      return col_iterator(this, m_begin);
    }
    col_iterator col_end() {
      return col_iterator(this, m_begin + ncols());
    }

    //
    // Const Iterators
    //
    typedef CCDetail::ConstRowIterator<const self, typename T::const_iterator> const_row_iterator;
    const_row_iterator row_begin() const {
      return const_row_iterator(this, m_const_begin);
    }
    const_row_iterator row_end() const {
      return const_row_iterator(this, m_const_end);
    }

    typedef CCDetail::ConstColIterator<const self, typename T::const_iterator> const_col_iterator;
    const_col_iterator col_begin() const {
      return const_col_iterator(this, m_const_begin);
    }
    const_col_iterator col_end() const {
      return const_col_iterator(this, m_const_begin + ncols());
    }

    //
    // 2D iterators
    //
    typedef Gamera::ImageIterator<ConnectedComponent, typename T::iterator> Iterator;

    Iterator upperLeft() {
      return Iterator(this, m_image_data->begin(), m_image_data->stride())
	+ Diff2D(offset_x() - m_image_data->page_offset_x(), offset_y() - m_image_data->page_offset_y());
    }
    Iterator lowerRight() {
      return Iterator(this, m_image_data->begin(), m_image_data->stride())
	+ Diff2D(offset_x() + ncols() - m_image_data->page_offset_x(),
		 offset_y() + nrows() - m_image_data->page_offset_y());
    }

    typedef Gamera::ConstImageIterator<const ConnectedComponent,
				  typename T::const_iterator> ConstIterator;
    ConstIterator upperLeft() const {
      return ConstIterator(this, static_cast<const T*>(m_image_data)->begin(), m_image_data->stride())
	+ Diff2D(offset_x() - m_image_data->page_offset_x(),
		 offset_y() - m_image_data->page_offset_y());
    }
    ConstIterator lowerRight() const {
      return ConstIterator(this, static_cast<const T*>(m_image_data)->begin(), m_image_data->stride())
	+ Diff2D(offset_x() + ncols() - m_image_data->page_offset_x(),
		 offset_y() + nrows() - m_image_data->page_offset_y());
    }

    //
    // Vector iterator
    //
    typedef CCDetail::VecIterator<self, row_iterator, col_iterator> vec_iterator;
    vec_iterator vec_begin() { return vec_iterator(row_begin()); }
    vec_iterator vec_end() { return vec_iterator(row_end()); }

    typedef CCDetail::ConstVecIterator<self,
      const_row_iterator, const_col_iterator> const_vec_iterator;
    const_vec_iterator vec_begin() const {
      return const_vec_iterator(row_begin());
    }
    const_vec_iterator vec_end() const {
      return const_vec_iterator(row_end());
    }

    //
    // OPERATOR ACCESS
    //
    col_iterator operator[](size_t n) {
      return col_iterator(this, m_begin + (n * data()->stride())); }
    const_col_iterator operator[](size_t n) const {
      return const_col_iterator(this, m_begin + (n * data()->stride())); }
  private:
    /*
      We pre-compute iterators here in an effort to make begin() and end()
      methods a little faster. Unfortunately we have to to keep around both
      normal and const iterators.
    */
    void calculate_iterators() {
      m_begin = m_image_data->begin()
        // row offset
        + (m_image_data->stride() * (offset_y() - m_image_data->page_offset_y()))
        // col offset
        + (offset_x() - m_image_data->page_offset_x());
      m_end = m_image_data->begin()
        // row offset
        + (m_image_data->stride() * ((offset_y() - m_image_data->page_offset_y()) + nrows()))
        // column offset
        + (offset_x() - m_image_data->page_offset_x());
      const T* cmd = static_cast<const T*>(m_image_data);
      m_const_begin = cmd->begin()
        // row offset
	+ (m_image_data->stride() * (offset_y() - m_image_data->page_offset_y()))
        // col offset
        + (offset_x() - m_image_data->page_offset_x());
      m_const_end = cmd->begin()
        // row offset
        + (m_image_data->stride() * ((offset_y() - m_image_data->page_offset_y()) + nrows()))
        // column offset
        + (offset_x() - m_image_data->page_offset_x());
    }
    void range_check() {
      if (offset_y() + nrows() - m_image_data->page_offset_y() > m_image_data->nrows() ||
	  offset_x() + ncols() - m_image_data->page_offset_x() > m_image_data->ncols()
	  || offset_y() < m_image_data->page_offset_y()
	  || offset_x() < m_image_data->page_offset_x()) {
	char error[1024];
	sprintf(error, "Image view dimensions out of range for data\n");
	sprintf(error, "%s\tnrows %d\n", error, (int)nrows());
	sprintf(error, "%s\toffset_y %d\n", error, (int)offset_y());
	sprintf(error, "%s\tdata nrows %d\n", error, (int)m_image_data->nrows());
	sprintf(error, "%s\tncols %d\n", error, (int)ncols());
	sprintf(error, "%s\toffset_x %d\n", error, (int)offset_x());
	sprintf(error, "%s\tdata ncols %d\n", error,(int)m_image_data->ncols());
	throw std::range_error(error);
      }
    }
    // Pointer to the data for this view
    T* m_image_data;
    // Cached iterators - see calculate_iterators above.
    typename T::iterator m_begin, m_end;
    typename T::const_iterator m_const_begin, m_const_end;
    // The label for this connected-component
    value_type m_label;
  };


  template<class T>
  class MultiLabelCC : public ImageBase<typename T::value_type> {
  public:
    using ImageBase<typename T::value_type>::ncols;
    using ImageBase<typename T::value_type>::nrows;
    using ImageBase<typename T::value_type>::offset_x;
    using ImageBase<typename T::value_type>::offset_y;

    // standard STL typedefs
    typedef typename T::value_type value_type;
    typedef typename T::pointer pointer;
    typedef typename T::reference reference;
    typedef typename T::difference_type difference_type;
    // Gamera specific
    typedef T data_type;
	
    // Vigra typedefs
    typedef value_type PixelType;

    // convenience typedefs
    typedef MultiLabelCC self;
    typedef ImageBase<typename T::value_type> base_type;

    //
    // CONSTRUCTORS
    //
    MultiLabelCC() : base_type() {
      m_image_data = 0;
    }

    MultiLabelCC(T& image_data)
      : base_type(image_data.offset(),
		  image_data.dim()) {
      m_image_data = &image_data;
      range_check();
      calculate_iterators();
    }
    
    MultiLabelCC(T& image_data, value_type label,
		       const Rect& rect)
      : base_type(rect){
      m_image_data = &image_data;
      range_check();
      calculate_iterators();

      m_labels[label]=new Rect(rect);
    }
    MultiLabelCC(T& image_data, value_type label,
		       const Point& upper_left,
		       const Point& lower_right)
      : base_type(upper_left, lower_right){
      m_image_data = &image_data;
      range_check();
      calculate_iterators();

      m_labels[label]=new Rect(upper_left, lower_right);
    }
    MultiLabelCC(T& image_data, value_type label,
		       const Point& upper_left,
		       const Size& size)
      : base_type(upper_left, size){
      m_image_data = &image_data;
      range_check();
      calculate_iterators();

      m_labels[label]=new Rect(upper_left, size);
    }

    MultiLabelCC(T& image_data, value_type label,
		       const Point& upper_left,
		       const Dim& dim)
      : base_type(upper_left, dim){
      m_image_data = &image_data;
      range_check();
      calculate_iterators();

      m_labels[label]=new Rect(upper_left, dim);
    }
    
    //
    // DESTRUCTOR
    //
    virtual ~MultiLabelCC(){
      for(it=m_labels.begin(); it!=m_labels.end(); it++){
         delete it->second;
      }
    }

    //
    // COPY CONSTRUCTORS
    //
    MultiLabelCC(const self& other, const Rect& rect)
      : base_type(rect) {
      m_image_data = other.m_image_data;
      copy_labels(other);
      m_neighbors=other.m_neighbors;
      range_check();
      calculate_iterators();
    }
    MultiLabelCC(const self& other, const Point& upper_left,
		       const Point& lower_right)
      : base_type(upper_left, lower_right) {
      m_image_data = other.m_image_data;
      copy_labels(other);
      m_neighbors=other.m_neighbors;
      range_check();
      calculate_iterators();
    }
    MultiLabelCC(const self& other, const Point& upper_left,
		       const Size& size)
      : base_type(upper_left, size) {
      m_image_data = other.m_image_data;
      copy_labels(other);
      m_neighbors=other.m_neighbors;
      range_check();
      calculate_iterators();
    }
    MultiLabelCC(const self& other, const Point& upper_left,
		       const Dim& dim)
      : base_type(upper_left, dim) {
      m_image_data = other.m_image_data;
      copy_labels(other);

      m_neighbors=other.m_neighbors;
      range_check();
      calculate_iterators();
    }
      
    //
    //Conversion
    //
    typename std::list<ConnectedComponent<T>*>* convert_to_cc_list(){
      typename std::list<ConnectedComponent<T>*>* cc_list=new std::list<ConnectedComponent<T>*>();
      ConnectedComponent<T>* cc;
      for(it=m_labels.begin(); it!=m_labels.end(); it++){
        cc=new ConnectedComponent<T>( *((T*)this->data()), it->first, *(it->second));
        cc_list->push_back(cc);
      }
      return cc_list;
    }

    ConnectedComponent<T>* convert_to_cc(){
      // we must use iterators because set() is blocked for MlCc's
      typename MultiLabelCC::vec_iterator i = this->vec_begin();
      int label = m_labels.begin()->first;
      for ( ; i != this->vec_end(); ++i) {
        if (is_black(*i)) *i = label;
      }
      for(it=m_labels.begin(); it!=m_labels.end(); it++){
         delete it->second;
      }
      m_labels.clear();
      m_labels[label] = new Rect((Rect)*this);
      ConnectedComponent<T>* cc=new ConnectedComponent<T>( *((T*)this->data()), label, this->ul(), this->lr() );
      return cc;
    }

    //
    //  FUNCTION ACCESS
    //
    value_type get(const Point& point) const{
      value_type tmp = *(m_const_begin + (point.y() * m_image_data->stride()) + point.x());
      if(m_labels.find(tmp) != m_labels.end())
        return tmp;
      else
        return 0;    		
      }

    void set(const Point& p, value_type value) {
      // we simply set the pixel value regardless of the label
      // warning: when value is neither of the labels, the pixel will
      // appear to be white, even though we have written a different value
      *(m_begin + (p.y() * m_image_data->stride()) + p.x()) = value;
    }

    //
    // DIMENSIONS
    //
    // redefine the dimensions change function from Rect
    virtual void dimensions_change() {
      range_check();
      calculate_iterators();
    }

    //
    // Misc
    //
    virtual T* data() const { return m_image_data; }
    ImageView<T> parent() {
      return ImageView<T>(*m_image_data, 0, 0, m_image_data->nrows(),
			   m_image_data->ncols());
    }
    ImageView<T> image() {
      return ImageView<T>(*m_image_data, this->origin(), this->dim());
    }
    
/*
    typename std::map<value_type, Rect*> labels(){
      return m_labels;
    }
*/

    void get_labels(typename std::vector<int> &labels){
      for (it=m_labels.begin(); it!=m_labels.end(); it++){
        labels.push_back(it->first);
      }
    }
    
    bool has_label(value_type label) const {
    	return m_labels.find(label) != m_labels.end();
    }
    
    void add_label(value_type label, Rect& rect) {
      if(m_labels.size()==0){
        this->rect_set(rect.ul(),rect.lr());
      }
      //beware rect is only a reference and m_labels just stores pointers => you have to make a copy of rect
      m_labels[label]=new Rect(rect);
      this->union_rect(rect);
    }

    void remove_label(value_type label){
      it=m_labels.find(label);
      if(it!=m_labels.end()){
        delete it->second;
        m_labels.erase(label);
        find_bounding_box();
      }
    }
    
    void find_bounding_box(){
      if(m_labels.size()==0){
        this->rect_set(Point(0,0), Point(0,0));
      } else {
        coord_t maxX = 0;
        coord_t maxY = 0;
        coord_t minX = std::numeric_limits<coord_t>::max();
        coord_t minY = std::numeric_limits<coord_t>::max();
        
        for(this->it=this->m_labels.begin(); this->it!=this->m_labels.end(); this->it++){
          if(maxX<this->it->second->lr_x()) maxX=this->it->second->lr_x();
          if(maxY<this->it->second->lr_y()) maxY=this->it->second->lr_y();
          if(minX>this->it->second->ul_x()) minX=this->it->second->ul_x();
          if(minY>this->it->second->ul_y()) minY=this->it->second->ul_y();
        }
        
        this->rect_set(Point(minX,minY), Point(maxX,maxY));
      }
    }
    
    void add_neighbors(value_type i, value_type j){
      m_neighbors.push_back(i);
      m_neighbors.push_back(j);
    }

    void get_neighbors(typename std::vector<int> &neighbors){
      neighbors=this->m_neighbors;
    }

    void relabel(typename std::vector<typename std::vector<int>* >& labelVector, typename std::vector<self*>& mlccs){
      self* mlcc;
      for (size_t i=0; i<labelVector.size(); i++){
        mlcc=new self(*((T*)this->data()));
        mlccs.push_back(mlcc);
        for (size_t j=0; j<labelVector[i]->size(); j++){
          Rect* rect=m_labels[labelVector[i]->at(j)];
          if(rect!=NULL){
            value_type label=(value_type)(labelVector[i]->at(j));
            mlcc->add_label(label, *rect);
          } else {
            //tidy up
            for (size_t k=0; k<mlccs.size(); k++)
              delete mlccs[k];
            char error[64];
            sprintf(error, "There is no label %d stored in this MLCC.\n", labelVector[i]->at(j));
            throw std::runtime_error(error);
          }
        }
      }
    }

    //
    // Iterators
    //
    typedef MLCCDetail::RowIterator<self, typename T::iterator> row_iterator;
    row_iterator row_begin() {
      return row_iterator(this, m_begin);
    }
    row_iterator row_end() {
      return row_iterator(this, m_end);
    }

    typedef MLCCDetail::ColIterator<self, typename T::iterator> col_iterator;
    col_iterator col_begin() {
      return col_iterator(this, m_begin);
    }
    col_iterator col_end() {
      return col_iterator(this, m_begin + ncols());
    }

    //
    // Const Iterators
    //
    typedef MLCCDetail::ConstRowIterator<const self, typename T::const_iterator> const_row_iterator;
    const_row_iterator row_begin() const {
      return const_row_iterator(this, m_const_begin);
    }
    const_row_iterator row_end() const {
      return const_row_iterator(this, m_const_end);
    }

    typedef MLCCDetail::ConstColIterator<const self, typename T::const_iterator> const_col_iterator;
    const_col_iterator col_begin() const {
      return const_col_iterator(this, m_const_begin);
    }
    const_col_iterator col_end() const {
      return const_col_iterator(this, m_const_begin + ncols());
    }

    //
    // 2D iterators
    //
    typedef Gamera::ImageIterator<MultiLabelCC, typename T::iterator> Iterator;

    Iterator upperLeft() {
      return Iterator(this, m_image_data->begin(), m_image_data->stride())
        + Diff2D(offset_x() - m_image_data->page_offset_x(), offset_y() - m_image_data->page_offset_y());
    }
    Iterator lowerRight() {
      return Iterator(this, m_image_data->begin(), m_image_data->stride())
        + Diff2D(offset_x() + ncols() - m_image_data->page_offset_x(),
            offset_y() + nrows() - m_image_data->page_offset_y());
    }

    typedef Gamera::ConstImageIterator<const MultiLabelCC,
				  typename T::const_iterator> ConstIterator;
    ConstIterator upperLeft() const {
      return ConstIterator(this, static_cast<const T*>(m_image_data)->begin(), m_image_data->stride())
        + Diff2D(offset_x() - m_image_data->page_offset_x(),
            offset_y() - m_image_data->page_offset_y());
    }
    ConstIterator lowerRight() const {
      return ConstIterator(this, static_cast<const T*>(m_image_data)->begin(), m_image_data->stride())
        + Diff2D(offset_x() + ncols() - m_image_data->page_offset_x(),
            offset_y() + nrows() - m_image_data->page_offset_y());
    }

    //
    // Vector iterator
    //
    typedef MLCCDetail::VecIterator<self, row_iterator, col_iterator> vec_iterator;
    vec_iterator vec_begin() { return vec_iterator(row_begin()); }
    vec_iterator vec_end() { return vec_iterator(row_end()); }

    typedef MLCCDetail::ConstVecIterator<self,
      const_row_iterator, const_col_iterator> const_vec_iterator;
    const_vec_iterator vec_begin() const {
      return const_vec_iterator(row_begin());
    }
    const_vec_iterator vec_end() const {
      return const_vec_iterator(row_end());
    }

    //
    // OPERATOR ACCESS
    //
    col_iterator operator[](size_t n) {
      return col_iterator(this, m_begin + (n * data()->stride())); }
    const_col_iterator operator[](size_t n) const {
      return const_col_iterator(this, m_begin + (n * data()->stride())); }

    //for initialization of iterators
    const typename std::map<value_type, Rect*>* get_labels_pointer() const {
      return &m_labels;
    }
  private:
    void copy_labels(const self& other){
      typename std::map<value_type, Rect*>::const_iterator iter;
      for (iter = other.m_labels.begin(); iter != other.m_labels.end(); iter++){
        m_labels[iter->first]=new Rect(*(iter->second));
      }
    }

    /*
      We pre-compute iterators here in an effort to make begin() and end()
      methods a little faster. Unfortunately we have to keep around both
      normal and const iterators.
    */
    void calculate_iterators() {
      m_begin = m_image_data->begin()
        // row offset
        + (m_image_data->stride() * (offset_y() - m_image_data->page_offset_y()))
        // col offset
        + (offset_x() - m_image_data->page_offset_x());
      m_end = m_image_data->begin()
        // row offset
        + (m_image_data->stride() * ((offset_y() - m_image_data->page_offset_y()) + nrows()))
        // column offset
        + (offset_x() - m_image_data->page_offset_x());
      const T* cmd = static_cast<const T*>(m_image_data);
      m_const_begin = cmd->begin()
        // row offset
        + (m_image_data->stride() * (offset_y() - m_image_data->page_offset_y()))
        // col offset
        + (offset_x() - m_image_data->page_offset_x());
      m_const_end = cmd->begin()
        // row offset
        + (m_image_data->stride() * ((offset_y() - m_image_data->page_offset_y()) + nrows()))
        // column offset
        + (offset_x() - m_image_data->page_offset_x());
    }
    void range_check() {
      if (offset_y() + nrows() - m_image_data->page_offset_y() > m_image_data->nrows() ||
    	  offset_x() + ncols() - m_image_data->page_offset_x() > m_image_data->ncols()
    	  || offset_y() < m_image_data->page_offset_y()
    	  || offset_x() < m_image_data->page_offset_x()) {
      	char error[1024];
      	sprintf(error, "Image view dimensions out of range for data\n");
      	sprintf(error, "%s\tnrows %d\n", error, (int)nrows());
      	sprintf(error, "%s\toffset_y %d\n", error, (int)offset_y());
      	sprintf(error, "%s\tdata nrows %d\n", error, (int)m_image_data->nrows());
      	sprintf(error, "%s\tncols %d\n", error, (int)ncols());
      	sprintf(error, "%s\toffset_x %d\n", error, (int)offset_x());
      	sprintf(error, "%s\tdata ncols %d\n", error,(int)m_image_data->ncols());
      	throw std::range_error(error);
      }
    }
    // Pointer to the data for this view
    T* m_image_data;
    // Cached iterators - see calculate_iterators above.
    typename T::iterator m_begin, m_end;
    typename T::const_iterator m_const_begin, m_const_end;

    // The labels/rects for this connected-component
    typename std::map<value_type, Rect*> m_labels;
    typename std::map<value_type, Rect*>::iterator it;

    // The neighborhood-relations
    typename std::vector<int> m_neighbors;
  };
}

#endif
