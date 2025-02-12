#pragma once
#include "../aex_utils.h"
#include "../aex_hash_table.h"

namespace aex{

//template<typename _Key,
//        typename traits>
//struct aex_hash_table_block_con : public aex_hash_table_block{
//    typedef aex_default_components<traits>         components;
//    typedef typename components::RWLock            RWLock;
//    RWLock lock;
//};

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
    typedef typename HashTableBase::hash_node_ptr  hash_node_ptr;
    typedef typename HashTableBase::HashTableBlock HashTableBlock;
    typedef typename HashTableBase::size_type      size_type;
    typedef typename components::HashTable         HashTable;
    typedef typename components::RWLock            RWLock;
    typedef typename components::ID_type           ID_type;
    typedef typename components::version_type      version_type;
    typedef typename components::MRUnit            MRUnit;
    typedef typename components::EpochBasedMemoryReclamationStrategy EpochBasedMemoryReclamationStrategy;

    aex_hash_table_con():HashTableBase(){}

    explicit aex_hash_table_con(int _slot_size):HashTableBase(_slot_size){
        AEX_SUCCESS("hash table construct");
        AEX_ASSERT((this->slot_size & (-this->slot_size)) == this->slot_size);
    }

    aex_hash_table_con(self &other_table):HashTableBase(other_table){
        AEX_SUCCESS("hash table construct");
    }

    aex_hash_table_con(self &&other_table):HashTableBase(std::move(other_table)){
        AEX_SUCCESS("hash table construct");
    }

    ~aex_hash_table_con(){}

    //void free_hash_table(){
    //    AEX_PRINT("free_hash_table");
    //    this->HashTableBase::free_hash_table();
    //}

    self& operator=(self &other){
        AEX_SUCCESS("hash table copy");
        *static_cast<HashTableBase*>(this) = static_cast<HashTableBase>(other);
        return *this;
    }

    self& operator=(self &&other){
        AEX_SUCCESS("hash table move");
        *static_cast<HashTableBase*>(this) = std::move(static_cast<HashTableBase>(other));
        return *this;
    }

    inline ULL memory_used() const {
        return this->HashTableBase::memory_used() + this->slot_size * sizeof(RWLock);
    }


    inline void clear(){
        this->free_hash_table();
        this->slot_size = traits::MIN_HASH_TABLE_SIZE;
        this->size = 0;
        this->table_ = new HashTableBlock[traits::MIN_HASH_TABLE_SIZE]();
        this->real_slot_size = this->get_real_slot_size(traits::MIN_HASH_TABLE_SIZE);
    }
    
    inline void rescale(const slot_type _slot_size){
        AEX_ASSERT((_slot_size & (-_slot_size)) == _slot_size);
        AEX_ASSERT(_slot_size >= (slot_type)traits::MIN_HASH_TABLE_SIZE);
        slot_type new_real_slot_size = this->get_real_slot_size(_slot_size);
        HashTableBlock* new_hash_table = new HashTableBlock[_slot_size];

        AEX_WARNING("[hashtable rescale] slot_size=" << this->slot_size << ", _slot_size=" << _slot_size << ", size=" << this->size << ", real_slot_size=" << this->real_slot_size << ", new_real_slot_size=" << new_real_slot_size);
        this->size = 0;
        for (slot_type i = 0; i < this->slot_size; ++i){
            for (HashTableBlock* b = this->table_ + i; b != nullptr; b = b->next){
                this->size += b->size;
                for (int j = 0; j < b->size; ++j){
                    hash_type new_hash_key = (reinterpret_cast<unsigned long long>(b->unit_array[j].id) * traits::K1 + static_cast<unsigned long long>(b->unit_array[j].pos) * traits::K2) % new_real_slot_size;
                    new_hash_table[new_hash_key].insert(b->unit_array[j].id, b->unit_array[j].pos, b->unit_array[j].key, b->unit_array[j].child);
                }
            }
        }
        HashTable* hash_table_copy = new HashTable();
        memcpy(hash_table_copy, this, sizeof(HashTable));
        this->ebr->scheduleForDeletion(MRUnit(MemoryReclaimType::HashTable, hash_table_copy));
        this->slot_size = _slot_size;
        this->real_slot_size = new_real_slot_size;
        this->table_ = new_hash_table;
    }

