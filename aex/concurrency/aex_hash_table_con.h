#pragma once
#include "../aex_utils.h"
#include "../aex_hash_table.h"

namespace aex{


template<typename _Key,
        typename traits>
struct alignas(64) aex_hash_table_block_con{
//struct aex_hash_table_block_con{
    typedef aex_default_components<traits> components;
    typedef typename components::base_node base_node;
    typedef base_node* node_ptr;
    typedef typename traits::slot_type slot_type;
    typedef typename traits::key_type key_type;
    typedef typename components::ID_type ID_type;
    typedef aex_hash_table_block_con<_Key, traits> self;
    typedef aex_hash_table_block_unit<_Key, traits> Unit;
    typedef aex_hash_table_block_unit_K<_Key, traits> Unit_K;
    typedef aex_hash_table_block_unit_V<_Key, traits> Unit_V;
    typedef typename components::RWLock RWLock;

    Unit          unit_array[traits::HASH_TABLE_BLOCK_SIZE];
    RWLock        lock;
    int           size;
    self*         next;

    aex_hash_table_block_con():lock(), size(0), next(nullptr){}

    self& operator=(self &other){
        for (self* b = this, *ob = &other; ob != nullptr; b = b->next, ob = ob->next){
            b->size = ob->size;
            memcpy(unit_array, ob->unit_array, sizeof(Unit) * traits::HASH_TABLE_BLOCK_SIZE);
            if (ob->next != nullptr){
                b->next = new self();
            }
        }
    }

    inline std::pair<key_type, node_ptr> find(const ID_type id, const slot_type _pos) {
        for (self* b = this; b != nullptr; b = b->next){
            for (int i = 0; i < b->size; ++i)
            if (b->unit_array[i].pos == _pos && b->unit_array[i].id == id)
                return std::make_pair(b->unit_array[i].key, b->unit_array[i].child);
        }
        return std::make_pair(0, nullptr);
    }

    inline bool exists(const ID_type id, const slot_type _pos) {
        for (self* b = this; b != nullptr; b = b->next){
            for (int i = 0; i < b->size; ++i)
            if (b->unit_array[i].pos == _pos && b->unit_array[i].id == id)
                return true;
        }
        return false;
    }

    inline void insert(const ID_type id, const slot_type _pos, const key_type x, const node_ptr y){
        self* insert_block = this;
        if (this->size == traits::HASH_TABLE_BLOCK_SIZE){
            if (this->next == nullptr){
                this->next = new self();
            }
            else if (this->next->size == traits::HASH_TABLE_BLOCK_SIZE){
                self* new_block = new self();
                new_block->next = this->next;
                this->next = new_block;
            }
            insert_block = this->next;
        }
        {
            int& _size = insert_block->size;
            insert_block->unit_array[_size].pos    = _pos;
            insert_block->unit_array[_size].id     = id;
            insert_block->unit_array[_size].key    = x;
            insert_block->unit_array[_size].child  = y;
            ++insert_block->size;
        }
    }

    inline void erase(const ID_type id, const slot_type _pos){
        self* erase_block = this;
        self* tail = (erase_block->next == nullptr) ? this : this->next;
        for(self* erase_block = this; erase_block != nullptr; erase_block = erase_block->next){
            for (int i = 0; i < erase_block->size; ++i)
                if (erase_block->unit_array[i].pos == _pos && erase_block->unit_array[i].id == id){
                    erase_block->unit_array[i]  = tail->unit_array[tail->size - 1];
                    --tail->size;
                    return;
                }
        }
        //AEX_PRINT("id=" << id << ", pos=" << _pos);
        AEX_ASSERT(0 == 1);
    }

