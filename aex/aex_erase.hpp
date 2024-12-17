#pragma once
#include "aex/aex.h"

namespace aex{

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::_erase(const key_type &key){
#define ERASE_UNLOCK() erase_unlock(i_n(node), pos, next_pos);
    node_ptr node, child;
    slot_type pos, next_pos;
    version_type now_version;
    bool ret;
    int restart_count = 0;

    SL();
    if (root == nullptr){
        SU();
        return false;
    }
_erase_start:
    if (restart_count++)
        yield(restart_count);
    node = root;
    SL(node);
    AEX_ASSERT(check_lock_shared(root));
    if (!check_erase_SMO(node)){
        SU(node);
        goto _erase_start;
    }
    now_version = this->version;
    
    while (true){
        child = find_update(i_n(node), key, pos, next_pos);
        AEX_PRINT("child=" << child << ", pos=" << pos << ", next_pos=" << next_pos << ", slot_size=" << i_n(node)->slot_size);
        if (child->type == NodeType::LeafNode){
            AEX_PRINT("1");
            if (now_version < l_n(child)->version){
                ERASE_UNLOCK(); SU(child);
                goto _erase_start;
            }
            
            if (!isfew(l_n(child)) || pos == 0){
                AEX_PRINT("2");
                if (!TUL(child)){
                    ERASE_UNLOCK(); SU(child);
                    goto _erase_start;
                }
                ret = l_n(child)->erase(key);
                ERASE_UNLOCK(); XU(child);
            }
            else{
                key_type prev_key;
                node_ptr prev_node;
                if (node->type == NodeType::HashNode){
                    if (!h_n(node)->try_array_upgrade_lock(pos - 1, next_pos)){
                        SU(child); SU(h_n(node), pos - 1, next_pos);
                        goto _erase_start;
                    }
                    std::tie(prev_key, prev_node) = hash_table.find(node, h_n(node)->prev_item_find(pos - 1));
                }
                else{
                    if (!TUL(node)){
                        SU(child); SU(node);
                        goto _erase_start;
                    }
                    prev_node = d_n(node)->child_ptr[pos - 1];
                }
                data_node_ptr prev_child = find_tail_leaf(prev_node);
                if (prev_child->size + child->size - l_n(child)->exists(key) <= traits::DATA_NODE_SLOT_SIZE){
                    if (!TUL(prev_child)){
                        SU(child); XU(i_n(node), pos - 1, next_pos);
                        goto _erase_start;
                    }
                    if (!TUL(child)){
                        SU(child); XU(prev_child); XU(i_n(node), pos - 1, next_pos);
                        goto _erase_start;
                    }
                    ret = l_n(child)->erase(key);
                    merge(prev_child, l_n(child));
                    XU(prev_child);
                }
                else{
                    SU(prev_child);
                    ret = l_n(child)->erase(key);
                    XU(child);
                }

                if (node->type == NodeType::HashNode)
                    erase(h_n(node), pos, next_pos);
                else
                    erase(d_n(node), pos);
                XU(i_n(node), pos - 1, next_pos);
            }
            SU();
            return ret;
        }
        else if (d_n(child)->size == 1){
            AEX_PRINT("7");
            AEX_ASSERT(child->type == NodeType::DenseNode);
            if (!erase_lock(i_n(node), pos, next_pos)){
                SU(child);
                ERASE_UNLOCK();
                goto _erase_start;
            }
            if (!TUL(child)){
                SU(child);
                if (node->type == NodeType::HashNode)
                    XU(h_n(node), pos - 1, next_pos);
                else
                    XU(node);
                goto _erase_start;
            }
            key_type child_key;
            node_ptr _;
            if (node->type == NodeType::HashNode){
                std::tie(child_key, _) = hash_table.find(node, pos);
                AEX_ASSERT(_ == child);
            }
            else
                child_key = d_n(node)->key_ptr[0];
            update(i_n(node), pos, next_pos, child_key, d_n(child)->child_ptr[0]);
            free_node(child);
            continue;
        }
        AEX_PRINT("8");
        if (!check_erase_SMO(child)){
            SU(child);
            ERASE_UNLOCK();
            goto _erase_start;
        }
        ERASE_UNLOCK();
        node = child;
    }
    AEX_ASSERT(0 == 1);
    SU();
    #undef ERASE_UNLOCK
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::erase(hash_node_ptr node, const slot_type pos, const slot_type next_pos){
    AEX_ASSERT(node->is_occupied(pos) == true);
    AEX_ASSERT(check_lock_shared(node));
    AEX_ASSERT(check_unlock(node));
    key_type prev_key;
    node_ptr prev_node;
    std::tie(prev_key, prev_node) = hash_table.find(node, node->prev_item_find(pos - 1));
    bitmap_impl::set_zero(node->bitmap_ptr, pos);
    if ((pos & 63) == 0)
        hash_table.update(node, pos, prev_key, prev_node);
    else
        hash_table.erase(node, pos);
    for (slot_type j = highbit<slot_type, traits::SLOT_PER_SHORT_CUT>(pos + 1); j < next_pos; j += traits::SLOT_PER_SHORT_CUT)
        hash_table.update(node, j, prev_key, prev_node);
    node->meta_lock.lock();
    --node->size;
    node->meta_lock.unlock();
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
    if constexpr(traits::AllowRebuild)
        if (is_rebuild(i_n(node))){
            if (!TUL(node))
                return false;
            rebuild(i_n(node));
            DL(node);
            return true;
        }
    if (isfew(i_n(node))){
        if (!TUL(node))
            return false;
        narrow(i_n(node));
        DL(node);
    }
    return true;
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::erase_lock(inner_node_ptr node, const slot_type pos, const slot_type next_pos) const {
    if (node->type == NodeType::HashNode){
        if (!h_n(node)->try_array_upgrade_lock(pos - 1, next_pos))
            return false;        
    }
    else{
        if (!TUL(node))
            return false;
    }
    return true;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::erase_unlock(inner_node_ptr node, const slot_type pos, const slot_type next_pos) const {
    AEX_ASSERT(node->type != NodeType::LeafNode);
    if (node->type == NodeType::HashNode){
        h_n(node)->array_unlock_shared(pos - 1, next_pos);
        SU(node);
    }
    else
        XU(node);
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::merge(data_node_ptr left_node, data_node_ptr right_node){
    AEX_ASSERT(check_lock(left_node));
    AEX_ASSERT(check_lock(right_node));
    AEX_ASSERT(left_node != right_node);
    AEX_ASSERT(left_node->next == right_node);
    AEX_ASSERT(left_node->size + right_node->size <= traits::MIN_DATA_NODE_SLOT_SIZE);
    #ifdef AEX_DEBUG
    ++this->opt_stats.data_node_merge_cnt;
    #endif
    std::move(right_node->key,  right_node->key  + right_node->size, left_node->key  + left_node->size);
    std::move(right_node->data, right_node->data + right_node->size, left_node->data + left_node->size);
    left_node->size += right_node->size;
    left_node->next = right_node->next;
    free_node(right_node);
}

}