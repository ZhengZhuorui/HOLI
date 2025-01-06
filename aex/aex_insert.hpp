#pragma once
#include "aex/aex.h"

namespace aex{

template<typename _Key, typename _Val, typename traits>
inline std::pair<typename aex_tree<_Key, _Val, traits>::iterator, bool> aex_tree<_Key, _Val, traits>::insert(const key_type key, const value_type &value){
    //AEX_ASSERT(root->size == 0 || node_zero_key(root) == std::numeric_limits<key_type>::lowest());
    bool tail, flag;
    key_type split_key;
    hash_node_ptr top_node;
    inner_node_ptr node;
    node_ptr child;
    slot_type pos, top_pos = 0, split_pos;
    ULL now_version;
    int restart_count = 0;
    std::pair<iterator, bool> ret;
    //AEX_PRINT("root=" << root);
insert_start:
    if (restart_count++)
        yield(restart_count);
    now_version = this->version.load();
    top_node = nullptr;
    node = root;
    DEBUG_CHECK_UNLOCK(root);
    SL(node);    
    if (isfull(node)){
        if (!TUL(node)){
            SU(node); goto insert_start;
        }
        if (node->type == NodeType::HashNode)
            expand(h_n(node));
        else{
            if (!d_n(node)->is_parent || !expand(d_n(node))){
                dense_node_ptr new_node_0 = Allocator::allocate_dense_node();
                dense_node_ptr new_node_1 = Allocator::allocate_dense_node();
                split(d_n(node), new_node_1);
                new_node_0->size = node->size;
                std::move(d_n(node)->key_ptr,   d_n(node)->key_ptr   + node->size, new_node_0->key_ptr);
                std::move(d_n(node)->child_ptr, d_n(node)->child_ptr + node->size, new_node_0->child_ptr);
                node->size = 2;
                d_n(node)->key_ptr[0] = new_node_0->key_ptr[0]; d_n(node)->child_ptr[0] = new_node_0;
                d_n(node)->key_ptr[1] = new_node_1->key_ptr[0]; d_n(node)->child_ptr[1] = new_node_1;
                d_n(node)->is_parent = true;
            }
        }
        DL(node);
    }

    while (true){
        child = find_insert(node, key, pos);
        if constexpr (traits::AllowConcurrency)
            if (now_version < child->version){
                SU(child); insert_unlock(top_node, node);
                goto insert_start;
            }
        //AEX_PRINT("node=" << node << ", key=" << key << ", pos=" << pos << ", node->size=" << node->size << ", child->type=" << to_string(child->type) << ", child=" << child << ", child->size=" << child->size);
        tail = (node->type == NodeType::HashNode) ? (last_node(h_n(node)) == child) : (pos == d_n(node)->size - 1);
        if constexpr (traits::AllowRebuild)
            if ((tail || pos == 0) && (child->type != NodeType::LeafNode && node->type == NodeType::HashNode && child->size >= node->size * traits::MIN_REBUILD_RATIO)){
                //AEX_PRINT("node=" << node << ", key=" << key << ", pos=" << pos << ", node->size=" << node->size << ", slot_size=" << node->slot_size << ", child->type=" << to_string(child->type) << ", child=" << child << ", child->size=" << child->size);
                if (!TUL(node)){
                    SU(child); insert_unlock(top_node, node); goto insert_start;
                }
                SU(child);
                if (top_node != node && top_node != nullptr)
                    SU(top_node);
                rebuild(node);
                XU(node);
                goto insert_start;
            }
        if (!tail){
            if (top_node != nullptr){
                SU(top_node);
                top_node = nullptr;
            }
            if (node->type == NodeType::HashNode){
                top_node = h_n(node);
                top_pos = pos;
            }
        }
        if (child->type == NodeType::LeafNode){
            slot_type child_pos = l_n(child)->find_lower_pos(key);
            AEX_DEBUG_BLOCK({if (l_n(child)->next != nullptr)if (l_n(child)->next->key[0] < key){
                AEX_ERROR("key=" << key << ", " << l_n(child)->key[child->size - 1] << ", " << l_n(child)->next->key[0]);
                AEX_ERROR("now_version=" << now_version << ", " << l_n(child)->version << ", " << l_n(child)->next->version);
                AEX_ERROR("node->type=" << to_string(node->type) << ", node->version=" << node->version);
                AEX_ASSERT(0 == 1);
            }});
            if (!traits::AllowMultiKey && pos < l_n(child)->size && l_n(child)->key[pos] == key){
                ret = std::make_pair(iterator(l_n(child), child_pos), false);
                SU(child); insert_unlock(top_node, node);
                return ret;
            }
            if (isfull(l_n(child))){
                split_key = l_n(child)->key[traits::DATA_NODE_SLOT_SIZE >> 1];
                if (top_node != nullptr && top_node != node){
                    if (check_upgrade(top_node, split_key, top_pos)){
                        SU(node);
                        node = top_node;
                        pos = top_pos;
                    }
                    else{
                        SU(top_node);
                        top_node = nullptr;
                    }
                }
                data_node_ptr new_node;
                if (node->type == NodeType::HashNode){
                    split_pos = h_n(node)->predict(split_key);
                    if (!TUL(child)){
                        SU(child); SU(node);
                        goto insert_start;
                    }
                    ret = insert_data_node(l_n(child), new_node, key, value);
                    if (!h_n(node)->is_occupied_con(split_pos) && split_pos < h_n(node)->slot_size)
                        insert_no_collision(h_n(node), split_pos, split_key, new_node);
                    else
                        insert_collision(h_n(node), pos, split_key, new_node);                    
                    add_version(child, new_node);
                    XU(child); SU(node);
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
                    add_version(child, new_node);
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
        
        if (isfull(child)){
            if (child->type == NodeType::HashNode){
                if (!TUL(child)){
                    SU(child); insert_unlock(top_node, node);
                    goto insert_start;
                }
                expand(h_n(child));
                DL(child);
            }
            else{
                AEX_ASSERT(child->type == NodeType::DenseNode);
                flag = false;
                if (d_n(child)->is_parent){
                    if (!TUL(child)){
                        SU(child); insert_unlock(top_node, node);
                        goto insert_start;
                    }
                    if (!expand(d_n(child)))
                        d_n(child)->is_parent = false;
                    else
                        flag = true;
                    DL(child);
                }
                if (!flag){
                    split_key = d_n(child)->key_ptr[traits::DENSE_NODE_SLOT_SIZE / 2];
                    if (top_node != nullptr && top_node != node){
                        if (check_upgrade(top_node, split_key, top_pos)){
                            SU(node);
                            node = top_node;
                            pos = top_pos;
                        }
                        else{
                            SU(top_node);
                            top_node = nullptr;
                        }
                    }
                    dense_node_ptr new_node;
                    if (node->type == NodeType::HashNode){
                        if (!TUL(child)){
                            SU(child); SU(node);
                            goto insert_start;
                        }
                        split_pos = h_n(node)->predict(split_key);
                        new_node = Allocator::allocate_dense_node();
                        split(d_n(child), new_node);
                        if (!h_n(node)->is_occupied_con(split_pos) && split_pos < h_n(node)->slot_size)
                            insert_no_collision(h_n(node), split_pos, split_key, new_node);
                        else
                            insert_collision(h_n(node), pos, split_key, new_node);
                        add_version(child, new_node);
                        XU(child); SU(node);
                        goto insert_start;
                    }
                    else{
                        AEX_ASSERT(node->type == NodeType::DenseNode);
                        if (!TUL(node)){
                            SU(child);SU(node);
                            goto insert_start;
                        }
                        if (!TUL(child)){
                            SU(child);XU(node);
                            goto insert_start;
                        }
                        new_node = Allocator::allocate_dense_node();
                        split(d_n(child), new_node);
                        insert(d_n(node), split_key, new_node);
                        add_version(child, new_node);
                        XU(child); XU(node);
                        goto insert_start;
                    }
                }
            }
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
    node->is_parent = ((old_node->type == NodeType::DenseNode) & (new_node->type == NodeType::HashNode) );
    node->size = 2;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::__construct_insert(hash_node_ptr node, const slot_type pos, const slot_type next_pos, const key_type key, const node_ptr child){
    AEX_ASSERT(node->type == NodeType::HashNode);
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
inline void aex_tree<_Key, _Val, traits>::insert_collision(hash_node_ptr node, const slot_type pos, const key_type key, const node_ptr child){
    AEX_ASSERT(node->type == NodeType::HashNode);
    AEX_ASSERT(check_lock_shared(node));
    AEX_ASSERT(pos < node->slot_size);
    #ifdef AEX_DEBUG
    opt_stats.allocate_dense_node_cnt++;
    #endif
    dense_node_ptr new_node = Allocator::allocate_dense_node();
    key_type prev_key;
    node_ptr prev_child;
    std::tie(prev_key, prev_child) = hash_table.find(node, pos);
    construct_tmp_node(new_node, prev_key, prev_child, key, child);
    AEX_ASSERT(prev_child->type == child->type);
    hash_table.update(node, pos, new_node->key_ptr[0], new_node);
    slot_type next_pos = node->next_item_con(pos + 1);
    for (slot_type i = highbit<slot_type, traits::SLOT_PER_SHORTCUT>(pos + 1); i < next_pos; i += traits::SLOT_PER_SHORTCUT)
        hash_table.update(node, i, new_node->key_ptr[0], new_node);
    SU(node, next_pos);
}


template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::insert_no_collision(hash_node_ptr node, const slot_type pos, const key_type key, const node_ptr child){
    AEX_ASSERT(node->type == NodeType::HashNode);
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
    AEX_ASSERT(node->type == NodeType::DenseNode);
    const slot_type pos = std::upper_bound(node->key_ptr + 1, node->key_ptr + node->size, key) - node->key_ptr;
    std::move_backward(node->key_ptr   + pos, node->key_ptr   + node->size, node->key_ptr   + node->size + 1);
    std::move_backward(node->child_ptr + pos, node->child_ptr + node->size, node->child_ptr + node->size + 1);
    node->key_ptr[pos]   = key;
    node->child_ptr[pos] = child;
    ++node->size;
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
    new_node = new data_node(this->version.load());
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
    AEX_ASSERT(old_node->size == traits::DATA_NODE_SLOT_SIZE);
    AEX_ASSERT(check_lock(old_node));
    #ifdef AEX_DEBUG
    ++opt_stats.data_node_split_cnt;
    #endif
    
    new_node->next = old_node->next;
    old_node->next = new_node;    
    
    const ULL mid = traits::DATA_NODE_SLOT_SIZE / 2;
    std::move(old_node->key  + mid, old_node->key  + old_node->size, new_node->key );
    std::move(old_node->data + mid, old_node->data + old_node->size, new_node->data);
    
    new_node->size = old_node->size - mid;
    old_node->size = mid;
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::split(dense_node_ptr old_node, dense_node_ptr new_node){
    AEX_ASSERT(old_node->size == traits::DENSE_NODE_SLOT_SIZE);
    AEX_ASSERT(check_lock(old_node));
    #ifdef AEX_DEBUG
    ++opt_stats.dense_node_split_cnt;
    #endif
    
    const size_type mid = traits::DENSE_NODE_SLOT_SIZE / 2;
    std::move(old_node->key_ptr   + mid, old_node->key_ptr   + old_node->size, new_node->key_ptr  );
    std::move(old_node->child_ptr + mid, old_node->child_ptr + old_node->size, new_node->child_ptr);
    
    new_node->size = old_node->size - mid;
    old_node->size = mid;
}

}
