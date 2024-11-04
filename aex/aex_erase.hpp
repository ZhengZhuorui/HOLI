#pragma once
#include "aex/aex.h"

namespace aex{

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::_erase(const key_type &key){
#define ERASE_UNLOCK() erase_unlock(node, pos - 1, next_pos, child);
#define ERASE_NODE_UNLOCK(node, pos, next_pos) \
    {\
        node->array_unlock_shared(pos, next_pos); \
        if (pos2slot(pos - 1) != pos2slot(pos))   \
            node->array_unlock_shared(pos - 1);   \
    }

    key_type split_key, update_key;
    node_ptr node, child, prev_pos, pos, next_pos, next_next_pos, update_node;
    node = root;
    version_type now_version;
    bool ret, tail, erase_flag;

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
        child = find_update(node, key, tail, pos, next_pos);
        if (child->type == NodeType::LeafNode){
            if (child->version > now_version){
                ERASE_UNLOCK();
                goto _erase_start;
            }

            if (tail){
                if (child->size == 1 && l_n(child)->key[0] == key){
                    if (!erase_tail_leaf_node(node, child)){
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

            if (isfew(child)){
                XL(child->next);
                if (node->type == NodeType::DenseNode){
                    if (!TUL(node)){
                        XU(child->next);
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
                    if (node->try_array_upgrade_lock(next_pos, next_pos)){
                        XU(child->next);
                        ERASE_UNLOCK();
                        goto _erase_start;
                    }
                    next_next_pos = node->array_lock_until_next_item(next_pos, next_pos + 1);
                    ret = l_n(child->erase(key));
                    merge(l_n(child), l_n(child)->next);
                    erase(h_n(node), pos, next_pos, next_next_pos);
                    node->array_unlock(next_pos, next_next_pos);
                    node->array_unlock_shared(pos, next_pos - traits::SLOT_PER_LOCK);
                    XU(child);
                    return ret;
                }
            }
        }
        else if (child->size == 1){
            AEX_ASSERT(child->type == NodeType::DenseNode);
            if (erase_lock(node, pos, next_pos)){
                ERASE_UNLOCK();
                goto _erase_start;
            }
            update_key = d_n(child)->key_ptr[0];
            update_node = d_n(child)->child_ptr[0];
            update(node, pos, next_pos, update_key, update_node);
            free_node(child);
            continue;
        }

        if (!check_erase_SMO(child)){
            if (node->type == NodeType::HashNode)
                ERASE_NODE_UNLOCK(node, pos, next_pos);
            SU(child);
            goto _erase_loop_start;
        }

        if (node->type == NodeType::HashNode)
            ERASE_NODE_UNLOCK(node, pos, next_pos);
        SU(node);

        node = child;
    }
    #undef ERASE_UNLOCK
    #undef ERASE_NODE_UNLOCK
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::erase(hash_node_ptr node, const slot_type prev_pos, const slot_type pos, const slot_type next_pos){
    AEX_ASSERT(node->is_occupied(pos) == true);
    
    key_type prev_key;
    node_ptr prev_node;

    std::tie(prev_key, prev_node) = hash_table.find(node, prev_pos);
    bitmap_impl::set_zero(node->bitmap_ptr, pos);
    hash_table.erase(node, pos);
    for (slot_type j = highbit_64(pos); j < next_pos; j += 64)
        hash_table.update(node, j, prev_key, prev_node);
    node->array_downgrade_lock(prev_pos, next_pos);
    --node->size;
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

/* erase a iterator recursive */
template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::erase_iterator(const_iterator &iter){
    AEX_ASSERT(!traits::AllowConcurrency);
    AEX_ASSERT(node != empty_leaf);
    AEX_ASSERT(check_lock(node));
    this->erase(iter.key);
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::erase_init(const key_type &key, bool &erase_flag){
    SL();
    if (key < this->min_key){
        erase_flag = false;
        return false;
    }
_erase_init_start:
    SL(root);
    if (root->size == 1){
        AEX_ASSERT(root->type == NodeType::DenseNode);
        AEX_ASSERT(root->key_ptr[0] == this->min_key);
        SL(root->child_ptr[0]);
        if (root->child_ptr[0] == NodeType::LeafNode && root->child_ptr[0] == 1 && l_n(root->child_ptr[0])->key[0] == key){
            if (!TUL(root)){
                SU(root);
                SU(root->child_ptr[0]);
                goto _erase_init_start;
            }
            free_node(root->child_ptr[0]);
            free_node(root);
            root = nullptr;
            head_leaf = tail_leaf = nullptr;
            erase_flag = true;
            return false;
        }
    }
    return true;
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::check_erase_SMO(node_ptr node){
    AEX_ASSERT(node->type != NodeType::LeafNode);
    if (is_rebuild(node)){
        if (!TUL(node))
            return false;
        rebuild(node);
        DL(node);
    }
    else if (isfew(node)){
        if (!TUL(node))
            return false;
        narrow(node);
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
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::erase_unlock(inner_node_ptr node, const slot_type pos, const slot_type next_pos, node_ptr child){
    AEX_ASSERT(node->type != NodeType::LeafNode);
    if (node->type == NodeType::HashNode){
        h_n(node)->array_unlock_shared(pos - 1, next_pos);
        //h_n(node)->array_downgrade_lock(pos, next_pos);
        //if (pos2slot(pos - 1) != pos2slot(pos))
        //    h_n(node)->array_unlock_shared(pos - 1, pos - 1);
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
        if (this->tail_leaf == left_node)
            this->tail_leaf = right_node;
        left_node->next = right_node->next;
        free_node(right_node);
        return true;
    }
    else return false;
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::erase_tail_leaf_node(inner_node_ptr parent, data_node_ptr child, const slot_type pos, const slot_type next_pos){
    const slot_type prev_pos = parent->prev_item_find(pos - 1);
    node_ptr prev_node = nullptr;
    AEX_ASSERT(pos != 0);
    AEX_ASSERT(child->prev != nullptr);

    if (!TUL(child))
        return false;
    if (!TXL(child->prev)){
        XU(child);
        return false;
    }
    if (erase_lock(i_n(parent), pos, next_pos)){
        XU(prev_node);
        DL(child);
        return false;
    }
    prev_node->next = child->next;
    XU(prev_node);
    if (tail_leaf == child)
        tail_leaf = prev_node;
    if (parent->type == NodeType::HashNode)
        erase(parent, prev_pos, pos, next_pos);
    else
        erase(parent, pos);
    return true;
}

}