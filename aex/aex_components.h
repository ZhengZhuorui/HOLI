#pragma once
#include <type_traits>

#include "aex/aex_utils.h"

namespace aex{

template<typename _Tp,
        typename traits>
struct aex_node_balance_stats{
    aex_node_balance_stats(){}
    aex_node_balance_stats(unsigned long long _recent_update_timestamp, double _train_times, double _write_times){}
    inline void update_frequency(unsigned long long timestamp){}
    inline void update_write_frequency(unsigned long long timestamp){}
    inline void update_train_frequency(unsigned long long timestamp){}
    inline double get_write_times(){return 0;}
    inline double get_train_times(){return 0;}
    inline double get_recent_update_timestamp(){return 0;}
};

template<typename traits>
struct aex_node_balance_stats<std::true_type, traits>{
    unsigned long long recent_update_timestamp;
    double train_times, write_times;
    static constexpr double lambda = traits::FORGET_RATE;
    aex_node_balance_stats():recent_update_timestamp(0), train_times(0), write_times(0){}
    aex_node_balance_stats(unsigned long long _recent_update_timestamp,
                            double _train_times, double _write_times
                            ):recent_update_timestamp(_recent_update_timestamp), 
                            train_times(_train_times), 
                            write_times(_write_times){} 
    inline void update_frequency(unsigned long long timestamp){
        double forget_rate = rapid_pow(lambda, timestamp - recent_update_timestamp);
        //double forget_rate = lambda_pow(lambda, timestamp - recent_update_timestamp)
        write_times = write_times * forget_rate;
        train_times = train_times * forget_rate;
        recent_update_timestamp = timestamp;
    }

    inline void update_write_frequency(unsigned long long timestamp){
        update_frequency(timestamp);
        write_times += 1; 
    }

    inline void update_train_frequency(unsigned long long timestamp){
        update_frequency(timestamp);
        train_times += 1; 
    }

    inline double get_write_times(){return write_times;}
    inline double get_train_times(){return train_times;}
    inline double get_recent_update_timestamp(){return recent_update_timestamp;}

};

template<typename _Tp, typename traits>
struct aex_tree_balance_stats{
    aex_tree_balance_stats(){}
    void update_timestamp(){}
    inline unsigned long long get_timestamp(){return 1;}
    inline double get_lambda_timestamp(){return 1;}
    inline void print_stats(){}
};

template<typename traits>
struct aex_tree_balance_stats<std::true_type, traits>{
    unsigned long long timestamp;
    double lambda_timestamp;
    static constexpr double lambda = traits::FORGET_RATE;
    aex_tree_balance_stats():timestamp(0), lambda_timestamp(0){}
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

template<typename _Tp>
struct aex_node_mutex{
    inline void lock(){}
    inline void unlock(){}
    inline void lock_shared(){}
    inline void unlock_shared(){}
    inline bool try_lock_shared(){}
    //inline void inc_cnt(){}
    //inline void dec_cnt(){}
    //inline int get_cnt(){return 0;}
};


template<>
struct aex_node_mutex<std::true_type>{
    inline void lock(){mutex.lock();}
    inline void unlock(){
        mutex.unlock();
    }
    inline void lock_shared(){mutex.lock_shared();}
    inline void unlock_shared(){mutex.unlock_shared();}
    inline bool try_lock_shared(){return mutex.try_lock_shared();}
    inline void wait_to_unlock(){
        std::lock_guard<std::mutex> lk(cv_mutex);
        cv_mutex.wait(lk);
    }
    //inline void inc_cnt(){++cnt;}
    //inline void dec_cnt(){--cnt;}
    //inline int get_cnt(){return cnt.load();}
    std::shared_mutex mutex;
    std::condition_variable cv;
    std::mutex cv_mutex;
};

template<typename _Tp>
struct aex_node_spinlock{
    inline void lock(){}
    inline void unlock(){}
    inline void lock_shared(){}
    inline void unlock_shared(){}
    inline void try_lock_shared(){}
    //inline bool lock_shared_to_unique(){}
    //inline void inc_cnt(){}
    //inline void dec_cnt(){}
    int get_cnt(){return 0;}
};


template<>
struct aex_node_spinlock<std::true_type>{
    aex_node_spinlock() : writeLock(false), readCount(0) {}
    void lock_shared() {
        while (writeLock.load(std::memory_order_acquire));
        readCount.fetch_add(1, std::memory_order_acquire);
    }

