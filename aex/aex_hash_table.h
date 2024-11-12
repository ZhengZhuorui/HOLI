#pragma once

namespace aex{
    
template<typename _Key,
        typename traits>
struct aex_hash_table_block{
    typedef aex_default_components<traits> components;
    typedef typename components::base_node base_node;
    typedef base_node* node_ptr;
    typedef typename traits::slot_type slot_type;
    typedef typename traits::key_type key_type;
    typedef aex_hash_table_block<_Key, traits> self;

    key_type key[traits::HASH_TABLE_BLOCK_SIZE];
    node_ptr child[traits::HASH_TABLE_BLOCK_SIZE], 
             parent[traits::HASH_TABLE_BLOCK_SIZE];
    int      pos[traits::HASH_TABLE_BLOCK_SIZE];
    //NodeType type[traits::HASH_TABLE_BLOCK_SIZE];
    char     size;
    self*    next;

    aex_hash_table_block():size(0), next(nullptr){}

    self& operator=(self &other){
        self* b = this;
        self* ob = &other;
        while (ob != nullptr){
            b->size = ob->size;
            std::copy(ob->key,    ob->key    + ob->size, key);
            std::copy(ob->child,  ob->child  + ob->size, child);
            std::copy(ob->parent, ob->parent + ob->size, parent);
            std::copy(ob->pos,    ob->pos    + ob->size, pos);
            //std::copy(ob->type,   ob->type   + ob->size, type);
            if (ob->next != nullptr)
                b->next = new aex_hash_table_block();
            b = b->next;
            ob = ob->next;
        }
    }

    std::pair<key_type, node_ptr> find(const node_ptr _node, const slot_type _pos){
        self* b = this;
        do{
            for (int i = 0; i < b->size; ++i)
                if (parent[i] == _node && pos[i] == _pos)
                    return std::make_pair(key[i], child[i]);
            b = b->next;
        }while(b != nullptr);
        return std::make_pair(std::numeric_limits<key_type>::lowest(), nullptr);
    }

    //std::pair<node_ptr, NodeType> find_insert(const node_ptr _node, const slot_type _pos, const key_type x){
    //    HashTableBlock* b = this;
    //    do{
    //        for (char i = 0; i < size; ++i)
    //            if (parent[i] == _node && pos[i] == _pos && key[i] <= x)
    //                return std::make_pair(child[i], type[i]);
    //        b = b->next;
    //    }while(b != nullptr);
    //    return std::make_pair(nullptr, NodeType::LeafNode);
    //}

    node_ptr find(const node_ptr _node, const slot_type _pos, const key_type x){
        self* b = this;
        do{
            for (int i = 0; i < b->size; ++i)
                if (parent[i] == _node && pos[i] == _pos && key[i] <= x)
                    return child[i];
            b = b->next;
        }while(b != nullptr);
        return nullptr;
    }

    void insert(const node_ptr _node, const slot_type _pos, const key_type x, const node_ptr y){
        self* insert_block = this;
        //NodeType _type = y->type;
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
            int _size = insert_block->size;
            insert_block->parent[_size] = _node;
            insert_block->pos[_size]    = _pos;
            insert_block->key[_size]    = x;
            insert_block->child[_size]  = y;
            ++insert_block->size;
        }
    }

    bool erase(const node_ptr _node, const slot_type _pos){
        self* erase_block = this;
        self* tail = (erase_block->next == nullptr) ? this : this->next;
        do{
            for (int i = 0; i < erase_block->size; ++i)
                if (erase_block->parent[i] == _node && 
                    erase_block->pos[i] == _pos){
                    std::swap(erase_block->key[i], tail->key[tail->size - 1]);
                    std::swap(erase_block->child[i], tail->child[tail->size - 1]);
                    std::swap(erase_block->parent[i], tail->parent[tail->size - 1]);
                    std::swap(erase_block->pos[i], tail->pos[tail->size - 1]);
                    --tail->size;
                    if (tail != this && tail->size == 0){
                        this->next = tail->next;
                        delete tail;
                    }
                    return true;
                }
        }while(erase_block->next == nullptr);
        return false;
    }

