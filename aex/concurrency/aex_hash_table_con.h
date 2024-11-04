#pragma once
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

    aex_hash_table_con():aex_hash_table_con(){
        lock_array = new RWLock[this->slot_size];
    }

    explicit aex_hash_table_con(int _slot_size):HashTableBase(_slot_size){
        AEX_ASSERT((this->slot_size & (-this->slot_size)) == this->slot_size);
        lock_array = new RWLock[this->slot_size];
    }

    aex_hash_table_con(self &other_table):HashTableBase(other_table){
        lock_array = new RWLock[this->slot_size];
    }

    aex_hash_table_con(self &&other_table):HashTableBase(std::move(other_table)){
        lock_array = other_table.lock_array;
        other_table.lock_array = other_table.lock_array;
        other_table.lock_array = nullptr;
    }

    self& operator=(self &other){
        static_cast<HashTableBase*>(this) = static_cast<HashTableBase*>(&other);
        if (lock_array != nullptr)
            delete lock_array;
        lock_array = new RWLock[this->slot_size];
        return *this;
    }

    self& operator=(self &&other){
        static_cast<HashTableBase*>(this) = std::move(static_cast<HashTableBase*>(&other));
        this->slot_size = other.slot_size;
        this->table_ = other.table_;
        lock_array = other.lock_array;
        other.lock_array = nullptr;
        return *this;
    }

    ~aex_hash_table_con(){
        destory();
    }

    inline size_t memory_used(){
        return this->HashTableBase::memory_used() + this->slot_size * sizeof(RWLock);
    }

    inline void destory(){
        this->HashTableBase::destory();
        if (this->lock_array != nullptr)
            delete this->lock_array;
        this->lock_array = nullptr;
    }

    inline void rescale(const slot_type _slot_size){
        this->HashTableBase::rescale(_slot_size);
        delete this->lock_array;
        this->lock_array = new RWLock[_slot_size];
    }

    inline void narrow(){
        rescale(this->table_->slot_size >> 1);
    }

    inline void expand(){
        rescale(this->table_->slot_size << 1);
    }

    inline void insert(const node_ptr ori_node, const slot_type pos, const key_type key, const node_ptr child){
        hash_type hash_key;
insert_start:
        lock.lock_shared();
        hash_key = get_hash_key(ori_node, pos);
        if (this->table_->block[hash_key].size == traits::HASH_TABLE_BLOCK_SIZE && this->isfull()){
            if (!lock.try_upgrade_lock()){
                lock.unlock_shared();
                goto insert_start;
            }
            lock_array[hash_key].unlock();
            expand();
            lock.downgrade_lock();
        }
        hash_key = get_hash_key(ori_node, pos);
        lock_array[hash_key].lock();
        this->table_[hash_key]->insert(ori_node, pos, key, child);
        lock_array[hash_key].unlock();
        ++this->size;
        lock.unlock_shared();
    }

    /**
     * @brief return the ori_node->child[pos] if ori_node->key[pos] > key
     */
    inline node_ptr find(const node_ptr ori_node, const slot_type pos, const key_type key) {
        lock.lock_shared();
        hash_type hash_key = get_hash_key(ori_node, pos);
        lock_array[hash_key].lock_shared();
        node_ptr ret = this->table_[hash_key]->find(ori_node, pos, key);
        lock_array[hash_key].unlock_shared();
        lock.unlock_shared();
        return ret;        
    }

    /**
     * @brief return the (ori_node->key[pos], ori_node->child[pos])
     */
    inline std::pair<key_type, node_ptr> find(const slot_type ori_node, const slot_type pos) {
        lock.lock_shared();
        hash_type hash_key = get_hash_key(ori_node, pos);
        lock_array[hash_key].lock_shared();
        auto ret = this->table_[hash_key]->find(ori_node, pos);
        lock_array[hash_key].unlock_shared();
        lock.unlock_shared();
        return ret;
    }

    /**
     * @brief erase ori_node->child[pos] if ori_node->child[pos] == child
     */
    inline bool erase(const node_ptr ori_node, const slot_type pos){
        hash_type hash_key;
erase_start:
        lock.lock_shared();
        if (this->isfew()){
            if (!lock.try_upgrade_lock()){
                lock.unlock_shared();
                goto erase_start;
            }
            narrow();
            lock.downgrade_lock();
        }
        hash_key = get_hash_key(ori_node, pos);
        lock_array[hash_key].lock();
        bool ret = this->table_[hash_key]->erase(ori_node, pos);
        lock_array[hash_key].unlock();
        if (ret)
            --this->size;
        lock.unlock_shared();
        return ret;
    }

    inline bool update(const node_ptr ori_node, const slot_type pos, const key_type update_key, const node_ptr update_node){
        lock.lock_shared();
        hash_type hash_key = get_hash_key(ori_node, pos);
        lock_array[hash_key].lock();
        bool ret = this->table_[hash_key]->update(ori_node, pos, update_key, update_node);
        lock_array[hash_key].unlock();
        AEX_ASSERT(ret == false);
        lock.unlock_shared();
        return ret;
    }

    RWLock* lock_array;
    std::atomic<size_t> size;
    RWLock lock;
};

}