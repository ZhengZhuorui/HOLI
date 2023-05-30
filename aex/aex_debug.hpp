#pragma once

#include "aex/aex.h"

namespace aex{

template<typename _Key, typename _Val, typename traits>
std::pair<typename aex_tree<_Key, _Val, traits>::key_type, bool> aex_tree<_Key, _Val, traits>::_debug(node_ptr node, aex_stats &msg){
    bool flag = true;
    key_type first_key;
    if (node->prop & node_property::LEAF){
        ++msg.data_node;
        data_node_ptr dn = static_cast<data_node_ptr>(node);
        first_key = dn->key[0];
        for (size_type i = 0; i < dn->size; ++i){
            if (i > 0 && dn->key[i] < dn->key[i - 1]){
                AEX_DEBUG_PRINT("Error! node[" << i-1 << "]=" << dn->key[i - 1] << " node[" << i << "]=" << dn->key[i]);
                flag = false;
            }
        }
    }
    else{
        ++msg.inner_node;
        inner_node_ptr in = static_cast<inner_node_ptr>(node);
        key_type* node_key = in->key_ptr;
        node_ptr* node_child = in->child_ptr;
        if (node->prop & node_property::ML_NODE){
            size_type cnt = 0;
            bitmap bm = in->bitmap_ptr;
            size_type last = in->last();
            first_key = node_key[0];
            for (size_type i = 0; i <= last; ++i){
                // check if the key is larger than prev position key
                if (i > 0 && node_key[i] < node_key[i - 1]){
                    AEX_DEBUG_PRINT("Error! node[" << i - 1 << "]=" << node_key[i - 1] << " node[" << i << "]=" << node_key[i]);
                    flag = false;
                }
                if (bitmap_impl::at(bm, i)){
                    ++cnt;
                    auto res = _debug(node_child[i]);
                    flag &= res.second;
                    // check the child last key is equal to the node key
                    //if (node_key[i] != res.first){
                    //    AEX_DEBUG_PRINT("Error! key=" << node_key[i] << " son node last key=" << res.first << " node=" << node << "son=" << node_child[i]);
                    //    flag = false;
                    //}
                    // check if the key position is smaller than predict position
                    size_type pos = in->predict(node_key[i]);
                    if (i < pos || i - pos >= traits::ERROR_BOUND){
                        AEX_DEBUG_PRINT("pos=" << i << " predict=" << pos);
                        flag = false;
                    }
                }
                
            }
        }
        else{
            first_key = node_key[0];
            for (size_type i = 0; i < in->last(); ++i){
                // check if the key is larger than prev position key 
                if (i > 0 && node_key[i] < node_key[i - 1]){
                    AEX_DEBUG_PRINT("Error! node[" << i - 1 << "]=" << node_key[i - 1] << " node[" << i << "]=" << node_key[i]);
                    flag = false;
                }
                
                auto res = _debug(node_child[i]);
                if (node_key[i] != res.first){
                    flag = false;
                    AEX_DEBUG_PRINT("Error! key=" << node_key[i] << " son node last key=" << res.first << " node=" << node << "son=" << node_child[i]);
                }
                flag &= res.second; 
                //AEX_ASSERT(i < node->predict(node_key[i]));
            }
        }
    }
    return std::make_pair(first_key, flag);
}

template<typename _Key, typename _Val, typename traits>
bool aex_tree<_Key, _Val, traits>::debug_error(){
    AEX_DEBUG_FORMAT("size=%lld, root->size=%lld", this->m_stats.size, root->size);
    aex_stats debug_stats;
    std::pair<key_type, bool> res = (this->root == nullptr)? std::make_pair(0LL, true) : _debug(this->root, debug_stats);
    if (debug_stats.inner_node != this->m_stats.inner_node){
        AEX_DEBUG_PRINT("Inner node num error!, debug.inner_node=" << debug_stats.inner_node << "tree.inner_node=" << this->m_stats.inner_node);
    }
    if (debug_stats.data_node != this->m_stats.data_node){
        AEX_DEBUG_PRINT("Data node num error!, debug.data_node=" << debug_stats.data_node << "tree.data_node=" << this->m_stats.data_node);
    }
    if (this->node_allocator.inner_node_nums != this->m_stats.inner_node){
        AEX_DEBUG_PRINT("Inner node no free!, allocator.inner_node=" << this->node_allocator.inner_node_nums << "tree.inner_node=" << this->m_stats.inner_node);
    }
    if (this->node_allocator.data_node_nums != this->m_stats.data_node){
        AEX_DEBUG_PRINT("Data node no free!, allocator.data_node=" << this->node_allocator.data_node_nums << "tree.data_node=" << this->m_stats.data_node);
    }
    size_type cnt = 0;
    bool flag = res.second;
    key_type prev_key;
    for (iterator it = begin(); it != end(); ++it){
        if (cnt > 0){
            /* check item is ordered */
            if (it.key() < prev_key){
                flag = false;
            }
        }
        else prev_key = it.key();
        ++cnt;
    }
    /* check the data item is correct */
    if (cnt != this->m_stats.size){
        flag = false;
        AEX_DEBUG_PRINT("Error!");
    }
    return flag;
}

}