    void unlock_shared() {
        readCount.fetch_sub(1, std::memory_order_release);
    }

    void lock() {
        bool expected = false;
        while (!writeLock.compare_exchange_weak(expected, true, std::memory_order_acquire)) {
            expected = false;
        }
        while (readCount.load(std::memory_order_acquire) > 0);
    }

    void unlock() {
        writeLock.store(false, std::memory_order_release);
    }

    bool try_lock_shared(){
        if (writeLock.load(std::memory_order_acquire)){
            return false;
        }
        readCount.fetch_add(1, std::memory_order_acquire);
        return true;
    }

    int get_cnt(){
        return readCount.load(std::memory_order_acquire);
    }
    std::atomic<bool> writeLock;
    std::atomic<int> readCount;
};


template<typename _Key, 
        typename _Val,
        #ifdef AEX_TLI
        typename SearchClass
        #endif
        typename _Tp>
struct aex_concurrency_components{
    #ifdef AEX_TLI
    typedef aex_node_base<_Key, _Val, SearchClass, traits> base_node;

    typedef aex_inner_node<_Key, _Val, SearchClass, traits> inner_node;

    typedef aex_static_data_node<_Key, _Val, SearchClass, traits> static_data_node;
#else
    
    typedef aex_node_base<_Key, _Val, traits> base_node;

    typedef aex_inner_node<_Key, _Val, traits> inner_node;

    typedef aex_static_data_node<_Key, _Val, traits> static_data_node;
#endif
    typedef aex_node_spinlock<_Tp> aex_node_mutex;

    typedef aex_node_allocator<> NodeAllocator;

};

template<typename _Key, 
        typename _Val,
        #ifdef AEX_TLI
        typename SearchClass
        #endif
        >
struct aex_concurrency_components<std::true_type>{
    #ifdef AEX_TLI
    typedef aex_node_base<_Key, _Val, SearchClass, traits> base_node;

    typedef aex_inner_node_con<_Key, _Val, SearchClass, traits> inner_node;

    typedef aex_static_data_node_con<_Key, _Val, SearchClass, traits> static_data_node;
#else
    
    typedef aex_node_base<_Key, _Val, traits> base_node;

    typedef aex_inner_node_con<_Key, _Val, traits> inner_node;

    typedef aex_static_data_node_con<_Key, _Val, traits> static_data_node;
#endif
    typedef aex_node_mutex<std::true_type> aex_node_mutex;

    typedef aex_node_allocator_con node_allocator;
};

template<typename _Tp>
struct aex_balance_components{
    typedef aex_node_balance_stats<_Tp> node_balance_stats;
    typedef aex_tree_balance_stats<_Tp> tree_balance_stats;
};

template<
#ifdef AEX_TLI
typename SearchClass,
#endif
typename traits>
struct aex_components{
    typedef aex_balance_components<typename traits::AllowBalance> balance_componets;
    typedef aex_concurrency_components<typename traits::key_type, 
                                    typename traits::value_type, 
                                    #ifdef AEX_TLI
                                    typename SearchClass,
                                    #endif
                                    typename traits::AllowMultiThread> concurrency_components;

    typedef typename balance_componets<traits::AllowBalance>::node_balance_stats node_balance_stats;
    typedef typename balance_componets<traits::AllowBalance>::tree_balance_stats tree_balance_stats;

    typedef typename concurrency_components::aex_node_mutex aex_node_mutex;
    typedef typename concurrency_components::base_node base_node;
    typedef typename concurrency_components::inner_node inner_node;
    typedef typename concurrency_components::data_node data_node;
    typedef typename concurrency_components::node_allocator node_allocator;
};

}