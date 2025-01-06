#pragma once
#include <type_traits>
#include <atomic>

#include "aex/aex_utils.h"

namespace aex{

template<typename traits, bool _ = traits::AllowConcurrency>
struct aex_spinlock{
    aex_spinlock(){}
    ~aex_spinlock(){}
    void init(){}
    inline void lock(){}
    inline void unlock(){}
    inline bool is_lock(){return true;}
    //inline bool is_lock(){return false;}
};

template<typename traits>
struct aex_spinlock<traits, true>{
    typedef aex_spinlock<traits, true> self;
    aex_spinlock() : writeLock(false) {}
    void init(){writeLock=false;}
    ~aex_spinlock(){}
    inline void lock() {
        bool expected = false;
        while (!writeLock.compare_exchange_weak(expected, true)) {
            expected = false;
        }
    }
    inline void unlock() {
        writeLock.store(false);
    }
    inline bool is_lock(){return writeLock.load();}
    //inline bool is_lock(){return false;}

    std::atomic<bool> writeLock;
};

template<typename traits, bool _ = traits::AllowConcurrency>
struct aex_rw_spinlock{
    aex_rw_spinlock(){}
    void init(){}
    inline void lock() const {}
    inline void unlock() const {}
    inline void lock_shared() const {}
    inline void unlock_shared() const {}
    inline void upgrade_lock() const {}
    inline void downgrade_lock() const {}
    inline bool try_lock() const {return true;}
    inline bool try_lock_shared() const {return true;}
    inline bool try_upgrade_lock() const {return true;}
    inline bool try_upgrade_lock_without_read() const {return true;}
    inline bool is_lock() const {return false;}
    inline bool is_lock_shared() const {return false;}
};

template<typename traits>
struct aex_rw_spinlock<traits, true>{
    typedef aex_rw_spinlock<traits, true> self;
    aex_rw_spinlock() : lockCount(0) {}
    void init(){lockCount = 0;}
    aex_rw_spinlock(const self &x) = delete;
    aex_rw_spinlock(const self &&x) = delete;
    ~aex_rw_spinlock(){}
    self& operator = (const self &x) = delete;
    self& operator = (const self &&x) = delete;
    void lock_shared() {
        unsigned short expected = lockCount.load() & (~1);
        unsigned short result   = expected + 0b10;
        while (!lockCount.compare_exchange_weak(expected, result)) {
            expected = lockCount.load() & (~1);
            result   = expected + 0b10;
        }
    }

    void unlock_shared() {
        AEX_ASSERT(is_lock_shared());
        lockCount.fetch_sub(0b10);
    }
    
    bool try_lock_shared(){
        unsigned short expected = lockCount.load() & (~1);
        unsigned short result   = expected + 0b10;
        if (!lockCount.compare_exchange_strong(expected, result)) 
            return false;
        return true;
    }

    void lock() {
        unsigned short expected = lockCount.load() & (~1);
        unsigned short result   = expected | 1;
        while (!lockCount.compare_exchange_weak(expected, result)) {
            expected = lockCount.load() & (~1);
            result   = expected | 1;
        }
        while (lockCount.load() >= 0b10);
    }

    void unlock() {
        AEX_ASSERT(is_lock());
        lockCount.fetch_sub(1);
    }

    bool try_lock(){
        unsigned short expected = lockCount.load() & (~1);
        unsigned short result   = expected | 1;
        if (!lockCount.compare_exchange_strong(expected, result)) 
            return false;
        while (lockCount.load() >= 0b10);
        return true;
    }

    bool try_upgrade_lock_without_read(){
        //unsigned int expected = lockCount.load() & (~1);
        //unsigned int result   = expected - 1;
        unsigned short expected = 0b10;
        unsigned short result   = 0b1;
        if (!lockCount.compare_exchange_strong(expected, result)) 
            return false;
        return true;
    }
    
    bool try_upgrade_lock(){
        unsigned short expected = lockCount.load() & (~1);
        unsigned short result   = expected - 1;
        if (!lockCount.compare_exchange_strong(expected, result)) 
            return false;
        while (lockCount.load() >= 0b10);
        return true;
    }

    // unused. may deadlock
    [[deprecated]] void upgrade_lock(){
        unsigned short expected = 0b10;
        unsigned short result   = 0b1;
        while (!lockCount.compare_exchange_weak(expected, result)) {
            expected = lockCount.load() & (~1);
            result   = expected - 1;
        }
    }

    void downgrade_lock(){
        AEX_ASSERT(is_lock());
        lockCount.fetch_add(1);
    }

