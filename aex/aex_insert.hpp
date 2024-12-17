#pragma once
#include "aex/aex.h"

namespace aex{

template<typename _Key, typename _Val, typename traits>
inline std::pair<typename aex_tree<_Key, _Val, traits>::iterator, bool> aex_tree<_Key, _Val, traits>::insert(const key_type &key, const value_type &value){
    #define INSERT_UNLOCK() insert_unlock(top_node, top_pos, top_next_pos, node, pos, next_pos)
    //AEX_ASSERT(root->size == 0 || node_zero_key(root) == std::numeric_limits<key_type>::lowest());
    bool restart = false, inserted, tail;
    key_type split_key;
    hash_node_ptr top_node;
    slot_type pos, next_pos, top_pos = 0, top_next_pos = 0, split_pos;
    data_node_ptr new_node;
    node_ptr node, child;
    version_type now_version;
    int restart_count = 0;
    std::pair<iterator, bool> ret;
    SL();
insert_start:
    if (restart_count++)
        yield(restart_count);
    now_version = this->version;
    top_node = nullptr;
    node = root;
    SL(node);    
    if (!check_insert_SMO(i_n(node))){
        SU(node);
        goto insert_start;
    }

    while (true){
        child = find_update(i_n(node), key, pos, next_pos);
        AEX_PRINT("node=" << node << ", key=" << key << ", pos=" << pos << ", next_pos=" << next_pos);
        AEX_DEBUG_BLOCK({if constexpr(traits::AllowConcurrency){
            if (node->type == NodeType::HashNode){
                if (pos - 1 >= 0) AEX_ASSERT(h_n(node)->lock_array[pos2slot(pos - 1)].is_lock_shared());
                if (next_pos < h_n(node)->slot_size) AEX_ASSERT(h_n(node)->lock_array[pos2slot(next_pos)].is_lock_shared());
                //if (next_pos + traits::SLOT_PER_LOCK < h_n(node)->slot_size) AEX_ASSERT(!h_n(node)->lock_array[pos2slot(next_pos) + 1].is_lock_shared());
            }
        }});

        tail = (node->type == NodeType::HashNode) ? (next_pos >= h_n(node)->slot_size) : (next_pos >= d_n(node)->size);        
        if (!tail){
            if (top_node != nullptr){
                SU(top_node, top_pos - 1, top_next_pos);
                top_node = nullptr;
            }
            if (node->type == NodeType::HashNode && next_pos - pos > 1){
                top_node = h_n(node);
                top_pos  = pos;
                top_next_pos = next_pos;
            }
        }

        if (child->type == NodeType::LeafNode){
            if (now_version < l_n(child)->version){
                INSERT_UNLOCK();
                SU(child);
                goto insert_start;
            }
            slot_type child_pos = l_n(child)->find_lower_pos(key);
            AEX_DEBUG_BLOCK({if (l_n(child)->next != nullptr) AEX_ASSERT(l_n(child)->next->key[0] > key);});
            if (!traits::AllowMultiKey && pos < l_n(child)->size && l_n(child)->key[pos] == key){
                ret = std::make_pair(iterator(l_n(child), child_pos), false);
                INSERT_UNLOCK();
                SU(child);
                return ret;
            }
            if (isfull(l_n(child))){
                inserted = false;
                split_key = l_n(child)->key[traits::MIN_DATA_NODE_SLOT_SIZE >> 1];
                if (top_node != nullptr && top_node != node){
                    split_pos = top_node->predict(split_key);
                    inserted = check_upgrade_and_lock(top_node, split_pos, top_pos, top_next_pos, restart);
                    if (restart){
                        SU(child);
                        INSERT_UNLOCK();
                        goto insert_start;
                    }
                    if (inserted){
                        if (!TUL(child)){
                            SU(child);
                            XU(top_node, top_pos - 1, top_next_pos);
                            SU(h_n(node), pos - 1, next_pos);
                            goto insert_start;
                        }
                        ret = insert_data_node(l_n(child), new_node, key, value);
                        insert(top_node, split_pos, top_next_pos, split_key, new_node);
                        XU(child);
                        if (node->type == NodeType::HashNode)
                            h_n(node)->array_unlock_shared(pos - 1, next_pos);
                        SU(node);
                        XU(top_node, top_pos - 1, top_next_pos);
                    }
                    else
                        SU(top_node, top_pos - 1, top_next_pos);
                    top_node = nullptr;
                }
                if (!inserted){
                    if (node->type == NodeType::HashNode){
                        split_pos = h_n(node)->predict(split_key);
                        if (!h_n(node)->try_array_upgrade_lock(pos - 1, next_pos)){
                            SU(child); SU(h_n(node), pos - 1, next_pos);
                            goto insert_start;
                        }
                        if (!TUL(child)){
                            SU(child); XU(h_n(node), pos - 1, next_pos);
                            goto insert_start;
                        }
                        ret = insert_data_node(l_n(child), new_node, key, value);
                        if (split_pos < next_pos && split_pos < h_n(node)->slot_size)
                            insert(h_n(node), split_pos, next_pos, split_key, new_node);
                        else
                            insert(h_n(node), pos, next_pos, split_key, new_node);
                        XU(child); XU(h_n(node), pos - 1, next_pos);
                    }
                    else{
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
            }
            else{
                if (!TUL(child)){
                    SU(child); INSERT_UNLOCK();
                    goto insert_start;
                }
                ret = std::make_pair(iterator(l_n(child), child_pos), true);
                l_n(child)->insert(key, value, child_pos);
                XU(child); INSERT_UNLOCK();
            }
            SU();
            ++this->m_stats.size;
            return ret;
        }

        if (!check_insert_SMO(i_n(child))){
            INSERT_UNLOCK();
            goto insert_start;
        }
        AEX_ASSERT(check_lock_shared(child));
        if (node != top_node){
            AEX_ASSERT(top_node == nullptr || tail == true);
            if (node->type == NodeType::HashNode)
                h_n(node)->array_unlock_shared(pos - 1, next_pos);
            SU(node);
        }
        node = child;
    }
    AEX_ASSERT(0 == 1);
    return ret;
    #undef INSERT_UNLOCK
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::construct_tmp_node(dense_node_ptr node, const key_type &old_key, const node_ptr old_node, const key_type &new_key, const node_ptr new_node){
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
inline void aex_tree<_Key, _Val, traits>::__construct_insert(hash_node_ptr node, const slot_type pos, const slot_type next_pos, const key_type &key, const node_ptr child){
    AEX_ASSERT(pos < node->slot_size);
    AEX_ASSERT(pos < next_pos);
    AEX_ASSERT(node->is_occupied(pos) == false);
    AEX_ASSERT(hash_table.find(node, pos).second == nullptr);
    bitmap_impl::set_one(node->bitmap_ptr, pos);
    AEX_ASSERT(node->is_occupied(pos) == true);
    hash_table.insert(node, pos, key, child);
    for (slot_type i = highbit<slot_type, traits::SLOT_PER_SHORTCUT>(pos + 1); i < next_pos; i += traits::SLOT_PER_SHORTCUT)
        hash_table.insert(node, i, key, child);
    node->meta_lock.lock();
    ++node->size;
    node->meta_lock.unlock();
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::__insert(hash_node_ptr node, const slot_type pos, const slot_type next_pos, const key_type &key, const node_ptr child){
    AEX_ASSERT(pos < node->slot_size);
    AEX_ASSERT(pos < next_pos);
    AEX_ASSERT(node->is_occupied(pos) == false);
    bitmap_impl::set_one(node->bitmap_ptr, pos);
    if ((pos & 63) == 0)
        hash_table.update(node, pos, key, child);
    else
        hash_table.insert(node, pos, key, child);
    for (slot_type i = highbit<slot_type, traits::SLOT_PER_SHORTCUT>(pos + 1); i < next_pos; i += traits::SLOT_PER_SHORTCUT)
        hash_table.update(node, i, key, child);
    node->meta_lock.lock();
    ++node->size;
    node->meta_lock.unlock();
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::insert(hash_node_ptr node, const slot_type pos, const slot_type next_pos, const key_type &key, const node_ptr child){
    key_type old_key;
    node_ptr old_node;

    AEX_ASSERT(check_lock_shared(node));
    AEX_ASSERT(pos < node->slot_size);

    if (node->is_occupied(pos)){
        #ifdef AEX_DEBUG
        opt_stats.allocate_dense_node_cnt++;
        #endif
        dense_node_ptr new_node = Allocator::allocate_dense_node(traits::MIN_DENSE_NODE_SLOT_SIZE);
        std::tie(old_key, old_node) = hash_table.find(node, pos);
        construct_tmp_node(new_node, old_key, old_node, key, child);
        hash_table.update(node, pos, new_node->key_ptr[0], new_node);
        for (slot_type i = highbit<slot_type, traits::SLOT_PER_SHORTCUT>(pos + 1); i < next_pos; i += traits::SLOT_PER_SHORTCUT)
            hash_table.update(node, i, new_node->key_ptr[0], new_node);
        AEX_ASSERT(check_unlock_shared(new_node));
        AEX_ASSERT(check_unlock(new_node));
    }
    else {
        __insert(node, pos, next_pos, key, child);
    }
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::insert(dense_node_ptr node, const key_type &key, const node_ptr child){
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
inline bool aex_tree<_Key, _Val, traits>::check_upgrade_and_lock(hash_node_ptr node, const slot_type split_pos, const slot_type top_pos, const slot_type top_next_pos, bool &restart) const {
    AEX_ASSERT(node->type == NodeType::HashNode);
    restart = false;
    if (top_pos == split_pos || top_next_pos == split_pos || split_pos >= node->slot_size)
        return false;
    AEX_ASSERT(!node->is_occupied(split_pos));
    restart = !(node->try_array_upgrade_lock(top_pos - 1, top_next_pos));
    if (restart)
        return false;
    return true;
}

template<typename _Key, typename _Val, typename traits>
inline std::pair<typename aex_tree<_Key, _Val, traits>::iterator, bool> aex_tree<_Key, _Val, traits>::insert_data_node(data_node_ptr node, data_node_ptr &new_node, const key_type &key, const value_type &value){
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
inline void aex_tree<_Key, _Val, traits>::insert_unlock(hash_node_ptr top_node, const slot_type top_pos, const slot_type top_next_pos, node_ptr node, const slot_type pos, const slot_type next_pos) const {
    AEX_ASSERT(check_lock_shared(node));
    if constexpr (!traits::AllowConcurrency)
        return;
    if (node->type == NodeType::HashNode)
        h_n(node)->array_unlock_shared(pos - 1, next_pos);                    
    SU(node);
    if (top_node != node && top_node != nullptr){                                          
        AEX_ASSERT(check_lock_shared(top_node));                        
        SU(top_node, top_pos - 1, top_next_pos);
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
    new_node->size = old_node->size - mid;
    old_node->size = mid;
}

}