    //inline void narrow(){ rescale(this->slot_size >> 1); }
    inline void expand(){ rescale(this->slot_size << 1); }

    inline unsigned long long get_hash_key(const ID_type id, const slot_type pos) const {
        return (reinterpret_cast<unsigned long long>(id) * traits::K1 + static_cast<unsigned long long>(pos) * traits::K2) % this->real_slot_size;
    }

    inline void insert(const hash_node_ptr parent, const slot_type pos, const key_type key, const node_ptr child){
        hash_type hash_key;
        HashTableBlock* block;
        int restart_count = 0;
insert_start:
        if (restart_count > 0)
            yield(restart_count);
        restart_count++;
        bool need_restart = false;
        version_type table_version = lock.readLockOrRestart(need_restart);
        if (need_restart) goto insert_start;
        if (this->isfull()){
            lock.upgradeToWriteLockOrRestart(table_version, need_restart);
            if (need_restart) goto insert_start;
            if (this->isfull()) expand();
            table_version = lock.downgradeLock();
            if (need_restart) goto insert_start;
        }
        hash_key = this->get_hash_key(parent->id, pos);
        block = this->table_ + hash_key;
        lock.checkOrRestart(table_version, need_restart);
        if (need_restart) goto insert_start;
        block->lock.writeLockOrRestart(need_restart);
        if (need_restart) goto insert_start;
        block->insert(parent->id, pos, key, child);
        block->lock.writeUnlock();
        lock.readUnlockOrRestart(table_version, need_restart);
        if (need_restart) goto insert_start;
        add_size();
    }

    /**
     * @brief return the (node->key[pos], node->child[pos])
     */
    inline std::pair<key_type, node_ptr> find(const hash_node_ptr node, const slot_type pos) const {
        int restart_count = 0;
        hash_type hash_key;
find_start:
        if (restart_count > 0)
            yield(restart_count);
        restart_count++;
        bool need_restart = false;
        version_type table_version = lock.readLockOrRestart(need_restart);
        if (need_restart) goto find_start;
        hash_key = this->get_hash_key(node->id, pos);
        HashTableBlock* block = this->table_ + hash_key;
        lock.checkOrRestart(table_version, need_restart);
        if (need_restart) goto find_start;
        auto ret = block->find(node->id, pos);
        return ret;
    }

    /**
     * @brief erase node->child[pos]
     */
    inline void erase(const hash_node_ptr node, const slot_type pos){
        int restart_count = 0;
        hash_type hash_key;
        HashTableBlock* block;
    erase_start:
        if (restart_count > 0)
            yield(restart_count);
        restart_count++;
        bool need_restart = false;

        version_type table_version = lock.readLockOrRestart(need_restart);
        if (need_restart) goto erase_start;

        hash_key = this->get_hash_key(node->id, pos);
        block = this->table_ + hash_key;

        lock.checkOrRestart(table_version, need_restart);
        if (need_restart) goto erase_start;

        block->lock.writeLockOrRestart(need_restart);
        if (need_restart) goto erase_start;
//        if (this->isfew()){
//            if (!lock.try_upgrade_lock()){
//                lock.unlock_shared();
//                goto erase_start;
//            }
//            narrow();
//            lock.downgrade_lock();
//        }
        block->erase(node->id, pos);
        block->lock.writeUnlock();
        lock.readUnlockOrRestart(table_version, need_restart);
        if (need_restart) goto erase_start;
        sub_size();
    }

