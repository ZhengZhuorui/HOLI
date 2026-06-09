#pragma once
#include <sys/mman.h>
namespace aex{

template<typename _Key,
        typename _Val,
        typename traits>
struct aex_hash_table_bucket{
    _Key keys[traits::HASH_TABLE_BLOCK_SIZE];
    _Val values[traits::HASH_TABLE_BLOCK_SIZE];
};

template<typename _Key,
        typename _Val,
        typename traits>
class aex_hash_table{
public:
    using key_type = _Key;
    using value_type = _Val;
    using components = aex_default_components<traits>;
    //using HashTableBucket = aex_hash_table_bucket<_Key, _Val>;
    using HashTableBucket = typename components::HashTableBucket<_Key, _Val, traits>;
    
    using self = aex_hash_table<_Key, _Val, traits>;
    static constexpr uint64_t EMPTY_MASK = 0xFFFFFFFFFFFFFFFFULL;
    static constexpr uint64_t DEL_MASK = 0xFFFFFFFFFFFFFFFEULL;

    aex_hash_table():table_(nullptr), slot_size(0){}

    void alloc(size_t slot_size){
        size_t alloc_size = sizeof(HashTableBucket) * slot_size;
        table_ = (HashTableBucket*)mmap(NULL, alloc_size, PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (sizeof(HashTableBucket) * slot_size >= 65536)
            madvise(this->table_, alloc_size, MADV_HUGEPAGE);
    }

    explicit aex_hash_table(LL _slot_size):slot_size(_slot_size){
        AEX_ASSERT((this->slot_size & (-this->slot_size)) == this->slot_size);
        AEX_PRINT("slot_size=" << slot_size);
        alloc(slot_size);
        memset(this->table_, 0xff, this->slot_size * sizeof(HashTableBucket));
    }

    aex_hash_table(self &other_table):slot_size(other_table.slot_size){
        //table_ = new HashTableBucket[slot_size]();
        alloc(slot_size);
        memcpy(table_, other_table.table_, sizeof(HashTableBucket) * slot_size);
    }

    aex_hash_table(self &&other_table){
        this->table_ = nullptr;
        this->slot_size = other_table.slot_size;
        this->table_ = other_table.table_;
    }

    void set(size_t _slot_size){
        this->slot_size = _slot_size;
        alloc(slot_size);
        memset(this->table_, 0xff, this->slot_size * sizeof(HashTableBucket));
    }

    ~aex_hash_table(){
        this->clear();
    }

    self& operator = (self &other_table){
        if (this->table_ != nullptr)
            clear();
        this->slot_size = other_table.slot_size;
        alloc(slot_size);
        for (size_t i = 0; i < this->slot_size; ++i)
            table_[i] = other_table.table_[i];
        return *this;
    }

    self& operator = (self &&other_table){
        this->slot_size = other_table.slot_size;
        this->table_ = other_table.table_;
        return *this;
    }

    void clear(){
        //delete[] this->table_;
        auto res = munmap(this->table_, sizeof(HashTableBucket) * this->slot_size);
        if (res != 0){
            AEX_PRINT("munmap failed! res=" << res);
        }
        AEX_ASSERT(res == 0);
        this->table_ = nullptr;
    }

    inline size_t memory_used() const{
        return this->slot_size * sizeof(HashTableBucket);
    }

    inline void print_stats() const {
        AEX_HINT("Bucket size=" << sizeof(HashTableBucket));
        AEX_HINT("[HashTable Stats]: slot_size=" << slot_size);
    }

    inline uint64_t get_hash_key(const key_type key) const {
        return (uint64_t)_mm_crc32_u64(0,(uint64_t)key) & (this->slot_size - 1);
    }
    
    inline void insert(const key_type key, const value_type value){
        const uint64_t hash_key = get_hash_key(key);
        //const __m256i vec_k =_mm256_set1_epi64x(key);
        const __m256i vec_e =_mm256_set1_epi64x(EMPTY_MASK);
        const __m256i vec_d =_mm256_set1_epi64x(DEL_MASK);
        for (uint64_t i = hash_key; ; i = (i + 1) & (this->slot_size - 1)){
            const __m256i x = _mm256_loadu_si256((const __m256i*)table_[i].keys);
            const int mask = _mm256_cmpeq_epi64_mask(x, vec_e) | _mm256_cmpeq_epi64_mask(x, vec_d);
            if (mask != 0){
                int p = __builtin_ctz(mask);
                table_[i].keys[p] = key;
                table_[i].values[p] = value;
                return;
            }
        }
    }    

    /**
     * @brief return the (node->key[pos], node->child[pos])
     */
    inline value_type find(const key_type key) const {
        const uint64_t hash_key = get_hash_key(key);
        const __m256i vec_k =_mm256_set1_epi64x(key);
        //const __m256i vec_e =_mm256_set1_epi64x(EMPTY_MASK);
        for (uint64_t i = hash_key; ; i = (i + 1) & (this->slot_size - 1)){
            const __m256i x = _mm256_loadu_si256((const __m256i*)table_[i].keys);
            const __mmask8 mask = _mm256_cmpeq_epi64_mask(x, vec_k);
            if (__builtin_expect(mask != 0, 1)){
                return table_[i].values[__builtin_ctz(mask)];
            }
            if (reinterpret_cast<uint64_t*>(table_[i].keys)[traits::HASH_TABLE_BLOCK_SIZE - 1] == EMPTY_MASK){
                AEX_ASSERT(0 == 1);
                return value_type();
            }
        }
    }

    inline bool exists(const key_type key) const {
        const uint64_t hash_key = get_hash_key(key);
        const __m256i vec_k =_mm256_set1_epi64x(key);
        const __m256i vec_e =_mm256_set1_epi64x(EMPTY_MASK);
        for (uint64_t i = hash_key; ; i = (i + 1) & (this->slot_size - 1)){
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
        const uint64_t hash_key = get_hash_key(key);
        const __m256i vec_k =_mm256_set1_epi64x(key);
        const __m256i vec_e =_mm256_set1_epi64x(EMPTY_MASK);
        for (uint64_t i = hash_key; ; i = (i + 1) & (this->slot_size - 1)){
            const __m256i x = _mm256_loadu_si256((const __m256i*)table_[i].keys);
            const int mask = _mm256_cmpeq_epi64_mask(x, vec_k);
            if (mask != 0){
                table_[i].keys[__builtin_ctz(mask)] = DEL_MASK;
                return;
            }
            const int empty_mask = _mm256_cmpeq_epi64_mask(x, vec_e);
            if (empty_mask != 0){
                AEX_ASSERT(0 == 1);
                return;
            }
        }
    }

    inline void update(const key_type key, const value_type value){
        const uint64_t hash_key = get_hash_key(key);
        const __m256i vec_k =_mm256_set1_epi64x(key);
        const __m256i vec_e =_mm256_set1_epi64x(EMPTY_MASK);
        for (uint64_t i = hash_key; ; i = (i + 1) & (this->slot_size - 1)){
            const __m256i x = _mm256_loadu_si256((const __m256i*)table_[i].keys);
            const int mask = _mm256_cmpeq_epi64_mask(x, vec_k);
            if (mask != 0){
                table_[i].values[__builtin_ctz(mask)] = value;
                return;
            }
            const int empty_mask = _mm256_cmpeq_epi64_mask(x, vec_e);
            if (empty_mask != 0){
                AEX_ASSERT(0 == 1);
                return;
            }
        }
    }

    HashTableBucket* table_;
    size_t slot_size;
};


};