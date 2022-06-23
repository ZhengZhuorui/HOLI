#pragma once

#include <bits/stdc++.h>

namespace aex{

template<typename _Key, typename _Val, typename traits> class aex_base_iterator;
template<typename _Key, typename _Val, typename traits> class aex_base_const_iterator;
template<typename _Key, typename _Val, typename traits> class aex_base_reverse_iterator;
template<typename _Key, typename _Val, typename traits> class aex_base_reverse_const_iterator;

template<typename _Key,
        typename _Val,
        typename traits=aex_traits<_Key, _Val> >
class aex_base_iterator{
public:

    typedef _Key key_type;

    typedef _Val value_type;

    typedef value_type& reference;

    typedef value_type* pointer;

    typedef std::bidirectional_iterator_tag iterator_category;

    typedef ptrdiff_t                  difference_type;

    typedef aex_base_iterator<_Key, _Val, traits> self;

    typedef aex_data_node<_Key, _Val, traits> node_type;

    typedef node_type* node_ptr;

    inline _aex_base_iterator() : _M_node(NULL), offset(0){}
    
    inline _aex_base_iterator(node_type* ptr, u_int8_t _offset):_M_node(ptr), offset(_offset){}
    
    inline _aex_base_iterator(const aex_base_reverse_iterator &it) : _M_node(it._M_node), offset(it.offset){}    
    
    reference operator*(){
        return std::pair(_M_node->key[offset], _M_node->data[offset]);
    }

    self& operator++(){
        ++offset;
        if (offset >= DATA_NODE_SLOT_SIZE){
            if (_M_node->next == NULL) return iterator(_M_node, traits::DATA_NODE_SLOT_SIZE);
            offset = 0;
            _M_node = _M_node->next;
        }
        return *this;
    }

    self& operator++(int){
        self _tmp = *this;
        ++offset;
        if (offset >= traits::DATA_NODE_SLOT_SIZE){
            if (_M_node->next == NULL) return iterator(_M_node, traits::DATA_NODE_SLOT_SIZE);
            offset = 0;
            _M_node = _M_node->next;
        }
        return _tmp;
    }

    self& operator--(){
        --offset;
        if (offset < 0){
            _M_node = _M_node->prev;
            offset = _M_node->size;
        }
        return *this;
    }
    _Self& operator--(int){
        self _tmp = *this;
        --offset;
        if (offset < 0){
            _M_node = _M_node->prev;
            offset = _M_node->size;
        }
        return _tmp;
    }

    bool operator==(const _Self& x) const {
        return (_M_node == x._M_node) && (offset == x.offset);
    }

    bool operator!=(const _Self& __x) const{ 
        return  (_M_node != x._M_node) || (offset != x.offset);
    }

protected:
    
    inline const key_type& key() const {
        return _M_node->key[offset];
    }

    inline value_type& value() const {
        return _M_node->data[offset];
    }

    inline node_ptr get_node(){
        return _M_node;
    }

private:
    
    u_int8_t offset;
    
    data_node_ptr _M_node;
};

template<typename _Key,
        typename _Val,
        typename traits=aex_traits<_Key, _Val> >
class aex_base_const_iterator{
public:

    typedef _Key key_type;

    typedef _Val value_type;

    typedef value_type& reference;

    typedef value_type* pointer;

    typedef std::bidirectional_iterator_tag iterator_category;

    typedef ptrdiff_t                  difference_type;

    typedef typename aex_base_const_iterator<_Key, _Val> self;

    typedef typename aex_data_node<_Key, _Val, traits> node_type;

    typedef node_type* node_ptr;

    inline aex_base_const_iterator() : _M_node(NULL), offset(0){}
    
    inline aex_base_const_iterator(const node_ptr ptr, u_int8_t _offset):_M_node(ptr), offset(_offset){}
    
    inline aex_base_const_iterator(const aex_base_iterator &it) : _M_node(it._M_node), offset(it.offset){}    
    
    inline aex_base_const_iterator(const aex_base_reverse_iterator &it) : _M_node(it._M_node), offset(it.offset){}    
    
    inline aex_base_const_iterator(const aex_base_reverse_const_iterator &it) : _M_node(it._M_node), offset(it.offset){}    

    
    inline reference operator*() const{
        //return static_cast<_Link_ptr>(_M_node->item[offset]);
        return std::pair(_M_node->key[offset], _M_node->data[offset]);
    }

    self& operator++(){
        ++offset;
        if (offset >= traits::DATA_NODE_SLOT_SIZE){
            if (_M_node->next == NULL) return iterator(_M_node, traits::DATA_NODE_SLOT_SIZE);
            offset = 0;
            _M_node = _M_node->next;
        }
        return *this;
    }

    self& operator++(int){
        self _tmp = *this;

        ++offset;
        if (offset >= traits::DATA_NODE_SLOT_SIZE){
            if (_M_node->next == NULL) return iterator(_M_node, traits::DATA_NODE_SLOT_SIZE);
            offset = 0;
            _M_node = _M_node->next;
        }

        return _tmp;
    }

    self& operator--(){
        --offset;
        if (offset < 0){
            _M_node = _M_node->prev;
            offset = _M_node->size;
        }
        return *this;
    }
    _Self& operator--(int){
        self _tmp = *this;
        --offset;
        if (offset < 0){
            _M_node = _M_node->prev;
            offset = _M_node->size;
        }
        return _tmp;
    }

    bool operator==(const _Self& x) const {
        return (_M_node == x._M_node) && (offset == x.offset);
    }

    bool operator!=(const _Self& __x) const{ 
        return  (_M_node != x._M_node) || (offset != x.offset);
    }

protected:
    