    inline bool is_lock() const {return (lockCount.load() & 1) == 1;}
    inline bool is_lock_shared() const {return lockCount.load() >= 0b10;}
    std::atomic<unsigned short> lockCount;
};

template<typename T>
struct empty_type{
    typedef empty_type<T> self;
    empty_type() = default;
    empty_type(const T &t){}
    self& operator = (const self &y){return *this;}
    self& operator = (const T &y){return *this;}
    bool operator == (const T &y){return true;}
    bool operator == (const self &y){return true;}
    bool operator != (const T &y){return true;}
    inline T load() const {return 0;}
    inline void store(T t){}
    inline self& operator++(){return *this;}
    inline self& operator--(){return *this;}
};

template<typename T>
struct no_atomic_type{
    typedef no_atomic_type<T> self;
    no_atomic_type() = default;
    no_atomic_type(const T &t){x = t;}
    self& operator = (const self &y){x = y.x; return *this;}
    self& operator = (const T &y){x = y; return *this;}
    bool operator == (const T &y){return x == y;}
    bool operator == (const self &y){return x == y.x;}
    inline T load() const {return x;}
    inline void store(T t){x = t;}
    inline self& operator++(){x++;return *this;}
    inline self& operator--(){x--;return *this;}
    T x;
};


template<typename traits, bool _ = traits::AllowConcurrency>
struct aex_concurrency_components{
    typedef typename traits::key_type   key_type;
    typedef typename traits::value_type value_type;
    typedef no_atomic_type<LL>          atomic_size_type;
    typedef empty_type<unsigned int>    ref_count_type;

    typedef aex_node_base<key_type, value_type, traits>        base_node;
    typedef aex_node_base<key_type, value_type, traits>        inner_node;
    typedef aex_hash_node<key_type, value_type, traits>        hash_node;
    typedef aex_dense_node<key_type, value_type, traits>       dense_node;
    typedef aex_static_data_node<key_type, value_type, traits> data_node;
    //typedef aex_hash_data_node<key_type, value_type, traits>   data_node;
    typedef base_node* node_ptr;
    typedef inner_node* inner_node_ptr;
    typedef hash_node* hash_node_ptr;
    typedef dense_node* dense_node_ptr;
    typedef data_node* data_node_ptr;    
    
    typedef aex_rw_spinlock<traits> RWLock;
    typedef aex_spinlock<traits>    Lock;
    typedef aex_allocator<key_type, value_type, traits> Allocator;
    typedef aex_hash_table<key_type, traits>            HashTable;
    typedef ULL                                         version_type;
    typedef std::atomic_uint64_t                        atomic_version_type;
    //typedef empty_type<unsigned long long> version_type;
};

template<typename traits>
struct aex_concurrency_components<traits, true>{
    typedef typename traits::key_type key_type;
    typedef typename traits::value_type value_type;
    typedef std::atomic_int64_t atomic_size_type;
    typedef std::atomic_uint8_t ref_count_type;

    typedef aex_node_base<key_type, value_type, traits>            base_node;
    typedef aex_node_base<key_type, value_type, traits>            inner_node;
    typedef aex_hash_node_con<key_type, value_type, traits>        hash_node;
    typedef aex_dense_node<key_type, value_type, traits>           dense_node;
    typedef aex_static_data_node<key_type, value_type, traits>     data_node;
    //typedef aex_hash_data_node<key_type, value_type, traits>       data_node;

    typedef base_node*  node_ptr;
    typedef inner_node* inner_node_ptr;
    typedef hash_node*  hash_node_ptr;
    typedef dense_node* dense_node_ptr;
    typedef data_node*  data_node_ptr;    
    //typedef std::atomic<base_node*> atomicPtr;

    typedef aex_rw_spinlock<traits> RWLock;
    typedef aex_spinlock<traits>    Lock;
    // TODO: 
    typedef aex_allocator<key_type, value_type, traits> Allocator;
    typedef aex_hash_table_con<key_type, traits>        HashTable;
    typedef ULL                                         version_type;
    typedef std::atomic_uint64_t                             atomic_version_type;
};

template<typename traits>
struct aex_default_components{
    typedef typename traits::key_type   key_type;
    typedef typename traits::value_type value_type;
    typedef typename traits::slot_type  slot_type;
    typedef LL size_type;
    typedef aex_concurrency_components<traits> concurrency_components;
    
    typedef typename concurrency_components::atomic_size_type      atomic_size_type;
    typedef typename concurrency_components::ref_count_type ref_count_type;
    typedef typename concurrency_components::Lock           Lock;
    typedef typename concurrency_components::RWLock         RWLock;
    typedef typename concurrency_components::base_node      base_node;
    typedef typename concurrency_components::inner_node     inner_node;
    typedef typename concurrency_components::hash_node      hash_node;
    typedef typename concurrency_components::dense_node     dense_node;    
    typedef typename concurrency_components::data_node      data_node;
    typedef typename concurrency_components::node_ptr       node_ptr;
    typedef typename concurrency_components::inner_node_ptr inner_node_ptr;
    typedef typename concurrency_components::hash_node_ptr  hash_node_ptr;
    typedef typename concurrency_components::dense_node_ptr dense_node_ptr;
    typedef typename concurrency_components::data_node_ptr  data_node_ptr;    
    typedef typename concurrency_components::Allocator      Allocator;
    typedef typename concurrency_components::HashTable      HashTable;
    typedef aex_hash_table_block<key_type, traits>          HashTableBlock;
    typedef typename concurrency_components::version_type   version_type;
    typedef typename concurrency_components::atomic_version_type   atomic_version_type;
    typedef gap_array_linear_model_hash_table<key_type, traits>    InnerNodeModel;
    typedef linear_model<key_type, traits> DataNodeModel;

    typedef aex_bitmap_impl<traits> bitmap_impl;

};

}