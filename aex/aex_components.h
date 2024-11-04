#pragma once
#include <type_traits>
#include <atomic>

#include "aex/aex_utils.h"

namespace aex{

template<typename traits, bool _ = traits::AllowConcurrency>
struct aex_spinlock{
    aex_spinlock(){}
    ~aex_spinlock(){}
    inline void lock(){}
    inline void unlock(){}
    inline bool is_lock(){return false;}
};

template<typename traits>
struct aex_spinlock<traits, true>{
    typedef aex_spinlock<traits, true> self;
    aex_spinlock() : writeLock(false) {}
    aex_spinlock(const self &x){}
    aex_spinlock(const self &&x){}
    ~aex_spinlock(){}
    self& operator = (const self &x){return *this;}
    self& operator = (const self &&x){return *this;}
    inline void lock() {
        bool expected = false;
        while (!writeLock.compare_exchange_weak(expected, true, std::memory_order_acquire)) {
            expected = false;
        }
    }
    inline void unlock() {
        writeLock.store(false, std::memory_order_release);
    }
    inline bool is_lock(){return false;}

    std::atomic<bool> writeLock;
};

template<typename traits, bool _ = traits::AllowConcurrency>
struct aex_rw_spinlock{
    aex_rw_spinlock(){}
    inline void lock(){}
    inline void unlock(){}
    inline void lock_shared(){}
    inline void unlock_shared(){}
    inline void upgrade_lock(){}
    inline void downgrade_lock(){}
    inline bool try_lock(){return true;}
    inline bool try_lock_shared(){return true;}
    inline bool try_upgrade_lock(){return true;}
    inline bool is_lock(){return false;}
    inline bool is_lock_shared(){return false;}
};

template<typename traits>
struct aex_rw_spinlock<traits, true>{
    typedef aex_rw_spinlock<traits, true> self;
    //aex_rw_spinlock() : writeLock(false), readCount(0) {}
    aex_rw_spinlock() : lockCount(0) {}
    aex_rw_spinlock(const self &x) = delete;
    aex_rw_spinlock(const self &&x) = delete;
    ~aex_rw_spinlock(){}
    self& operator = (const self &x) = delete;
    self& operator = (const self &&x) = delete;
    void lock_shared() {
        int expected = lockCount.load() & (~1);
        int result   = expected + 0b10;
        while (!lockCount.compare_exchange_weak(expected, result, std::memory_order_acquire)) {
            expected = lockCount.load() & (~1);
            result   = expected + 0b10;
        }
    }

    void unlock_shared() {
        lockCount.fetch_sub(0b10);
    }
    
    bool try_lock_shared(){
        int expected = lockCount.load() & (~1);
        int result   = expected + 0b10;
        if (!lockCount.compare_exchange_weak(expected, result, std::memory_order_acquire)) 
            return false;
        return true;
    }

    void lock() {
        //bool expected = false;
        int expected = lockCount.load() & (~1);
        int result   = expected | 1;
        while (!lockCount.compare_exchange_weak(expected, result, std::memory_order_acquire)) {
            expected = lockCount.load() & (~1);
            result   = expected | 1;
        }
        while (lockCount.load() >= 0b10);
    }

    void unlock() {
        //writeLock.store(false, std::memory_order_release);
        lockCount.store(0, std::memory_order_release);
    }

    bool try_lock(){
        int expected = lockCount.load() & (~1);
        int result   = expected | 1;
        if (!lockCount.compare_exchange_weak(expected, result, std::memory_order_acquire)) return false;
        while (lockCount.load() >= 0b10);
        return true;
    }

    bool try_upgrade_lock(){
        AEX_ASSERT(lockCount.load() >= 0b10);
        int expected = lockCount.load() & (~1);
        int result   = expected - 1;
        if (!lockCount.compare_exchange_weak(expected, result, std::memory_order_acquire)) 
            return false;
        while (lockCount.load() >= 0b10);
        return true;
    }

    // unused. may deadlock
    void upgrade_lock(){
        int expected, result;
        AEX_ASSERT(lockCount.load() >= 0b10);
        expected = lockCount.load() & (~1);
        result   = expected - 1;
        while (!lockCount.compare_exchange_weak(expected, result, std::memory_order_acquire)) {
            expected = lockCount.load() & (~1);
            result   = expected - 1;
        }
        while (lockCount.load() >= 0b10);
    }

    void downgrade_lock(){
        lockCount.fetch_add(1);
    }

