#pragma once
#include "aex/aex_utils.h"
#include "aex/aex_hash_table.h"

namespace aex{

template<typename _Key,
        typename traits>
class aex_hash_table_con : public aex_hash_table<_Key, traits>{
public:
    typedef _Key       key_type;
    typedef typename traits::slot_type             slot_type;
    typedef aex_hash_table<_Key, traits>           HashTableBase;
    typedef aex_hash_table_con<_Key, traits>       self;
    typedef aex_default_components<traits>         components;
    typedef typename HashTableBase::hash_type      hash_type;
    typedef typename HashTableBase::base_node      base_node;
    typedef typename HashTableBase::node_ptr       node_ptr;
    typedef typename HashTableBase::HashTableBlock HashTableBlock;
    typedef typename HashTableBase::size_type      size_type;
    typedef typename components::RWLock            RWLock;

    aex_hash_table_con():HashTableBase(){
        lock_array = new RWLock[this->slot_size]();
    }

    explicit aex_hash_table_con(int _slot_size):HashTableBase(_slot_size), con_stats(){
        AEX_ASSERT((this->slot_size & (-this->slot_size)) == this->slot_size);
        lock_array = new RWLock[this->slot_size]();
    }

    aex_hash_table_con(self &other_table):HashTableBase(other_table),  con_stats(){
        lock_array = new RWLock[this->slot_size]();
    }

    aex_hash_table_con(self &&other_table):HashTableBase(std::move(other_table)), con_stats(){
        this->lock_array = other_table.lock_array;
        other_table.lock_array = nullptr;
    }

    ~aex_hash_table_con(){
        delete[] this->lock_array;
    }

    self& operator=(self &other){
        static_cast<HashTableBase*>(this) = static_cast<HashTableBase*>(&other);
        AEX_ASSERT(this->lock_array != nullptr);
        delete[] lock_array;
        lock_array = new RWLock[this->slot_size]();
        return *this;
    }

    self& operator=(self &&other){
        AEX_ASSERT(this->lock_array != nullptr);
        static_cast<HashTableBase*>(this) = std::move(static_cast<HashTableBase*>(&other));
        delete[] lock_array;
        lock_array = other.lock_array;
        other.lock_array = nullptr;
        return *this;
    }

    inline ULL memory_used() const {
        return this->HashTableBase::memory_used() + this->slot_size * sizeof(RWLock);
    }

    void clear(){
        this->HashTableBase::clear();
        AEX_ASSERT(this->lock_array != nullptr);
        delete[] this->lock_array;
        lock_array = new RWLock[traits::MIN_HASH_TABLE_SIZE]();
    }

    inline void rescale(const slot_type _slot_size){
        //AEX_WARNING("[hashtable rescale con] slot_size=" << this->slot_size << ", _slot_size=" << _slot_size << ", size=" << this->size);
        this->HashTableBase::rescale(_slot_size);
        AEX_ASSERT(this->lock_array != nullptr);
        delete[] this->lock_array;
        this->lock_array = new RWLock[_slot_size]();
    }

    //inline void narrow(){ rescale(this->slot_size >> 1); }
    inline void expand(){ rescale(this->slot_size << 1); }

    inline void insert(const node_ptr parent, const slot_type pos, const key_type key, const node_ptr child){
        hash_type hash_key;
        int restart_count = 0;
        AEX_DEBUG_BLOCK({--con_stats.insert_restart_cnt;});
insert_start:
        AEX_DEBUG_BLOCK({++con_stats.insert_restart_cnt;});
        if (restart_count++)
            yield(restart_count);
        AEX_LOCK_SL_WAIT_CNT(lock);
        lock.lock_shared();
        if (this->isfull()){
            AEX_LOCK_XL_WAIT_CNT(lock);
            if (!lock.try_upgrade_lock()){
                lock.unlock_shared();
                goto insert_start;
            }
            expand();
            lock.downgrade_lock();
        }
        hash_key = this->get_hash_key(parent, pos);
        AEX_LOCK_XL_WAIT_CNT(lock_array[hash_key]);
        lock_array[hash_key].lock();
        this->table_[hash_key].insert(parent, pos, key, child);
        lock_array[hash_key].unlock();
        ++this->size;
        lock.unlock_shared();
    }