    void update(const ID_type id, const slot_type _pos, const key_type update_key, const node_ptr update_node){
        for (self *b = this; b != nullptr; b = b->next){
            for (int i = 0; i < b->size; ++i)
                if (b->unit_array[i].pos == _pos && b->unit_array[i].id == id){
                    b->unit_array[i].key   = update_key;
                    b->unit_array[i].child = update_node;
                    return;
                }
        }
        AEX_ASSERT(0 == 1);
    }
};

template<typename traits>
struct alignas(64) _HashTableRescaleParams : public ConcurrencyParams{
    typedef _HashTableRescaleParams<traits> self;
    typedef aex_default_components<traits> components;
    typedef typename components::HashTableBlock HashTableBlock;
    typedef typename traits::slot_type slot_type;
    _HashTableRescaleParams():ConcurrencyParams(ConcurrencyType::HashTableRescale){}
    HashTableBlock* new_table; 
    HashTableBlock* old_table; 
    slot_type new_slot_size, n, size, start;
};

template<typename _Key,
        typename traits>
class aex_hash_table_con : public aex_hash_table<_Key, traits>{
public:
    typedef _Key       key_type;
    typedef typename traits::slot_type             slot_type;
    typedef aex_hash_table<_Key, traits>           HashTableBase;
    typedef aex_hash_table_con<_Key, traits>       self;
    typedef aex_default_components<traits>         components;
    typedef typename components::Index             Index;
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
    //typedef typename components::LockFreeStack     LockFreeStack;
    typedef typename components::LockFreeQueue     LockFreeQueue;
    typedef typename components::HashTableRescaleParams HashTableRescaleParams;
    typedef typename components::EpochBasedMemoryReclamationStrategy EpochBasedMemoryReclamationStrategy;
    

    //aex_hash_table_con(){}

    explicit aex_hash_table_con(int _slot_size):HashTableBase(_slot_size), work_queue(){
        AEX_SUCCESS("hash table construct");
        AEX_ASSERT((this->slot_size & (-this->slot_size)) == this->slot_size);
    }

    aex_hash_table_con(self &other_table):HashTableBase(other_table), work_queue(){
        AEX_SUCCESS("hash table construct");
    }

    aex_hash_table_con(self &&other_table):HashTableBase(std::move(other_table)), work_queue(){
        AEX_SUCCESS("hash table construct");
    }

    ~aex_hash_table_con(){}

    self& operator=(self &other){
        memcpy(this, &other, sizeof(HashTableBase));
        return *this;
    }

    inline ULL memory_used() const {
        return this->HashTableBase::memory_used() + this->slot_size * sizeof(RWLock);
    }

    inline void clear(){
        this->free_hash_table();
        this->slot_size = traits::MIN_HASH_TABLE_SIZE;
        this->size = 0;
        this->real_slot_size = this->get_real_slot_size(traits::MIN_HASH_TABLE_SIZE);
        this->table_ = new HashTableBlock[this->slot_size]();
    }

    //inline bool work_concurrency(){
    //    ConcurrencyParams *params;
    //    bool flag = this->work_queue.pop(params);
    //    if (flag) rescale_unit(static_cast<HashTableRescaleParams*>(params));
    //    return flag;
    //}
    
    inline bool work_concurrency(){
        ConcurrencyParams *params;
        bool flag = this->work_queue.pop(params);
        if (flag)
            rescale_unit(static_cast<HashTableRescaleParams*>(params));
        return flag;
    }

    inline void yield(const int count) {
        if (!this->work_queue.empty())
            while (this->work_concurrency());
        else
            _yield(count);
        //if (count <= 3){
        //    _mm_pause();
        //}
        //else{
        //    if (!this->work_queue.empty())
        //        while (this->work_concurrency());
        //}
            
    }

    inline void check_expand(){
    check_expand_start:
        bool need_restart = false;
        if (this->isfull()){
            //lock.upgradeToWriteLockOrRestart(table_version, need_restart);
            lock.writeLockOrRestart(need_restart);
            if (need_restart){
                yield(0);
                goto check_expand_start;
            }
            if (this->isfull()) 
                expand();
            lock.writeUnlock();
        }
    }
    
