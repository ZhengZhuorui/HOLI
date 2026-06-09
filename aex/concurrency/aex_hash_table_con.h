#pragma once
//#include "aex_traits.h"
//#include "aex_hash_table.h"
//#include "concurrency/aex_lock.h"
namespace aex{

template<typename _Key,
        typename _Val,
        typename traits>
struct aex_hash_table_con_bucket{
    //using traits = aex_default_traits<ULL, ULL, false, void, true>;
    using components = aex_default_components<traits>;
    using RWLock = typename components::RWLock;
    using self = aex_hash_table_con_bucket<_Key, _Val, traits>;
    self &operator =(const self &other){
        std::copy(other.keys, other.keys + 4, this->keys);
        std::copy(other.values, other.values + 4, this->values);
        return *this;
    }
    RWLock lock;
    _Key keys[traits::HASH_TABLE_BLOCK_SIZE];
    _Val values[traits::HASH_TABLE_BLOCK_SIZE];
};

template<typename _Key,
        typename _Val,
        typename traits>
class aex_hash_table_con : public aex_hash_table<_Key, _Val, traits>{
public:
    using key_type = _Key;
    using value_type = _Val;
    using components = aex_default_components<traits>;
    //using HashTableBucket = aex_hash_table_bucket<_Key, _Val>;
    using HashTableBucket = typename components::HashTableBucket<_Key, _Val, traits>;
    using self = aex_hash_table_con<key_type, value_type, traits>;
    using parent = aex_hash_table<key_type, value_type, traits>;
    using parent::find;
    using parent::insert;
    using parent::exists;
    using parent::erase;
    using parent::update;
    using parent::get_hash_key;
    using parent::slot_size;
    using parent::table_;
    using parent::EMPTY_MASK;
    using parent::DEL_MASK;
    //static constexpr uint64_t EMPTY_MASK = 0xFFFFFFFFFFFFFFFFULL;
    //static constexpr uint64_t DEL_MASK = 0xFFFFFFFFFFFFFFFEULL;

    //aex_hash_table_con():aex_hash_table(){}
    aex_hash_table_con(){}

    explicit aex_hash_table_con(LL _slot_size):parent(_slot_size){
        //AEX_IMPORTANT("create hash table1");
        for (size_t i = 0; i < slot_size; ++i)
            table_[i].lock.init();
    }

    aex_hash_table_con(self &other_table):parent(other_table){
        for (size_t i = 0; i < slot_size; ++i)
            table_[i].lock.init();
    }

    aex_hash_table_con(self &&other_table):parent(std::move(other_table)){
        for (size_t i = 0; i < slot_size; ++i)
            table_[i].lock.init();
    }

    ~aex_hash_table_con(){
        this->clear();
    }

    void set(size_t _slot_size){
        this->parent::set(_slot_size);
        for (size_t i = 0; i < slot_size; ++i)
            table_[i].lock.init();
    }

    self& operator = (self &other_table){
        *(parent*)(this) = (parent)other_table;
        return *this;
    }

    self& operator = (self &&other_table){
        *(parent*)(this) = (parent)std::move(other_table);
        return *this;
    }

    inline size_t memory_used() const{
        return slot_size * sizeof(HashTableBucket);
    }
    
    inline void insert(const key_type key, const value_type value){
        int restart_count = 0;
    insert_start:
        if (restart_count > 0)
            _yield(restart_count);
        restart_count++;
        bool need_restart = false;
        const uint64_t hash_key = get_hash_key(key);
        const __m256i vec_e =_mm256_set1_epi64x(EMPTY_MASK);
        const __m256i vec_d =_mm256_set1_epi64x(DEL_MASK);
        for (uint64_t i = hash_key; ; i = (i + 1) & (slot_size - 1)){
            const __m256i x = _mm256_loadu_si256((const __m256i*)table_[i].keys);
            const int mask = _mm256_cmpeq_epi64_mask(x, vec_e) | _mm256_cmpeq_epi64_mask(x, vec_d);
            if (mask != 0){
                int p = __builtin_ctz(mask);
                table_[i].lock.writeLockOrRestart(need_restart);
                if (need_restart || ((uint64_t)(table_[i].keys[p]) != EMPTY_MASK && (uint64_t)(table_[i].keys[p]) != DEL_MASK)) goto insert_start;
                table_[i].keys[p] = key;
                table_[i].values[p] = value;
                table_[i].lock.writeUnlock();
                return;
            }
        }
    }

