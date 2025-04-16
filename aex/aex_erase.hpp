#pragma once
#include "aex.h"

namespace aex{

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::_erase(const key_type key){
    node_ptr node, child;
    slot_type pos, next_pos;
    bool ret;
    node = root;
    check_erase_SMO(node);
    
    while (true){
        child = find_erase(i_n(node), key, pos, next_pos);
        AEX_PRINT("child=" << child << ", pos=" << pos << ", next_pos=" << next_pos);
        if (child->type == NodeType::LeafNode){
            ret = l_n(child)->erase(key);
            //if (!isfew(l_n(child)) || pos == 0){
            //    ret = l_n(child)->erase(key);
            //}
            //else{
            if (isfew(l_n(child)) && pos > 0){
                key_type prev_key;
                node_ptr prev_node;
                data_node_ptr prev_child;
                if (node->type == NodeType::HashNode){
                    std::tie(prev_key, prev_node) = hash_table.find(h_n(node), h_n(node)->prev_item_find(pos - 1));
                    prev_child = find_tail_leaf(prev_node);
                }
                else{
                    prev_node = d_n(node)->child_ptr[pos - 1];
                    prev_child = find_tail_leaf(prev_node);
                }

                AEX_ASSERT(prev_child->next == child);
                if (prev_child->size + child->size <= traits::DATA_NODE_SLOT_SIZE){
                    ret = l_n(child)->erase(key);
                    if (node->type == NodeType::HashNode){
                        erase(h_n(node), pos, next_pos, child);
                        if (h_n(node)->tail_node == child) h_n(node)->tail_node = tail_node(h_n(node));
                    }
                    else
                        erase(d_n(node), pos);
                    merge(prev_child, l_n(child));
                }
                //else{
                //    ret = l_n(child)->erase(key);
                //}
            }
            return ret;
        }

        if (child->size == 1){
            AEX_ASSERT(child->type == NodeType::DenseNode);
            if (node->type == NodeType::HashNode){
                key_type child_key;
                node_ptr _;
                std::tie(child_key, _) = hash_table.find(h_n(node), pos);
                AEX_ASSERT(_ == child);
                update(h_n(node), pos, next_pos, child, child_key, d_n(child)->child_ptr[0]);
                if (h_n(node)->tail_node == child) h_n(node)->tail_node = tail_node(h_n(node));
            }
            else{
                d_n(node)->child_ptr[pos] = d_n(child)->child_ptr[0];
            }
            free_node(child);
        }
        check_erase_SMO(child);
        node = child;
    }
    AEX_ASSERT(0 == 1);
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::erase(hash_node_ptr node, const slot_type pos, const slot_type next_pos, const node_ptr old_node){
    AEX_ASSERT(node->is_occupied(pos) == true);
    AEX_ASSERT(check_unlock(node));
    key_type prev_key;
    node_ptr prev_node;
    std::tie(prev_key, prev_node) = hash_table.find(node, node->prev_item_find(pos - 1));
    bitmap_impl::set_zero(node->bitmap_ptr, pos);
    if ((pos & (traits::SLOT_PER_SHORTCUT - 1)) == 0)
        //hash_table.compare_and_swap(node, pos, old_node, prev_key, prev_node);
        hash_table.update(node, pos, prev_key, prev_node);
    else
        hash_table.erase(node, pos);
    for (slot_type j = highbit<slot_type, traits::SLOT_PER_SHORTCUT>(pos + 1); j < next_pos; j += traits::SLOT_PER_SHORTCUT){
        hash_table.update(node, j, prev_key, prev_node);
        //if (!hash_table.update(node, j, prev_key, prev_node))
        //    break;
    }
    --node->size;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::erase(dense_node_ptr node, const slot_type pos){
    AEX_ASSERT(check_lock(node));
    AEX_ASSERT(pos < node->size);
    std::move(node->key_ptr   + pos + 1, node->key_ptr   + node->size, node->key_ptr   + pos);
    std::move(node->child_ptr + pos + 1, node->child_ptr + node->size, node->child_ptr + pos);
    --node->size;
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::check_erase_SMO(node_ptr node){
    AEX_ASSERT(node->type != NodeType::LeafNode);
    if (node->type == NodeType::HashNode){
        if (isfew(h_n(node))){
            narrow(h_n(node));
        }
    }
    return true;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::merge(data_node_ptr left_node, data_node_ptr right_node){
    AEX_ASSERT(check_lock(left_node));
    AEX_ASSERT(check_lock(right_node));
    AEX_ASSERT(left_node != right_node);
    AEX_ASSERT(left_node->next == right_node);
    AEX_ASSERT(left_node->size + right_node->size <= traits::DATA_NODE_SLOT_SIZE);
    #ifdef AEX_DEBUG
    ++this->opt_stats.data_node_merge_cnt;
    #endif
    std::move(right_node->key,  right_node->key  + right_node->size, left_node->key  + left_node->size);
    std::move(right_node->data, right_node->data + right_node->size, left_node->data + left_node->size);
    //if constexpr (std::is_same_v<data_node, aex_hash_data_node<_Key, _Val, traits>>)
    //    std::move(right_node->fp,   right_node->fp + right_node->size, left_node->fp + left_node->size);
    left_node->size += right_node->size;
    left_node->next = right_node->next;
    free_node_helper(right_node);
}

}