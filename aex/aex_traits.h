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

template<typename _Key, 
        typename _Val,
        typename _AllowMultiKey=std::false_type,
        typename _AllowMultiThread=std::false_type>
struct aex_default_traits{

    typedef _Key key_type;

    typedef _Val value_type;

    typedef unsigned long long size_type;

    typedef int pos_type;

    typedef unsigned long long bitmap_base;

    typedef bitmap_base* bitmap;

    typedef unsigned char version_type;

    //typedef unsigned long long pos_type;

    typedef std::false_type used_as_set;

    typedef _AllowMultiKey AllowMultiKey;

    typedef _AllowMultiThread AllowMultiThread;

    // Allow balance inner node and data node when read and write frequency update?
    typedef std::false_type AllowRWBalance;

    // Allow balance inner node when insert an item?
    typedef std::false_type AllowInsertBalance;

    // Allow data node slot size dynamic? (static data node slot size is MIN_DATA_NODE_SLOT_SIZE)
    typedef std::true_type AllowDynamicDataNode;
    //typedef std::false_type AllowDynamicDataNode;
    
    static const int ERROR_BOUND = 8;

    static const pos_type MIN_INNER_NODE_SLOT_SIZE = 8;

    static const pos_type MIN_ML_INNER_NODE_SLOT_SIZE = 64;

    static const pos_type MAX_NODE_SLOT_SIZE = 1 << 25;

    static const pos_type MIN_DATA_NODE_SLOT_SIZE = 8;

    static const pos_type MAX_DATA_NODE_SLOT_SIZE= 1 << 20;

    static const pos_type MIN_ML_DATA_NODE_SLOT_SIZE = 32;
    
    static constexpr float DATA_NODE_FEW_RATIO = 0.5;
        
    static constexpr float DATA_NODE_FULL_RATIO = 1;

    static constexpr float DENSITY_NARROW_RATIO = 0.5;
    //static constexpr float DENSITY_NARROW_RATIO = 0.75;
    
    static constexpr float EXPAND_RATIO = DATA_NODE_FULL_RATIO / DATA_NODE_FEW_RATIO;

    static constexpr float NARROW_RATIO = 0.5;

    static constexpr float MERGE_COST_PARA = 1;

    static constexpr float SPLIT_COST_PARA = 1;

    static const int BINEARY_SEARCH_SIZE = 32;

    static const int NODE_MUTEX_SLOT_SIZE = ERROR_BOUND;

    static const int MAX_DEPTH = 16;

    static const char INIT_REWIRED_CNT = 5;

    static const int LAMBDA_ = 100000;

    static constexpr float LEARNING_COST = 10;

    static const int CACHE_LINE_SIZE = MIN_DATA_NODE_SLOT_SIZE;

    static constexpr float MAX_ALLOW_ERROR = 2.0 / log(2);

    static constexpr float MAX_LINEAR_PROBE_ALLOW_ERROR = 4.0 / log(2);

    static const bool debug = true;

    static const int MAX_SEGMENT_NUM = 8;
    
};

template<typename _Key, 
        typename _Val,
        typename _AllowMultiKey=std::false_type,
        typename _AllowMultiThread=std::false_type>
struct aex_rw_balance_traits : public aex::aex_default_traits<_Key, _Val, _AllowMultiKey, _AllowMultiThread>{
    // Allow balance inner node and data node when read and write frequency update?
    typedef std::true_type AllowRWBalance;
};


}