    bool update(const node_ptr _node, const slot_type _pos, const key_type update_key, const node_ptr update_node){
        self* b = this;
        do{
            for (int i = 0; i < b->size; ++i)
                if (parent[i] == _node && pos[i] == _pos){
                    key[i]   = update_key;
                    child[i] = update_node;
                    return true;
                }
            b = b->next;
        }while(b != nullptr);
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

    typedef aex_hash_table<_Key, traits> self;

    aex_hash_table():slot_size(traits::MIN_HASH_TABLE_SIZE), size(0){
        table_ = new HashTableBlock[slot_size];
    }

    explicit aex_hash_table(LL _slot_size):slot_size(_slot_size), size(0){
        AEX_ASSERT((slot_size & (-slot_size)) == slot_size);
        table_ = new HashTableBlock[slot_size];
    }

    aex_hash_table(self &other_table):slot_size(other_table.slot_size), size(other_table.size){
        if (slot_size < traits::MIN_ML_INNER_NODE_SIZE){
            table_ = nullptr;
            return;
        }
        table_ = new HashTableBlock[slot_size];
        for (slot_type i = 0; i < this->slot_size; ++i){
            table_[i] = other_table.table_[i];
            HashTableBlock *b = other_table.table_[i];
            HashTableBlock *nb = table_[i];
            while (b->next != nullptr){
                nb->next = new HashTableBlock();
                *(nb->next) = *(b->next);
                b = b->next;nb = nb->next;
            }
        }
    }

    aex_hash_table(self &&other_table):slot_size(other_table.slot_size), size(other_table.size){
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

    inline ULL memory_used(){
        if (table_ == nullptr) return 0;
        else{
            ULL ret = sizeof(HashTableBlock) * this->slot_size;
            for (slot_type i = 0; i < this->slot_size; ++i)
            if (table_[i].next != nullptr){
                HashTableBlock *b = table_[i].next;
                while (b != nullptr){
                    ret += sizeof(HashTableBlock);
                    b = b->next;
                }
            }
            return ret;
        }
    }

    inline void print_stats(){
        AEX_HINT("[HashTable] size=" << size << ", slot_size=" << slot_size);
    }

    self& operator = (self &other_table){
        AEX_ASSERT(this->slot_size == other_table.slot_size);
        AEX_ASSERT(table_ != nullptr);
        delete this->table_;
        table_ = new HashTableBlock[slot_size];
        for (slot_type i = 0; i < this->slot_size; ++i){
            table_[i] = other_table.table_[i];
            HashTableBlock *b = other_table.table_[i];
            HashTableBlock *nb = table_[i];
            while (b->next != nullptr){
                nb->next = new HashTableBlock();
                *(nb->next) = *(b->next);
                b = b->next;nb = nb->next;
            }
        }
        return *this;
    }

    self& operator = (self &&other_table){
        AEX_ASSERT(table_ != nullptr);
        delete this->table_;
        if (slot_size < traits::MIN_ML_INNER_NODE_SIZE){
            return *this;
        }
        this->slot_size = other_table.slot_size;
        this->table_ = other_table.table_;
        other_table.table_ = nullptr;
        return *this;
    }

    void destory(HashTableBlock* b){
        HashTableBlock *c = b->next, *t;
        while (c != nullptr){
            t = c;
            c = c->next;
            delete t;
        }
        delete b;
    }

    void destory(){
        AEX_ASSERT(table_ != nullptr);
        for (slot_type i = 0; i < this->slot_size; ++i){
            if (table_[i].next != nullptr)
                destory(table_[i].next);
        }
        delete table_;
    }

    inline unsigned long long get_hash_key(const node_ptr n, const slot_type pos) const {
        unsigned long long _ = (unsigned long long)(pos);
        return (reinterpret_cast<unsigned long long>(n) * traits::K1 + _ * traits::K2) & (slot_size - 1);
    }

    inline void clear(){
        destory();
        table_ = new HashTableBlock[traits::MIN_HASH_TABLE_SIZE];
    }

    inline bool isfull() const {
        return 1.0 * this->size / this->slot_size >= traits::HASH_TABLE_FULL_RATIO;
    }

    inline bool isfew() const {
        return 1.0 * this->size / this->slot_size < traits::HASH_TABLE_FEW_RATIO;
    }
    
    inline void rescale(const slot_type _slot_size){
        HashTableBlock* new_hash_table = new HashTableBlock[_slot_size];
        for (slot_type i = 0; i < this->slot_size; ++i){
            HashTableBlock *b = &table_[i];
            do{
                for (unsigned char j = 0; j < b->size; ++j){
                    hash_type hash_key = get_hash_key(b->parent[j], b->pos[j]);
                    new_hash_table[hash_key].insert(b->parent[j], b->pos[j], b->key[j], b->child[j]);
                }
                b = b->next;
            }while(b == nullptr);
        }
        destory();
        table_ = new_hash_table;
        this->slot_size = _slot_size;
    }

    inline void narrow(){
        rescale(this->slot_size >> 1);
    }

    inline void expand(){
        rescale(this->slot_size << 1);
    }

    /**
     * @brief insert (node->key[pos], node->child[pos]) in hash table
     */
    inline void insert(const node_ptr parent, const slot_type pos, const key_type key, const node_ptr child){
        hash_type hash_key = get_hash_key(parent, pos);
        if (table_[hash_key].size == traits::HASH_TABLE_BLOCK_SIZE && this->isfull())
            expand();
        hash_key = get_hash_key(parent, pos);
        table_[hash_key].insert(parent, pos, key, child);
        ++size;
    }

    /**
     * @brief return the node->child[pos] if node->key[pos] > key
     */
    inline node_ptr find(const node_ptr node, const slot_type pos, const key_type key) const {
        hash_type hash_key = get_hash_key(node, pos);
        return table_[hash_key].find(node, pos, key);
    }

    /**
     * @brief return the (node->key[pos], node->child[pos])
     */
    inline std::pair<key_type, node_ptr> find(const node_ptr node, const slot_type pos) const {
        hash_type hash_key = get_hash_key(node, pos);
        return table_[hash_key].find(node, pos);
    }

    /**
     * @brief erase node->child[pos]
     */
    inline bool erase(const node_ptr node, const slot_type pos){
        if (isfew())
            narrow();
        hash_type hash_key = get_hash_key(node, pos);
        bool ret = table_[hash_key].erase(node, pos);
        if (ret)
            --size;
        return ret;
    }

    inline bool update(const node_ptr parent, const slot_type pos, const key_type update_key, const node_ptr update_node){
        hash_type hash_key = get_hash_key(parent, pos);
        bool ret = table_[hash_key].update(parent, pos, update_key, update_node);
        AEX_ASSERT(ret == false);
        return ret;
    }

    LL slot_size;
    size_type size;
    HashTableBlock* table_;
};


}