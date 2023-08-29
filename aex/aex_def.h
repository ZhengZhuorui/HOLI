#pragma once
namespace aex{

template<typename _Key, typename _Val, typename traits> class aex_tree;

template<typename _Key, typename _Val, typename traits> class aex_node_allocator;

template<typename _Key, typename _Val, typename traits> struct aex_inner_node;
template<typename _Key, typename _Val, typename traits> struct aex_data_node;
template<typename _Key, typename _Val, typename traits> struct aex_static_data_node;

template<typename _Tp> struct aex_node_balance_stats;

template<typename _Tp> struct aex_tree_balance_stats;

template<typename _Key, typename _Val, typename _AllowMultiThread> struct aex_default_traits;

template<typename _Tp, typename traits> class linear_model;
template<typename _Tp, typename traits> class gap_array_linear_model;
template<typename _Tp, typename traits> class piecewise_linear_model;

template<typename _Key, typename _Val, typename traits> class aex_iterator;
template<typename _Key, typename _Val, typename traits> class aex_const_iterator;
template<typename _Key, typename _Val, typename traits> class aex_reverse_iterator;
template<typename _Key, typename _Val, typename traits> class aex_const_reverse_iterator;

}