    inline void update(const hash_node_ptr parent, const slot_type pos, const key_type update_key, const node_ptr update_node){
        int restart_count = 0;
        HashTableBlock* block;
    update_start:
        if (restart_count > 0)
            yield(restart_count);
        restart_count++;
        bool need_restart = false;
        version_type table_version = lock.readLockOrRestart(need_restart);
        if (need_restart) goto update_start;
        hash_type hash_key = this->get_hash_key(parent->id, pos);
        block = this->table_ + hash_key;
        lock.checkOrRestart(table_version, need_restart);
        if (need_restart) goto update_start;
        block->lock.writeLockOrRestart(need_restart);
        block->update(parent->id, pos, update_key, update_node);
        block->lock.writeUnlock();
        lock.readUnlockOrRestart(table_version, need_restart);
        if (need_restart) goto update_start;
    }

    inline bool compare_and_swap(const hash_node_ptr parent, const slot_type pos, const node_ptr ori_node, const key_type update_key, const node_ptr update_node){
        int restart_count = 0;
        HashTableBlock* block;
    compare_and_swap_start:
        if (restart_count > 0)
            yield(restart_count);
        restart_count++;
        bool need_restart = false;
        version_type table_version = lock.readLockOrRestart(need_restart);
        if (need_restart) goto compare_and_swap_start;
        bool res = false;
        hash_type hash_key = this->get_hash_key(parent->id, pos);
        block = this->table_ + hash_key;

        lock.checkOrRestart(table_version, need_restart);
        if (need_restart) goto compare_and_swap_start;

        block->lock.writeLockOrRestart(need_restart);
        if (need_restart) goto compare_and_swap_start;
        key_type find_key;
        node_ptr find_node;
        std::tie(find_key, find_node) = block->find(parent->id, pos);
        if (find_node == ori_node){
            block->update(parent->id, pos, update_key, update_node);
            res = true;
        }
        block->lock.writeUnlock();
        lock.readUnlockOrRestart(table_version, need_restart);
        if (need_restart) goto compare_and_swap_start;
        return res;
    }

    inline void print_stats() const {
        this->HashTableBase::print_stats();
    }

    inline void add_size() {
        //if (this->slot_size * traits::HASH_TABLE_BLOCK_SIZE * traits::HASH_TABLE_FULL_RATIO / traits::BLOCK_SIZE >= 4){
        size_type min_add_cnt = this->slot_size * traits::HASH_TABLE_BLOCK_SIZE * traits::HASH_TABLE_FULL_RATIO / traits::SIZE_BLOCK_CNT;
        if (rand() % min_add_cnt == 0)
            __sync_fetch_and_add(&(this->size), min_add_cnt);
        /*
        if (this->slot_size * traits::HASH_TABLE_BLOCK_SIZE * traits::HASH_TABLE_FULL_RATIO >= traits::SIZE_BLOCK_CNT * traits::MIN_ADD_CNT){
            size_type min_add_cnt = this->slot_size * traits::HASH_TABLE_BLOCK_SIZE * traits::HASH_TABLE_FULL_RATIO / traits::SIZE_BLOCK_CNT;
            if (rand() % min_add_cnt == 0){
                __sync_fetch_and_add(&(this->size), min_add_cnt);
            }
        }
        else{
            __sync_fetch_and_add(&(this->size), 1);
        }
        */
    }

    inline void sub_size() {
        if (this->slot_size * traits::HASH_TABLE_BLOCK_SIZE * traits::HASH_TABLE_FULL_RATIO >= traits::SIZE_BLOCK_CNT * traits::MIN_ADD_CNT){
            size_type min_add_cnt = this->slot_size * traits::HASH_TABLE_BLOCK_SIZE * traits::HASH_TABLE_FULL_RATIO / traits::SIZE_BLOCK_CNT;
            if (rand() % min_add_cnt == 0){
                __sync_fetch_and_sub(&this->size, min_add_cnt);
            }
        }
        else{
            __sync_fetch_and_sub(&this->size, 1);
        }
    }

    mutable RWLock lock;
    EpochBasedMemoryReclamationStrategy *ebr;
};

}