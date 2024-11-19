#pragma once

namespace aex{

template<typename _Key,
        typename traits>
struct aex_hash_table_unit{
    typedef aex_default_components<traits> components;
    typedef typename components::base_node base_node;
    typedef base_node* node_ptr;
    typedef typename traits::slot_type slot_type;
    typedef typename traits::key_type key_type;
    typedef aex_hash_table_unit<_Key, traits> self;
    key_type key;
    slot_type pos;
    node_ptr parent, child;
    self* next;
    aex_hash_table_unit():next(nullptr){}
};

template<typename _Key,
        typename traits>
struct aex_hash_table_block{
    typedef aex_default_components<traits> components;
    typedef typename components::base_node base_node;
    typedef base_node* node_ptr;
    typedef typename traits::slot_type slot_type;
    typedef typename traits::key_type key_type;
    typedef aex_hash_table_block<_Key, traits> self;
    typedef aex_hash_table_unit<_Key, traits> Unit;
    Unit* entry;

    aex_hash_table_block():entry(nullptr){}

    self& operator=(self &other){
        if (entry == nullptr && other.entry != nullptr)
            entry = new Unit();
        for (Unit* b = entry, *ob = other.entry; ob != nullptr; b = b->next, ob = ob->next){
            b->key = ob->key;
            b->child = ob->child;
            b->parent = ob->parent;
            b->pos = ob->pos;
            if (ob->next != nullptr)
                b->next = new Unit();
        }
    }

    inline std::pair<key_type, node_ptr> find(const node_ptr _node, const slot_type _pos){
        for (Unit* b = entry; b != nullptr; b = b->next){
            if (b->parent == _node && b->pos == _pos)
                return std::make_pair(b->key, b->child);
        }
        return std::make_pair(std::numeric_limits<key_type>::lowest(), nullptr);
    }

    inline void insert(const node_ptr _node, const slot_type _pos, const key_type x, const node_ptr y){
        Unit* new_unit = new Unit();
        new_unit->next = entry;
        entry = new_unit;
        entry->parent = _node;
        entry->pos    = _pos;
        entry->key    = x;
        entry->child  = y;
    }

    inline bool erase(const node_ptr _node, const slot_type _pos){
        for(Unit* erase_block = entry; erase_block != nullptr; erase_block = erase_block->next){
            if (erase_block->parent == _node && erase_block->pos == _pos){
                std::swap(erase_block->parent, entry->parent);
                std::swap(erase_block->pos,    entry->pos);
                std::swap(erase_block->key,    entry->key);
                std::swap(erase_block->child,  entry->child);
                Unit* next = entry->next;
                delete entry;
                entry = next;
                return true;
            }
        }
        return false;
    }

    inline bool update(const node_ptr _node, const slot_type _pos, const key_type update_key, const node_ptr update_node){
        for (Unit *b = entry; b != nullptr; b = b->next){
            if (b->parent == _node && b->pos == _pos){
                b->key   = update_key;
                b->child = update_node;
                return true;
            }
        }
        return false;
    }
};

template<typename _Key,
        typename traits>
class aex_hash_table{
public:
    typedef _Key     key_type;
    typedef aex_default_components<traits>      components;
    typedef typename traits::slot_type          slot_type;
    typedef typename traits::hash_type          hash_type;
    typedef typename components::size_type          size_type;
    typedef typename components::base_node      base_node;
    typedef typename components::node_ptr       node_ptr;
    typedef typename components::HashTableBlock HashTableBlock;
    typedef typename HashTableBlock::Unit       Unit;

    typedef aex_hash_table<_Key, traits> self;

    aex_hash_table():slot_size(traits::MIN_HASH_TABLE_SIZE), size(0){
        table_ = new HashTableBlock[slot_size]();
        real_slot_size = get_real_slot_size(slot_size);
    }

    explicit aex_hash_table(LL _slot_size):slot_size(_slot_size), size(0){
        AEX_ASSERT((slot_size & (-slot_size)) == slot_size);
        table_ = new HashTableBlock[slot_size]();
        real_slot_size = get_real_slot_size(slot_size);
    }

    aex_hash_table(self &other_table):slot_size(other_table.slot_size), real_slot_size(other_table.real_slot_size), size(other_table.size){
        if (slot_size < traits::MIN_ML_INNER_NODE_SIZE){
            table_ = nullptr;
            return;
        }
        table_ = new HashTableBlock[slot_size]();
        for (slot_type i = 0; i < this->slot_size; ++i)
            table_[i] = other_table.table_[i];
        
    }

    aex_hash_table(self &&other_table):slot_size(other_table.slot_size), real_slot_size(other_table.real_slot_size), size(other_table.size){
        if (slot_size < traits::MIN_ML_INNER_NODE_SIZE){
            table_ = nullptr;
            return;
        }
        this->slot_size = other_table.slot_size;
        this->table_ = other_table.table_;
        other_table.table_ = nullptr;
    }

    ~aex_hash_table(){
        destory();
    }

    self& operator = (self &other_table){
        AEX_ASSERT(this->slot_size == other_table.slot_size);
        AEX_ASSERT(table_ != nullptr);
        delete[] this->table_;
        table_ = new HashTableBlock[slot_size]();
        for (slot_type i = 0; i < this->slot_size; ++i){
            table_[i] = other_table.table_[i];
        }
        return *this;
    }

