#pragma once

namespace aex{

template<typename _Key,
        typename traits>
struct aex_hash_table_block_unit_K{
    typedef aex_default_components<traits> components;
    typedef typename traits::slot_type slot_type;
    typedef typename traits::key_type key_type;
    typedef typename components::base_node base_node;
    typedef typename components::ID_type ID_type;
    ID_type id;
    slot_type pos;
};

template<typename _Key,
        typename traits>
struct aex_hash_table_block_unit_V{
    
    typedef aex_default_components<traits> components;
    typedef typename traits::slot_type slot_type;
    typedef typename traits::key_type key_type;
    typedef typename components::base_node base_node;
    typedef typename components::node_ptr node_ptr;
    typedef typename components::ID_type ID_type;
    key_type key;
    node_ptr child;
};

//#pragma pack(push)
//#pragma pack(1)
template<typename _Key,
        typename traits>
struct alignas(32) aex_hash_table_block_unit{
    typedef aex_default_components<traits> components;
    typedef typename traits::slot_type slot_type;
    typedef typename traits::key_type key_type;
    typedef typename components::base_node base_node;
    typedef typename components::ID_type ID_type;
    typedef base_node* node_ptr;
    slot_type pos;
    ID_type id;
    key_type key;
    node_ptr child;
};
//#pragma pack(pop)

template<typename _Key,
        typename traits>
struct aex_hash_table_block{
    typedef aex_default_components<traits> components;
    typedef typename components::base_node base_node;
    typedef base_node* node_ptr;
    typedef typename traits::slot_type slot_type;
    typedef typename traits::key_type key_type;
    typedef typename components::ID_type ID_type;
    typedef aex_hash_table_block<_Key, traits> self;
    typedef aex_hash_table_block_unit<_Key, traits> Unit;
    typedef aex_hash_table_block_unit_K<_Key, traits> Unit_K;
    typedef aex_hash_table_block_unit_V<_Key, traits> Unit_V;
    typedef typename components::RWLock RWLock;

    Unit          unit_array[traits::HASH_TABLE_BLOCK_SIZE];
    int           size;
    self*         next;

    aex_hash_table_block():size(0), next(nullptr){}

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
                    if (tail != this && tail->size == 0){
                        this->next = tail->next;
                        delete tail;
                    }
                    return;
                }
        }
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


template<typename _Key,
        typename traits>
class aex_hash_table{
public:
    typedef _Key     key_type;
    typedef aex_default_components<traits>        components;
    typedef typename traits::slot_type            slot_type;
    typedef typename traits::hash_type            hash_type;
    typedef typename components::size_type        size_type;
    typedef typename components::atomic_size_type atomic_size_type;
    typedef typename components::base_node        base_node;
    typedef typename components::node_ptr         node_ptr;
    typedef typename components::hash_node_ptr    hash_node_ptr;
    typedef typename components::HashTableBlock   HashTableBlock;
    //typedef typename HashTableBlock::Unit         Unit;
    typedef typename components::HashTable        HashTable;
    typedef typename components::MRUnit           MRUnit;
    typedef typename components::ID_type          ID_type;
    typedef typename components::EpochBasedMemoryReclamationStrategy EpochBasedMemoryReclamationStrategy;

    typedef aex_hash_table<_Key, traits> self;

    aex_hash_table(){}

    explicit aex_hash_table(LL _slot_size):slot_size(_slot_size){
        size = 0;
        AEX_ASSERT((slot_size & (-slot_size)) == slot_size);
        table_ = new HashTableBlock[slot_size]();
        real_slot_size = get_real_slot_size(slot_size);
    }

    aex_hash_table(self &other_table):slot_size(other_table.slot_size), real_slot_size(other_table.real_slot_size){
        size = other_table.size.load();
        table_ = new HashTableBlock[slot_size]();
        for (slot_type i = 0; i < this->slot_size; ++i)
            table_[i] = other_table.table_[i];
        
    }

    aex_hash_table(self &&other_table):slot_size(other_table.slot_size), real_slot_size(other_table.real_slot_size){
        size = other_table.size.load();
        if (slot_size < traits::MIN_ML_INNER_NODE_SIZE){
            table_ = nullptr;
            return;
        }
        this->slot_size = other_table.slot_size;
        this->table_ = other_table.table_;
        other_table.table_ = nullptr;
    }

