#pragma once

namespace aex{
    
template<typename _Key,
        typename traits>
class aex_hash_table{
public:
    typedef aex_default_components<traits> components;
    typedef typename components::base_node base_node;
    typedef base_node* node_ptr;
    typedef typename traits::bitmap_base bitmap_base;
    typedef typename traits::bitmap bitmap;
    typedef typename traits::slot_type slot_type;
    typedef typename traits::key_type key_type;

    typedef aex_hash_table<_Key, traits> self;

    
    static unsigned long long SIZE_ARRAY_MEMORY_USED(int log_size){
        return sizeof(unsigned char) * ((1 << log_size) + 8);
        //return sizeof(unsigned char) * ((1 << log_size));
    }

    static unsigned long long POS_ARRAY_USED(int log_size){
        //return sizeof(slot_type) * (traits::ERROR_BOUND * ((1 << log_size) + 2));
        return sizeof(slot_type) * (traits::ERROR_BOUND * ((1 << log_size)));
    }

    static unsigned long long KEY_ARRAY_USED(int log_size){
        //return sizeof(key_type) * (traits::ERROR_BOUND * ((1 << log_size) + 2));
        return sizeof(key_type) * (traits::ERROR_BOUND * ((1 << log_size)));
    }

    static unsigned long long PTR_ARRAY_USED(int log_size){
        //return sizeof(node_ptr) * (traits::ERROR_BOUND * ((1 << log_size) + 2));
        return sizeof(node_ptr) * (traits::ERROR_BOUND * ((1 << log_size)));
    }

    static unsigned long long MEMORY_USED(int log_size){
        return SIZE_ARRAY_MEMORY_USED(log_size) + POS_ARRAY_USED(log_size) + KEY_ARRAY_USED(log_size) + PTR_ARRAY_USED(log_size);
    }

    //aex_hash_table():size_ptr(nullptr), ori_pos(nullptr), key_ptr(nullptr), child_ptr(nullptr), slot_size(0), log_size(0){
    //    AEX_PRINT("???");
    //}

    explicit aex_hash_table(int _slot_size){
        this->slot_size = _slot_size;
        this->log_size = std::max(0, __builtin_ctz(slot_size) - traits::LOG_HASH_TABLE_RATIO);
        if (slot_size < traits::MIN_ML_INNER_NODE_SIZE){
            size_ptr = nullptr;
            return;
        }
        AEX_ASSERT((slot_size & (-slot_size)) == slot_size);
        unsigned char* ptr = static_cast<unsigned char*>(malloc(MEMORY_USED(log_size)));
        memset(ptr, 0, MEMORY_USED(log_size));
        size_ptr = ptr;
        ori_pos = reinterpret_cast<slot_type*>(ptr + SIZE_ARRAY_MEMORY_USED(log_size));
        key_ptr = reinterpret_cast<key_type*>(ptr + SIZE_ARRAY_MEMORY_USED(log_size) + POS_ARRAY_USED(log_size));
        child_ptr = reinterpret_cast<node_ptr*>(ptr + SIZE_ARRAY_MEMORY_USED(log_size) + POS_ARRAY_USED(log_size) + KEY_ARRAY_USED(log_size));
    }

    ~aex_hash_table(){
        if (size_ptr != nullptr)
            free(size_ptr);
    }

    aex_hash_table(self &other_table):log_size(other_table.log_size), slot_size(other_table.slot_size){
        if (slot_size < traits::MIN_ML_INNER_NODE_SIZE){
            size_ptr = nullptr;
            return;
        }
        unsigned char* ptr = static_cast<unsigned char*>(malloc(MEMORY_USED(log_size)));
        size_ptr = ptr;
        ori_pos = reinterpret_cast<slot_type*>(ptr + SIZE_ARRAY_MEMORY_USED(log_size));
        key_ptr = reinterpret_cast<key_type*>(ptr + SIZE_ARRAY_MEMORY_USED(log_size) + POS_ARRAY_USED(log_size));
        child_ptr = reinterpret_cast<node_ptr*>(ptr + SIZE_ARRAY_MEMORY_USED(log_size) + POS_ARRAY_USED(log_size) + KEY_ARRAY_USED(log_size));
        memcpy(size_ptr, other_table.size_ptr, MEMORY_USED(log_size));
    }

