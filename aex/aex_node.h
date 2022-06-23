#pragma once
#include <bits/stdc++.h>
#include "aex/aex_model.h"
#include "aex/aex_utils.h"
#include "aex/aex_traits.h"

namespace aex{
template<typename _Key,
        typename _Val,
        typename traits = aex_traits<_Key, _Val> >
class aex_node_base_iterator;

template<typename _Key,
        typename _Val,
        typename traits = aex_traits<_Key, _Val> >
struct aex_node_base{
public:
    typedef _Key key_type;

    typedef _Val value_type;

    typedef traits::size_type size_type;

    typedef typename _aex_node_base<_Key, _Val> node;

    typedef typename aex_node_base<key_type, value_type> self;

    typedef self* node_ptr;

    typedef aex_base_iterator iterator;

    size_type size, slot_size;

    unsigned int prop, level;
};

template<typename _Key, 
        typename _Val,
        typename traits>
struct aex_inner_node;

template<typename _Key, 
        typename _Val,
        typename traits>
struct aex_data_node;

template<typename _Key,
        typename _Val,
        typename traits = aex_traits<_Key, _Val> >
struct aex_inner_node : public aex_node_base<_Key, _Val, traits>{
public:

    typedef _Key key_type;

    typedef _Val value_type;

    typedef aex_inner_node<_Key, _Val, traits> self;

    typedef u_int64_t* bitmap;

    typedef aex_bitmap_impl<traits> bitmap_impl;

    typedef typename traits::size_type size_type;

    typedef typename traits::Model Model;

    const static size_type memory_used = sizeof(self);

    Model model;

    inline key_type* key_ptr(){
        return static_cast<key_type*>(align_8bytes(static_cast<char*>(this) + memory_used));
    }

    inline node_ptr* key_ptr(){
        return static_cast<node_ptr*>(align_8bytes(static_cast<char*>(this) + memory_used) + 
        KEY_MEMORY_USED(this->slot_size));
    }

    inline bitmap bitmap_ptr(){
        return static_cast<bitmap>(align_8bytes(static_cast(this) + memory_used) + 
        KEY_MEMORY_USED(this->slot_size) + PTR_MEMORY_USED(this->slot_size));
    }

    void construct(const key_type* const k, const node_ptr* const child, const int n){
        size_type start=0, pos;
        bitmap _bm = this->bitmap_ptr();
        key_type* node_k = this->key_ptr();
        child_ptr* node_child = this->child_ptr();
        node->size = n;
        if (node->prop & ML_NODE){
            Model::train(k, child, n, node->slot_size, node->_m);
            for (size_type i = 0; i < n; ++i){
                pos = _m->predict(k[i]);
                while (start <= pos){
                    node_k[start] = k[i];
                    node_child[start] = child[i];
                    ++start;
                }
                bitmap_impl::set_one(_bm, start - 1);
            }
        }
        else{
            memcpy(node_k, k, n * sizeof(key_type));
            memcpy(node_child, child, n * sizeof(node_ptr));
        }
    }

    void copy(const self* const node){
        memcpy(this, node, sizeof(self));
        memcpy(this->bitmap_ptr(), node->bitmap_ptr(), BITMAP_MEMORY_USED(node->slot_size));
        memcpy(this->key_ptr(), node->key_ptr(), KEY_MEMORY_USED(this->slot_size));
        memcpy(this->child_ptr(), node->child_ptr(), PTR_MEMORY_USED(this->slot_size));
    }

    // return the slot of child node
    inline size_type at(const node_ptr const &node){
        bitmap bm = this->bitmap_ptr();
        node_ptr child = this->child_ptr();
        if (prop & ML_NODE){
            for (size_type i = 0; i < ; ++i)
            if (bitmap_impl::at(bm, i) && child[i] == node)
            return i;
        }
        else{
            for (size_type i = 0; i < this->size; ++i)
            if (child[i] == node)
            return i
        }
        return this->slot_size;
    }

    inline size_type at(const key_type const &key){
        key_type *k = key_ptr();
        for (size_type i = 0; i < size; ++i)
        if (k[i] >= key) return i;
        return size;
    }

    inline node_ptr first(){
        if (this->prop & ML_NODE){
            bitmap bm = bitmap_ptr();
            for (size_type i = 0; i < slot_size; ++i)
                if (bitmap_impl::at(bm, i)) 
                    return child_ptr()[i];
        }
        else{
            return this->key[0];
        }
    }

    inline node_ptr last(){
        if (this->prop & ML_NODE){
            bitmap bm = bitmap_ptr()
            for (size_type i = slot_size - 1; i >= 0; --i)
                if (bitmap_impl::at(bm, i))
                    return child_ptr()[i];
        }
        else{
            return this->key[this->size - 1];
        }
    }
    
    inline node_ptr prev(size_type pos){
        bitmap bm = this->bitmap_ptr();
        if (prop & ML_NODE){
            for (size_type i = pos; i > 0; --i)
            if (bitmap_impl::at(bm, i))
                return i;
            return slot_size;
        }
        else return (pos == 0) ? slot_size : (pos - 1);
    }

    inline node_ptr next(size_type pos){
        bitmap bm = this->bitmap_ptr();
        if (prop & ML_NODE){
            for (size_type i = pos; i < slot_size; ++i)
            if (bitmap_impl::at(bm, i))
                return i;
            return slot_size;
        }
        else return (pos == (size - 1)) ? slot_size : pos + 1;
    }

    inline size_type real_slot_size(){
        return (prop & ML_NODE) ? (slot_size - traits::ERROR_BOUND) : slot_size;
    }

};

template<typename _Key,
        typename _Val,
        typename traits = aex_traits<_Key, _Val> >
struct aex_data_node : public aex_node_base<_Key, _Val, traits>{
public:
    typedef aex_data_node<_Key, _Val, traits> self;
    key_type key[traits::DATA_NODE_SIZE];
    value_type data[traits::DATA_NODE_SIZE];
    self* prev, next;
};


}