    ~aex_hash_table(){}

    self& operator = (self &other_table){
        AEX_ASSERT(this->slot_size == other_table.slot_size);
        AEX_ASSERT(table_ != nullptr);
        if (this->table_ != nullptr)
            free_hash_table(table_);
        table_ = new HashTableBlock[slot_size]();
        for (slot_type i = 0; i < this->slot_size; ++i){
            table_[i] = other_table.table_[i];
        }
        this->real_slot_size = other_table.real_slot_size;
        this->size = other_table.size;
        return *this;
    }

    self& operator = (self &&other_table){
        AEX_ASSERT(table_ != nullptr);
        if (this->table_ != nullptr)
            free_hash_table(table_);
        this->slot_size = other_table.slot_size;
        this->real_slot_size = other_table.real_slot_size;
        this->size = other_table.size;
        this->table_ = other_table.table_;
        other_table.table_ = nullptr;
        return *this;
    }

    inline slot_type get_real_slot_size(const slot_type _slot_size) const {
        for (slot_type i = _slot_size; i >= 0; --i)
            if (is_prime(i)){
                return i;
                break;
            }
        return 0;
    }

    inline ULL memory_used() const{
        if (table_ == nullptr) return 0;
        else{
            ULL ret = sizeof(HashTableBlock) * this->slot_size;
            for (slot_type i = 0; i < this->slot_size; ++i)
                for (HashTableBlock *b = table_[i].next; b != nullptr; b = b->next)
                    ret += sizeof(HashTableBlock);
            return ret;
        }
    }

    inline void print_stats() const {
        AEX_HINT("Block size=" << sizeof(HashTableBlock));
        AEX_HINT("[HashTable Stats]: size=" << size << ", slot_size=" << slot_size);
        long long cnt = 0;
        for (slot_type i = 0; i < this->slot_size; ++i){
            cnt += (table_[i].next != nullptr);
        }
        AEX_HINT("collision cnt=" << cnt);
    }

    void free_hash_table(){
        for (slot_type i = 0; i < this->slot_size; ++i){
            for (HashTableBlock *b = this->table_[i].next, *t; b != nullptr; ){
                t = b;
                b = b->next;
                delete t;
            }
        }
        delete[] this->table_;
        this->table_ = nullptr;
    }

    inline unsigned long long get_hash_key(const ID_type id, const slot_type pos, const slot_type _slot_size) const {
        return (reinterpret_cast<unsigned long long>(id) * traits::K1 + static_cast<unsigned long long>(pos) * traits::K2) % _slot_size;
    }

    inline unsigned long long get_hash_key(const ID_type id, const slot_type pos) const {
        return (reinterpret_cast<unsigned long long>(id) * traits::K1 + static_cast<unsigned long long>(pos) * traits::K2) % slot_size;
    }

    inline void clear(){
        slot_size = traits::MIN_HASH_TABLE_SIZE;
        this->free_hash_table();
        this->size = 0;
        table_ = new HashTableBlock[slot_size]();
        real_slot_size = get_real_slot_size(slot_size);
    }

    inline bool isfull() const {
        return 1.0 * this->size / (this->slot_size * traits::HASH_TABLE_BLOCK_SIZE) >= traits::HASH_TABLE_FULL_RATIO;
    }

    //inline bool isfew() const {
    //    return allow_narrow && this->slot_size > (slot_type)traits::MIN_HASH_TABLE_SIZE && 1.0 * this->size / this->slot_size < traits::HASH_TABLE_FEW_RATIO;
    //}
    