    //inline void rescale_unit(HashTableBlock* new_table, slot_type new_real_slot_size, HashTableBlock* old_table, const slot_type n){
    inline void rescale_unit(HashTableRescaleParams *worker){
        HashTableBlock* new_table = worker->new_table;
        const slot_type new_slot_size   = worker->new_slot_size;
        HashTableBlock* old_table = worker->old_table;
        const slot_type n = worker->n;
        worker->size = 0;
        for (int i = 0; i < n; ++i){
            old_table[i].lock.writeLock();
            for (HashTableBlock* b = old_table + i; b != nullptr; b = b->next){                
                worker->size += b->size;
                for (int j = 0; j < b->size; ++j){
                    hash_type new_hash_key = get_hash_key(b->unit_array[j].id, b->unit_array[j].pos, new_slot_size);
                    HashTableBlock *insert_block = new_table + new_hash_key;
                    insert_block->lock.writeLock();
                    insert_block->insert(b->unit_array[j].id, b->unit_array[j].pos, b->unit_array[j].key, b->unit_array[j].child);
                    insert_block->lock.writeUnlock();
                }
            }
        }
        worker->finish_flag = true;
    }

    inline void rescale_con(const slot_type _slot_size){
        AEX_ASSERT(this->lock.isLocked());
        AEX_ASSERT((_slot_size & (-_slot_size)) == _slot_size);
        AEX_ASSERT(_slot_size >= (slot_type)traits::MIN_HASH_TABLE_SIZE);
        slot_type new_real_slot_size = this->get_real_slot_size(_slot_size);
        HashTableBlock* old_hash_table = this->table_;
        slot_type old_slot_size = this->slot_size;
        HashTableBlock* new_hash_table = new HashTableBlock[_slot_size];

        AEX_WARNING("[hashtable rescale con] slot_size=" << this->slot_size << ", _slot_size=" << _slot_size << ", size=" << this->size << ", real_slot_size=" << this->real_slot_size << ", new_real_slot_size=" << new_real_slot_size);
        this->size = 0;

        const slot_type unit_size = traits::THREAD_UNIT_SIZE / traits::HASH_TABLE_BLOCK_SIZE * 2;
        //const slot_type unit_size = traits::THREAD_UNIT_SIZE;
        const int worker_num = this->slot_size / unit_size + (this->slot_size % unit_size != 0);
        //AEX_PRINT("unit_size=" << unit_size << "worker_num=" << worker_num);
        std::vector<HashTableRescaleParams> worker(worker_num);
        for (slot_type i = 0, pos = 0; i < worker_num; ++i, pos += unit_size){
            worker[i].new_table = new_hash_table;
            worker[i].new_slot_size = new_real_slot_size;
            worker[i].old_table = old_hash_table + pos;
            worker[i].n = std::min(unit_size, old_slot_size - pos);
            bool flag = this->work_queue.bounded_push(&worker[i]);
            while (!flag) {
                this->work_concurrency();
                flag = this->work_queue.bounded_push(&worker[i]);
                //this->work_queue.push(&worker[i]);
            }
        }

        while (this->work_concurrency());
        this->size = 0;
        for (int i = 0; i < worker_num; ++i){
            while (worker[i].finish_flag == false) {
                this->work_concurrency();
            }//join
            this->size += worker[i].size;
        }

        AEX_WARNING("real_size=" << this->size);
        HashTableBase* hash_table_copy = new HashTableBase();
        hash_table_copy->slot_size = this->slot_size;
        hash_table_copy->real_slot_size = this->real_slot_size;
        hash_table_copy->size = this->size;
        hash_table_copy->table_ = this->table_;
        this->slot_size = _slot_size;
        this->real_slot_size = new_real_slot_size;
        this->table_ = new_hash_table;
        this->ebr->scheduleForDeletion(MRUnit(MemoryReclaimType::HashTable, hash_table_copy));
        AEX_WARNING("[hashtable rescale con] finish");
    }
    
