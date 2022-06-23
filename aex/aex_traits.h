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
        typename _AllowMultiKey=std::__false_type,
        typename _Model=linear_model<_Key> >
struct aex_traits{

    typedef _Key key_type;

    typedef _Val value_type;

    typedef size_t size_type;

    typedef _AllowMultiKey AllowMultiKey;

    typedef _used_as_set used_as_set;
    
    typedef _Model Model;

    typedef u_int64_t* bitmap;
    
    static const int ERROR_BOUND = 8;

    static const int DATA_NODE_SLOT_SIZE = 8;

    static const int MIN_INNER_NODE_SLOT_SIZE = 8;
    
    static const int MIN_ML_INNER_NODE_SLOT_SIZE = 16;

    static const float DATA_NODE_FEW_RATIO = 0.5;
        
    static const float DATA_NODE_FULL_RATIO = 1;
    
    static const float INNER_NODE_FEW_RATIO = 0.25;
    
    static const float INNER_NODE_FULL_RATIO = 0.5;
    
    static const int INNER_NODE_EXPAND_RATIO = 2;

    static const float INNER_NODE_NARROW_RATIO = 0.5;

    static const int MAX_LEVEL = 16;
    
};

}