    /**
     * @brief return the (node->key[pos], node->child[pos])
     */
    inline value_type find(const key_type key, bool &need_restart) const {
        const uint64_t hash_key = get_hash_key(key);
        const __m256i vec_k =_mm256_set1_epi64x(key);
        const __m256i vec_e =_mm256_set1_epi64x(EMPTY_MASK);
        for (uint64_t i = hash_key; ; i = (i + 1) & (slot_size - 1)){
            uint64_t bucket_version = table_[i].lock.readLockOrRestart(need_restart);
            if (need_restart) return value_type();
            const __m256i x = _mm256_loadu_si256((const __m256i*)table_[i].keys);
            const int mask = _mm256_cmpeq_epi64_mask(x, vec_k);
            if (mask != 0){
                table_[i].lock.readUnlockOrRestart(bucket_version, need_restart);
                return table_[i].values[__builtin_ctz(mask)];
            }
            const int empty_mask = _mm256_cmpeq_epi64_mask(x, vec_e);
            if (empty_mask != 0){
                table_[i].lock.readUnlockOrRestart(bucket_version, need_restart);
                return std::make_pair(0, nullptr);
            }
        }
    }

    inline bool exists(const key_type key) const {
        const uint64_t hash_key = get_hash_key(key);
        const __m256i vec_k =_mm256_set1_epi64x(key);
        const __m256i vec_e =_mm256_set1_epi64x(EMPTY_MASK);
        for (uint64_t i = hash_key; ; i = (i + 1) & (slot_size - 1)){
            const __m256i x = _mm256_loadu_si256((const __m256i*)table_[i].keys);
            const int mask = _mm256_cmpeq_epi64_mask(x, vec_k);
            if (mask != 0){
                return true;
            }
            const int empty_mask = _mm256_cmpeq_epi64_mask(x, vec_e);
            if (empty_mask != 0){
                return false;
            }
        }
    }

    inline void erase(const key_type key){
        int restart_count = 0;
    erase_start:
        if (restart_count > 0)
            _yield(restart_count);
        restart_count++;
        bool need_restart = false;

        const uint64_t hash_key = get_hash_key(key);
        const __m256i vec_k =_mm256_set1_epi64x(key);
        const __m256i vec_e =_mm256_set1_epi64x(EMPTY_MASK);
        for (uint64_t i = hash_key; ; i = (i + 1) & (slot_size - 1)){
            table_[i].lock.writeLockOrRestart(need_restart);
            if (need_restart) goto erase_start;
            const __m256i x = _mm256_loadu_si256((const __m256i*)table_[i].keys);
            const int mask = _mm256_cmpeq_epi64_mask(x, vec_k);
            if (mask != 0){
                table_[i].keys[__builtin_ctz(mask)] = DEL_MASK;
                table_[i].lock.writeUnlock();
                return;
            }
            const int empty_mask = _mm256_cmpeq_epi64_mask(x, vec_e);
            if (empty_mask != 0){
                AEX_ASSERT(0 == 1);
                table_[i].lock.writeUnlock();
                return;
            }
            table_[i].lock.writeUnlock();
        }
    }

    inline void update(const uint64_t key, const value_type value){
        int restart_count = 0;
    update_start:
        if (restart_count > 0)
            _yield(restart_count);
        restart_count++;
        bool need_restart = false;

        const uint64_t hash_key = this->get_hash_key(key);
        const __m256i vec_k =_mm256_set1_epi64x(key);
        const __m256i vec_e =_mm256_set1_epi64x(this->EMPTY_MASK);
        for (uint64_t i = hash_key; ; i = (i + 1) & (this->slot_size - 1)){
            table_[i].lock.writeLockOrRestart(need_restart);
            if (need_restart) goto update_start;
            const __m256i x = _mm256_loadu_si256((const __m256i*)table_[i].keys);
            const int mask = _mm256_cmpeq_epi64_mask(x, vec_k);
            if (mask != 0){
                table_[i].value[__builtin_ctz(mask)] = value;
                return;
            }
            const int empty_mask = _mm256_cmpeq_epi64_mask(x, vec_e);
            if (empty_mask != 0){
                AEX_ASSERT(0 == 1);
                return;
            }
            table_[i].lock.writeUnlock();
        }
    }

};


}