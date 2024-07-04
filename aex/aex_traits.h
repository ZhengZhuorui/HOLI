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
        bool _AllowMultiKey=false,
        typename _SearchClass=void,
        bool _AllowConcurrency=false,
        int _AllowBalance=0,
        bool _AllowDynamicDataNode=false,
        bool _AllowMergeNode=false,
        int _ERROR_BOUND=8,
        int _MAX_MODEL_ARGS=8>
struct aex_default_traits{

    typedef _Key key_type;

    typedef _Val value_type;

    typedef _SearchClass SearchClass;

    typedef unsigned long long size_type;

    typedef int slot_type;

    typedef unsigned long long bitmap_base;

    typedef bitmap_base* bitmap;

    typedef unsigned char version_type;

    typedef aex_default_balance_args MODEL_ARGS;

    static constexpr bool AllowMultiKey = _AllowMultiKey;

    static constexpr bool AllowConcurrency = _AllowConcurrency;

    // Allow balance inner node and data node when read and write frequency update?
    //typedef std::true_type AllowRWBalance;
    static constexpr bool AllowRWBalance = ((_AllowBalance & 1) == 1);

    // Allow balance inner node when insert an item?
    //typedef std::false_type AllowInsertBalance;
    static constexpr bool AllowInsertBalance = ((_AllowBalance & 1) == 1);

    // Allow tree balance tree struct in lookup, insert and erase.
    //typedef std::true_type AllowBalance;
    static constexpr bool AllowBalance = ((_AllowBalance & 1) == 1);

    static_assert((AllowRWBalance | AllowInsertBalance) == AllowBalance);

    // Allow data node slot size dynamic? (static data node slot size is MIN_DATA_NODE_SLOT_SIZE)
    // If data node slot size is dynamic(lazy update), it must AllowRWBalance.
    //typedef std::false_type AllowDynamicDataNode;
    static constexpr bool AllowDynamicDataNode = false;

    static_assert((AllowRWBalance | (!AllowDynamicDataNode)) == true);

    static constexpr bool AllowSplitBalance = ((_AllowBalance & 2) == 2);

    static constexpr bool AllowMergeNode = true;
    
    static constexpr int ERROR_BOUND = _ERROR_BOUND; 

    static constexpr int DATA_NODE_ERROR_BOUND = 4;

    static constexpr slot_type MIN_INNER_NODE_SLOT_SIZE = AEX_MAX(8, 256 / (sizeof(key_type) + sizeof(void*)));

    static constexpr slot_type LEFT_BUFFER_SIZE = ERROR_BOUND;
    static constexpr slot_type RIGHT_BUFFER_SIZE = ERROR_BOUND;
    static constexpr slot_type EXTERN_BUFFER_SIZE = LEFT_BUFFER_SIZE + RIGHT_BUFFER_SIZE;

    //static constexpr slot_type MIN_INNER_NODE_SLOT_SIZE = 32;
    //static constexpr slot_type MIN_INNER_NODE_SLOT_SIZE = 8;

    static constexpr slot_type MIN_DATA_NODE_SLOT_SIZE = AEX_MAX(8, 128 / (sizeof(key_type)));
    //static constexpr slot_type MIN_DATA_NODE_SLOT_SIZE = 8;

    //static constexpr slot_type MAX_DENSE_INNER_NODE_SLOT_SIZE = 128;

    //static constexpr slot_type MIN_ML_INNER_NODE_SLOT_SIZE = 64;

    static constexpr slot_type MIN_ML_INNER_NODE_SIZE = 64;
    //static constexpr slot_type MIN_ML_INNER_NODE_SIZE = MIN_INNER_NODE_SLOT_SIZE;

    static constexpr slot_type MAX_INNER_NODE_SLOT_SIZE = 1 << 25;

    static constexpr slot_type MAX_DATA_NODE_SLOT_SIZE = 1 << 20;

    static constexpr slot_type MIN_ML_DATA_NODE_SLOT_SIZE = 32;
    
    static constexpr float DATA_NODE_FEW_RATIO = 0.5;
        
    static constexpr float DATA_NODE_FULL_RATIO = 1;

    static constexpr float DENSITY_NARROW_RATIO = 1.0 / (traits::MIN_INNER_NODE_SLOT_SIZE / 2);

    static constexpr float EXPAND_RATIO = 2;

    static constexpr float MERGE_COST_PARA = 1;

    static constexpr float SPLIT_COST_PARA = 1;

    static constexpr int BINEARY_SEARCH_SIZE = 32;

    static constexpr int NODE_MUTEX_SLOT_SIZE = 64;

    static constexpr int MAX_DEPTH = 16;

    static constexpr float MAX_ALLOW_ERROR = 0.5 / log(2);

    static constexpr bool debug = true;

    static constexpr int MAX_MODEL_ARGS = _MAX_MODEL_ARGS;

    static constexpr int MAX_SEGMENT_NUM = _MAX_MODEL_ARGS;

    static constexpr unsigned long long INNER_NODE_MAX_DIFFERENT_VALUE = 0x10000000000000ULL;

    static constexpr size_t BINSEARCH_THRESHOLD = 256;

    static constexpr double FORGET_RATE = 1 - 0.0000000001;

    static constexpr double RETRAIN_RATIO = 0.5;
};


}