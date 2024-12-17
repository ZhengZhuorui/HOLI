namespace aex{

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::find(const hash_node_ptr node, const key_type &key) const {
    AEX_ASSERT(node->is_occupied(0));
    slot_type pos = node->predict(key), pos1 = -1;
    key_type find_key;
    node_ptr res = nullptr;
    node->array_lock_shared(pos - 1, pos);
    if (node->is_occupied(pos) || (pos & 63) == 0)
        std::tie(find_key, res) = hash_table.find(node, pos);
    if (res == nullptr || find_key > key){
        pos1 = node->prev_item_find(pos - 1);
        std::tie(find_key, res) = hash_table.find(node, pos1);
    }
    AEX_ASSERT(find_key <= key);
    AEX_ASSERT(res != nullptr);
    SL(res);
    node->array_unlock_shared(pos - 1, pos);
    return res;
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::find(const dense_node_ptr node, const key_type &key) const {
    //return node->child_ptr[aex:::lower_bound_with_error_bound(node->key_ptr, node->key_ptr + node->size, x) - node->key_ptr];
    slot_type pos = aex::linear_search_upper_bound(node->key_ptr + 1, node->key_ptr + node->size, key) - node->key_ptr - 1;
    node_ptr res = node->child_ptr[pos];
    SL(res);
    return res;
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::find(const inner_node_ptr node, const key_type &key) const {
    switch (node->type){
        case NodeType::HashNode  : { return find(h_n(node), key); }
        case NodeType::DenseNode : { return find(d_n(node), key); }
        default : { AEX_ASSERT(0 == 1); return nullptr;}
    }
}

template<typename _Key, typename _Val, typename traits>
inline void aex_tree<_Key, _Val, traits>::range_query(const key_type &lower_key, const key_type &upper_key, std::vector<std::pair<key_type, value_type>>& answer) const {
    SL();
    if constexpr(traits::AllowConcurrency){
        answer.clear();
        data_node_ptr inode = find_leaf_con(lower_key);
        slot_type i;
        while (inode != nullptr){
            for (i = 0; i < inode->size; ++i){
                if (inode->key[i] <= upper_key)
                    answer.emplace_back(inode->key[i], inode->data[i]);
            }
            if (inode->next == nullptr || i < inode->size) {
                SU(inode);
                break;
            }
            data_node_ptr next_node = inode->next;
            SL(next_node);
            SU(inode);
            inode = next_node;
        }
    }
    else{
        const_iterator iter = this->find_iterator(lower_key);
        while(iter.key() <= upper_key){
            answer.emplace_back(iter.key(), iter.data());
            ++iter;
        }
    }
    SU();
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::data_node_ptr aex_tree<_Key, _Val, traits>::find_leaf_con(const key_type &key) const {
    node_ptr node, child;
    version_type now_version;
find_leaf_con_start:
    node = root;
    now_version = this->version;
    SL(node);
    while(node->type != NodeType::LeafNode){
        child = find(i_n(node), key);
        SU(node);
        node = child;
    }
    if (l_n(node)->version > now_version){
        SU(node);
        goto find_leaf_con_start;
    }
    return l_n(node);
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::data_node_ptr aex_tree<_Key, _Val, traits>::find_leaf(const key_type &key) const {
    if constexpr (traits::AllowConcurrency)
        return find_leaf_con(key);
    node_ptr node = root;
    while (node->type != NodeType::LeafNode)
        node = find(i_n(node), key);
    return l_n(node);
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::find_update(hash_node_ptr node, const key_type &key, slot_type &pos, slot_type &next_pos) const {
    int restart_count = 0;
find_insert_start:
    if (restart_count++)
        yield(restart_count);
    bool restart = false;
    pos = node->predict(key);
    key_type find_key;
    node_ptr res = nullptr;
    node->array_lock_shared(pos - 1, pos);

    if (node->is_occupied(pos)){
        std::tie(find_key, res) = hash_table.find(node, pos);
        AEX_ASSERT(res != nullptr);
        if (find_key > key){
            next_pos = pos;
            res = nullptr;
        }
        else
            next_pos = node->array_lock_shared_until_next_item(pos, pos + 1);  
    }
    else
        next_pos = node->array_lock_shared_until_next_item(pos, pos + 1);

    if (res == nullptr){
        pos = node->try_array_lock_shared_until_prev_item(pos - 1, restart);
        if (restart){
            node->array_unlock_shared(pos - 1, next_pos);
            goto find_insert_start;
        }
        std::tie(find_key, res) = hash_table.find(node, pos);
        AEX_ASSERT(find_key <= key);
    }
    AEX_ASSERT(res != nullptr);
    AEX_DEBUG_BLOCK({if (res->type != NodeType::LeafNode) AEX_ASSERT(find_key <= node_zero_key(i_n(res)));});
    AEX_DEBUG_BLOCK({if (next_pos < node->slot_size){
        key_type next_key;
        node_ptr next_child;
        std::tie(next_key, next_child) = hash_table.find(node, next_pos);
        if (key >= next_key){
            AEX_PRINT("key=" << key << ", start=" << node->model.args.start << ", find_key=" << find_key << ", next_key=" << next_key << ", pos=" << pos << ", next_pos=" << next_pos << ", next_child=" << next_child);
            AEX_PRINT("predict=" << node->predict(key) << ", slope=" << node->model.args.slope << ", predict - 1 =" << (key - node->model.args.start) * node->model.args.slope << ", predict=" << (key - node->model.args.start) * node->model.args.slope + 1);
        }
        AEX_ASSERT(next_child != nullptr);
        AEX_ASSERT(key < next_key);}});
    AEX_DEBUG_BLOCK({if constexpr(traits::AllowConcurrency) if (pos - 1 >= 0) if (!node->lock_array[pos2slot(pos - 1)].is_lock_shared()) {
        AEX_ERROR("node=" << node << ", pos=" << pos);AEX_ASSERT(node->lock_array[pos2slot(pos - 1)].is_lock_shared());}});
    AEX_DEBUG_BLOCK({if constexpr(traits::AllowConcurrency) if (next_pos < node->slot_size) AEX_ASSERT(node->lock_array[pos2slot(next_pos)].is_lock_shared());});
    SL(res);
    return res;
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::find_update(dense_node_ptr node, const key_type &key, slot_type &pos, slot_type &next_pos) const {
    pos = aex::linear_search_upper_bound(node->key_ptr + 1, node->key_ptr + node->size, key) - node->key_ptr - 1;
    next_pos = pos + 1;
    node_ptr res = node->child_ptr[pos];
    SL(res);
    return res;
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::node_ptr aex_tree<_Key, _Val, traits>::find_update(inner_node_ptr node, const key_type &key, slot_type &pos, slot_type &next_pos) const {
    switch (node->type){
        case NodeType::HashNode  : { return find_update(h_n(node), key, pos, next_pos); }
        case NodeType::DenseNode : { return find_update(d_n(node), key, pos, next_pos); }
        default : { AEX_ASSERT(0 == 1); return nullptr; }
    }
}

template<typename _Key, typename _Val, typename traits>
inline typename aex_tree<_Key, _Val, traits>::data_node_ptr aex_tree<_Key, _Val, traits>::find_tail_leaf(node_ptr node) const {
    AEX_ASSERT(check_lock_shared(node));
    key_type key;
    node_ptr child;
    while (node->type != NodeType::LeafNode){
        if (node->type == NodeType::DenseNode){
            child = d_n(node)->child_ptr[d_n(node)->size - 1];
            SL(child);
        }
        else{
            h_n(node)->array_lock_shared(h_n(node)->slot_size - 1, h_n(node)->slot_size - 1);
            std::tie(key, child) = hash_table.find(node, h_n(node)->prev_item_find(h_n(node)->slot_size - 1));
            SL(child);
            h_n(node)->array_unlock_shared(h_n(node)->slot_size - 1, h_n(node)->slot_size - 1);
        }
        SU(node);
        node = child;
    }
    return l_n(node);
}


}