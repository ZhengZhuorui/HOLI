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
        typename _AllowMultiKey,
        typename _AllowMultiThread>
struct aex_traits{

    typedef _Key key_type;

    typedef _Val value_type;

    typedef size_t size_type;

    //typedef unsigned long long pos_type;

    typedef _used_as_set used_as_set;

    //static const bool AllowMultiKey = _AllowMultiKey;
    typedef _AllowMultiKey AllowMultiKey;

    typedef _AllowMultiThread AllowMultiThread;
    
    static const int ERROR_BOUND = 8;

    static const int DATA_NODE_SLOT_SIZE_BIT = 3;

    static const int DATA_NODE_SLOT_SIZE = (1 << DATA_NODE_SLOT_SIZE_BIT);

    static const int MIN_INNER_NODE_SLOT_SIZE = 8;
    
    static const int MIN_ML_NODE_SLOT_SIZE = 16;
    
    static const int MIN_COMPLEX_ML_DATA_NODE_SLOT_SIZE = 4096;

    static constexpr float DATA_NODE_FEW_RATIO = 0.5;
        
    static constexpr float DATA_NODE_FULL_RATIO = 1;
    
    static constexpr float INNER_NODE_FEW_RATIO = 0.5;
    
    static constexpr float INNER_NODE_FULL_RATIO = 0.9;
    
    static const int EXPAND_RATIO = 2;

    static constexpr float INNER_NODE_NARROW_RATIO = 0.5;

    //static const int MAX_LEVEL = 16;

    static const int CACHE_LINE_SIZE = DATA_NODE_SLOT_SIZE;

    static const bool debug = true;
    
};

}