    inline void rescale(const slot_type _slot_size){
        AEX_ASSERT(this->lock.isLocked());
        AEX_ASSERT((_slot_size & (-_slot_size)) == _slot_size);
        AEX_ASSERT(_slot_size >= (slot_type)traits::MIN_HASH_TABLE_SIZE);
        if (this->slot_size >= traits::THREAD_UNIT_SIZE){
            this->rescale_con(_slot_size);
            return;
        }

        slot_type new_real_slot_size = this->get_real_slot_size(_slot_size);
        HashTableBlock* new_hash_table = new HashTableBlock[_slot_size];

        AEX_WARNING("[hashtable rescale] slot_size=" << this->slot_size << ", _slot_size=" << _slot_size << ", size=" << this->size << ", real_slot_size=" << this->real_slot_size << ", new_real_slot_size=" << new_real_slot_size);
        this->size = 0;
        for (slot_type i = 0; i < this->slot_size; ++i){
            this->table_[i].lock.writeLock();
            for (HashTableBlock* b = this->table_ + i; b != nullptr; b = b->next){                
                this->size += b->size;
                for (int j = 0; j < b->size; ++j){
                    hash_type new_hash_key = this->get_hash_key(b->unit_array[j].id, b->unit_array[j].pos, new_real_slot_size);
                    HashTableBlock* insert_block = new_hash_table + new_hash_key;
                    insert_block->insert(b->unit_array[j].id, b->unit_array[j].pos, b->unit_array[j].key, b->unit_array[j].child);
                }
            }
        }
        AEX_WARNING("real_size=" << this->size);
        HashTableBase* hash_table_copy = new HashTableBase();
        memcpy(hash_table_copy, this, sizeof(HashTableBase));

        this->slot_size = _slot_size;
        this->real_slot_size = new_real_slot_size;
        this->table_ = new_hash_table;
        this->ebr->scheduleForDeletion(MRUnit(MemoryReclaimType::HashTable, hash_table_copy));
        AEX_WARNING("rescale finish...");
    }

    //inline void narrow(){ rescale(this->slot_size >> 1); }
    //inline void expand(){ rescale(this->slot_size << 1); }
    inline void expand(){ rescale(this->slot_size << 2); }

    inline unsigned long long get_hash_key(const ID_type id, const slot_type pos, const slot_type _slot_size) const {
        return (reinterpret_cast<unsigned long long>(id) * traits::K1 + static_cast<unsigned long long>(pos) * traits::K2) % _slot_size;
        //return (reinterpret_cast<unsigned long long>(id) * traits::K1 + static_cast<unsigned long long>(pos) * traits::K2) % real_slot_size_copy;
    }

    inline unsigned long long get_hash_key(const ID_type id, const slot_type pos) const {
        return (reinterpret_cast<unsigned long long>(id) * traits::K1 + static_cast<unsigned long long>(pos) * traits::K2) % this->real_slot_size;
        //return (reinterpret_cast<unsigned long long>(id) * traits::K1 + static_cast<unsigned long long>(pos) * traits::K2) % real_slot_size_copy;
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

        version_type table_version = this->lock.readLockOrRestart(need_restart);
        if (need_restart) goto insert_start;
        hash_key = this->get_hash_key(parent->id, pos);
        block = this->table_ + hash_key;
        this->lock.readUnlockOrRestart(table_version, need_restart);
        if (need_restart) goto insert_start;
        
        block->lock.writeLockOrRestart(need_restart);
        if (need_restart) goto insert_start;
        block->insert(parent->id, pos, key, child);
        bool add_size_flag = add_size();
        block->lock.writeUnlock();
        if (add_size_flag) this->check_expand();
    }

    /**
     * @brief return the (node->key[pos], node->child[pos])
     */
    inline std::pair<key_type, node_ptr> find(const hash_node_ptr node, const slot_type pos) const {
        int restart_count = 0;
find_start:
        if (restart_count > 0)
            const_cast<self*>(this)->yield(restart_count);
        restart_count++;
        bool need_restart = false;

        //version_type table_version = this->lock.readLockOrRestart(need_restart);
        //if (need_restart) goto find_start;
        //const hash_type hash_key = this->get_hash_key(node->id, pos);
        //HashTableBlock* block = this->table_ + hash_key;
        //this->lock.readUnlockOrRestart(table_version, need_restart);
        //if (need_restart) goto find_start;

        version_type& _local_version = this->get_local_version();
        const hash_type hash_key = this->get_hash_key(node->id, pos);
        HashTableBlock* block = this->table_ + hash_key;
        this->lock.readUnlockOrRestart(_local_version, need_restart);
        if (need_restart){
            _local_version = this->lock.readLockOrRestart(need_restart);
            goto find_start;
        }

        //__builtin_prefetch((char*)(block) + 64);
        version_type block_version = block->lock.readLockOrRestart(need_restart);
        if (need_restart) goto find_start;
        std::pair<key_type, node_ptr> ret = block->find(node->id, pos);
        block->lock.readUnlockOrRestart(block_version, need_restart);
        if (need_restart) goto find_start;
        return ret;
    }

