#pragma once
#include "aex/aex.h"

namespace aex{

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::_erase(const key_type &key){
#define ERASE_UNLOCK() erase_unlock(i_n(node), pos, next_pos, child);

    node_ptr node, child;
    slot_type pos, next_pos, next_next_pos;
    version_type now_version;
    bool ret, erase_flag, tail;

_erase_start:
    if (!erase_init(key, erase_flag)){
        return erase_flag;
    }
    SL(node);
    
    if (!check_erase_SMO(node)){
        SU(node);
        goto _erase_start;
    }
    now_version = this->version;
    
    while (true){
_erase_loop_start:
        child = find_update(i_n(node), key, pos, next_pos);
        tail = (node->type == NodeType::HashNode) ? (next_pos >= h_n(node)->slot_size) : (next_pos >= d_n(node)->size);
        if (child->type == NodeType::LeafNode){
            if (now_version < l_n(child)->version){
                ERASE_UNLOCK();
                goto _erase_start;
            }

            if (tail){
                if (l_n(child)->size == 1 && l_n(child)->key[0] == key){
                    if (!erase_tail_leaf_node(i_n(node), l_n(child), pos, next_pos, now_version)){
                        ERASE_UNLOCK();
                        goto _erase_start;
                    }
                    return true;
                }
                else{
                    ret = l_n(child)->erase(key);
                    ERASE_UNLOCK();
                    return ret;
                }
            }
            if (isfew(l_n(child))){
                XL(l_n(child)->next);
                if (node->type == NodeType::DenseNode){
                    if (!TUL(node)){
                        XU(l_n(child)->next);
                        ERASE_UNLOCK();
                        goto _erase_start;
                    }
                    ret = l_n(child)->erase(key);
                    merge(l_n(child), l_n(child)->next);
                    AEX_ASSERT(next_pos == pos + 1);
                    erase(d_n(node), next_pos);
                    ERASE_UNLOCK();
                    return ret;
                }
                else{
                    AEX_ASSERT(node->type == NodeType::HashNode);
                    if (h_n(node)->try_array_upgrade_lock(next_pos, next_pos)){
                        XU(l_n(child)->next);
                        ERASE_UNLOCK();
                        goto _erase_start;
                    }
                    next_next_pos = h_n(node)->array_lock_until_next_item(next_pos, next_pos + 1);
                    ret = l_n(child)->erase(key);
                    merge(l_n(child), l_n(child)->next);
                    erase(h_n(node), pos, next_pos, next_next_pos);
                    h_n(node)->array_unlock(next_pos, next_next_pos);
                    h_n(node)->array_unlock_shared(pos - 1, next_pos - traits::SLOT_PER_LOCK);
                    XU(child);
                    return ret;
                }
            }
        }
        else if (child->type == NodeType::DenseNode && d_n(child)->size == 1){
            AEX_ASSERT(child->type == NodeType::DenseNode);
            if (erase_lock(i_n(node), pos, next_pos)){
                ERASE_UNLOCK();
                goto _erase_start;
            }
            update(i_n(node), pos, next_pos, d_n(child)->key_ptr[0], d_n(child)->child_ptr[0]);
            free_node(i_n(child));
            continue;
        }

        if (!check_erase_SMO(child)){
            if (node->type == NodeType::HashNode)
                h_n(node)->array_unlock_shared(pos - 1, next_pos); 
            SU(child);
            goto _erase_loop_start;
        }

        if (node->type == NodeType::HashNode)
            h_n(node)->array_unlock_shared(pos - 1, next_pos); 
        SU(node);

        node = child;
    }
    #undef ERASE_UNLOCK
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::erase(hash_node_ptr node, const slot_type prev_pos, const slot_type pos, const slot_type next_pos){
    AEX_ASSERT(node->is_occupied(pos) == true);
    
    key_type prev_key;
    node_ptr prev_node;

    std::tie(prev_key, prev_node) = hash_table.find(node, prev_pos);
    bitmap_impl::set_zero(node->bitmap_ptr, pos);
    hash_table.erase(node, pos);
    for (slot_type j = highbit_64(pos); j < next_pos; j += traits::SLOT_PER_LOCK)
        hash_table.update(node, j, prev_key, prev_node);
    node->array_downgrade_lock(prev_pos, next_pos);
    node->meta_lock.lock();
    --node->size;
    node->meta_lock.unlock();
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::erase(dense_node_ptr node, const slot_type pos){
    AEX_ASSERT(check_lock(node));
    AEX_ASSERT(pos == node->size);
    std::move(node->key_ptr   + pos + 1, node->key_ptr   + node->size, node->key_ptr   + pos);
    std::move(node->child_ptr + pos + 1, node->child_ptr + node->size, node->child_ptr + pos);
    DL(node);
    --node->size;
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::erase_init(const key_type &key, bool &erase_flag){
    node_ptr child;
    SL();
_erase_init_start:
    SL(root);
    if (root->type == NodeType::DenseNode && d_n(root)->size == 1){
        AEX_ASSERT(d_n(root)->key_ptr[0] == std::numeric_limits<key_type>::lowest());
        SL(d_n(root)->child_ptr[0]);
        child = d_n(root)->child_ptr[0];
        if (child->type == NodeType::LeafNode && l_n(child)->size == 1 && l_n(child)->key[0] == key){
            if (!TUL(root)){
                SU(root);
                SU(child);
                goto _erase_init_start;
            }
            free_node(child);
            free_node(root);
            root = nullptr;
            head_leaf = nullptr;
            erase_flag = true;
            return false;
        }
    }
    return true;
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::check_erase_SMO(node_ptr node){
    AEX_ASSERT(node->type != NodeType::LeafNode);
    if (is_rebuild(i_n(node))){
        if (!TUL(node))
            return false;
        rebuild(i_n(node));
        DL(node);
    }
    else if (isfew(i_n(node))){
        if (!TUL(node))
            return false;
        narrow(i_n(node));
        DL(node);
    }
    return true;
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::erase_lock(inner_node_ptr node, const slot_type pos, const slot_type next_pos){
    if (node->type == NodeType::HashNode){
        if (h_n(node)->try_array_upgrade_lock(pos, next_pos))
            return false;        
    }
    else{
        if (!TUL(node))
            return false;
    }
    return true;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::erase_unlock(inner_node_ptr node, const slot_type pos, const slot_type next_pos, node_ptr child){
    AEX_ASSERT(node->type != NodeType::LeafNode);
    if (node->type == NodeType::HashNode){
        h_n(node)->array_unlock_shared(pos - 1, next_pos);
        SU(node);
    }
    else
        XU(node);
    XU(child);
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::merge(data_node_ptr left_node, data_node_ptr right_node){
    AEX_ASSERT(left_node != right_node);
    if (left_node->size + right_node->size <= traits::MIN_DATA_NODE_SLOT_SIZE){
        #ifdef AEX_DEBUG
        ++this->opt_stats.data_node_merge_cnt;
        #endif
        std::move(right_node->key,  right_node->key  + right_node->size, left_node->key  + left_node->size);
        std::move(right_node->data, right_node->data + right_node->size, left_node->data + left_node->size);
        left_node->size += right_node->size;
        left_node->next = right_node->next;
        free_node(right_node);
        return true;
    }
    else return false;
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::erase_tail_leaf_node(inner_node_ptr parent, data_node_ptr child, const slot_type pos, const slot_type next_pos, const version_type now_version){
    [[maybe_unused]]key_type key;
    node_ptr prev_node = nullptr;
    data_node_ptr prev_child;
    slot_type prev_pos = 0;
    if (parent->type == NodeType::HashNode){
        prev_pos = h_n(parent)->prev_item_find(pos - 1);
        std::tie(key, prev_node) = hash_table.find(parent, prev_pos);
    }
    else
        prev_node = d_n(parent)->child_ptr[pos - 1];
    SL(prev_node);
    prev_child = find_tail_leaf(prev_node);
    AEX_ASSERT(check_lock_shared(prev_child));
    AEX_ASSERT(prev_child->next == child);
    AEX_ASSERT(pos != 0);
    if (now_version < l_n(prev_child)->version){
        SU(prev_child);
        return false;
    }
    if (!TUL(prev_child)){
        SU(prev_child);
        return false;
    }
    if (!TUL(child)){
        XU(prev_child);
        return false;
    }

    prev_child->next = child->next;
    ++this->version;
    prev_child->version = this->version;
    XU(prev_child);
    if (parent->type == NodeType::HashNode)
        erase(h_n(parent), prev_pos, pos, next_pos);
    else
        erase(d_n(parent), pos);
    return true;
}

}