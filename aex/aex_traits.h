#pragma once
namespace aex{

template<typename _Tp>
class aex_type_traits{

};

template<>
class aex_type_traits<int>{
    typedef int64_t args_type; 
};

template<>
class aex_type_traits<long long>{
    typedef int64_t args_type; 
};

template<>
class aex_type_traits<float>{
    typedef float args_type; 
};

template<>
class aex_type_traits<double>{
    typedef double args_type; 
};

template<typename _Tp>
class linear_model;

template<typename _Key, 
        typename _Val,
        typename _used_as_set,
        typename _AllowMultiThread=std::false_type,
        typename _AllowBalance=std::false_type>
struct aex_default_traits{

    typedef _Key key_type;

    typedef _Val value_type;

    typedef size_t size_type;

    typedef unsigned long long bitmap_base;

    typedef bitmap_base* bitmap;

    typedef unsigned char version_type;

    //typedef unsigned long long pos_type;

    typedef _used_as_set used_as_set;

    typedef _AllowMultiThread AllowMultiThread;

    typedef _AllowBalance AllowBalance;
    
    static const int ERROR_BOUND = 4;

    static const int MIN_INNER_NODE_SLOT_SIZE = 8;

    static const int MIN_ML_INNER_NODE_SLOT_SIZE = 16;

    static const int MAX_INNER_NODE_SLOT_SIZE = 1 << 20;

    static const int MIN_DATA_NODE_SLOT_SIZE = 8;

    static const int MAX_DATA_NODE_SLOT_SIZE= 1 << 20;

    static const int MIN_ML_DATA_NODE_SLOT_SIZE = 16;
    
    static const int MIN_COMPLEX_ML_DATA_NODE_SLOT_SIZE = 4096;

    static constexpr float DATA_NODE_FEW_RATIO = 0.5;
        
    static constexpr float DATA_NODE_FULL_RATIO = 1;
    
    static constexpr float INNER_NODE_FEW_RATIO = 0.4;
    
    static constexpr float INNER_NODE_FULL_RATIO = 0.875;
    
    static const int EXPAND_RATIO = 2;

    static constexpr float NARROW_RATIO = 0.5;

    static constexpr float MERGE_COST_PARA = 1;

    static constexpr float SPLIT_COST_PARA = 1;

    static const int BINEARY_SEARCH_SIZE = 32;

    static const int MIN_BLOCK_SIZE = 128;

    static const int NODE_MUTEX_SLOT_SIZE = ERROR_BOUND;

    static const int MAX_LEVEL = 16;

    static const char INIT_REWIRED_CNT = 5;

    static const int LAMBDA_ = 100000;

    static constexpr float LEARNING_COST = 10;

    static const int CACHE_LINE_SIZE = MIN_DATA_NODE_SLOT_SIZE;

    static constexpr float MAX_ALLOW_ERROR = 0.25;

    static const bool debug = true;
    
};

}