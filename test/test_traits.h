#pragma once
#include "aex/aex_traits.h"

template<typename _Key, 
        typename _Val,
        typename _AllowConcurrency=std::false_type>
struct test_traits{

    typedef _Key key_type;

    typedef _Val value_type;

    typedef unsigned long long size_type;

    typedef int slot_type;

    typedef unsigned long long bitmap_base;

    typedef bitmap_base* bitmap;

    typedef unsigned char version_type;

    //typedef aex::aex_default_balance_args MODEL_ARGS;
    typedef aex::aex_default_balance_args MODEL_ARGS;

    typedef _AllowConcurrency AllowConcurrency;

    // Allow balance inner node and data node when read and write frequency update?
    typedef std::true_type AllowRWBalance;

    // Allow balance inner node when insert an item?
    typedef std::true_type AllowInsertBalance;

    // Allow tree balance tree struct in lookup, insert and erase.
    typedef std::true_type AllowBalance;

    static_assert((AllowRWBalance::value | AllowInsertBalance::value) == AllowBalance::value);

    // Allow data node slot size dynamic? (static data node slot size is MIN_DATA_NODE_SLOT_SIZE)
    // If data node slot size is dynamic(lazy update), it must AllowRWBalance.
    typedef std::false_type AllowDynamicDataNode;

    static_assert((AllowRWBalance::value | (!AllowDynamicDataNode::value)) == true);
    
    static const int ERROR_BOUND = 8;

    static const int DATA_NODE_ERROR_BOUND = 2;

    static const slot_type MIN_INNER_NODE_SLOT_SIZE = 8;

    static const slot_type MIN_ML_INNER_NODE_SLOT_SIZE = 64;

    static const slot_type MIN_ML_INNER_NODE_SIZE = 64;

    static const slot_type MAX_INNER_NODE_SLOT_SIZE = 1 << 25;

    static const slot_type MIN_DATA_NODE_SLOT_SIZE = 16;

    static const slot_type MAX_DATA_NODE_SLOT_SIZE= 1 << 20;

    static const slot_type MIN_ML_DATA_NODE_SLOT_SIZE = 32;
    
    static constexpr float DATA_NODE_FEW_RATIO = 0.5;
        
    static constexpr float DATA_NODE_FULL_RATIO = 1;

    static constexpr float DENSITY_NARROW_RATIO = 0.5;

    static constexpr float MERGE_COST_PARA = 1;

    static constexpr float SPLIT_COST_PARA = 1;

    static const int BINEARY_SEARCH_SIZE = 32;

    static const int NODE_MUTEX_SLOT_SIZE = ERROR_BOUND;

    static const int MAX_DEPTH = 16;

    static const char INIT_REWIRED_CNT = 5;

    static constexpr float LEARNING_COST = 10;

    static const int CACHE_LINE_SIZE = MIN_DATA_NODE_SLOT_SIZE;

    static constexpr float MAX_ALLOW_ERROR = 2.0 / log(2);

    static constexpr float MAX_LINEAR_PROBE_ALLOW_ERROR = 4.0 / log(2);

    static const bool debug = true;

    static const int MAX_SEGMENT_NUM = 8;
};


template<typename _Key, 
        typename _Val,
        typename _AllowConcurrency = std::false_type>
struct test_dynamic_data_node_traits{

    typedef _Key key_type;

    typedef _Val value_type;

    typedef unsigned long long size_type;

    typedef int slot_type;

    typedef unsigned long long bitmap_base;

    typedef bitmap_base* bitmap;

    typedef unsigned char version_type;

    typedef aex::aex_default_balance_args MODEL_ARGS;

    typedef _AllowConcurrency AllowConcurrency;

    // Allow balance inner node and data node when read and write frequency update?
    typedef std::true_type AllowRWBalance;

    // Allow balance inner node when insert an item?
    typedef std::true_type AllowInsertBalance;

    // Allow tree balance tree struct in lookup, insert and erase.
    typedef std::true_type AllowBalance;

    static_assert((AllowRWBalance::value | AllowInsertBalance::value) == AllowBalance::value);

    // Allow data node slot size dynamic? (static data node slot size is MIN_DATA_NODE_SLOT_SIZE)
    // If data node slot size is dynamic(lazy update), it must AllowRWBalance.
    typedef std::false_type AllowDynamicDataNode;

    static_assert((AllowRWBalance::value | (!AllowDynamicDataNode::value)) == true);
    
    static const int ERROR_BOUND = 8;

    static const int DATA_NODE_ERROR_BOUND = 2;

    static const slot_type MIN_INNER_NODE_SLOT_SIZE = 8;

    static const slot_type MIN_ML_INNER_NODE_SLOT_SIZE = 64;

    static const slot_type MIN_ML_INNER_NODE_SIZE = 64;

    static const slot_type MAX_INNER_NODE_SLOT_SIZE = 1 << 25;

    static const slot_type MIN_DATA_NODE_SLOT_SIZE = 16;

    static const slot_type MAX_DATA_NODE_SLOT_SIZE= 1 << 20;

    static const slot_type MIN_ML_DATA_NODE_SLOT_SIZE = 32;
    
    static constexpr float DATA_NODE_FEW_RATIO = 0.5;
        
    static constexpr float DATA_NODE_FULL_RATIO = 1;

    static constexpr float DENSITY_NARROW_RATIO = 0.5;

    static constexpr float MERGE_COST_PARA = 1;

    static constexpr float SPLIT_COST_PARA = 1;

    static const int BINEARY_SEARCH_SIZE = 32;

    static const int NODE_MUTEX_SLOT_SIZE = ERROR_BOUND;

    static const int MAX_DEPTH = 16;

    static const char INIT_REWIRED_CNT = 5;

    static constexpr float LEARNING_COST = 10;

    static const int CACHE_LINE_SIZE = MIN_DATA_NODE_SLOT_SIZE;

    static constexpr float MAX_ALLOW_ERROR = 2.0 / log(2);

    static const bool debug = true;

    static const int MAX_SEGMENT_NUM = 8;
};