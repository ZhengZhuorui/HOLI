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

template<int K>
constexpr int TEMPLATE_LOG(){
    return TEMPLATE_LOG<(K >> 1)>() + 1;
}

template<>
constexpr int TEMPLATE_LOG<1>(){
    return 0;
}

template<bool _AllowConcurrency>
struct aex_concurrency_traits{
    
    
};

template<>
struct aex_concurrency_traits<true>{
    
};

template<typename _Key, 
        typename _Val,
        bool _AllowMultiKey=false,
        typename _SearchClass=void,
        bool _AllowConcurrency=false,
        int _LOG_ERROR_BOUND=4,
        int _MAX_MODEL_ARGS=8,
        bool _AllowBalance=false>
struct aex_default_traits{
    typedef _Key                     key_type;
    typedef _Val                     value_type;
    typedef _SearchClass             SearchClass;
    typedef long long                slot_type;
    typedef unsigned long long       bitmap_base;
    typedef bitmap_base*             bitmap;
    typedef unsigned long long       hash_type;

    typedef aex_default_balance_args MODEL_ARGS;

    static constexpr bool AllowMultiKey    = _AllowMultiKey;
    static constexpr bool AllowConcurrency = _AllowConcurrency;
    typedef aex_concurrency_traits<AllowConcurrency> con_traits;
    // Allow balance inner node and data node when read and write frequency update?
    //typedef std::true_type AllowRWBalance;
    static constexpr bool AllowRWBalance = ((_AllowBalance & 1) == 1);

    // Allow balance inner node when insert an item?
    //typedef std::false_type AllowInsertBalance;
    static constexpr bool AllowInsertBalance = ((_AllowBalance & 1) == 1);

    // Allow tree balance tree struct in lookup, insert and erase.
    //typedef std::true_type AllowBalance;
    static constexpr bool AllowBalance = ((_AllowBalance & 1) == 1);

    static constexpr bool AllowExtend = false;
    static constexpr bool AllowRebuild = false;
    static constexpr bool AllowErase = false;

    static constexpr bool AllowMemoryPool = false;

    static constexpr bool DebugMode = true;

    static_assert((AllowRWBalance | AllowInsertBalance) == AllowBalance);

    // Allow data node slot size dynamic? (static data node slot size is DATA_NODE_SLOT_SIZE)
    // If data node slot size is dynamic(lazy update), it must AllowRWBalance.
    //typedef std::false_type AllowDynamicDataNode;
    static constexpr bool AllowDynamicDataNode = false;

    static_assert((AllowRWBalance | (!AllowDynamicDataNode)) == true);

    static constexpr bool AllowSplitBalance = ((_AllowBalance & 2) == 2);

    //static constexpr bool AllowMergeNode = _AllowMergeNode;
    
    static constexpr int LOG_ERROR_BOUND = _LOG_ERROR_BOUND; 
    static constexpr int ERROR_BOUND = 1 << LOG_ERROR_BOUND; 

    static constexpr slot_type MAX_INNER_NODE_SLOT_SIZE = 1LL << 56;

    static constexpr int MAX_DENSE_NODE_SLOT_SIZE = 32;
    static constexpr int MIN_DENSE_NODE_SLOT_SIZE = 16;
    //static constexpr int MAX_DENSE_NODE_SLOT_SIZE = 16;
    //static constexpr int MIN_DENSE_NODE_SLOT_SIZE = 8;
    static constexpr int DENSE_NODE_SLOT_SIZE = MIN_DENSE_NODE_SLOT_SIZE;

    static constexpr slot_type SLOT_PER_LOCK = 64;
    static constexpr slot_type LOG_SLOT_PER_LOCK = TEMPLATE_LOG<SLOT_PER_LOCK>();
    static constexpr slot_type SLOT_PER_SHORTCUT = 512;
    static constexpr slot_type LOG_SLOT_PER_SHORTCUT = TEMPLATE_LOG<SLOT_PER_SHORTCUT>();
    static constexpr slot_type MIN_HASH_NODE_CNT      = 8;
    static constexpr slot_type MIN_HASH_NODE_SLOT_SIZE = 128;
    //static constexpr slot_type MAX_HASH_NODE_SIZE      = MAX_INNER_NODE_SLOT_SIZE;
    static constexpr slot_type MAX_HASH_NODE_SLOT_SIZE = MAX_INNER_NODE_SLOT_SIZE;
    static constexpr slot_type HASH_NODE_MAX_GAP_SLOT = 8;
    static constexpr slot_type HASH_NODE_MAX_GAP = HASH_NODE_MAX_GAP_SLOT * SLOT_PER_SHORTCUT;
    
    static constexpr hash_type K1 = 999999937;
    static constexpr hash_type K2 = 1099999997;
    static constexpr hash_type K3 = 699999953;
    static constexpr hash_type K4 = 799999999;
    static constexpr hash_type K5 = 899999963;
    static constexpr hash_type K6 = 1000000000000000003;
    static constexpr hash_type K7 = 576460752303423433;
    static constexpr hash_type MAX_INT = (1LL << 32) - 1;