    aex_hash_table(self &&other_table):log_size(other_table.log_size), slot_size(other_table.slot_size){
        if (slot_size < traits::MIN_ML_INNER_NODE_SIZE){
            size_ptr = nullptr;
            return;
        }
        unsigned char* ptr = reinterpret_cast<unsigned char*>(other_table.size_ptr);
        size_ptr = ptr;
        ori_pos = reinterpret_cast<slot_type*>(ptr + SIZE_ARRAY_MEMORY_USED(log_size));
        key_ptr = reinterpret_cast<key_type*>(ptr + SIZE_ARRAY_MEMORY_USED(log_size) + POS_ARRAY_USED(log_size));
        child_ptr = reinterpret_cast<node_ptr*>(ptr + SIZE_ARRAY_MEMORY_USED(log_size) + POS_ARRAY_USED(log_size) + KEY_ARRAY_USED(log_size));
        other_table.size_ptr = nullptr;
    }

    inline size_t memory(){
        if (size_ptr == nullptr) return 0;
        else return MEMORY_USED(this->log_size);
    }

    void set_slot_size(slot_type _slot_size){
        AEX_ASSERT((_slot_size & (-_slot_size)) == _slot_size);

        if (size_ptr != nullptr)
            free(size_ptr);
        this->slot_size = _slot_size;
        log_size = std::max(0, __builtin_ctz(slot_size) - traits::LOG_HASH_TABLE_RATIO);
        if (slot_size < traits::MIN_ML_INNER_NODE_SIZE){
            size_ptr = nullptr;
            return;
        }
        unsigned char* ptr = reinterpret_cast<unsigned char*>(malloc(MEMORY_USED(log_size)));
        memset(ptr, 0, MEMORY_USED(log_size));
        size_ptr = ptr;
        ori_pos = reinterpret_cast<slot_type*>(ptr + SIZE_ARRAY_MEMORY_USED(log_size));
        key_ptr = reinterpret_cast<key_type*>(ptr + SIZE_ARRAY_MEMORY_USED(log_size) + POS_ARRAY_USED(log_size));
        child_ptr = reinterpret_cast<node_ptr*>(ptr + SIZE_ARRAY_MEMORY_USED(log_size) + POS_ARRAY_USED(log_size) + KEY_ARRAY_USED(log_size));
    }

    self& operator = (self &other_table){
        AEX_ASSERT(this->log_size == other_table.log_size);
        AEX_ASSERT(this->slot_size == other_table.slot_size);
        if (slot_size < traits::MIN_ML_INNER_NODE_SIZE){
            return *this;
        }
        memcpy(this->size_ptr, other_table.size_ptr, MEMORY_USED(log_size));
        return *this;
    }

    self& operator = (self &&other_table){
        //AEX_PRINT("size_ptr=" << reinterpret_cast<unsigned long long>(this->size_ptr) << ", this->slot_size=" << this->slot_size << ", other_table->slot_size=" << other_table.slot_size << ", size_ptr=" << reinterpret_cast<unsigned long long>(other_table.size_ptr));
        //AEX_PRINT("log_size=" << this->log_size << ", other_table.log_size=" << other_table.log_size);
        if (this->size_ptr != nullptr)
            free(this->size_ptr);
        if (slot_size < traits::MIN_ML_INNER_NODE_SIZE){
            return *this;
        }
        this->slot_size = other_table.slot_size;
        this->log_size = other_table.log_size;
        unsigned char* ptr = reinterpret_cast<unsigned char*>(other_table.size_ptr);
        size_ptr = ptr;
        ori_pos = reinterpret_cast<slot_type*>(ptr + SIZE_ARRAY_MEMORY_USED(log_size));
        key_ptr = reinterpret_cast<key_type*>(ptr + SIZE_ARRAY_MEMORY_USED(log_size) + POS_ARRAY_USED(log_size));
        child_ptr = reinterpret_cast<node_ptr*>(ptr + SIZE_ARRAY_MEMORY_USED(log_size) + POS_ARRAY_USED(log_size) + KEY_ARRAY_USED(log_size));
        other_table.size_ptr = nullptr;
        return *this;
    }

    inline int hash_array_size(){
        //return (1 << log_size) + 2;
        return (1 << log_size);
    }

    inline void clear(){
        memset(size_ptr, 0, MEMORY_USED(log_size));
    }
    
    inline int fingerprint(const slot_type pos) const{
        //if (pos == 0)
        //    return 1 << log_size;
        //else if (pos == slot_size - 1)
        //    return (1 << log_size) + 1;
        //else 
            return pos & ((1 << log_size) - 1);
    }

