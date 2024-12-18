#pragma once

#include "aex/aex_node.h"
namespace aex{

template<typename _Key,
        typename _Val,
        typename traits>
struct aex_hash_node_con : public aex_hash_node<_Key, _Val, traits>{
    typedef _Key                                   key_type;
    typedef _Val                                   value_type;
    typedef aex_inner_node<_Key, _Val, traits>     inner_node;
    typedef aex_hash_node<_Key, _Val, traits>      parent;
    typedef aex_hash_node_con<_Key, _Val, traits>  self;
    typedef aex_tree<key_type, value_type, traits> base_tree;
    typedef typename base_tree::components         components;
    typedef typename parent::slot_type             slot_type;
    typedef typename parent::Model                 Model;
    typedef typename parent::bitmap                bitmap;
    typedef typename parent::bitmap_base           bitmap_base;
    typedef typename parent::bitmap_impl           bitmap_impl;
    typedef typename parent::node_ptr              node_ptr;
    typedef typename components::RWLock            RWLock;

    //aex_hash_node_con(slot_type slot_size):parent(slot_size){init();}
    //~aex_hash_node_con(){clear();}   
    aex_hash_node_con() = delete;
    ~aex_hash_node_con() = delete;
    aex_hash_node_con(aex_hash_node_con &other) = delete;
    aex_hash_node_con& operator = (aex_hash_node_con &other) = delete;

    void clear(){
        this->parent::clear();
        if (this->lock_array != nullptr){
            delete[] this->lock_array;
            this->lock_array = nullptr;
        }
    }

    void init(){
        this->parent::init();
        this->lock_array = new RWLock[this->slot_size / traits::SLOT_PER_LOCK + 1]();
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
            if (!lock_array[i].try_upgrade_lock_without_read()){
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
        if (pos < 0)
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
    }

    mutable RWLock* lock_array;
};

//template<typename _Key,
//        typename _Val,
//        typename traits>
//struct aex_data_node_con : public aex_static_data_node<_Key, _Val, traits>{
//    version_type version;
//    aex_data_node_con() :aex_static_data_node(), version(0){
//};

}
