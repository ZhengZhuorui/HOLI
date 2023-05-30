#pragma once
#include "aex/aex.h"

namespace aex{

template<typename _Key, typename _Val, typename traits>
typename aex_tree<_Key, _Val, traits>::data_node_ptr aex_tree<_Key, _Val, traits>::find_head_leaf(node_ptr node) const{
    while (!(node->prop & node_property::LEAF))
        node = static_cast<inner_node_ptr>(node)->child_ptr[0];
    return node;
}

template<typename _Key, typename _Val, typename traits>
typename aex_tree<_Key, _Val, traits>::data_node_ptr aex_tree<_Key, _Val, traits>::find_tail_leaf(node_ptr node) const {
    while (!(node->prop & node_property::LEAF))
        node = static_cast<inner_node_ptr>(node)->child_ptr[static_cast<inner_node_ptr>(node)->last()];
    return node;
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::find_lower(const inner_node_ptr node, const key_type &x){
    node_ptr* child = node->child_ptr;
    size_type pos = node->find_lower_pos(x);
    AEX_FORMAT("child[%lld]=%p", pos, child[pos]); 
    return (pos == node->slot_size) ? nullptr : child[pos];
}

// if no item greater than or equal x, return end()
template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::iterator aex_tree<_Key, _Val, traits>::find_lower(const data_node_ptr node, const key_type &x){
    size_type pos = node->find_lower_pos(x);
    if (pos == node->slot_size)
        return end();
    return iterator(node, pos);
}

// find the lowest item greater than or equal x, if no, return end()
template<typename _Key, typename _Val, typename traits>
typename aex_tree<_Key, _Val, traits>::iterator aex_tree<_Key, _Val, traits>::find_lower(const key_type &key, std::true_type AllowBalance){
    //mutex timestamp
    ++this->m_stats.timestamp;
    node_ptr node;
    bool flag;
    //node_ptr stack[traits::MAX_DEPTH];
    //int top = 1;
    do{
        flag = true;
        node = root;
        while (!(node->prop & node_property::LEAF)){
            if (check_balance_merge(node)){
                flag = false;
                balance_merge_subtree(node);
                break;
            }
            else 
                node = find_lower(static_cast<inner_node_ptr>(node), key, AllowBalance);
        }
    }while(flag == false);
    return find_lower(static_cast<data_node_ptr>(node), key);
    //return iterator(node, key);
}

template<typename _Key, typename _Val, typename traits>
typename aex_tree<_Key, _Val, traits>::iterator aex_tree<_Key, _Val, traits>::find_lower(const key_type &key, std::false_type AllowBalance){
    //mutex timestamp
    ++this->m_stats.timestamp;
    node_ptr node = root;
    while (!(node->prop & node_property::LEAF)){
        node = find_lower(static_cast<inner_node_ptr>(node), key);
    }
    iterator iter = find_lower(static_cast<data_node_ptr>(node), key);
    return iter;
}

template<typename _Key, typename _Val, typename traits>
typename aex_tree<_Key, _Val, traits>::iterator aex_tree<_Key, _Val, traits>::find_lower_with_trace(const key_type &key, node_ptr* stack, int &top, std::true_type AllowBalance){
    //mutex timestamp
    ++this->m_stats.timestamp;
    bool flag;
    node_ptr node;
    stack[top = 0] = nullptr;
    do{
        top = 1;
        flag = true;
        node = root;
        while (!(node->prop & node_property::LEAF)){
            stack[top++] = node;
            if (check_balance_merge(node)){
                flag = false;
                balance_merge(node, stack, top);
                break;
            }
            else 
                node = find_lower(static_cast<inner_node_ptr>(node), key, AllowBalance);
        }
    }while(flag == false);
    stack[top++] = node;
    return find_lower(static_cast<data_node_ptr>(node), key);
}

template<typename _Key, typename _Val, typename traits>
typename aex_tree<_Key, _Val, traits>::iterator aex_tree<_Key, _Val, traits>::find_lower_with_trace(const key_type &key, node_ptr* stack, int &top, std::false_type AllowBalance){
    //mutex timestamp
    ++this->m_stats.timestamp;
    node_ptr node = root;
    stack[top = 0] = nullptr;
    top++;
    while (!(node->prop & node_property::LEAF)){
        stack[top++] = node;
        node = find_lower(static_cast<inner_node_ptr>(node), key);
    }
    stack[top++] = node;
    return find_lower(static_cast<data_node_ptr>(node), key);
}

template<typename _Key, typename _Val, typename traits>
typename aex_tree<_Key, _Val, traits>::iterator aex_tree<_Key, _Val, traits>::find_upper(const key_type &key, std::true_type AllowBalance) {
    iterator ret = this->find_lower(key, AllowBalance);
    if (ret.key() == key) ++ret;
    return ret;
}

template<typename _Key, typename _Val, typename traits>
typename aex_tree<_Key, _Val, traits>::iterator aex_tree<_Key, _Val, traits>::find_upper(const key_type &key, std::false_type AllowBalance) {
    iterator ret = this->find_lower(key, AllowBalance);
    if (ret.key() == key) ++ret;
    return ret;
}

}