    self& operator = (self &&other_table){
        AEX_ASSERT(table_ != nullptr);
        delete[] this->table_;
        if (slot_size < traits::MIN_ML_INNER_NODE_SIZE){
            return *this;
        }
        this->slot_size = other_table.slot_size;
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

    inline ULL memory_used(){
        if (table_ == nullptr) return 0;
        else{
            ULL ret = sizeof(HashTableBlock) * this->slot_size;
            for (slot_type i = 0; i < this->slot_size; ++i)
                for (Unit *b = table_[i].entry; b != nullptr; b = b->next)
                    ret += sizeof(Unit);
            return ret;
        }
    }

    inline void print_stats(){
        AEX_HINT("[HashTable Stats]: size=" << size << ", slot_size=" << slot_size);
        long long cnt = 0;
        for (slot_type i = 0; i < this->slot_size; ++i){
            cnt += (table_[i].entry != nullptr);
        }
        AEX_HINT("cnt=" << cnt << ", avg collision=" << 1.0 * size / cnt);
    }

    void destory(){
        AEX_ASSERT(table_ != nullptr);
        for (slot_type i = 0; i < this->slot_size; ++i){
            for (Unit *b = table_[i].entry, *t; b != nullptr; ){
                t = b;
                b = b->next;
                delete t;
            }
        }
        delete[] table_;
    }

    inline unsigned long long get_hash_key(const node_ptr n, const slot_type pos) const {
        return (reinterpret_cast<unsigned long long>(n) * traits::K1 + static_cast<unsigned long long>(pos) * traits::K2) % real_slot_size;
    }

    inline void clear(){
        destory();
        table_ = new HashTableBlock[traits::MIN_HASH_TABLE_SIZE]();
        slot_size = traits::MIN_HASH_TABLE_SIZE;
        real_slot_size = get_real_slot_size(slot_size);
    }

    inline bool isfull() const {
        return 1.0 * this->size / this->slot_size >= traits::HASH_TABLE_FULL_RATIO;
    }

    inline bool isfew() const {
        return this->slot_size > (slot_type)traits::MIN_HASH_TABLE_SIZE && 1.0 * this->size / this->slot_size < traits::HASH_TABLE_FEW_RATIO;
    }
    
    inline void rescale(const slot_type _slot_size){
        //AEX_WARNING("[hashtable rescale] slot_size=" << this->slot_size << ", _slot_size=" << _slot_size << ", size=" << this->size);
        AEX_ASSERT((_slot_size & (-_slot_size)) == _slot_size);
        AEX_ASSERT(_slot_size >= (slot_type)traits::MIN_HASH_TABLE_SIZE);
        slot_type new_real_slot_size = get_real_slot_size(_slot_size);
        HashTableBlock* new_hash_table = new HashTableBlock[_slot_size]();
        for (slot_type i = 0; i < this->slot_size; ++i){
            for (Unit* b = table_[i].entry; b != nullptr; b = b->next){
                hash_type new_hash_key = (reinterpret_cast<unsigned long long>(b->parent) * traits::K1 + static_cast<unsigned long long>(b->pos) * traits::K2) % new_real_slot_size;
                new_hash_table[new_hash_key].insert(b->parent, b->pos, b->key, b->child);
            }
        }
        destory();
        table_ = new_hash_table;
        this->slot_size = _slot_size;
        this->real_slot_size = new_real_slot_size;
    }

    inline void narrow(){ rescale(this->slot_size >> 1); }
    inline void expand(){ rescale(this->slot_size << 1); }

    /**
     * @brief insert <node, pos>: <node->key[pos], node->child[pos]> in hash table
     */
    inline void insert(const node_ptr parent, const slot_type pos, const key_type key, const node_ptr child){
        hash_type hash_key = get_hash_key(parent, pos);
        if (this->isfull()){
            expand();
            hash_key = get_hash_key(parent, pos);
        }
        //AEX_PRINT("hash_key=" << hash_key << ", pos=" << pos << ", key=" << key << ", child=" << child);
        table_[hash_key].insert(parent, pos, key, child);
        ++size;
    }

    /**
     * @brief return the (node->key[pos], node->child[pos])
     */
    inline std::pair<key_type, node_ptr> find(const node_ptr node, const slot_type pos) const {
        const hash_type hash_key = get_hash_key(node, pos);
        //AEX_PRINT("hash_key=" << hash_key << ", pos=" << pos );
        return table_[hash_key].find(node, pos);
    }

    /**
     * @brief erase node->array[pos]. Return true if node->array[pos] exists.
     */
    inline bool erase(const node_ptr node, const slot_type pos){
        if (isfew())
            narrow();
        const hash_type hash_key = get_hash_key(node, pos);
        bool ret = table_[hash_key].erase(node, pos);
        if (ret)
            --size;
        return ret;
    }

    inline bool update(const node_ptr parent, const slot_type pos, const key_type update_key, const node_ptr update_node){
        const hash_type hash_key = get_hash_key(parent, pos);
        bool ret = table_[hash_key].update(parent, pos, update_key, update_node);
        AEX_ASSERT(ret == true);
        return ret;
    }

    LL slot_size, real_slot_size;
    size_type size;
    HashTableBlock* table_;
};


}