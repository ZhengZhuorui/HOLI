#pragma once
#include <type_traits>
#include <atomic>

#include "aex_utils.h"
#include "concurrency/aex_lock.h"

namespace aex{

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

enum class ConcurrencyType{
    None,
    LockArray,
    GetChilds,
    ConstructSMO,
    HashTableRescale,
};

struct ConcurrencyParams{
    explicit ConcurrencyParams(ConcurrencyType _type): finish_flag(false), type(_type) {}
    //virtual void operator()() = 0;
    volatile bool finish_flag;
    ConcurrencyType type;
};

template<typename traits, bool _ = traits::AllowConcurrency>
struct aex_concurrency_components{
    typedef typename traits::key_type   key_type;
    typedef typename traits::value_type value_type;
    typedef no_atomic_type<LL>          atomic_size_type;
    typedef empty_type<unsigned int>    ref_count_type;

    typedef aex_node_base<key_type, value_type, traits>        base_node;
    typedef aex_inner_node<key_type, value_type, traits>       inner_node;
    typedef aex_hash_node<key_type, value_type, traits>        hash_node;
    typedef aex_dense_node<key_type, value_type, traits>       dense_node;
    typedef aex_static_data_node<key_type, value_type, traits> data_node;
    //typedef aex_data_node_con<key_type, value_type, traits> data_node;
    typedef base_node*  node_ptr;
    typedef inner_node* inner_node_ptr;
    typedef hash_node*  hash_node_ptr;
    typedef dense_node* dense_node_ptr;
    typedef data_node*  data_node_ptr;    
    
    //typedef aex_rw_spinlock<traits> RWLock;
    typedef OptLock<traits>         RWLock;
    typedef aex_spinlock<traits>    Lock;
    typedef aex_allocator<key_type, value_type, traits> Allocator;
    typedef aex_hash_table_block<key_type, traits>      HashTableBlock;
    typedef aex_hash_table<key_type, traits>            HashTable;
    typedef ULL                                         version_type;
    typedef std::atomic<version_type>                   atomic_version_type;
    typedef ULL                                         ID_type;
    typedef no_atomic_type<ID_type>                     atomic_ID_type;
    //typedef empty_type<unsigned long long> version_type;
};

template<typename traits>
struct aex_concurrency_components<traits, true>{
    typedef typename traits::key_type key_type;
    typedef typename traits::value_type value_type;
    typedef std::atomic_int64_t atomic_size_type;
    typedef std::atomic_uint8_t ref_count_type;

    typedef aex_node_base<key_type, value_type, traits>            base_node;
    typedef aex_inner_node<key_type, value_type, traits>           inner_node;
    typedef aex_hash_node_con<key_type, value_type, traits>        hash_node;
    typedef aex_dense_node<key_type, value_type, traits>           dense_node;
    typedef aex_data_node_con<key_type, value_type, traits>        data_node;
    //typedef aex_static_data_node<key_type, value_type, traits>        data_node;

    typedef base_node*  node_ptr;
    typedef inner_node* inner_node_ptr;
    typedef hash_node*  hash_node_ptr;
    typedef dense_node* dense_node_ptr;
    typedef data_node*  data_node_ptr;    
    //typedef std::atomic<base_node*> atomicPtr;

    //typedef aex_rw_spinlock<traits> RWLock;
    typedef OptLock<traits>         RWLock;
    typedef aex_spinlock<traits>    Lock;
    // TODO: 
    typedef aex_allocator<key_type, value_type, traits> Allocator;
    typedef aex_hash_table_block_con<key_type, traits>  HashTableBlock;
    //typedef aex_hash_table_block<key_type, traits>      HashTableBlock;
    typedef aex_hash_table_con<key_type, traits>        HashTable;
    //typedef aex_hash_table<key_type, traits>        HashTable;
    typedef uint64_t                                    version_type;
    typedef std::atomic<version_type>                   atomic_version_type;
    typedef ULL                                         ID_type;
    typedef std::atomic<ID_type>                        atomic_ID_type;
    
};

template<typename traits>
struct aex_default_components{
    typedef typename traits::key_type   key_type;
    typedef typename traits::value_type value_type;
    typedef typename traits::slot_type  slot_type;
    typedef aex_concurrency_components<traits> concurrency_components;
    
    typedef aex_tree<key_type, value_type, traits>          Index;

    typedef LL size_type;
    typedef typename concurrency_components::atomic_size_type      atomic_size_type;
    typedef typename concurrency_components::ref_count_type        ref_count_type;
    typedef typename concurrency_components::Lock                  Lock;
    typedef typename concurrency_components::RWLock                RWLock;
    typedef typename concurrency_components::base_node             base_node;
    typedef typename concurrency_components::inner_node            inner_node;
    typedef typename concurrency_components::hash_node             hash_node;
    typedef typename concurrency_components::dense_node            dense_node;    
    typedef typename concurrency_components::data_node             data_node;
    typedef typename concurrency_components::node_ptr              node_ptr;
    typedef typename concurrency_components::inner_node_ptr        inner_node_ptr;
    typedef typename concurrency_components::hash_node_ptr         hash_node_ptr;
    typedef typename concurrency_components::dense_node_ptr        dense_node_ptr;
    typedef typename concurrency_components::data_node_ptr         data_node_ptr;    
    typedef typename concurrency_components::Allocator             Allocator;
    typedef typename concurrency_components::HashTableBlock        HashTableBlock;
    typedef typename concurrency_components::HashTable             HashTable;
    typedef typename concurrency_components::version_type          version_type;
    typedef typename concurrency_components::atomic_version_type   atomic_version_type;
    typedef typename concurrency_components::ID_type               ID_type;
    typedef typename concurrency_components::atomic_ID_type        atomic_ID_type;
    typedef aex_hash_table<key_type, traits>                           HashTableBase;
    typedef gap_array_linear_model_hash_table<key_type, traits>    InnerNodeModel;
    typedef linear_model<key_type, traits> DataNodeModel;
    typedef MemoryReclaimUnit<traits>      MRUnit;
    typedef aex_ThreadSpecificEpochBasedReclamationInformation<traits> ThreadSpecificEpochBasedReclamationInformation;
    typedef aex_EpochBasedMemoryReclamationStrategy<traits>            EpochBasedMemoryReclamationStrategy;
    typedef aex_EpochGuard<traits>                                     EpochGuard;
    //typedef typename boost::lockfree::stack<std::function<void()>> LockFreeStack;
    //typedef typename boost::lockfree::queue<std::function<void()>> LockFreeQueue;
    //typedef typename boost::lockfree::queue<ConcurrencyParams*> LockFreeQueue;
    typedef typename boost::lockfree::queue<ConcurrencyParams*, boost::lockfree::capacity<1024> > LockFreeQueue;
    typedef _LockArrayParams<traits> LockArrayParams;
    typedef _GetChildsParams<traits> GetChildsParams;
    typedef _ConstructSMOParams<traits> ConstructSMOParams;
    typedef _HashTableRescaleParams<traits> HashTableRescaleParams;

    typedef aex_bitmap_impl<traits> bitmap_impl;

};

}
