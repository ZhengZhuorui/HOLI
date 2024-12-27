#pragma once
#include "aex/aex.h"

namespace aex{

template<typename _Key, typename _Val, typename traits>
inline std::pair<typename aex_tree<_Key, _Val, traits>::iterator, bool> aex_tree<_Key, _Val, traits>::insert(const key_type key, const value_type &value){
    //AEX_ASSERT(root->size == 0 || node_zero_key(root) == std::numeric_limits<key_type>::lowest());
    bool tail;
    key_type split_key;
    inner_node_ptr top_node, node;
    node_ptr child;
    slot_type pos, top_pos = 0, split_pos;
    data_node_ptr new_node;
    version_type now_version;
    int restart_count = 0;
    std::pair<iterator, bool> ret;
    //AEX_PRINT("root=" << root);
insert_start:
    if (restart_count++)
        yield(restart_count);
    now_version = this->version;
    top_node = nullptr;
    node = root;
    SL(node);    
    if (!check_insert_SMO(node)){
        SU(node);
        goto insert_start;
    }

    while (true){
        child = find_insert(node, key, pos);
        //AEX_PRINT("node=" << node << ", key=" << key << ", pos=" << pos << ", child->type=" << to_string(child->type));
        tail = (node->type == NodeType::HashNode) ? (last_node(h_n(node)) == child) : (pos == d_n(node)->size - 1);
        if (!tail){
            if (top_node != nullptr){
                SU(top_node);
                top_node = nullptr;
            }
            top_node = node;
            top_pos = pos;
        }

        if (child->type == NodeType::LeafNode){
            if (now_version < l_n(child)->version){
                SU(child); insert_unlock(top_node, node);
                goto insert_start;
            }
            slot_type child_pos = l_n(child)->find_lower_pos(key);
            AEX_DEBUG_BLOCK({if (l_n(child)->next != nullptr) AEX_ASSERT(l_n(child)->next->key[0] > key);});
            if (!traits::AllowMultiKey && pos < l_n(child)->size && l_n(child)->key[pos] == key){
                ret = std::make_pair(iterator(l_n(child), child_pos), false);
                SU(child); insert_unlock(top_node, node);
                return ret;
            }
            if (isfull(l_n(child))){
                //AEX_PRINT("1");
                split_key = l_n(child)->key[traits::MIN_DATA_NODE_SLOT_SIZE >> 1];
                if (top_node != nullptr && top_node != node){
                    if ((top_node->type == NodeType::HashNode && check_upgrade(h_n(top_node), split_key, top_pos))
                        || (top_node->type == NodeType::DenseNode && pos < node->slot_size - 1)){
                        SU(node);
                        node = top_node;
                        pos = top_pos;
                    }
                    else{
                        SU(top_node);
                        top_node = nullptr;
                    }
                }
                if (node->type == NodeType::HashNode){
                    split_pos = h_n(node)->predict(split_key);
                    if (!TUL(child)){
                        SU(child); SU(node);
                        goto insert_start;
                    }
                    ret = insert_data_node(l_n(child), new_node, key, value);
                    if (!h_n(node)->is_occupied_con(split_pos) && split_pos < h_n(node)->slot_size){
                        //AEX_PRINT("2, node=" << node << ", pos=" << pos << ", split_pos=" << split_pos);
                        insert_no_collision(h_n(node), split_pos, split_key, new_node);
                    }
                    else{
                        //AEX_PRINT("3, node=" << node << ", pos=" << pos << ", split_pos=" << split_pos);
                        insert_collision(h_n(node), pos, split_key, new_node);
                    }
                    XU(child); SU(node);
                }
                else{
                    //AEX_PRINT("4");
                    if (!TUL(node)){
                        SU(child); SU(node);
                        goto insert_start;
                    }
                    if (!TUL(child)){
                        SU(child); XU(node);
                        goto insert_start;
                    }
                    ret = insert_data_node(l_n(child), new_node, key, value);
                    insert(d_n(node), split_key, new_node);
                    XU(child); XU(node);
                }
            }
            else{
                if (!TUL(child)){
                    SU(child); insert_unlock(top_node, node);
                    goto insert_start;
                }
                ret = std::make_pair(iterator(l_n(child), child_pos), true);
                l_n(child)->insert(key, value, child_pos);
                XU(child); insert_unlock(top_node, node);
            }
            ++this->m_stats.size;
            return ret;
        }

        if (!check_insert_SMO(i_n(child))){
            SU(child); insert_unlock(top_node, node);
            goto insert_start;
        }
        AEX_ASSERT(check_lock_shared(child));
        if (node != top_node){
            AEX_ASSERT(top_node == nullptr || tail == true);
            SU(node);
        }
        node = i_n(child);
    }
    AEX_ASSERT(0 == 1);
    return ret;
    #undef INSERT_UNLOCK
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::construct_tmp_node(dense_node_ptr node, const key_type old_key, const node_ptr old_node, const key_type new_key, const node_ptr new_node){
    node->key_ptr[0]   = old_key;
    node->key_ptr[1]   = new_key;
    node->child_ptr[0] = old_node;
    node->child_ptr[1] = new_node;
    if (old_key > new_key){
        std::swap(node->key_ptr[0],   node->key_ptr[1]);
        std::swap(node->child_ptr[0], node->child_ptr[1]);
    }
    node->size = 2;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::__construct_insert(hash_node_ptr node, const slot_type pos, const slot_type next_pos, const key_type key, const node_ptr child){
    AEX_ASSERT(check_lock(node));
    AEX_ASSERT(pos < node->slot_size);
    AEX_ASSERT(pos < next_pos);
    AEX_ASSERT(node->is_occupied(pos) == false);
    AEX_ASSERT(hash_table.find(node, pos).second == nullptr);
    hash_table.insert(node, pos, key, child);
    for (slot_type i = highbit<slot_type, traits::SLOT_PER_SHORTCUT>(pos + 1); i < next_pos; i += traits::SLOT_PER_SHORTCUT)
        hash_table.insert(node, i, key, child);
    
    bitmap_impl::set_one(node->bitmap_ptr, pos);
    node->meta_lock.lock();
    ++node->size;
    node->meta_lock.unlock();
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::insert_collision(hash_node_ptr node, const slot_type pos, const key_type key, const data_node_ptr child){
    AEX_ASSERT(check_lock_shared(node));
    AEX_ASSERT(pos < node->slot_size);
    #ifdef AEX_DEBUG
    opt_stats.allocate_dense_node_cnt++;
    #endif
    dense_node_ptr new_node = Allocator::allocate_dense_node(traits::MIN_DENSE_NODE_SLOT_SIZE);
    key_type prev_key;
    node_ptr prev_child;
    std::tie(prev_key, prev_child) = hash_table.find(node, pos);
    AEX_ASSERT(prev_child->type == NodeType::LeafNode);
    AEX_ASSERT(l_n(prev_child)->next == child);
    construct_tmp_node(new_node, prev_key, prev_child, key, child);
    hash_table.update(node, pos, new_node->key_ptr[0], new_node);
    slot_type next_pos = node->next_item_con(pos + 1);
    for (slot_type i = highbit<slot_type, traits::SLOT_PER_SHORTCUT>(pos + 1); i < next_pos; i += traits::SLOT_PER_SHORTCUT)
        hash_table.update(node, i, new_node->key_ptr[0], new_node);
    SU(node, next_pos);
}


template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::insert_no_collision(hash_node_ptr node, const slot_type pos, const key_type key, const data_node_ptr child){
    AEX_ASSERT(check_lock_shared(node));
    AEX_ASSERT(pos < node->slot_size);
    if ((pos & (traits::SLOT_PER_SHORTCUT - 1)) == 0)
        hash_table.update(node, pos, key, child);
    else
        hash_table.insert(node, pos, key, child);
    slot_type next_pos = node->next_item_con(pos + 1);
    AEX_ASSERT(pos < next_pos);
    for (slot_type i = highbit<slot_type, traits::SLOT_PER_SHORTCUT>(pos + 1); i < next_pos; i += traits::SLOT_PER_SHORTCUT)
        hash_table.update(node, i, key, child);
    SU(node, next_pos);
    node->set_one(pos);
    node->meta_lock.lock();
    ++node->size;
    node->meta_lock.unlock();
    
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::insert(dense_node_ptr node, const key_type key, const node_ptr child){
    AEX_ASSERT(check_lock(node));
    const slot_type pos = std::upper_bound(node->key_ptr + 1, node->key_ptr + node->size, key) - node->key_ptr;
    std::move_backward(node->key_ptr   + pos, node->key_ptr   + node->size, node->key_ptr   + node->size + 1);
    std::move_backward(node->child_ptr + pos, node->child_ptr + node->size, node->child_ptr + node->size + 1);
    node->key_ptr[pos]   = key;
    node->child_ptr[pos] = child;
    ++node->size;
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::check_insert_SMO(inner_node_ptr node){
    AEX_ASSERT(node->type != NodeType::LeafNode);
    if constexpr (traits::AllowRebuild)
        if (is_rebuild(node)){
            if (!TUL(node))
                return false;
            rebuild(node);
            DL(node);
            return true;
        }
    if (isfull(node)){
        if (!TUL(node))
            return false;
        while (isfull(node))
            expand(node);
        DL(node);
    }
    return true;
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::check_upgrade(hash_node_ptr node, const key_type split_key, const slot_type top_pos) const {
    AEX_ASSERT(node->type == NodeType::HashNode);
    slot_type split_pos = node->predict(split_key);
    if (top_pos == split_pos || node->is_occupied_con(split_pos) || split_pos >= node->slot_size)
        return false;
    return true;
}

template<typename _Key, typename _Val, typename traits>
inline std::pair<typename aex_tree<_Key, _Val, traits>::iterator, bool> aex_tree<_Key, _Val, traits>::insert_data_node(data_node_ptr node, data_node_ptr &new_node, const key_type key, const value_type &value){
    AEX_ASSERT(check_lock(node));
    slot_type pos = node->find_lower_pos(key);
    std::pair<iterator, bool> ret;
    #ifdef AEX_DEBUG
    opt_stats.allocate_data_node_cnt++;
    #endif
    new_node = new data_node(this->version);
    split(node, new_node);
    if (pos <= node->size){
        l_n(node)->insert(key, value, pos);
        ret = std::make_pair(iterator(node, pos), true);
        AEX_ASSERT(key <= new_node->key[0]);
    }
    else {
        pos -= node->size;
        l_n(new_node)->insert(key, value, pos);
        ret = std::make_pair(iterator(new_node, pos), true);
        AEX_ASSERT(key >= node->key[node->size - 1]);
    }
    return ret;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::insert_unlock(inner_node_ptr top_node, inner_node_ptr node) const {
    if constexpr (!traits::AllowConcurrency)
        return;                
    SU(node);
    if (top_node != node && top_node != nullptr){                       
        SU(top_node);
    } 
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::split(data_node_ptr old_node, data_node_ptr new_node){
    AEX_ASSERT(traits::AllowDynamicDataNode == false);
    #ifdef AEX_DEBUG
    ++opt_stats.data_node_split_cnt;
    #endif
    
    if constexpr (traits::AllowConcurrency){
        ++this->version;
        old_node->version = new_node->version = this->version;
    }
    new_node->next = old_node->next;
    old_node->next = new_node;    
    
    ULL mid = traits::MIN_DATA_NODE_SLOT_SIZE >> 1;
    std::move(old_node->key  + mid, old_node->key  + old_node->size, new_node->key );
    std::move(old_node->data + mid, old_node->data + old_node->size, new_node->data);
    
    //if constexpr (std::is_same_v<data_node, aex_hash_data_node<_Key, _Val, traits>>)
    //    std::move(old_node->fp   + mid, old_node->fp   + old_node->size, new_node->fp);
    new_node->size = old_node->size - mid;
    old_node->size = mid;
}

}
