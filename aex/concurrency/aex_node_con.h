#pragma once

#include "../aex_node.h"
namespace aex{

// bitmap is used as updatable memory in aex_hash_node_con
template<typename _Key,
        typename _Val,
        typename traits>
struct aex_hash_node_con : public aex_hash_node<_Key, _Val, traits>{
    typedef _Key                                   key_type;
    typedef _Val                                   value_type;
    typedef aex_hash_node<_Key, _Val, traits>      parent;
    typedef aex_hash_node_con<_Key, _Val, traits>  self;
    typedef aex_tree<key_type, value_type, traits> base_tree;
    typedef typename base_tree::components         components;
    //typedef aex_default_components<traits>         components;
    typedef typename traits::hash_type             hash_type;
    typedef typename parent::slot_type             slot_type;
    typedef typename parent::Model                 Model;
    typedef typename parent::bitmap                bitmap;
    typedef typename parent::bitmap_base           bitmap_base;
    typedef typename parent::bitmap_impl           bitmap_impl;
    typedef typename parent::node_ptr              node_ptr;
    typedef typename parent::inner_node            inner_node;
    typedef typename components::RWLock            RWLock;
    typedef typename components::Lock              Lock;
    typedef typename components::version_type      version_type;
    typedef typename components::atomic_version_type      atomic_version_type;
    typedef typename components::size_type         size_type;

    //typedef components::pos2slot pos2slot;
    //using components::pos2slot;

    //aex_hash_node_con(slot_type slot_size):parent(slot_size){init();}
    //~aex_hash_node_con(){clear();}   
    aex_hash_node_con():parent(){};
    ~aex_hash_node_con(){};
    aex_hash_node_con(aex_hash_node_con &other){
        memcpy(this, &other, sizeof(self));
    }
    aex_hash_node_con& operator = (aex_hash_node_con &other){
        memcpy(this, &other, sizeof(self));
        return *this;
    }

    void clear(){
        if (this->bitmap_ptr != nullptr){
            delete[] this->bitmap_ptr;
            this->bitmap_ptr = nullptr;
        }
        if (this->lock_array != nullptr){
            delete[] this->lock_array;
            this->lock_array = nullptr;
        }
    }

    void init(){
        this->size = 0;
        this->bitmap_ptr = new bitmap_base[this->slot_size / traits::SLOT_PER_LOCK + 1]();
        //this->lock_array = new RWLock[this->slot_size / traits::SLOT_PER_LOCK + 1];
        this->lock_array = new Lock[this->slot_size / traits::SLOT_PER_LOCK + 1];
    }

    inline void set_one(const slot_type x) {
        __sync_fetch_and_or(this->bitmap_ptr + pos2slot(x), 1ULL << (x & 63));
    }

    inline void set_zero(const slot_type x) {
        __sync_fetch_and_and(this->bitmap_ptr + pos2slot(x), ~(1ULL << (x & 63)));
    }

    inline void add_size(){
        if (this->slot_size * traits::HASH_NODE_FULL_RATIO >= traits::SIZE_BLOCK_CNT * traits::MIN_ADD_CNT){
            size_type min_add_cnt = this->slot_size * traits::HASH_NODE_FULL_RATIO / traits::SIZE_BLOCK_CNT;
            if (get_randint(min_add_cnt) == 1)
                __sync_fetch_and_add(&this->size, min_add_cnt);
        }
        else{
            __sync_fetch_and_add(&this->size, 1);
        }
    }
    inline void sub_size(){
        if (this->slot_size * traits::HASH_NODE_FULL_RATIO >= traits::SIZE_BLOCK_CNT * traits::MIN_ADD_CNT){
            size_type min_add_cnt = this->slot_size * traits::HASH_NODE_FULL_RATIO / traits::SIZE_BLOCK_CNT;
            if (get_randint(min_add_cnt) == 1)
                __sync_fetch_and_sub(&this->size, min_add_cnt);
        }
        else{
            __sync_fetch_and_sub(&this->size, 1);
        }
    }
    //atomic_version_type *version_array;
    //mutable RWLock* lock_array;
    mutable Lock* lock_array;
};

template<typename _Key,
        typename _Val,
        typename traits>
struct aex_data_node_con : public aex_static_data_node<_Key, _Val, traits>{
public:
    typedef _Key key_type;
    typedef _Val value_type;
    typedef aex_tree<key_type, value_type, traits> base_tree;
    typedef typename base_tree::components components;
    typedef typename components::base_node base_node;

    typedef aex_static_data_node<_Key, _Val, traits> base_data_node;

    aex_data_node_con() : base_data_node(){}
    
    aex_data_node_con(aex_data_node_con &other_node) : base_data_node(other_node){
        this->min_key = other_node.min_key;
    }

    aex_data_node_con(aex_data_node_con &&other_node) :base_data_node(other_node){
        this->min_key = other_node.min_key;
    }

    aex_data_node_con& operator = (aex_data_node_con &other_node) {
        *static_cast<base_data_node*>(this) = static_cast<base_data_node>(other_node);
        this->min_key = other_node.min_key;
        return *this;
    }

    aex_data_node_con& operator = (aex_data_node_con &&other_node) {
        *static_cast<base_data_node*>(this) = static_cast<base_data_node>(other_node);
        this->min_key = other_node.min_key;
        return *this;
    }

    inline void construct(const key_type *_key, const value_type *_data, int nums){
        this->base_data_node::construct(_key, _data, nums);
        this->min_key = _key[0];
    }

    inline void construct(const std::pair<key_type, value_type> *_data, int nums){
        this->base_data_node::construct(_data, nums);
        this->min_key = _data[0].first;
    }
    key_type min_key;
};

}
