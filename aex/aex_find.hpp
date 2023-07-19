#pragma once
#include "aex/aex.h"

namespace aex{

// find the lowest item greater than or equal x, if no, return end()
template<typename _Key, typename _Val, typename traits>
typename aex_tree<_Key, _Val, traits>::data_node_ptr aex_tree<_Key, _Val, traits>::find_leaf(const key_type &key, std::true_type AllowBalance){
    //mutex timestamp
    ++this->m_stats.timestamp;
    node_ptr node = nullptr, parent = nullptr;
    bool flag;
    //node_ptr stack[traits::MAX_DEPTH];
    //int top = 1;
    do{
        flag = true;
        node = root;
        while (!(node->prop & node_property::LEAF)){
            check_balance_merge(node, parent);
            if (check_balance_merge(node)){
                flag = false;
                balance_merge_subtree(node);
                parent = nullptr;
                break;
            }
            else {
                parent = node;
                node = find(static_cast<inner_node_ptr>(node), key, AllowBalance);
            }
        }
    }while(flag == false);
    return static_cast<data_node_ptr>(node);
    //return iterator(node, key);
}

template<typename _Key, typename _Val, typename traits>
typename aex_tree<_Key, _Val, traits>::data_node_ptr aex_tree<_Key, _Val, traits>::find_leaf(const key_type &key, std::false_type AllowBalance){
    //mutex timestamp
    ++this->m_stats.timestamp;
    node_ptr node = root;
    while (!(node->prop & node_property::LEAF)){
        node = find(static_cast<inner_node_ptr>(node), key);
    }
    return static_cast<data_node_ptr>(node);
}

//template<typename _Key, typename _Val, typename traits>
//typename aex_tree<_Key, _Val, traits>::data_node_ptr aex_tree<_Key, _Val, traits>::find_leaf_with_trace(const key_type &key, node_ptr* stack, int &top){
//    //mutex timestamp
//    ++this->m_stats.timestamp;
//    node_ptr node = root;
//    stack[top = 0] = nullptr;
//    top++;
//    while (!(node->prop & node_property::LEAF)){
//        stack[top++] = node;
//        node = find(static_cast<inner_node_ptr>(node), key);
//    }
//    stack[top++] = node;
//    return static_cast<data_node_ptr>(node);
//}


}