    inline void rescale(const slot_type _slot_size){
        AEX_ASSERT((_slot_size & (-_slot_size)) == _slot_size);
        AEX_ASSERT(_slot_size >= (slot_type)traits::MIN_HASH_TABLE_SIZE);
        slot_type new_real_slot_size = get_real_slot_size(_slot_size);
        HashTableBlock* new_hash_table = new HashTableBlock[_slot_size]();
        AEX_WARNING("[hashtable rescale] slot_size=" << this->slot_size << ", _slot_size=" << _slot_size << ", size=" << this->size << ", real_slot_size=" << this->real_slot_size << ", new_real_slot_size=" << new_real_slot_size);
        for (slot_type i = 0; i < this->slot_size; ++i){
            for (HashTableBlock* b = table_ + i; b != nullptr; b = b->next){
                for (int j = 0; j < b->size; ++j){
                    //hash_type new_hash_key = (reinterpret_cast<unsigned long long>(b->unit_array[j].id) * traits::K1 + static_cast<unsigned long long>(b->unit_array[j].pos) * traits::K2) % new_real_slot_size;
                    hash_type new_hash_key = get_hash_key(b->unit_array[j].id, b->unit_array[j].pos, _slot_size);
                    new_hash_table[new_hash_key].insert(b->unit_array[j].id, b->unit_array[j].pos, b->unit_array[j].key, b->unit_array[j].child);
                    //hash_type new_hash_key = (reinterpret_cast<unsigned long long>(b->id[j]) * traits::K1 + static_cast<unsigned long long>(b->pos[j]) * traits::K2) % new_real_slot_size;
                    //new_hash_table[new_hash_key].insert(b->id[j], b->pos[j], b->unit_array_V[j].key, b->unit_array_V[j].child);
                }
            }
        }
        this->free_hash_table();
        table_ = new_hash_table;
        this->slot_size = _slot_size;
        this->real_slot_size = new_real_slot_size;
    }

    //inline void narrow(){ rescale(this->slot_size >> 1); }
    inline void expand(){ rescale(this->slot_size << 1); }
    //inline void expand(){ rescale(this->slot_size << 2); }

    /**
     * @brief insert <node, pos>: <node->key[pos], node->child[pos]> in hash table
     */
    inline void insert(const hash_node_ptr node, const slot_type pos, const key_type key, const node_ptr child){
        AEX_ASSERT(this->find(node, pos).second == nullptr);
        hash_type hash_key = get_hash_key(node->id, pos);
        //AEX_PRINT("id=" << id << ", pos=" << pos << ", hash_key=" << hash_key);
        if (this->isfull()){
            expand();
            hash_key = get_hash_key(node->id, pos);
        }
        table_[hash_key].insert(node->id, pos, key, child);
        ++size;
    }    

    /**
     * @brief return the (node->key[pos], node->child[pos])
     */
    inline std::pair<key_type, node_ptr> find(const hash_node_ptr node, const slot_type pos) const {
        const hash_type hash_key = get_hash_key(node->id, pos);
        //__builtin_prefetch((char*)(table_ + hash_key) + 64);
        return table_[hash_key].find(node->id, pos);
    }

    inline bool exists(const hash_node_ptr node, const slot_type pos) const {
        const hash_type hash_key = get_hash_key(node->id, pos);
        return table_[hash_key].exists(node->id, pos);
    }

    /**
     * @brief erase node->array[pos]. Return true if node->array[pos] exists.
     */
    inline void erase(const hash_node_ptr node, const slot_type pos){
        //if (isfew())
        //    narrow();
        const hash_type hash_key = get_hash_key(node->id, pos);
        table_[hash_key].erase(node->id, pos);
        --size;
    }

    inline void update(const hash_node_ptr node, const slot_type pos, const key_type update_key, const node_ptr update_node){
        const hash_type hash_key = get_hash_key(node->id, pos);
        table_[hash_key].update(node->id, pos, update_key, update_node);
    }

    inline bool compare_and_swap(const hash_node_ptr node, const slot_type pos, const node_ptr ori_node, const key_type update_key, const node_ptr update_node){
        update(node, pos, update_key, update_node);
        return true;
    }

    //inline bool compare_and_swap(const node_ptr id, const slot_type pos, const node_ptr ori_node, const slot_type copy_pos){
    //    const hash_type hash_key1 = get_hash_key(id, pos), hash_key2 = get_hash_key(id, copy_pos);
    //    key_type find_key;
    //    node_ptr find_node;
    //    std::tie(find_key, find_node) = table_[hash_key1].find(id, pos);
    //    if (find_node == ori_node){
    //        std::tie(find_key, find_node) = table_[hash_key2].find(id, copy_pos);
    //        table_[hash_key1].update(id, pos, find_key, find_node);
    //        return true;
    //    }
    //    return false;
    //}

    LL slot_size, real_slot_size;
    size_type size;
    HashTableBlock* table_;
};


};