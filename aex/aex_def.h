#pragma once
namespace aex{

template<typename _Key, typename _Val, typename traits> class aex_tree;
template<typename _Key, typename _Val, typename traits> class aex_allocator;
template<typename _Key, typename _Val, typename traits> struct aex_node_base;
template<typename _Key, typename _Val, typename traits> struct aex_inner_node;
template<typename _Key, typename _Val, typename traits> struct aex_hash_node;
template<typename _Key, typename _Val, typename traits> struct aex_dense_node;
template<typename _Key, typename _Val, typename traits> struct aex_static_data_node;
//template<typename _Key, typename _Val, typename traits> struct aex_hash_data_node;
//template<typename _Key, typename _Val, typename traits> struct aex_test_node;

//template<typename traits, bool _> struct aex_spinlock;
//template<typename traits, bool _> struct aex_rw_lock;
//template<typename traits, bool _> struct OptLock;


template<typename traits> class  aex_EpochBasedMemoryReclamationStrategy;
template<typename traits> class  aex_EpochGuard;
template<typename traits> struct MemoryReclaimUnit;

template<typename _Key, typename traits> struct aex_hash_table_block;
template<typename _Key, typename traits> class aex_hash_table;
template<typename _Key, typename traits> struct aex_hash_table_block_con;
template<typename _Key, typename traits> class aex_hash_table_con;

template<typename _Tp, bool _> struct aex_node_spinlock;

template<typename _Key, 
        typename _Val,
        bool _AllowMultiKey,
        typename _SearchClass,
        bool _AllowConcurrency,
        int _LOG_ERROR_BOUND,
        int _MAX_MODEL_ARGS,
        bool _AllowBalance>
struct aex_default_traits;

template<typename _Tp, typename traits> class linear_model;
template<typename _Tp, typename traits> class gap_array_linear_model;
template<typename _Tp, typename traits> class gap_array_linear_model_hash_table;
template<typename _Tp, typename traits> class piecewise_linear_model;
template<typename _Tp, typename traits> class piecewise_linear_model_2;
template<typename _Tp, typename traits> class piecewise_linear_model_3;
template<typename _Tp, typename traits> class PDM;
template<typename _Tp, typename traits> class PDM_hash_table;
template<typename _Tp, typename Model, typename traits> class PDM_AVX;
template<typename _Tp, typename Model, typename traits> class PDM_hash_table_AVX;

template<typename _Key, typename _Val, typename traits> class aex_iterator;
template<typename _Key, typename _Val, typename traits> class aex_const_iterator;
template<typename _Key, typename _Val, typename traits> class aex_reverse_iterator;
template<typename _Key, typename _Val, typename traits> class aex_const_reverse_iterator;

template<typename _Key, typename _Val, typename traits> struct aex_hash_node_con;
template<typename _Key, typename _Val, typename traits> struct aex_data_node_con;
template<typename _Key, typename _Val, typename traits> struct aex_hash_node_copy;

template<typename traits> struct aex_default_components;

template<typename traits> struct _LockArrayParams;
template<typename traits> struct _GetChildsParams;
template<typename traits> struct _ConstructSMOParams;
template<typename traits> struct _HashTableRescaleParams;

}