    inline bool is_lock(){return (lockCount & 1) == 1;}
    inline bool is_lock_shared(){return (lockCount >> 1) > 0;}
    //std::atomic<bool> writeLock;
    //std::atomic<int> readCount;
    std::atomic<int> lockCount;
};

template<typename traits, bool AllowSplitBalance = traits::AllowSplitBalance>
struct aex_node_split_stats{
    aex_node_split_stats(){}
    inline void update(long long _cnt){}
    inline void set(long long _cnt){}
    inline long long get(){return 1;}
};

template<typename traits>
struct aex_node_split_stats<traits, true>{
    aex_node_split_stats(){}
    inline void update(long long _cnt){cnt += _cnt;}
    inline void set(long long _cnt){cnt =  _cnt;}
    inline long long get(){return cnt;}
    long long cnt;
};


template<typename traits, bool AllowBalance = traits::AllowBalance, bool AllowConcurrency = traits::AllowConcurrency>
struct aex_node_balance_stats{
    aex_node_balance_stats(){}
    aex_node_balance_stats(unsigned long long _recent_update_timestamp, double _SMO_times, double _write_times){}
    inline void update_frequency(unsigned long long timestamp){}
    inline void update_write_frequency(unsigned long long timestamp){}
    inline void update_SMO_frequency(unsigned long long timestamp){}
    inline double get_write_times(){return 0;}
    inline double get_SMO_times(){return 0;}
    inline double get_recent_update_timestamp(){return 0;}
};

template<typename traits>
struct aex_node_balance_stats<traits, true, false>{
    unsigned long long recent_update_timestamp;
    double SMO_times, write_times;
    static constexpr double lambda = traits::FORGET_RATE;
    aex_node_balance_stats():recent_update_timestamp(0), SMO_times(0), write_times(0){}
    aex_node_balance_stats(unsigned long long _recent_update_timestamp,
                            double _SMO_times, double _write_times
                            ):recent_update_timestamp(_recent_update_timestamp), 
                            SMO_times(_SMO_times), 
                            write_times(_write_times){} 
    inline void update_frequency(unsigned long long timestamp){
        double forget_rate = rapid_pow(lambda, timestamp - recent_update_timestamp);
        //double forget_rate = lambda_pow(lambda, timestamp - recent_update_timestamp)
        write_times = write_times * forget_rate;
        SMO_times = SMO_times * forget_rate;
        recent_update_timestamp = timestamp;
    }

    inline void update_write_frequency(unsigned long long timestamp){
        update_frequency(timestamp);
        write_times += 1; 
    }

    inline void update_SMO_frequency(unsigned long long timestamp){
        update_frequency(timestamp);
        SMO_times += 1; 
    }

    inline double get_write_times(){return write_times;}
    inline double get_SMO_times(){return SMO_times;}
    inline double get_recent_update_timestamp(){return recent_update_timestamp;}

};

template<typename traits>
struct aex_node_balance_stats<traits, true, true>{
    typedef aex_node_balance_stats<traits, true, true> self;
    unsigned long long recent_update_timestamp;
    double SMO_times, write_times;
    static constexpr double lambda = traits::FORGET_RATE;
    aex_node_balance_stats():recent_update_timestamp(0), SMO_times(0), write_times(0){}
    aex_node_balance_stats(unsigned long long _recent_update_timestamp,
                            double _SMO_times, double _write_times
                            ):recent_update_timestamp(_recent_update_timestamp), 
                            SMO_times(_SMO_times), 
                            write_times(_write_times){} 
    //self& operator = (self &x){
    //    recent_update_timestamp = x.recent_update_timestamp;
    //    train_times = x.train_times;
    //    write_times = x.write_times;
    //    return *this;
    //}
    //self& operator = (self &&x){
    //    recent_update_timestamp = x.recent_update_timestamp;
    //    train_times = x.train_times;
    //    write_times = x.write_times;
    //    return *this;
    //}
    inline void update_frequency(unsigned long long timestamp){
        lk.lock();
        double forget_rate = rapid_pow(lambda, timestamp - recent_update_timestamp);
        write_times = write_times * forget_rate;
        SMO_times = SMO_times * forget_rate;
        recent_update_timestamp = timestamp;
        lk.unlock();
    }

    inline void update_write_frequency(unsigned long long timestamp){
        lk.lock();
        double forget_rate = rapid_pow(lambda, timestamp - recent_update_timestamp);
        write_times = write_times * forget_rate;
        SMO_times = SMO_times * forget_rate;
        recent_update_timestamp = timestamp;
        write_times += 1; 
        lk.unlock();
    }

    inline void update_SMO_frequency(unsigned long long timestamp){
        lk.lock();
        double forget_rate = rapid_pow(lambda, timestamp - recent_update_timestamp);
        write_times = write_times * forget_rate;
        SMO_times = SMO_times * forget_rate;
        recent_update_timestamp = timestamp;
        SMO_times += 1; 
        lk.unlock();
    }

    inline double get_write_times(){return write_times;}
    inline double get_SMO_times(){return SMO_times;}
    inline double get_recent_update_timestamp(){return recent_update_timestamp;}

