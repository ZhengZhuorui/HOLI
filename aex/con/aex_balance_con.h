#pragma once
namespace aex{


#include <type_traits>

#include "aex/aex_utils.h"

namespace aex{

template<typename _Tp,
        typename traits>
struct aex_node_balance_con_stats{
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
struct aex_node_balance_con_stats<std::true_type, traits>{
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
        std::lock_guard<std::mutex> lock(this->mtx);
        double forget_rate = rapid_pow(lambda, timestamp - recent_update_timestamp);
        write_times = write_times * forget_rate;
        train_times = train_times * forget_rate;
        recent_update_timestamp = timestamp;
    }

    inline void update_write_frequency(unsigned long long timestamp){
        std::lock_guard<std::mutex> lock(this->mtx);
        double forget_rate = rapid_pow(lambda, timestamp - recent_update_timestamp);
        write_times = write_times * forget_rate;
        train_times = train_times * forget_rate;
        recent_update_timestamp = timestamp;
        write_times += 1; 
    }

    inline void update_train_frequency(unsigned long long timestamp){
        std::lock_guard<std::mutex> lock(this->mtx);
        double forget_rate = rapid_pow(lambda, timestamp - recent_update_timestamp);
        write_times = write_times * forget_rate;
        train_times = train_times * forget_rate;
        recent_update_timestamp = timestamp;
        train_times += 1; 
    }

    inline double get_write_times(){return write_times;}
    inline double get_train_times(){return train_times;}
    inline double get_recent_update_timestamp(){return recent_update_timestamp;}
    std::mutex mtx;

};

template<typename _Tp, typename traits>
struct aex_tree_balance_con_stats{
    aex_tree_balance_stats(){}
    void update_timestamp(){}
    inline unsigned long long get_timestamp(){return 1;}
    inline double get_lambda_timestamp(){return 1;}
    inline void print_stats(){}
};

template<typename traits>
struct aex_tree_balance_con_stats<std::true_type, traits>{
    unsigned long long timestamp;
    double lambda_timestamp;
    static constexpr double lambda = traits::FORGET_RATE;
    aex_tree_balance_stats():timestamp(0), lambda_timestamp(0){}
    inline void update_timestamp(){
        std::lock_guard<std::mutex> lock(this->mtx);
        this->timestamp++;
        lambda_timestamp = lambda_timestamp * this->lambda + 1;
    }
    inline unsigned long long get_timestamp(){return timestamp;}
    inline double get_lambda_timestamp(){return lambda_timestamp;}
    inline void print_stats(){
        AEX_IMPORTANT("[balance stats]: timestamp=" << timestamp << "lambda timestamp=" << lambda_timestamp);
    }
    std::mutex mtx;
};

}


}