    inline bool insert(const slot_type old_pos, const key_type x, const node_ptr y){
        int hash_key = fingerprint(old_pos);
        if (size_ptr[hash_key] >= traits::ERROR_BOUND)
            return false;
        int ptr_offset = hash_key << traits::LOG_ERROR_BOUND;
        slot_type *pos = ori_pos + ptr_offset;
        key_type *key = key_ptr + ptr_offset;
        node_ptr *child = child_ptr + ptr_offset;
        slot_type inserted_pos = aex::linear_search_lower_bound(key, key + size_ptr[hash_key], x) - key;
        std::move_backward(pos + inserted_pos, pos + size_ptr[hash_key], pos + size_ptr[hash_key] + 1);
        std::move_backward(key + inserted_pos, key + size_ptr[hash_key], key + size_ptr[hash_key] + 1);
        std::move_backward(child + inserted_pos, child + size_ptr[hash_key], child + size_ptr[hash_key] + 1);
        pos[inserted_pos] = old_pos;
        key[inserted_pos] = x;
        child[inserted_pos] = y;
        ++size_ptr[hash_key];
        return true;
        
    }

    inline node_ptr find(const slot_type old_pos, const key_type x) const{
        int hash_key = fingerprint(old_pos);
        int ptr_offset = hash_key << traits::LOG_ERROR_BOUND;
        slot_type *pos = ori_pos + ptr_offset;
        key_type *key = key_ptr + ptr_offset;
        node_ptr *child = child_ptr + ptr_offset;
        //AEX_PRINT("hash_key=" << hash_key << "size_ptr=" << int(size_ptr[hash_key]));
        for (int i = 0; i < size_ptr[hash_key] && old_pos >= pos[i]; ++i){
            //AEX_PRINT("x=" << x << ", key=" << key[i] << ", old_pos=" << old_pos << ", pos=" << pos[i]);
            if (x <= key[i]){
                return child[i];
            }
        }
        return nullptr;
    }

    inline std::pair<key_type, bool> find(const slot_type old_pos, const node_ptr y) const{
        int hash_key = fingerprint(old_pos);
        int ptr_offset = hash_key << traits::LOG_ERROR_BOUND;
        slot_type *pos = ori_pos + ptr_offset;
        key_type *key = key_ptr + ptr_offset;
        node_ptr *child = child_ptr + ptr_offset;
        for (int i = 0; i < size_ptr[hash_key] && old_pos >= pos[i]; ++i){
            if (y == child[i]){
                return std::make_pair(key[i], true);
            }
        }
        return std::make_pair(0, false);
    }

    inline std::pair<key_type, node_ptr> pop(const slot_type old_pos){
        int hash_key = fingerprint(old_pos);
        int ptr_offset = hash_key << traits::LOG_ERROR_BOUND;
        slot_type *pos = ori_pos + ptr_offset;
        key_type *key = key_ptr + ptr_offset;
        node_ptr *child = child_ptr + ptr_offset;
        for (int i = 0; i < size_ptr[hash_key] && old_pos >= pos[i]; ++i){
            if (old_pos == pos[i]){
                key_type res_key = key[i];
                node_ptr res_node = child[i];
                pos[size_ptr[hash_key] - 1] = -1;
                std::move(pos + i + 1, pos + size_ptr[hash_key], pos + i);
                std::move(key + i + 1, key + size_ptr[hash_key], key + i);
                std::move(child + i + 1, child + size_ptr[hash_key], child + i);
                --size_ptr[hash_key];
                return std::make_pair(res_key, res_node);
            }
        }
        return std::make_pair(0, nullptr);
    }

    inline std::pair<key_type, node_ptr> top(const slot_type old_pos){
        int hash_key = fingerprint(old_pos);
        int ptr_offset = hash_key << traits::LOG_ERROR_BOUND;
        slot_type *pos = ori_pos + ptr_offset;
        key_type *key = key_ptr + ptr_offset;
        node_ptr *child = child_ptr + ptr_offset;
        std::pair<key_type, node_ptr> res = std::make_pair(0, nullptr);
        for (int i = 0; i < size_ptr[hash_key]; ++i){
            if (old_pos == pos[i]){
                res = std::make_pair(key[i], child[i]);
            }
        }
        return res;
    }

    inline bool erase(const slot_type old_pos, const node_ptr y){
        int hash_key = fingerprint(old_pos);
        int ptr_offset = hash_key << traits::LOG_ERROR_BOUND;
        slot_type *pos = ori_pos + ptr_offset;
        key_type *key = key_ptr + ptr_offset;
        node_ptr *child = child_ptr + ptr_offset;

        for (int i = 0; i < size_ptr[hash_key] && old_pos >= pos[i]; ++i){
            if (y == child[i]){
                std::move(pos + i + 1, pos + size_ptr[hash_key], pos + i);
                std::move(key + i + 1, key + size_ptr[hash_key], key + i);
                std::move(child + i + 1, child + size_ptr[hash_key], child + i);
                pos[size_ptr[hash_key] - 1] = -1;
                --size_ptr[hash_key];
                return true;
            }
        }
        return false;
    }
    int log_size, slot_size;
    unsigned char* size_ptr;
    slot_type* ori_pos;
    key_type* key_ptr;
    node_ptr* child_ptr;
};


}