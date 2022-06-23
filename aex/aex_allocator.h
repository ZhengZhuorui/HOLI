#pragma once
#include <bits/stdc++.h>
#include "aex/aex_utils.h"
#include "aex/aex_node.h"

namespace aex
{

enum memory_prop{
    MAX_POOL_MEMORY=0x400000,
    MAX_POOL_NODE=0x20,
    DEFAULT_DATA_NODE_SIZE=0x8,
};

struct memory_config{
    std::vector<size_t> node_size_vec;
};



template<typename _Key,
        typename _Val,
        typename _aex_traits=aex_traits<_Key, _Val> >
class aex_allocator{
public:
    memory_config _config;
    char* data_node_pool;
    std::vector<char *> lt;
    aex_allocator(){}

    aex_allocator(memory_config config):_config(config){
        /* 
        *   TODO: memory pool
        */
    }

    static inline char* _allocate(size_t size){
        /* 
        *   TODO: memory pool
        */
       return static_cast<char*>(malloc(size));
    }

    template<typename _Tp>
    static inline _Tp* allocate(size_t size){
        /* 
        *   TODO: memory pool
        */
        return static_cast<char*>(malloc(size * sizeof(_Tp)))
    }

    static inline void free(char* p){
        /* 
        *   TODO: memory pool
        */
        if (p != nullptr)
            free(p);
    }

};


template<typename _Key, 
        typename _Val,
        typename aex_traits=aex_traits<_Key, _Val> >
class node_allocator{

    typedef _Key key_type;
    typedef _Val value_type;

    typedef aex_allocator allocator;
    typedef traits::Model Model;
    typedef aex_inner_node inner_node;
    typedef inner_node* inner_node_ptr;
    typedef aex_data_node data_node;
    typedef data_node* data_node_ptr;

    typedef aex_inner_node::bitmap bitmap;

    
    inline static size_t ML_INNER_NODE_MEMORY_USED(size_t slot_size){ 
        return BITMAP_MEMORY_USED(x) + KEY_MEMORY_USED<_Key>(x) + PTR_MEMORY_USED(x) +
        align_8bytes(sizeof(aex_inner_node<key_type, value_type, traits>)) ;
    }

    inline static size_t INNER_NODE_MEMORY_USED(size_t slot_size){
        return KEY_MEMORY_USED<_Key>(x) + PTR_MEMORY_USED(x) + 
        align_8bytes(sizeof(aex_inner_node<key_type, value_type, traits>));
    }


    inline static inner_node_ptr allocate_inner_node(size_t slot_size, int level, bool ml_node_flag=true){
        /*
        *   TODO: memory pool
        */
        size_t tmp;

        tmp = traits::MIN_INNER_NODE_SIZE;
        while (tmp < slot_size) tmp <<= 1;

        if (tmp > MIN_ML_INNER_NODE_SIZE && ml_node_flag){
            slot_size += traits::ERROR_BOUND;
            size_t memory_used = ML_INNER_NODE_MEMORY_USED(slot_size);
            bitmap bm;
            p = static_cast<inner_node_ptr>(allocator::_allocate(memory_used));
            p->slot_size = slot_size;
            p->prop = p->size = 0;
            p->prop |= ML_NODE;
            bm = p->get_bitmap_ptr();
            memset(bm, 0, BITMAP_MEMORY_USED(slot_size));
        }
        else{
            size_t memory_used = INNER_NODE_MEMORY_USED(slot_size);
            bitmap bm;
            p = static_cast<inner_node_ptr>(allocator::_allocate(memory_used));
            p->prop = p->size = 0;
        }
        return node;
    }
    
    inline static data_node_ptr allocate_data_node(){
        data_node_ptr node = allocator::allocate<data_node>(1);
        node->size = 0;
        node->level = 1;
        node->prev = node->next = nullptr;
        node->prop = LEAF;
        return node;
    }

    inline static void deallocate(inner_node_ptr p){
        /* 
        *   TODO: memory pool
        */
        if (p != nullptr)
            free(p);
    }

    inline static void deallocate(data_node_ptr p){
        /* 
        *   TODO: memory pool
        */
        if (p != nullptr)
            free(p);

    }

    inline static void deallocate(node_ptr p){
        if (p != nullptr){
            if (p->prop & LEAF) free_node(static_cast<data_node_ptr>(p));
            else free_node(static_cast<inner_node_ptr>(p));
        }
    }

};

} // namespace name
