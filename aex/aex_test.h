#pragma once
namespace aex{

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::check_lock(node_ptr node) const {
    if constexpr (traits::AllowConcurrency)
        return node->node_lock.is_lock();
    else
        return true;
}
template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::check_lock_shared(node_ptr node) const {
    if constexpr (traits::AllowConcurrency)
        return node->node_lock.is_lock_shared();
    else
        return true;
}
template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::check_unlock(node_ptr node) const {
    if constexpr (traits::AllowConcurrency)
        return !node->node_lock.is_lock();
    else
        return true;
}
template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::check_unlock_shared(node_ptr node) const {
    if constexpr (traits::AllowConcurrency)
        return !node->node_lock.is_lock_shared();
    else
        return true;
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::check_node(node_ptr node) const {
    switch (node->type){
        case NodeType::LeafNode : { return check_node(l_n(node));}
        case NodeType::DenseNode: { return check_node(d_n(node));}
        case NodeType::HashNode : { return check_node(h_n(node));}
        default: { AEX_ERROR("Unknown Type"); return false; }
    }
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::check_node(data_node_ptr node) const {
    bool flag = true;
    //if (node->size == 0){
    //    AEX_ERROR("ERROR! size=0");
    //    flag = false;
    //}
    if constexpr (!traits::AllowUnsorted)
    for (slot_type i = 0; i < node->size - 1; ++i)
    if (node->key[i] > node->key[i + 1]){
        AEX_ERROR("ERROR! key[" << i << "]=" << node->key[i] << ", key[i+1]=" << node->key[i + 1] << ", size=" << node->size);
        flag = false;
    }
    return flag;
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::check_node(dense_node_ptr node) const {
    bool flag = true;
    if (node->size > traits::DENSE_NODE_SLOT_SIZE){
        AEX_ERROR("ERROR! dense node size larger than threshold, size=" << node->size);
        flag = false;
    }
    if  (node != root && (node->size == 0 || node->size == 1)){
        AEX_ERROR("ERROR! size=" << node->size);
        flag = false;
    }
    for (slot_type i = 0; i < node->size - 1; ++i){
        if (node->key_ptr[i] > node->key_ptr[i + 1]){
            AEX_ERROR("ERROR! key[" << i << "]=" << node->key_ptr[i] << ", key[i+1]=" << node->key_ptr[i + 1] << ", size=" << node->size);
            flag = false;
        }
        if (node->child_ptr[i] == node->child_ptr[i + 1]){
            AEX_ERROR("ERROR! child pointer is same");
            flag = false;
        }
    }
    return flag;
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::check_node(hash_node_ptr node) const {
    bool flag = true;
    if (node->size < traits::MIN_HASH_NODE_CNT){
        AEX_ERROR("ERROR! hash node size=" << node->size << ", slot size=" << node->slot_size);
        flag = false;
    }
    key_type his_key = std::numeric_limits<key_type>::lowest();
    slot_type cnt = 0;
    for(slot_type i = 0; i < node->slot_size; i = node->next_item(i + 1)){
        key_type key;
        node_ptr child;
        std::tie(key, child) = hash_table.find(node, i);
        if (child == nullptr){
            AEX_ERROR("child == nullptr. slot=" << i << ", slot & 63=" << (i & 63));
            flag = false;
        }
        if (his_key > key){
            AEX_ERROR("key is not unordered. his_key=" << his_key << ", key=" << key);
            flag = false;
        }
        ++cnt;
        his_key = key;
    }
    if (cnt != node->size){
        AEX_ERROR("cnt != node->size. cnt=" << cnt << ", node->size=" << node->size);
        flag = false;
    }
    AEX_DEBUG_BLOCK({
        if constexpr(traits::AllowConcurrency) 
            for (slot_type i = 0; i < pos2slot(node->slot_size); ++i) {
                //if (node->lock_array[i].lockCount.load() != 0)
                //    AEX_ERROR("node=" << node << ", i=" << i << ", " << node->lock_array[i].lockCount.load());
                AEX_ASSERT(!node->lock_array[i].is_lock());
                AEX_ASSERT(!node->lock_array[i].is_lock_shared());
                //AEX_ASSERT(!node->update_lock_array[i].is_lock());
            }
    });
    return flag;
}


template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::check_insert_root(inner_node_ptr node){
    AEX_XL_WAIT_CNT(node);
    if (!TUL(node)){
        SU(node); return false;
    }
    if (node->type == NodeType::HashNode)
        expand(h_n(node));
    else{
        if (!d_n(node)->is_parent || !expand(d_n(node))){
            AEX_ASSERT(node->size == traits::DENSE_NODE_SLOT_SIZE);
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
    return true;
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::check_insert_data_node(hash_node_ptr &top_node, inner_node_ptr &node, node_ptr &child, slot_type pos, const key_type key, const value_type &value){
    bool top_flag;
    slot_type split_pos;
    key_type split_key;

    if (isfull(l_n(child))){
        if constexpr (traits::AllowUnsorted)
            if (!l_n(child)->is_sorted) l_n(child)->sort();
        top_flag = false;
        split_key = l_n(child)->key[traits::DATA_NODE_SLOT_SIZE / 2];
        if (top_node != nullptr && top_node != node){
            split_pos = top_node->predict(split_key);
            if (split_pos < top_node->slot_size && !top_node->is_occupied(split_pos)){
                top_flag = true;
                SU(node);
                node = top_node;
            }
            else SU(top_node);
        }
        data_node_ptr new_node;
        if (node->type == NodeType::HashNode){
            if (!top_flag)
                split_pos = h_n(node)->predict(split_key);
            AEX_XL_WAIT_CNT(child);
            if (!TUL(child)){ SU(child); SU(node); return false; }
            if (!h_n(node)->is_occupied(split_pos) && split_pos < h_n(node)->slot_size){
                insert_data_node(l_n(child), new_node, key, value);
                AEX_ASSERT(new_node->key[0] == split_key);
                insert_no_collision(h_n(node), split_pos, split_key, new_node);
            }
            else{
                if constexpr(traits::AllowConcurrency){
                    if (top_flag){ XU(child); SU(node); return false; }
                }
                insert_data_node(l_n(child), new_node, key, value);
                AEX_ASSERT(new_node->key[0] == split_key);
                pos = h_n(node)->prev_item_con(pos);
                AEX_DEBUG_BLOCK({if constexpr (traits::AllowConcurrency) AEX_ASSERT(h_n(node)->lock_array[pos2slot(pos)].is_lock_shared());});
                SU(h_n(node), pos);
                insert_collision(h_n(node), pos, split_key, new_node);
            }
            add_version(child, new_node);
            XU(child); SU(node);
        }
        else{
            AEX_XL_WAIT_CNT(node);
            if (!TUL(node)){  SU(child); SU(node); return false; }
            AEX_XL_WAIT_CNT(child);
            if (!TUL(child)){ SU(child); XU(node); return false; }
            insert_data_node(l_n(child), new_node, key, value);
            insert(d_n(node), split_key, new_node);
            add_version(child, new_node);
            XU(child); XU(node);
        }
    }
    else{
        AEX_XL_WAIT_CNT(child);
        if (!TUL(child)){ SU(child); insert_unlock(top_node, node); return false; }
        l_n(child)->insert(key, value);
        XU(child); insert_unlock(top_node, node);
    }
    ++this->m_stats.size;
    return true;
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::check_insert_SMO(hash_node_ptr &top_node, inner_node_ptr &node, node_ptr &child, slot_type &pos, const key_type key, version_type now_version){
    bool flag, top_flag, collision_flag;
    key_type split_key;
    slot_type split_pos;
    data_node_ptr tail_leaf;
    if (child->type == NodeType::HashNode){
        AEX_XL_WAIT_CNT(child);
        if (!TUL(child)){ SU(child); insert_unlock(top_node, node); return false; }
        expand(h_n(child));
        DL(child);
    }
    else{
        AEX_ASSERT(child->type == NodeType::DenseNode);
        flag = false;
        if (d_n(child)->is_parent){
            AEX_XL_WAIT_CNT(child);
            if (!TUL(child)){
                SU(child); insert_unlock(top_node, node);
                return false;
            }
            if (!expand(d_n(child)))
                d_n(child)->is_parent = false;
            else
                flag = true;
            DL(child);
        }
        if (!flag){
            split_key = d_n(child)->key_ptr[traits::DENSE_NODE_SLOT_SIZE / 2];
            flag = (key >= split_key);
            top_flag = false;
            if (top_node != nullptr && top_node != node){
                split_pos = top_node->predict(split_key);
                if (split_pos < top_node->slot_size && !top_node->is_occupied(split_pos)){
                    top_flag = true;
                    SU(node);
                    node = top_node;
                }
                else if (!flag){
                    SU(top_node);
                    top_node = nullptr;
                }
            }
            dense_node_ptr new_node;
            if (node->type == NodeType::HashNode){
                if (!top_flag) split_pos = h_n(node)->predict(split_key);  
                AEX_XL_WAIT_CNT(child);
                if (!TUL(child)){ SU(child); SU(node); return false; }
                collision_flag = true;
                if (!h_n(node)->is_occupied(split_pos) && split_pos < h_n(node)->slot_size){
                    AEX_XL_WAIT_CNT(d_n(child)->child_ptr[traits::DENSE_NODE_SLOT_SIZE - 1]);
                    XL(d_n(child)->child_ptr[traits::DENSE_NODE_SLOT_SIZE - 1]);
                    tail_leaf = find_tail_leaf(d_n(child)->child_ptr[traits::DENSE_NODE_SLOT_SIZE - 1], now_version);
                    if (tail_leaf == nullptr){ XU(child); SU(node); return false; }
                    if (!h_n(node)->is_occupied(split_pos) && split_pos < h_n(node)->slot_size) collision_flag = false;
                    else{
                        if (top_flag){ 
                            XU(tail_leaf); XU(child); SU(node); return false;
                        }
                    }
                }
                new_node = Allocator::allocate_dense_node();
                split(d_n(child), new_node);
                if (!collision_flag){
                    insert_no_collision(h_n(node), split_pos, split_key, new_node);
                    if constexpr (traits::AllowConcurrency)
                        XU(tail_leaf);
                }
                else {
                    pos = h_n(node)->prev_item_con(pos);
                    AEX_DEBUG_BLOCK({if constexpr (traits::AllowConcurrency) AEX_ASSERT(h_n(node)->lock_array[pos2slot(pos)].is_lock_shared());});
                    SU(h_n(node), pos);
                    insert_collision(h_n(node), pos, split_key, new_node);
                }
                add_version(child, new_node);
                AEX_ASSERT(new_node->key_ptr[0] == split_key);
                if (flag){ AEX_SL_WAIT_CNT(new_node); SL(new_node); XU(child); child = new_node; }
                else{ if (collision_flag) { AEX_ASSERT(top_flag == false); top_node = nullptr;} DL(child); }
            }
            else{
                AEX_ASSERT(node->type == NodeType::DenseNode);
                AEX_XL_WAIT_CNT(node);
                if (!TUL(node)) { SU(child); SU(node); return false; }
                AEX_XL_WAIT_CNT(child);
                if (!TUL(child)){ SU(child); XU(node); return false; }
                new_node = Allocator::allocate_dense_node();
                split(d_n(child), new_node);
                insert(d_n(node), split_key, new_node);
                add_version(child, new_node);
                AEX_ASSERT(new_node->key_ptr[0] == split_key);
                if (flag){ AEX_SL_WAIT_CNT(new_node); SL(new_node); XU(child); child = new_node; }
                else { top_node = nullptr; DL(child);}
                DL(node);
            }
            AEX_ASSERT(d_n(child)->key_ptr[0] <= key);
        }
    }
    return true;
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::_insert_con_debug(const key_type key, const value_type &value){
    //AEX_ASSERT(root->size == 0 || node_zero_key(root) == std::numeric_limits<key_type>::lowest());
    bool tail;
    hash_node_ptr top_node;
    inner_node_ptr node;
    node_ptr child;
    slot_type pos;
    ULL now_version;
    int restart_count = 0;
    AEX_DEBUG_BLOCK({--con_stats.insert_restart_cnt;});
insert_start:
    if (restart_count > 0)
        yield(restart_count);
    restart_count++;
    AEX_DEBUG_BLOCK({++con_stats.insert_restart_cnt;});
    now_version = this->version.load();
    top_node = nullptr;
    node = root;
    AEX_SL_WAIT_CNT(node);
    SL(node);    
    if (isfull(node)){
        if (!check_insert_root(node))
            goto insert_start;
    }

    while (true){
        child = find_insert(node, key, pos);
        if constexpr (traits::AllowConcurrency)
            if (now_version < child->version){
                SU(child); insert_unlock(top_node, node);
                goto insert_start;
            }
        tail = (node->type == NodeType::HashNode) ? (last_node(h_n(node)) == child) : (pos == d_n(node)->size - 1);
        
        if (!tail){
            if (top_node != nullptr){
                SU(top_node);
                top_node = nullptr;
            }
            if (node->type == NodeType::HashNode)
                top_node = h_n(node);
        }
        if (child->type == NodeType::LeafNode){
            if (!traits::AllowMultiKey && l_n(child)->find(key) > child->size){
                SU(child); insert_unlock(top_node, node);
                return false;
            }
            if (!check_insert_data_node(top_node, node, child, pos, key, value)){
                goto insert_start;
            }
            return true;
        }
        
        if (isfull(child)){
            if (!check_insert_SMO(top_node, node, child, pos, key, now_version))
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
    return true;
}

template<typename _Key, typename _Val, typename traits>
inline bool aex_tree<_Key, _Val, traits>::_insert_con_debug_test(const key_type key, const value_type value){
    int x = key % 10240;
    //AEX_PRINT("1");
    //test_vec[x][y] = std::make_pair(key, value);
    
    int restart_count = 0;
insert_start:
    if (restart_count > 0)
        yield(restart_count);
    restart_count++;
    bool restart = false;
    //test_lock[x].writeLockOrRestart(restart);
    test_lock[x].lock();
    if (restart){
        //AEX_DEBUG_BLOCK({++opt_stats.XL_wait_cnt;});
        //++con_stats.XL_wait_cnt;
        goto insert_start;
    }
    test_vec[x].push_back(std::make_pair(key, value));
    test_lock[x].unlock();
    //test_lock[x].writeUnlock();
    
    return true;
}

}