    /**
     * @brief return the (node->key[pos], node->child[pos])
     */
    inline std::pair<key_type, node_ptr> find(const node_ptr node, const slot_type pos) const {
        AEX_LOCK_SL_WAIT_CNT(lock);
        lock.lock_shared();
        const hash_type hash_key = this->get_hash_key(node, pos);
        AEX_LOCK_SL_WAIT_CNT(lock_array[hash_key]);
        lock_array[hash_key].lock_shared();
        auto ret = this->table_[hash_key].find(node, pos);
        lock_array[hash_key].unlock_shared();
        lock.unlock_shared();
        return ret;
    }

    inline bool exists(const node_ptr node, const slot_type pos) const {
        bool ret;
        lock.lock_shared();
        const hash_type hash_key = get_hash_key(node, pos);
        lock_array[hash_key].lock_shared();
        ret = this->table_[hash_key].exists(node, pos);
        lock_array[hash_key].unlock_shared();
        lock.unlock_shared();
        return ret;
    }

    /**
     * @brief erase node->child[pos]
     */
    inline void erase(const node_ptr node, const slot_type pos){
        hash_type hash_key;
        lock.lock_shared();
//erase_start:
//        if (this->isfew()){
//            if (!lock.try_upgrade_lock()){
//                lock.unlock_shared();
//                goto erase_start;
//            }
//            narrow();
//            lock.downgrade_lock();
//        }
        hash_key = this->get_hash_key(node, pos);
        lock_array[hash_key].lock();
        this->table_[hash_key].erase(node, pos);
        lock_array[hash_key].unlock();
        --this->size;
        lock.unlock_shared();
    }

    inline void update(const node_ptr parent, const slot_type pos, const key_type update_key, const node_ptr update_node){
        AEX_LOCK_SL_WAIT_CNT(lock);
        lock.lock_shared();
        const hash_type hash_key = this->get_hash_key(parent, pos);
        AEX_LOCK_XL_WAIT_CNT(lock_array[hash_key]);
        lock_array[hash_key].lock();
        this->table_[hash_key].update(parent, pos, update_key, update_node);
        lock_array[hash_key].unlock();
        lock.unlock_shared();
    }

    inline bool compare_and_swap(const node_ptr parent, const slot_type pos, const node_ptr ori_node, const key_type update_key, const node_ptr update_node){
        lock.lock_shared();
        bool res = false;
        const hash_type hash_key = this->get_hash_key(parent, pos);
        lock_array[hash_key].lock();
        key_type find_key;
        node_ptr find_node;
        std::tie(find_key, find_node) = this->table_[hash_key].find(parent, pos);
        if (find_node == ori_node){
            this->table_[hash_key].update(parent, pos, update_key, update_node);
            res = true;
        }
        lock_array[hash_key].unlock();
        lock.unlock_shared();
        return res;
    }

    inline void print_stats() const {
        this->HashTableBase::print_stats();
        this->con_stats.print_stats();
    }

    /*
    inline bool compare_and_swap(const node_ptr parent, const slot_type pos, const node_ptr ori_node, const slot_type copy_pos){
        lock.lock_shared();
        bool res = false;
        const hash_type hash_key1 = this->get_hash_key(parent, pos), hash_key2 = this->get_hash_key(parent, copy_pos);
        hash_type h1 = hash_key1, h2 = hash_key2;
        if (h1 > h2) std::swap(h1, h2);
        lock_array[h1].lock();
        lock_array[h2].lock();

        key_type find_key;
        node_ptr find_node;
        std::tie(find_key, find_node) = this->table_[hash_key1].find(parent, pos);
        if (find_node == ori_node){
            std::tie(find_key, find_node) = this->table_[hash_key2].find(parent, copy_pos);
            this->table_[hash_key1].update(parent, pos, find_key, find_node);
            res = true;
        }
        lock_array[h2].unlock();
        lock_array[h1].unlock();
        lock.unlock_shared();
        return res;
    }*/

    mutable RWLock* lock_array;
    mutable RWLock lock;
    mutable concurrency_stats con_stats;
};

}