    aex_spinlock<traits> lk;
};

template<typename traits, bool AllowBalance = traits::AllowBalance, bool AllowConcurrency = traits::AllowConcurrency>
struct aex_tree_balance_stats{
    aex_tree_balance_stats(){}
    ~aex_tree_balance_stats(){}
    void update_timestamp(){}
    inline unsigned long long get_timestamp(){return 1;}
    inline double get_lambda_timestamp(){return 1;}
    inline void print_stats(){}
};

template<typename traits>
struct aex_tree_balance_stats<traits, true, false>{
    unsigned long long timestamp;
    double lambda_timestamp;
    static constexpr double lambda = traits::FORGET_RATE;
    aex_tree_balance_stats():timestamp(0), lambda_timestamp(0){}
    ~aex_tree_balance_stats(){}
    inline void update_timestamp(){
        this->timestamp++;
        lambda_timestamp = lambda_timestamp * this->lambda + 1;
    }
    inline unsigned long long get_timestamp(){return timestamp;}
    inline double get_lambda_timestamp(){return lambda_timestamp;}
    inline void print_stats(){
        AEX_IMPORTANT("[balance stats]: timestamp=" << timestamp << "lambda timestamp=" << lambda_timestamp);
    }
};

template<typename traits>
struct aex_tree_balance_stats<traits, true, true>{
    typedef aex_tree_balance_stats<traits, true, true> self;
    volatile long long timestamp;
    volatile double lambda_timestamp;
    static constexpr double lambda = traits::FORGET_RATE;
    aex_tree_balance_stats():timestamp(0), lambda_timestamp(0){}
    ~aex_tree_balance_stats(){}
    //self& operator = (const self &x){
    //    timestamp = x.timestamp; 
    //    lambda_timestamp = x.lambda_timestamp;
    //    return *this;
    //}
    //self& operator = (self &&x){
    //    timestamp = x.timestamp;
    //    lambda_timestamp = x.lambda_timestamp;
    //    return *this;
    //}
    inline void update_timestamp(){
        lk.lock();
        this->timestamp++;
        lambda_timestamp = lambda_timestamp * this->lambda + 1;
        lk.unlock();
    }
    inline unsigned long long get_timestamp(){return timestamp;}
    inline double get_lambda_timestamp(){return lambda_timestamp;}
    inline void print_stats(){
        AEX_IMPORTANT("[balance stats]: timestamp=" << timestamp << "lambda timestamp=" << lambda_timestamp);
    }
    aex_spinlock<traits> lk;
};

//template<typename T* >
//struct no_atomic_ptr{
//    typedef T* _Ptr;
//    typedef no_atomic_ptr<T*> self;
//
//    no_atomic_ptr(_Ptr t){ptr = t;}
//
//    self& operator = (_Ptr t){ptr = t; return *this;}
//
//    self& operator = (self &x){ptr = x.ptr;return *this;}
//
//    inline _Ptr load(){return ptr;}
//
//    inline void store(_Ptr t){ptr = t;}
//
//    T* ptr;
//}

template<typename T>
struct empty_type{
    empty_type(){}
    empty_type& operator=(T &x){return *this;}
    bool operator==(T &x){return true;}
};

template<typename traits, bool _ = traits::AllowConcurrency>
struct aex_concurrency_components{
    typedef typename traits::key_type   key_type;
    typedef typename traits::value_type value_type;
    typedef unsigned long long          size_type;

    typedef aex_node_base<key_type, value_type, traits>        base_node;
    typedef aex_inner_node<key_type, value_type, traits>       inner_node;
    typedef aex_hash_node<key_type, value_type, traits>        hash_node;
    typedef aex_dense_node<key_type, value_type, traits>       dense_node;
    typedef aex_static_data_node<key_type, value_type, traits> data_node;
    typedef base_node* node_ptr;
    typedef inner_node* inner_node_ptr;
    typedef hash_node* hash_node_ptr;
    typedef dense_node* dense_node_ptr;
    typedef data_node* data_node_ptr;    
    
    typedef aex_rw_spinlock<traits> RWLock;
    typedef aex_spinlock<traits>    Lock;
    typedef aex_allocator<key_type, value_type, traits> Allocator;
    typedef aex_hash_table<key_type, traits>            HashTable;
    typedef empty_type<unsigned long long> version_type;
};

template<typename traits>
struct aex_concurrency_components<traits, true>{
    typedef typename traits::key_type key_type;
    typedef typename traits::value_type value_type;
    typedef std::atomic_uint64_t size_type;

    typedef aex_node_base<key_type, value_type, traits>            base_node;
    typedef aex_inner_node<key_type, value_type, traits>           inner_node;
    typedef aex_hash_node_con<key_type, value_type, traits>        hash_node;
    typedef aex_dense_node<key_type, value_type, traits>           dense_node;
    typedef aex_static_data_node<key_type, value_type, traits>     data_node;

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

    typedef unsigned long long version_type;
};

template<typename traits>
struct aex_default_components{
    typedef typename traits::key_type   key_type;
    typedef typename traits::value_type value_type;
    typedef aex_concurrency_components<traits> concurrency_components;

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
    typedef typename concurrency_components::size_type      size_type;
    typedef gap_array_linear_model_hash_table<key_type, traits> InnerNodeModel;
    typedef linear_model<key_type, traits> DataNodeModel;

    typedef aex_bitmap_impl<traits> bitmap_impl;
};

}