    inline std::pair<key_type, node_ptr> find(const hash_node_ptr node, const slot_type pos, bool &need_restart) const {
        std::pair<key_type, node_ptr> ret;
        version_type table_version = this->lock.readLockOrRestart(need_restart);
        if (need_restart) return ret;
        const hash_type hash_key = this->get_hash_key(node->id, pos);
        HashTableBlock* block = this->table_ + hash_key;
        this->lock.readUnlockOrRestart(table_version, need_restart);
        if (need_restart) return ret;
        
        version_type block_version = block->lock.readLockOrRestart(need_restart);
        if (need_restart) return ret;
        ret = block->find(node->id, pos);
        block->lock.readUnlockOrRestart(block_version, need_restart);
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
        version_type table_version = this->lock.readLockOrRestart(need_restart);
        if (need_restart) goto erase_start;
        hash_key = this->get_hash_key(node->id, pos);
        block = this->table_ + hash_key;
        this->lock.readUnlockOrRestart(table_version, need_restart);
        if (need_restart) goto erase_start;
        block->lock.writeLockOrRestart(need_restart);
        if (need_restart) goto erase_start;
        block->erase(node->id, pos);
        if (block->next != nullptr && block->next->size == 0){
            HashTableBlock* erase_block = block->next;
            block->next = erase_block->next;
            this->ebr->scheduleForDeletion(MRUnit(MemoryReclaimType::HashTableBlock, erase_block));
        }
        this->sub_size();
        block->lock.writeUnlock();
    }

    inline void update(const hash_node_ptr parent, const slot_type pos, const key_type update_key, const node_ptr update_node){
        int restart_count = 0;
        HashTableBlock* block;
    update_start:
        if (restart_count > 0)
            yield(restart_count);
        restart_count++;
        bool need_restart = false;
        version_type table_version = this->lock.readLockOrRestart(need_restart);
        if (need_restart) goto update_start;
        hash_type hash_key = this->get_hash_key(parent->id, pos);
        block = this->table_ + hash_key;
        this->lock.readUnlockOrRestart(table_version, need_restart);
        if (need_restart) goto update_start;
        block->lock.writeLockOrRestart(need_restart);
        if (need_restart) goto update_start;
        block->update(parent->id, pos, update_key, update_node);
        block->lock.writeUnlock();
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

        lock.readUnlockOrRestart(table_version, need_restart);
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
        ULL _size = 0;
        for (slot_type i = 0; i < this->slot_size; ++i){
            for (HashTableBlock* b = this->table_ + i; b != nullptr; b = b->next)
                _size += b->size;
        }
        AEX_HINT("[HashTable Stats] real_size=" << _size);
    }

    inline bool add_size() {
        bool flag = false;
        if (this->slot_size * traits::HASH_TABLE_BLOCK_SIZE * traits::HASH_TABLE_FULL_RATIO >= traits::SIZE_BLOCK_CNT * traits::MIN_ADD_CNT){
            size_type min_add_cnt = this->slot_size * traits::HASH_TABLE_BLOCK_SIZE * traits::HASH_TABLE_FULL_RATIO / traits::SIZE_BLOCK_CNT;
            if (get_randint(min_add_cnt) == 1){
                flag = true;
                __sync_fetch_and_add(&(this->size), min_add_cnt);
            }
        }
        else{
            flag = true;
            __sync_fetch_and_add(&(this->size), 1);
        }
        return flag;
    }

