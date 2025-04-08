#pragma once

#include "../aex_node.h"
namespace aex{

// bitmap is used as updatable memory in aex_hash_node_con
template<typename _Key,
        typename _Val,
        typename traits>
struct alignas(64) aex_hash_node_con : public aex_hash_node<_Key, _Val, traits>{
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
    typedef typename components::hash_node         hash_node;
    typedef typename components::hash_node_ptr     hash_node_ptr;
    typedef typename components::atomic_version_type      atomic_version_type;
    typedef typename components::size_type         size_type;

    //typedef components::pos2slot pos2slot;
    //using components::pos2slot;

    //aex_hash_node_con(slot_type slot_size):parent(slot_size){init();}
    //~aex_hash_node_con(){clear();}   
    aex_hash_node_con():parent(), lock_array(nullptr), copy(nullptr){};
    ~aex_hash_node_con(){};
    aex_hash_node_con(aex_hash_node_con &other){
        memcpy(this, &other, sizeof(self));
    }
    aex_hash_node_con& operator = (aex_hash_node_con &other){
        memcpy(this, &other, sizeof(self));
        return *this;
    }

    inline void clear(){
        if (this->bitmap_ptr != nullptr){
            delete[] this->bitmap_ptr;
            this->bitmap_ptr = nullptr;
        }
        if (this->lock_array != nullptr){
            delete[] this->lock_array;
            this->lock_array = nullptr;
        }
    }

    inline void init(){
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
    //aex_hash_node_copy copy_node;
    hash_node_ptr copy;
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
        this->next_min_key = other_node.next_min_key;
    }

    aex_data_node_con(aex_data_node_con &&other_node) :base_data_node(other_node){
        this->next_min_key = other_node.next_min_key;
    }

    aex_data_node_con& operator = (aex_data_node_con &other_node) {
        *static_cast<base_data_node*>(this) = static_cast<base_data_node>(other_node);
        this->next_min_key = other_node.next_min_key;
        return *this;
    }

    aex_data_node_con& operator = (aex_data_node_con &&other_node) {
        *static_cast<base_data_node*>(this) = static_cast<base_data_node>(other_node);
        this->next_min_key = other_node.next_min_key;
        return *this;
    }

    //inline void construct(const key_type *_key, const value_type *_data, int nums){
    //    this->base_data_node::construct(_key, _data, nums);
    //    this->next_min_key = _key[0];
    //}

    //inline void construct(const std::pair<key_type, value_type> *_data, int nums){
    //    this->base_data_node::construct(_data, nums);
    //    this->next_min_key = _data[0].first;
    //}

    inline int find(const key_type x) const {
        int pos;
        //if constexpr (sizeof(key_type) == 8 && traits::DATA_NODE_SLOT_SIZE == 16){
        //    pos = cmp_eq_epi64x16(this->key, x);
        //}
        //else{
            //pos = find_lower_pos(x);
            //int _size = std::min((int)this->size, traits::DATA_NODE_SLOT_SIZE);
            //pos = std::lower_bound(this->key, this->key + this->size, x) - this->key;
            pos = linear_search_lower_bound<const key_type>(this->key, this->key + this->size, x) - this->key;
        //}
        if (pos >= this->size || this->key[pos] != x)
            return this->size;
        return pos;
    }

    inline int find_lower_pos(const key_type x) const {
        if constexpr (std::is_same_v<typename traits::SearchClass, void> == false)
            return traits::SearchClass::lower_bound(this->key, this->key + this->size, x, this->key) - this->key;
        //return aex::linear_search_lower_bound(this->key, this->key + this->size, x) - this->key;
        //if constexpr (std::is_same_v<key_type, ULL>)
            //return std::lower_bound(this->key, this->key + this->size, x) - this->key;
        //else
        //return aex::linear_search_lower_bound_avx512<traits::DATA_NODE_SLOT_SIZE, key_type>(this->key, (int)this->size, x);
        return linear_search_lower_bound<const key_type>(this->key, this->key + this->size, x) - this->key;
        //return std::lower_bound(this->key, this->key + this->size, x) - this->key;
        
    }

    inline int find_upper_pos(const key_type x) const {
        if constexpr (std::is_same_v<typename traits::SearchClass, void> == false)
            return traits::SearchClass::upper_bound(this->key, this->key + this->size, x, this->key) - this->key;
        return linear_search_upper_bound<const key_type>(this->key, this->key + this->size, x) - this->key;
        //return std::upper_bound(this->key, this->key + this->size, x) - this->key;
        //return aex::linear_search_upper_bound_avx512<traits::DATA_NODE_SLOT_SIZE, key_type>(this->key, (int)this->size, x);
    }

    key_type next_min_key;
};

}
