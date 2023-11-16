#pragma once
#include <cmath>
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

struct aex_default_balance_args{
    static constexpr double INNER_NODE_MODEL_SEARCH_FACTOR = 2.0; // find a child/data with learned model needs MODEL_SEARCH_FACTOR cost
    static constexpr double DATA_NODE_MODEL_SEARCH_FACTOR = 1.0; // find a child/data with learned model needs MODEL_SEARCH_FACTOR cost
    static constexpr double BINEARY_SEARCH_FACTOR = 1.0; // find a child/data with bineary search needs BINEARY_SEARCH_FACTOR * log(n) cost

    static constexpr double DENSE_ARRAY_INSERT_FACTOR = 1.0; // insert a child/data in dense array needs DENSE_ARRAY_INSERT_FACTOR * n cost
    static constexpr double GAP_ARRAY_INSERT_FACTOR = 8.0; // insert a child/data in gap array needs GAP_ARRAY_INSERT_FACTOR cost

    static constexpr double DATA_NODE_TRAIN_FACTOR = 1.0; // train a data node needs DATA_NODE_TRAIN_FACTOR * n cost
    static constexpr double INNER_NODE_TRAIN_FACTOR = 8.0; // train a inner node needs INNER_NODE_TRAIN_FACTOR * n cost
};

#define AEX_MAX(a, b) (((a) < (b)) ? (b) : (a))

template<typename _Key, 
        typename _Val,
        typename _AllowMultiThread=std::false_type>
struct aex_default_traits{

    typedef _Key key_type;

    typedef _Val value_type;

    typedef unsigned long long size_type;

    typedef int slot_type;

    typedef unsigned long long bitmap_base;

    typedef bitmap_base* bitmap;

    typedef unsigned char version_type;

    typedef aex_default_balance_args MODEL_ARGS;

    typedef _AllowMultiThread AllowMultiThread;

    // Allow balance inner node and data node when read and write frequency update?
    typedef std::false_type AllowRWBalance;

    // Allow balance inner node when insert an item?
    typedef std::false_type AllowInsertBalance;

    // Allow tree balance tree struct in lookup, insert and erase.
    typedef std::false_type AllowBalance;

    static_assert((AllowRWBalance::value | AllowInsertBalance::value) == AllowBalance::value);

    // Allow data node slot size dynamic? (static data node slot size is MIN_DATA_NODE_SLOT_SIZE)
    // If data node slot size is dynamic(lazy update), it must AllowRWBalance.
    typedef std::false_type AllowDynamicDataNode;

    static_assert((AllowRWBalance::value | (!AllowDynamicDataNode::value)) == true);
    
    static constexpr int ERROR_BOUND = 8;

    static constexpr int DATA_NODE_ERROR_BOUND = 2;

    //static constexpr slot_type MIN_INNER_NODE_SLOT_SIZE = AEX_MAX(16, 256 / (sizeof(key_type) + sizeof(void*)));
    static constexpr slot_type MIN_INNER_NODE_SLOT_SIZE = 8;

    //static constexpr slot_type MIN_DATA_NODE_SLOT_SIZE = AEX_MAX(16, 256 / (sizeof(key_type)));
    static constexpr slot_type MIN_DATA_NODE_SLOT_SIZE = 8;

    static constexpr slot_type MIN_ML_INNER_NODE_SLOT_SIZE = MIN_INNER_NODE_SLOT_SIZE;

    static constexpr slot_type MIN_ML_INNER_NODE_SIZE = MIN_INNER_NODE_SLOT_SIZE;

    static constexpr slot_type MAX_INNER_NODE_SLOT_SIZE = 1 << 25;

    static constexpr slot_type MAX_DATA_NODE_SLOT_SIZE= 1 << 20;

    static constexpr slot_type MIN_ML_DATA_NODE_SLOT_SIZE = 32;
    
    static constexpr float DATA_NODE_FEW_RATIO = 0.5;
        
    static constexpr float DATA_NODE_FULL_RATIO = 1;

    static constexpr float DENSITY_NARROW_RATIO = 0.5;

    static constexpr float EXPAND_RATIO = 2;

    static constexpr float MERGE_COST_PARA = 1;

    static constexpr float SPLIT_COST_PARA = 1;

    static constexpr int BINEARY_SEARCH_SIZE = 32;

    static constexpr int NODE_MUTEX_SLOT_SIZE = ERROR_BOUND;

    static constexpr int MAX_DEPTH = 16;

    static constexpr float MAX_ALLOW_ERROR = 0.5 / log(2);

    static constexpr bool debug = true;

    static constexpr int MAX_SEGMENT_NUM = 4;

    static constexpr unsigned long long INNER_NODE_MAX_DIFFERENT_VALUE = 0x10000000000000ULL;
};

}