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
    typedef typename components::version_type      version_type;
    typedef typename components::hash_node         hash_node;
    typedef typename components::hash_node_ptr     hash_node_ptr;
    typedef typename components::atomic_version_type      atomic_version_type;
    typedef typename components::size_type         size_type;
    using parent::bitmap_ptr;
    using parent::slot_size;
    using parent::hash_table;
    using parent::calc_slot_size;

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
        this->hash_table.clear();
    }

    inline void init(){
        this->size = 0;
        this->bitmap_ptr = new bitmap_base[this->slot_size / traits::SLOT_PER_LOCK + 1]();
        this->lock_array = new std::atomic<uint64_t>[this->slot_size / traits::SLOT_PER_LOCK + 1];
        for (slot_type i = 0; i < this->slot_size / traits::SLOT_PER_LOCK + 1; ++i)
            this->lock_array[i].store(0);
        AEX_PRINT("lock_array[0]=" << lock_array[0].load());
        this->hash_table.set(calc_slot_size(1.0 * slot_size * traits::HASH_NODE_FULL_RATIO / traits::HASH_TABLE_BLOCK_SIZE / traits::HASH_TABLE_FULL_RATIO));
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

    inline bool try_lock(const slot_type pos) {
        AEX_ASSERT(traits::AllowConcurrency);
        slot_type lock_pos = pos2slot(pos);
        uint64_t expected = lock_array[lock_pos].load() & (~(1 << (pos & 63)));
        uint64_t result   = expected | (1 << (pos & 63));
        if (!lock_array[lock_pos].compare_exchange_strong(expected, result)) {
            _mm_pause();
            return false;
        }
        return true;
    }

    inline void unlock(const slot_type pos) {
        AEX_ASSERT(traits::AllowConcurrency);
        slot_type lock_pos = pos2slot(pos);
        uint64_t expected = lock_array[lock_pos].load() | (1 << (pos & 63));
        uint64_t result   = expected & (~(1 << (pos & 63)));
        if (!lock_array[lock_pos].compare_exchange_weak(expected, result)) {
            expected = lock_array[lock_pos].load() | (1 << (pos & 63));
        }
    }

    inline uint64_t get_mask(const slot_type pos, const slot_type next_pos){
        slot_type v_pos = pos & 63, v_next_pos = next_pos & 63;
        AEX_ASSERT(v_pos <= v_next_pos);
        uint64_t mask = ((1ULL << (v_next_pos)) - 1) << v_pos;
        return mask;
    }

    inline bool try_lock_item(const slot_type pos, const slot_type next_pos){
        AEX_ASSERT(pos2slot(pos) == pos2slot(next_pos));
        slot_type lock_pos = pos2slot(pos);
        uint64_t mask = get_mask(pos, next_pos);
        uint64_t expected = lock_array[lock_pos].load() & (~mask);
        uint64_t result   = expected | mask;
        if (!lock_array[lock_pos].compare_exchange_strong(expected, result)) {
            _mm_pause();
            return false;
        }
        return true;
    }

    inline void unlock_item(const slot_type pos, const slot_type next_pos){
        AEX_ASSERT(pos2slot(pos) == pos2slot(next_pos));
        slot_type lock_slot = pos2slot(pos);
        uint64_t mask = get_mask(pos, next_pos);
        uint64_t expected = lock_array[lock_slot].load() | mask;
        uint64_t result   = expected & (~mask);
        while (!lock_array[lock_slot].compare_exchange_weak(expected, result)) {
            _mm_pause();
            expected = lock_array[lock_slot].load() | mask;
            result   = expected & (~mask);
        }
    }

    inline bool try_lock(const slot_type pos, const slot_type next_pos) {
        AEX_ASSERT(traits::AllowConcurrency);
        slot_type p=pos, q=((pos2slot(p) + 1) << 6) - 1;
        for (slot_type i = pos2slot(pos); i <= pos2slot(next_pos); ++i){
            if (q > next_pos) q = next_pos;
            bool res = try_lock_item(p, q);
            if (!res) {
                if (pos < p - 1) this->unlock(pos, p - 1);
                return false;
            }
            p = q + 1;
            q += 64;
        }
        return true;
    }

    inline void unlock(const slot_type pos, const slot_type next_pos) {
        slot_type p=pos, q=((pos2slot(p) + 1) << 6) - 1;
        for (slot_type i = pos2slot(pos); i <= pos2slot(next_pos); ++i){
            if (q > next_pos) q = next_pos;
            this->unlock_item(p, q);
            p = q + 1;
            q += 64;
        }
    }

    inline slot_type prev_item_find(slot_type x) const {
        const slot_type y = x & (~511);
        if (x <= 0)
            return x;
        bitmap text = bitmap_ptr + (x >> 6);
        const bitmap_base base = (*text) << (63 - (x & 63));
        if (base != 0)
            return x - _lzcnt_u64(base);
        x = (x & (~63)) - 1;
        if (x < y)
            return y;
        --text;
        while (x - 64 > y && (*text) == 0){
            --text;
            x -= 64;
        }
        x -= (*text == 0) ? 63 : _lzcnt_u64(*text);
        return x;
    }


    mutable std::atomic<uint64_t>* lock_array;
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


    inline int find(const key_type x) const {
        int pos;
            pos = linear_search_lower_bound<const key_type>(this->key, this->key + this->size, x) - this->key;
        if (pos >= this->size || this->key[pos] != x)
            return this->size;
        return pos;
    }

    inline int find_lower_pos(const key_type x) const {
        return linear_search_lower_bound<const key_type>(this->key, this->key + this->size, x) - this->key;
        
    }

    inline int find_upper_pos(const key_type x) const {
        return linear_search_upper_bound<const key_type>(this->key, this->key + this->size, x) - this->key;
    }

    key_type next_min_key;
};

}
