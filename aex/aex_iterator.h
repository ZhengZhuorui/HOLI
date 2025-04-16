#pragma once
#include "aex_def.h"

namespace aex{

template<typename _Key,
        typename _Val,
        typename traits>
class aex_iterator{
public:
    typedef _Key key_type;
    typedef _Val value_type;
    typedef value_type& reference;
    typedef value_type* pointer;
    typedef typename traits::slot_type slot_type;
    typedef std::forward_iterator_tag iterator_category;
    typedef aex_iterator<_Key, _Val, traits> self;
    typedef aex_tree<key_type, value_type, traits> base_tree;
    typedef typename base_tree::data_node data_node;
    typedef data_node* data_node_ptr;

    inline aex_iterator() : _M_node(nullptr), offset(0){}
    inline aex_iterator(data_node* ptr, slot_type _offset) :_M_node(ptr), offset(_offset){}
    reference operator*(){ return std::pair<key_type, value_type>(_M_node->key[offset], _M_node->data[offset]); }

    self& operator++(){
        ++offset;
        if (offset >= _M_node->size){
            offset = 0;
            _M_node = _M_node->next;
        }
        while (_M_node != nullptr && _M_node->size == 0)
            _M_node = _M_node->next;
        return *this;
    }

    self operator++(int){
        self tmp = *this;
        ++offset;
        if (offset >= _M_node->size){
            offset = 0;
            _M_node = _M_node->next;
        }
        while (_M_node != nullptr && _M_node->size == 0)
            _M_node = _M_node->next;
        return tmp;
    }
    bool operator==(const self& x) const { return (_M_node == x._M_node) && (offset == x.offset); }
    bool operator!=(const self& x) const{ return  (_M_node != x._M_node) || (offset != x.offset); }
    inline const key_type& key() const { return _M_node->key[offset]; }
    inline value_type& data() const { return _M_node->data[offset]; }
    inline data_node_ptr get_node(){ return _M_node; }

#ifndef AEX_DEBUG
protected:
private:
#endif
    friend class aex_const_iterator<_Key, _Val, traits>;
    friend class aex_tree<_Key, _Val, traits>;
    data_node_ptr _M_node;
    slot_type     offset;
};

template<typename _Key,
        typename _Val,
        typename traits>
class aex_const_iterator{
public:
    typedef _Key key_type;
    typedef _Val value_type;
    typedef value_type& reference;
    typedef value_type* pointer;
    typedef typename traits::slot_type slot_type;
    typedef std::forward_iterator_tag iterator_category;
    typedef aex_const_iterator<_Key, _Val, traits> self;
    typedef aex_tree<key_type, value_type, traits> base_tree;
    typedef typename base_tree::data_node data_node;
    typedef data_node* data_node_ptr;
    inline aex_const_iterator() : _M_node(nullptr), offset(0){}
    inline aex_const_iterator(data_node* ptr, slot_type _offset):_M_node(ptr), offset(_offset){}
    inline aex_const_iterator(const aex_iterator<_Key, _Val, traits> &it) : _M_node(it._M_node), offset(it.offset){}    
    inline reference operator*() const { return std::pair<key_type, value_type>(_M_node->key[offset], _M_node->data[offset]); }

    self& operator++(){
        ++offset;
        if (offset >= _M_node->size){
            offset = 0;
            _M_node = _M_node->next;
        }
        while (_M_node != nullptr && _M_node->size == 0)
            _M_node = _M_node->next;
        return *this;
    }
    self operator++(int){
        self tmp = *this;
        ++offset;
        if (offset >= _M_node->size){
            offset = 0;
            _M_node = _M_node->next;
        }
        while (_M_node != nullptr && _M_node->size == 0)
            _M_node = _M_node->next;
        return tmp;
    }
    bool operator==(const self& x) const { return (_M_node == x._M_node) && (offset == x.offset); }
    bool operator!=(const self& x) const{ return  (_M_node != x._M_node) || (offset != x.offset); }
    inline const key_type& key() const { return _M_node->key[offset]; }
    inline const value_type& data() const { return _M_node->data[offset]; }
    inline data_node_ptr get_node(){
        return _M_node;
    }

#ifndef AEX_DEBUG
protected:
private:
#endif
    friend class aex_iterator<_Key, _Val, traits>;
    friend class aex_tree<_Key, _Val, traits>;
    data_node_ptr _M_node;
    slot_type     offset;

};

}