    static constexpr int HASH_TABLE_BLOCK_SIZE = (!AllowConcurrency) ? 8 : 1;
    //static constexpr int HASH_TABLE_BLOCK_SIZE = 1;

    static constexpr float DATA_NODE_FEW_RATIO       = 0.5;
    static constexpr float DATA_NODE_FULL_RATIO      = 1;
    static constexpr int   LOG_DATA_NODE_FEW_RATIO   = 1;
    static constexpr int   LOG_DATA_NODE_FULL_RATIO  = 0;
    static constexpr int   MIN_DATA_NODE_SLOT_SIZE   = (!AllowConcurrency) ? 16 : 16;
    static constexpr int   DATA_NODE_SLOT_SIZE       = MIN_DATA_NODE_SLOT_SIZE;
    static constexpr int   LARGE_HASH_NODE_SLOT_SIZE = 65536;
    static constexpr int   SIZE_ARRAY_SIZE           = 32;
    static constexpr float INIT_DATA_NODE_DENSITY    = 1.0;
    static constexpr float INIT_DATA_NODE_DENSITY_CON= 0.8;

    static constexpr float HASH_NODE_FULL_RATIO      = 1.0 / 8;
    //static constexpr float HASH_NODE_FEW_RATIO       = 1.0 / 32;
    //static constexpr float HASH_NODE_FEW_RATIO       = (!AllowConcurrency) ? 1.0 / 32 : 1.0 / 64;
    static constexpr float HASH_NODE_FEW_RATIO       = 1.0 / 64;
    //static constexpr float HASH_NODE_FEW_RATIO       = 1.0 / 16;
    static constexpr int   LOG_HASH_NODE_FULL_RATIO  = 4;
    static constexpr int   LOG_HASH_NODE_FEW_RATIO   = 6;
    static constexpr float DENSE_NODE_FULL_RATIO     = DATA_NODE_FULL_RATIO;
    static constexpr float DENSE_NODE_FEW_RATIO      = DATA_NODE_FEW_RATIO;

    //static constexpr float HASH_TABLE_FEW_RATIO             = 0.375;
    static constexpr float HASH_TABLE_FEW_RATIO             = 0.1875;
    static constexpr float HASH_TABLE_FULL_RATIO            = 0.75;
    static constexpr unsigned long long MIN_HASH_TABLE_SIZE = 16;


    static constexpr float MIN_REBUILD_RATIO = 1.0;
    static constexpr int MAX_DEPTH = 15;

    static constexpr int SIZE_BLOCK_CNT = 64;
    static constexpr int MIN_ADD_CNT = 4;

    static constexpr int MEMORY_POOL = 1024;

    static constexpr int THREAD_UNIT_SIZE = 8192;

    // ========== old version ==========
    static constexpr slot_type MIN_INNER_NODE_SLOT_SIZE = AEX_MAX(8, 256 / (sizeof(key_type) + sizeof(void*)));
    static constexpr int DATA_NODE_ERROR_BOUND = 4;
    static constexpr slot_type LOG_INNER_NODE_SLOT_SIZE = TEMPLATE_LOG<MIN_INNER_NODE_SLOT_SIZE>();
    static constexpr slot_type MAX_MODEL_DP_SEGMENT_SIZE = 1 << 14;
    static constexpr slot_type MAX_DATA_NODE_SLOT_SIZE = 1 << 20;
    static constexpr slot_type MIN_ML_DATA_NODE_SLOT_SIZE = 32;
    static constexpr float DENSITY_NARROW_RATIO = 1.0 / (MIN_INNER_NODE_SLOT_SIZE / 4);
    static constexpr int LOG_DENSITY_NARROW_RATIO = LOG_INNER_NODE_SLOT_SIZE - 2;
    //static_assert(std::abs((1 << LOG_DENSITY_NARROW_RATIO) * DENSITY_NARROW_RATIO - 1) < 1e-5);
    static constexpr float EXPAND_RATIO = 2;
    static constexpr float MERGE_COST_PARA = 1;
    static constexpr float SPLIT_COST_PARA = 1;
    static constexpr int BINEARY_SEARCH_SIZE = 32;
    static constexpr int NODE_MUTEX_SLOT_SIZE = 64;
    static constexpr float MAX_ALLOW_ERROR = 0.5 / log(2);
    static constexpr int MAX_SEGMENT_NUM = _MAX_MODEL_ARGS;
    static constexpr unsigned long long INNER_NODE_MAX_DIFFERENT_VALUE = 0x10000000000000ULL;
    static constexpr ULL BINSEARCH_THRESHOLD = 256;
    static constexpr double FORGET_RATE = 1 - 0.0000000001;
    static constexpr double RETRAIN_RATIO = 0.5;
    //static constexpr int LOG_HASH_TABLE_RATIO = 2 + LOG_ERROR_BOUND;
    static constexpr int LOG_HASH_TABLE_RATIO = 1 + LOG_ERROR_BOUND;
};


}