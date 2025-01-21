#pragma once
#include "../aex_utils.h"
#include "../aex_hash_table.h"

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
    typedef typename HashTableBase::hash_node_ptr  hash_node_ptr;
    typedef typename HashTableBase::HashTableBlock HashTableBlock;
    typedef typename HashTableBase::size_type      size_type;
    typedef typename components::HashTable         HashTable;
    typedef typename components::RWLock            RWLock;
    typedef typename components::ID_type           ID_type;
    typedef typename components::version_type      version_type;
    typedef typename components::MRUnit            MRUnit;
    typedef typename components::EpochBasedMemoryReclamationStrategy EpochBasedMemoryReclamationStrategy;

    aex_hash_table_con():HashTableBase(){
    }

    explicit aex_hash_table_con(int _slot_size):HashTableBase(_slot_size){
        AEX_ASSERT((this->slot_size & (-this->slot_size)) == this->slot_size);
        lock_array = new RWLock[this->slot_size]();
    }

    aex_hash_table_con(self &other_table):HashTableBase(other_table){
        lock_array = new RWLock[this->slot_size]();
    }

    aex_hash_table_con(self &&other_table):HashTableBase(std::move(other_table)){
        this->lock_array = other_table.lock_array;
        other_table.lock_array = nullptr;
    }

    ~aex_hash_table_con(){}

    void free_hash_table(){
        this->HashTableBase::free_hash_table();
        delete[] this->lock_array;
    }

    self& operator=(self &other){
        *static_cast<HashTableBase*>(this) = static_cast<HashTableBase>(other);
        AEX_ASSERT(this->lock_array != nullptr);
        delete[] lock_array;
        this->lock_array = new RWLock[this->slot_size]();
        return *this;
    }

    self& operator=(self &&other){
        AEX_ASSERT(this->lock_array != nullptr);
        *static_cast<HashTableBase*>(this) = std::move(static_cast<HashTableBase>(other));
        delete[] lock_array;
        lock_array = other.lock_array;
        other.lock_array = nullptr;
        return *this;
    }

    inline ULL memory_used() const {
        return this->HashTableBase::memory_used() + this->slot_size * sizeof(RWLock);
    }


    inline void clear(){
        AEX_WARNING("clear");
        //HashTable* hash_table_copy = new HashTable();
        //memcpy(hash_table_copy, this, sizeof(HashTable));
        //this->ebr->scheduleForDeletion(MRUnit(MemoryReclaimType::HashTable, hash_table_copy));
        this->free_hash_table();
        AEX_PRINT("lock_array=" << (void*)this->lock_array);
        AEX_ASSERT(this->lock_array != nullptr);
        delete[] this->lock_array;
        this->lock_array = nullptr;
        this->slot_size = traits::MIN_HASH_TABLE_SIZE;
        this->size = 0;
        this->table_ = new HashTableBlock[traits::MIN_HASH_TABLE_SIZE]();
        this->real_slot_size = this->get_real_slot_size(traits::MIN_HASH_TABLE_SIZE);
        this->lock_array = new RWLock[traits::MIN_HASH_TABLE_SIZE]();
        AEX_PRINT("lock_array=" << (void*)this->lock_array);
        AEX_WARNING("lock_array[0].version=" << this->lock_array[0].typeVersionLockObsolete);
    }
    
    inline void rescale(const slot_type _slot_size){
        AEX_WARNING("_slot_size=" << _slot_size);
        AEX_ASSERT((_slot_size & (-_slot_size)) == _slot_size);
        AEX_ASSERT(_slot_size >= (slot_type)traits::MIN_HASH_TABLE_SIZE);
        slot_type new_real_slot_size = this->get_real_slot_size(_slot_size);
        HashTableBlock* new_hash_table = new HashTableBlock[_slot_size];

        AEX_WARNING("[hashtable rescale] slot_size=" << this->slot_size << ", _slot_size=" << _slot_size << ", size=" << this->size << ", real_slot_size=" << this->real_slot_size << ", new_real_slot_size=" << new_real_slot_size << ", table_=" << this->table_);
        this->size = 0;
        for (slot_type i = 0; i < this->slot_size; ++i){
            for (HashTableBlock* b = this->table_ + i; b != nullptr; b = b->next){
                AEX_PRINT("b=" << b);
                AEX_PRINT("i=" << i << ", b->size=" << b->size);
                this->size += b->size;
                for (int j = 0; j < b->size; ++j){
                    AEX_PRINT("j=" << j << ", " << b->unit_array[j].id << ", " << b->unit_array[j].pos);
                    hash_type new_hash_key = (reinterpret_cast<unsigned long long>(b->unit_array[j].id) * traits::K1 + static_cast<unsigned long long>(b->unit_array[j].pos) * traits::K2) % new_real_slot_size;
                    AEX_PRINT("hash_key=" << new_hash_key);
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
        this->lock_array = new RWLock[_slot_size]();
    }

    //inline void narrow(){ rescale(this->slot_size >> 1); }
    inline void expand(){ rescale(this->slot_size << 1); }

    inline unsigned long long get_hash_key(const ID_type id, const slot_type pos) const {
        return (reinterpret_cast<unsigned long long>(id) * traits::K1 + static_cast<unsigned long long>(pos) * traits::K2) % this->real_slot_size;
    }

    inline void insert(const hash_node_ptr parent, const slot_type pos, const key_type key, const node_ptr child){
        hash_type hash_key;
        HashTable table_copy;
        int restart_count = 0;
insert_start:
        if (restart_count > 0)
            yield(restart_count);
        restart_count++;
        bool need_restart = false;
        version_type table_version = lock.readLockOrRestart(need_restart);
        if (need_restart) goto insert_start;
        AEX_PRINT("1");
        if (this->isfull()){
            lock.upgradeToWriteLockOrRestart(table_version, need_restart);
            if (need_restart) goto insert_start;
            AEX_PRINT("2");
            if (this->isfull()) expand();
            table_version = lock.downgradeLock();
            AEX_PRINT("table_version=" << table_version);
            if (need_restart) goto insert_start;
        }
        AEX_PRINT("3");
        memcpy(&table_copy, this, sizeof(HashTable));
        lock.checkOrRestart(table_version, need_restart);
        AEX_PRINT("4");
        if (need_restart) goto insert_start;
        hash_key = table_copy.get_hash_key(parent->id, pos);
        AEX_WARNING("lock_array[0].version=" << this->lock_array[0].typeVersionLockObsolete);
        table_copy.lock_array[hash_key].writeLockOrRestart(need_restart);
        AEX_PRINT("hash key=" << hash_key << "need_restart=" << need_restart << ", version=" << table_copy.lock_array[hash_key].typeVersionLockObsolete);
        if (need_restart) exit(0);
        if (need_restart) goto insert_start;
        table_copy.table_[hash_key].insert(parent->id, pos, key, child);
        AEX_ASSERT(table_copy.table_[hash_key].size <= 16);
        //AEX_PRINT("hash key=" << hash_key << ", size=" << table_copy.table_[hash_key].size);
        table_copy.lock_array[hash_key].writeUnlock();
        lock.readUnlockOrRestart(table_version, need_restart);
        AEX_PRINT("6");
        if (need_restart) goto insert_start;
        add_size();
        AEX_PRINT("10");
    }

    /**
     * @brief return the (node->key[pos], node->child[pos])
     */
    inline std::pair<key_type, node_ptr> find(const hash_node_ptr node, const slot_type pos) const {
        int restart_count = 0;
        hash_type hash_key;
        HashTable table_copy;
find_start:
        if (restart_count > 0)
            yield(restart_count);
        restart_count++;
        bool need_restart = false;
        version_type table_version = lock.readLockOrRestart(need_restart);
        if (need_restart) goto find_start;
        memcpy(&table_copy, this, sizeof(HashTable));
        lock.checkOrRestart(table_version, need_restart);
        if (need_restart) goto find_start;
        hash_key = table_copy.get_hash_key(node->id, pos);
        version_type line_version = table_copy.lock_array[hash_key].readLockOrRestart(need_restart);
        if (need_restart) goto find_start;
        auto ret = table_copy.table_[hash_key].find(node->id, pos);
        table_copy.lock_array[hash_key].readUnlockOrRestart(line_version, need_restart);
        if (need_restart) goto find_start;
        lock.readUnlockOrRestart(table_version, need_restart);
        if (need_restart) goto find_start;
        return ret;
    }

    /**
     * @brief erase node->child[pos]
     */
    inline void erase(const hash_node_ptr node, const slot_type pos){
        int restart_count = 0;
        hash_type hash_key;
        HashTable table_copy;
    erase_start:
        if (restart_count > 0)
            yield(restart_count);
        restart_count++;
        bool need_restart = false;
        version_type table_version = lock.readLockOrRestart(need_restart);
        if (need_restart) goto erase_start;
        memcpy(&table_copy, this, sizeof(HashTable));
        lock.checkOrRestart(table_version, need_restart);
        if (need_restart) goto erase_start;

        hash_key = table_copy.get_hash_key(node->id, pos);
        table_copy.lock_array[hash_key].writeLockOrRestart(need_restart);
        if (need_restart) goto erase_start;
//        if (this->isfew()){
//            if (!lock.try_upgrade_lock()){
//                lock.unlock_shared();
//                goto erase_start;
//            }
//            narrow();
//            lock.downgrade_lock();
//        }
        table_copy.table_[hash_key].erase(node->id, pos);
        table_copy.lock_array[hash_key].writeUnlock();
        lock.readUnlockOrRestart(table_version, need_restart);
        if (need_restart) goto erase_start;
        sub_size();
    }

    inline void update(const hash_node_ptr parent, const slot_type pos, const key_type update_key, const node_ptr update_node){
        int restart_count = 0;
        HashTable table_copy;
    update_start:
        if (restart_count > 0)
            yield(restart_count);
        restart_count++;
        bool need_restart = false;
        version_type table_version = lock.readLockOrRestart(need_restart);
        if (need_restart) goto update_start;
        memcpy(&table_copy, this, sizeof(HashTable));
        lock.checkOrRestart(table_version, need_restart);
        if (need_restart) goto update_start;
        hash_type hash_key = table_copy.get_hash_key(parent->id, pos);
        table_copy.lock_array[hash_key].writeLockOrRestart(need_restart);
        if (need_restart) goto update_start;
        table_copy.table_[hash_key].update(parent->id, pos, update_key, update_node);
        table_copy.lock_array[hash_key].writeUnlock();
        lock.readUnlockOrRestart(table_version, need_restart);
        if (need_restart) goto update_start;
    }

    inline bool compare_and_swap(const hash_node_ptr parent, const slot_type pos, const node_ptr ori_node, const key_type update_key, const node_ptr update_node){
        int restart_count = 0;
        HashTable table_copy;
    compare_and_swap_start:
        if (restart_count > 0)
            yield(restart_count);
        restart_count++;
        bool need_restart = false;
        version_type table_version = lock.readLockOrRestart(need_restart);
        if (need_restart) goto compare_and_swap_start;
        memcpy(&table_copy, this, sizeof(HashTable));
        lock.checkOrRestart(table_version, need_restart);
        if (need_restart) goto compare_and_swap_start;

        bool res = false;
        hash_type hash_key = table_copy.get_hash_key(parent->id, pos);
        table_copy.lock_array[hash_key].writeLockOrRestart(need_restart);
        if (need_restart) goto compare_and_swap_start;
        key_type find_key;
        node_ptr find_node;
        std::tie(find_key, find_node) = table_copy.table_[hash_key].find(parent->id, pos);
        if (find_node == ori_node){
            table_copy.table_[hash_key].update(parent->id, pos, update_key, update_node);
            res = true;
        }
        table_copy.lock_array[hash_key].writeUnlock();
        lock.readUnlockOrRestart(table_version, need_restart);
        if (need_restart) goto compare_and_swap_start;
        return res;
    }

    inline void print_stats() const {
        this->HashTableBase::print_stats();
        //this->con_stats.print_stats();
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
    EpochBasedMemoryReclamationStrategy *ebr;
};

}