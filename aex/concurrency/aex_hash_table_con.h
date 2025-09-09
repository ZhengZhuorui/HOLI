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

    inline std::pair<key_type, node_ptr> find(const slot_type _pos) {
        if constexpr (traits::HASH_TABLE_BLOCK_SIZE > 1){
            for (self* b = this; b != nullptr; b = b->next){
                for (int i = 0; i < b->size; ++i)
                if (b->unit_array[i].pos == _pos)
                    return std::make_pair(b->unit_array[i].key, b->unit_array[i].child);
            }
        }
        else{
            for (self* b = this; b != nullptr; b = b->next){
                if (b->unit_array[0].pos == _pos)
                    return std::make_pair(b->unit_array[0].key, b->unit_array[0].child);
            }
        }
        return std::make_pair(0, nullptr);
    }

    inline bool exists(const slot_type _pos) {
        for (self* b = this; b != nullptr; b = b->next){
            for (int i = 0; i < b->size; ++i)
            if (b->unit_array[i].pos == _pos)
                return true;
        }
        return false;
    }

    inline void insert(const slot_type _pos, const key_type x, const node_ptr y){
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
            insert_block->unit_array[_size].key    = x;
            insert_block->unit_array[_size].child  = y;
            ++insert_block->size;
        }
    }

    inline void erase(const slot_type _pos){
        //self* erase_block = this;
        //self* tail = (erase_block->next == nullptr) ? this : this->next;
        for(self* erase_block = this; erase_block != nullptr; erase_block = erase_block->next){
            for (int i = 0; i < erase_block->size; ++i)
                if (erase_block->unit_array[i].pos == _pos){
                    --erase_block->size;
                    //erase_block->unit_array[i]  = tail->unit_array[tail->size - 1];
                    //--tail->size;
                    return;
                }
        }
        //AEX_PRINT("id=" << id << ", pos=" << _pos);
        AEX_ASSERT(0 == 1);
    }

    void update(const slot_type _pos, const key_type update_key, const node_ptr update_node){
        for (self *b = this; b != nullptr; b = b->next){
            for (int i = 0; i < b->size; ++i)
                if (b->unit_array[i].pos == _pos){
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

    aex_hash_table_con(){}

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

    self& operator=(self &other){
        memcpy(this, &other, sizeof(HashTableBase));
        return *this;
    }


    //inline unsigned long long get_hash_key(const slot_type pos) const {
    //    return (static_cast<unsigned long long>(pos) * traits::K2) % this->slot_size;
    //}

    inline void insert(const slot_type pos, const key_type key, const node_ptr child){
        hash_type hash_key;
        HashTableBlock* block;
        int restart_count = 0;
insert_start:
        if (restart_count > 0)
            _yield(restart_count);
        restart_count++;
        bool need_restart = false;
        hash_key = this->get_hash_key(pos);
        block = this->table_ + hash_key;
        block->lock.writeLockOrRestart(need_restart);
        if (need_restart) goto insert_start;
        block->insert(pos, key, child);
        block->lock.writeUnlock();
    }

    /**
     * @brief return the (node->key[pos], node->child[pos])
     */
    inline std::pair<key_type, node_ptr> find(const slot_type pos) const {
        int restart_count = 0;
find_start:
        if (restart_count > 0)
            _yield(restart_count);
        restart_count++;
        bool need_restart = false;

        const hash_type hash_key = this->get_hash_key(pos);
        HashTableBlock* block = this->table_ + hash_key;

        //__builtin_prefetch((char*)(block) + 64);
        version_type block_version = block->lock.readLockOrRestart(need_restart);
        if (need_restart) goto find_start;
        std::pair<key_type, node_ptr> ret = block->find(pos);
        block->lock.readUnlockOrRestart(block_version, need_restart);
        if (need_restart) goto find_start;
        return ret;
    }

    inline std::pair<key_type, node_ptr> find(const slot_type pos, bool &need_restart) const {
        std::pair<key_type, node_ptr> ret;
        const hash_type hash_key = this->get_hash_key(pos);
        HashTableBlock* block = this->table_ + hash_key;
        
        version_type block_version = block->lock.readLockOrRestart(need_restart);
        if (need_restart) return ret;
        ret = block->find(pos);
        block->lock.readUnlockOrRestart(block_version, need_restart);
        return ret;
    }

    /**
     * @brief erase node->child[pos]
     */
    inline void erase(const slot_type pos){
        int restart_count = 0;
        hash_type hash_key;
        HashTableBlock* block;
    erase_start:
        if (restart_count > 0)
            _yield(restart_count);
        restart_count++;
        bool need_restart = false;
        hash_key = this->get_hash_key(pos);
        block = this->table_ + hash_key;
        block->lock.writeLockOrRestart(need_restart);
        if (need_restart) goto erase_start;
        block->erase(pos);
        //if (block->next != nullptr && block->next->size == 0){
        //    HashTableBlock* erase_block = block->next;
        //    block->next = erase_block->next;
        //    delete erase_block;
        //    //this->ebr->scheduleForDeletion(MRUnit(MemoryReclaimType::HashTableBlock, erase_block));
        //}
        block->lock.writeUnlock();
    }

    inline void update(const slot_type pos, const key_type update_key, const node_ptr update_node){
        int restart_count = 0;
        HashTableBlock* block;
    update_start:
        if (restart_count > 0)
            _yield(restart_count);
        restart_count++;
        bool need_restart = false;
        if (need_restart) goto update_start;
        hash_type hash_key = this->get_hash_key(pos);
        block = this->table_ + hash_key;
        if (need_restart) goto update_start;
        block->lock.writeLockOrRestart(need_restart);
        if (need_restart) goto update_start;
        block->update(pos, update_key, update_node);
        block->lock.writeUnlock();
    }

    inline bool compare_and_swap(const slot_type pos, const node_ptr ori_node, const key_type update_key, const node_ptr update_node){
        int restart_count = 0;
        HashTableBlock* block;
        bool res = false;
    compare_and_swap_start:
        if (restart_count > 0)
            _yield(restart_count);
        restart_count++;
        bool need_restart = false;
        hash_type hash_key = this->get_hash_key(pos);
        block = this->table_ + hash_key;

        block->lock.writeLockOrRestart(need_restart);
        if (need_restart) goto compare_and_swap_start;
        key_type find_key;
        node_ptr find_node;
        std::tie(find_key, find_node) = block->find(pos);
        if (find_node == ori_node){
            block->update(pos, update_key, update_node);
            res = true;
        }
        block->lock.writeUnlock();
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

};


}