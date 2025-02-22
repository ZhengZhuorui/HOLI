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
        //AEX_ASSERT(sizeof(aex_hash_node_con) == 96);
        //move_avx512((char*)(&other), (char*)(this));
        //move_avx256((char*)(&other) + 64, (char*)(this) + 64);
    }
    aex_hash_node_con& operator = (aex_hash_node_con &other){
        memcpy(this, &other, sizeof(self));
        //this->bitmap_ptr = other.bitmap_ptr;
        //this->model = other.model;
        //this->slot_size = other.slot_size;
        //this->id = other.id;
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
        this->lock_array = new RWLock[this->slot_size / traits::SLOT_PER_LOCK + 1];
    }

    inline void set_one(const slot_type x) {
        __sync_fetch_and_or(this->bitmap_ptr + pos2slot(x), 1ULL << (x & 63));
    }

    inline void set_zero(const slot_type x) {
        __sync_fetch_and_and(this->bitmap_ptr + pos2slot(x), ~(1ULL << (x & 63)));
    }

    inline size_type add_size_rand(){
        if (this->slot_size * traits::HASH_NODE_FULL_RATIO >= traits::SIZE_BLOCK_CNT * traits::MIN_ADD_CNT){
            size_type min_add_cnt = this->slot_size * traits::HASH_NODE_FULL_RATIO / traits::SIZE_BLOCK_CNT;
            if (get_randint(min_add_cnt) == 1)
                return min_add_cnt;
            else
                return 0;
        }
        else{
            return 1;
        }
    }
    inline size_type sub_size_rand(){
        if (this->slot_size * traits::HASH_NODE_FULL_RATIO >= traits::SIZE_BLOCK_CNT * traits::MIN_ADD_CNT){
            size_type min_add_cnt = this->slot_size * traits::HASH_NODE_FULL_RATIO / traits::SIZE_BLOCK_CNT;
            if (get_randint(min_add_cnt) == 1)
                return min_add_cnt;
            else
                return 0;
        }
        else{
            return 1;
        }
    }

    /*
    inline void arrayCheckOrRestart(const slot_type start, const slot_type end, const version_type tot_version, bool &need_restart) const {
        version_type version = 0;
        for (slot_type i = pos2slot(start); i <= pos2slot(end); ++i)
            version += version_array[i].load();
        if (version != tot_version)
            need_restart = true;
    }
    inline void arrayCheckOrRestart(const slot_type pos, const version_type version, bool &need_restart) const{
        if (version != version_array[pos2slot(pos)])
            need_restart = true;
    }

    inline void versionUpdate(const slot_type pos){
        ++version_array[pos2slot(pos)];
    }

    inline slot_type prev_item_con(slot_type x, version_type &version) const {
        if (x <= 0){
            return x;
        }
        if (pos2slot(x) != pos2slot(x + 1))
            version += version_array[pos2slot(x)].load();
        bitmap text = this->bitmap_ptr + (x >> 6);
        const bitmap_base base = (*text) << (63 - (x & 63));
        if (base != 0)
            return x - __builtin_clzll(base);
        x -= (x & 63) + 1;
        --text;
        version += version_array[pos2slot(x)].load();
        while ((*text) == 0){
            --text;
            x -= 64;
            version += version_array[pos2slot(x)].load();
        }
        x -= __builtin_clzll(*text);
        return x;
    }

    
    inline slot_type prev_item_find_con(slot_type x, version_type &version) const {
        if (x <= 0){
            return x;
        }
        const slot_type y = x - (x & (traits::SLOT_PER_SHORTCUT - 1));
        if (pos2slot(x) != pos2slot(x + 1))
        version += version_array[pos2slot(x)].load();
        bitmap text = this->bitmap_ptr + (x >> 6);
        const bitmap_base base = (*text) << (63 - (x & 63));
        if (base != 0)
            return x - __builtin_clzll(base);
        x -= (x & 63) + 1;
        if (x < y)
            return y;
        --text;
        version += version_array[pos2slot(x)].load();
        while (x - 64 > y && (*text) == 0){    
            --text;
            x -= 64;
            version += version_array[pos2slot(x)].load();
        }
        x -= __builtin_clzll(*text) - ((*text) == 0);
        return x;
    } 
    

    inline slot_type next_item_con(slot_type x, version_type &version) const {
        if (pos2slot(x) != pos2slot(x - 1))
            version += version_array[pos2slot(x)].load();
        if (x >= this->slot_size){
            return this->slot_size;
        }
        bitmap text = this->bitmap_ptr + (x >> 6);
        const bitmap_base base = (*text) >> (x & 63);
        if (base != 0)
            return x + __builtin_ctzll(base);
        x += 64 - (x & 63);
        ++text;
        version += version_array[pos2slot(x)].load();
        while (x < this->slot_size && (*text) == 0){
            ++text;
            x += 64;
            version += version_array[pos2slot(x)].load();
        }
        if (x < this->slot_size)
            x += __builtin_ctzll((*text));
        return x;
    }

    
    void array_lock(slot_type l_pos, slot_type r_pos) const {
        if (l_pos > r_pos)
            return;
        l_pos = std::max((slot_type)0, l_pos);
        r_pos = std::min(this->slot_size - 1, r_pos);
        for (slot_type i = pos2slot(l_pos); i <= pos2slot(r_pos); ++i)
            lock_array[i].lock();
    }


    void array_unlock(slot_type l_pos, slot_type r_pos) const {
        if (l_pos > r_pos)
            return;
        l_pos = std::max((slot_type)0, l_pos);
        r_pos = std::min(this->slot_size - 1, r_pos);
        for (slot_type i = pos2slot(l_pos); i <= pos2slot(r_pos); ++i){
            AEX_DEBUG_BLOCK({if (!lock_array[i].is_lock()) AEX_PRINT("i=" << i);});
            AEX_ASSERT(lock_array[i].is_lock());
            lock_array[i].unlock();
        }
    }

    void array_lock_shared(slot_type l_pos, slot_type r_pos) const {
        if (l_pos > r_pos)
            return;
        l_pos = std::max((slot_type)0, l_pos);
        r_pos = std::min(this->slot_size - 1, r_pos);
        for (slot_type i = pos2slot(l_pos); i <= pos2slot(r_pos); ++i)
            lock_array[i].lock_shared();
    }
    
    void array_unlock_shared(slot_type l_pos, slot_type r_pos) const {
        if (l_pos > r_pos)
            return;
        l_pos = std::max((slot_type)0, l_pos);
        r_pos = std::min(this->slot_size - 1, r_pos);
        for (slot_type i = pos2slot(l_pos); i <= pos2slot(r_pos); ++i){
            AEX_DEBUG_BLOCK({if constexpr(traits::AllowConcurrency) if (!lock_array[i].is_lock_shared()) AEX_ERROR("l_pos=" << l_pos << ", r_pos=" << r_pos << ",l_slot=" << pos2slot(l_pos) << ", i=" << i << ", lockCount=" << lock_array[i].lockCount.load()); });
            AEX_ASSERT(lock_array[i].is_lock_shared());
            lock_array[i].unlock_shared();
        }
    }

    inline bool try_array_upgrade_lock(slot_type l_pos, slot_type r_pos) const {
        if (l_pos > r_pos)
            return true;
        l_pos = std::max((slot_type)0, l_pos);
        r_pos = std::min(this->slot_size - 1, r_pos);
        for (slot_type i = pos2slot(l_pos); i <= pos2slot(r_pos); ++i){
            AEX_ASSERT(this->lock_array[i].is_lock_shared());
            if (!lock_array[i].try_upgrade_lock()){
                for (slot_type j = pos2slot(l_pos); j < i; ++j)
                    lock_array[j].downgrade_lock();
                return false;
            }
        }
        return true;
    }

    inline void array_downgrade_lock(slot_type l_pos, slot_type r_pos) const {
        if (l_pos > r_pos)
            return;
        l_pos = std::max((slot_type)0, l_pos);
        r_pos = std::min(this->slot_size - 1, r_pos);
        for (slot_type i = pos2slot(l_pos); i <= pos2slot(r_pos); ++i){
            AEX_ASSERT(lock_array[i].is_lock());
            lock_array[i].downgrade_lock();
        }
    }

    inline slot_type array_lock_shared_until_next_item(slot_type prev_pos, slot_type pos) const {
        slot_type x = pos;
        if (x >= this->slot_size)
            return x;
        if (pos2slot(pos) != pos2slot(prev_pos))
            lock_array[pos2slot(pos)].lock_shared();
        bitmap text = this->bitmap_ptr + (x >> 6);
        bitmap_base base = (*text) >> (x & 63);
        x += (base == 0) ? (64 - (x & 63)) : __builtin_ctzll(base);
        while (base == 0 && x < this->slot_size){
            lock_array[pos2slot(x)].lock_shared();
            ++text;
            base = *text;
            x += __builtin_ctzll(base);
        }
        return x;
    }

    inline slot_type try_array_lock_shared_until_prev_item(slot_type pos, bool &restart) const {
        if (pos <= 0)
            return 0;
        const slot_type end_slot = pos2slot(pos);
        AEX_ASSERT(lock_array[end_slot].is_lock_shared());
        slot_type x = pos;
        restart = false;

        bitmap text = this->bitmap_ptr + (x >> 6);
        bitmap_base base = (*text) << (63 - (x & 63));
        x -= (base == 0) ? ((x & 63) + 1) : __builtin_clzll(base);
        while (base == 0 && x > 0){
            if (!lock_array[pos2slot(x)].try_lock_shared()){
                restart = true;
                for (slot_type i = pos2slot(x) + 1; i < end_slot; ++i)
                    lock_array[i].unlock_shared();
                return x;
            }
            --text;
            base = *text;
            x -= __builtin_clzll(base);
        }
        if (pos2slot(x) != pos2slot(x - 1))
            if (!lock_array[pos2slot(x - 1)].try_lock_shared()){
                restart = true;
                for (slot_type i = pos2slot(x) ; i < end_slot; ++i)
                    lock_array[i].unlock_shared();
            }
        return x;
    }*/

    //atomic_version_type *version_array;
    mutable RWLock* lock_array;
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
