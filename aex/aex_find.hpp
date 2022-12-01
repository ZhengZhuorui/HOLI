#pragma once
#include "aex/aex.h"

namespace aex{

template<typename _Key, typename _Val, typename traits>
typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::find_head_leaf(node_ptr node) const{
    ++m_stats.read_times;
    while (!(node->prop & LEAF))
        node = static_cast<inner_node_ptr>(node)->child_ptr[static_cast<inner_node_ptr>(node)->first()];
    return node;
}

template<typename _Key, typename _Val, typename traits>
typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::find_tail_leaf(node_ptr node) const {
    ++m_stats.read_times;
    while (!(node->prop & LEAF))
        node = static_cast<inner_node_ptr>(node)->child_ptr[static_cast<inner_node_ptr>(node)->last()];
    return node;
}

// if no item greater than or equal x, return node->slot_size (ml node) or node->size(otherwise)
template<typename _Key, typename _Val, typename traits>
typename aex_tree<_Key, _Val, traits>::size_type aex_tree<_Key, _Val, traits>::find_lower_pos(inner_node_ptr node, const key_type &x, std::true_type tp){
    ++m_stats.read_times;
    if (check_merge(node)){
        data_node_ptr merged_node = merge(node);
        return find_lower_pos(static_cast<data_node_ptr>(merged_node), x);
    }
    AEX_PRINT("BEGIN");
    key_type* key = node->key_ptr;
    if (node->prop & ML_NODE){
        size_type pos = node->predict(x), upper_bound = std::min(pos + traits::ERROR_BOUND + 1, node->slot_size);
        for (size_type i = pos; i < upper_bound; ++i)
        if (key[i] >= x){
            return i;
        }
        return node->slot_size;
    }
    else{
        size_type ret = std::lower_bound(key, key + size, x) - key;
        return ret;
    }
}

template<typename _Key, typename _Val, typename traits>
typename aex_tree<_Key, _Val, traits>::size_type aex_tree<_Key, _Val, traits>::find_lower_pos(const inner_node_ptr node, const key_type &x, std::false_type tp) const {
    AEX_PRINT("BEGIN");
    key_type* key = node->key_ptr;
    if (node->prop & ML_NODE){
        size_type pos = node->predict(x), upper_bound = std::min(pos + traits::ERROR_BOUND + 1, node->slot_size);
        for (size_type i = pos; i < upper_bound; ++i)
        if (key[i] >= x){
            return i;
        }
        return node->slot_size;
    }
    else{
        size_type ret = std::lower_bound(key, key + size, x) - key;
        return ret;
    }
}

// if no item greater than or equal x, return node->size
template<typename _Key, typename _Val, typename traits>
typename aex_tree<_Key, _Val, traits>::size_type aex_tree<_Key, _Val, traits>::find_lower_pos(const data_node_ptr node, const key_type &x) const {
    ++m_stats.read_times;
    if (check_merge(node))
        merge(node);
    size_type pos;
    if (node->prop & ML_NODE){
        if (node->prop & COMPLEX_MODEL){
            pos = node->model.complex_model.predict(x);
            pos = exponential_search_lower_bound(node->key, node->key + node->size, pos, x) - node->key;
        }
        else{
            pos = node->model.easy_model.predict(x);
            pos = exponential_search_lower_bound(node->key, node->key + node->size, pos, x) - node->key;
        }
    }
    else{
        pos = std::lower_bound(node->key, node->key + size, x) - node->key;
        if (pos == size) pos = node->size;
    }
    return pos;
    
}

// if no item greater than x, return node->slot_size (ml node) or node->size(otherwise)
template<typename _Key, typename _Val, typename traits>
typename aex_tree<_Key, _Val, traits>::size_type aex_tree<_Key, _Val, traits>::find_upper_pos(inner_node_ptr node, const key_type &x, std::true_type tp) {
    ++m_stats.read_times;
    if (check_merge(node)){
        data_node_ptr merged_node = merge(node);
        return find_upper_pos(merged_node);
    }
    key_type* key = node->key_ptr;
    if (node->prop & ML_NODE){
        size_type pos = node->predict(x), upper_bound = std::min(pos + traits::ERROR_BOUND + 1, node->slot_size);
        for (size_type i = pos; i < upper_bound; ++i)
        if (key[i] > x)
            return i;
        return node->slot_size;
    }
    else{
        size_type ret = std::upper_bound(key, key + node->size, x) - key;
        return ret;
    }
}

// if no item greater than x, return node->size
template<typename _Key, typename _Val, typename traits>
typename aex_tree<_Key, _Val, traits>::size_type aex_tree<_Key, _Val, traits>::find_upper_pos(const data_node_ptr node, const key_type &x) {
    AEX_PRINT("BEGIN");
    size_type pos;
    if (node->prop & ML_NODE){
        if (node->prop & COMPLEX_MODEL){
            pos = node->model.complex_model.predict(x);
            pos = exponential_search_upper_bound(node->key, node->key + node->size, pos, x) - node->key;
        }
        else{
            pos = node->model.easy_model.predict(x);
            pos = exponential_search_upper_bound(node->key, node->key + node->size, pos, x) - node->key;
        }
    }
    else{
        pos = std::upper_bound(node->key, node->key + size, k) - node->key;
        if (pos == size) pos = node->size;
    }
}

// if no item greater than or equal x, return NULL
template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::find_lower(inner_node_ptr node, const key_type &x, std::true_type tp) {
    node_ptr* child = node->child_ptr;
    size_type pos = find_lower_pos(node, x, tp);
    AEX_PRINT("pos=" << pos);
    AEX_PRINT("child[pos]=" << child[pos]);
    return (pos == node->slot_size) ? nullptr : child[pos];
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::find_lower(const inner_node_ptr node, const key_type &x, std::false_type fp) const{
    node_ptr* child = node->child_ptr;
    size_type pos = find_lower_pos(node, x, fp);
    AEX_PRINT("pos=" << pos);
    AEX_PRINT("child[pos]=" << child[pos]);
    return (pos == node->slot_size) ? nullptr : child[pos];
}

// if no item greater than or equal x, return end()
template<typename _Key, typename _Val, typename traits>
typename aex_tree<_Key, _Val, traits>::iterator aex_tree<_Key, _Val, traits>::find_lower(const data_node_ptr node, const key_type &key) {
    size_type pos = find_lower_pos(node, key);
    if (pos == node->slot_size)
        return end();
    return iterator(node, pos);
}

// find the lowest item greater than or equal x, if no, return end()
template<typename _Key, typename _Val, typename traits>
typename aex_tree<_Key, _Val, traits>::iterator aex_tree<_Key, _Val, traits>::find_lower(const key_type &key, std::true_type tp){
    node_ptr node = root;
    std::true_type tp;
    while (!(node->prop & LEAF)){
        node = find_lower(static_cast<inner_node_ptr>(node), key, tp);
    }
    return find_lower(static_cast<data_node_ptr>(node), key);
}


template<typename _Key, typename _Val, typename traits>
typename aex_tree<_Key, _Val, traits>::const_iterator aex_tree<_Key, _Val, traits>::find_lower(const key_type &key, std::false_type fp) const {
    node_ptr node = root;
    std::false_type fp;
    while (!(node->prop & LEAF)){
        node = find_lower(static_cast<inner_node_ptr>(node), key, fp);
    }
    return find_lower(static_cast<data_node_ptr>(node), key);
}

// find the lowest item greater than x, if no, return end()
template<typename _Key, typename _Val, typename traits>
typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::find_upper(inner_node_ptr node, const key_type &x, std::true_type tp) {
    node_ptr* child = node->child_ptr;
    size_type pos = find_upper_pos(node, x, tp);
    return (pos == node->slot_size) ? nullptr : child[pos];
}

template<typename _Key, typename _Val, typename traits>
typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::find_upper(const inner_node_ptr node, const key_type &x, std::false_type tp) const{
    node_ptr* child = node->child_ptr;
    size_type pos = find_upper_pos(node, x, fp);
    return (pos == node->slot_size) ? nullptr : child[pos];
}

template<typename _Key, typename _Val, typename traits>
typename aex_tree<_Key, _Val, traits>::iterator aex_tree<_Key, _Val, traits>::find_upper(const data_node_ptr node, const key_type &x) {
    for (size_type i = 0; i < node->size; ++i)
    if (node->key[i] > x)
        return iterator(node, i);
    return end();
}

template<typename _Key, typename _Val, typename traits>
typename aex_tree<_Key, _Val, traits>::iterator aex_tree<_Key, _Val, traits>::find_upper(const key_type &key, std::true_type tp) {
    node_ptr node = root;
    while (!(node & LEAF)){
        node = find_upper(static_cast<inner_node_ptr>(node), key, tp);
    }
    return find_upper(static_cast<data_node_ptr>(node), key, tp);
}

template<typename _Key, typename _Val, typename traits>
typename aex_tree<_Key, _Val, traits>::const_iterator aex_tree<_Key, _Val, traits>::find_upper(const key_type &key, std::false_type fp) const {
    node_ptr node = root;
    while (!(node & LEAF)){
        node = find_upper(static_cast<inner_node_ptr>(node), key, fp);
    }
    return find_upper(static_cast<data_node_ptr>(node), key, fp);
}

}