    inline void sub_size() {
        if (this->slot_size * traits::HASH_TABLE_BLOCK_SIZE * traits::HASH_TABLE_FULL_RATIO >= traits::SIZE_BLOCK_CNT * traits::MIN_ADD_CNT){
            size_type min_add_cnt = this->slot_size * traits::HASH_TABLE_BLOCK_SIZE * traits::HASH_TABLE_FULL_RATIO / traits::SIZE_BLOCK_CNT;
            //if (hash_key % min_add_cnt == 0){
            //if (utils_get_hash_key(hash_key, min_add_cnt) == 1)
            if (get_randint(min_add_cnt) == 1)
                __sync_fetch_and_sub(&this->size, min_add_cnt);
        }
        else{
            __sync_fetch_and_sub(&this->size, 1);
        }
    }

    /*inline HashTableBlock* get_block(const hash_node_ptr node, const slot_type pos) const {
        bool need_restart = false;
get_block_start:
        if (need_restart){
            if (!work_queue.empty())
                while (const_cast<self*>(this)->work_concurrency());
            need_restart = false;
        }
        version_type table_version = this->lock.readLockOrRestart(need_restart);
        if (need_restart) goto get_block_start;
        const hash_type hash_key = this->get_hash_key(node->id, pos);
        HashTableBlock* block = this->table_ + hash_key;
        this->lock.readUnlockOrRestart(table_version, need_restart);
        if (need_restart) goto get_block_start;
        return block;
    }*/

    inline HashTableBlock* get_block(const hash_node_ptr node, const slot_type pos) const {
get_block_start:
        bool need_restart = false;
        version_type& _local_version = this->get_local_version();
        const hash_type hash_key = this->get_hash_key(node->id, pos);
        HashTableBlock* block = this->table_ + hash_key;
        this->lock.readUnlockOrRestart(_local_version, need_restart);
        if (need_restart){
            if (!work_queue.empty())
                while (const_cast<self*>(this)->work_concurrency());
            _local_version = this->lock.readLockOrRestart(need_restart);
            goto get_block_start;
        }
        return block;
    }

    inline HashTableBlock* get_block(const hash_node_ptr node, const slot_type pos, bool &need_restart) const {
        version_type& _local_version = this->get_local_version();
        const hash_type hash_key = this->get_hash_key(node->id, pos);
        HashTableBlock* block = this->table_ + hash_key;
        this->lock.readUnlockOrRestart(_local_version, need_restart);
        if (need_restart){
            _local_version = this->lock.readLockOrRestart(need_restart);
        }
        return block;
    }

    /*
    inline self& getInstance(version_type& _table_version){
    getInstance_start:
        bool need_restart = false;
        this->lock.checkOrRestart(table_version, need_restart);
        if (need_restart) {
            yield(0);
            _instance = *this;
            table_version = this->lock.readLockOrRestart(need_restart);
            goto getInstance_start;
        }
        _table_version = table_version;
        return _instance;
    }

    inline self& getInstance(){
    getInstance_start:
        bool need_restart = false;
        this->lock.checkOrRestart(table_version, need_restart);
        if (need_restart) {
            yield(0);
            _instance = *this;
            table_version = this->lock.readLockOrRestart(need_restart);
            goto getInstance_start;
        }
        return _instance;
    }*/
    HashTableBlock* table_copy;
    mutable RWLock lock;
    //Index *index;
    EpochBasedMemoryReclamationStrategy *ebr;
    LockFreeQueue work_queue;
    
    //static inline HashTableBlock*& get_local_table(){
    //    static thread_local HashTableBlock* local_table = nullptr;
    //    return local_table;
    //}

    //static inline slot_type& get_local_slot_size(){
    //    static thread_local slot_type local_slot_size = traits::MIN_HASH_TABLE_SIZE;
    //    return local_slot_size;
    //}

    static inline version_type& get_local_version(){
        static thread_local version_type local_version = 1;
        return local_version;
    }
    //std::queue<ConcurrencyParams*> work_queue;
    //static thread_local self _instance;
    //static thread_local HashTableBlock* table_copy_;
    //static thread_local slot_type slot_size_copy;
    //static thread_local version_type table_version = -1;
    //LockFreeQueue work_queue;
};

//template<typename _Key, typename traits>
//thread_local typename aex_hash_table_con<_Key, traits>::version_type aex_hash_table_con<_Key, traits>::local_version = 1;

}