    inline const key_type& key() const {
        return _M_node->key[offset];
    }

    inline const value_type& value() const {
        return _M_node->data[offset];
    }

    inline node_ptr get_node(){
        return _M_node;
    }

private:
    u_int8_t offset;
    
    data_node_ptr _M_node;

};

template<typename _Key,
        typename _Val,
        typename traits=aex_traits<_Key, _Val> >
class aex_base_reverse_iterator{
public:

    typedef _Key key_type;
    
    typedef _Val value_type;

    typedef value_type& reference;

    typedef value_type* pointer;

    typedef std::bidirectional_iterator_tag iterator_category;

    typedef ptrdiff_t                  difference_type;

    typedef aex_base_iterator<_Key, _Val> self;

    typedef aex_data_node<_Key, _Val> node_type;

    typedef node_type* node_ptr;

    u_int8_t offset;

    data_node_ptr _M_node;

    inline _aex_base_iterator() : _M_node(NULL), offset(0){}
    
    inline _aex_base_iterator(node_type* ptr, u_int8_t _offset):_M_node(ptr), offset(_offset){}
    
    inline _aex_base_iterator(const aex_base_iterator &it) : _M_node(it._M_node), offset(it.offset){}    
    
    reference operator*(){
        return std::pair(_M_node->key[offset], _M_node->data[offset]);
    }

    self& operator--(){
        ++offset;
        if (offset >= DATA_NODE_SLOT_SIZE){
            if (_M_node->next == NULL) return iterator(_M_node, traits::DATA_NODE_SLOT_SIZE);
            offset = 0;
            _M_node = _M_node->next;
        }
        return *this;
    }

    self& operator--(int){
        self _tmp = *this;
        ++offset;
        if (offset >= traits::DATA_NODE_SLOT_SIZE){
            if (_M_node->next == NULL) return iterator(_M_node, traits::DATA_NODE_SLOT_SIZE);
            offset = 0;
            _M_node = _M_node->next;
        }
        return _tmp;
    }

    self& operator++(){
        --offset;
        if (offset < 0){
            _M_node = _M_node->prev;
            offset = _M_node->size;
        }
        return *this;
    }
    _Self& operator++(int){
        self _tmp = *this;
        --offset;
        if (offset < 0){
            _M_node = _M_node->prev;
            offset = _M_node->size;
        }
        return tmp;
    }

    inline bool operator==(const _Self& x) const {
        return (_M_node == x._M_node) && (offset == x.offset);
    }

    inline bool operator!=(const _Self& __x) const { 
        return  (_M_node != x._M_node) || (offset != x.offset);
    }

protected:
    
    inline const key_type& key() const {
        return _M_node->key[offset];
    }

    inline value_type& value() const{
        return _M_node->data[offset];
    }

    inline node_ptr get_node() const{
        return _M_node;
    }

};

template<typename _Key,
        typename _Val,
        typename traits=aex_traits<_Key, _Val> >
class aex_base_reverse_const_iterator{
public:

    typedef _Key key_type;
    
    typedef _Val value_type;

    typedef value_type& reference;

    typedef value_type* pointer;

    typedef std::bidirectional_iterator_tag iterator_category;

    typedef ptrdiff_t                  difference_type;

    typedef aex_base_iterator<_Key, _Val> self;

    typedef aex_data_node<_Key, _Val> node_type;

    typedef node_type* node_ptr;

    u_int8_t offset;

    data_node_ptr _M_node;

    inline _aex_base_iterator() : _M_node(NULL), offset(0){}
    
    inline _aex_base_iterator(node_type* ptr, u_int8_t _offset):_M_node(ptr), offset(_offset){}
    
    inline _aex_base_iterator(const aex_base_iterator &it) : _M_node(it._M_node), offset(it.offset){}    

    inline _aex_base_iterator(const aex_base_const_iterator &it) : _M_node(it._M_node), offset(it.offset){}    

    inline _aex_base_iterator(const aex_base_reverse_iterator &it) : _M_node(it._M_node), offset(it.offset){}    
    
    reference operator*(){
        return std::pair(_M_node->key[offset], _M_node->data[offset]);
    }

    self& operator--(){
        ++offset;
        if (offset >= DATA_NODE_SLOT_SIZE){
            if (_M_node->next == NULL) return iterator(_M_node, traits::DATA_NODE_SLOT_SIZE);
            offset = 0;
            _M_node = _M_node->next;
        }
        return *this;
    }

    self& operator--(int){
        self _tmp = *this;
        ++offset;
        if (offset >= traits::DATA_NODE_SLOT_SIZE){
            if (_M_node->next == NULL) return iterator(_M_node, traits::DATA_NODE_SLOT_SIZE);
            offset = 0;
            _M_node = _M_node->next;
        }
        return _tmp;
    }

    self& operator++(){
        --offset;
        if (offset < 0){
            _M_node = _M_node->prev;
            offset = _M_node->size;
        }
        return *this;
    }
    _Self& operator++(int){
        self _tmp = *this;
        --offset;
        if (offset < 0){
            _M_node = _M_node->prev;
            offset = _M_node->size;
        }
        return tmp;
    }

    inline bool operator==(const _Self& x) const {
        return (_M_node == x._M_node) && (offset == x.offset);
    }

    inline bool operator!=(const _Self& __x) const { 
        return  (_M_node != x._M_node) || (offset != x.offset);
    }

protected:
    
    inline const key_type& key() const {
        return _M_node->key[offset];
    }

    inline const value_type& value() const{
        return _M_node->data[offset];
    }

    inline node_ptr get_node() const{
        return _M_node;
    }

};


}